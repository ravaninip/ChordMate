#pragma once
#include "Theory.h"
#include <string>
#include <vector>

namespace cm
{
// Stato completo del lavoro dell'utente: impostazioni + progressione + melodia + cattura MIDI.
// Nessuna dipendenza da JUCE: serializzabile in testo e testabile da riga di comando.
struct Project
{
    // armonia
    Key key;
    bool sevenths = false;
    double adventure = 0.3;
    int presetIndex = 0;
    int progLength = 4;
    int stepsPerChord = 16;                  // 8 = mezza battuta, 16 = 1 battuta, 32 = 2 battute
    ChordStyle chordStyle = ChordStyle::Block;
    bool bass = true;
    uint32_t progSeed = 1;

    // melodia
    double density = 0.5;
    double leaps = 0.3;
    int lowNote = 60;
    int highNote = 84;
    uint32_t melodySeed = 1;
    bool resolveEnd = true;

    // cattura
    int quantGrid = kStepTicks;              // 0 = off
    double quantStrength = 1.0;
    bool quantLength = false;
    bool snapToScale = false;

    double volume = 0.7;
    double uiScale = 1.0;                    // scala della finestra del plugin

    std::vector<Chord> progression;
    std::vector<Note> melody;
    std::vector<Note> capture;

    // azioni
    void applyPreset();
    void randomProgression();
    void generateMelodyNow();
    void harmonizeCapture();

    // risultati
    std::vector<Note> chordNotes() const;
    std::vector<Note> processedCapture() const;
    int lengthTicks() const;                 // lunghezza della progressione
    static int captureLengthTicks (const std::vector<Note>& notes);   // arrotondata alla battuta

    std::string serialize() const;
    bool deserialize (const std::string& text);
};
}
