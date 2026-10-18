#include "PluginEditor.h"
#include "Engine/MidiWriter.h"

#include <cmath>

using ui::U8;

namespace
{
const int kSpcValues[3] = { 8, 16, 32 };
const int kQuantValues[5] = { 0, cm::kPPQ, cm::kPPQ / 2, cm::kPPQ / 4, cm::kPPQ / 8 };

juce::String capitalised (const juce::String& s)
{
    return s.substring (0, 1).toUpperCase() + s.substring (1);
}
}

ChordMatePanel::ChordMatePanel (ChordMateProcessor& p)
    : proc (p), prj (p.project)
{
    setLookAndFeel (&laf);
    setSize (kWidth, kHeight);

    auto setupLabel = [this] (juce::Label& l, const juce::String& t, float size, juce::Colour c, bool bold = false)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (ui::font (size, bold));
        l.setColour (juce::Label::textColourId, c);
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };
    auto setupButton = [this] (juce::Button& b, const juce::String& t)
    {
        b.setButtonText (t);
        addAndMakeVisible (b);
    };

    // ------------------------------------------------------------ barra superiore
    setupLabel (title, "ChordMate", 24.0f, ui::accent, true);

    static const char* rootNames[12] = { "C", "C# / Db", "D", "D# / Eb", "E", "F", "F# / Gb", "G", "G# / Ab", "A", "A# / Bb", "B" };
    for (int i = 0; i < 12; ++i) rootBox.addItem (rootNames[i], i + 1);
    for (int i = 0; i < cm::kNumScales; ++i)
        scaleBox.addItem (capitalised (U8 (cm::scaleInfo (cm::scaleFromIndex (i)).name)), i + 1);
    addAndMakeVisible (rootBox);
    addAndMakeVisible (scaleBox);
    rootBox.setTooltip (U8 ("Tonica della tonalità"));
    scaleBox.setTooltip (U8 ("Scala / modo"));

    for (int i = 0; i < cm::kNumGenres; ++i)
        genreBox.addItem (U8 ("Genere: ") + U8 (cm::genreName (cm::genreFromIndex (i))), i + 1);
    genreBox.setTooltip (U8 ("Macro-genere di riferimento: cambia preset, suggerimenti, ritmo della melodia, "
                             "basso, colore degli accordi e arrangiamento"));
    colourBox.addItem (U8 ("Triadi"), 1);
    colourBox.addItem (U8 ("Settime"), 2);
    colourBox.addItem (U8 ("Estese (9ª in scala)"), 3);
    colourBox.setTooltip (U8 ("Colore degli accordi: la 9ª si aggiunge solo se appartiene alla scala"));
    addAndMakeVisible (genreBox);
    addAndMakeVisible (colourBox);

    setupLabel (bpmLabel, "", 13.0f, ui::dim);
    bpmLabel.setJustificationType (juce::Justification::centredRight);
    setupButton (playBtn, U8 ("▶  Ascolta tutto"));
    addAndMakeVisible (volume);

    // ------------------------------------------------------------ ascolto live
    setupLabel (liveLabel, "", 14.0f, ui::text);
    setupButton (useKeyBtn, U8 ("Usa questa tonalità"));
    setupButton (addChordBtn, U8 ("+ Aggiungi accordo"));
    setupButton (playThroughToggle, U8 ("Suona anche il MIDI in ingresso"));
    useKeyBtn.setTooltip (U8 ("Imposta la tonalità stimata dalle note che hai suonato negli ultimi secondi"));
    addChordBtn.setTooltip (U8 ("Aggiunge alla progressione l'accordo che stai tenendo premuto (o l'ultimo riconosciuto)"));

    // ------------------------------------------------------------ progressione
    for (int n = 2; n <= cm::kMaxChords; ++n) lengthBox.addItem (juce::String (n) + U8 (" accordi"), n);
    spcBox.addItem (U8 ("Mezza battuta per accordo"), 1);
    spcBox.addItem (U8 ("1 battuta per accordo"), 2);
    spcBox.addItem (U8 ("2 battute per accordo"), 3);
    for (int i = 0; i < cm::kNumChordStyles; ++i)
        styleBox.addItem (U8 (cm::chordStyleName (cm::ChordStyle (i))), i + 1);
    for (auto* c : { &presetBox, &lengthBox, &spcBox, &styleBox }) addAndMakeVisible (*c);
    setupButton (applyPresetBtn, U8 ("Applica preset"));
    setupButton (randomBtn, U8 ("Genera casuale"));
    setupButton (reharmBtn, U8 ("Rearmonizza"));
    reharmBtn.setTooltip (U8 ("Sostituisce alcuni accordi con altri della stessa funzione, dominanti secondarie "
                              "o sostituzioni di tritono, nello stile del genere (primo e ultimo restano)"));
    setupButton (clearBtn, U8 ("Svuota"));
    setupButton (bassToggle, U8 ("Basso"));
    addAndMakeVisible (adventure);
    lengthBox.setTooltip (U8 ("Quanti accordi genera il pulsante «Genera casuale»"));
    adventure.slider.setTooltip (U8 ("0 = solo accordi della tonalità · più alto = prestiti modali, dominanti secondarie"));

    for (size_t i = 0; i < slots.size(); ++i)
    {
        slots[i].index = (int) i;
        slots[i].onClick = [this] (int idx)
        {
            const int n = (int) prj.progression.size();
            if (idx < n)
            {
                selectedSlot = idx;
                proc.auditionChord (prj.progression[(size_t) idx]);
            }
            else
            {
                selectedSlot = n - 1;
                insertChord (coloured (cm::chordAtDegree (prj.key, 0, false, true)), false);
                return;
            }
            refreshSlots();
            refreshSuggestions();
        };
        slots[i].onMenu = [this] (int idx) { slotMenu (idx); };
        addChildComponent (slots[i]);
    }

    setupLabel (suggTitle, "", 13.0f, ui::dim);
    for (size_t i = 0; i < sugg.size(); ++i)
    {
        sugg[i].index = (int) i;
        sugg[i].onClick = [this] (int idx, bool replace)
        {
            if (idx < (int) currentSuggestions.size())
                insertChord (currentSuggestions[(size_t) idx].chord, replace);
        };
        addChildComponent (sugg[i]);
    }

    // ------------------------------------------------------------ melodia
    for (auto* s : { &energy, &movement, &tension }) addAndMakeVisible (*s);
    energy.slider.setTooltip (U8 ("Quante note, quanto corte e quanto accentate"));
    movement.slider.setTooltip (U8 ("0 = per gradi congiunti · 1 = salti ampi"));
    tension.slider.setTooltip (U8 ("0 = note dell'accordo sui tempi forti · alto = appoggiature, 9ª/13ª, cromatismi"));
    for (int i = 0; i < cm::kNumContours; ++i) contourBox.addItem (U8 (cm::contourName (cm::Contour (i))), i + 1);
    for (int i = 0; i < cm::kNumRegisters; ++i) registerBox.addItem (U8 (cm::registerName (i)), i + 1);
    contourBox.setTooltip (U8 ("Forma della frase: dove va la melodia nel tempo"));
    registerBox.setTooltip (U8 ("Altezza della melodia"));
    addAndMakeVisible (contourBox);
    addAndMakeVisible (registerBox);
    setupButton (melodyBtn, U8 ("Nuova melodia"));
    melodyBtn.setTooltip (U8 ("Genera una nuova melodia sugli accordi. I controlli la ritoccano senza cambiarla del tutto."));
    addAndMakeVisible (roll);

    // ------------------------------------------------------------ arrangiamento
    for (int i = 0; i < cm::kNumEnsembles; ++i) ensembleBox.addItem (U8 (cm::ensembleName (cm::ensembleFromIndex (i))), i + 1);
    for (int i = 0; i < cm::kNumPolyharmony; ++i) polyBox.addItem (U8 (cm::polyharmonyName (cm::polyharmonyFromIndex (i))), i + 1);
    ensembleBox.setTooltip (U8 ("Chi suona gli accordi: condotta delle parti a 4 voci, ostinati con gli stili ritmici"));
    polyBox.setTooltip (U8 ("Poliarmonia: estensioni in scala o una triade diversa sopra l'accordo (poliaccordo)"));
    addAndMakeVisible (ensembleBox);
    addAndMakeVisible (polyBox);
    setupLabel (partsInfo, "", 12.0f, ui::dim);
    setupLabel (polyInfo, "", 12.5f, ui::text);

    // ------------------------------------------------------------ cattura MIDI
    setupButton (recBtn, U8 ("● Registra"));
    recBtn.setClickingTogglesState (true);
    recBtn.setToggleState (proc.isRecording(), juce::dontSendNotification);
    if (proc.isRecording()) recBtn.setButtonText (U8 ("■ Stop registrazione"));
    recBtn.setTooltip (U8 ("Registra le note MIDI che arrivano al plugin (routing «MIDI From» di Live)"));
    setupButton (playCapBtn, U8 ("Riascolta"));
    setupButton (clearCapBtn, U8 ("Cancella cattura"));
    setupButton (harmonizeBtn, U8 ("Armonizza → accordi"));
    harmonizeBtn.setTooltip (U8 ("Propone una progressione che sta bene sotto la melodia catturata"));
    const char* quantNames[5] = { "Quantizza: off", "1/4", "1/8", "1/16", "1/32" };
    for (int i = 0; i < 5; ++i) quantBox.addItem (U8 (quantNames[i]), i + 1);
    addAndMakeVisible (quantBox);
    addAndMakeVisible (quantStrength);
    setupButton (snapToggle, U8 ("Note nella scala"));
    setupButton (quantLenToggle, U8 ("Quantizza durate"));
    snapToggle.setTooltip (U8 ("Sposta le note stonate sulla nota di scala più vicina"));
    setupLabel (capInfo, "", 12.5f, ui::dim);

    // ------------------------------------------------------------ esportazione
    dragChords.caption = U8 ("Accordi .mid");
    dragChords.subtitle = U8 ("trascina su una traccia MIDI di Live");
    dragChords.colour = ui::accent3;
    dragMelody.caption = U8 ("Melodia .mid");
    dragMelody.subtitle = U8 ("trascina su una traccia MIDI di Live");
    dragMelody.colour = ui::accent;
    dragArrangement.caption = U8 ("Arrangiamento .mid");
    dragArrangement.subtitle = U8 ("multitraccia: una traccia per sezione");
    dragArrangement.colour = juce::Colour (0xffc792ea);
    dragCapture.caption = U8 ("Cattura .mid");
    dragCapture.subtitle = U8 ("note registrate (con quantizzazione)");
    dragCapture.colour = ui::accent2;

    dragChords.makeFile = [this]
    {
        const auto n = prj.chordNotes();
        return n.empty() ? juce::File() : writeMidi (n, prj.lengthTicks(), "Accordi");
    };
    dragChords.hasContent = [this] { return ! prj.progression.empty(); };
    dragMelody.makeFile = [this]
    {
        return prj.melody.empty() ? juce::File() : writeMidi (prj.melody, prj.lengthTicks(), "Melodia");
    };
    dragMelody.hasContent = [this] { return ! prj.melody.empty(); };
    dragArrangement.makeFile = [this] { return prj.progression.empty() ? juce::File() : writeArrangementMidi(); };
    dragArrangement.hasContent = [this] { return ! prj.progression.empty(); };
    dragCapture.makeFile = [this]
    {
        const auto n = prj.processedCapture();
        return n.empty() ? juce::File() : writeMidi (n, cm::Project::captureLengthTicks (n), "Cattura");
    };
    dragCapture.hasContent = [this] { return ! prj.capture.empty(); };
    for (auto* d : { &dragChords, &dragMelody, &dragArrangement, &dragCapture })
    {
        d->onSave = [this] (const juce::File& f) { saveMidiAs (f); };
        addAndMakeVisible (*d);
    }

    setupLabel (hint,
                U8 ("Trascina un riquadro su una traccia MIDI (clic = salva). «Arrangiamento» va nello "
                    "spazio vuoto sotto le tracce: Live crea una traccia per sezione."),
                11.0f, ui::dim);
    hint.setMinimumHorizontalScale (1.0f);

    // ------------------------------------------------------------ callback
    rootBox.onChange = [this] { if (uiReady) onRootChanged (rootBox.getSelectedId() - 1); };
    scaleBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.key.scale = cm::scaleFromIndex (scaleBox.getSelectedId() - 1);
        rebuildPresetBox();
        if (! prj.melody.empty()) prj.generateMelodyNow();
        refreshAll();
        restartPreviewIfPlaying();
    };
    genreBox.onChange = [this] { if (uiReady) onGenreChanged (genreBox.getSelectedId() - 1); };
    colourBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.setColour (colourBox.getSelectedId() - 1);
        afterProgressionChanged();
    };

    playBtn.onClick = [this]
    {
        if (proc.isPreviewPlaying()) proc.stopPreview();
        else proc.playAll();
    };
    volume.slider.onValueChange = [this] { proc.volume.store ((float) volume.slider.getValue()); };
    playThroughToggle.onClick = [this] { proc.playThrough.store (playThroughToggle.getToggleState()); };

    useKeyBtn.onClick = [this]
    {
        if (! haveKeyGuess) return;
        prj.key = lastKeyGuess.key;
        rebuildPresetBox();
        syncControlsFromProject();
        if (! prj.melody.empty()) prj.generateMelodyNow();
        refreshAll();
    };
    addChordBtn.onClick = [this]
    {
        if (liveChord.valid) insertChord (liveChord.chord, false);
    };

    applyPresetBtn.onClick = [this]
    {
        prj.presetIndex = std::max (0, presetBox.getSelectedId() - 1);
        prj.applyPreset();
        selectedSlot = (int) prj.progression.size() - 1;
        afterProgressionChanged();
        proc.playAll();
    };
    randomBtn.onClick = [this]
    {
        prj.progLength = lengthBox.getSelectedId();
        prj.progSeed = (uint32_t) juce::Random::getSystemRandom().nextInt (1000000) + 1;
        prj.randomProgression();
        selectedSlot = (int) prj.progression.size() - 1;
        afterProgressionChanged();
        proc.playAll();
    };
    reharmBtn.onClick = [this]
    {
        if (prj.progression.size() < 3)
        {
            hint.setText (U8 ("Per rearmonizzare servono almeno 3 accordi (il primo e l'ultimo restano)."), juce::dontSendNotification);
            return;
        }
        prj.progSeed = (uint32_t) juce::Random::getSystemRandom().nextInt (1000000) + 1;
        prj.reharmonizeNow();
        afterProgressionChanged();
        proc.playAll();
    };
    clearBtn.onClick = [this]
    {
        prj.progression.clear();
        prj.melody.clear();
        selectedSlot = -1;
        proc.stopPreview();
        afterProgressionChanged();
    };
    lengthBox.onChange = [this] { if (uiReady) prj.progLength = lengthBox.getSelectedId(); };
    spcBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.stepsPerChord = kSpcValues[juce::jlimit (0, 2, spcBox.getSelectedId() - 1)];
        if (! prj.melody.empty()) prj.generateMelodyNow();
        refreshRoll();
        restartPreviewIfPlaying();
    };
    styleBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.chordStyle = cm::ChordStyle (juce::jlimit (0, cm::kNumChordStyles - 1, styleBox.getSelectedId() - 1));
        refreshRoll();
        restartPreviewIfPlaying();
    };
    bassToggle.onClick = [this]
    {
        prj.bass = bassToggle.getToggleState();
        refreshRoll();
        restartPreviewIfPlaying();
    };
    adventure.slider.onValueChange = [this]
    {
        prj.adventure = adventure.slider.getValue();
        refreshSuggestions();
    };

    auto melodyTweak = [this]
    {
        if (! uiReady) return;
        prj.density = energy.slider.getValue();
        prj.leaps = movement.slider.getValue();
        prj.tension = tension.slider.getValue();
        afterMelodySettingsChanged();
    };
    for (auto* s : { &energy, &movement, &tension }) s->slider.onValueChange = melodyTweak;
    contourBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.contour = cm::Contour (juce::jlimit (0, cm::kNumContours - 1, contourBox.getSelectedId() - 1));
        afterMelodySettingsChanged();
    };
    registerBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.setRegister (registerBox.getSelectedId() - 1);
        afterMelodySettingsChanged();
    };

    ensembleBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.ensemble = cm::ensembleFromIndex (ensembleBox.getSelectedId() - 1);
        refreshRoll();
        refreshArrangeInfo();
        restartPreviewIfPlaying();
    };
    polyBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.poly = cm::polyharmonyFromIndex (polyBox.getSelectedId() - 1);
        refreshRoll();
        refreshArrangeInfo();
        restartPreviewIfPlaying();
    };

    melodyBtn.onClick = [this]
    {
        if (prj.progression.empty())
        {
            hint.setText (U8 ("Prima crea una progressione: la melodia nasce sugli accordi."), juce::dontSendNotification);
            return;
        }
        prj.melodySeed = (uint32_t) juce::Random::getSystemRandom().nextInt (1000000) + 1;
        prj.generateMelodyNow();
        refreshRoll();
        proc.playAll();
    };
    recBtn.onClick = [this]
    {
        const bool on = recBtn.getToggleState();
        proc.setRecording (on);
        recBtn.setButtonText (on ? U8 ("■ Stop registrazione") : U8 ("● Registra"));
    };
    playCapBtn.onClick = [this] { proc.playCapture(); };
    clearCapBtn.onClick = [this]
    {
        proc.clearCapture();
        refreshRoll();
        refreshCaptureInfo();
    };
    quantBox.onChange = [this]
    {
        if (! uiReady) return;
        prj.quantGrid = kQuantValues[juce::jlimit (0, 4, quantBox.getSelectedId() - 1)];
        refreshRoll();
    };
    quantStrength.slider.onValueChange = [this]
    {
        prj.quantStrength = quantStrength.slider.getValue();
        refreshRoll();
    };
    snapToggle.onClick = [this] { prj.snapToScale = snapToggle.getToggleState(); refreshRoll(); };
    quantLenToggle.onClick = [this] { prj.quantLength = quantLenToggle.getToggleState(); refreshRoll(); };
    harmonizeBtn.onClick = [this]
    {
        if (prj.capture.empty())
        {
            capInfo.setText (U8 ("Nessuna nota catturata: premi «Registra» e suona la melodia."), juce::dontSendNotification);
            return;
        }
        prj.harmonizeCapture();
        selectedSlot = 0;
        afterProgressionChanged();
        proc.playAll();
    };

    rebuildPresetBox();
    syncControlsFromProject();
    lastRevision = proc.projectRevision.load();
    lastCaptureRevision = proc.captureRevision.load();
    uiReady = true;
    selectedSlot = (int) prj.progression.size() - 1;
    refreshAll();
    startTimerHz (30);
}

