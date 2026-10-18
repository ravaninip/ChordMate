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
    static constexpr int kWidth = 980, kHeight = 740;

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
    void afterProgressionChanged();
    void restartPreviewIfPlaying();
    void updateLivePanel();

    // azioni
    void onRootChanged (int newRoot);
    void insertChord (const cm::Chord& c, bool replace);
    void slotMenu (int index);
    void convertSevenths();
    juce::File writeMidi (const std::vector<cm::Note>& notes, int lengthTicks, const juce::String& label);
    void saveMidiAs (const juce::File& tempFile);
    cm::Chord suggestionSource() const;

    ChordMateProcessor& proc;
    cm::Project& prj;
    ui::DarkLookAndFeel laf;

    // barra superiore
    juce::Label title;
    juce::ComboBox rootBox, scaleBox;
    juce::ToggleButton seventhsToggle;
    juce::Label bpmLabel;
    juce::TextButton playBtn;
    ui::LabeledSlider volume { ui::U8 ("Volume anteprima"), 0.0, 1.0, 0.01 };

    // ascolto live
    juce::Label liveLabel;
    juce::TextButton useKeyBtn, addChordBtn;
    juce::ToggleButton playThroughToggle;

    // progressione
    juce::ComboBox presetBox, lengthBox, spcBox, styleBox;
    juce::TextButton applyPresetBtn, randomBtn, clearBtn;
    juce::ToggleButton bassToggle;
    ui::LabeledSlider adventure { ui::U8 ("Avventura (accordi colore)"), 0.0, 1.0, 0.01 };
    std::array<ui::ChordSlot, cm::kMaxChords> slots;
    juce::Label suggTitle;
    std::array<ui::SuggestionButton, 6> sugg;
    std::vector<cm::Suggestion> currentSuggestions;
    int selectedSlot = -1;

    // melodia
    ui::LabeledSlider density { ui::U8 ("Densità ritmica"), 0.0, 1.0, 0.01 };
    ui::LabeledSlider leaps { ui::U8 ("Salti melodici"), 0.0, 1.0, 0.01 };
    ui::LabeledSlider lowNote { ui::U8 ("Nota più bassa"), 36, 96, 1 };
    ui::LabeledSlider highNote { ui::U8 ("Nota più alta"), 36, 108, 1 };
    juce::TextButton melodyBtn;
    juce::ToggleButton resolveToggle;
    ui::RollView roll;

    // cattura
    juce::TextButton recBtn, playCapBtn, clearCapBtn, harmonizeBtn;
    juce::ComboBox quantBox;
    ui::LabeledSlider quantStrength { ui::U8 ("Forza quantizzazione"), 0.0, 1.0, 0.01 };
    juce::ToggleButton snapToggle, quantLenToggle;
    juce::Label capInfo;

    // esportazione
    ui::DragHandle dragChords, dragMelody, dragCapture;
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

    juce::Rectangle<int> secProg, secMelody, secCapture, secExport;

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
