#pragma once
// ChordMate - generi (macro-stili), poliarmonia e arrangiamento per ensemble (C++17 puro, niente JUCE)
#include "Theory.h"

#include <array>
#include <string>
#include <vector>

namespace cm
{
// ------------------------------------------------------------ ensemble e poliarmonia
enum class Ensemble { Piano, StringQuartet, StringOrchestra, FullOrchestra, Choir, Brass };
constexpr int kNumEnsembles = 6;
const char* ensembleName (Ensemble e);
Ensemble ensembleFromIndex (int i);

// Off = accordi così come sono · Extensions = 9/11/13 in scala nelle voci superiori
// UpperTriad = poliaccordo: una triade diversa sopra l'accordo (es. D/C = Cmaj7(9,♯11,13))
enum class Polyharmony { Off, Extensions, UpperTriad };
constexpr int kNumPolyharmony = 3;
const char* polyharmonyName (Polyharmony p);
Polyharmony polyharmonyFromIndex (int i);

// ------------------------------------------------------------ profilo di genere
struct GenreExtra { const char* roman; int fromMask; int weight; const char* reason; };
struct StylePreset { const char* name; const char* romans; };

struct GenreProfile
{
    const char* name;
    const char* description;

    // armonia
    std::vector<StylePreset> major, minor;      // preset in numeri romani (vuoti per Universale)
    std::array<double, 7> degreeMul;            // peso dei gradi diatonici come destinazione (maggiore)
    std::array<double, 7> degreeMulMinor;       // idem in tonalità minore (i ii° III iv v VI VII)
    double colourMul;                           // quanto spingere gli accordi "colore"
    bool tritoneSubs;                           // la rearmonizzazione può usare ♭II7
    std::vector<GenreExtra> majorExtras, minorExtras;

    // default applicati quando scegli il genere
    int colour;
    ChordStyle chordStyle;
    int stepsPerChord;
    BassStyle bass;
    double adventure;

    // melodia
    bool customRhythm;                          // false = ritmo "universale" storico
    std::array<double, 16> rhythm;              // probabilità di attacco per sedicesimo (a energia 0,5)
    double legato;                              // frazione della distanza dalla nota successiva
    double swing;                               // 0..1
    double motif;                               // quanto si ripete il motivo (0..1)
    double energy, movement, tension;
    Contour contour;
    int reg;                                    // registro (vedi registerName)

    Ensemble ensemble;
    Polyharmony poly;
};

const GenreProfile& genreProfile (Genre g);
const char* genreName (Genre g);

// preset per genere (Universale = preset storici di presetsFor)
std::vector<std::string> presetNames (Genre g, const Key& k);
std::vector<Chord> presetChords (Genre g, const Key& k, int index, int colour);

// ------------------------------------------------------------ poliarmonia
struct UpperTriad { bool valid = false; Chord triad; };
// triade superiore tutta nella scala, con almeno una nota nuova rispetto all'accordo
UpperTriad upperStructure (const Chord& c, const Key& k);
std::string polychordName (const Chord& base, const UpperTriad& u, const Key& k);

// ------------------------------------------------------------ voice leading a 4 parti
struct VoiceRange { int low, high; };
// per ogni accordo: { basso, tenore, contralto, soprano } con condotta delle parti classica
// (basso sulla fondamentale, terza e settima sempre presenti, niente quinte/ottave parallele se evitabili)
std::vector<std::array<int, 4>> voiceSATB (const std::vector<Chord>& chords, const std::array<VoiceRange, 4>& ranges);

// ------------------------------------------------------------ arrangiamento
struct Part
{
    std::string name;
    int channel = 0;          // 0-15
    int program = -1;         // General MIDI (0-127), -1 = nessuno
    std::vector<Note> notes;
};

struct ArrangeOptions
{
    Key key;
    Ensemble ensemble = Ensemble::Piano;
    Polyharmony poly = Polyharmony::Off;
    ChordStyle style = ChordStyle::Block;
    int stepsPerChord = 16;
    bool bass = true;
    BassStyle bassStyle = BassStyle::Sustained;
    double swing = 0.0;
};

std::vector<Part> arrange (const std::vector<Chord>& chords, const ArrangeOptions& o);
std::vector<Note> mergeParts (const std::vector<Part>& parts);

} // namespace cm