ChordMatePanel::~ChordMatePanel()
{
    stopTimer();     // la registrazione continua anche a finestra chiusa (la gestisce il processor)
    setLookAndFeel (nullptr);
}

// ====================================================================== sincronizzazione
void ChordMatePanel::syncControlsFromProject()
{
    const bool was = uiReady;
    uiReady = false;

    rootBox.setSelectedId (prj.key.root + 1, juce::dontSendNotification);
    scaleBox.setSelectedId ((int) prj.key.scale + 1, juce::dontSendNotification);
    genreBox.setSelectedId ((int) prj.genre + 1, juce::dontSendNotification);
    colourBox.setSelectedId (juce::jlimit (0, cm::kNumColours - 1, prj.colour) + 1, juce::dontSendNotification);
    presetBox.setSelectedId (juce::jlimit (0, juce::jmax (0, presetBox.getNumItems() - 1), prj.presetIndex) + 1,
                             juce::dontSendNotification);
    lengthBox.setSelectedId (juce::jlimit (2, cm::kMaxChords, prj.progLength), juce::dontSendNotification);

    int spcId = 2;
    for (int i = 0; i < 3; ++i) if (kSpcValues[i] == prj.stepsPerChord) spcId = i + 1;
    spcBox.setSelectedId (spcId, juce::dontSendNotification);
    styleBox.setSelectedId ((int) prj.chordStyle + 1, juce::dontSendNotification);
    bassToggle.setToggleState (prj.bass, juce::dontSendNotification);
    adventure.slider.setValue (prj.adventure, juce::dontSendNotification);

    energy.slider.setValue (prj.density, juce::dontSendNotification);
    movement.slider.setValue (prj.leaps, juce::dontSendNotification);
    tension.slider.setValue (prj.tension, juce::dontSendNotification);
    contourBox.setSelectedId ((int) prj.contour + 1, juce::dontSendNotification);
    registerBox.setSelectedId (prj.registerIndex + 1, juce::dontSendNotification);
    ensembleBox.setSelectedId ((int) prj.ensemble + 1, juce::dontSendNotification);
    polyBox.setSelectedId ((int) prj.poly + 1, juce::dontSendNotification);

    int qId = 4;
    for (int i = 0; i < 5; ++i) if (kQuantValues[i] == prj.quantGrid) qId = i + 1;
    quantBox.setSelectedId (qId, juce::dontSendNotification);
    quantStrength.slider.setValue (prj.quantStrength, juce::dontSendNotification);
    snapToggle.setToggleState (prj.snapToScale, juce::dontSendNotification);
    quantLenToggle.setToggleState (prj.quantLength, juce::dontSendNotification);

    volume.slider.setValue (proc.volume.load(), juce::dontSendNotification);
    playThroughToggle.setToggleState (proc.playThrough.load(), juce::dontSendNotification);

    uiReady = was;
}

