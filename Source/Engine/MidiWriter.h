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
