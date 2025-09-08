// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/passes_options.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/sticky_orbits.h"
#include "fractals/lorenz.h"
#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/full_screen_prompt.h"
#include "ui/get_corners.h"
#include "ui/id_keys.h"

#include <algorithm>
#include <iterator>

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

/*
     passes_options invoked by <p> key
*/

int passes_options()
{
    const char *choices[20];
    const char *pass_calc_modes[] = {"rect", "line"};

    FullScreenValues values[25];
    int i;

    const bool old_keep_screen_coords = g_keep_screen_coords;

    int ret = 0;

pass_option_restart:
    // fill up the choices (and previous values) arrays
    int k = -1;

    choices[++k] = "Periodicity (0=off, <0=show, >0=on, -255..+255)";
    values[k].type = 'i';
    int old_periodicity = g_user_periodicity_value;
    values[k].uval.ival = old_periodicity;

    choices[++k] = "Orbit delay (0 = none)";
    values[k].type = 'i';
    int old_orbit_delay = g_orbit_delay;
    values[k].uval.ival = old_orbit_delay;

    choices[++k] = "Orbit interval (1 ... 255)";
    values[k].type = 'i';
    int old_orbit_interval = (int) g_orbit_interval;
    values[k].uval.ival = old_orbit_interval;

    choices[++k] = "Maintain screen coordinates";
    values[k].type = 'y';
    values[k].uval.ch.val = g_keep_screen_coords ? 1 : 0;

    choices[++k] = "Orbit pass shape (rect, line)";
    //   choices[++k] = "Orbit pass shape (rect,line,func)";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 5;
    values[k].uval.ch.list_len = std::size(pass_calc_modes);
    values[k].uval.ch.list = pass_calc_modes;
    values[k].uval.ch.val = (g_draw_mode == OrbitDrawMode::RECTANGLE) ? 0
        : (g_draw_mode == OrbitDrawMode::LINE)                        ? 1
                                                                      : /* function */ 2;
    OrbitDrawMode old_draw_mode = g_draw_mode;

    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_PASSES_OPTIONS};
        i = full_screen_prompt("Passes Options\n"
                              "(not all combinations make sense)\n"
                              "(Press F2 for corner parameters)\n"
                              "(Press F6 for calculation parameters)", k+1, choices, values, 64 | 4, nullptr);
    }
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    int j = 0;   // return code

    g_user_periodicity_value = values[++k].uval.ival;
    g_user_periodicity_value = std::min(g_user_periodicity_value, 255);
    g_user_periodicity_value = std::max(g_user_periodicity_value, -255);
    if (g_user_periodicity_value != old_periodicity)
    {
        j = 1;
    }

    g_orbit_delay = values[++k].uval.ival;
    if (g_orbit_delay != old_orbit_delay)
    {
        j = 1;
    }

    g_orbit_interval = values[++k].uval.ival;
    g_orbit_interval = std::min(g_orbit_interval, 255L);
    g_orbit_interval = std::max(g_orbit_interval, 1L);
    if (g_orbit_interval != old_orbit_interval)
    {
        j = 1;
    }

    g_keep_screen_coords = values[++k].uval.ch.val != 0;
    if (g_keep_screen_coords != old_keep_screen_coords)
    {
        j = 1;
    }
    if (!g_keep_screen_coords)
    {
        g_set_orbit_corners = false;
    }

    switch (values[++k].uval.ch.val)
    {
    default:
    case 0:
        g_draw_mode = OrbitDrawMode::RECTANGLE;
        break;
    case 1:
        g_draw_mode = OrbitDrawMode::LINE;
        break;
    case 2:
        g_draw_mode = OrbitDrawMode::FUNCTION;
        break;
    }
    if (g_draw_mode != old_draw_mode)
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

} // namespace id::ui