void ChordMatePanel::rebuildPresetBox()
{
    const bool was = uiReady;
    uiReady = false;
    presetBox.clear (juce::dontSendNotification);
    const auto list = cm::presetNames (prj.genre, prj.key);
    for (size_t i = 0; i < list.size(); ++i) presetBox.addItem (U8 (list[i]), (int) i + 1);
    prj.presetIndex = juce::jlimit (0, (int) list.size() - 1, prj.presetIndex);
    presetBox.setSelectedId (prj.presetIndex + 1, juce::dontSendNotification);
    uiReady = was;
}

void ChordMatePanel::refreshAll()
{
    selectedSlot = juce::jmin (selectedSlot, (int) prj.progression.size() - 1);
    refreshSlots();
    refreshSuggestions();
    refreshRoll();
    refreshCaptureInfo();
    refreshArrangeInfo();
}

void ChordMatePanel::refreshSlots()
{
    const int n = (int) prj.progression.size();
    for (int i = 0; i < (int) slots.size(); ++i)
    {
        auto& s = slots[(size_t) i];
        if (i < n)
        {
            const auto& c = prj.progression[(size_t) i];
            s.name = U8 (cm::chordName (c, prj.key));
            s.roman = U8 (cm::romanNumeral (c, prj.key));
            s.empty = false;
            s.selected = i == selectedSlot;
            s.setVisible (true);
        }
        else if (i == n)
        {
            s.empty = true;
            s.selected = false;
            s.playing = false;
            s.setVisible (true);
        }
        else
            s.setVisible (false);
        s.repaint();
    }
}

