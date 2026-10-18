#include "Style.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace cm
{
namespace
{
int mod12 (int x) { return ((x % 12) + 12) % 12; }

bool inParent (int pc, const Key& k) { return inScale (pc, Key { k.root, harmonicParent (k.scale) }); }

std::vector<std::string> tokens (const std::string& s)
{
    std::vector<std::string> out;
    std::istringstream is (s);
    std::string t;
    while (is >> t) out.push_back (t);
    return out;
}

// "bVII IV I" -> "♭VII – IV – I"
std::string prettyRomans (const std::string& s)
{
    std::string out;
    for (const auto& t : tokens (s))
    {
        if (! out.empty()) out += " \xE2\x80\x93 ";
        std::string u = t;
        if (u.size() > 1 && u[0] == 'b') u = "\xE2\x99\xAD" + u.substr (1);
        else if (u.size() > 1 && u[0] == '#') u = "\xE2\x99\xAF" + u.substr (1);
        out += u;
    }
    return out;
}

std::array<double, 16> rhythm (std::initializer_list<double> v)
{
    std::array<double, 16> a {};
    size_t i = 0;
    for (double x : v) if (i < 16) a[i++] = x;
    return a;
}

std::vector<GenreProfile> buildProfiles()
{
    std::vector<GenreProfile> out (static_cast<size_t> (kNumGenres));
    const std::array<double, 7> flat { 1, 1, 1, 1, 1, 1, 1 };

    auto base = [&] (GenreProfile& p, const char* name, const char* desc)
    {
        p.name = name;
        p.description = desc;
        p.degreeMul = flat;
        p.degreeMulMinor = flat;
        p.colourMul = 1.0;
        p.tritoneSubs = false;
        p.colour = 0;
        p.chordStyle = ChordStyle::Block;
        p.stepsPerChord = 16;
        p.bass = BassStyle::Sustained;
        p.adventure = 0.3;
        p.customRhythm = true;
        p.rhythm = {};
        p.legato = 0.9;
        p.swing = 0.0;
        p.motif = 0.6;
        p.energy = 0.5;
        p.movement = 0.3;
        p.tension = 0.25;
        p.contour = Contour::Free;
        p.reg = 2;
        p.ensemble = Ensemble::Piano;
        p.poly = Polyharmony::Off;
    };

    {   // ---------------------------------------------------------------- Universale
        auto& p = out[size_t (Genre::Universal)];
        base (p, "Universale", "Teoria classica senza stile preciso: i preset e i suggerimenti di sempre.");
        p.customRhythm = false;
        p.legato = 0.95;
        p.motif = 0.5;
    }
    {   // ---------------------------------------------------------------- Pop
        auto& p = out[size_t (Genre::Pop)];
        base (p, "Pop", "Giri da 4 accordi, ritornelli orecchiabili, melodia che ripete il motivo.");
        p.major = { { "Il giro pop", "I V vi IV" }, { "Emotivo", "vi IV I V" }, { "Anni '50", "I vi IV V" },
                    { "Sensibile", "IV I V vi" }, { "Ballata", "I iii vi IV" }, { "Inno", "I IV vi V" },
                    { "Canone", "I V vi iii IV I IV V" }, { "Sospeso", "Isus2 V vi IV" } };
        p.minor = { { "Epico pop", "i bVI bIII bVII" }, { "Malinconico", "i bVII bVI bVII" },
                    { "Notturno", "i iv bVI bVII" }, { "Ritornello", "bVI bVII i i" }, { "Radio", "i bIII bVII bVI" } };
        p.degreeMul = { 1.1, 0.8, 0.7, 1.4, 1.3, 1.4, 0.3 };
        p.degreeMulMinor = { 1.1, 0.4, 1.3, 1.1, 0.7, 1.5, 1.4 };
        p.colourMul = 0.8;
        p.chordStyle = ChordStyle::Pulse8;
        p.bass = BassStyle::Root8;
        p.adventure = 0.25;
        p.rhythm = rhythm ({ 1, 0, .2, 0, .5, 0, .6, .15, .7, 0, .3, 0, .4, 0, .6, .1 });
        p.legato = 0.85;
        p.motif = 0.85;
        p.energy = 0.55; p.movement = 0.3; p.tension = 0.2;
        p.contour = Contour::Arch;
    }
    {   // ---------------------------------------------------------------- Rock
        auto& p = out[size_t (Genre::Rock)];
        base (p, "Rock", "Quinte potenti, prestiti misolidi (♭VII) e ottavi dritti.");
        p.major = { { "Classic rock", "I bVII IV I" }, { "Blues-rock", "I IV V IV" }, { "Stadio", "I V bVII IV" },
                    { "Mario cadence", "I bVI bVII I" }, { "Garage", "I bIII IV I" }, { "Arena", "IV I V vi" } };
        p.minor = { { "Hard rock", "i bVII bVI bVII" }, { "Dorico", "i bIII bVII IV" }, { "Grunge", "i bVI bIII bVII" },
                    { "Riff", "i iv bVII i" }, { "Discesa", "i bVII bVI V" } };
        p.degreeMul = { 1.2, 0.6, 0.6, 1.5, 1.3, 1.0, 0.3 };
        p.degreeMulMinor = { 1.2, 0.3, 1.2, 1.2, 0.8, 1.4, 1.6 };
        p.colourMul = 1.2;
        p.majorExtras = { { "bVII", 0x1D, 8, "♭VII misolidio: il suono rock per eccellenza" },
                          { "bIII", 0x19, 4, "♭III rock-blues: grinta e potenza" } };
        p.minorExtras = { { "IV", 0x41, 4, "IV dorico: il classico riff rock in minore" } };
        p.chordStyle = ChordStyle::Pulse8;
        p.bass = BassStyle::Root8;
        p.adventure = 0.4;
        p.rhythm = rhythm ({ 1, 0, .4, 0, .8, 0, .4, 0, .9, 0, .4, 0, .8, 0, .5, 0 });
        p.legato = 0.8;
        p.motif = 0.75;
        p.energy = 0.65; p.movement = 0.35; p.tension = 0.2;
        p.reg = 1;
    }
    {   // ---------------------------------------------------------------- Jazz
        auto& p = out[size_t (Genre::Jazz)];
        base (p, "Jazz", "ii-V-I, accordi estesi, dominanti secondarie, sostituzioni di tritono, swing e walking bass.");
        p.major = { { "ii-V-I", "ii7 V7 Imaj7 Imaj7" }, { "Turnaround", "Imaj7 vi7 ii7 V7" },
                    { "Rhythm changes", "Imaj7 VI7 ii7 V7" }, { "Catena di dominanti", "iii7 VI7 ii7 V7" },
                    { "Tritono", "ii7 bII7 Imaj7 Imaj7" }, { "Backdoor", "Imaj7 IVmaj7 iv7 bVII7" },
                    { "Lungo", "Imaj7 IV7 iii7 VI7 ii7 V7 Imaj7 Imaj7" } };
        p.minor = { { "ii-V-i minore", "iiø7 V7b9 i7 i7" }, { "Notturno", "i7 iv7 bVII7 bIIImaj7" },
                    { "Blues minore", "i7 iv7 bVI7 V7b9" }, { "Modale", "i7 bVImaj7 iiø7 V7b9" },
                    { "Autumn", "iv7 bVII7 bIIImaj7 bVImaj7 iiø7 V7b9 i7 i7" } };
        p.degreeMul = { 1.0, 1.6, 1.2, 1.0, 1.4, 1.3, 0.6 };
        p.degreeMulMinor = { 1.0, 1.5, 1.2, 1.1, 1.0, 1.1, 1.0 };
        p.colourMul = 1.6;
        p.tritoneSubs = true;
        p.majorExtras = { { "VI7", 0x07, 4, "VI7: dominante verso ii (turnaround)" },
                          { "bVII7", 0x08, 4, "♭VII7 backdoor: risolve morbido su I" },
                          { "iv7", 0x08, 3, "iv7 in prestito: malinconia jazz" },
                          { "#iv\xC3\xB8" "7", 0x09, 2, "♯ivø7: discesa cromatica verso IV-iii" } };
        p.minorExtras = { { "bII7", 0x12, 4, "♭II7 sostituto di tritono: scende sulla tonica" },
                          { "bVI7", 0x09, 3, "♭VI7: colore blues, scende alla dominante" } };
        p.colour = 2;
        p.bass = BassStyle::Walking;
        p.adventure = 0.6;
        p.rhythm = rhythm ({ .9, 0, .5, 0, .6, 0, .6, 0, .7, 0, .6, 0, .5, 0, .7, 0 });
        p.legato = 0.9;
        p.swing = 0.6;
        p.motif = 0.35;
        p.energy = 0.55; p.movement = 0.5; p.tension = 0.55;
        p.contour = Contour::Wave;
        p.poly = Polyharmony::Extensions;
    }
    {   // ---------------------------------------------------------------- R&B / Neo-soul
        auto& p = out[size_t (Genre::Soul)];
        base (p, "R&B / Neo-soul", "Accordi di 9ª, prestiti minori (iv7), groove sincopato e leggermente swingato.");
        p.major = { { "Neo-soul", "IVmaj7 iii7 ii7 Imaj7" }, { "Gospel", "Imaj7 III7 vi7 IV7" },
                    { "Slow jam", "ii7 V7sus4 Imaj7 vi7" }, { "Prestito", "Imaj7 iii7 IVmaj7 iv7" },
                    { "Groove", "vi7 ii7 V7sus4 Imaj7" }, { "Soul", "Imaj7 bVII9 IVmaj7 IVmaj7" } };
        p.minor = { { "R&B notturno", "i7 iv7 i7 V7b9" }, { "Neo-soul minore", "i7 bIIImaj7 iv7 bVII7" },
                    { "Dorico", "i7 IV9 i7 IV9" }, { "Smooth", "iv7 bVII7 bIIImaj7 bVImaj7" } };
        p.degreeMul = { 1.0, 1.4, 1.3, 1.4, 1.0, 1.3, 0.4 };
        p.degreeMulMinor = { 1.0, 0.8, 1.3, 1.4, 1.1, 1.2, 1.2 };
        p.colourMul = 1.3;
        p.tritoneSubs = true;
        p.majorExtras = { { "bVII9", 0x09, 4, "♭VII9: groove gospel / neo-soul" },
                          { "iv7", 0x08, 4, "iv7 in prestito: la svolta neo-soul" },
                          { "III7", 0x05, 3, "III7: dominante verso vi, colore gospel" } };
        p.minorExtras = { { "IV9", 0x01, 4, "IV9 dorico: tipico R&B in minore" } };
        p.colour = 2;
        p.bass = BassStyle::Syncopated;
        p.adventure = 0.5;
        p.rhythm = rhythm ({ .9, 0, .2, .35, .3, 0, .5, .2, .6, 0, .3, .4, .3, .2, .5, .1 });
        p.legato = 0.85;
        p.swing = 0.3;
        p.motif = 0.6;
        p.energy = 0.5; p.movement = 0.35; p.tension = 0.5;
        p.contour = Contour::Arch;
        p.reg = 1;
        p.poly = Polyharmony::Extensions;
    }
    {   // ---------------------------------------------------------------- Elettronica / Dance
        auto& p = out[size_t (Genre::Electronic)];
        base (p, "Elettronica / Dance", "Loop minori ipnotici, accordi pulsanti, basso in ottave e hook molto ripetuto.");
        p.major = { { "Festival", "vi IV I V" }, { "Euforico", "I V vi IV" }, { "Build-up", "IV V vi vi" },
                    { "Sospeso", "I Vsus4 vi IV" } };
        p.minor = { { "Progressive", "i bVI bIII bVII" }, { "Trance", "i bVII bVI bVII" }, { "Deep", "i bVI iv bVII" },
                    { "Drop", "bVI bVII i i" }, { "Techno", "i iv i bVII" } };
        p.degreeMul = { 1.0, 0.6, 0.6, 1.4, 1.3, 1.5, 0.2 };
        p.degreeMulMinor = { 1.1, 0.3, 1.3, 1.2, 0.7, 1.6, 1.5 };
        p.colourMul = 0.7;
        p.minorExtras = { { "IV", 0x01, 3, "IV dorico: apre il drop" } };
        p.chordStyle = ChordStyle::Pulse8;
        p.bass = BassStyle::Octaves;
        p.adventure = 0.2;
        p.rhythm = rhythm ({ 1, 0, .6, 0, .4, 0, .8, 0, .6, 0, .7, 0, .4, 0, .8, .2 });
        p.legato = 0.7;
        p.motif = 0.9;
        p.energy = 0.7; p.movement = 0.3; p.tension = 0.15;
        p.contour = Contour::Rising;
    }
    {   // ---------------------------------------------------------------- Hip-hop / Lo-fi
        auto& p = out[size_t (Genre::HipHop)];
        base (p, "Hip-hop / Lo-fi", "Settime morbide, loop di 4 accordi, groove laid-back con swing.");
        p.major = { { "Lo-fi", "IVmaj7 iii7 ii7 Imaj7" }, { "Boom bap", "Imaj7 vi7 ii7 V7" },
                    { "Chill", "ii7 V7 iii7 vi7" }, { "Nostalgia", "IVmaj7 iv7 Imaj7 vi7" } };
        p.minor = { { "Dark trap", "i bVI i bVII" }, { "Lo-fi minore", "i7 bVImaj7 bIIImaj7 bVII7" },
                    { "Frigio", "i bII i bVII" }, { "Soul campionato", "i7 IV7 i7 IV7" }, { "Notte", "i7 iv7 bVImaj7 V7" } };
        p.degreeMul = { 1.0, 1.3, 1.2, 1.3, 1.0, 1.4, 0.3 };
        p.degreeMulMinor = { 1.1, 0.8, 1.2, 1.3, 1.0, 1.4, 1.2 };
        p.colourMul = 1.1;
        p.majorExtras = { { "iv7", 0x08, 4, "iv7 in prestito: nostalgia lo-fi" } };
        p.minorExtras = { { "bII", 0x41, 3, "♭II frigio: scuro e minaccioso" },
                          { "IV7", 0x01, 3, "IV7 dorico: soul campionato" } };
        p.colour = 1;
        p.bass = BassStyle::Syncopated;
        p.adventure = 0.4;
        p.rhythm = rhythm ({ .8, 0, 0, .5, .2, 0, .6, 0, .3, .5, 0, .4, .3, 0, .5, .2 });
        p.legato = 0.75;
        p.swing = 0.45;
        p.motif = 0.85;
        p.energy = 0.45; p.movement = 0.3; p.tension = 0.35;
        p.contour = Contour::Falling;
        p.reg = 1;
    }
    {   // ---------------------------------------------------------------- Cinematico / Epico
        auto& p = out[size_t (Genre::Cinematic)];
        base (p, "Cinematico / Epico", "Mediante cromatiche, ♭VI-♭VII, ostinati d'archi, orchestra piena e poliaccordi.");
        p.major = { { "Eroico", "I bVI bVII I" }, { "Meraviglia", "I III IV iv" }, { "Volo", "vi IV I V" },
                    { "Mediante", "I bIII IV I" }, { "Speranza", "IV vi I V" }, { "Trailer", "I V vi bVI" } };
        p.minor = { { "Epico", "i bVI bIII bVII" }, { "Battaglia", "i bVI bVII i" }, { "Tragico", "i bVI iv V" },
                    { "Napoletano", "i bII i V" }, { "Oscurità", "i bIII bVII iv" }, { "Finale", "i bVI bVII I" } };
        p.degreeMul = { 1.0, 0.6, 0.9, 1.4, 1.1, 1.4, 0.3 };
        p.degreeMulMinor = { 1.1, 0.4, 1.3, 1.3, 0.7, 1.6, 1.4 };
        p.colourMul = 1.5;
        p.majorExtras = { { "bIII", 0x11, 4, "♭III mediante cromatica: meraviglia epica" },
                          { "III", 0x01, 3, "III maggiore: mediante cromatica luminosa" } };
        p.minorExtras = { { "I", 0x18, 3, "Terza piccarda: tonica maggiore inattesa, catarsi" },
                          { "IV", 0x01, 3, "IV dorico: speranza nell'oscurità" } };
        p.chordStyle = ChordStyle::Pulse8;
        p.adventure = 0.55;
        p.rhythm = rhythm ({ 1, 0, 0, 0, .35, 0, 0, 0, .7, 0, 0, 0, .3, 0, .15, 0 });
        p.legato = 1.0;
        p.motif = 0.6;
        p.energy = 0.35; p.movement = 0.5; p.tension = 0.4;
        p.contour = Contour::Arch;
        p.ensemble = Ensemble::FullOrchestra;
        p.poly = Polyharmony::UpperTriad;
    }
    {   // ---------------------------------------------------------------- Classico / Orchestrale
        auto& p = out[size_t (Genre::Classical)];
        base (p, "Classico / Orchestrale", "Cadenze, dominanti secondarie, condotta delle parti a 4 voci (SATB).");
        p.major = { { "Cadenza perfetta", "I IV V7 I" }, { "Cadenza composta", "I vi ii V7" },
                    { "Dominante della dominante", "I II7 V I" }, { "Canone", "I V vi iii IV I IV V" },
                    { "Barocco", "I IV vii\xC2\xB0 iii vi ii V7 I" }, { "Corale", "I V vi IV I ii V I" } };
        p.minor = { { "Cadenza minore", "i iv V7 i" }, { "Sopradominante", "i ii\xC3\xB8" "7 V7 i" },
                    { "Napoletana", "i bII V7 i" }, { "Lamento", "i bVI iv V" },
                    { "Circolo delle quinte", "i iv bVII bIII bVI ii\xC3\xB8" "7 V7 i" } };
        p.degreeMul = { 1.1, 1.3, 0.8, 1.3, 1.6, 1.1, 0.9 };
        p.degreeMulMinor = { 1.1, 1.2, 1.0, 1.3, 1.4, 1.1, 0.8 };
        p.colourMul = 0.7;
        p.majorExtras = { { "#iv\xC2\xB0" "7", 0x0A, 3, "♯iv°7: diminuita che porta alla dominante" },
                          { "bII", 0x08, 2, "Napoletana ♭II: pathos classico" } };
        p.minorExtras = { { "vii\xC2\xB0" "7", 0x09, 4, "vii°7 diminuita: tensione verso la tonica" },
                          { "I", 0x10, 2, "Terza piccarda: chiusura in maggiore" } };
        p.adventure = 0.3;
        p.rhythm = rhythm ({ 1, 0, .45, 0, .85, 0, .45, 0, .9, 0, .45, 0, .85, 0, .45, 0 });
        p.legato = 0.9;
        p.motif = 0.65;
        p.energy = 0.45; p.movement = 0.35; p.tension = 0.3;
        p.contour = Contour::Arch;
        p.ensemble = Ensemble::StringQuartet;
    }
    {   // ---------------------------------------------------------------- Latin / Bossa
        auto& p = out[size_t (Genre::Latin)];
        base (p, "Latin / Bossa nova", "Settime e none, II7 lidio, cadenza andalusa, ritmo sincopato.");
        p.major = { { "Ipanema", "Imaj7 II7 ii7 bII7" }, { "Bossa", "Imaj7 vi7 ii7 V7" }, { "Samba", "ii7 V7 Imaj7 VI7" },
                    { "Saudade", "Imaj7 iv7 iii7 VI7" } };
        p.minor = { { "Andalusa", "i bVII bVI V" }, { "Bossa minore", "i7 iv7 V7b9 i7" }, { "Tango", "i iv V7 i" },
                    { "Salsa", "i7 iv7 bVII7 bIIImaj7" }, { "Flamenco", "iv bIII bII I" } };
        p.degreeMul = { 1.0, 1.5, 1.1, 1.1, 1.4, 1.2, 0.5 };
        p.degreeMulMinor = { 1.0, 1.2, 1.1, 1.3, 1.2, 1.2, 1.2 };
        p.colourMul = 1.2;
        p.tritoneSubs = true;
        p.majorExtras = { { "bVII7", 0x08, 3, "♭VII7: colore bossa" } };
        p.minorExtras = { { "bII", 0x50, 4, "♭II frigio/andaluso: sapore flamenco" } };
        p.colour = 2;
        p.chordStyle = ChordStyle::Offbeat;
        p.bass = BassStyle::RootFifth;
        p.adventure = 0.45;
        p.rhythm = rhythm ({ 1, 0, 0, .6, 0, 0, .7, 0, .3, 0, .7, 0, .4, .5, 0, .2 });
        p.legato = 0.85;
        p.motif = 0.6;
        p.energy = 0.5; p.movement = 0.4; p.tension = 0.45;
        p.contour = Contour::Wave;
        p.poly = Polyharmony::Extensions;
    }
    {   // ---------------------------------------------------------------- Ambient
        auto& p = out[size_t (Genre::Ambient)];
        base (p, "Ambient / Atmosferico", "Accordi lunghi e sospesi, IV lidio, poche note lunghe, archi e poliaccordi.");
        p.major = { { "Galleggiante", "I IV I IV" }, { "Lidio", "I II I II" }, { "Alba", "IVmaj7 Imaj7 vi7 V" },
                    { "Sospensione", "Isus2 IV vi IV" } };
        p.minor = { { "Nebbia", "i bVI i bVI" }, { "Profondità", "i bIII bVI bVII" }, { "Dorico", "i IV i IV" },
                    { "Frigio", "i bII i bII" } };
        p.degreeMul = { 1.1, 0.8, 1.1, 1.5, 0.7, 1.4, 0.3 };
        p.degreeMulMinor = { 1.1, 0.4, 1.3, 1.2, 0.6, 1.5, 1.2 };
        p.majorExtras = { { "II", 0x01, 3, "II maggiore lidio: sospeso, aperto" } };
        p.minorExtras = { { "IV", 0x01, 3, "IV dorico: luce soffusa" }, { "bII", 0x01, 2, "♭II frigio: nebbia scura" } };
        p.colour = 2;
        p.stepsPerChord = 32;
        p.adventure = 0.35;
        p.rhythm = rhythm ({ 1, 0, 0, 0, 0, 0, .2, 0, .5, 0, 0, 0, .15, 0, 0, 0 });
        p.legato = 1.0;
        p.motif = 0.5;
        p.energy = 0.2; p.movement = 0.45; p.tension = 0.45;
        p.contour = Contour::Wave;
        p.reg = 3;
        p.ensemble = Ensemble::StringOrchestra;
        p.poly = Polyharmony::UpperTriad;
    }
    return out;
}

// ------------------------------------------------------------ voicing di una triade (o accordo) in un registro
std::vector<int> voiceInRange (const std::vector<int>& pcs, const std::vector<int>* prev, int low, int high)
{
    const int n = int (pcs.size());
    std::vector<int> best;
    double bestCost = 1e18;
    for (int r = 0; r < n; ++r)
        for (int start = low; start < low + 12; ++start)
        {
            if (mod12 (start) != pcs[size_t (r)]) continue;
            std::vector<int> v { start };
            int p = start;
            for (int i = 1; i < n; ++i)
            {
                ++p;
                while (mod12 (p) != pcs[size_t ((r + i) % n)]) ++p;
                v.push_back (p);
            }
            if (v.back() > high) continue;
            double cost = 0;
            if (prev != nullptr && prev->size() == v.size())
                for (size_t i = 0; i < v.size(); ++i) cost += std::abs (v[i] - (*prev)[i]);
            else
                cost = std::abs ((v.front() + v.back()) / 2.0 - (low + high) / 2.0);
            if (cost < bestCost) { bestCost = cost; best = v; }
        }
    if (best.empty())
        for (int pc : pcs) best.push_back (low + mod12 (pc - low));
    return best;
}

struct Hit { int step, len; };

std::vector<Hit> styleHits (ChordStyle s, int spc)
{
    std::vector<Hit> h;
    switch (s)
    {
        case ChordStyle::Block:      h.push_back ({ 0, spc }); break;
        case ChordStyle::Arp16:      for (int i = 0; i < spc; ++i) h.push_back ({ i, 1 }); break;
        case ChordStyle::ArpUpDown8:
        case ChordStyle::Pulse8:     for (int i = 0; i < spc; i += 2) h.push_back ({ i, 2 }); break;
        case ChordStyle::Offbeat:    for (int i = 2; i < spc; i += 4) h.push_back ({ i, 2 }); break;
    }
    return h;
}

enum class Role { Bass, BassLow, Tenor, Alto, Soprano, Horns, LowBrass, Timpani, Winds, Upper };

struct PartSpec
{
    const char* name;
    int program;
    Role role;
    bool ostinato;
    int velocity;
};
} // namespace

// ============================================================ nomi
const char* ensembleName (Ensemble e)
{
    switch (e)
    {
        case Ensemble::Piano:           return "Pianoforte / Pad";
        case Ensemble::StringQuartet:   return "Quartetto d'archi";
        case Ensemble::StringOrchestra: return "Orchestra d'archi";
        case Ensemble::FullOrchestra:   return "Orchestra sinfonica";
        case Ensemble::Choir:           return "Coro SATB";
        case Ensemble::Brass:           return "Ensemble di ottoni";
    }
    return "";
}

Ensemble ensembleFromIndex (int i) { return Ensemble (std::clamp (i, 0, kNumEnsembles - 1)); }

const char* polyharmonyName (Polyharmony p)
{
    switch (p)
    {
        case Polyharmony::Off:        return "Armonia semplice";
        case Polyharmony::Extensions: return "Estensioni (9ª, 11ª, 13ª)";
        case Polyharmony::UpperTriad: return "Poliaccordi (triade superiore)";
    }
    return "";
}

Polyharmony polyharmonyFromIndex (int i) { return Polyharmony (std::clamp (i, 0, kNumPolyharmony - 1)); }

const GenreProfile& genreProfile (Genre g)
{
    static const std::vector<GenreProfile> profiles = buildProfiles();
    return profiles[size_t (std::clamp (int (g), 0, kNumGenres - 1))];
}

const char* genreName (Genre g) { return genreProfile (g).name; }

// ============================================================ preset
std::vector<std::string> presetNames (Genre g, const Key& k)
{
    std::vector<std::string> out;
    if (g == Genre::Universal)
    {
        for (const auto& p : presetsFor (k)) out.push_back (p.name);
        return out;
    }
    const auto& gp = genreProfile (g);
    for (const auto& p : (isMinorFamily (k.scale) ? gp.minor : gp.major))
        out.push_back (prettyRomans (p.romans) + "  (" + p.name + ")");
    return out;
}

std::vector<Chord> presetChords (Genre g, const Key& k, int index, int colour)
{
    colour = std::clamp (colour, 0, kNumColours - 1);
    std::vector<Chord> out;
    if (g == Genre::Universal)
    {
        out = chordsFromPreset (k, index, colour >= 1);
        if (colour == 2) for (auto& c : out) c = adaptColour (c, k, 2);
        return out;
    }
    const auto& gp = genreProfile (g);
    const auto& list = isMinorFamily (k.scale) ? gp.minor : gp.major;
    if (list.empty()) return out;
    const auto& p = list[size_t (std::clamp (index, 0, int (list.size()) - 1))];
    for (const auto& t : tokens (p.romans))
    {
        Chord c;
        if (parseRoman (t, k, c) && int (out.size()) < kMaxChords) out.push_back (adaptColour (c, k, colour));
    }
    return out;
}

// ============================================================ poliarmonia
UpperTriad upperStructure (const Chord& c, const Key& k)
{
    struct Opt { int offset; Quality q; };
    std::vector<Opt> opts;
    const Quality q = c.quality;
    const bool dominant = isDominantQuality (q);

    if (dominant)
        opts = { { 2, Quality::Maj }, { 9, Quality::Maj }, { 8, Quality::Maj }, { 3, Quality::Min } };
    else switch (q)
    {
        case Quality::Maj: case Quality::Maj7: case Quality::Maj9: case Quality::Add9: case Quality::Six:
            opts = { { 2, Quality::Maj }, { 7, Quality::Maj }, { 9, Quality::Min }, { 4, Quality::Min } }; break;
        case Quality::Min: case Quality::Min7: case Quality::Min9: case Quality::MinAdd9: case Quality::Min6:
        case Quality::MinMaj7:
            opts = { { 10, Quality::Maj }, { 2, Quality::Min }, { 5, Quality::Maj }, { 7, Quality::Min } }; break;
        case Quality::Dim: case Quality::HalfDim7: case Quality::Dim7:
            opts = { { 8, Quality::Maj }, { 3, Quality::Min } }; break;
        case Quality::Sus2: case Quality::Sus4:
            opts = { { 10, Quality::Maj }, { 7, Quality::Min } }; break;
        default: break;
    }

    const auto base = chordPitchClasses (c);
    for (const auto& o : opts)
    {
        const Chord t { mod12 (c.root + o.offset), o.q };
        const auto pcs = chordPitchClasses (t);
        bool allIn = true;
        int fresh = 0;
        for (int pc : pcs)
        {
            allIn = allIn && inParent (pc, k);
            if (std::find (base.begin(), base.end(), pc) == base.end()) ++fresh;
        }
        // il ♭9 contro la fondamentale stona su accordi non dominanti
        bool clash = false;
        if (! dominant)
            for (int pc : pcs) clash = clash || pc == mod12 (c.root + 1);
        if (allIn && fresh >= 1 && ! clash) return { true, t };
    }
    return {};
}

std::string polychordName (const Chord& base, const UpperTriad& u, const Key& k)
{
    if (! u.valid) return chordName (base, k);
    return chordName (u.triad, k) + "/" + chordName (base, k);
}

// ============================================================ SATB
std::vector<std::array<int, 4>> voiceSATB (const std::vector<Chord>& chords, const std::array<VoiceRange, 4>& rg)
{
    std::vector<std::array<int, 4>> out;
    std::array<int, 4> prev {};
    bool have = false;

    for (const auto& c : chords)
    {
        const auto pcs = chordPitchClasses (c);
        const int n = int (pcs.size());
        std::vector<int> required;
        if (n >= 2) required.push_back (pcs[1]);
        if (n >= 4) required.push_back (pcs[3]);
        if (n >= 5) required.push_back (pcs[4]);

        auto isTone = [&] (int p) { return std::find (pcs.begin(), pcs.end(), mod12 (p)) != pcs.end(); };
        std::array<std::vector<int>, 4> cand;
        for (int p = rg[0].low; p <= rg[0].high; ++p) if (mod12 (p) == c.root) cand[0].push_back (p);
        for (int v = 1; v < 4; ++v)
            for (int p = rg[size_t (v)].low; p <= rg[size_t (v)].high; ++p) if (isTone (p)) cand[size_t (v)].push_back (p);

        std::array<int, 4> best {};
        double bestCost = 1e18;
        for (int b : cand[0])
            for (int t : cand[1])
            {
                if (t <= b) continue;
                for (int a : cand[2])
                {
                    if (a <= t || a - t > 12) continue;
                    for (int s : cand[3])
                    {
                        if (s <= a || s - a > 12) continue;
                        const std::array<int, 4> v { b, t, a, s };
                        double cost = 0;

                        // note obbligatorie (terza, settima, nona) nelle voci superiori
                        bool missing = false;
                        for (int r : required)
                            if (mod12 (t) != r && mod12 (a) != r && mod12 (s) != r) missing = true;
                        if (missing) continue;
                        if (n == 3 && mod12 (t) != pcs[2] && mod12 (a) != pcs[2] && mod12 (s) != pcs[2]) cost += 4;   // manca la quinta
                        int thirds = 0, sevenths = 0;
                        for (int x : { t, a, s })
                        {
                            if (mod12 (x) == pcs[1]) ++thirds;
                            if (n >= 4 && mod12 (x) == pcs[3]) ++sevenths;
                        }
                        if (thirds > 1) cost += 6;
                        if (sevenths > 1) cost += 20;
                        if (t - b > 19) cost += 3;

                        if (have)
                        {
                            cost += 0.5 * std::abs (b - prev[0]);
                            for (size_t i = 1; i < 4; ++i)
                            {
                                const int mv = std::abs (v[i] - prev[i]);
                                cost += mv;
                                if (mv > 7) cost += 5;
                            }
                            // quinte e ottave parallele
                            for (size_t i = 0; i < 4; ++i)
                                for (size_t j = i + 1; j < 4; ++j)
                                {
                                    const int mi = v[i] - prev[i], mj = v[j] - prev[j];
                                    if (mi == 0 || mj == 0 || (mi > 0) != (mj > 0)) continue;
                                    const int before = mod12 (prev[j] - prev[i]), after = mod12 (v[j] - v[i]);
                                    if (before == after && (after == 0 || after == 7)) cost += 25;
                                }
                        }
                        else
                        {
                            cost += std::abs (s - (rg[3].low + rg[3].high) / 2.0 - 2.0);
                            cost += std::abs (a - (rg[2].low + rg[2].high) / 2.0);
                            cost += 0.5 * std::abs (t - (rg[1].low + rg[1].high) / 2.0);
                        }
                        if (cost < bestCost) { bestCost = cost; best = v; }
                    }
                }
            }

        if (bestCost >= 1e17)
        {
            // ripiego: voicing chiuso standard sopra il basso
            const auto v = voiceChord (c, nullptr, (rg[2].low + rg[2].high) / 2);
            best = { rg[0].low + mod12 (c.root - rg[0].low), v[0], v[std::min<size_t> (1, v.size() - 1)], v.back() };
        }
        out.push_back (best);
        prev = best;
        have = true;
    }
    return out;
}

// ============================================================ arrangiamento
std::vector<Note> mergeParts (const std::vector<Part>& parts)
{
    std::vector<Note> out;
    for (const auto& p : parts) out.insert (out.end(), p.notes.begin(), p.notes.end());
    std::stable_sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });
    return out;
}

