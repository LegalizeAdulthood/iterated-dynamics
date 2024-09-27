// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_sound_params.h"

#include "port.h"
#include "prototyp.h"

#include "choice_builder.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "sound.h"
#include "value_saver.h"

#include <algorithm>
#include <cstdlib>

static int get_music_parms();
static int get_scale_map();

static int s_scale_map[12]{};
static int s_menu2{};

int get_sound_params()
{
    ChoiceBuilder<15> builder;
    /* routine to get sound settings  */
    const char *soundmodes[]{"off", "beep", "x", "y", "z"};
    int old_soundflag, old_orbit_delay;
    int i;
    bool old_start_showorbit;

    old_soundflag = g_sound_flag;
    old_orbit_delay = g_orbit_delay;
    old_start_showorbit = g_start_show_orbit;

    /* g_sound_flag bits 0..7 used as thus:
       bit 0,1,2 controls sound beep/off and x,y,z
          (0 == off 1 == beep, 2 == x, 3 == y, 4 == z)
       bit 3 controls PC speaker
       bit 4 controls sound card OPL3 FM sound
       bit 5 controls midi output
       bit 6 controls pitch quantise
       bit 7 free! */
get_sound_restart:
    s_menu2 = 0;
    builder.reset()
        .list("Sound (off, beep, x, y, z)", 5, 4, soundmodes, g_sound_flag & SOUNDFLAG_ORBITMASK)
        .yes_no("Use PC internal speaker?", (g_sound_flag & SOUNDFLAG_SPEAKER) != 0)
        .yes_no("Use sound card output?", (g_sound_flag & SOUNDFLAG_OPL3_FM) != 0)
        .yes_no("Midi...not implemented yet", (g_sound_flag & SOUNDFLAG_MIDI) != 0)
        .yes_no("Quantize note pitch ?", (g_sound_flag & SOUNDFLAG_QUANTIZED) != 0)
        .int_number("Orbit delay in ms (0 = none)", g_orbit_delay)
        .int_number("Base Hz Value", g_base_hertz)
        .yes_no("Show orbits?", g_start_show_orbit)
        .comment("")
        .comment("Press F6 for FM synth parameters, F7 for scale mappings")
        .comment("Press F4 to reset to default values");

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_SOUND};
        i = builder.prompt("Sound Control Screen", 255);
    }
    if (i < 0)
    {
        g_sound_flag = old_soundflag;
        g_orbit_delay = old_orbit_delay;
        g_start_show_orbit = old_start_showorbit;
        return -1; /*escaped */
    }

    g_sound_flag = builder.read_list();
    g_sound_flag |= builder.read_yes_no() ? SOUNDFLAG_SPEAKER : 0;
    g_sound_flag |= builder.read_yes_no() ? SOUNDFLAG_OPL3_FM : 0;
    g_sound_flag |= builder.read_yes_no() ? SOUNDFLAG_MIDI : 0;
    g_sound_flag |= builder.read_yes_no() ? SOUNDFLAG_QUANTIZED : 0;

    g_orbit_delay = builder.read_int_number();
    g_base_hertz = builder.read_int_number();
    g_start_show_orbit = builder.read_yes_no();

    /* now do any initialization needed and check for soundcard */
    if ((g_sound_flag & SOUNDFLAG_OPL3_FM) && !(old_soundflag & SOUNDFLAG_OPL3_FM))
    {
        driver_init_fm();
    }

    if (i == ID_KEY_F6)
    {
        get_music_parms(); /* see below, for controling fmsynth */
        goto get_sound_restart;
    }

    if (i == ID_KEY_F7)
    {
        get_scale_map(); /* see below, for setting scale mapping */
        goto get_sound_restart;
    }

    if (i == ID_KEY_F4)
    {
        g_sound_flag = SOUNDFLAG_SPEAKER | SOUNDFLAG_BEEP; /* reset to default */
        g_orbit_delay = 0;
        g_base_hertz = 440;
        g_start_show_orbit = 0;
        goto get_sound_restart;
    }

    return g_sound_flag != old_soundflag &&
            ((g_sound_flag & SOUNDFLAG_ORBITMASK) > 1 || (old_soundflag & SOUNDFLAG_ORBITMASK) > 1)
        ? 1
        : 0;
}

