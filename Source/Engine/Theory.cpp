#include "Theory.h"
#include "Style.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace cm
{
namespace
{
int mod12 (int x) { return ((x % 12) + 12) % 12; }

int popcount (uint32_t v)
{
    int n = 0;
    while (v) { n += int (v & 1u); v >>= 1; }
    return n;
}

const ScaleInfo kScales[kNumScales] = {
    { "maggiore",             { 0, 2, 4, 5, 7, 9, 11 } },
    { "minore naturale",      { 0, 2, 3, 5, 7, 8, 10 } },
    { "dorica",               { 0, 2, 3, 5, 7, 9, 10 } },
    { "frigia",               { 0, 1, 3, 5, 7, 8, 10 } },
    { "lidia",                { 0, 2, 4, 6, 7, 9, 11 } },
    { "misolidia",            { 0, 2, 4, 5, 7, 9, 10 } },
    { "minore armonica",      { 0, 2, 3, 5, 7, 8, 11 } },
    { "minore melodica",      { 0, 2, 3, 5, 7, 9, 11 } },
    { "pentatonica maggiore", { 0, 2, 4, 7, 9 } },
    { "pentatonica minore",   { 0, 3, 5, 7, 10 } },
    { "blues",                { 0, 3, 5, 6, 7, 10 } },
};

const std::vector<int> kQualityIntervals[kNumQualities] = {
    { 0, 4, 7 }, { 0, 3, 7 }, { 0, 3, 6 }, { 0, 4, 8 }, { 0, 2, 7 }, { 0, 5, 7 },
    { 0, 4, 7, 11 }, { 0, 4, 7, 10 }, { 0, 3, 7, 10 }, { 0, 3, 6, 10 }, { 0, 3, 6, 9 }, { 0, 3, 7, 11 },
    { 0, 4, 7, 14 }, { 0, 3, 7, 14 }, { 0, 4, 7, 11, 14 }, { 0, 3, 7, 10, 14 }, { 0, 4, 7, 10, 14 },
    { 0, 4, 7, 9 }, { 0, 3, 7, 9 }, { 0, 5, 7, 10 }, { 0, 4, 7, 10, 13 },
};

const char* const kQualitySuffix[kNumQualities] = {
    "", "m", "dim", "aug", "sus2", "sus4", "maj7", "7", "m7", "m7b5", "dim7", "m(maj7)",
    "add9", "m(add9)", "maj9", "m9", "9", "6", "m6", "7sus4", "7b9",
};

// nota nella scala "parente" a 7 note (per le pentatoniche: la scala da cui derivano)
bool inParentScale (int pc, const Key& k)
{
    return inScale (pc, Key { k.root, harmonicParent (k.scale) });
}

// riduzione a triade di qualsiasi qualità
Quality triadOf (Quality q)
{
    switch (q)
    {
        case Quality::Min: case Quality::Min7: case Quality::MinMaj7: case Quality::MinAdd9:
        case Quality::Min9: case Quality::Min6:                       return Quality::Min;
        case Quality::Dim: case Quality::HalfDim7: case Quality::Dim7: return Quality::Dim;
        case Quality::Aug:                                            return Quality::Aug;
        case Quality::Sus2:                                           return Quality::Sus2;
        case Quality::Sus4: case Quality::Dom7Sus4:                   return Quality::Sus4;
        default:                                                      return Quality::Maj;
    }
}

Quality classify (int third, int fifth, int seventh)
{
    if (seventh >= 0)
    {
        if (third == 4 && fifth == 7 && seventh == 11) return Quality::Maj7;
        if (third == 4 && fifth == 7 && seventh == 10) return Quality::Dom7;
        if (third == 3 && fifth == 7 && seventh == 10) return Quality::Min7;
        if (third == 3 && fifth == 6 && seventh == 10) return Quality::HalfDim7;
        if (third == 3 && fifth == 6 && seventh == 9)  return Quality::Dim7;
        if (third == 3 && fifth == 7 && seventh == 11) return Quality::MinMaj7;
    }
    if (third == 4 && fifth == 7) return Quality::Maj;
    if (third == 3 && fifth == 7) return Quality::Min;
    if (third == 3 && fifth == 6) return Quality::Dim;
    if (third == 4 && fifth == 8) return Quality::Aug;
    return Quality::Maj;
}

int majorEquivalentTonic (const Key& k)
{
    switch (k.scale)
    {
        case ScaleType::Major: case ScaleType::MajorPentatonic: return k.root;
        case ScaleType::Dorian:     return mod12 (k.root + 10);
        case ScaleType::Phrygian:   return mod12 (k.root + 8);
        case ScaleType::Lydian:     return mod12 (k.root + 7);
        case ScaleType::Mixolydian: return mod12 (k.root + 5);
        default:                    return mod12 (k.root + 3);   // famiglia minore
    }
}

// ----------------------------------------------------------------- tabelle di transizione
// indici: 0=I 1=ii 2=iii 3=IV 4=V 5=vi 6=vii
const int kMajorT[7][7] = {
    { 0, 5, 3, 8, 8, 7, 1 },
    { 2, 0, 1, 2, 10, 2, 1 },
    { 2, 4, 0, 7, 3, 8, 1 },
    { 7, 5, 1, 0, 9, 3, 1 },
    { 10, 1, 2, 4, 0, 7, 0 },
    { 3, 8, 4, 8, 6, 0, 0 },
    { 10, 0, 4, 0, 0, 3, 0 },
};
// indici: 0=i 1=ii° 2=III 3=iv 4=v 5=VI 6=VII
const int kMinorT[7][7] = {
    { 0, 2, 6, 8, 5, 8, 7 },
    { 3, 0, 0, 2, 9, 0, 2 },
    { 4, 2, 0, 6, 2, 8, 7 },
    { 7, 0, 2, 0, 8, 4, 6 },
    { 10, 0, 2, 3, 0, 7, 0 },
    { 4, 0, 5, 7, 6, 0, 9 },
    { 9, 0, 8, 4, 0, 5, 0 },
};

struct Extra
{
    int offset;           // semitoni dalla tonica
    Quality triad;
    Quality seventh;
    int fromMask;         // gradi di partenza (bit 0 = grado I)
    int weight;
    const char* reason;
    bool colour;          // true = si propone solo con "avventura" > 0
};

const Extra kMajorExtras[] = {
    { 10, Quality::Maj, Quality::Maj7, 0x19, 5, "Prestito modale ♭VII (misolidio): suono rock/epico", true },
    { 5,  Quality::Min, Quality::Min7, 0x09, 5, "Prestito modale iv (dal minore): malinconia agrodolce", true },
    { 8,  Quality::Maj, Quality::Maj7, 0x09, 3, "Prestito modale ♭VI: drammatico, cinematografico", true },
    { 4,  Quality::Maj, Quality::Dom7, 0x0D, 4, "Dominante secondaria V/vi: spinge verso vi", true },
    { 9,  Quality::Maj, Quality::Dom7, 0x29, 3, "Dominante secondaria V/ii: spinge verso ii", true },
    { 2,  Quality::Maj, Quality::Dom7, 0x2B, 4, "Dominante secondaria V/V: spinge verso V", true },
    { 1,  Quality::Maj, Quality::Dom7, 0x02, 2, "Sostituzione di tritono ♭II7: scende sulla tonica", true },
};

const Extra kMinorExtras[] = {
    { 7, Quality::Maj, Quality::Dom7, 0x6F, 9, "Dominante V (dalla minore armonica): cadenza forte", false },
    { 5, Quality::Maj, Quality::Maj7, 0x01, 3, "Colore dorico: IV maggiore, più luminoso", true },
    { 1, Quality::Maj, Quality::Maj7, 0x09, 3, "Napoletana ♭II: tensione classica prima della dominante", true },
};

struct Candidate
{
    Chord chord;
    int degree = -1;          // grado diatonico, -1 = accordo "colore"
    bool isExtra = false;
    int fromMask = 0;
    double weight = 0;
    bool colour = false;
    const char* reason = nullptr;
};

int colourOf (bool sevenths, int colour) { return colour >= 0 ? std::clamp (colour, 0, kNumColours - 1) : (sevenths ? 1 : 0); }

std::vector<Candidate> buildCandidates (const Key& k, bool sevenths, Genre genre = Genre::Universal, int colour = -1)
{
    const int col = colourOf (sevenths, colour);
    const bool sev = col >= 1;
    std::vector<Candidate> out;
    for (int d = 0; d < 7; ++d)
    {
        Chord c = chordAtDegree (k, d, sev, false);
        if (col == 2) c = adaptColour (c, k, 2);
        out.push_back ({ c, d, false, 0, 0, false, nullptr });
    }

    auto addExtra = [&] (const Chord& c, int fromMask, double weight, bool isColour, const char* reason)
    {
        for (auto& o : out)
            if (o.chord == c)
            {
                if (o.isExtra) { o.fromMask |= fromMask; o.weight = std::max (o.weight, weight); }
                return;
            }
        out.push_back ({ c, -1, true, fromMask, weight, isColour, reason });
    };

    const bool minorFam = isMinorFamily (k.scale);
    const Extra* ex = minorFam ? kMinorExtras : kMajorExtras;
    const int n = minorFam ? int (sizeof (kMinorExtras) / sizeof (Extra)) : int (sizeof (kMajorExtras) / sizeof (Extra));

    for (int i = 0; i < n; ++i)
    {
        Chord c { mod12 (k.root + ex[i].offset), sev ? ex[i].seventh : ex[i].triad };
        if (col == 2) c = adaptColour (c, k, 2);
        addExtra (c, ex[i].fromMask, ex[i].weight, ex[i].colour, ex[i].reason);
    }

    const auto& gp = genreProfile (genre);
    for (const auto& ge : (minorFam ? gp.minorExtras : gp.majorExtras))
    {
        Chord c;
        if (! parseRoman (ge.roman, k, c)) continue;
        addExtra (adaptColour (c, k, col), ge.fromMask, ge.weight, true, ge.reason);
    }
    return out;
}

Candidate candidateFor (const Chord& c, const Key& k)
{
    const int d = degreeOf (c.root, k);
    if (d >= 0 && isDiatonicChord (c, k)) return { c, d, false, 0, 0, false, nullptr };
    return { c, -1, true, 0x7F, 3, false, "" };
}

const char* degreeReason (int deg, bool minorFam)
{
    static const char* maj[7] = {
        "Tonica: casa, scioglie la tensione",
        "Sopradominante: prepara la dominante",
        "Mediante: colore morbido, vicina alla tonica",
        "Sottodominante: apre e si allontana dalla tonica",
        "Dominante: massima tensione, chiede risoluzione",
        "Relativa: tonica alternativa, più malinconica",
        "Sensibile: tensione che spinge alla tonica" };
    static const char* mnr[7] = {
        "Tonica: casa, scioglie la tensione",
        "Sopradominante diminuita: prepara la dominante",
        "Mediante: relativa maggiore, schiarisce",
        "Sottodominante minore: colore scuro",
        "Quinto grado minore: tensione morbida",
        "Sesto grado: luminoso, epico",
        "Settimo grado: spinge verso la tonica" };
    const size_t i = size_t (std::clamp (deg, 0, 6));
    return minorFam ? mnr[i] : maj[i];
}

// peso (0 = escluso) di una transizione from -> candidato, con motivazione
double weightFrom (const Chord& from, const Candidate& c, const Key& k, double adventure, std::string* reason,
                   Genre genre = Genre::Universal)
{
    if (from == c.chord) return 0.0;

    const auto& gp = genreProfile (genre);
    const bool minorFam = isMinorFamily (k.scale);
    const int fromDeg = degreeOf (from.root, k);
    const double adv = std::clamp (adventure, 0.0, 1.0);
    const bool resolves = c.chord.root == mod12 (from.root + 5) || c.chord.root == mod12 (from.root - 1);
    const bool fromDiatonic = isDiatonicChord (from, k);
    const bool fromDomLike = from.quality == Quality::Maj || isDominantQuality (from.quality);

    double w = 0.0;
    const char* why = nullptr;

    if (fromDiatonic || (! fromDomLike && fromDeg >= 0))
    {
        if (! c.isExtra)
        {
            w = (minorFam ? kMinorT : kMajorT)[fromDeg][c.degree];
            if (minorFam && c.degree == 4 && harmonicParent (k.scale) == ScaleType::NaturalMinor)
                w = (w + 1) / 2.0;
            w *= (minorFam ? gp.degreeMulMinor : gp.degreeMul)[size_t (c.degree)];
            why = degreeReason (c.degree, minorFam);
            if (fromDeg == 4 && c.degree == 0) why = "Cadenza perfetta V→I: risolve la tensione";
            else if (fromDeg == 4 && c.degree == 5 && ! minorFam) why = "Cadenza d'inganno V→vi: risoluzione a sorpresa";
            else if (fromDeg == 3 && c.degree == 0) why = "Cadenza plagale IV→I: risoluzione morbida";
        }
        else if ((c.fromMask >> fromDeg) & 1)
        {
            w = c.weight * (c.colour ? 1.6 * adv * gp.colourMul : 1.0);
            why = c.reason;
        }
    }
    else if (fromDomLike)
    {
        // accordo non diatonico di tipo dominante: ci si aspetta la risoluzione
        if (! c.isExtra)
        {
            if (resolves) { w = 10; why = "Risoluzione: la dominante scende di quinta"; }
            else          { w = 1.0; why = degreeReason (c.degree, minorFam); }
        }
    }
    else if (! c.isExtra)
    {
        w = 2.5;
        why = degreeReason (c.degree, minorFam);
    }

    if (reason != nullptr && why != nullptr) *reason = why;
    return w;
}

} // namespace

// ============================================================ scale
const ScaleInfo& scaleInfo (ScaleType t) { return kScales[int (t)]; }
ScaleType scaleFromIndex (int i) { return ScaleType (std::clamp (i, 0, kNumScales - 1)); }

ScaleType harmonicParent (ScaleType t)
{
    switch (t)
    {
        case ScaleType::MajorPentatonic: return ScaleType::Major;
        case ScaleType::MinorPentatonic:
        case ScaleType::Blues:           return ScaleType::NaturalMinor;
        default:                         return t;
    }
}

bool isMinorFamily (ScaleType t)
{
    switch (harmonicParent (t))
    {
        case ScaleType::NaturalMinor: case ScaleType::Dorian: case ScaleType::Phrygian:
        case ScaleType::HarmonicMinor: case ScaleType::MelodicMinor: return true;
        default: return false;
    }
}

bool inScale (int pc, const Key& k)
{
    for (int iv : scaleInfo (k.scale).intervals)
        if (mod12 (k.root + iv) == mod12 (pc)) return true;
    return false;
}

int degreeOf (int pc, const Key& k)
{
    const auto& iv = scaleInfo (harmonicParent (k.scale)).intervals;
    for (int i = 0; i < int (iv.size()); ++i)
        if (mod12 (k.root + iv[size_t (i)]) == mod12 (pc)) return i;
    return -1;
}

bool keyUsesFlats (const Key& k)
{
    switch (majorEquivalentTonic (k))
    {
        case 5: case 10: case 3: case 8: case 1: case 6: return true;
        default: return false;
    }
}

std::string noteName (int pc, bool flats)
{
    static const char* sh[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* fl[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    return (flats ? fl : sh)[mod12 (pc)];
}

std::string keyName (const Key& k)
{
    return noteName (k.root, keyUsesFlats (k)) + " " + scaleInfo (k.scale).name;
}

// ============================================================ accordi
const std::vector<int>& qualityIntervals (Quality q) { return kQualityIntervals[int (q)]; }
const char* qualitySuffix (Quality q) { return kQualitySuffix[int (q)]; }

std::vector<int> chordPitchClasses (const Chord& c)
{
    std::vector<int> out;
    for (int iv : qualityIntervals (c.quality)) out.push_back (mod12 (c.root + iv));
    return out;
}

std::string chordName (const Chord& c, const Key& k)
{
    // accordi fuori scala (♭VII, ♭VI, napoletana...) si scrivono con i bemolli
    const bool flats = keyUsesFlats (k) || ! inScale (c.root, k);
    return noteName (c.root, flats) + qualitySuffix (c.quality);
}

Chord chordAtDegree (const Key& k, int degree, bool sevenths, bool raiseDominantInMinor)
{
    const auto& iv = scaleInfo (harmonicParent (k.scale)).intervals;
    degree = std::clamp (degree, 0, 6);
    auto at = [&] (int step)
    {
        const int idx = (degree + step) % 7;
        const int oct = (degree + step) / 7;
        return iv[size_t (idx)] + 12 * oct;
    };
    const int r = at (0);
    Chord c;
    c.root = mod12 (k.root + iv[size_t (degree)]);
    c.quality = classify (at (2) - r, at (4) - r, sevenths ? at (6) - r : -1);
    if (raiseDominantInMinor && degree == 4 && harmonicParent (k.scale) == ScaleType::NaturalMinor)
        c.quality = sevenths ? Quality::Dom7 : Quality::Maj;
    return c;
}

std::vector<Chord> diatonicChords (const Key& k, bool sevenths)
{
    std::vector<Chord> out;
    for (int d = 0; d < 7; ++d) out.push_back (chordAtDegree (k, d, sevenths, false));
    return out;
}

bool isDiatonicChord (const Chord& c, const Key& k)
{
    const int d = degreeOf (c.root, k);
    if (d < 0) return false;
    if (c == chordAtDegree (k, d, false) || c == chordAtDegree (k, d, true)
        || c == chordAtDegree (k, d, false, true) || c == chordAtDegree (k, d, true, true))
        return true;
    if (int (c.quality) < kNumBasicQualities) return false;
    // accordi estesi (9ª, 6ª, add9...): diatonici se la triade lo è e le note aggiunte sono in scala
    const Chord tri { c.root, triadOf (c.quality) };
    if (tri != chordAtDegree (k, d, false) && tri != chordAtDegree (k, d, false, true)) return false;
    for (int iv : qualityIntervals (c.quality))
        if (iv > 7 && ! inParentScale (c.root + iv, k)) return false;
    return true;
}

Genre genreFromIndex (int i) { return Genre (std::clamp (i, 0, kNumGenres - 1)); }

bool isMinorQuality (Quality q)
{
    return triadOf (q) == Quality::Min || triadOf (q) == Quality::Dim;
}

bool isDominantQuality (Quality q)
{
    return q == Quality::Dom7 || q == Quality::Dom9 || q == Quality::Dom7b9 || q == Quality::Dom7Sus4;
}

Chord adaptColour (const Chord& c, const Key& k, int colour)
{
    colour = std::clamp (colour, 0, kNumColours - 1);
    const int r = c.root;
    auto in = [&] (int iv) { return inParentScale (r + iv, k); };
    Chord out = c;

    if (colour == 0)
    {
        out.quality = triadOf (c.quality);
        return out;
    }

    // livello 1: settima (le dominanti restano dominanti, le altre prendono la settima della scala)
    switch (c.quality)
    {
        case Quality::Maj: case Quality::Add9: case Quality::Maj9: case Quality::Six:
            out.quality = (c.quality == Quality::Maj9 || in (11)) ? Quality::Maj7 : Quality::Dom7; break;
        case Quality::Min: case Quality::MinAdd9: case Quality::Min9: case Quality::Min6:
            out.quality = (in (10) || ! in (11)) ? Quality::Min7 : Quality::MinMaj7; break;
        case Quality::Dim:
            out.quality = in (10) ? Quality::HalfDim7 : Quality::Dim7; break;
        case Quality::Sus4:
            out.quality = in (10) ? Quality::Dom7Sus4 : Quality::Sus4; break;
        case Quality::Dom9: case Quality::Dom7b9:
            out.quality = Quality::Dom7; break;
        default: break;
    }
    if (colour == 1)
    {
        // a livello 1 le "colorazioni" esplicite (6, add9) tornano settime; il resto è già a posto
        return out;
    }

    // livello 2: le qualità di colore esplicite restano; le settime prendono la 9ª se è in scala
    switch (c.quality)
    {
        case Quality::Six: case Quality::Min6: case Quality::Add9: case Quality::MinAdd9:
        case Quality::Maj9: case Quality::Min9: case Quality::Dom9: case Quality::Dom7b9:
            return c;
        default: break;
    }
    switch (out.quality)
    {
        case Quality::Maj7: if (in (2)) out.quality = Quality::Maj9; break;
        case Quality::Min7: if (in (2)) out.quality = Quality::Min9; break;
        case Quality::Dom7:
            if (in (2)) out.quality = Quality::Dom9;
            else if (in (1)) out.quality = Quality::Dom7b9;
            break;
        default: break;
    }
    return out;
}

bool parseRoman (const std::string& token, const Key& k, Chord& out)
{
    size_t i = 0;
    int acc = 0;
    for (;;)
    {
        if (token.compare (i, 1, "b") == 0 && i + 1 < token.size() && (token[i + 1] == 'I' || token[i + 1] == 'V'
                                                                       || token[i + 1] == 'i' || token[i + 1] == 'v'))
        { --acc; i += 1; }
        else if (token.compare (i, 1, "#") == 0) { ++acc; i += 1; }
        else if (token.compare (i, 3, "\xE2\x99\xAD") == 0) { --acc; i += 3; }   // ♭
        else if (token.compare (i, 3, "\xE2\x99\xAF") == 0) { ++acc; i += 3; }   // ♯
        else break;
    }

    static const char* numerals[7] = { "VII", "III", "IV", "VI", "II", "V", "I" };
    static const int offsets[7] = { 11, 4, 5, 9, 2, 7, 0 };
    int offset = -1;
    bool lower = false;
    for (int n = 0; n < 7 && offset < 0; ++n)
    {
        const std::string num = numerals[n];
        if (token.size() < i + num.size()) continue;
        bool up = true, low = true;
        for (size_t j = 0; j < num.size(); ++j)
        {
            const char ch = token[i + j];
            up = up && ch == num[j];
            low = low && ch == char (num[j] + ('a' - 'A'));
        }
        if (up || low)
        {
            offset = offsets[n];
            lower = low;
            i += num.size();
        }
    }
    if (offset < 0) return false;

    const std::string s = token.substr (i);
    Quality q;
    if      (s.empty())                                              q = lower ? Quality::Min : Quality::Maj;
    else if (s == "7")                                               q = lower ? Quality::Min7 : Quality::Dom7;
    else if (s == "maj7")                                            q = lower ? Quality::MinMaj7 : Quality::Maj7;
    else if (s == "(maj7)")                                          q = Quality::MinMaj7;
    else if (s == "\xC2\xB0" || s == "o" || s == "dim")              q = Quality::Dim;          // °
    else if (s == "\xC2\xB0" "7" || s == "o7" || s == "dim7")        q = Quality::Dim7;
    else if (s == "\xC3\xB8" "7" || s == "\xC3\xB8" || s == "m7b5")  q = Quality::HalfDim7;     // ø
    else if (s == "+" || s == "aug")                                 q = Quality::Aug;
    else if (s == "sus2")                                            q = Quality::Sus2;
    else if (s == "sus4" || s == "sus")                              q = Quality::Sus4;
    else if (s == "7sus4" || s == "7sus")                            q = Quality::Dom7Sus4;
    else if (s == "add9")                                            q = lower ? Quality::MinAdd9 : Quality::Add9;
    else if (s == "maj9")                                            q = Quality::Maj9;
    else if (s == "9")                                               q = lower ? Quality::Min9 : Quality::Dom9;
    else if (s == "6")                                               q = lower ? Quality::Min6 : Quality::Six;
    else if (s == "7b9")                                             q = Quality::Dom7b9;
    else return false;

    out = { mod12 (k.root + offset + acc), q };
    return true;
}

std::string romanNumeral (const Chord& c, const Key& k)
{
    static const char* up[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    static const char* off[12] = { "I", "♭II", "II", "♭III", "III", "IV", "♯IV", "V", "♭VI", "VI", "♭VII", "VII" };

    const int deg = degreeOf (c.root, k);
    std::string base = (deg >= 0 && isDiatonicChord (c, k)) ? up[deg] : off[mod12 (c.root - k.root)];

    if (isMinorQuality (c.quality))
        for (auto& ch : base) if (ch == 'I' || ch == 'V') ch = char (ch + ('a' - 'A'));

    switch (c.quality)
    {
        case Quality::Dim:      base += "°"; break;
        case Quality::Aug:      base += "+"; break;
        case Quality::Sus2:     base += "sus2"; break;
        case Quality::Sus4:     base += "sus4"; break;
        case Quality::Maj7:     base += "maj7"; break;
        case Quality::Dom7:     base += "7"; break;
        case Quality::Min7:     base += "7"; break;
        case Quality::HalfDim7: base += "ø7"; break;
        case Quality::Dim7:     base += "°7"; break;
        case Quality::MinMaj7:  base += "(maj7)"; break;
        case Quality::Add9:     case Quality::MinAdd9: base += "add9"; break;
        case Quality::Maj9:     base += "maj9"; break;
        case Quality::Min9:     case Quality::Dom9: base += "9"; break;
        case Quality::Six:      case Quality::Min6: base += "6"; break;
        case Quality::Dom7Sus4: base += "7sus4"; break;
        case Quality::Dom7b9:   base += "7♭9"; break;
        default: break;
    }
    return base;
}

// ============================================================ analisi
std::vector<KeyGuess> detectKey (const std::array<double, 12>& h)
{
    static const double maj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double mnr[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

    double sum = 0;
    for (double v : h) sum += v;
    if (sum <= 1e-9) return {};

    std::vector<KeyGuess> out;
    const double mx = sum / 12.0;
    for (int mode = 0; mode < 2; ++mode)
    {
        const double* p = mode == 0 ? maj : mnr;
        double mp = 0;
        for (int i = 0; i < 12; ++i) mp += p[i];
        mp /= 12.0;

        for (int root = 0; root < 12; ++root)
        {
            double num = 0, dx = 0, dp = 0;
            for (int i = 0; i < 12; ++i)
            {
                const double x = h[size_t (mod12 (root + i))] - mx;
                const double y = p[i] - mp;
                num += x * y; dx += x * x; dp += y * y;
            }
            KeyGuess g;
            g.key.root = root;
            g.key.scale = mode == 0 ? ScaleType::Major : ScaleType::NaturalMinor;
            g.confidence = (dx > 0 && dp > 0) ? num / std::sqrt (dx * dp) : 0.0;
            out.push_back (g);
        }
    }
    std::sort (out.begin(), out.end(), [] (const KeyGuess& a, const KeyGuess& b) { return a.confidence > b.confidence; });
    out.resize (3);
    return out;
}

DetectedChord detectChord (uint16_t mask, int bass)
{
    DetectedChord best;
    int bestScore = -1000;
    if (popcount (mask) < 3) return best;

    for (int root = 0; root < 12; ++root)
    {
        if (! ((mask >> root) & 1)) continue;
        for (int q = 0; q < kNumQualities; ++q)
        {
            uint32_t tmask = 0;
            for (int iv : kQualityIntervals[q]) tmask |= 1u << mod12 (root + iv);
            if ((tmask & ~uint32_t (mask)) != 0) continue;           // mancano note dell'accordo
            const int extras = popcount (uint32_t (mask) & ~tmask);
            int score = popcount (tmask) * 10 - extras * 4;
            if (root == bass) score += 5;
            if (q == int (Quality::Maj) || q == int (Quality::Min)) score += 1;
            if (q >= kNumBasicQualities) score -= 1;              // a parità vincono le letture classiche
            if (score > bestScore)
            {
                bestScore = score;
                best.valid = true;
                best.chord = { root, Quality (q) };
            }
        }
    }
    return best;
}

// ============================================================ suggerimenti
std::vector<Suggestion> suggestNext (const Chord& from, const Key& k, bool sevenths, double adventure, int maxCount,
                                     Genre genre, int colour)
{
    std::vector<Suggestion> out;
    for (const auto& c : buildCandidates (k, sevenths, genre, colour))
    {
        std::string reason;
        const double w = weightFrom (from, c, k, adventure, &reason, genre);
        if (w >= 1.0)
            out.push_back ({ c.chord, w, c.degree, c.isExtra, reason });
    }
    std::stable_sort (out.begin(), out.end(), [] (const Suggestion& a, const Suggestion& b) { return a.score > b.score; });
    if (int (out.size()) > maxCount) out.resize (size_t (std::max (0, maxCount)));
    return out;
}

const std::vector<Preset>& presetsFor (const Key& k)
{
    static const std::vector<Preset> major = {
        { "I – V – vi – IV  (pop)",              { 0, 4, 5, 3 } },
        { "vi – IV – I – V  (emotivo)",          { 5, 3, 0, 4 } },
        { "I – vi – IV – V  (anni '50)",         { 0, 5, 3, 4 } },
        { "ii – V – I – I  (jazz)",              { 1, 4, 0, 0 } },
        { "I – IV – V – IV  (rock)",             { 0, 3, 4, 3 } },
        { "I – iii – IV – V",                    { 0, 2, 3, 4 } },
        { "I – IV – vi – V",                     { 0, 3, 5, 4 } },
        { "I – V – vi – iii – IV – I – IV – V  (canone)", { 0, 4, 5, 2, 3, 0, 3, 4 } },
    };
    static const std::vector<Preset> minor = {
        { "i – VI – III – VII  (epico)",         { 0, 5, 2, 6 } },
        { "i – iv – V – i  (classico)",          { 0, 3, 4, 0 } },
        { "i – VII – VI – VII",                  { 0, 6, 5, 6 } },
        { "i – VI – VII – i",                    { 0, 5, 6, 0 } },
        { "ii° – V – i – i  (jazz)",             { 1, 4, 0, 0 } },
        { "i – iv – VII – III",                  { 0, 3, 6, 2 } },
        { "i – VI – iv – V",                     { 0, 5, 3, 4 } },
        { "i – VII – VI – V  (andalusa)",        { 0, 6, 5, 4 } },
    };
    return isMinorFamily (k.scale) ? minor : major;
}

std::vector<Chord> chordsFromPreset (const Key& k, int presetIndex, bool sevenths)
{
    const auto& list = presetsFor (k);
    const auto& p = list[size_t (std::clamp (presetIndex, 0, int (list.size()) - 1))];
    std::vector<Chord> out;
    for (int d : p.degrees) out.push_back (chordAtDegree (k, d, sevenths, true));
    return out;
}

std::vector<Chord> generateProgression (const ProgressionOptions& o)
{
    const int length = std::clamp (o.length, 1, kMaxChords);
    const double adv = std::clamp (o.adventure, 0.0, 1.0);
    const int col = colourOf (o.sevenths, o.colour);
    const auto cands = buildCandidates (o.key, o.sevenths, o.genre, col);
    Rng rng (o.seed);

    std::vector<Chord> out;
    out.push_back (chordAtDegree (o.key, 0, col >= 1, true));
    if (col == 2) out[0] = adaptColour (out[0], o.key, 2);
    const Candidate tonic { out[0], 0, false, 0, 0, false, nullptr };

    for (int i = 1; i < length; ++i)
    {
        const Chord cur = out.back();
        std::vector<double> ws (cands.size(), 0.0);
        double sum = 0;
        for (size_t j = 0; j < cands.size(); ++j)
        {
            double w = weightFrom (cur, cands[j], o.key, adv, nullptr, o.genre);
            if (w < 1.0) continue;
            w = std::pow (w, 1.6 - adv);
            if (i == length - 1)   // ultimo accordo: preferisci chi riporta alla tonica
                w *= 1.0 + weightFrom (cands[j].chord, tonic, o.key, adv, nullptr, o.genre) / 5.0;
            ws[j] = w;
            sum += w;
        }
        if (sum <= 0) { out.push_back (cur); continue; }
        double r = rng.uni() * sum;
        size_t pick = 0;
        for (size_t j = 0; j < ws.size(); ++j)
        {
            if (ws[j] <= 0) continue;
            pick = j;
            r -= ws[j];
            if (r <= 0) break;
        }
        out.push_back (cands[pick].chord);
    }
    return out;
}

namespace
{
// funzione armonica: 0 = tonica, 1 = sottodominante, 2 = dominante
int harmonicFunction (const Chord& c, const Key& k)
{
    const int off = mod12 (c.root - k.root);
    const bool minorFam = isMinorFamily (k.scale);
    if (isDominantQuality (c.quality) && off != 7 && off != 1) return 2;   // dominanti secondarie: spingono
    switch (off)
    {
        case 0:  return 0;
        case 2:  return 1;
        case 3:  return minorFam ? 0 : 1;
        case 4:  return isMinorQuality (c.quality) ? 0 : 2;
        case 5:  return 1;
        case 7:  return 2;
        case 8:  return 1;
        case 9:  return minorFam ? 1 : 0;
        case 10: return minorFam ? 2 : 1;
        case 11: return 2;
        case 1:  return isDominantQuality (c.quality) ? 2 : 1;
        default: return 1;
    }
}
}

std::vector<Chord> reharmonize (const std::vector<Chord>& chords, const Key& k, int colour, double adventure,
                                Genre genre, uint32_t seed)
{
    std::vector<Chord> out = chords;
    const int n = int (chords.size());
    if (n < 3) return out;

    const auto& gp = genreProfile (genre);
    const double adv = std::clamp (adventure, 0.0, 1.0);
    const int col = std::clamp (colour, 0, kNumColours - 1);
    const auto cands = buildCandidates (k, col >= 1, genre, col);
    Rng rng (seed);

    auto domOf = [&] (int root)   // dominante (secondaria) di root, nel colore scelto
    {
        Chord c { mod12 (root + 7), Quality::Dom7 };
        if (col == 0) c.quality = Quality::Maj;
        else if (col == 2) c = adaptColour (c, k, 2);
        return c;
    };

    bool changedAny = false;
    std::vector<int> order;
    for (int i = 1; i < n - 1; ++i) order.push_back (i);

    for (int pass = 0; pass < 2 && ! changedAny; ++pass)
        for (int i : order)
        {
            if (pass == 0 && rng.uni() > 0.4 + 0.45 * adv) continue;

            const Chord prev = out[size_t (i - 1)], cur = out[size_t (i)], next = out[size_t (i + 1)];
            const int fn = harmonicFunction (cur, k);
            const Candidate nextC = candidateFor (next, k);

            std::vector<std::pair<Chord, double>> opts;
            for (const auto& c : cands)
            {
                if (c.chord == cur || c.chord == prev || c.chord == next) continue;
                if (harmonicFunction (c.chord, k) != fn) continue;
                double w = weightFrom (prev, c, k, adv, nullptr, genre) + weightFrom (c.chord, nextC, k, adv, nullptr, genre);
                if (c.isExtra) w *= 0.4 + adv * gp.colourMul;
                if (w > 0.5) opts.push_back ({ c.chord, w });
            }
            // dominante secondaria (o sostituto di tritono) che prepara l'accordo successivo
            if (adv > 0.15 && mod12 (next.root - k.root) != 0 && triadOf (next.quality) != Quality::Dim)
            {
                const Chord d = domOf (next.root);
                if (d != cur && d != prev) opts.push_back ({ d, 6.0 * adv * gp.colourMul + 1.0 });
                if (gp.tritoneSubs && adv > 0.3)
                {
                    Chord t { mod12 (next.root + 1), col == 0 ? Quality::Maj : Quality::Dom7 };
                    if (col == 2) t = adaptColour (t, k, 2);
                    if (t != cur && t != prev) opts.push_back ({ t, 4.0 * adv * gp.colourMul });
                }
            }
            if (opts.empty()) continue;

            double sum = 0;
            for (auto& o : opts) { o.second = std::pow (o.second, 1.3); sum += o.second; }
            double r = rng.uni() * sum;
            Chord pick = opts.back().first;
            for (auto& o : opts)
            {
                r -= o.second;
                if (r <= 0) { pick = o.first; break; }
            }
            out[size_t (i)] = pick;
            changedAny = true;
        }
    return out;
}

// ============================================================ voicing
std::vector<int> voiceChord (const Chord& c, const std::vector<int>* previous, int centre)
{
    const auto pcs = chordPitchClasses (c);
    const int n = int (pcs.size());
    std::vector<int> prevSorted;
    if (previous != nullptr) { prevSorted = *previous; std::sort (prevSorted.begin(), prevSorted.end()); }

    std::vector<int> best;
    double bestCost = 1e18;

    for (int r = 0; r < n; ++r)
        for (int low = 46; low <= 66; ++low)
        {
            std::vector<int> v;
            int p = low;
            while (mod12 (p) != pcs[size_t (r)]) ++p;
            v.push_back (p);
            for (int i = 1; i < n; ++i)
            {
                const int pc = pcs[size_t ((r + i) % n)];
                ++p;
                while (mod12 (p) != pc) ++p;
                v.push_back (p);
            }
            if (v.front() < 45 || v.back() > 79) continue;

            double avg = 0;
            for (int x : v) avg += x;
            avg /= n;

            double cost;
            if (! prevSorted.empty())
            {
                const size_t m = std::min (prevSorted.size(), v.size());
                cost = 0;
                for (size_t i = 0; i < m; ++i) cost += std::abs (v[i] - prevSorted[i]);
                cost += 2.0 * std::abs (int (prevSorted.size()) - int (v.size()));
                cost += 0.15 * std::abs (avg - centre);
            }
            else
                cost = std::abs (avg - centre) + 1.5 * r;

            if (cost < bestCost - 1e-9) { bestCost = cost; best = v; }
        }

    if (best.empty())
        for (int pc : pcs) best.push_back (48 + pc);
    return best;
}

std::vector<std::vector<int>> voiceProgression (const std::vector<Chord>& chords, int centre)
{
    std::vector<std::vector<int>> out;
    for (size_t i = 0; i < chords.size(); ++i)
        out.push_back (voiceChord (chords[i], i == 0 ? nullptr : &out.back(), centre));
    return out;
}

const char* chordStyleName (ChordStyle s)
{
    switch (s)
    {
        case ChordStyle::Block:      return "Accordi pieni";
        case ChordStyle::Arp16:      return "Arpeggio 1/16";
        case ChordStyle::ArpUpDown8: return "Arpeggio su-giù 1/8";
        case ChordStyle::Pulse8:     return "Pulsante 1/8";
        case ChordStyle::Offbeat:    return "Contrattempo";
    }
    return "";
}

std::vector<Note> renderChords (const std::vector<Chord>& chords, const RenderOptions& o)
{
    std::vector<Note> out;
    const int spc = std::max(1, o.stepsPerChord);
    const int slot = spc * kStepTicks;
    const auto voicings = voiceProgression (chords);
    auto gate = [] (int ticks) { return std::max (1, int (ticks * 0.92)); };

    for (size_t i = 0; i < chords.size(); ++i)
    {
        const int t0 = int (i) * slot;
        const auto& v = voicings[i];
        const int n = int (v.size());

        if (o.bass && o.bassStyle == BassStyle::Sustained)
            out.push_back ({ 36 + chords[i].root, t0, slot, std::min (127, o.velocity + 4) });

        switch (o.style)
        {
            case ChordStyle::Block:
                for (int p : v) out.push_back ({ p, t0, slot, o.velocity });
                break;

            case ChordStyle::Arp16:
                for (int s = 0; s < spc; ++s)
                    out.push_back ({ v[size_t (s % n)], t0 + s * kStepTicks, gate (kStepTicks), o.velocity - (s % 4 == 0 ? 0 : 12) });
                break;

            case ChordStyle::ArpUpDown8:
            {
                std::vector<int> order;
                for (int j = 0; j < n; ++j) order.push_back (j);
                for (int j = n - 2; j >= 1; --j) order.push_back (j);
                for (int s = 0, k = 0; s < spc; s += 2, ++k)
                    out.push_back ({ v[size_t (order[size_t (k) % order.size()])], t0 + s * kStepTicks,
                                     gate (2 * kStepTicks), o.velocity - (s % 4 == 0 ? 0 : 10) });
                break;
            }

            case ChordStyle::Pulse8:
                for (int s = 0; s < spc; s += 2)
                    for (int p : v) out.push_back ({ p, t0 + s * kStepTicks, gate (2 * kStepTicks), o.velocity - (s % 4 == 0 ? 0 : 14) });
                break;

            case ChordStyle::Offbeat:
                for (int s = 2; s < spc; s += 4)
                    for (int p : v) out.push_back ({ p, t0 + s * kStepTicks, gate (2 * kStepTicks), o.velocity });
                break;
        }
    }
    if (o.bass && o.bassStyle != BassStyle::Sustained)
    {
        const auto b = renderBass (chords, o);
        out.insert (out.end(), b.begin(), b.end());
    }
    std::stable_sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });
    return out;
}

// ============================================================ basso
const char* bassStyleName (BassStyle s)
{
    switch (s)
    {
        case BassStyle::Sustained:  return "Basso tenuto";
        case BassStyle::Root8:      return "Ottavi sulla fondamentale";
        case BassStyle::RootFifth:  return "Fondamentale-quinta";
        case BassStyle::Walking:    return "Walking";
        case BassStyle::Syncopated: return "Sincopato";
        case BassStyle::Octaves:    return "Ottave";
    }
    return "";
}

std::vector<Note> renderBass (const std::vector<Chord>& chords, const RenderOptions& o)
{
    std::vector<Note> out;
    const int spc = std::max (1, o.stepsPerChord);
    const int slot = spc * kStepTicks;
    const int vel = std::min (127, o.velocity + 4);

    // fondamentale nell'ottava più vicina alla precedente (Mi1..Mi2 circa)
    int prevRoot = 36 + (chords.empty() ? 0 : chords[0].root);
    auto rootNear = [&] (int pc, int ref)
    {
        int best = 28 + mod12 (pc - 28), bestD = 1000;
        for (int p = best; p <= 52; p += 12)
            if (std::abs (p - ref) < bestD) { bestD = std::abs (p - ref); best = p; }
        return best;
    };

    for (size_t i = 0; i < chords.size(); ++i)
    {
        const int t0 = int (i) * slot;
        const Chord& c = chords[i];
        const int root = (o.bassStyle == BassStyle::Walking) ? rootNear (c.root, prevRoot) : 36 + c.root;
        const int fifth = root + (triadOf (c.quality) == Quality::Dim ? 6 : (triadOf (c.quality) == Quality::Aug ? 8 : 7));
        auto add = [&] (int pitch, int step, int len, int v)
        {
            if (step >= spc) return;
            len = std::min (len, spc - step);
            out.push_back ({ pitch, t0 + step * kStepTicks, std::max (1, int (len * kStepTicks * 0.9)), std::clamp (v, 1, 127) });
        };

        switch (o.bassStyle)
        {
            case BassStyle::Sustained:
                out.push_back ({ root, t0, slot, vel });
                break;

            case BassStyle::Root8:
                for (int s = 0; s < spc; s += 2) add (root, s, 2, vel - (s % 4 == 0 ? 0 : 14));
                break;

            case BassStyle::RootFifth:   // figura bossa / pop: R . . . . . 5 . | 5 . . . . . R .
                for (int h = 0; h < spc; h += 16)
                {
                    add (root, h, 6, vel);
                    add (fifth - 12 >= 28 ? fifth - 12 : fifth, h + 6, 2, vel - 12);
                    add (fifth - 12 >= 28 ? fifth - 12 : fifth, h + 8, 6, vel - 6);
                    add (root, h + 14, 2, vel - 14);
                }
                break;

            case BassStyle::Syncopated:  // groove R&B / hip-hop
                for (int h = 0; h < spc; h += 16)
                {
                    add (root, h, 3, vel);
                    add (root, h + 3, 2, vel - 16);
                    add (root + 12, h + 6, 2, vel - 10);
                    add (root, h + 10, 3, vel - 4);
                    add (fifth, h + 14, 2, vel - 14);
                }
                break;

            case BassStyle::Octaves:     // ottave in ottavi (dance)
                for (int s = 0; s < spc; s += 2) add (s % 4 == 0 ? root : root + 12, s, 2, vel - (s % 4 == 0 ? 0 : 8));
                break;

            case BassStyle::Walking:
            {
                // un quarto per tempo: fondamentale, note d'accordo/scala verso la prossima, avvicinamento cromatico
                const int beats = std::max (1, spc / 4);
                const int nextRoot = rootNear (chords[(i + 1) % chords.size()].root, root);
                const auto pcs = chordPitchClasses (c);
                int cur = root;
                for (int b = 0; b < beats; ++b)
                {
                    int p;
                    if (b == 0) p = root;
                    else if (b == beats - 1) p = nextRoot + ((i % 2 == 0) ? -1 : 1);       // cromatismo
                    else
                    {
                        // nota d'accordo (o di scala) che si muove verso la prossima fondamentale
                        const int dir = nextRoot >= cur ? 1 : -1;
                        p = cur + dir;
                        bool found = false;
                        for (int step = 1; step <= 4 && ! found; ++step)
                        {
                            const int q = cur + dir * step;
                            if (std::find (pcs.begin(), pcs.end(), mod12 (q)) != pcs.end()) { p = q; found = true; }
                        }
                        for (int step = 1; step <= 2 && ! found; ++step)
                            if (inParentScale (cur + dir * step, o.key)) { p = cur + dir * step; found = true; }
                    }
                    p = std::clamp (p, 28, 55);
                    add (p, b * 4, 4, vel - (b == 0 ? 0 : 10));
                    cur = p;
                }
                break;
            }
        }
        prevRoot = root;
    }
    std::stable_sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });
    return out;
}

// ============================================================ melodia
const char* contourName (Contour c)
{
    switch (c)
    {
        case Contour::Free:    return "Contorno libero";
        case Contour::Arch:    return "Arco (sale e scende)";
        case Contour::Rising:  return "Ascendente";
        case Contour::Falling: return "Discendente";
        case Contour::Wave:    return "Onda";
    }
    return "";
}

const char* registerName (int r)
{
    static const char* names[kNumRegisters] = { "Registro basso", "Registro medio", "Registro medio-alto", "Registro alto" };
    return names[std::clamp (r, 0, kNumRegisters - 1)];
}

void registerRange (int r, int& low, int& high)
{
    static const int lo[kNumRegisters] = { 48, 55, 60, 67 };
    static const int hi[kNumRegisters] = { 67, 76, 84, 91 };
    r = std::clamp (r, 0, kNumRegisters - 1);
    low = lo[r];
    high = hi[r];
}

void applySwing (std::vector<Note>& notes, double amount)
{
    const int shift = int (std::lround (std::clamp (amount, 0.0, 1.0) * kPPQ / 6.0));
    if (shift <= 0) return;
    for (auto& n : notes)
        if (n.startTick % kPPQ == kPPQ / 2)
        {
            n.startTick += shift;
            n.lengthTick = std::max (1, n.lengthTick - shift);
        }
}

std::vector<Note> generateMelody (const std::vector<Chord>& chords, const MelodyOptions& o)
{
    std::vector<Note> out;
    if (chords.empty()) return out;

    const auto& gp = genreProfile (o.genre);
    Rng rng (o.seed);
    const int spc = std::max (1, o.stepsPerChord);
    const int total = int (chords.size()) * spc;
    const int numBars = (total + 15) / 16;
    const double d = std::clamp (o.density, 0.0, 1.0);
    const double tension = std::clamp (o.tension, 0.0, 1.0);
    const double motif = std::clamp (gp.motif, 0.0, 1.0);
    const int low = std::min (o.lowNote, o.highNote);
    const int high = std::max (o.lowNote, o.highNote);
    const int centre = (low + high) / 2;
    const double pi = 3.14159265358979;

    auto chordAt = [&] (int step) -> const Chord& { return chords[size_t (std::min (step / spc, int (chords.size()) - 1))]; };

    // ------------------------------------------------ ritmo: frasi di 4 battute (motivo, ripetizione, variante, cadenza)
    auto onsetProb = [&] (int s)
    {
        if (! gp.customRhythm)
        {
            if (s == 0)     return 1.0;
            if (s % 4 == 0) return 0.45 + 0.45 * d;
            if (s % 2 == 0) return 0.65 * d;
            return 0.30 * d * d;
        }
        const double base = gp.rhythm[size_t (s)];
        if (s == 0) return std::min (1.0, base + 0.25);
        double p = base * (0.3 + 1.4 * d);
        if (s % 2 == 1) p += std::max (0.0, d - 0.65) * 0.5;
        return std::clamp (p, 0.0, 1.0);
    };

    using Bar = std::array<bool, 16>;
    std::vector<Bar> bars (static_cast<size_t> (numBars));
    std::vector<int> source (static_cast<size_t> (numBars), -1);     // battuta di cui si ripete il motivo
    auto fresh = [&] (Bar& p) { for (int s = 0; s < 16; ++s) p[size_t (s)] = rng.uni() < onsetProb (s); };
    auto mutate = [&] (Bar& p, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const size_t s = size_t (1 + int (rng.uni() * 15));
            p[s] = ! p[s];
        }
    };

    for (int b = 0; b < numBars; ++b)
    {
        const int pos = b % 4;
        Bar& bar = bars[size_t (b)];
        if (b == 0)
            fresh (bar);
        else if (pos == 0)
        {
            if (rng.uni() < motif * 0.8) { bar = bars[0]; source[size_t (b)] = 0; }
            else fresh (bar);
        }
        else if (pos == 2)
        {
            if (rng.uni() < motif)
            {
                bar = bars[size_t (b - 2)];
                source[size_t (b)] = b - 2;
                if (rng.uni() < 0.6) mutate (bar, 1 + int (rng.uni() * 2));
            }
            else fresh (bar);
        }
        else if (rng.uni() < motif)
        {
            bar = bars[size_t (b - 1)];
            source[size_t (b)] = b - 1;
            if (rng.uni() < 0.35) mutate (bar, 1);
        }
        else
            fresh (bar);

        if (pos == 3 || (b == numBars - 1 && numBars > 1))      // fine frase: si respira
            for (int s = 8; s < 16; ++s)
                bar[size_t (s)] = s == 8 || (s == 12 && rng.uni() < 0.5 * d);
        bar[0] = true;
    }

    std::vector<int> onsets;
    for (int s = 0; s < total; ++s)
        if (bars[size_t (s / 16)][size_t (s % 16)] || s % spc == 0) onsets.push_back (s);

    // ------------------------------------------------ contorno
    auto target = [&] (int s) -> double
    {
        const double t = total > 1 ? double (s) / double (total - 1) : 0.0;
        const double span = high - low;
        switch (o.contour)
        {
            case Contour::Arch:    return low + span * (0.3 + 0.45 * std::sin (pi * t));
            case Contour::Rising:  return low + span * (0.25 + 0.55 * t);
            case Contour::Falling: return low + span * (0.8 - 0.55 * t);
            case Contour::Wave:    return low + span * (0.5 + 0.28 * std::sin (2.0 * pi * t * std::max (1.0, numBars / 2.0)));
            case Contour::Free:    break;
        }
        return centre;
    };
    const double pull = o.contour == Contour::Free ? 10.0 : 6.0;

    // gradi della scala nel registro (per ripetere il motivo "in sequenza" sul nuovo accordo)
    std::vector<int> scalePitches;
    for (int q = low - 12; q <= high + 12; ++q)
        if (inScale (q, o.key)) scalePitches.push_back (q);
    auto scaleIndex = [&] (int p)
    {
        int best = 0;
        for (int i = 0; i < int (scalePitches.size()); ++i)
            if (std::abs (scalePitches[size_t (i)] - p) < std::abs (scalePitches[size_t (best)] - p)) best = i;
        return best;
    };
    auto scaleStep = [&] (int p, int steps)
    {
        if (scalePitches.empty()) return p;
        const int i = std::clamp (scaleIndex (p) + steps, 0, int (scalePitches.size()) - 1);
        return scalePitches[size_t (i)];
    };

    std::vector<char> transfer (static_cast<size_t> (numBars), 0);
    for (int b = 0; b < numBars; ++b)
        transfer[size_t (b)] = source[size_t (b)] >= 0 && rng.uni() < 0.35 + 0.6 * motif;
    std::vector<int> pitchAt (static_cast<size_t> (total), -1);
    int anchorBar = -1, anchorNew = 0, anchorSrc = 0;

    int prev = centre, dir = 0, lastInterval = 0;
    const double spread = 1.2 + std::clamp (o.leaps, 0.0, 1.0) * 7.0;

    for (size_t idx = 0; idx < onsets.size(); ++idx)
    {
        const int s = onsets[idx];
        const int b = s / 16;
        const Chord& ch = chordAt (s);
        const auto cpcs = chordPitchClasses (ch);
        auto isChordTone = [&] (int pc) { return std::find (cpcs.begin(), cpcs.end(), mod12 (pc)) != cpcs.end(); };
        auto isExtension = [&] (int pc)
        {
            return (mod12 (pc) == mod12 (ch.root + 2) || mod12 (pc) == mod12 (ch.root + 9)) && inScale (pc, o.key);
        };
        const bool chordStart = s % spc == 0;
        const bool strong = s % 4 == 0;
        const int srcStep = source[size_t (b)] >= 0 ? s - 16 * (b - source[size_t (b)]) : -1;
        const bool canTransfer = transfer[size_t (b)] && srcStep >= 0 && pitchAt[size_t (srcStep)] >= 0;

        int pitch = -1;
        if (canTransfer && anchorBar == b)
        {
            // stesso disegno del motivo, trasportato per gradi di scala
            const int delta = scaleIndex (pitchAt[size_t (srcStep)]) - scaleIndex (anchorSrc);
            pitch = scaleStep (anchorNew, delta);
            if ((chordStart || (strong && tension < 0.5)) && ! isChordTone (pitch))
            {
                int best = -1;
                for (int dd = 1; dd <= 2 && best < 0; ++dd)
                {
                    const int a = pitch + (lastInterval >= 0 ? dd : -dd), c = pitch - (lastInterval >= 0 ? dd : -dd);
                    if (isChordTone (a)) best = a;
                    else if (isChordTone (c)) best = c;
                }
                pitch = best;
            }
            if (pitch < low || pitch > high) pitch = -1;
        }

        if (pitch < 0)
        {
            std::vector<std::pair<int, double>> cand;
            double sum = 0;
            for (int q = low; q <= high; ++q)
            {
                const bool ct = isChordTone (q);
                const bool sc = inScale (q, o.key);
                if (! ct && ! sc) continue;
                const bool ext = ! ct && isExtension (q);

                double w;
                if (chordStart)  w = ct ? 1.0 : (ext && tension > 0.5 ? (tension - 0.5) * 0.8 : 0.0);
                else if (strong) w = ct ? 1.0 : (ext ? 0.12 + 0.7 * tension : 0.12 + 0.45 * tension);
                else             w = ct ? (sc ? 1.25 : 0.6) * (1.0 - 0.35 * tension) : 1.0 + 0.3 * tension;
                if (w <= 0) continue;

                const int iv = q - prev;
                w *= std::exp (-double (std::abs (iv)) / spread);
                if (iv == 0) w *= (s % 2 == 1 ? 0.3 : 0.6);
                if (dir != 0 && iv != 0 && ((iv > 0) == (dir > 0))) w *= 1.25;
                if (std::abs (lastInterval) >= 5 && iv != 0 && ((iv > 0) != (lastInterval > 0)) && std::abs (iv) <= 2) w *= 1.8;
                w *= std::exp (-std::abs (q - target (s)) / pull);

                cand.push_back ({ q, w });
                sum += w;
            }

            pitch = prev;
            if (cand.empty() || sum <= 0)
            {
                int bestDist = 1000;
                for (int q = low; q <= high; ++q)
                    if (isChordTone (q) && std::abs (q - prev) < bestDist) { bestDist = std::abs (q - prev); pitch = q; }
            }
            else
            {
                double r = rng.uni() * sum;
                pitch = cand.back().first;
                for (auto& c : cand)
                {
                    r -= c.second;
                    if (r <= 0) { pitch = c.first; break; }
                }
            }
            if (canTransfer && anchorBar != b)
            {
                anchorBar = b;
                anchorNew = pitch;
                anchorSrc = pitchAt[size_t (srcStep)];
            }
        }

        const int next = idx + 1 < onsets.size() ? onsets[idx + 1] : total;
        int cap = 8 - int (d * 4);
        if (gp.legato >= 0.97 || (b % 4 == 3 && s % 16 >= 8)) cap = 16;
        const int lenSteps = std::max (1, std::min (next - s, cap));
        const double legato = gp.customRhythm ? std::min (gp.legato, 0.97) : 0.95;
        int vel = strong ? 98 : (s % 2 == 0 ? 86 : 72);
        vel += int ((d - 0.5) * 16.0);
        vel = std::clamp (vel + int (rng.uni() * 11) - 5, 40, 120);

        out.push_back ({ pitch, s * kStepTicks, std::max (1, int (lenSteps * kStepTicks * legato)), vel });
        pitchAt[size_t (s)] = pitch;
        dir = (pitch > prev) - (pitch < prev);
        lastInterval = pitch - prev;
        prev = pitch;
    }

    // tensione alta: note di avvicinamento cromatico prima dei tempi forti
    if (tension > 0.55)
        for (size_t i = 0; i + 1 < out.size(); ++i)
        {
            const int s = out[i].startTick / kStepTicks, sn = out[i + 1].startTick / kStepTicks;
            if (s % 4 == 0 || s % spc == 0 || sn % 4 != 0) continue;
            if (rng.uni() > (tension - 0.55) * 1.6) continue;
            const int p = out[i + 1].pitch + (rng.uni() < 0.7 ? -1 : 1);
            if (p >= low && p <= high && p != out[i + 1].pitch) out[i].pitch = p;
        }

    if (o.resolveEnd && ! out.empty())
    {
        Note& last = out.back();
        const int rootPc = chords.back().root;
        int bestDist = 1000, bestPitch = last.pitch;
        for (int q = low; q <= high; ++q)
            if (mod12 (q) == rootPc && std::abs (q - last.pitch) < bestDist) { bestDist = std::abs (q - last.pitch); bestPitch = q; }
        last.pitch = bestPitch;
        last.lengthTick = std::max (1, std::min (total * kStepTicks - last.startTick, 16 * kStepTicks));
    }

    if (gp.swing > 0) applySwing (out, gp.swing);
    return out;
}

