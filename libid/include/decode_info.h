#pragma once

// This decoding functions handle big endian binary blobs.
// Iterated Dynamics always stores binary blobs, e.g. FRACTINT
// extension blocks in GIF files, etc., in little endian
// format.

struct EVOLUTION_INFO;
struct FRACTAL_INFO;
struct ORBITS_INFO;

void decode_evolver_info_big_endian(EVOLUTION_INFO *info, int dir);
void decode_fractal_info_big_endian(FRACTAL_INFO *info, int dir);
void decode_orbits_info_big_endian(ORBITS_INFO *info, int dir);

#if ID_BIG_ENDIAN
inline void decode_evolver_info(EVOLUTION_INFO *info, int dir)
{
    decode_evolver_info_big_endian(info, dir);
}
inline void decode_fractal_info(FRACTAL_INFO *info, int dir)
{
    decode_fractal_info_big_endian(info, dir);
}
inline void decode_orbits_info(ORBITS_INFO *info, int dir)
{
    decode_orbits_info_big_endian(info, dir);
}
#else
inline void decode_evolver_info(EVOLUTION_INFO * /*info*/, int /*dir*/)
{
}
inline void decode_fractal_info(FRACTAL_INFO * /*info*/, int /*dir*/)
{
}
inline void decode_orbits_info(ORBITS_INFO * /*info*/, int /*dir*/)
{
}
#endif
