#include "Project.h"

#include <algorithm>
#include <locale>
#include <sstream>

namespace cm
{
namespace
{
int effectiveColour (const Project& p)
{
    int c = std::clamp (p.colour, 0, kNumColours - 1);
    if (p.sevenths && c == 0) c = 1;
    return c;
}
}

void Project::applyGenreDefaults()
{
    const auto& gp = genreProfile (genre);
    setColour (gp.colour);
    chordStyle = gp.chordStyle;
    stepsPerChord = gp.stepsPerChord;
    adventure = gp.adventure;
    density = gp.energy;
    leaps = gp.movement;
    tension = gp.tension;
    contour = gp.contour;
    setRegister (gp.reg);
    ensemble = gp.ensemble;
    poly = gp.poly;
    presetIndex = std::clamp (presetIndex, 0, std::max (0, int (presetNames (genre, key).size()) - 1));
}

void Project::setColour (int c)
{
    colour = std::clamp (c, 0, kNumColours - 1);
    sevenths = colour >= 1;
    for (auto& ch : progression) ch = adaptColour (ch, key, colour);
}

void Project::setRegister (int r)
{
    registerIndex = std::clamp (r, 0, kNumRegisters - 1);
    registerRange (registerIndex, lowNote, highNote);
}

void Project::applyPreset()
{
    progression = presetChords (genre, key, presetIndex, effectiveColour (*this));
    progLength = int (progression.size());
}

void Project::randomProgression()
{
    ProgressionOptions o;
    o.key = key;
    o.length = progLength;
    o.sevenths = sevenths;
    o.colour = effectiveColour (*this);
    o.adventure = adventure;
    o.seed = progSeed;
    o.genre = genre;
    progression = generateProgression (o);
}

void Project::reharmonizeNow()
{
    progression = reharmonize (progression, key, effectiveColour (*this), adventure, genre, progSeed);
}

void Project::generateMelodyNow()
{
    MelodyOptions o;
    o.key = key;
    o.stepsPerChord = stepsPerChord;
    o.density = density;
    o.leaps = leaps;
    o.tension = tension;
    o.contour = contour;
    o.genre = genre;
    o.lowNote = lowNote;
    o.highNote = highNote;
    o.seed = melodySeed;
    o.resolveEnd = resolveEnd;
    melody = generateMelody (progression, o);
}

std::vector<Note> Project::processedCapture() const
{
    auto notes = capture;
    if (snapToScale) snapNotesToScale (notes, key);
    if (quantGrid > 0) quantizeNotes (notes, quantGrid, quantStrength, quantLength);
    return notes;
}

void Project::harmonizeCapture()
{
    const auto notes = processedCapture();
    if (notes.empty()) return;

    const int slot = std::max (1, stepsPerChord) * kStepTicks;
    int end = 0;
    for (const auto& n : notes) end = std::max (end, n.startTick + n.lengthTick);
    const int slotsNeeded = (end + slot - 1) / slot;
    const int num = std::clamp (std::max (slotsNeeded, 1), 1, kMaxChords);

    progression = harmonizeMelody (notes, key, stepsPerChord, num, sevenths, adventure, genre, effectiveColour (*this));
    progLength = int (progression.size());
}

std::vector<Part> Project::arrangement() const
{
    ArrangeOptions o;
    o.key = key;
    o.ensemble = ensemble;
    o.poly = poly;
    o.style = chordStyle;
    o.stepsPerChord = stepsPerChord;
    o.bass = bass;
    o.bassStyle = bassStyle();
    o.swing = genreProfile (genre).swing;
    return arrange (progression, o);
}

std::vector<Note> Project::chordNotes() const
{
    return mergeParts (arrangement());
}

std::vector<std::string> Project::polychordNames() const
{
    std::vector<std::string> out;
    for (const auto& c : progression)
    {
        if (poly == Polyharmony::UpperTriad) out.push_back (polychordName (c, upperStructure (c, key), key));
        else if (poly == Polyharmony::Extensions) out.push_back (chordName (adaptColour (c, key, 2), key));
        else out.push_back (chordName (c, key));
    }
    return out;
}

int Project::lengthTicks() const
{
    return int (progression.size()) * std::max (1, stepsPerChord) * kStepTicks;
}

int Project::captureLengthTicks (const std::vector<Note>& notes)
{
    int end = 0;
    for (const auto& n : notes) end = std::max (end, n.startTick + n.lengthTick);
    const int bar = 4 * kPPQ;
    return std::max (bar, ((end + bar - 1) / bar) * bar);
}

// ------------------------------------------------------------ serializzazione
namespace
{
std::string notesToString (const std::vector<Note>& v)
{
    std::ostringstream os;
    os.imbue (std::locale::classic());
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i) os << ';';
        os << v[i].pitch << ':' << v[i].startTick << ':' << v[i].lengthTick << ':' << v[i].velocity;
    }
    return os.str();
}