std::vector<Part> arrange (const std::vector<Chord>& input, const ArrangeOptions& o)
{
    std::vector<Part> parts;
    if (input.empty()) return parts;

    std::vector<Chord> chords = input;
    if (o.poly == Polyharmony::Extensions)
        for (auto& c : chords) c = adaptColour (c, o.key, 2);

    const int spc = std::max (1, o.stepsPerChord);
    const int slot = spc * kStepTicks;

    // triadi superiori (poliaccordi)
    std::vector<UpperTriad> uppers;
    if (o.poly == Polyharmony::UpperTriad)
        for (const auto& c : chords) uppers.push_back (upperStructure (c, o.key));

    auto upperPart = [&] (const char* name, int program, int low, int high, int velocity)
    {
        Part p { name, int (parts.size()), program, {} };
        std::vector<int> prev;
        for (size_t i = 0; i < chords.size(); ++i)
        {
            if (! uppers[i].valid) continue;
            const auto v = voiceInRange (chordPitchClasses (uppers[i].triad), prev.empty() ? nullptr : &prev, low, high);
            for (int x : v) p.notes.push_back ({ x, int (i) * slot, slot, velocity });
            prev = v;
        }
        if (! p.notes.empty()) parts.push_back (p);
    };

    if (o.ensemble == Ensemble::Piano)
    {
        RenderOptions r;
        r.style = o.style;
        r.stepsPerChord = spc;
        r.bass = false;
        r.key = o.key;
        parts.push_back ({ "Accordi", 0, 0, renderChords (chords, r) });
        if (o.bass)
        {
            r.bass = true;
            r.bassStyle = o.bassStyle;
            parts.push_back ({ "Basso", 1, 33, renderBass (chords, r) });
        }
        if (o.poly == Polyharmony::UpperTriad) upperPart ("Triade superiore", 48, 72, 91, 70);
    }
    else
    {
        std::array<VoiceRange, 4> rg { { { 36, 60 }, { 48, 72 }, { 55, 81 }, { 62, 88 } } };
        std::vector<PartSpec> specs;
        const char* upperName = "Triade superiore";
        int upperProgram = 48, upperLow = 74, upperHigh = 96;

        switch (o.ensemble)
        {
            case Ensemble::StringQuartet:
                rg = { { { 36, 60 }, { 48, 72 }, { 55, 81 }, { 62, 88 } } };
                specs = { { "Violoncello", 42, Role::Bass, false, 80 }, { "Viola", 41, Role::Tenor, true, 72 },
                          { "Violino II", 40, Role::Alto, true, 74 }, { "Violino I", 40, Role::Soprano, false, 88 } };
                upperName = "Archi divisi (triade superiore)"; upperProgram = 49;
                break;
            case Ensemble::StringOrchestra:
                rg = { { { 36, 60 }, { 48, 72 }, { 55, 81 }, { 62, 88 } } };
                specs = { { "Contrabbassi", 43, Role::BassLow, false, 78 }, { "Violoncelli", 42, Role::Bass, false, 80 },
                          { "Viole", 41, Role::Tenor, true, 72 }, { "Violini II", 40, Role::Alto, true, 74 },
                          { "Violini I", 40, Role::Soprano, false, 88 } };
                upperName = "Violini I divisi (triade superiore)";
                break;
            case Ensemble::FullOrchestra:
                rg = { { { 36, 60 }, { 48, 72 }, { 55, 81 }, { 62, 88 } } };
                specs = { { "Contrabbassi", 43, Role::BassLow, false, 78 }, { "Violoncelli", 42, Role::Bass, false, 80 },
                          { "Viole", 41, Role::Tenor, true, 72 }, { "Violini II", 40, Role::Alto, true, 74 },
                          { "Violini I", 40, Role::Soprano, false, 88 }, { "Corni", 60, Role::Horns, false, 70 },
                          { "Ottoni gravi", 57, Role::LowBrass, false, 82 }, { "Timpani", 47, Role::Timpani, false, 92 } };
                if (o.poly != Polyharmony::UpperTriad) specs.push_back ({ "Legni", 73, Role::Winds, false, 72 });
                upperName = "Legni (triade superiore)"; upperProgram = 73; upperLow = 72; upperHigh = 96;
                break;
            case Ensemble::Choir:
                rg = { { { 40, 62 }, { 48, 67 }, { 55, 74 }, { 60, 81 } } };
                specs = { { "Bassi", 52, Role::Bass, false, 78 }, { "Tenori", 52, Role::Tenor, false, 74 },
                          { "Contralti", 52, Role::Alto, false, 74 }, { "Soprani", 52, Role::Soprano, false, 84 } };
                upperName = "Soprani divisi (triade superiore)"; upperProgram = 52; upperLow = 70; upperHigh = 88;
                break;
            case Ensemble::Brass:
                rg = { { { 34, 58 }, { 46, 67 }, { 52, 72 }, { 58, 80 } } };
                specs = { { "Tuba", 58, Role::BassLow, false, 80 }, { "Tromboni", 57, Role::Bass, false, 80 },
                          { "Corni", 60, Role::Alto, true, 74 }, { "Trombe", 56, Role::Soprano, true, 86 } };
                upperName = "Trombe divise (triade superiore)"; upperProgram = 56; upperLow = 70; upperHigh = 86;
                break;
            case Ensemble::Piano: break;
        }

        const auto satb = voiceSATB (chords, rg);
        const auto hits = styleHits (o.style, spc);
        const bool rhythmic = o.style != ChordStyle::Block && o.ensemble != Ensemble::Choir;

        for (const auto& sp : specs)
        {
            if (sp.role == Role::BassLow && ! o.bass) continue;
            Part p { sp.name, int (parts.size()), sp.program, {} };

            if (sp.role == Role::BassLow && o.bassStyle != BassStyle::Sustained)
            {
                RenderOptions r;
                r.stepsPerChord = spc;
                r.bassStyle = o.bassStyle;
                r.key = o.key;
                r.velocity = sp.velocity - 4;
                p.notes = renderBass (chords, r);
                parts.push_back (p);
                continue;
            }

            for (size_t i = 0; i < chords.size(); ++i)
            {
                const auto& v = satb[i];
                const int t0 = int (i) * slot;
                auto sustain = [&] (int pitch, int vel) { p.notes.push_back ({ pitch, t0, slot, std::clamp (vel, 1, 127) }); };

                switch (sp.role)
                {
                    case Role::BassLow: sustain (v[0] - 12 >= 28 ? v[0] - 12 : v[0], sp.velocity); break;
                    case Role::Bass:    sustain (v[0], sp.velocity); break;
                    case Role::Horns:   sustain (v[1], sp.velocity); sustain (v[2], sp.velocity); break;
                    case Role::Winds:   if (v[3] + 12 <= 96) sustain (v[3] + 12, sp.velocity); break;
                    case Role::LowBrass:
                        p.notes.push_back ({ v[0], t0, std::max (1, slot / 2), sp.velocity });
                        if (v[0] + 7 <= 62) p.notes.push_back ({ v[0] + 7, t0, std::max (1, slot / 2), sp.velocity - 6 });
                        break;
                    case Role::Timpani:
                    {
                        int tp = 41 + mod12 (chords[i].root - 41);
                        if (tp > 53) tp -= 12;
                        p.notes.push_back ({ tp, t0, 2 * kStepTicks, sp.velocity });
                        break;
                    }
                    case Role::Tenor: case Role::Alto: case Role::Soprano:
                    {
                        const size_t vi = sp.role == Role::Tenor ? 1 : (sp.role == Role::Alto ? 2 : 3);
                        const int own = v[vi];
                        if (sp.ostinato && rhythmic)
                        {
                            // ostinato: la voce ripete (o spezza con la voce sopra) il ritmo dello stile
                            const int other = vi < 3 ? v[vi + 1] : own + 12;
                            const bool broken = o.style == ChordStyle::Arp16 || o.style == ChordStyle::ArpUpDown8;
                            for (size_t h = 0; h < hits.size(); ++h)
                            {
                                const int pitch = (broken && h % 2 == 1) ? other : own;
                                const int vel = sp.velocity - (hits[h].step % 4 == 0 ? 0 : 10);
                                p.notes.push_back ({ pitch, t0 + hits[h].step * kStepTicks,
                                                     std::max (1, int (hits[h].len * kStepTicks * 0.9)), vel });
                            }
                        }
                        else
                        {
                            sustain (own, sp.velocity);
                            // archi: i Violini I raddoppiano all'ottava se non c'è il poliaccordo sopra
                            if (sp.role == Role::Soprano && o.poly != Polyharmony::UpperTriad
                                && (o.ensemble == Ensemble::StringOrchestra || o.ensemble == Ensemble::FullOrchestra)
                                && own + 12 <= 96)
                                sustain (own + 12, sp.velocity - 8);
                        }
                        break;
                    }
                    case Role::Upper: break;
                }
            }
            if (! p.notes.empty()) parts.push_back (p);
        }

        if (o.poly == Polyharmony::UpperTriad) upperPart (upperName, upperProgram, upperLow, upperHigh, 70);
    }

    for (size_t i = 0; i < parts.size(); ++i)
    {
        int ch = int (i);
        if (ch >= 9) ++ch;                // salta il canale batteria
        parts[i].channel = std::min (ch, 15);
        if (o.swing > 0) applySwing (parts[i].notes, o.swing);
        std::stable_sort (parts[i].notes.begin(), parts[i].notes.end(),
                          [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });
    }
    return parts;
}

} // namespace cm