// ============================================================ strumenti MIDI
void quantizeNotes (std::vector<Note>& notes, int grid, double strength, bool quantizeLength)
{
    if (grid <= 0) return;
    strength = std::clamp (strength, 0.0, 1.0);
    for (auto& n : notes)
    {
        const int target = int (std::lround (double (n.startTick) / grid)) * grid;
        const int ns = std::max (0, n.startTick + int (std::lround ((target - n.startTick) * strength)));
        if (quantizeLength)
        {
            const int end = n.startTick + n.lengthTick;
            const int targetEnd = int (std::lround (double (end) / grid)) * grid;
            const int ne = end + int (std::lround ((targetEnd - end) * strength));
            n.lengthTick = std::max (1, ne - ns);
        }
        n.startTick = ns;
    }
    std::stable_sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b)
                      { return a.startTick != b.startTick ? a.startTick < b.startTick : a.pitch < b.pitch; });
}

void snapNotesToScale (std::vector<Note>& notes, const Key& k)
{
    for (auto& n : notes)
    {
        if (inScale (n.pitch, k)) continue;
        for (int d = 1; d <= 3; ++d)
        {
            if (inScale (n.pitch - d, k)) { n.pitch -= d; break; }
            if (inScale (n.pitch + d, k)) { n.pitch += d; break; }
        }
    }
}

