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

// This decoding functions handle big endian binary blobs.
// Iterated Dynamics always stores binary blobs, e.g. FRACTINT
// extension blocks in GIF files, etc., in little endian
// format.

#if ID_BIG_ENDIAN
void encode_fractal_info_big_endian(FractalInfo &info);
void decode_fractal_info_big_endian(FractalInfo &info);
void encode_evolver_info_big_endian(ui::EvolutionInfo &info);
void decode_evolver_info_big_endian(ui::EvolutionInfo &info);
void encode_orbits_info_big_endian(OrbitsInfo &info);
void decode_orbits_info_big_endian(OrbitsInfo &info);

inline void decode_evolver_info(ui::EvolutionInfo &info)
{
    decode_evolver_info_big_endian(info);
}

inline void decode_fractal_info(FractalInfo &info)
{
    decode_fractal_info_big_endian(info);
}

inline void decode_orbits_info(OrbitsInfo &info)
{
    decode_orbits_info_big_endian(info);
}

inline void encode_evolver_info(ui::EvolutionInfo &info)
{
    encode_evolver_info_big_endian(info);
}

inline void encode_fractal_info(FractalInfo &info)
{
    encode_fractal_info_big_endian(info);
}

inline void encode_orbits_info(OrbitsInfo &info)
{
    encode_orbits_info_big_endian(info);
}

#else
inline void decode_evolver_info(ui::EvolutionInfo &info)
{
}

inline void decode_fractal_info(FractalInfo &info)
{
}

inline void decode_orbits_info(OrbitsInfo &info)
{
}

inline void encode_evolver_info(ui::EvolutionInfo &info)
{
}

inline void encode_fractal_info(FractalInfo &info)
{
}

inline void encode_orbits_info(OrbitsInfo &info)
{
}

#endif

} // namespace id::io
