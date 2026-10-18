#include "Theory.h"

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
};

const char* const kQualitySuffix[kNumQualities] = {
    "", "m", "dim", "aug", "sus2", "sus4", "maj7", "7", "m7", "m7b5", "dim7", "m(maj7)",
};

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

bool isMinorQuality (Quality q)
{
    return q == Quality::Min || q == Quality::Min7 || q == Quality::Dim || q == Quality::HalfDim7
           || q == Quality::Dim7 || q == Quality::MinMaj7;
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
    int degree = -1;
    const Extra* extra = nullptr;
};

std::vector<Candidate> buildCandidates (const Key& k, bool sevenths)
{
    std::vector<Candidate> out;
    for (int d = 0; d < 7; ++d)
        out.push_back ({ chordAtDegree (k, d, sevenths, false), d, nullptr });

    const bool minorFam = isMinorFamily (k.scale);
    const Extra* ex = minorFam ? kMinorExtras : kMajorExtras;
    const int n = minorFam ? int (sizeof (kMinorExtras) / sizeof (Extra)) : int (sizeof (kMajorExtras) / sizeof (Extra));

    for (int i = 0; i < n; ++i)
    {
        Chord c { mod12 (k.root + ex[i].offset), sevenths ? ex[i].seventh : ex[i].triad };
        bool dup = false;
        for (auto& o : out) if (o.chord == c) dup = true;
        if (! dup) out.push_back ({ c, -1, &ex[i] });
    }
    return out;
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
double weightFrom (const Chord& from, const Candidate& c, const Key& k, double adventure, std::string* reason)
{
    if (from == c.chord) return 0.0;

    const bool minorFam = isMinorFamily (k.scale);
    const int fromDeg = degreeOf (from.root, k);
    const double adv = std::clamp (adventure, 0.0, 1.0);
    const bool resolves = c.chord.root == mod12 (from.root + 5) || c.chord.root == mod12 (from.root - 1);
    const bool fromDiatonic = isDiatonicChord (from, k);

    double w = 0.0;
    const char* why = nullptr;
    std::string whyOwn;

    if (fromDiatonic || ((from.quality != Quality::Dom7 && from.quality != Quality::Maj) && fromDeg >= 0))
    {
        if (c.extra == nullptr)
        {
            w = (minorFam ? kMinorT : kMajorT)[fromDeg][c.degree];
            if (minorFam && c.degree == 4 && harmonicParent (k.scale) == ScaleType::NaturalMinor)
                w = (w + 1) / 2.0;
            why = degreeReason (c.degree, minorFam);
            if (fromDeg == 4 && c.degree == 0) why = "Cadenza perfetta V→I: risolve la tensione";
            else if (fromDeg == 4 && c.degree == 5 && ! minorFam) why = "Cadenza d'inganno V→vi: risoluzione a sorpresa";
            else if (fromDeg == 3 && c.degree == 0) why = "Cadenza plagale IV→I: risoluzione morbida";
        }
        else if ((c.extra->fromMask >> fromDeg) & 1)
        {
            w = c.extra->weight * (c.extra->colour ? 1.6 * adv : 1.0);
            why = c.extra->reason;
        }
    }
    else if (from.quality == Quality::Dom7 || from.quality == Quality::Maj)
    {
        // accordo non diatonico di tipo dominante: ci si aspetta la risoluzione
        if (c.extra == nullptr)
        {
            if (resolves) { w = 10; why = "Risoluzione: la dominante scende di quinta"; }
            else          { w = 1.0; why = degreeReason (c.degree, minorFam); }
        }
    }
    else if (c.extra == nullptr)
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
    return c == chordAtDegree (k, d, false) || c == chordAtDegree (k, d, true)
        || c == chordAtDegree (k, d, false, true) || c == chordAtDegree (k, d, true, true);
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
std::vector<Suggestion> suggestNext (const Chord& from, const Key& k, bool sevenths, double adventure, int maxCount)
{
    std::vector<Suggestion> out;
    for (const auto& c : buildCandidates (k, sevenths))
    {
        std::string reason;
        const double w = weightFrom (from, c, k, adventure, &reason);
        if (w >= 1.0)
            out.push_back ({ c.chord, w, c.degree, c.extra != nullptr, reason });
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
    const auto cands = buildCandidates (o.key, o.sevenths);
    Rng rng (o.seed);

    std::vector<Chord> out;
    out.push_back (chordAtDegree (o.key, 0, o.sevenths, true));
    const Candidate tonic { out[0], 0, nullptr };

    for (int i = 1; i < length; ++i)
    {
        const Chord cur = out.back();
        std::vector<double> ws (cands.size(), 0.0);
        double sum = 0;
        for (size_t j = 0; j < cands.size(); ++j)
        {
            double w = weightFrom (cur, cands[j], o.key, adv, nullptr);
            if (w < 1.0) continue;
            w = std::pow (w, 1.6 - adv);
            if (i == length - 1)   // ultimo accordo: preferisci chi riporta alla tonica
                w *= 1.0 + weightFrom (cands[j].chord, tonic, o.key, adv, nullptr) / 5.0;
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

        if (o.bass)
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
    std::stable_sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });
    return out;
}

// ============================================================ melodia
std::vector<Note> generateMelody (const std::vector<Chord>& chords, const MelodyOptions& o)
{
    std::vector<Note> out;
    if (chords.empty()) return out;

    Rng rng (o.seed);
    const int spc = std::max (1, o.stepsPerChord);
    const int total = int (chords.size()) * spc;
    const int numBars = (total + 15) / 16;
    const double d = std::clamp (o.density, 0.0, 1.0);
    const int low = std::min (o.lowNote, o.highNote);
    const int high = std::max (o.lowNote, o.highNote);
    const int centre = (low + high) / 2;

    // ritmo: una griglia da 16 step per battuta, spesso ripetuta con piccole varianti
    std::vector<std::array<bool, 16>> bars (static_cast<size_t> (numBars));
    auto fresh = [&] (std::array<bool, 16>& p)
    {
        for (int s = 0; s < 16; ++s)
        {
            double prob;
            if (s == 0)          prob = 1.0;
            else if (s % 4 == 0) prob = 0.45 + 0.45 * d;
            else if (s % 2 == 0) prob = 0.65 * d;
            else                 prob = 0.30 * d * d;
            p[size_t (s)] = rng.uni() < prob;
        }
    };
    for (int b = 0; b < numBars; ++b)
    {
        if (b == 0) { fresh (bars[0]); continue; }
        if (rng.uni() < 0.5)
        {
            bars[size_t (b)] = bars[0];
            if (rng.uni() < 0.5)
            {
                const size_t s = size_t (1 + int (rng.uni() * 15));
                bars[size_t (b)][s] = ! bars[size_t (b)][s];
            }
        }
        else
            fresh (bars[size_t (b)]);
    }

    std::vector<int> onsets;
    for (int s = 0; s < total; ++s)
        if (bars[size_t (s / 16)][size_t (s % 16)] || s % spc == 0) onsets.push_back (s);

    int prev = centre, dir = 0, lastInterval = 0;
    const double spread = 1.2 + std::clamp (o.leaps, 0.0, 1.0) * 7.0;

    for (size_t idx = 0; idx < onsets.size(); ++idx)
    {
        const int s = onsets[idx];
        const Chord& ch = chords[size_t (s / spc)];
        const auto cpcs = chordPitchClasses (ch);
        auto isChordTone = [&] (int pc) { return std::find (cpcs.begin(), cpcs.end(), pc) != cpcs.end(); };
        const bool chordStart = s % spc == 0;
        const bool strong = s % 4 == 0;

        std::vector<std::pair<int, double>> cand;
        double sum = 0;
        for (int q = low; q <= high; ++q)
        {
            const int pc = mod12 (q);
            const bool ct = isChordTone (pc);
            const bool sc = inScale (pc, o.key);
            if (! ct && ! sc) continue;

            double w;
            if (chordStart)  w = ct ? 1.0 : 0.0;
            else if (strong) w = ct ? 1.0 : 0.12;
            else             w = ct ? (sc ? 1.25 : 0.6) : 1.0;
            if (w <= 0) continue;

            const int iv = q - prev;
            w *= std::exp (-double (std::abs (iv)) / spread);
            if (iv == 0) w *= (s % 2 == 1 ? 0.3 : 0.6);
            if (dir != 0 && iv != 0 && ((iv > 0) == (dir > 0))) w *= 1.25;
            if (std::abs (lastInterval) >= 5 && iv != 0 && ((iv > 0) != (lastInterval > 0)) && std::abs (iv) <= 2) w *= 1.8;
            w *= std::exp (-double (std::abs (q - centre)) / 10.0);

            cand.push_back ({ q, w });
            sum += w;
        }

        int pitch = prev;
        if (cand.empty() || sum <= 0)
        {
            int bestDist = 1000;
            for (int q = low; q <= high; ++q)
                if (isChordTone (mod12 (q)) && std::abs (q - prev) < bestDist) { bestDist = std::abs (q - prev); pitch = q; }
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

        const int next = idx + 1 < onsets.size() ? onsets[idx + 1] : total;
        const int cap = 8 - int (d * 4);
        const int lenSteps = std::max (1, std::min (next - s, cap));
        int vel = strong ? 98 : (s % 2 == 0 ? 86 : 72);
        vel = std::clamp (vel + int (rng.uni() * 11) - 5, 40, 120);

        out.push_back ({ pitch, s * kStepTicks, std::max (1, int (lenSteps * kStepTicks * 0.95)), vel });
        dir = (pitch > prev) - (pitch < prev);
        lastInterval = pitch - prev;
        prev = pitch;
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
                                    int numChords, bool sevenths, double adventure)
{
    numChords = std::clamp (numChords, 1, kMaxChords);
    adventure = std::clamp (adventure, 0.0, 1.0);
    const auto cands = buildCandidates (k, sevenths);
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
            f += cands[c].extra ? -0.04 * (1.0 - adventure) : 0.03;
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
                const double trans = (p == c) ? 0.2 : weightFrom (cands[p].chord, cands[c], k, adventure, nullptr) / 10.0;
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