cm::Chord ChordMatePanel::suggestionSource() const
{
    if (prj.progression.empty())
        return coloured (cm::chordAtDegree (prj.key, 4, false, true));
    const int idx = (selectedSlot >= 0 && selectedSlot < (int) prj.progression.size())
                        ? selectedSlot : (int) prj.progression.size() - 1;
    return prj.progression[(size_t) idx];
}

void ChordMatePanel::refreshSuggestions()
{
    const auto from = suggestionSource();
    currentSuggestions = cm::suggestNext (from, prj.key, prj.sevenths, prj.adventure, (int) sugg.size(), prj.genre, prj.colour);

    if (prj.progression.empty())
        suggTitle.setText (U8 ("Per iniziare, prova uno di questi accordi (clic = aggiungi):"), juce::dontSendNotification);
    else
        suggTitle.setText (U8 ("Dopo ") + U8 (cm::chordName (from, prj.key)) + " (" + U8 (cm::romanNumeral (from, prj.key))
                               + U8 (") puoi provare — clic: aggiungi dopo l'accordo selezionato · clic destro: sostituisci"),
                           juce::dontSendNotification);

    for (int i = 0; i < (int) sugg.size(); ++i)
    {
        auto& b = sugg[(size_t) i];
        if (i < (int) currentSuggestions.size())
        {
            const auto& s = currentSuggestions[(size_t) i];
            b.name = U8 (cm::chordName (s.chord, prj.key));
            b.roman = U8 (cm::romanNumeral (s.chord, prj.key));
            b.reason = U8 (s.reason);
            b.borrowed = s.borrowed;
            b.setVisible (true);
        }
        else
            b.setVisible (false);
        b.repaint();
    }
}