static int get_scale_map()
{
    ChoiceBuilder<15> builder;
    int i;

    ++s_menu2;
get_map_restart:
    builder.reset()
        .int_number("Scale map C (1)", s_scale_map[0])
        .int_number("Scale map C#(2)", s_scale_map[1])
        .int_number("Scale map D (3)", s_scale_map[2])
        .int_number("Scale map D#(4)", s_scale_map[3])
        .int_number("Scale map E (5)", s_scale_map[4])
        .int_number("Scale map F (6)", s_scale_map[5])
        .int_number("Scale map F#(7)", s_scale_map[6])
        .int_number("Scale map G (8)", s_scale_map[7])
        .int_number("Scale map G#(9)", s_scale_map[8])
        .int_number("Scale map A (10)", s_scale_map[9])
        .int_number("Scale map A#(11)", s_scale_map[10])
        .int_number("Scale map B (12)", s_scale_map[11])
        .comment("")
        .comment("Press F6 for FM synth parameters")
        .comment("Press F4 to reset to default values");

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_MUSIC};
        i = builder.prompt("Scale Mapping Screen", 255);
    }
    if (i < 0)
    {
        return -1;
    }

    for (int j = 0; j <= 11; j++)
    {
        s_scale_map[j] = std::min(12, std::abs(builder.read_int_number()));
    }

    if (i == ID_KEY_F6 && s_menu2 == 1)
    {
        get_music_parms(); /* see below, for controling fmsynth */
        goto get_map_restart;
    }
    if (i == ID_KEY_F6 && s_menu2 == 2)
    {
        s_menu2--;
    }

    if (i == ID_KEY_F4)
    {
        for (int j = 0; j <= 11; j++)
        {
            s_scale_map[j] = j + 1;
        }
        goto get_map_restart;
    }

    return 0;
}

static int get_music_parms()
{
    ChoiceBuilder<11> builder;
    const char *attenmodes[] = {"none", "low", "mid", "high"};
    help_labels oldhelpmode;
    int i;

    s_menu2++;
get_music_restart:
    builder.reset()
        .int_number("polyphony 1..9", g_polyphony + 1)
        .int_number("Wave type 0..7", g_fm_wavetype)
        .int_number("Note attack time   0..15", g_fm_attack)
        .int_number("Note decay time    0..15", g_fm_decay)
        .int_number("Note sustain level 0..15", g_fm_sustain)
        .int_number("Note release time  0..15", g_fm_release)
        .int_number("Soundcard volume?  0..63", g_fm_volume)
        .list("Hi pitch attenuation", 4, 4, attenmodes, g_hi_attenuation)
        .comment("")
        .comment("Press F7 for scale mappings")
        .comment("Press F4 to reset to default values");

    oldhelpmode = g_help_mode; /* this prevents HELP from activating */
    g_help_mode = help_labels::HELP_MUSIC;
    i = builder.prompt("FM Synth Card Control Screen", 255);
    g_help_mode = oldhelpmode; /* re-enable HELP */
    if (i < 0)
    {
        return -1;
    }

    g_polyphony = std::min(8, std::abs(builder.read_int_number() - 1));
    g_fm_wavetype = (builder.read_int_number()) & 0x07;
    g_fm_attack = (builder.read_int_number()) & 0x0F;
    g_fm_decay = (builder.read_int_number()) & 0x0F;
    g_fm_sustain = (builder.read_int_number()) & 0x0F;
    g_fm_release = (builder.read_int_number()) & 0x0F;
    g_fm_volume = (builder.read_int_number()) & 0x3F;
    g_hi_attenuation = builder.read_list();
    if (g_sound_flag & SOUNDFLAG_OPL3_FM)
    {
        driver_init_fm();
    }

    if (i == ID_KEY_F7 && s_menu2 == 1)
    {
        get_scale_map(); /* see above, for setting scale mapping */
        goto get_music_restart;
    }
    if (i == ID_KEY_F7 && s_menu2 == 2)
    {
        s_menu2--;
    }

    if (i == ID_KEY_F4)
    {
        g_polyphony = 0;
        g_fm_wavetype = 0;
        g_fm_attack = 5;
        g_fm_decay = 10;
        g_fm_sustain = 13;
        g_fm_release = 5;
        g_fm_volume = 63;
        g_hi_attenuation = 0;
        if (g_sound_flag & SOUNDFLAG_OPL3_FM)
        {
            driver_init_fm();
        }
        goto get_music_restart;
    }

    return 0;
}
