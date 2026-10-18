// Test del motore di teoria di ChordMate (nessuna dipendenza da JUCE).
#include "Engine/MidiWriter.h"
#include "Engine/Project.h"
#include "Engine/Theory.h"
#include "Engine/Style.h"

#include <array>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <set>

using namespace cm;

static int g_checks = 0, g_failed = 0;

#define CHECK(...)                                                                     \
    do { ++g_checks; if (! (__VA_ARGS__)) { ++g_failed;                                \
        std::printf ("  FAIL  %s:%d  %s\n", __FILE__, __LINE__, #__VA_ARGS__); } } while (0)

#define CHECK_EQ(a, b)                                                                 \
    do { ++g_checks; auto va_ = (a); auto vb_ = (b); if (! (va_ == vb_)) { ++g_failed; \
        std::cout << "  FAIL  " << __FILE__ << ":" << __LINE__ << "  " #a " == " #b     \
                  << "   (" << va_ << " vs " << vb_ << ")\n"; } } while (0)

static std::vector<std::string> names (const std::vector<Chord>& cs, const Key& k)
{
    std::vector<std::string> out;
    for (auto& c : cs) out.push_back (chordName (c, k));
    return out;
}

static uint16_t maskOf (std::initializer_list<int> pcs)
{
    uint16_t m = 0;
    for (int p : pcs) m |= uint16_t (1u << (p % 12));
    return m;
}

static void testScalesAndChords()
{
    std::puts ("scale e accordi");
    const Key C { 0, ScaleType::Major };
    CHECK ((names (diatonicChords (C, false), C) == std::vector<std::string> { "C", "Dm", "Em", "F", "G", "Am", "Bdim" }));
    CHECK ((names (diatonicChords (C, true), C) == std::vector<std::string> { "Cmaj7", "Dm7", "Em7", "Fmaj7", "G7", "Am7", "Bm7b5" }));

    const Key Am { 9, ScaleType::NaturalMinor };
    CHECK ((names (diatonicChords (Am, false), Am) == std::vector<std::string> { "Am", "Bdim", "C", "Dm", "Em", "F", "G" }));

    const Key F { 5, ScaleType::Major };
    CHECK (keyUsesFlats (F));
    CHECK ((names (diatonicChords (F, false), F) == std::vector<std::string> { "F", "Gm", "Am", "Bb", "C", "Dm", "Edim" }));
    CHECK (! keyUsesFlats (Key { 7, ScaleType::Major }));
    CHECK (keyUsesFlats (Key { 2, ScaleType::NaturalMinor }));   // Re minore = Fa maggiore

    // minore armonica: III aumentato, V maggiore
    const Key Ah { 9, ScaleType::HarmonicMinor };
    const auto h = diatonicChords (Ah, false);
    CHECK_EQ (int (h[2].quality), int (Quality::Aug));
    CHECK_EQ (int (h[4].quality), int (Quality::Maj));

    // pentatoniche -> accordi dalla scala parente
    CHECK ((names (diatonicChords (Key { 0, ScaleType::MajorPentatonic }, false), C) == names (diatonicChords (C, false), C)));

    CHECK_EQ (romanNumeral (Chord { 0, Quality::Maj }, C), std::string ("I"));
    CHECK_EQ (romanNumeral (Chord { 2, Quality::Min }, C), std::string ("ii"));
    CHECK_EQ (romanNumeral (Chord { 11, Quality::Dim }, C), std::string ("vii°"));
    CHECK_EQ (romanNumeral (Chord { 7, Quality::Dom7 }, C), std::string ("V7"));
    CHECK_EQ (romanNumeral (Chord { 10, Quality::Maj }, C), std::string ("♭VII"));
    CHECK_EQ (romanNumeral (Chord { 5, Quality::Maj }, Am), std::string ("VI"));
    CHECK_EQ (romanNumeral (Chord { 5, Quality::Min }, Am), std::string ("♭vi"));   // Fam in La minore: non diatonico
    CHECK_EQ (romanNumeral (Chord { 2, Quality::Min }, Am), std::string ("iv"));

    CHECK (isDiatonicChord (Chord { 7, Quality::Maj }, Am));      // V armonico accettato in minore
    CHECK (! isDiatonicChord (Chord { 4, Quality::Dom7 }, C));    // E7 in Do
}

static void testDetection()
{
    std::puts ("riconoscimento accordi e tonalità");
    auto d = detectChord (maskOf ({ 0, 4, 7 }));
    CHECK (d.valid && d.chord == Chord { 0, Quality::Maj });
    d = detectChord (maskOf ({ 9, 0, 4 }));
    CHECK (d.valid && d.chord == Chord { 9, Quality::Min });
    d = detectChord (maskOf ({ 7, 11, 2, 5 }));
    CHECK (d.valid && d.chord == Chord { 7, Quality::Dom7 });
    d = detectChord (maskOf ({ 0, 4, 7, 11 }));
    CHECK (d.valid && d.chord == Chord { 0, Quality::Maj7 });
    d = detectChord (maskOf ({ 4, 7, 0 }), 4);                      // rivolto: basso = Mi
    CHECK (d.valid && d.chord == Chord { 0, Quality::Maj });
    d = detectChord (maskOf ({ 0, 4 }));
    CHECK (! d.valid);

    // tonalità: profilo di Krumhansl ruotato = esattamente quella tonalità
    static const double minProfile[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    std::array<double, 12> h {};
    for (int i = 0; i < 12; ++i) h[size_t ((9 + i) % 12)] = minProfile[i];
    auto g = detectKey (h);
    CHECK (! g.empty() && g[0].key.root == 9 && g[0].key.scale == ScaleType::NaturalMinor);

    std::array<double, 12> cmaj {};
    const int w[12] = { 5, 0, 3, 0, 4, 3, 0, 5, 0, 3, 0, 2 };   // note della scala di Do, tonica e quinta forti
    for (int i = 0; i < 12; ++i) cmaj[size_t (i)] = w[i];
    g = detectKey (cmaj);
    CHECK (! g.empty() && g[0].key.root == 0 && g[0].key.scale == ScaleType::Major);
    CHECK (detectKey (std::array<double, 12> {}).empty());
}

static void testSuggestions()
{
    std::puts ("suggerimenti accordi");
    const Key C { 0, ScaleType::Major };
    auto s = suggestNext (Chord { 7, Quality::Maj }, C, false, 0.3, 6);
    CHECK (! s.empty() && s[0].chord == Chord { 0, Quality::Maj });
    s = suggestNext (Chord { 2, Quality::Min }, C, false, 0.3, 6);
    CHECK (! s.empty() && s[0].chord == Chord { 7, Quality::Maj });

    s = suggestNext (Chord { 0, Quality::Maj }, C, false, 0.3, 4);
    std::set<int> roots;
    for (auto& x : s) roots.insert (x.chord.root);
    CHECK (roots.count (5) && roots.count (7) && roots.count (9));        // F, G, Am nelle prime 4

    s = suggestNext (Chord { 7, Quality::Dom7 }, C, true, 0.3, 3);
    CHECK (! s.empty() && s[0].chord == Chord { 0, Quality::Maj7 });

    // dominante secondaria: E7 in Do -> Am
    s = suggestNext (Chord { 4, Quality::Dom7 }, C, false, 0.3, 3);
    CHECK (! s.empty() && s[0].chord.root == 9);
    s = suggestNext (Chord { 2, Quality::Dom7 }, C, false, 0.3, 3);
    CHECK (! s.empty() && s[0].chord.root == 7);

    // minore: dopo Dm (iv) il primo consiglio è E maggiore (V armonico), poi si torna a Am
    const Key Am { 9, ScaleType::NaturalMinor };
    s = suggestNext (Chord { 2, Quality::Min }, Am, false, 0.0, 4);
    CHECK (! s.empty() && s[0].chord == Chord { 4, Quality::Maj });
    s = suggestNext (Chord { 4, Quality::Maj }, Am, false, 0.0, 3);
    CHECK (! s.empty() && s[0].chord == Chord { 9, Quality::Min });

    // adventure 0: nessun accordo "colore" in tonalità maggiore
    s = suggestNext (Chord { 0, Quality::Maj }, C, false, 0.0, 20);
    bool anyBorrowed = false;
    for (auto& x : s) anyBorrowed |= x.borrowed;
    CHECK (! anyBorrowed);
    s = suggestNext (Chord { 0, Quality::Maj }, C, false, 1.0, 20);
    anyBorrowed = false;
    for (auto& x : s) anyBorrowed |= x.borrowed;
    CHECK (anyBorrowed);
    for (auto& x : s) CHECK (! x.reason.empty());
}

static void testProgressions()
{
    std::puts ("progressioni");
    const Key C { 0, ScaleType::Major };
    ProgressionOptions o;
    o.key = C; o.length = 6; o.seed = 42; o.adventure = 0.0;
    auto a = generateProgression (o), b = generateProgression (o);
    CHECK (a == std::vector<Chord> (b));
    CHECK_EQ (int (a.size()), 6);
    CHECK (a[0] == Chord { 0, Quality::Maj });
    for (auto& c : a) CHECK (isDiatonicChord (c, C));                    // adventure 0 = solo diatonici

    std::set<std::string> distinct;
    for (uint32_t seed = 1; seed <= 20; ++seed)
    {
        o.seed = seed; o.adventure = 0.5;
        auto p = generateProgression (o);
        std::string key;
        for (auto& c : p) key += chordName (c, C) + " ";
        distinct.insert (key);
        CHECK (p[0] == Chord { 0, Quality::Maj });
    }
    CHECK (distinct.size() >= 8);

    const Key Am { 9, ScaleType::NaturalMinor };
    CHECK ((names (chordsFromPreset (Am, 1, false), Am) == std::vector<std::string> { "Am", "Dm", "E", "Am" }));
    CHECK ((names (chordsFromPreset (C, 0, false), C) == std::vector<std::string> { "C", "G", "Am", "F" }));
    CHECK ((names (chordsFromPreset (C, 3, true), C) == std::vector<std::string> { "Dm7", "G7", "Cmaj7", "Cmaj7" }));
    for (auto& p : presetsFor (C)) CHECK (! p.degrees.empty() && int (p.degrees.size()) <= kMaxChords);
    for (auto& p : presetsFor (Am)) CHECK (! p.degrees.empty() && int (p.degrees.size()) <= kMaxChords);
}

static void testVoicingAndRender()
{
    std::puts ("voicing e rendering accordi");
    const Key C { 0, ScaleType::Major };
    const auto prog = chordsFromPreset (C, 0, false);
    const auto v = voiceProgression (prog);
    CHECK_EQ (int (v.size()), 4);
    for (size_t i = 0; i < v.size(); ++i)
    {
        CHECK_EQ (int (v[i].size()), 3);
        for (int p : v[i]) CHECK (p >= 45 && p <= 79);
        if (i > 0)
        {
            int move = 0;
            for (size_t j = 0; j < 3; ++j) move += std::abs (v[i][j] - v[i - 1][j]);
            CHECK (move <= 8);
        }
    }

    RenderOptions r;
    r.style = ChordStyle::Block;
    CHECK_EQ (int (renderChords (prog, r).size()), 16);                  // 4 x (3 + basso)
    r.style = ChordStyle::Arp16;
    CHECK_EQ (int (renderChords (prog, r).size()), 4 * 16 + 4);
    r.style = ChordStyle::Offbeat;
    CHECK_EQ (int (renderChords (prog, r).size()), 4 * 4 * 3 + 4);
    r.style = ChordStyle::ArpUpDown8;
    CHECK_EQ (int (renderChords (prog, r).size()), 4 * 8 + 4);
    r.bass = false;
    CHECK_EQ (int (renderChords (prog, r).size()), 4 * 8);

    // le note non escono dalla lunghezza della progressione
    r.style = ChordStyle::Block;
    for (auto& n : renderChords (prog, r)) CHECK (n.startTick + n.lengthTick <= 4 * 16 * kStepTicks);
}

static void testMelody()
{
    std::puts ("melodia");
    const Key C { 0, ScaleType::Major };
    const auto prog = chordsFromPreset (C, 0, false);
    MelodyOptions o;
    o.key = C; o.seed = 7;

    const auto a = generateMelody (prog, o), b = generateMelody (prog, o);
    CHECK (a == b);
    CHECK (! a.empty());
    CHECK_EQ (a[0].startTick, 0);

    auto chordTone = [&] (const Note& n, int stepsPerChord)
    {
        const auto& ch = prog[size_t ((n.startTick / kStepTicks) / stepsPerChord)];
        const auto pcs = chordPitchClasses (ch);
        return std::find (pcs.begin(), pcs.end(), n.pitch % 12) != pcs.end();
    };
    CHECK (chordTone (a[0], 16));

    for (size_t i = 0; i < a.size(); ++i)
    {
        CHECK (a[i].pitch >= 60 && a[i].pitch <= 84);
        const auto& ch = prog[size_t ((a[i].startTick / kStepTicks) / 16)];
        const auto pcs = chordPitchClasses (ch);
        const bool ok = inScale (a[i].pitch, C) || std::find (pcs.begin(), pcs.end(), a[i].pitch % 12) != pcs.end();
        CHECK (ok);
        if (i + 1 < a.size()) CHECK (a[i].startTick + a[i].lengthTick <= a[i + 1].startTick);   // monofonica
    }
    CHECK (a.back().pitch % 12 == prog.back().root);                      // chiude sulla fondamentale dell'ultimo accordo

    // la densità si sente: più note con density alta
    double sparse = 0, dense = 0;
    for (uint32_t s = 1; s <= 30; ++s)
    {
        o.seed = s;
        o.density = 0.1; sparse += double (generateMelody (prog, o).size());
        o.density = 0.9; dense += double (generateMelody (prog, o).size());
    }
    CHECK (dense > sparse * 1.5);

    // pentatonica: solo note della scala (o dell'accordo corrente)
    o.key = Key { 0, ScaleType::MinorPentatonic };
    o.seed = 3;
    for (auto& n : generateMelody (prog, o)) CHECK (n.pitch >= 60 && n.pitch <= 84);

    // salti: con leaps alti l'intervallo medio cresce
    double small = 0, large = 0;
    for (uint32_t s = 1; s <= 30; ++s)
    {
        o.key = C; o.seed = s; o.density = 0.7;
        auto avg = [&] (double leaps)
        {
            o.leaps = leaps;
            auto m = generateMelody (prog, o);
            double sum = 0; int n = 0;
            for (size_t i = 1; i < m.size(); ++i) { sum += std::abs (m[i].pitch - m[i - 1].pitch); ++n; }
            return n ? sum / n : 0.0;
        };
        small += avg (0.0); large += avg (1.0);
    }
    CHECK (large > small);

    CHECK (generateMelody ({}, o).empty());
}

static void testMidiTools()
{
    std::puts ("strumenti MIDI");
    std::vector<Note> notes = { { 60, 130, 100, 90 } };
    quantizeNotes (notes, 120, 1.0, false);
    CHECK_EQ (notes[0].startTick, 120);
    notes = { { 60, 130, 100, 90 } };
    quantizeNotes (notes, 120, 0.5, false);
    CHECK_EQ (notes[0].startTick, 125);
    notes = { { 60, 130, 100, 90 } };
    quantizeNotes (notes, 120, 0.0, false);
    CHECK_EQ (notes[0].startTick, 130);
    notes = { { 60, 130, 100, 90 } };
    quantizeNotes (notes, 120, 1.0, true);
    CHECK_EQ (notes[0].startTick, 120);
    CHECK_EQ (notes[0].lengthTick, 120);
    notes = { { 60, 1, 50, 90 } };
    quantizeNotes (notes, 0, 1.0, false);                                 // griglia off = nessun cambiamento
    CHECK_EQ (notes[0].startTick, 1);

    const Key C { 0, ScaleType::Major };
    notes = { { 61, 0, 100, 90 }, { 63, 0, 100, 90 }, { 66, 0, 100, 90 }, { 64, 0, 100, 90 } };
    snapNotesToScale (notes, C);
    for (auto& n : notes) CHECK (inScale (n.pitch, C));
    CHECK_EQ (notes[0].pitch, 60);                                        // Do# -> Do (in basso a parità)
    CHECK_EQ (notes[3].pitch, 64);                                        // già in scala

    // armonizzazione
    auto bar = [] (std::vector<Note>& v, int b, int p0, int p1, int p2)
    {
        const int t = b * 4 * kPPQ;
        v.push_back ({ p0, t, kPPQ, 90 });
        v.push_back ({ p1, t + kPPQ, kPPQ, 90 });
        v.push_back ({ p2, t + 2 * kPPQ, kPPQ, 90 });
    };
    std::vector<Note> mel;
    bar (mel, 0, 64, 67, 72);   // E G C
    bar (mel, 1, 65, 69, 72);   // F A C
    bar (mel, 2, 62, 67, 71);   // D G B
    bar (mel, 3, 60, 64, 67);   // C E G
    auto h = harmonizeMelody (mel, C, 16, 4, false, 0.3);
    CHECK ((names (h, C) == std::vector<std::string> { "C", "F", "G", "C" }));

    std::vector<Note> mel2;
    bar (mel2, 0, 69, 72, 76);  // A C E
    h = harmonizeMelody (mel2, C, 16, 1, false, 0.3);
    CHECK ((names (h, C) == std::vector<std::string> { "Am" }));
    CHECK_EQ (int (harmonizeMelody ({}, C, 16, 4, false, 0.3).size()), 4);   // senza note: progressione comunque valida
}

// parser minimale di SMF formato 0 per verificare il writer
struct ParsedMidi { bool ok = false; int division = 0, tracks = 0, tempoUs = 0, endTick = 0; std::vector<Note> notes; };

static ParsedMidi parseMidi (const std::vector<uint8_t>& f)
{
    ParsedMidi r;
    auto u32 = [&] (size_t i) { return (uint32_t (f[i]) << 24) | (uint32_t (f[i + 1]) << 16) | (uint32_t (f[i + 2]) << 8) | f[i + 3]; };
    if (f.size() < 22 || std::string (f.begin(), f.begin() + 4) != "MThd") return r;
    r.tracks = (f[10] << 8) | f[11];
    r.division = (f[12] << 8) | f[13];
    if (std::string (f.begin() + 14, f.begin() + 18) != "MTrk") return r;
    const size_t len = u32 (18);
    size_t i = 22;
    const size_t end = i + len;
    if (end != f.size()) return r;

    int tick = 0;
    std::vector<std::pair<int, Note>> open;   // pitch -> nota aperta
    while (i < end)
    {
        uint32_t delta = 0;
        while (true) { const uint8_t b = f[i++]; delta = (delta << 7) | (b & 0x7F); if (! (b & 0x80)) break; }
        tick += int (delta);
        const uint8_t st = f[i++];
        if (st == 0xFF)
        {
            const uint8_t type = f[i++];
            const uint8_t l = f[i++];
            if (type == 0x51) r.tempoUs = (f[i] << 16) | (f[i + 1] << 8) | f[i + 2];
            if (type == 0x2F) r.endTick = tick;
            i += l;
        }
        else if ((st & 0xF0) == 0x90 || (st & 0xF0) == 0x80)
        {
            const int p = f[i++], v = f[i++];
            if ((st & 0xF0) == 0x90 && v > 0) open.push_back ({ p, { p, tick, 0, v } });
            else
                for (size_t k = 0; k < open.size(); ++k)
                    if (open[k].first == p)
                    {
                        Note n = open[k].second;
                        n.lengthTick = tick - n.startTick;
                        r.notes.push_back (n);
                        open.erase (open.begin() + long (k));
                        break;
                    }
        }
        else return r;   // evento inatteso
    }
    r.ok = open.empty();
    return r;
}

static void testMidiWriter()
{
    std::puts ("scrittura file MIDI");
    const Key C { 0, ScaleType::Major };
    const auto prog = chordsFromPreset (C, 0, false);
    RenderOptions ro;
    const auto notes = renderChords (prog, ro);

    const int len = 4 * 4 * kPPQ;
    const auto file = buildMidiFile (notes, 123.0, len, "ChordMate Accordi");
    const auto p = parseMidi (file);
    CHECK (p.ok);
    CHECK_EQ (p.tracks, 1);
    CHECK_EQ (p.division, kPPQ);
    CHECK_EQ (p.endTick, len);
    CHECK (std::abs (p.tempoUs - int (60000000.0 / 123.0)) <= 1);
    CHECK_EQ (int (p.notes.size()), int (notes.size()));

    auto sorted = [] (std::vector<Note> v)
    {
        std::sort (v.begin(), v.end(), [] (const Note& a, const Note& b)
                   { return a.startTick != b.startTick ? a.startTick < b.startTick : a.pitch < b.pitch; });
        return v;
    };
    CHECK (sorted (p.notes) == sorted (notes));

    // la fine della clip non si accorcia sotto l'ultima nota
    const auto p2 = parseMidi (buildMidiFile (notes, 120.0, 10, ""));
    CHECK (p2.ok && p2.endTick >= notes.back().startTick);

    // lunghezza della cattura: arrotondata alla battuta
    CHECK_EQ (Project::captureLengthTicks ({ { 60, 0, 100, 90 } }), 4 * kPPQ);
    CHECK_EQ (Project::captureLengthTicks ({ { 60, 4 * kPPQ, kPPQ, 90 } }), 2 * 4 * kPPQ);

    // note vuote: file valido
    const auto p3 = parseMidi (buildMidiFile ({}, 120.0, 4 * kPPQ, "vuoto"));
    CHECK (p3.ok && p3.notes.empty() && p3.endTick == 4 * kPPQ);
}

static void testProject()
{
    std::puts ("progetto: serializzazione e azioni");
    Project a;
    a.key = { 9, ScaleType::Dorian };
    a.sevenths = true;
    a.adventure = 0.65;
    a.stepsPerChord = 8;
    a.chordStyle = ChordStyle::ArpUpDown8;
    a.bass = false;
    a.progSeed = 99;
    a.progLength = 6;
    a.density = 0.8;
    a.melodySeed = 5;
    a.quantGrid = 60;
    a.snapToScale = true;
    a.volume = 0.5;
    a.uiScale = 1.25;
    a.randomProgression();
    a.generateMelodyNow();
    a.capture = { { 60, 10, 200, 80 }, { 64, 500, 100, 70 } };

    CHECK_EQ (int (a.progression.size()), 6);
    CHECK (! a.melody.empty());

    Project b;
    CHECK (b.deserialize (a.serialize()));
    CHECK (b.progression == a.progression);
    CHECK (b.melody == a.melody);
    CHECK (b.capture == a.capture);
    CHECK (b.key.root == 9 && b.key.scale == ScaleType::Dorian);
    CHECK (b.sevenths && ! b.bass && b.stepsPerChord == 8 && b.chordStyle == ChordStyle::ArpUpDown8);
    CHECK (std::abs (b.adventure - 0.65) < 0.002 && std::abs (b.density - 0.8) < 0.002 && std::abs (b.volume - 0.5) < 0.002);
    CHECK (std::abs (b.uiScale - 1.25) < 0.002);
    CHECK_EQ (b.serialize(), a.serialize());

    Project c;
    CHECK (! c.deserialize ("roba a caso"));
    CHECK (! c.deserialize (""));
    CHECK (c.deserialize ("ChordMateProject v1\nkey=99\nprog=1:2,x:y,3\nmel=garbage\n"));   // tollerante
    CHECK (c.key.root <= 11);

    // cattura -> snap + quantize -> armonizza
    Project p;
    p.key = { 0, ScaleType::Major };
    p.snapToScale = true;
    p.quantGrid = kPPQ;
    p.quantStrength = 1.0;
    p.capture = { { 61, 20, 400, 90 }, { 64, 500, 400, 90 }, { 67, 980, 400, 90 } };   // Do#, Mi, Sol un po' storti
    const auto proc = p.processedCapture();
    for (auto& n : proc) { CHECK (inScale (n.pitch, p.key)); CHECK (n.startTick % kPPQ == 0); }
    p.harmonizeCapture();
    CHECK_EQ (int (p.progression.size()), 1);
    CHECK (p.progression[0] == Chord { 0, Quality::Maj });

    // l'esportazione della progressione ha la lunghezza giusta
    p.progression = chordsFromPreset (p.key, 0, false);
    CHECK_EQ (p.lengthTicks(), 4 * 4 * kPPQ);
    p.applyPreset();
    CHECK_EQ (p.progLength, 4);
}


// ====================================================================== generi, colore, poliarmonia, arrangiamento
static void testRomanAndColour()
{
    std::puts ("numeri romani e colore degli accordi");
    const Key C { 0, ScaleType::Major };
    const Key Am { 9, ScaleType::NaturalMinor };
    Chord c;
    CHECK (parseRoman ("bVII", C, c) && c == Chord { 10, Quality::Maj });
    CHECK (parseRoman ("ii7", C, c) && c == Chord { 2, Quality::Min7 });
    CHECK (parseRoman ("Imaj9", C, c) && c == Chord { 0, Quality::Maj9 });
    CHECK (parseRoman ("V7b9", Am, c) && c == Chord { 4, Quality::Dom7b9 });
    CHECK (parseRoman ("vii\xC2\xB0" "7", Am, c) && c == Chord { 8, Quality::Dim7 });       // G#°7 in La minore
    CHECK (parseRoman ("ii\xC3\xB8" "7", Am, c) && c == Chord { 11, Quality::HalfDim7 });
    CHECK (parseRoman ("\xE2\x99\xAD" "VI", C, c) && c == Chord { 8, Quality::Maj });       // ♭VI
    CHECK (parseRoman ("V7sus4", C, c) && c == Chord { 7, Quality::Dom7Sus4 });
    CHECK (parseRoman ("iv6", C, c) && c == Chord { 5, Quality::Min6 });
    CHECK (! parseRoman ("X7", C, c));
    CHECK (! parseRoman ("Ifoo", C, c));

    // colore: la 9ª solo se è nella scala
    CHECK (adaptColour ({ 2, Quality::Min }, C, 1) == Chord { 2, Quality::Min7 });
    CHECK (adaptColour ({ 2, Quality::Min }, C, 2) == Chord { 2, Quality::Min9 });
    CHECK (adaptColour ({ 4, Quality::Min }, C, 2) == Chord { 4, Quality::Min7 });          // Fa# non è in Do
    CHECK (adaptColour ({ 7, Quality::Maj }, C, 2) == Chord { 7, Quality::Dom9 });
    CHECK (adaptColour ({ 0, Quality::Maj }, C, 2) == Chord { 0, Quality::Maj9 });
    CHECK (adaptColour ({ 11, Quality::Dim }, C, 1) == Chord { 11, Quality::HalfDim7 });
    CHECK (adaptColour ({ 0, Quality::Maj9 }, C, 0) == Chord { 0, Quality::Maj });
    CHECK (adaptColour ({ 7, Quality::Dom9 }, C, 1) == Chord { 7, Quality::Dom7 });
    CHECK (adaptColour ({ 4, Quality::Maj }, Am, 2) == Chord { 4, Quality::Dom7b9 });       // E7♭9 in La minore
    CHECK (adaptColour ({ 0, Quality::Six }, C, 2) == Chord { 0, Quality::Six });           // colore esplicito resta

    CHECK_EQ (chordName ({ 2, Quality::Min9 }, C), std::string ("Dm9"));
    CHECK_EQ (romanNumeral ({ 2, Quality::Min9 }, C), std::string ("ii9"));
    CHECK (isDiatonicChord ({ 0, Quality::Maj9 }, C));
    CHECK (! isDiatonicChord ({ 4, Quality::Min9 }, C));
    auto d = detectChord (maskOf ({ 9, 0, 4, 7 }));
    CHECK (d.valid && d.chord == Chord { 9, Quality::Min7 });                              // Am7 batte C6
    d = detectChord (maskOf ({ 2, 5, 9, 0, 4 }), 2);
    CHECK (d.valid && d.chord == Chord { 2, Quality::Min9 });
}

static void testGenres()
{
    std::puts ("generi: preset, suggerimenti, progressioni, rearmonizzazione");
    const Key C { 0, ScaleType::Major };
    const Key Am { 9, ScaleType::NaturalMinor };

    for (int g = 0; g < kNumGenres; ++g)
    {
        const Genre genre = genreFromIndex (g);
        const auto& gp = genreProfile (genre);
        CHECK (gp.name != nullptr && gp.description != nullptr);
        for (const Key& k : { C, Am })
        {
            const auto pn = presetNames (genre, k);
            CHECK (! pn.empty());
            const auto& list = isMinorFamily (k.scale) ? gp.minor : gp.major;
            for (int i = 0; i < int (pn.size()); ++i)
                for (int col = 0; col < kNumColours; ++col)
                {
                    const auto ch = presetChords (genre, k, i, col);
                    CHECK (! ch.empty() && int (ch.size()) <= kMaxChords);
                    if (genre != Genre::Universal)
                    {
                        // ogni simbolo del preset è stato letto
                        int tok = 1;
                        for (const char* s = list[size_t (i)].romans; *s; ++s) tok += *s == ' ';
                        CHECK_EQ (int (ch.size()), tok);
                    }
                    if (col == 0) for (auto& x : ch) CHECK (int (x.quality) < 6);   // triadi
                }
        }
        for (const auto& e : gp.majorExtras) { Chord c; CHECK (parseRoman (e.roman, C, c)); }
        for (const auto& e : gp.minorExtras) { Chord c; CHECK (parseRoman (e.roman, Am, c)); }
    }

    // Universale = comportamento storico
    CHECK ((names (presetChords (Genre::Universal, C, 0, 0), C) == std::vector<std::string> { "C", "G", "Am", "F" }));
    CHECK ((names (presetChords (Genre::Jazz, C, 0, 1), C) == std::vector<std::string> { "Dm7", "G7", "Cmaj7", "Cmaj7" }));
    CHECK ((names (presetChords (Genre::Jazz, C, 0, 2), C) == std::vector<std::string> { "Dm9", "G9", "Cmaj9", "Cmaj9" }));
    CHECK ((names (presetChords (Genre::Rock, C, 0, 0), C) == std::vector<std::string> { "C", "Bb", "F", "C" }));

    // jazz: dopo Dm7 viene G7; il rock spinge il ♭VII più della classica
    auto s = suggestNext ({ 2, Quality::Min7 }, C, true, 0.5, 6, Genre::Jazz, 1);
    CHECK (! s.empty() && s[0].chord == Chord { 7, Quality::Dom7 });
    auto countRoot = [&] (Genre g, int root)
    {
        int n = 0;
        for (uint32_t seed = 1; seed <= 60; ++seed)
        {
            ProgressionOptions o;
            o.key = C; o.length = 8; o.adventure = 0.6; o.seed = seed; o.genre = g;
            for (auto& c : generateProgression (o)) n += c.root == root;
        }
        return n;
    };
    CHECK (countRoot (Genre::Rock, 10) > countRoot (Genre::Classical, 10));
    CHECK (countRoot (Genre::Classical, 7) > countRoot (Genre::Ambient, 7));      // la classica ama la dominante

    // colore 2 = accordi estesi nelle progressioni
    ProgressionOptions o;
    o.key = C; o.length = 6; o.seed = 3; o.genre = Genre::Soul; o.colour = 2;
    const auto ext = generateProgression (o);
    bool anyExt = false;
    for (auto& c : ext) anyExt |= int (c.quality) >= kNumBasicQualities;
    CHECK (anyExt);

    // rearmonizzazione: stessa lunghezza, primo e ultimo fermi, qualcosa cambia, ripetibile
    const auto prog = presetChords (Genre::Pop, C, 0, 0);
    for (int g = 0; g < kNumGenres; ++g)
        for (uint32_t seed = 1; seed <= 6; ++seed)
        {
            const auto r = reharmonize (prog, C, 1, 0.6, genreFromIndex (g), seed);
            CHECK_EQ (r.size(), prog.size());
            CHECK (r.front() == prog.front() && r.back() == prog.back());
            CHECK (r != prog);
            CHECK (r == reharmonize (prog, C, 1, 0.6, genreFromIndex (g), seed));
        }
    CHECK (reharmonize ({ { 0, Quality::Maj }, { 7, Quality::Maj } }, C, 0, 0.5, Genre::Pop, 1).size() == 2);
}

static void testBassAndMelodyStyles()
{
    std::puts ("basso per genere e melodia a macro");
    const Key C { 0, ScaleType::Major };
    const auto prog = presetChords (Genre::Jazz, C, 1, 1);      // Imaj7 vi7 ii7 V7

    for (int b = 0; b < kNumBassStyles; ++b)
    {
        RenderOptions r;
        r.bassStyle = BassStyle (b);
        r.key = C;
        const auto notes = renderBass (prog, r);
        CHECK (! notes.empty());
        for (auto& n : notes)
        {
            CHECK (n.pitch >= 28 && n.pitch <= 60);
            CHECK (n.startTick + n.lengthTick <= int (prog.size()) * 16 * kStepTicks);
        }
        CHECK (bassStyleName (BassStyle (b))[0] != 0);
    }
    RenderOptions w;
    w.bassStyle = BassStyle::Walking;
    w.key = C;
    const auto walk = renderBass (prog, w);
    CHECK_EQ (int (walk.size()), 4 * 4);                         // un quarto per tempo
    for (size_t i = 0; i < prog.size() && i * 4 < walk.size(); ++i)
        CHECK (walk[i * 4].pitch % 12 == prog[i].root);           // il primo tempo è la fondamentale

    // melodia: per ogni genere e contorno resta monofonica, nel registro, ripetibile e chiude sulla tonica
    for (int g = 0; g < kNumGenres; ++g)
        for (int ct = 0; ct < kNumContours; ++ct)
        {
            MelodyOptions o;
            o.key = C; o.genre = genreFromIndex (g); o.contour = Contour (ct); o.seed = uint32_t (11 + g * 7 + ct);
            o.tension = 0.8; o.density = 0.7;
            const auto m = generateMelody (prog, o);
            CHECK (! m.empty());
            CHECK (m == generateMelody (prog, o));
            for (size_t i = 0; i < m.size(); ++i)
            {
                CHECK (m[i].pitch >= o.lowNote && m[i].pitch <= o.highNote);
                if (i + 1 < m.size()) CHECK (m[i].startTick + m[i].lengthTick <= m[i + 1].startTick);
            }
            CHECK (m.back().pitch % 12 == prog.back().root);
        }

    // tensione 0: solo note di scala o d'accordo; tensione alta: compaiono cromatismi
    int chromaticLow = 0, chromaticHigh = 0;
    for (uint32_t seed = 1; seed <= 30; ++seed)
    {
        MelodyOptions o;
        o.key = C; o.seed = seed; o.density = 0.8; o.genre = Genre::Pop;
        o.tension = 0.0;
        for (auto& n : generateMelody (prog, o)) chromaticLow += ! inScale (n.pitch, C);
        o.tension = 1.0;
        for (auto& n : generateMelody (prog, o)) chromaticHigh += ! inScale (n.pitch, C);
    }
    CHECK_EQ (chromaticLow, 0);
    CHECK (chromaticHigh > 0);

    // contorno ascendente: la seconda metà sta più in alto della prima
    double firstHalf = 0, secondHalf = 0;
    for (uint32_t seed = 1; seed <= 30; ++seed)
    {
        MelodyOptions o;
        o.key = C; o.seed = seed; o.contour = Contour::Rising; o.density = 0.7;
        const auto m = generateMelody (prog, o);
        const int half = int (prog.size()) * 16 * kStepTicks / 2;
        double a = 0, b = 0; int na = 0, nb = 0;
        for (auto& n : m) { if (n.startTick < half) { a += n.pitch; ++na; } else { b += n.pitch; ++nb; } }
        firstHalf += na ? a / na : 0; secondHalf += nb ? b / nb : 0;
    }
    CHECK (secondHalf > firstHalf + 30);                       // in media almeno una nota più in alto

    // swing (jazz): gli ottavi in levare slittano in avanti
    MelodyOptions j;
    j.key = C; j.genre = Genre::Jazz; j.density = 0.9; j.seed = 4;
    bool swung = false, straight = false;
    for (auto& n : generateMelody (prog, j))
    {
        swung |= n.startTick % kPPQ == kPPQ / 2 + kPPQ / 10;
        straight |= n.startTick % kPPQ == kPPQ / 2;
    }
    CHECK (swung && ! straight);

    // il motivo si ripete: nel pop molte seconde battute hanno lo stesso ritmo della prima
    MelodyOptions p;
    p.key = C; p.genre = Genre::Pop; p.density = 0.6;
    int same = 0;
    for (uint32_t seed = 1; seed <= 20; ++seed)
    {
        p.seed = seed;
        const auto m = generateMelody (prog, p);
        std::set<int> bar0, bar1;
        for (auto& n : m)
        {
            if (n.startTick < 16 * kStepTicks) bar0.insert (n.startTick);
            else if (n.startTick < 32 * kStepTicks) bar1.insert (n.startTick - 16 * kStepTicks);
        }
        same += bar0 == bar1;
    }
    CHECK (same >= 10);

    int lo = 0, hi = 0;
    for (int r = 0; r < kNumRegisters; ++r) { registerRange (r, lo, hi); CHECK (hi - lo >= 12); CHECK (registerName (r)[0] != 0); }
}

static void testPolyharmonyAndArrangement()
{
    std::puts ("poliarmonia e arrangiamento orchestrale");
    const Key C { 0, ScaleType::Major };
    const Key Am { 9, ScaleType::NaturalMinor };

    auto u = upperStructure ({ 5, Quality::Maj }, C);                 // F: triade di Sol sopra = Fmaj7(9,♯11)
    CHECK (u.valid && u.triad == Chord { 7, Quality::Maj });
    CHECK_EQ (polychordName ({ 5, Quality::Maj }, u, C), std::string ("G/F"));
    u = upperStructure ({ 0, Quality::Maj7 }, C);
    CHECK (u.valid && u.triad == Chord { 7, Quality::Maj });
    u = upperStructure ({ 2, Quality::Min }, C);
    CHECK (u.valid && u.triad == Chord { 0, Quality::Maj });          // C/Dm = Dm11
    u = upperStructure ({ 4, Quality::Dom7 }, Am);
    CHECK (u.valid && u.triad == Chord { 0, Quality::Maj });          // C/E7 = E7(♯9,♭13)
    for (int deg = 0; deg < 7; ++deg)
    {
        const Chord base = chordAtDegree (C, deg, true);
        const auto x = upperStructure (base, C);
        if (! x.valid) continue;
        for (int pc : chordPitchClasses (x.triad)) CHECK (inScale (pc, C));
    }

    // SATB: voci ordinate, nei registri, terza sempre presente, niente quinte/ottave parallele
    const auto prog = presetChords (Genre::Classical, C, 5, 0);      // corale I V vi IV I ii V I
    const std::array<VoiceRange, 4> rg { { { 40, 62 }, { 48, 67 }, { 55, 74 }, { 60, 81 } } };
    const auto v = voiceSATB (prog, rg);
    CHECK_EQ (v.size(), prog.size());
    int parallels = 0;
    for (size_t i = 0; i < v.size(); ++i)
    {
        CHECK (v[i][0] < v[i][1] && v[i][1] < v[i][2] && v[i][2] < v[i][3]);
        for (size_t k = 0; k < 4; ++k) CHECK (v[i][k] >= rg[k].low && v[i][k] <= rg[k].high);
        CHECK (v[i][0] % 12 == prog[i].root);
        const auto pcs = chordPitchClasses (prog[i]);
        bool third = false;
        for (size_t k = 1; k < 4; ++k) third |= v[i][k] % 12 == pcs[1];
        CHECK (third);
        if (i > 0)
            for (size_t a = 0; a < 4; ++a)
                for (size_t b = a + 1; b < 4; ++b)
                {
                    const int ma = v[i][a] - v[i - 1][a], mb = v[i][b] - v[i - 1][b];
                    if (ma == 0 || mb == 0 || (ma > 0) != (mb > 0)) continue;
                    const int before = ((v[i - 1][b] - v[i - 1][a]) % 12 + 12) % 12, after = ((v[i][b] - v[i][a]) % 12 + 12) % 12;
                    parallels += before == after && (after == 0 || after == 7);
                }
    }
    CHECK_EQ (parallels, 0);

    // arrangiamento: ogni ensemble e poliarmonia produce parti valide su canali distinti
    const auto epic = presetChords (Genre::Cinematic, C, 0, 0);
    for (int e = 0; e < kNumEnsembles; ++e)
        for (int p = 0; p < kNumPolyharmony; ++p)
            for (int st = 0; st < kNumChordStyles; ++st)
            {
                ArrangeOptions o;
                o.key = C; o.ensemble = ensembleFromIndex (e); o.poly = polyharmonyFromIndex (p);
                o.style = ChordStyle (st); o.bassStyle = BassStyle::Root8;
                const auto parts = arrange (epic, o);
                CHECK (parts.size() >= 2);
                std::set<int> channels;
                for (auto& part : parts)
                {
                    CHECK (! part.name.empty() && ! part.notes.empty());
                    CHECK (part.channel != 9 && part.channel >= 0 && part.channel <= 15);
                    channels.insert (part.channel);
                    for (auto& n : part.notes)
                    {
                        CHECK (n.pitch >= 24 && n.pitch <= 100);
                        CHECK (n.startTick >= 0 && n.startTick + n.lengthTick <= int (epic.size()) * 16 * kStepTicks);
                    }
                }
                CHECK_EQ (channels.size(), parts.size());
                if (o.ensemble == Ensemble::FullOrchestra) CHECK (parts.size() >= 8);
                if (o.poly == Polyharmony::UpperTriad && o.ensemble == Ensemble::FullOrchestra)
                    CHECK (parts.back().name.find ("triade superiore") != std::string::npos);
            }

    // MIDI multitraccia: formato 1, una traccia in più (tempo)
    ArrangeOptions o;
    o.key = C; o.ensemble = Ensemble::StringQuartet;
    std::vector<MidiTrack> tracks;
    for (auto& part : arrange (epic, o)) tracks.push_back ({ part.name, part.channel, part.program, part.notes });
    const auto f = buildMidiFileMulti (tracks, 100.0, 16 * kPPQ);
    CHECK (f.size() > 100 && std::string (f.begin(), f.begin() + 4) == "MThd");
    CHECK_EQ ((f[8] << 8) | f[9], 1);
    CHECK_EQ ((f[10] << 8) | f[11], int (tracks.size()) + 1);
    int chunks = 0;
    for (size_t i = 0; i + 4 <= f.size(); ++i) chunks += std::string (f.begin() + long (i), f.begin() + long (i) + 4) == "MTrk";
    CHECK_EQ (chunks, int (tracks.size()) + 1);

    // progetto: nuovi campi salvati e progetti vecchi ancora leggibili
    Project a;
    a.key = C;
    a.genre = Genre::Jazz;
    a.applyGenreDefaults();
    a.applyPreset();
    a.generateMelodyNow();
    CHECK (a.colour == 2 && a.sevenths && a.ensemble == Ensemble::Piano && a.poly == Polyharmony::Extensions);
    CHECK (! a.chordNotes().empty() && ! a.melody.empty());
    CHECK_EQ (a.polychordNames().size(), a.progression.size());
    a.ensemble = Ensemble::Choir; a.poly = Polyharmony::UpperTriad; a.contour = Contour::Wave; a.tension = 0.7;
    a.setRegister (3);
    Project b;
    CHECK (b.deserialize (a.serialize()));
    CHECK (b.genre == Genre::Jazz && b.colour == 2 && b.ensemble == Ensemble::Choir && b.poly == Polyharmony::UpperTriad);
    CHECK (b.contour == Contour::Wave && std::abs (b.tension - 0.7) < 0.002 && b.registerIndex == 3 && b.lowNote == 67);
    CHECK_EQ (b.serialize(), a.serialize());
    a.reharmonizeNow();
    CHECK_EQ (a.progression.size(), b.progression.size());

    Project old;
    CHECK (old.deserialize ("ChordMateProject v1\nsevenths=1\nprog=0:6,7:7\n"));
    CHECK (old.colour == 1 && old.genre == Genre::Universal);
    old.setColour (0);
    CHECK ((names (old.progression, C) == std::vector<std::string> { "C", "G" }));
}

int main()
{
    testScalesAndChords();
    testDetection();
    testSuggestions();
    testProgressions();
    testVoicingAndRender();
    testMelody();
    testMidiTools();
    testMidiWriter();
    testProject();
    testRomanAndColour();
    testGenres();
    testBassAndMelodyStyles();
    testPolyharmonyAndArrangement();

    std::printf ("\n%d controlli, %d falliti\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