void ChordMatePanel::refreshRoll()
{
    roll.setData (prj.chordNotes(), prj.melody, prj.processedCapture(), prj.lengthTicks());
}

void ChordMatePanel::refreshCaptureInfo()
{
    const int n = (int) prj.capture.size();
    capInfo.setText (n == 0 ? U8 ("Cattura vuota. Premi «Registra» e suona (o fai partire la traccia di Live che invia MIDI a ChordMate).")
                            : juce::String (n) + U8 (" note catturate · ")
                                  + U8 (cm::keyName (prj.key)) + U8 (" · usa «Armonizza» per ottenere gli accordi"),
                     juce::dontSendNotification);
}

void ChordMatePanel::refreshArrangeInfo()
{
    juce::String parts;
    for (const auto& p : prj.arrangement())
        parts += (parts.isEmpty() ? "" : ", ") + U8 (p.name);
    partsInfo.setText (parts.isEmpty() ? U8 ("Nessun accordo da arrangiare") : U8 ("Parti: ") + parts,
                       juce::dontSendNotification);

    juce::String chords;
    for (const auto& n : prj.polychordNames())
        chords += (chords.isEmpty() ? "" : U8 ("  ·  ")) + U8 (n);
    const auto label = prj.poly == cm::Polyharmony::UpperTriad ? U8 ("Poliaccordi: ")
                     : prj.poly == cm::Polyharmony::Extensions ? U8 ("Con le estensioni: ")
                                                               : U8 ("Accordi: ");
    polyInfo.setText (chords.isEmpty() ? juce::String() : label + chords, juce::dontSendNotification);
}

