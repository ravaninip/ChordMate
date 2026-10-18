#pragma once
// ChordMate - motore di teoria musicale (C++17 puro, nessuna dipendenza da JUCE)
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace cm
{
constexpr int kPPQ = 480;                 // tick per quarto
constexpr int kStepTicks = kPPQ / 4;      // una sedicesima
constexpr int kMaxChords = 8;

// ------------------------------------------------------------ RNG deterministico
class Rng
{
public:
    explicit Rng (uint32_t seed) : s (uint64_t (seed) * 2654435761ull + 0x9E3779B97F4A7C15ull) {}
    uint32_t nextU32()
    {
        s += 0x9E3779B97F4A7C15ull;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= z >> 31;
        return uint32_t (z >> 32);
    }
    double uni() { return nextU32() / 4294967296.0; }
private:
    uint64_t s;
};

// ------------------------------------------------------------ scale
enum class ScaleType { Major, NaturalMinor, Dorian, Phrygian, Lydian, Mixolydian,
                       HarmonicMinor, MelodicMinor, MajorPentatonic, MinorPentatonic, Blues };
constexpr int kNumScales = 11;

struct ScaleInfo { const char* name; std::vector<int> intervals; };
const ScaleInfo& scaleInfo (ScaleType t);
ScaleType scaleFromIndex (int i);
ScaleType harmonicParent (ScaleType t);   // scala a 7 note usata per costruire gli accordi
bool isMinorFamily (ScaleType t);

struct Key
{
    int root = 0;                         // 0 = C ... 11 = B
    ScaleType scale = ScaleType::Major;
};

bool inScale (int pitchClass, const Key& k);
int degreeOf (int pitchClass, const Key& k);          // posizione nella scala parente (0-6) o -1
bool keyUsesFlats (const Key& k);
std::string noteName (int pitchClass, bool flats);
std::string keyName (const Key& k);

// ------------------------------------------------------------ generi (macro-stili)
// Influenzano preset, suggerimenti, ritmo della melodia, basso e arrangiamento (vedi Style.h).
enum class Genre { Universal, Pop, Rock, Jazz, Soul, Electronic, HipHop, Cinematic, Classical, Latin, Ambient };
constexpr int kNumGenres = 11;
Genre genreFromIndex (int i);

// ------------------------------------------------------------ accordi
// Le prime 12 qualità sono quelle storiche (indici salvati nei progetti): le nuove vanno solo in coda.
enum class Quality { Maj, Min, Dim, Aug, Sus2, Sus4, Maj7, Dom7, Min7, HalfDim7, Dim7, MinMaj7,
                     Add9, MinAdd9, Maj9, Min9, Dom9, Six, Min6, Dom7Sus4, Dom7b9 };
constexpr int kNumQualities = 21;
constexpr int kNumBasicQualities = 12;

// colore degli accordi: 0 = triadi, 1 = settime, 2 = estese (9ª solo se in scala)
constexpr int kNumColours = 3;

const std::vector<int>& qualityIntervals (Quality q);
const char* qualitySuffix (Quality q);

struct Chord
{
    int root = 0;
    Quality quality = Quality::Maj;
    bool operator== (const Chord& o) const { return root == o.root && quality == o.quality; }
    bool operator!= (const Chord& o) const { return ! (*this == o); }
};

std::vector<int> chordPitchClasses (const Chord& c);
std::string chordName (const Chord& c, const Key& k);
std::string romanNumeral (const Chord& c, const Key& k);
Chord chordAtDegree (const Key& k, int degree, bool sevenths, bool raiseDominantInMinor = false);
std::vector<Chord> diatonicChords (const Key& k, bool sevenths);
bool isDiatonicChord (const Chord& c, const Key& k);

bool isMinorQuality (Quality q);
bool isDominantQuality (Quality q);
// Adatta un accordo al livello di colore (triade / settima / estesa) restando nella tonalità:
// la 9ª si aggiunge solo se è nella scala, altrimenti resta la settima (o 7♭9 sulle dominanti).
Chord adaptColour (const Chord& c, const Key& k, int colour);
// "ii7", "♭VII", "bVImaj7", "V7b9", "#iv°7"... -> accordo (radice relativa alla scala maggiore della tonica)
bool parseRoman (const std::string& token, const Key& k, Chord& out);

// ------------------------------------------------------------ analisi
struct KeyGuess { Key key; double confidence = 0; };
// istogramma delle 12 classi di altezza -> tonalità più probabili (ordinate)
std::vector<KeyGuess> detectKey (const std::array<double, 12>& histogram);

struct DetectedChord { bool valid = false; Chord chord; };
DetectedChord detectChord (uint16_t pitchClassMask, int bassPitchClass = -1);

// ------------------------------------------------------------ suggerimenti
struct Suggestion
{
    Chord chord;
    double score = 0;
    int degree = -1;          // grado diatonico, -1 se accordo "colore"
    bool borrowed = false;    // prestito modale / dominante secondaria ecc.
    std::string reason;
};

std::vector<Suggestion> suggestNext (const Chord& from, const Key& k, bool sevenths,
                                     double adventure, int maxCount, Genre genre = Genre::Universal, int colour = -1);

struct Preset { const char* name; std::vector<int> degrees; };
const std::vector<Preset>& presetsFor (const Key& k);
std::vector<Chord> chordsFromPreset (const Key& k, int presetIndex, bool sevenths);

struct ProgressionOptions
{
    Key key;
    int length = 4;
    bool sevenths = false;
    int colour = -1;          // -1 = deriva da sevenths
    double adventure = 0.3;
    uint32_t seed = 1;
    Genre genre = Genre::Universal;
};
std::vector<Chord> generateProgression (const ProgressionOptions& o);

// Rearmonizzazione: sostituisce alcuni accordi con altri della stessa funzione (tonica, sottodominante,
// dominante), dominanti secondarie o sostituzioni di tritono, secondo il genere. Primo e ultimo restano.
std::vector<Chord> reharmonize (const std::vector<Chord>& chords, const Key& k, int colour, double adventure,
                                Genre genre, uint32_t seed);

// ------------------------------------------------------------ note e voicing
struct Note
{
    int pitch = 60;
    int startTick = 0;
    int lengthTick = kStepTicks;
    int velocity = 90;
    bool operator== (const Note& o) const
    {
        return pitch == o.pitch && startTick == o.startTick && lengthTick == o.lengthTick && velocity == o.velocity;
    }
};

std::vector<int> voiceChord (const Chord& c, const std::vector<int>* previous, int centre = 60);
std::vector<std::vector<int>> voiceProgression (const std::vector<Chord>& chords, int centre = 60);

enum class ChordStyle { Block, Arp16, ArpUpDown8, Pulse8, Offbeat };
constexpr int kNumChordStyles = 5;
const char* chordStyleName (ChordStyle s);

// figura del basso (scelta dal genere): Sustained = una nota lunga per accordo
enum class BassStyle { Sustained, Root8, RootFifth, Walking, Syncopated, Octaves };
constexpr int kNumBassStyles = 6;
const char* bassStyleName (BassStyle s);

struct RenderOptions
{
    ChordStyle style = ChordStyle::Block;
    int stepsPerChord = 16;
    bool bass = true;
    BassStyle bassStyle = BassStyle::Sustained;
    Key key;                  // serve al basso walking (note di passaggio in scala)
    int velocity = 88;
};
std::vector<Note> renderChords (const std::vector<Chord>& chords, const RenderOptions& o);
std::vector<Note> renderBass (const std::vector<Chord>& chords, const RenderOptions& o);

// contorno della frase melodica
enum class Contour { Free, Arch, Rising, Falling, Wave };
constexpr int kNumContours = 5;
const char* contourName (Contour c);

// registro della melodia (imposta nota più bassa / più alta)
constexpr int kNumRegisters = 4;
const char* registerName (int r);
void registerRange (int r, int& low, int& high);

struct MelodyOptions
{
    Key key;
    int stepsPerChord = 16;
    double density = 0.5;     // "Energia": 0..1 (note più fitte, più corte, più accentate)
    double leaps = 0.3;       // "Movimento": 0 = a gradi congiunti, 1 = molti salti
    double tension = 0.25;    // "Tensione": 0 = solo note d'accordo sui tempi forti, 1 = appoggiature e cromatismi
    Contour contour = Contour::Free;
    Genre genre = Genre::Universal;   // ritmo, swing, ripetizione del motivo
    int lowNote = 60;
    int highNote = 84;
    uint32_t seed = 1;
    bool resolveEnd = true;
};
std::vector<Note> generateMelody (const std::vector<Chord>& chords, const MelodyOptions& o);

// sposta in avanti gli ottavi in levare (swing): amount 0..1 (1 = terzina piena)
void applySwing (std::vector<Note>& notes, double amount);

// ------------------------------------------------------------ strumenti MIDI
void quantizeNotes (std::vector<Note>& notes, int gridTicks, double strength, bool quantizeLength);
void snapNotesToScale (std::vector<Note>& notes, const Key& k);
std::vector<Chord> harmonizeMelody (const std::vector<Note>& melody, const Key& k, int stepsPerChord,
                                    int numChords, bool sevenths, double adventure,
                                    Genre genre = Genre::Universal, int colour = -1);

} // namespace cm
