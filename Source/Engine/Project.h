#pragma once
#include "Style.h"
#include "Theory.h"
#include <string>
#include <vector>

namespace cm
{
// Stato completo del lavoro dell'utente: impostazioni + progressione + melodia + cattura MIDI.
// Nessuna dipendenza da JUCE: serializzabile in testo e testabile da riga di comando.
// Limiti applicati allo stato caricato da un progetto (che può provenire da un file non fidato).
constexpr int kMaxStoredNotes = 4096;
constexpr int kMaxStoredTick = 1 << 24;      // ~8700 battute a 480 PPQ: nessun overflow sommando inizio e durata
constexpr size_t kMaxStateBytes = 1 << 20;

struct Project
{
    // stile
    Genre genre = Genre::Universal;

    // armonia
    Key key;
    bool sevenths = false;                   // tenuto in sincronia con colour (colour >= 1)
    int colour = 0;                          // 0 = triadi, 1 = settime, 2 = estese
    double adventure = 0.3;
    int presetIndex = 0;
    int progLength = 4;
    int stepsPerChord = 16;                  // 8 = mezza battuta, 16 = 1 battuta, 32 = 2 battute
    ChordStyle chordStyle = ChordStyle::Block;
    bool bass = true;
    uint32_t progSeed = 1;

    // melodia: tre macro (energia = density, movimento = leaps, tensione) + contorno + registro
    double density = 0.5;
    double leaps = 0.3;
    double tension = 0.25;
    Contour contour = Contour::Free;
    int registerIndex = 2;
    int lowNote = 60;
    int highNote = 84;
    uint32_t melodySeed = 1;
    bool resolveEnd = true;

    // arrangiamento
    Ensemble ensemble = Ensemble::Piano;
    Polyharmony poly = Polyharmony::Off;

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
    void applyGenreDefaults();               // imposta colore, ritmo, melodia, ensemble... tipici del genere
    void setColour (int c);                  // cambia il colore e adatta la progressione
    void setRegister (int r);
    void applyPreset();
    void randomProgression();
    void reharmonizeNow();
    void generateMelodyNow();
    void harmonizeCapture();

    // risultati
    BassStyle bassStyle() const { return genreProfile (genre).bass; }
    std::vector<Part> arrangement() const;   // parti dell'ensemble (senza melodia)
    std::vector<Note> chordNotes() const;    // tutte le parti armoniche fuse
    std::vector<Note> processedCapture() const;
    std::vector<std::string> polychordNames() const;
    int lengthTicks() const;                 // lunghezza della progressione
    static int captureLengthTicks (const std::vector<Note>& notes);   // arrotondata alla battuta

    std::string serialize() const;
    bool deserialize (const std::string& text);
};
}