void ChordMatePanel::afterMelodySettingsChanged()
{
    if (prj.melody.empty()) return;
    prj.generateMelodyNow();
    refreshRoll();
    restartPreviewIfPlaying();
}

void ChordMatePanel::restartPreviewIfPlaying()
{
    if (proc.isPreviewPlaying()) proc.playAll();
}

void ChordMatePanel::afterProgressionChanged()
{
    if (! prj.melody.empty()) prj.generateMelodyNow();
    prj.progLength = juce::jlimit (2, cm::kMaxChords, (int) prj.progression.size());
    syncControlsFromProject();
    refreshAll();
    restartPreviewIfPlaying();
}

// ====================================================================== azioni
void ChordMatePanel::onRootChanged (int newRoot)
{
    const int delta = newRoot - prj.key.root;
    if (delta == 0) return;
    prj.key.root = newRoot;
    for (auto& c : prj.progression) c.root = ((c.root + delta) % 12 + 12) % 12;   // trasporta la progressione
    afterProgressionChanged();
}

void ChordMatePanel::onGenreChanged (int index)
{
    prj.genre = cm::genreFromIndex (index);
    prj.applyGenreDefaults();      // colore, ritmo, basso, macro della melodia, ensemble e poliarmonia del genere
    rebuildPresetBox();
    if (! prj.melody.empty()) prj.generateMelodyNow();
    syncControlsFromProject();
    refreshAll();
    hint.setText (U8 (cm::genreProfile (prj.genre).description) + U8 (" «Applica preset» per un giro tipico."),
                  juce::dontSendNotification);
    restartPreviewIfPlaying();
}

cm::Chord ChordMatePanel::coloured (const cm::Chord& c) const
{
    return cm::adaptColour (c, prj.key, prj.colour);
}

void ChordMatePanel::insertChord (const cm::Chord& c, bool replace)
{
    auto& p = prj.progression;
    const bool validSel = selectedSlot >= 0 && selectedSlot < (int) p.size();

    if (replace && validSel)
        p[(size_t) selectedSlot] = c;
    else if ((int) p.size() < cm::kMaxChords)
    {
        const int pos = validSel ? selectedSlot + 1 : (int) p.size();
        p.insert (p.begin() + pos, c);
        selectedSlot = pos;
    }
    else if (validSel)
        p[(size_t) selectedSlot] = c;           // progressione piena: sostituisce
    else
        return;

    proc.auditionChord (c);
    afterProgressionChanged();
}

void ChordMatePanel::slotMenu (int index)
{
    auto& p = prj.progression;
    if (index < 0 || index >= (int) p.size()) return;

    juce::PopupMenu m;
    m.addItem (1, U8 ("Elimina"));
    m.addItem (2, U8 ("Duplica"), (int) p.size() < cm::kMaxChords);
    m.addSeparator();
    m.addItem (3, U8 ("Sposta a sinistra"), index > 0);
    m.addItem (4, U8 ("Sposta a destra"), index + 1 < (int) p.size());

    juce::Component::SafePointer<ChordMatePanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, index] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& pr = safe->prj.progression;
        if (index >= (int) pr.size()) return;

        switch (r)
        {
            case 1: pr.erase (pr.begin() + index); safe->selectedSlot = juce::jmin (index, (int) pr.size() - 1); break;
            case 2: pr.insert (pr.begin() + index, pr[(size_t) index]); safe->selectedSlot = index + 1; break;
            case 3: std::swap (pr[(size_t) index], pr[(size_t) index - 1]); safe->selectedSlot = index - 1; break;
            case 4: std::swap (pr[(size_t) index], pr[(size_t) index + 1]); safe->selectedSlot = index + 1; break;
            default: break;
        }
        safe->afterProgressionChanged();
    });
}

juce::File ChordMatePanel::writeMidi (const std::vector<cm::Note>& notes, int lengthTicks, const juce::String& label)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ChordMate");
    if (! dir.createDirectory()) return {};

    const double bpm = proc.hostBpm();
    const auto keyStr = U8 (cm::keyName (prj.key)).replaceCharacter (' ', '_').replace ("#", "s");
    const auto name = juce::File::createLegalFileName ("ChordMate_" + label + "_" + keyStr + "_"
                                                       + juce::String ((int) std::lround (bpm)) + "bpm") + ".mid";
    auto file = dir.getChildFile (name);
    const auto bytes = cm::buildMidiFile (notes, bpm, lengthTicks, ("ChordMate " + label).toStdString());
    return file.replaceWithData (bytes.data(), bytes.size()) ? file : juce::File();
}

juce::File ChordMatePanel::writeArrangementMidi()
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ChordMate");
    if (! dir.createDirectory()) return {};

    std::vector<cm::MidiTrack> tracks;
    int channel = 0;
    for (auto& p : prj.arrangement())
    {
        tracks.push_back ({ p.name, p.channel, p.program, p.notes });
        channel = std::max (channel, p.channel + 1);
    }
    if (! prj.melody.empty())
    {
        if (channel == 9) ++channel;
        const bool orchestral = prj.ensemble != cm::Ensemble::Piano;
        tracks.push_back ({ "Melodia", std::min (channel, 15), orchestral ? 73 : 80, prj.melody });
    }

    const double bpm = proc.hostBpm();
    const auto keyStr = U8 (cm::keyName (prj.key)).replaceCharacter (' ', '_').replace ("#", "s");
    const auto ens = U8 (cm::ensembleName (prj.ensemble)).replaceCharacter (' ', '_').replaceCharacter ('/', '-');
    const auto name = juce::File::createLegalFileName ("ChordMate_Arrangiamento_" + ens + "_" + keyStr + "_"
                                                       + juce::String ((int) std::lround (bpm)) + "bpm") + ".mid";
    auto file = dir.getChildFile (name);
    const auto bytes = cm::buildMidiFileMulti (tracks, bpm, prj.lengthTicks());
    return file.replaceWithData (bytes.data(), bytes.size()) ? file : juce::File();
}

