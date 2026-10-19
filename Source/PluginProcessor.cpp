#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

ChordMateProcessor::ChordMateProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    for (auto& h : hist) h.store (0.0f);

    for (int i = 0; i < 16; ++i) synth.addVoice (new PreviewVoice());
    synth.addSound (new PreviewSound());

    project.applyPreset();       // si parte con una progressione d'esempio (I-V-vi-IV in Do)
    startTimerHz (20);
}

ChordMateProcessor::~ChordMateProcessor()
{
    stopTimer();
}

bool ChordMateProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChordMateProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr > 0.0 ? sr : 44100.0;
    synth.setCurrentPlaybackSampleRate (sr);
    synthMidi.ensureSize (4096);
}

juce::AudioProcessorEditor* ChordMateProcessor::createEditor()
{
    return new ChordMateEditor (*this);
}

// ------------------------------------------------------------------ audio / MIDI
void ChordMateProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // informazioni dall'host
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
        }
    if (bpm < 20.0 || bpm > 400.0) bpm = 120.0;
    bpmAtomic.store (bpm);

    const double beatsPerSample = bpm / (60.0 * sampleRate);
    const bool rec = recording.load();
    if (rec && ! wasRecording)
    {
        recUseHost = playing && havePpq;
        recStartBeat = recUseHost ? ppq : internalBeat;
    }
    wasRecording = rec;
    const double blockBeat = (recUseHost && havePpq) ? ppq : internalBeat;

    // analisi delle note in ingresso + cattura
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            const int p = msg.getNoteNumber();
            held[(size_t) p] = true;
            hist[(size_t) (p % 12)].store (hist[(size_t) (p % 12)].load() + msg.getFloatVelocity());
            if (rec)
            {
                int s1, n1, s2, n2;
                capFifo.prepareToWrite (1, s1, n1, s2, n2);
                if (n1 > 0)
                {
                    capBuf[(size_t) s1] = { blockBeat + metadata.samplePosition * beatsPerSample - recStartBeat,
                                            (uint8_t) p, (uint8_t) msg.getVelocity(), true };
                    capFifo.finishedWrite (1);
                }
            }
        }
        else if (msg.isNoteOff())
        {
            const int p = msg.getNoteNumber();
            held[(size_t) p] = false;
            if (rec)
            {
                int s1, n1, s2, n2;
                capFifo.prepareToWrite (1, s1, n1, s2, n2);
                if (n1 > 0)
                {
                    capBuf[(size_t) s1] = { blockBeat + metadata.samplePosition * beatsPerSample - recStartBeat,
                                            (uint8_t) p, 0, false };
                    capFifo.finishedWrite (1);
                }
            }
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            held.fill (false);
    }
    internalBeat += n * beatsPerSample;

    uint16_t mask = 0;
    int lowest = -1;
    for (int p = 0; p < 128; ++p)
        if (held[(size_t) p])
        {
            mask = (uint16_t) (mask | (1u << (p % 12)));
            if (lowest < 0) lowest = p % 12;
        }
    heldMask.store (mask);
    lowestHeldPc.store (lowest);

    const float decay = (float) std::exp (-(double) n / (sampleRate * 20.0));   // memoria ~20 s
    for (auto& h : hist) h.store (h.load() * decay);

    // anteprima + (opzionale) suono del MIDI in ingresso
    synthMidi.clear();
    if (cutRequested.exchange (false))
        synth.allNotesOff (0, true);
    if (playThrough.load())
        synthMidi.addEvents (midi, 0, n, 0);

    if (previewActive.load())
    {
        const juce::SpinLock::ScopedTryLockType lock (previewLock);
        if (lock.isLocked() && previewActive.load())
            renderPreview (n, bpm);
    }

    synth.renderNextBlock (buffer, synthMidi, 0, n);
    buffer.applyGain (volume.load());
    midi.clear();
}

void ChordMateProcessor::renderPreview (int numSamples, double bpm)
{
    if (previewEvents.empty()) { previewActive.store (false); return; }

    const double tps = bpm * cm::kPPQ / (60.0 * sampleRate);     // tick per campione
    double pos = previewTicks;
    double remaining = numSamples * tps;
    double sampleBase = 0.0;
    const double total = (double) previewTotal;

    for (int guard = 0; guard < 8 && remaining > 1e-9; ++guard)
    {
        const double end = std::min (pos + remaining, total);
        const bool reachesEnd = end >= total - 1e-9;

        auto offsetOf = [&] (int tick)
        {
            return juce::jlimit (0, numSamples - 1, (int) (sampleBase + (tick - pos) / tps));
        };

        // prima i note-off, poi i note-on (stesso istante: la nota ripetuta riparte pulita)
        for (const auto& ev : previewEvents)
            if (ev.endTick >= pos && (ev.endTick < end || (reachesEnd && ev.endTick <= end + 1e-9)))
                synthMidi.addEvent (juce::MidiMessage::noteOff (ev.channel, ev.pitch), offsetOf (ev.endTick));

        for (const auto& ev : previewEvents)
            if (ev.startTick >= pos && ev.startTick < end)
                synthMidi.addEvent (juce::MidiMessage::noteOn (ev.channel, ev.pitch, (juce::uint8) ev.velocity),
                                    offsetOf (ev.startTick));

        const double used = end - pos;
        sampleBase += used / tps;
        remaining -= used;
        pos = end;

        if (reachesEnd)
        {
            pos = 0.0;
            if (! previewLoop)
            {
                previewActive.store (false);
                break;
            }
        }
    }

    previewTicks = pos;
    previewPosTick.store ((int) pos);
}

