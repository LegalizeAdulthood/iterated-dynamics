// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port_config.h>

namespace id::ui
{
struct EvolutionInfo;
}

namespace id::io
{

struct FractalInfo;
struct OrbitsInfo;

enum class DecodeDirection
{
    TO_FILE = 0,
    FROM_FILE = 1,
};

// This decoding functions handle big endian binary blobs.
// Iterated Dynamics always stores binary blobs, e.g. FRACTINT
// extension blocks in GIF files, etc., in little endian
// format.

#if ID_BIG_ENDIAN
void decode_evolver_info_big_endian(ui::EvolutionInfo *info, DecodeDirection dir);
void decode_fractal_info_big_endian(FractalInfo *info, DecodeDirection dir);
void decode_orbits_info_big_endian(OrbitsInfo *info, DecodeDirection dir);

inline void decode_evolver_info(EvolutionInfo *info, DecodeDirection dir)
{
    decode_evolver_info_big_endian(info, dir);
}
inline void decode_fractal_info(FractalInfo *info, DecodeDirection dir)
{
    decode_fractal_info_big_endian(info, dir);
}
inline void decode_orbits_info(OrbitsInfo *info, DecodeDirection dir)
{
    decode_orbits_info_big_endian(info, dir);
}
#else
inline void decode_evolver_info(ui::EvolutionInfo * /*info*/, DecodeDirection /*dir*/)
{
}
inline void decode_fractal_info(FractalInfo * /*info*/, DecodeDirection /*dir*/)
{
}
inline void decode_orbits_info(OrbitsInfo * /*info*/, DecodeDirection /*dir*/)
{
}
#endif

}
