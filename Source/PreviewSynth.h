#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Piccolo synth (saw + sinusoide, filtro passa-basso, inviluppo ADSR) usato solo per
// ascoltare in anteprima accordi e melodie dentro al plugin.
struct PreviewSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PreviewVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PreviewSound*> (s) != nullptr; }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        level = velocity;
        phase = 0.0;
        lp = 0.0f;
        const double sr = getSampleRate();
        const double fc = juce::jmin (freq * 4.0, 6000.0);
        lpCoeff = (float) (1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * fc / sr));
        adsr.setSampleRate (sr);
        adsr.setParameters ({ 0.008f, 0.18f, 0.55f, 0.22f });
        adsr.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            adsr.noteOff();
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    using juce::SynthesiserVoice::renderNextBlock;
    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive()) return;
        const double inc = freq / getSampleRate();
        const int chans = out.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const float env = adsr.getNextSample();
            phase += inc;
            if (phase >= 1.0) phase -= 1.0;
            const float saw = (float) (2.0 * phase - 1.0);
            const float sine = (float) std::sin (juce::MathConstants<double>::twoPi * phase);
            const float raw = (0.55f * saw + 0.45f * sine) * level * env * 0.22f;
            lp += lpCoeff * (raw - lp);

            for (int c = 0; c < chans; ++c)
                out.addSample (c, startSample + i, lp);

            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    juce::ADSR adsr;
    double freq = 440.0, phase = 0.0;
    float level = 0.0f, lp = 0.0f, lpCoeff = 0.2f;
};
