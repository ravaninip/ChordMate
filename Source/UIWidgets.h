#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "Engine/Theory.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace ui
{
// ----------------------------------------------------------------------- utilità
inline juce::String U8 (const char* s) { return juce::String::fromUTF8 (s); }
inline juce::String U8 (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

inline juce::Font font (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::String midiNoteName (int note)
{
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
}

// ----------------------------------------------------------------------- tavolozza
inline const juce::Colour bg      { 0xff12151a };
inline const juce::Colour panel   { 0xff1b2028 };
inline const juce::Colour edge    { 0xff2c3541 };
inline const juce::Colour text    { 0xffe6eaf0 };
inline const juce::Colour dim     { 0xff8a95a5 };
inline const juce::Colour accent  { 0xff3ddc97 };   // verde: melodia / azione
inline const juce::Colour accent2 { 0xffffb84d };   // ambra: gradi / cattura
inline const juce::Colour accent3 { 0xff6ea8ff };   // blu: accordi

class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::TextButton::buttonColourId, panel.brighter (0.12f));
        setColour (juce::TextButton::buttonOnColourId, accent.darker (0.3f));
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        setColour (juce::ComboBox::backgroundColourId, panel.brighter (0.08f));
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, edge);
        setColour (juce::ComboBox::arrowColourId, dim);
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent3.darker (0.4f));
        setColour (juce::Slider::thumbColourId, accent);
        setColour (juce::Slider::trackColourId, accent.darker (0.5f));
        setColour (juce::Slider::backgroundColourId, edge);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, dim);
        setColour (juce::Label::textColourId, text);
    }
};

// ----------------------------------------------------------------------- slider con etichetta
class LabeledSlider : public juce::Component
{
public:
    LabeledSlider (const juce::String& caption, double min, double max, double step)
    {
        label.setText (caption, juce::dontSendNotification);
        label.setFont (font (12.0f));
        label.setColour (juce::Label::textColourId, dim);
        label.setBorderSize ({ 0, 0, 0, 0 });
        addAndMakeVisible (label);

        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
        slider.setRange (min, max, step);
        addAndMakeVisible (slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (14));
        slider.setBounds (r);
    }

    juce::Label label;
    juce::Slider slider;
};

// ----------------------------------------------------------------------- accordo nella progressione
class ChordSlot : public juce::Component
{
public:
    std::function<void (int)> onClick, onMenu;
    int index = 0;
    juce::String name, roman;
    bool selected = false, empty = false, playing = false;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        if (empty)
        {
            g.setColour (edge);
            g.drawRoundedRectangle (r, 8.0f, 1.0f);
            g.setColour (dim);
            g.setFont (font (24.0f));
            g.drawText ("+", r, juce::Justification::centred);
            return;
        }
        g.setColour (selected ? accent3.withAlpha (0.28f) : panel.brighter (0.08f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (playing ? accent : (selected ? accent3 : edge));
        g.drawRoundedRectangle (r, 8.0f, (selected || playing) ? 2.0f : 1.0f);

        auto top = r.removeFromTop (r.getHeight() * 0.62f);
        g.setColour (text);
        g.setFont (font (22.0f, true));
        g.drawText (name, top, juce::Justification::centredBottom);
        g.setColour (accent2);
        g.setFont (font (14.0f));
        g.drawText (roman, r, juce::Justification::centredTop);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition())) return;
        if (e.mods.isPopupMenu()) { if (onMenu && ! empty) onMenu (index); }
        else if (onClick) onClick (index);
    }
};

// ----------------------------------------------------------------------- suggerimento
class SuggestionButton : public juce::Component
{
public:
    std::function<void (int, bool)> onClick;   // (indice, sostituisci?)
    int index = 0;
    juce::String name, roman, reason;
    bool borrowed = false;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (3.0f);
        g.setColour (panel.brighter (0.06f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (borrowed ? accent2.withAlpha (0.8f) : edge);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        auto inner = r.reduced (8.0f, 5.0f);
        auto head = inner.removeFromTop (24.0f);
        g.setColour (text);
        g.setFont (font (19.0f, true));
        g.drawText (name, head.removeFromLeft (head.getWidth() * 0.55f), juce::Justification::centredLeft);
        g.setColour (accent2);
        g.setFont (font (14.0f));
        g.drawText (roman, head, juce::Justification::centredRight);

        g.setColour (dim);
        g.setFont (font (11.5f));
        g.drawFittedText (reason, inner.toNearestInt(), juce::Justification::topLeft, 3, 0.9f);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()) && onClick)
            onClick (index, e.mods.isPopupMenu());
    }
};

