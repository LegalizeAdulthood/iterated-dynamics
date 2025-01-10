// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// This decoding functions handle big endian binary blobs.
// Iterated Dynamics always stores binary blobs, e.g. FRACTINT
// extension blocks in GIF files, etc., in little endian
// format.

struct EvolutionInfo;
struct FractalInfo;
struct OrbitsInfo;

void decode_evolver_info_big_endian(EvolutionInfo *info, int dir);
void decode_fractal_info_big_endian(FractalInfo *info, int dir);
void decode_orbits_info_big_endian(OrbitsInfo *info, int dir);

#if ID_BIG_ENDIAN
inline void decode_evolver_info(EvolutionInfo *info, int dir)
{
    decode_evolver_info_big_endian(info, dir);
}
inline void decode_fractal_info(FractalInfo *info, int dir)
{
    decode_fractal_info_big_endian(info, dir);
}
inline void decode_orbits_info(OrbitsInfo *info, int dir)
{
    decode_orbits_info_big_endian(info, dir);
}
#else
inline void decode_evolver_info(EvolutionInfo * /*info*/, int /*dir*/)
{
}
inline void decode_fractal_info(FractalInfo * /*info*/, int /*dir*/)
{
}
inline void decode_orbits_info(OrbitsInfo * /*info*/, int /*dir*/)
{
}
#endif