// ------------------------------------------------------------------ anteprima (thread messaggi)
std::vector<ChordMateProcessor::PreviewEvent> ChordMateProcessor::toEvents (const std::vector<cm::Note>& notes, int channel)
{
    std::vector<PreviewEvent> out;
    out.reserve (notes.size());
    for (const auto& n : notes)
        out.push_back ({ n.startTick, n.startTick + std::max (1, n.lengthTick), n.pitch, n.velocity, channel });
    return out;
}

void ChordMateProcessor::playEvents (std::vector<PreviewEvent> events, int totalTicks, bool loop)
{
    std::stable_sort (events.begin(), events.end(), [] (const PreviewEvent& a, const PreviewEvent& b)
                      { return a.startTick < b.startTick; });
    const bool any = ! events.empty();
    {
        const juce::SpinLock::ScopedLockType lock (previewLock);
        previewEvents = std::move (events);
        previewTotal = std::max (1, totalTicks);
        previewLoop = loop;
        previewTicks = 0.0;
    }
    previewPosTick.store (0);
    cutRequested.store (true);
    previewActive.store (any);
}

void ChordMateProcessor::playAll()
{
    auto ev = toEvents (project.chordNotes(), 1);
    auto mel = toEvents (project.melody, 2);
    ev.insert (ev.end(), mel.begin(), mel.end());

    int total = project.lengthTicks();
    for (const auto& e : ev) total = std::max (total, e.endTick);
    playEvents (std::move (ev), total, true);
}

void ChordMateProcessor::playCapture()
{
    const auto notes = project.processedCapture();
    playEvents (toEvents (notes, 3), cm::Project::captureLengthTicks (notes), false);
}

void ChordMateProcessor::auditionChord (const cm::Chord& c)
{
    std::vector<cm::Note> notes;
    for (int p : cm::voiceChord (c, nullptr))
        notes.push_back ({ p, 0, cm::kPPQ * 2, 90 });
    notes.push_back ({ 36 + c.root, 0, cm::kPPQ * 2, 94 });
    playEvents (toEvents (notes, 1), cm::kPPQ * 2 + cm::kPPQ / 2, false);
}

void ChordMateProcessor::stopPreview()
{
    previewActive.store (false);
    cutRequested.store (true);
    previewPosTick.store (0);
}

// ------------------------------------------------------------------ analisi e cattura
std::array<double, 12> ChordMateProcessor::pitchClassHistogram() const
{
    std::array<double, 12> out {};
    for (size_t i = 0; i < 12; ++i) out[i] = (double) hist[i].load();
    return out;
}

void ChordMateProcessor::resetHistogram()
{
    // il thread audio è l'unico scrittore, ma l'azzeramento da qui è innocuo (store atomico)
    for (auto& h : hist) h.store (0.0f);
}

bool ChordMateProcessor::pullCapturedNotes()
{
    bool changed = false;
    int s1, n1, s2, n2;
    capFifo.prepareToRead (capFifo.getNumReady(), s1, n1, s2, n2);

    auto handle = [&] (const CapEvent& e)
    {
        if (e.on)
        {
            pending[e.pitch] = { e.beat, e.velocity };
            return;
        }
        auto it = pending.find (e.pitch);
        if (it == pending.end()) return;
        const double start = std::max (0.0, it->second.beat);
        const double end = std::max (start, e.beat);
        if (project.capture.size() < (size_t) cm::kMaxStoredNotes)
            project.capture.push_back ({ e.pitch, (int) std::lround (start * cm::kPPQ),
                                         std::max (30, (int) std::lround ((end - start) * cm::kPPQ)),
                                         std::max (1, it->second.velocity) });
        pending.erase (it);
        changed = true;
    };

    for (int i = 0; i < n1; ++i) handle (capBuf[(size_t) (s1 + i)]);
    for (int i = 0; i < n2; ++i) handle (capBuf[(size_t) (s2 + i)]);
    capFifo.finishedRead (n1 + n2);

    if (! recording.load() && ! pending.empty())
    {
        for (auto& kv : pending)
            project.capture.push_back ({ kv.first, (int) std::lround (std::max (0.0, kv.second.beat) * cm::kPPQ),
                                         cm::kStepTicks, std::max (1, kv.second.velocity) });
        pending.clear();
        changed = true;
    }

    if (changed)
    {
        std::stable_sort (project.capture.begin(), project.capture.end(), [] (const cm::Note& a, const cm::Note& b)
                          { return a.startTick != b.startTick ? a.startTick < b.startTick : a.pitch < b.pitch; });
        captureRevision.fetch_add (1);
    }
    return changed;
}

void ChordMateProcessor::clearCapture()
{
    pending.clear();
    project.capture.clear();
    captureRevision.fetch_add (1);
}

// ------------------------------------------------------------------ stato
void ChordMateProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    project.volume = (double) volume.load();
    project.uiScale = (double) uiScale.load();
    const auto s = project.serialize();
    dest.append (s.data(), s.size());
}

void ChordMateProcessor::setStateInformation (const void* data, int size)
{
    if (data == nullptr || size <= 0) return;
    const std::string s ((const char*) data, (size_t) size);
    if (project.deserialize (s))
    {
        volume.store ((float) project.volume);
        uiScale.store ((float) project.uiScale);
        captureRevision.fetch_add (1);
        projectRevision.fetch_add (1);
    }
}

// ------------------------------------------------------------------ entry point JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordMateProcessor();
}
