// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

enum SoundFlags
{
    SOUNDFLAG_OFF        = 0,
    SOUNDFLAG_BEEP       = 1,
    SOUNDFLAG_X          = 2,
    SOUNDFLAG_Y          = 3,
    SOUNDFLAG_Z          = 4,
    SOUNDFLAG_ORBIT_MASK = 0x07,
    SOUNDFLAG_SPEAKER    = 8,
    SOUNDFLAG_OPL3_FM    = 16,
    SOUNDFLAG_MIDI       = 32,
    SOUNDFLAG_QUANTIZED  = 64,
    SOUNDFLAG_MASK       = 0x7F
};

extern int g_base_hertz;     // sound=x/y/x hertz value
extern int g_fm_attack;      //
extern int g_fm_decay;       //
extern int g_fm_release;     //
extern int g_fm_sustain;     //
extern int g_fm_wave_type;   //
extern int g_fm_volume;      // volume of OPL-3 sound output
extern int g_hi_attenuation; //
extern int g_polyphony;      //
extern int g_scale_map[12];  // array for mapping notes to a (user defined) scale
extern int g_sound_flag;     // sound control flags

bool sound_open();
void write_sound(int tone);
void sound_time_write();
void close_sound();

} // namespace id::engine
