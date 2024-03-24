#pragma once

extern int                   g_fm_attack;
extern int                   g_fm_decay;
extern int                   g_fm_release;
extern int                   g_fm_sustain;
extern int                   g_fm_wavetype;
extern int                   g_fm_volume;            // volume of OPL-3 soundcard output
extern int                   g_hi_attenuation;
extern int                   g_polyphony;
extern bool                  g_tab_or_help;

bool snd_open();
void w_snd(int);
void snd_time_write();
void close_snd();