void ChordMatePanel::saveMidiAs (const juce::File& tempFile)
{
    chooser = std::make_unique<juce::FileChooser> (
        U8 ("Salva il file MIDI"),
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (tempFile.getFileName()),
        "*.mid");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [tempFile] (const juce::FileChooser& fc)
                          {
                              const auto dest = fc.getResult();
                              if (dest != juce::File())
                                  tempFile.copyFileTo (dest.withFileExtension ("mid"));
                          });
}

// ====================================================================== timer / pannello live
void ChordMatePanel::updateLivePanel()
{
    const auto mask = proc.heldPitchClassMask();
    const bool flats = cm::keyUsesFlats (prj.key);

    juce::String notes;
    for (int pc = 0; pc < 12; ++pc)
        if ((mask >> pc) & 1) notes += U8 (cm::noteName (pc, flats)) + " ";

    const auto dc = cm::detectChord (mask, proc.lowestHeldPitchClass());
    liveChordHeld = dc.valid;
    if (dc.valid) liveChord = dc;

    const auto hist = proc.pitchClassHistogram();
    double sum = 0;
    for (double v : hist) sum += v;
    haveKeyGuess = false;
    juce::String keyText = U8 ("suona qualche nota…");
    if (sum > 1.5)
    {
        const auto g = cm::detectKey (hist);
        if (! g.empty())
        {
            lastKeyGuess = g[0];
            haveKeyGuess = true;
            keyText = U8 (cm::keyName (g[0].key)) + " (" + juce::String (juce::jlimit (0, 100, (int) std::lround (g[0].confidence * 100.0))) + "%)";
        }
    }

    juce::String chordText = U8 ("–");
    if (liveChord.valid)
        chordText = U8 (cm::chordName (liveChord.chord, prj.key)) + (liveChordHeld ? "" : U8 (" (ultimo)"));

    const auto text = U8 ("Note premute: ") + (notes.isEmpty() ? U8 ("–") : notes.trim())
                      + U8 ("     Accordo: ") + chordText + U8 ("     Tonalità probabile: ") + keyText;
    if (text != lastLiveText)
    {
        lastLiveText = text;
        liveLabel.setText (text, juce::dontSendNotification);
    }
    useKeyBtn.setEnabled (haveKeyGuess);
    addChordBtn.setEnabled (liveChord.valid && (int) prj.progression.size() < cm::kMaxChords);
}

void ChordMatePanel::timerCallback()
{
    if (const int rev = proc.projectRevision.load(); rev != lastRevision)
    {
        lastRevision = rev;
        rebuildPresetBox();
        syncControlsFromProject();
        refreshAll();
    }

    proc.pullCapturedNotes();
    if (const int rev = proc.captureRevision.load(); rev != lastCaptureRevision)
    {
        lastCaptureRevision = rev;
        refreshRoll();
        refreshCaptureInfo();
    }
    if (recBtn.getToggleState() != proc.isRecording())
    {
        recBtn.setToggleState (proc.isRecording(), juce::dontSendNotification);
        recBtn.setButtonText (proc.isRecording() ? U8 ("■ Stop registrazione") : U8 ("● Registra"));
    }

    updateLivePanel();

    bpmLabel.setText (juce::String ((int) std::lround (proc.hostBpm())) + " BPM", juce::dontSendNotification);

    const bool playing = proc.isPreviewPlaying();
    const auto want = playing ? U8 ("■  Stop") : U8 ("▶  Ascolta tutto");
    if (playBtn.getButtonText() != want) playBtn.setButtonText (want);

    const int tick = playing ? proc.previewPositionTick() : -1;
    roll.setPlayhead (tick);

    const int slotTicks = std::max (1, prj.stepsPerChord) * cm::kStepTicks;
    for (int i = 0; i < (int) prj.progression.size(); ++i)
    {
        const bool now = playing && tick >= 0 && (tick / slotTicks) == i;
        if (slots[(size_t) i].playing != now)
        {
            slots[(size_t) i].playing = now;
            slots[(size_t) i].repaint();
        }
    }
}

// ====================================================================== disegno e layout
void ChordMatePanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    auto header = [&] (juce::Rectangle<int> r, const juce::String& t)
    {
        g.setColour (ui::dim);
        g.setFont (ui::font (12.5f, true));
        g.drawText (t, r, juce::Justification::centredLeft);
        g.setColour (ui::edge);
        g.drawHorizontalLine (r.getBottom() - 1, (float) r.getX(), (float) r.getRight());
    };
    header (secProg, U8 ("PROGRESSIONE"));
    header (secMelody, U8 ("MELODIA"));
    header (secArrange, U8 ("ARRANGIAMENTO E POLIARMONIA"));
    header (secCapture, U8 ("CATTURA MIDI"));
    header (secExport, U8 ("ESPORTA IN LIVE"));
}

