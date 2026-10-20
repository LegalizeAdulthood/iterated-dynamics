// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/sound.h"

namespace id::engine
{

int g_base_hertz{};
int g_fm_attack{};
int g_fm_decay{};
int g_fm_release{};
int g_fm_sustain{};
int g_fm_volume{};
int g_fm_wave_type{};
int g_hi_attenuation{};
int g_polyphony{};
int g_scale_map[12]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
int g_sound_flag{};

bool sound_buzzer_enabled()
{
    return (g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_BEEP;
}

} // namespace id::engine
