#pragma once
#include "PluginProcessor.h"
#include "UIWidgets.h"

#include <array>

// Pannello con tutta l'interfaccia, disegnato a dimensione fissa (kWidth x kHeight)
// e scalato dall'editor: così la finestra si può ridimensionare senza rifare il layout.
class ChordMatePanel : public juce::Component,
                       private juce::Timer
{
public:
    static constexpr int kWidth = 1100, kHeight = 820;

    explicit ChordMatePanel (ChordMateProcessor&);
    ~ChordMatePanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // sincronizzazione controlli <-> progetto
    void syncControlsFromProject();
    void rebuildPresetBox();
    void refreshAll();
    void refreshSlots();
    void refreshSuggestions();
    void refreshRoll();
    void refreshCaptureInfo();
    void refreshArrangeInfo();
    void afterProgressionChanged();
    void afterMelodySettingsChanged();
    void restartPreviewIfPlaying();
    void updateLivePanel();

    // azioni
    void onRootChanged (int newRoot);
    void onGenreChanged (int index);
    void insertChord (const cm::Chord& c, bool replace);
    void slotMenu (int index);
    cm::Chord coloured (const cm::Chord& c) const;
    juce::File writeMidi (const std::vector<cm::Note>& notes, int lengthTicks, const juce::String& label);
    juce::File writeArrangementMidi();
    void saveMidiAs (const juce::File& tempFile);
    cm::Chord suggestionSource() const;

    ChordMateProcessor& proc;
    cm::Project& prj;
    ui::DarkLookAndFeel laf;

    // barra superiore
    juce::Label title;
    juce::ComboBox rootBox, scaleBox, genreBox, colourBox;
    juce::Label bpmLabel;
    juce::TextButton playBtn;
    ui::LabeledSlider volume { ui::U8 ("Volume anteprima"), 0.0, 1.0, 0.01 };

    // ascolto live
    juce::Label liveLabel;
    juce::TextButton useKeyBtn, addChordBtn;
    juce::ToggleButton playThroughToggle;

    // progressione
    juce::ComboBox presetBox, lengthBox, spcBox, styleBox;
    juce::TextButton applyPresetBtn, randomBtn, reharmBtn, clearBtn;
    juce::ToggleButton bassToggle;
    ui::LabeledSlider adventure { ui::U8 ("Avventura (accordi colore)"), 0.0, 1.0, 0.01 };
    std::array<ui::ChordSlot, cm::kMaxChords> slots;
    juce::Label suggTitle;
    std::array<ui::SuggestionButton, 6> sugg;
    std::vector<cm::Suggestion> currentSuggestions;
    int selectedSlot = -1;

    // melodia: tre macro + contorno + registro
    ui::LabeledSlider energy { ui::U8 ("Energia"), 0.0, 1.0, 0.01 };
    ui::LabeledSlider movement { ui::U8 ("Movimento (salti)"), 0.0, 1.0, 0.01 };
    ui::LabeledSlider tension { ui::U8 ("Tensione (colore)"), 0.0, 1.0, 0.01 };
    juce::ComboBox contourBox, registerBox;
    juce::TextButton melodyBtn;
    ui::RollView roll;

    // arrangiamento
    juce::ComboBox ensembleBox, polyBox;
    juce::Label partsInfo, polyInfo;

    // cattura
    juce::TextButton recBtn, playCapBtn, clearCapBtn, harmonizeBtn;
    juce::ComboBox quantBox;
    ui::LabeledSlider quantStrength { ui::U8 ("Forza quantizzazione"), 0.0, 1.0, 0.01 };
    juce::ToggleButton snapToggle, quantLenToggle;
    juce::Label capInfo;

    // esportazione
    ui::DragHandle dragChords, dragMelody, dragArrangement, dragCapture;
    juce::Label hint;
    std::unique_ptr<juce::FileChooser> chooser;

    // stato interno
    int lastRevision = 0;
    cm::KeyGuess lastKeyGuess;
    bool haveKeyGuess = false;
    cm::DetectedChord liveChord;
    bool liveChordHeld = false;
    juce::String lastLiveText;
    bool uiReady = false;

    int lastCaptureRevision = 0;

    juce::Rectangle<int> secProg, secMelody, secArrange, secCapture, secExport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordMatePanel)
};

// Editor del plugin: contiene il pannello e lo scala (finestra ridimensionabile, proporzioni fisse).
class ChordMateEditor : public juce::AudioProcessorEditor
{
public:
    explicit ChordMateEditor (ChordMateProcessor&);
    ~ChordMateEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ChordMateProcessor& proc;
    ChordMatePanel panel;
    juce::TooltipWindow tooltips { this, 600 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordMateEditor)
};
