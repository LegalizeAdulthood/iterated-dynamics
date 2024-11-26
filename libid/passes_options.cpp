// SPDX-License-Identifier: GPL-3.0-only
//
#include "passes_options.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "full_screen_prompt.h"
#include "get_corners.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "lorenz.h"
#include "sticky_orbits.h"
#include "value_saver.h"

/*
     passes_options invoked by <p> key
*/

int passes_options()
{
    char const *choices[20];
    char const *passcalcmodes[] = {"rect", "line"};

    FullScreenValues uvalues[25];
    int i;
    int j;
    int k;
    int ret;

    int old_periodicity;
    int old_orbit_delay;
    int old_orbit_interval;
    bool const old_keep_scrn_coords = g_keep_screen_coords;
    char old_drawmode;

    ret = 0;

pass_option_restart:
    // fill up the choices (and previous values) arrays
    k = -1;

    choices[++k] = "Periodicity (0=off, <0=show, >0=on, -255..+255)";
    uvalues[k].type = 'i';
    old_periodicity = g_user_periodicity_value;
    uvalues[k].uval.ival = old_periodicity;

    choices[++k] = "Orbit delay (0 = none)";
    uvalues[k].type = 'i';
    old_orbit_delay = g_orbit_delay;
    uvalues[k].uval.ival = old_orbit_delay;

    choices[++k] = "Orbit interval (1 ... 255)";
    uvalues[k].type = 'i';
    old_orbit_interval = (int)g_orbit_interval;
    uvalues[k].uval.ival = old_orbit_interval;

    choices[++k] = "Maintain screen coordinates";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_keep_screen_coords ? 1 : 0;

    choices[++k] = "Orbit pass shape (rect, line)";
    //   choices[++k] = "Orbit pass shape (rect,line,func)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 5;
    uvalues[k].uval.ch.llen = sizeof(passcalcmodes)/sizeof(*passcalcmodes);
    uvalues[k].uval.ch.list = passcalcmodes;
    uvalues[k].uval.ch.val = (g_draw_mode == 'r') ? 0
                             : (g_draw_mode == 'l') ? 1
                             :   /* function */    2;
    old_drawmode = g_draw_mode;

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_POPTS};
        i = fullscreen_prompt("Passes Options\n"
                              "(not all combinations make sense)\n"
                              "(Press F2 for corner parameters)\n"
                              "(Press F6 for calculation parameters)", k+1, choices, uvalues, 64 | 4, nullptr);
    }
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    j = 0;   // return code

    g_user_periodicity_value = uvalues[++k].uval.ival;
    if (g_user_periodicity_value > 255)
    {
        g_user_periodicity_value = 255;
    }
    if (g_user_periodicity_value < -255)
    {
        g_user_periodicity_value = -255;
    }
    if (g_user_periodicity_value != old_periodicity)
    {
        j = 1;
    }

    g_orbit_delay = uvalues[++k].uval.ival;
    if (g_orbit_delay != old_orbit_delay)
    {
        j = 1;
    }

    g_orbit_interval = uvalues[++k].uval.ival;
    if (g_orbit_interval > 255)
    {
        g_orbit_interval = 255;
    }
    if (g_orbit_interval < 1)
    {
        g_orbit_interval = 1;
    }
    if (g_orbit_interval != old_orbit_interval)
    {
        j = 1;
    }

    g_keep_screen_coords = uvalues[++k].uval.ch.val != 0;
    if (g_keep_screen_coords != old_keep_scrn_coords)
    {
        j = 1;
    }
    if (!g_keep_screen_coords)
    {
        g_set_orbit_corners = false;
    }

    {
        int tmp = uvalues[++k].uval.ch.val;
        switch (tmp)
        {
        default:
        case 0:
            g_draw_mode = 'r';
            break;
        case 1:
            g_draw_mode = 'l';
            break;
        case 2:
            g_draw_mode = 'f';
            break;
        }
    }
    if (g_draw_mode != old_drawmode)
    {
        j = 1;
    }

    if (i == ID_KEY_F2)
    {
        if (get_screen_corners() > 0)
        {
            ret = 1;
        }
        if (j)
        {
            ret = 1;
        }
        goto pass_option_restart;
    }

    if (i == ID_KEY_F6)
    {
        if (get_corners() > 0)
        {
            ret = 1;
        }
        if (j)
        {
            ret = 1;
        }
        goto pass_option_restart;
    }

    return j + ret;
}
