#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "Engine/Project.h"
#include "PreviewSynth.h"

#include <array>
#include <atomic>
#include <map>
#include <vector>

// ChordMate: strumento di assistenza all'arrangiamento.
// In Live 12 i plugin VST non possono emettere MIDI verso altre tracce, quindi il flusso di lavoro è:
//  - il plugin ASCOLTA il MIDI in ingresso (routing "MIDI From" di una traccia) per riconoscere
//    accordo e tonalità e per registrare una "cattura";
//  - genera accordi e melodie e li ESPORTA come file .mid da trascinare su una traccia MIDI di Live.
class ChordMateProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    struct PreviewEvent { int startTick, endTick, pitch, velocity, channel; };

    ChordMateProcessor();
    ~ChordMateProcessor() override;

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ChordMate"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- stato del lavoro (solo thread dei messaggi)
    cm::Project project;
    std::atomic<int> projectRevision { 0 };      // incrementato quando l'host ricarica lo stato

    // --- anteprima audio
    void playAll();                              // accordi + melodia in loop
    void playCapture();
    void auditionChord (const cm::Chord& c);
    void stopPreview();
    bool isPreviewPlaying() const { return previewActive.load(); }
    int previewPositionTick() const { return previewPosTick.load(); }

    // --- analisi del MIDI in ingresso
    uint16_t heldPitchClassMask() const { return heldMask.load(); }
    int lowestHeldPitchClass() const { return lowestHeldPc.load(); }
    std::array<double, 12> pitchClassHistogram() const;
    void resetHistogram();
    double hostBpm() const { return bpmAtomic.load(); }

    // --- cattura MIDI
    void setRecording (bool on) { recording = on; }
    bool isRecording() const { return recording.load(); }
    bool pullCapturedNotes();                    // true se la cattura è cambiata (solo thread dei messaggi)
    void clearCapture();
    std::atomic<int> captureRevision { 0 };      // incrementato a ogni modifica della cattura

    std::atomic<float> volume { 0.7f };
    std::atomic<bool> playThrough { false };
    std::atomic<float> uiScale { 1.0f };         // scala della finestra dell'editor (salvata nel progetto)

private:
    void timerCallback() override { pullCapturedNotes(); }   // la cattura funziona anche a finestra chiusa

    struct CapEvent { double beat; uint8_t pitch, velocity; bool on; };
    struct Pending { double beat; int velocity; };

    void playEvents (std::vector<PreviewEvent> events, int totalTicks, bool loop);
    void renderPreview (int numSamples, double bpm);
    static std::vector<PreviewEvent> toEvents (const std::vector<cm::Note>& notes, int channel);

    juce::Synthesiser synth;
    juce::MidiBuffer synthMidi;
    double sampleRate = 44100.0;

    // anteprima: scritta dal thread dei messaggi sotto lock, letta dal thread audio con try-lock
    juce::SpinLock previewLock;
    std::vector<PreviewEvent> previewEvents;
    int previewTotal = 1;
    bool previewLoop = true;
    double previewTicks = 0.0;
    std::atomic<bool> previewActive { false };
    std::atomic<bool> cutRequested { false };
    std::atomic<int> previewPosTick { 0 };

    // analisi (scrittore unico: thread audio)
    std::array<bool, 128> held {};
    std::atomic<uint16_t> heldMask { 0 };
    std::atomic<int> lowestHeldPc { -1 };
    std::array<std::atomic<float>, 12> hist;
    std::atomic<double> bpmAtomic { 120.0 };

    // registrazione
    std::atomic<bool> recording { false };
    bool wasRecording = false, recUseHost = false;
    double recStartBeat = 0.0, internalBeat = 0.0;
    juce::AbstractFifo capFifo { 2048 };
    std::array<CapEvent, 2048> capBuf {};
    std::map<int, Pending> pending;              // solo thread dei messaggi

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordMateProcessor)
};