std::vector<Chord> harmonizeMelody (const std::vector<Note>& melody, const Key& k, int stepsPerChord,
                                    int numChords, bool sevenths, double adventure, Genre genre, int colour)
{
    numChords = std::clamp (numChords, 1, kMaxChords);
    adventure = std::clamp (adventure, 0.0, 1.0);
    const auto cands = buildCandidates (k, sevenths, genre, colour);
    const size_t C = cands.size();
    const int slotTicks = std::max (1, stepsPerChord) * kStepTicks;

    std::vector<std::vector<double>> fit (size_t (numChords), std::vector<double> (C, 0.0));
    for (int i = 0; i < numChords; ++i)
    {
        double pcW[12] = {};
        double total = 0;
        const int a = i * slotTicks, b = a + slotTicks;
        for (const auto& n : melody)
        {
            const int ns = n.startTick, ne = ns + n.lengthTick;
            const int ov = std::min (ne, b) - std::max (ns, a);
            if (ov <= 0) continue;
            double w = ov;
            if (ns >= a && ns < b && ns % kPPQ == 0) w *= 1.5;
            pcW[mod12 (n.pitch)] += w;
            total += w;
        }
        if (total <= 0) continue;

        for (size_t c = 0; c < C; ++c)
        {
            const auto pcs = chordPitchClasses (cands[c].chord);
            double chordTone = 0, chromatic = 0;
            for (int pc = 0; pc < 12; ++pc)
            {
                const bool inChord = std::find (pcs.begin(), pcs.end(), pc) != pcs.end();
                if (inChord) chordTone += pcW[pc];
                else if (! inScale (pc, k)) chromatic += pcW[pc];
            }
            double f = chordTone / total - 0.3 * chromatic / total;
            f += cands[c].isExtra ? -0.04 * (1.0 - adventure) : 0.03;
            fit[size_t (i)][c] = f;
        }
    }

    const double lambda = 0.4;
    std::vector<std::vector<double>> best (size_t (numChords), std::vector<double> (C, -1e18));
    std::vector<std::vector<int>> back (size_t (numChords), std::vector<int> (C, 0));
    for (size_t c = 0; c < C; ++c)
        best[0][c] = fit[0][c] + (cands[c].degree == 0 ? 0.15 : 0.0);

    for (int i = 1; i < numChords; ++i)
        for (size_t c = 0; c < C; ++c)
        {
            double bs = -1e18;
            int bp = 0;
            for (size_t p = 0; p < C; ++p)
            {
                const double trans = (p == c) ? 0.2 : weightFrom (cands[p].chord, cands[c], k, adventure, nullptr, genre) / 10.0;
                const double sc = best[size_t (i - 1)][p] + lambda * trans;
                if (sc > bs) { bs = sc; bp = int (p); }
            }
            best[size_t (i)][c] = bs + fit[size_t (i)][c];
            back[size_t (i)][c] = bp;
        }

    size_t cur = 0;
    for (size_t c = 1; c < C; ++c)
        if (best[size_t (numChords - 1)][c] > best[size_t (numChords - 1)][cur]) cur = c;

    std::vector<Chord> out (static_cast<size_t> (numChords));
    for (int i = numChords - 1; i >= 0; --i)
    {
        out[size_t (i)] = cands[cur].chord;
        cur = size_t (back[size_t (i)][cur]);
    }
    return out;
}

} // namespace cm