// ----------------------------------------------------------------------- piano roll in sola lettura
class RollView : public juce::Component
{
public:
    void setData (std::vector<cm::Note> chordNotes, std::vector<cm::Note> melodyNotes,
                  std::vector<cm::Note> captureNotes, int minTicks)
    {
        chords = std::move (chordNotes);
        melody = std::move (melodyNotes);
        capture = std::move (captureNotes);
        totalTicks = std::max (minTicks, 4 * cm::kPPQ);
        for (auto* v : { &chords, &melody, &capture })
            for (auto& n : *v) totalTicks = std::max (totalTicks, n.startTick + n.lengthTick);
        const int bar = 4 * cm::kPPQ;
        totalTicks = ((totalTicks + bar - 1) / bar) * bar;
        repaint();
    }

    void setPlayhead (int tick)
    {
        if (tick != playTick) { playTick = tick; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (r, 6.0f);
        auto a = r.reduced (6.0f, 5.0f);

        int lo = 127, hi = 0;
        for (auto* v : { &chords, &melody, &capture })
            for (auto& n : *v) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        if (lo > hi) { lo = 48; hi = 84; }
        lo -= 2; hi += 2;
        while (hi - lo < 24) { --lo; ++hi; }
        const float span = float (hi - lo + 1);

        // griglia: battute marcate, quarti leggeri
        for (int t = 0; t <= totalTicks; t += cm::kPPQ)
        {
            const float x = a.getX() + a.getWidth() * float (t) / float (totalTicks);
            g.setColour (t % (4 * cm::kPPQ) == 0 ? edge.brighter (0.3f) : edge.withAlpha (0.5f));
            g.drawVerticalLine (int (x), a.getY(), a.getBottom());
        }

        auto draw = [&] (const std::vector<cm::Note>& v, juce::Colour c)
        {
            g.setColour (c);
            for (auto& n : v)
            {
                const float x = a.getX() + a.getWidth() * float (n.startTick) / float (totalTicks);
                const float w = std::max (2.0f, a.getWidth() * float (n.lengthTick) / float (totalTicks) - 1.0f);
                const float h = a.getHeight() / span;
                const float y = a.getBottom() - (float (n.pitch - lo) + 1.0f) * h;
                g.fillRoundedRectangle (x, y, w, std::max (2.0f, h - 0.5f), 1.5f);
            }
        };
        draw (chords, accent3.withAlpha (0.45f));
        draw (capture, accent2.withAlpha (0.85f));
        draw (melody, accent);

        if (playTick >= 0)
        {
            const float x = a.getX() + a.getWidth() * float (playTick) / float (totalTicks);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawVerticalLine (int (x), a.getY(), a.getBottom());
        }
    }

private:
    std::vector<cm::Note> chords, melody, capture;
    int totalTicks = 4 * cm::kPPQ;
    int playTick = -1;
};

// ----------------------------------------------------------------------- maniglia di trascinamento MIDI
// Trascina: crea il file .mid e lo passa al sistema (Live accetta il drop su una traccia MIDI).
// Clic semplice: "Salva con nome...".
class DragHandle : public juce::Component
{
public:
    std::function<juce::File()> makeFile;          // crea il file temporaneo (vuoto se nulla da esportare)
    std::function<bool()> hasContent;
    std::function<void (const juce::File&)> onSave;
    juce::String caption, subtitle;
    juce::Colour colour = accent;

    void paint (juce::Graphics& g) override
    {
        const bool ok = hasContent ? hasContent() : true;
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (ok ? colour.withAlpha (0.16f) : panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (ok ? colour : edge);
        g.drawRoundedRectangle (r, 8.0f, 1.5f);

        g.setColour (ok ? text : dim);
        g.setFont (font (15.0f, true));
        g.drawText (U8 ("≡  ") + caption, r.reduced (10.0f, 0.0f).removeFromTop (r.getHeight() * 0.58f),
                    juce::Justification::centredLeft);
        g.setColour (dim);
        g.setFont (font (11.5f));
        g.drawText (subtitle, r.reduced (10.0f, 0.0f).removeFromBottom (r.getHeight() * 0.42f),
                    juce::Justification::centredLeft);
    }

    void mouseDown (const juce::MouseEvent&) override { dragged = false; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragged || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        dragged = true;
        const auto f = makeFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragged || ! getLocalBounds().contains (e.getPosition()) || ! makeFile) return;
        const auto f = makeFile();
        if (f.existsAsFile() && onSave) onSave (f);
    }

private:
    bool dragged = false;
};

} // namespace ui