std::vector<std::string> split (const std::string& s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == sep) { out.push_back (cur); cur.clear(); }
        else cur += c;
    }
    if (! cur.empty() || ! out.empty()) out.push_back (cur);
    return out;
}

int toInt (const std::string& s, int fallback = 0)
{
    try { return std::stoi (s); } catch (...) { return fallback; }
}

std::vector<Note> notesFromString (const std::string& s)
{
    std::vector<Note> out;
    for (const auto& item : split (s, ';'))
    {
        if (int (out.size()) >= kMaxStoredNotes) break;
        const auto f = split (item, ':');
        if (f.size() != 4) continue;
        out.push_back ({ std::clamp (toInt (f[0]), 0, 127), std::clamp (toInt (f[1]), 0, kMaxStoredTick),
                         std::clamp (toInt (f[2]), 1, kMaxStoredTick), std::clamp (toInt (f[3], 90), 1, 127) });
    }
    return out;
}
}

std::string Project::serialize() const
{
    std::ostringstream os;
    os.imbue (std::locale::classic());
    os << "ChordMateProject v1\n";
    os << "key=" << key.root << "\n";
    os << "scale=" << int (key.scale) << "\n";
    os << "sevenths=" << int (sevenths) << "\n";
    os << "adventure=" << int (adventure * 1000 + 0.5) << "\n";
    os << "preset=" << presetIndex << "\n";
    os << "length=" << progLength << "\n";
    os << "spc=" << stepsPerChord << "\n";
    os << "style=" << int (chordStyle) << "\n";
    os << "bass=" << int (bass) << "\n";
    os << "progSeed=" << progSeed << "\n";
    os << "density=" << int (density * 1000 + 0.5) << "\n";
    os << "leaps=" << int (leaps * 1000 + 0.5) << "\n";
    os << "low=" << lowNote << "\n";
    os << "high=" << highNote << "\n";
    os << "melodySeed=" << melodySeed << "\n";
    os << "resolve=" << int (resolveEnd) << "\n";
    os << "qgrid=" << quantGrid << "\n";
    os << "qstr=" << int (quantStrength * 1000 + 0.5) << "\n";
    os << "qlen=" << int (quantLength) << "\n";
    os << "snap=" << int (snapToScale) << "\n";
    os << "volume=" << int (volume * 1000 + 0.5) << "\n";
    os << "uiscale=" << int (uiScale * 1000 + 0.5) << "\n";
    os << "genre=" << int (genre) << "\n";
    os << "colour=" << colour << "\n";
    os << "tension=" << int (tension * 1000 + 0.5) << "\n";
    os << "contour=" << int (contour) << "\n";
    os << "register=" << registerIndex << "\n";
    os << "ensemble=" << int (ensemble) << "\n";
    os << "poly=" << int (poly) << "\n";

    os << "prog=";
    for (size_t i = 0; i < progression.size(); ++i)
    {
        if (i) os << ',';
        os << progression[i].root << ':' << int (progression[i].quality);
    }
    os << "\n";
    os << "mel=" << notesToString (melody) << "\n";
    os << "cap=" << notesToString (capture) << "\n";
    return os.str();
}