void ChordMatePanel::resized()
{
    auto area = getLocalBounds().reduced (14, 12);
    auto take = [&] (int h, int gap = 6)
    {
        auto r = area.removeFromTop (h);
        area.removeFromTop (gap);
        return r;
    };
    auto put = [] (juce::Rectangle<int>& row, juce::Component& c, int w, int trimTop = 0)
    {
        c.setBounds (row.removeFromLeft (w).withTrimmedTop (trimTop));
        row.removeFromLeft (6);
    };

    // barra superiore
    {
        auto r = take (36, 8);
        volume.setBounds (r.removeFromRight (120));
        r.removeFromRight (6);
        playBtn.setBounds (r.removeFromRight (124).reduced (0, 3));
        r.removeFromRight (6);
        bpmLabel.setBounds (r.removeFromRight (70));
        put (r, title, 110);
        put (r, rootBox, 74, 5);
        put (r, scaleBox, 168, 5);
        put (r, genreBox, 200, 5);
        put (r, colourBox, 150, 5);
        for (auto* c : { &rootBox, &scaleBox, &genreBox, &colourBox }) c->setSize (c->getWidth(), 26);
    }

    // ascolto live
    {
        auto r = take (40, 6);
        playThroughToggle.setBounds (r.removeFromRight (210));
        r.removeFromRight (6);
        addChordBtn.setBounds (r.removeFromRight (140).reduced (0, 6));
        r.removeFromRight (6);
        useKeyBtn.setBounds (r.removeFromRight (140).reduced (0, 6));
        r.removeFromRight (6);
        liveLabel.setBounds (r);
    }

    // progressione
    secProg = take (18, 4);
    {
        auto r = take (28, 6);
        put (r, presetBox, 380);
        put (r, applyPresetBtn, 110);
        put (r, lengthBox, 110);
        put (r, randomBtn, 130);
        put (r, reharmBtn, 120);
        put (r, clearBtn, 80);
    }
    {
        auto r = take (34, 6);
        put (r, spcBox, 190, 8);
        put (r, styleBox, 190, 8);
        put (r, bassToggle, 70, 8);
        put (r, adventure, 280);
    }
    {
        auto r = take (64, 8);
        const int w = r.getWidth() / (int) slots.size();
        for (auto& s : slots) s.setBounds (r.removeFromLeft (w));
    }
    suggTitle.setBounds (take (18, 2));
    {
        auto r = take (80, 8);
        const int w = r.getWidth() / (int) sugg.size();
        for (auto& s : sugg) s.setBounds (r.removeFromLeft (w));
    }

    // melodia
    secMelody = take (18, 4);
    {
        auto r = take (36, 6);
        put (r, energy, 180);
        put (r, movement, 180);
        put (r, tension, 180);
        put (r, contourBox, 180, 8);
        put (r, registerBox, 170, 8);
        put (r, melodyBtn, 140, 6);
    }
    roll.setBounds (take (124, 8));

    // arrangiamento
    secArrange = take (18, 4);
    {
        auto r = take (28, 4);
        put (r, ensembleBox, 220);
        put (r, polyBox, 260);
        partsInfo.setBounds (r);
    }
    polyInfo.setBounds (take (18, 6));

    // cattura
    secCapture = take (18, 4);
    {
        auto r = take (34, 6);
        put (r, recBtn, 100, 4);
        put (r, playCapBtn, 80, 4);
        put (r, clearCapBtn, 110, 4);
        put (r, quantBox, 100, 4);
        put (r, quantStrength, 130);
        put (r, snapToggle, 110, 4);
        put (r, quantLenToggle, 130, 4);
        put (r, harmonizeBtn, 150, 4);
    }
    capInfo.setBounds (take (18, 6));

    // esportazione
    secExport = take (18, 2);
    {
        auto r = take (46, 0);
        put (r, dragChords, 200);
        put (r, dragMelody, 200);
        put (r, dragArrangement, 220);
        put (r, dragCapture, 200);
        hint.setBounds (r);
    }
}

// ====================================================================== editor (contenitore scalabile)
ChordMateEditor::ChordMateEditor (ChordMateProcessor& p)
    : AudioProcessorEditor (&p), proc (p), panel (p)
{
    addAndMakeVisible (panel);

    const double w = ChordMatePanel::kWidth, h = ChordMatePanel::kHeight;

    // scala salvata nel progetto (letta prima di setResizeLimits, che chiama resized() e la sovrascrive);
    // se la finestra non entra nello schermo la riduce
    double scale = juce::jlimit (0.6, 2.0, (double) proc.uiScale.load());
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto area = d->userBounds;
        if (area.getWidth() > 200 && area.getHeight() > 200)
            scale = juce::jmax (0.6, juce::jmin (scale, (area.getHeight() - 80) / h, (area.getWidth() - 40) / w));
    }

    setResizable (true, true);
    setResizeLimits ((int) (w * 0.6), (int) (h * 0.6), (int) (w * 2.0), (int) (h * 2.0));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio (w / h);
    setSize ((int) std::lround (w * scale), (int) std::lround (h * scale));
}

ChordMateEditor::~ChordMateEditor() = default;

void ChordMateEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
}

void ChordMateEditor::resized()
{
    const float scale = (float) getWidth() / (float) ChordMatePanel::kWidth;
    panel.setBounds (0, 0, ChordMatePanel::kWidth, ChordMatePanel::kHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
    proc.uiScale.store (scale);
}
