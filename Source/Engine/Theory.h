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

// ------------------------------------------------------------ accordi
enum class Quality { Maj, Min, Dim, Aug, Sus2, Sus4, Maj7, Dom7, Min7, HalfDim7, Dim7, MinMaj7 };
constexpr int kNumQualities = 12;

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
                                     double adventure, int maxCount);

struct Preset { const char* name; std::vector<int> degrees; };
const std::vector<Preset>& presetsFor (const Key& k);
std::vector<Chord> chordsFromPreset (const Key& k, int presetIndex, bool sevenths);

struct ProgressionOptions
{
    Key key;
    int length = 4;
    bool sevenths = false;
    double adventure = 0.3;
    uint32_t seed = 1;
};
std::vector<Chord> generateProgression (const ProgressionOptions& o);

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

struct RenderOptions
{
    ChordStyle style = ChordStyle::Block;
    int stepsPerChord = 16;
    bool bass = true;
    int velocity = 88;
};
std::vector<Note> renderChords (const std::vector<Chord>& chords, const RenderOptions& o);

struct MelodyOptions
{
    Key key;
    int stepsPerChord = 16;
    double density = 0.5;     // 0..1
    double leaps = 0.3;       // 0 = a gradi congiunti, 1 = molti salti
    int lowNote = 60;
    int highNote = 84;
    uint32_t seed = 1;
    bool resolveEnd = true;
};
std::vector<Note> generateMelody (const std::vector<Chord>& chords, const MelodyOptions& o);

// ------------------------------------------------------------ strumenti MIDI
void quantizeNotes (std::vector<Note>& notes, int gridTicks, double strength, bool quantizeLength);
void snapNotesToScale (std::vector<Note>& notes, const Key& k);
std::vector<Chord> harmonizeMelody (const std::vector<Note>& melody, const Key& k, int stepsPerChord,
                                    int numChords, bool sevenths, double adventure);

} // namespace cm
