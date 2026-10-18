// Smoke test del plugin senza host: processor (analisi, cattura, anteprima, stato) ed editor.
// Uso: chordmate_plugin_smoke [percorso_screenshot.png]
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cstdio>

static int g_fail = 0, g_checks = 0;
#define EXPECT(...)                                                                       \
    do { ++g_checks; if (! (__VA_ARGS__)) { ++g_fail;                                     \
        std::printf ("  FAIL  %s:%d  %s\n", __FILE__, __LINE__, #__VA_ARGS__); } } while (0)

static void collectComponents (juce::Component& c, std::vector<juce::Component*>& out)
{
    for (auto* child : c.getChildren())
    {
        out.push_back (child);
        collectComponents (*child, out);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    ChordMateProcessor p;
    p.setRateAndBufferSizeDetails (44100.0, 512);
    p.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    auto block = [&] (int noteOnPitch = -1, int noteOffPitch = -1, int offset = 0)
    {
        buf.clear();
        midi.clear();
        if (noteOnPitch >= 0)  midi.addEvent (juce::MidiMessage::noteOn (1, noteOnPitch, (juce::uint8) 100), offset);
        if (noteOffPitch >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, noteOffPitch), offset);
        p.processBlock (buf, midi);
        return buf.getMagnitude (0, 512);
    };

    std::puts ("processor: analisi del MIDI in ingresso");
    block (60); block (64); block (67);
    EXPECT (p.heldPitchClassMask() == ((1u << 0) | (1u << 4) | (1u << 7)));
    EXPECT (p.lowestHeldPitchClass() == 0);
    {
        const auto d = cm::detectChord (p.heldPitchClassMask(), p.lowestHeldPitchClass());
        EXPECT (d.valid && d.chord == cm::Chord { 0, cm::Quality::Maj });
    }
    block (-1, 60); block (-1, 64); block (-1, 67);
    EXPECT (p.heldPitchClassMask() == 0);
    EXPECT (p.lowestHeldPitchClass() == -1);

    // scala di Re maggiore suonata a lungo -> tonalità Re maggiore
    p.resetHistogram();
    const int dMajor[] = { 62, 64, 66, 67, 69, 71, 73, 74, 62, 66, 69, 62, 67, 71, 74 };
    for (int rep = 0; rep < 6; ++rep)
        for (int n : dMajor) { block (n); block (-1, n); }
    {
        const auto g = cm::detectKey (p.pitchClassHistogram());
        EXPECT (! g.empty() && g[0].key.root == 2 && g[0].key.scale == cm::ScaleType::Major);
    }

    std::puts ("processor: anteprima audio");
    {
        float peak = 0;
        for (int i = 0; i < 20; ++i) peak = std::max (peak, block());
        EXPECT (peak == 0.0f);                                  // niente anteprima = silenzio

        p.project = cm::Project();
        p.project.applyPreset();
        p.project.generateMelodyNow();
        p.playAll();
        peak = 0;
        int lastPos = -1;
        bool advanced = false;
        for (int i = 0; i < 400; ++i)
        {
            peak = std::max (peak, block());
            const int pos = p.previewPositionTick();
            if (lastPos >= 0 && pos != lastPos) advanced = true;
            lastPos = pos;
        }
        EXPECT (peak > 0.005f && peak < 1.0f);
        EXPECT (advanced);
        EXPECT (p.isPreviewPlaying());                          // in loop

        p.stopPreview();
        float tail = 0;
        for (int i = 0; i < 200; ++i) tail = block();
        EXPECT (tail < 1.0e-4f);                                // dopo lo stop si spegne
        EXPECT (! p.isPreviewPlaying());

        // anteprima di un singolo accordo: parte e si ferma da sola
        p.auditionChord ({ 7, cm::Quality::Dom7 });
        float aPeak = 0;
        for (int i = 0; i < 40; ++i) aPeak = std::max (aPeak, block());
        EXPECT (aPeak > 0.005f);
        for (int i = 0; i < 600; ++i) block();
        EXPECT (! p.isPreviewPlaying());
        float aTail = 0;
        for (int i = 0; i < 50; ++i) aTail = block();
        EXPECT (aTail < 1.0e-4f);
    }

    std::puts ("processor: cattura MIDI");
    {
        p.clearCapture();
        p.setRecording (true);
        block (60, -1, 100);
        block (64, -1, 0);
        block (-1, 60, 0);
        for (int i = 0; i < 40; ++i) block();
        block (-1, 64, 0);
        p.setRecording (false);
        block();
        EXPECT (p.pullCapturedNotes());
        EXPECT (p.project.capture.size() == 2);
        if (p.project.capture.size() == 2)
        {
            EXPECT (p.project.capture[0].pitch == 60 && p.project.capture[0].startTick < 20);
            EXPECT (p.project.capture[1].pitch == 64);
            EXPECT (p.project.capture[1].lengthTick > p.project.capture[0].lengthTick);   // la 64 dura di più
            EXPECT (p.project.capture[0].velocity == 100);
        }
        EXPECT (! p.pullCapturedNotes());
    }

    std::puts ("processor: stato");
    {
        p.project.key = { 9, cm::ScaleType::NaturalMinor };
        p.project.sevenths = true;
        p.project.progSeed = 7;
        p.project.randomProgression();
        p.project.generateMelodyNow();
        p.volume.store (0.4f);
        juce::MemoryBlock mb;
        p.getStateInformation (mb);
        EXPECT (mb.getSize() > 50);

        ChordMateProcessor q;
        const int rev = q.projectRevision.load();
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        EXPECT (q.projectRevision.load() == rev + 1);
        EXPECT (q.project.progression == p.project.progression);
        EXPECT (q.project.melody == p.project.melody);
        EXPECT (q.project.capture == p.project.capture);
        EXPECT (std::abs (q.volume.load() - 0.4f) < 0.002f);
        q.setStateInformation ("spazzatura", 10);              // non deve rompere nulla
        EXPECT (q.project.progression == p.project.progression);
    }

    std::puts ("editor: creazione, interazioni a tappeto e screenshot");
    {
        p.project = cm::Project();
        p.project.applyPreset();
        p.project.generateMelodyNow();
        p.project.capture = { { 64, 0, 400, 90 }, { 67, 480, 400, 90 }, { 72, 960, 400, 90 }, { 69, 1440, 800, 90 } };

        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        EXPECT (ed != nullptr);
        if (ed != nullptr)
        {
            std::printf ("  editor: %dx%d\n", ed->getWidth(), ed->getHeight());
            EXPECT (ed->getWidth() > 600 && ed->getHeight() > 400);

            std::vector<juce::Component*> all;
            collectComponents (*ed, all);
            EXPECT (all.size() > 40);

            // monkey test: clic su ogni pulsante, ogni voce di ogni combo, muove ogni slider
            int actions = 0;
            for (int round = 0; round < 3; ++round)
                for (auto* c : all)
                {
                    if (auto* b = dynamic_cast<juce::Button*> (c))
                    {
                        if (b->isEnabled() && b->isVisible()) { b->triggerClick(); ++actions; }
                    }
                    else if (auto* cb = dynamic_cast<juce::ComboBox*> (c))
                    {
                        const int n = cb->getNumItems();
                        for (int i = 0; i < n; ++i) { cb->setSelectedItemIndex (i, juce::sendNotificationSync); ++actions; }
                    }
                    else if (auto* s = dynamic_cast<juce::Slider*> (c))
                    {
                        for (double f : { 0.0, 0.5, 1.0 })
                        {
                            s->setValue (s->proportionOfLengthToValue (f), juce::sendNotificationSync);
                            ++actions;
                        }
                    }
                    for (int i = 0; i < 32; ++i) block();    // l'audio gira mentre l'interfaccia viene stressata
                }
            std::printf ("  azioni simulate: %d\n", actions);
            EXPECT (actions > 100);
            EXPECT (p.project.progression.size() >= 1 && p.project.progression.size() <= (size_t) cm::kMaxChords);
            for (auto& c : p.project.progression) EXPECT (c.root >= 0 && c.root < 12);
            p.stopPreview();

            // ripristina uno stato bello da fotografare
            p.project = cm::Project();
            p.project.key = { 0, cm::ScaleType::Major };
            p.project.applyPreset();
            p.project.generateMelodyNow();
            p.project.capture = { { 64, 0, 400, 90 }, { 67, 480, 400, 90 }, { 72, 960, 400, 90 }, { 69, 1440, 800, 90 } };
            p.projectRevision.fetch_add (1);

            if (argc > 1)
            {
                std::unique_ptr<juce::AudioProcessorEditor> ed2 (p.createEditor());
                const auto img = ed2->createComponentSnapshot (ed2->getLocalBounds(), true, 1.0f);
                juce::File out (juce::String::fromUTF8 (argv[1]));
                out.deleteFile();
                juce::FileOutputStream fos (out);
                juce::PNGImageFormat png;
                EXPECT (fos.openedOk() && png.writeImageToStream (img, fos));
                std::printf ("  screenshot: %s (%dx%d)\n", argv[1], img.getWidth(), img.getHeight());

                // secondo scatto: genere cinematico, orchestra sinfonica con poliaccordi
                p.project = cm::Project();
                p.project.key = { 9, cm::ScaleType::NaturalMinor };
                p.project.genre = cm::Genre::Cinematic;
                p.project.applyGenreDefaults();
                p.project.applyPreset();
                p.project.generateMelodyNow();
                p.projectRevision.fetch_add (1);
                std::unique_ptr<juce::AudioProcessorEditor> ed3 (p.createEditor());
                const auto img2 = ed3->createComponentSnapshot (ed3->getLocalBounds(), true, 1.0f);
                juce::File out2 = out.getSiblingFile (out.getFileNameWithoutExtension() + "_cinema.png");
                out2.deleteFile();
                juce::FileOutputStream fos2 (out2);
                EXPECT (fos2.openedOk() && png.writeImageToStream (img2, fos2));
                EXPECT (p.project.arrangement().size() >= 8);
            }
        }
    }

    std::printf ("\n%d controlli, %d falliti\n", g_checks, g_fail);
    return g_fail == 0 ? 0 : 1;
}