bool Project::deserialize (const std::string& text)
{
    if (text.size() > kMaxStateBytes || text.rfind ("ChordMateProject", 0) != 0) return false;

    Project p;   // parte dai default: i campi mancanti restano validi
    bool haveColour = false;
    for (const auto& line : split (text, '\n'))
    {
        const auto eq = line.find ('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr (0, eq), v = line.substr (eq + 1);

        if      (k == "key")        p.key.root = std::clamp (toInt (v), 0, 11);
        else if (k == "scale")      p.key.scale = scaleFromIndex (toInt (v));
        else if (k == "sevenths")   p.sevenths = toInt (v) != 0;
        else if (k == "adventure")  p.adventure = std::clamp (toInt (v) / 1000.0, 0.0, 1.0);
        else if (k == "preset")     p.presetIndex = std::clamp (toInt (v), 0, 255);
        else if (k == "length")     p.progLength = std::clamp (toInt (v, 4), 1, kMaxChords);
        else if (k == "spc")        p.stepsPerChord = std::clamp (toInt (v, 16), 4, 64);
        else if (k == "style")      p.chordStyle = ChordStyle (std::clamp (toInt (v), 0, kNumChordStyles - 1));
        else if (k == "bass")       p.bass = toInt (v) != 0;
        else if (k == "progSeed")   p.progSeed = uint32_t (std::max (0, toInt (v, 1)));
        else if (k == "density")    p.density = std::clamp (toInt (v) / 1000.0, 0.0, 1.0);
        else if (k == "leaps")      p.leaps = std::clamp (toInt (v) / 1000.0, 0.0, 1.0);
        else if (k == "low")        p.lowNote = std::clamp (toInt (v, 60), 0, 127);
        else if (k == "high")       p.highNote = std::clamp (toInt (v, 84), 0, 127);
        else if (k == "melodySeed") p.melodySeed = uint32_t (std::max (0, toInt (v, 1)));
        else if (k == "resolve")    p.resolveEnd = toInt (v) != 0;
        else if (k == "qgrid")      p.quantGrid = std::clamp (toInt (v), 0, kPPQ);
        else if (k == "qstr")       p.quantStrength = std::clamp (toInt (v) / 1000.0, 0.0, 1.0);
        else if (k == "qlen")       p.quantLength = toInt (v) != 0;
        else if (k == "snap")       p.snapToScale = toInt (v) != 0;
        else if (k == "volume")     p.volume = std::clamp (toInt (v) / 1000.0, 0.0, 1.0);
        else if (k == "uiscale")    p.uiScale = std::clamp (toInt (v, 1000) / 1000.0, 0.6, 2.0);
        else if (k == "genre")      p.genre = genreFromIndex (toInt (v));
        else if (k == "colour")     { p.colour = std::clamp (toInt (v), 0, kNumColours - 1); haveColour = true; }
        else if (k == "tension")    p.tension = std::clamp (toInt (v, 250) / 1000.0, 0.0, 1.0);
        else if (k == "contour")    p.contour = Contour (std::clamp (toInt (v), 0, kNumContours - 1));
        else if (k == "register")   p.registerIndex = std::clamp (toInt (v, 2), 0, kNumRegisters - 1);
        else if (k == "ensemble")   p.ensemble = ensembleFromIndex (toInt (v));
        else if (k == "poly")       p.poly = polyharmonyFromIndex (toInt (v));
        else if (k == "prog")
        {
            for (const auto& item : split (v, ','))
            {
                const auto f = split (item, ':');
                if (f.size() != 2 || int (p.progression.size()) >= kMaxChords) continue;
                p.progression.push_back ({ std::clamp (toInt (f[0]), 0, 11),
                                           Quality (std::clamp (toInt (f[1]), 0, kNumQualities - 1)) });
            }
        }
        else if (k == "mel") p.melody = notesFromString (v);
        else if (k == "cap") p.capture = notesFromString (v);
    }
    if (! haveColour) p.colour = p.sevenths ? 1 : 0;      // progetti salvati prima dei colori
    *this = p;
    return true;
}

} // namespace cm
