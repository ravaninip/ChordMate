#pragma once
#include "Theory.h"
#include <cstdint>
#include <string>
#include <vector>

namespace cm
{
// Standard MIDI File formato 0, una traccia, 480 PPQ, tempo e metrica 4/4 inclusi.
// lengthTicks fissa la posizione dell'End-of-Track (lunghezza della clip).
std::vector<uint8_t> buildMidiFile (const std::vector<Note>& notes, double bpm, int lengthTicks,
                                    const std::string& trackName);
}

namespace cm
{
// Standard MIDI File formato 1: traccia 0 con tempo e metrica, poi una traccia per parte
// (nome, canale e program change General MIDI). Trascinato in Live crea una traccia per parte.
struct MidiTrack
{
    std::string name;
    int channel = 0;          // 0-15
    int program = -1;         // -1 = nessun program change
    std::vector<Note> notes;
};
std::vector<uint8_t> buildMidiFileMulti (const std::vector<MidiTrack>& tracks, double bpm, int lengthTicks);
}
