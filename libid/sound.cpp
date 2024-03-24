#include "sound.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "stop_msg.h"
#include "update_save_name.h"
#include "wait_until.h"

#include <cstdio>
#include <ctime>

int g_fm_attack = 0;
int g_fm_decay = 0;
int g_fm_release = 0;
int g_fm_sustain = 0;
int g_fm_volume = 0;
int g_fm_wavetype = 0;
int g_hi_attenuation = 0;
int g_polyphony = 0;
// TODO: this doesn't appear to be used outside this file?
bool g_tab_or_help{}; // kludge for sound and tab or help key press

static std::FILE *s_snd_fp{};

// open sound file
bool snd_open()
{
    static char soundname[] = {"sound001.txt"};
    if ((g_orbit_save_flags & osf_midi) != 0 && s_snd_fp == nullptr)
    {
        s_snd_fp = std::fopen(soundname, "w");
        if (s_snd_fp == nullptr)
        {
            stopmsg(STOPMSG_NONE, "Can't open sound*.txt");
        }
        else
        {
            update_save_name(soundname);
        }
    }
    return s_snd_fp != nullptr;
}

/* This routine plays a tone in the speaker and optionally writes a file
   if the orbitsave variable is turned on */
void w_snd(int tone)
{
    if ((g_orbit_save_flags & osf_midi) != 0)
    {
        // cppcheck-suppress leakNoVarFunctionCall
        if (snd_open())
        {
            std::fprintf(s_snd_fp, "%-d\n", tone);
        }
    }
    g_tab_or_help = false;
    if (!driver_key_pressed())
    {
        // driver_key_pressed calls driver_sound_off() if TAB or F1 pressed
        // must not then call driver_sound_off(), else indexes out of synch
        //   if (20 < tone && tone < 15000)  better limits?
        //   if (10 < tone && tone < 5000)  better limits?
        if (driver_sound_on(tone))
        {
            wait_until(0, g_orbit_delay);
            if (!g_tab_or_help)   // kludge because wait_until() calls driver_key_pressed
            {
                driver_sound_off();
            }
        }
    }
}

void snd_time_write()
{
    // cppcheck-suppress leakNoVarFunctionCall
    if (snd_open())
    {
        std::fprintf(s_snd_fp, "time=%-ld\n", (long)std::clock()*1000/CLOCKS_PER_SEC);
    }
}

void close_snd()
{
    if (s_snd_fp)
    {
        std::fclose(s_snd_fp);
    }
    s_snd_fp = nullptr;
}
