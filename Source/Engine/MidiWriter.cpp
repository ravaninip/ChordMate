#include "MidiWriter.h"

#include <algorithm>
#include <cmath>

namespace cm
{
namespace
{
void writeVlq (std::vector<uint8_t>& v, uint32_t value)
{
    uint8_t buf[5];
    int n = 0;
    buf[n++] = uint8_t (value & 0x7F);
    while ((value >>= 7) > 0)
        buf[n++] = uint8_t (0x80 | (value & 0x7F));
    while (n--) v.push_back (buf[n]);
}

void write32 (std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back (uint8_t (x >> 24)); v.push_back (uint8_t (x >> 16));
    v.push_back (uint8_t (x >> 8));  v.push_back (uint8_t (x));
}

void write16 (std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back (uint8_t (x >> 8)); v.push_back (uint8_t (x));
}

struct Ev { int tick; int order; uint8_t status, d1, d2; };
}

std::vector<uint8_t> buildMidiFile (const std::vector<Note>& notes, double bpm, int lengthTicks,
                                    const std::string& trackName)
{
    std::vector<Ev> evs;
    for (const auto& n : notes)
    {
        const uint8_t p = uint8_t (std::clamp (n.pitch, 0, 127));
        const uint8_t v = uint8_t (std::clamp (n.velocity, 1, 127));
        const int start = std::max (0, n.startTick);
        const int end = start + std::max (1, n.lengthTick);
        evs.push_back ({ start, 1, 0x90, p, v });
        evs.push_back ({ end,   0, 0x80, p, 0 });
    }
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b)
                      { return a.tick != b.tick ? a.tick < b.tick : a.order < b.order; });

    std::vector<uint8_t> trk;

    if (! trackName.empty())
    {
        trk.push_back (0x00); trk.push_back (0xFF); trk.push_back (0x03);
        writeVlq (trk, uint32_t (trackName.size()));
        trk.insert (trk.end(), trackName.begin(), trackName.end());
    }

    const uint32_t tempo = uint32_t (std::lround (60000000.0 / std::clamp (bpm, 20.0, 400.0)));
    trk.push_back (0x00); trk.push_back (0xFF); trk.push_back (0x51); trk.push_back (0x03);
    trk.push_back (uint8_t (tempo >> 16)); trk.push_back (uint8_t (tempo >> 8)); trk.push_back (uint8_t (tempo));

    trk.push_back (0x00); trk.push_back (0xFF); trk.push_back (0x58); trk.push_back (0x04);
    trk.push_back (4); trk.push_back (2); trk.push_back (24); trk.push_back (8);

    int cursor = 0;
    for (const auto& e : evs)
    {
        writeVlq (trk, uint32_t (e.tick - cursor));
        trk.push_back (e.status); trk.push_back (e.d1);
        if (e.status != 0xC0 && e.status != 0xD0) trk.push_back (e.d2);
        cursor = e.tick;
    }

    const int endTick = std::max (lengthTicks, cursor);
    writeVlq (trk, uint32_t (endTick - cursor));
    trk.push_back (0xFF); trk.push_back (0x2F); trk.push_back (0x00);

    std::vector<uint8_t> file;
    file.insert (file.end(), { 'M', 'T', 'h', 'd' });
    write32 (file, 6);
    write16 (file, 0);          // formato 0
    write16 (file, 1);          // una traccia
    write16 (file, uint16_t (kPPQ));
    file.insert (file.end(), { 'M', 'T', 'r', 'k' });
    write32 (file, uint32_t (trk.size()));
    file.insert (file.end(), trk.begin(), trk.end());
    return file;
}

} // namespace cm
