// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_toggles2.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "engine/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/full_screen_prompt.h"

#include <array> // std::size
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*
        get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
    const char *choices[18];
    FullScreenValues values[23];
    double old_potential_param[3];
    double old_inversion[3];

    // fill up the choices (and previous values) arrays
    int k = -1;

    choices[++k] = "Look for finite attractor (0=no,>0=yes,<0=phase)";
    values[k].type = 'i';
    values[k].uval.ch.val = g_finite_attractor ? 1 : 0;

    choices[++k] = "Potential Max Color (0 means off)";
    values[k].type = 'i';
    old_potential_param[0] = g_potential_params[0];
    values[k].uval.ival = (int) old_potential_param[0];

    choices[++k] = "          Slope";
    values[k].type = 'd';
    old_potential_param[1] = g_potential_params[1];
    values[k].uval.dval = old_potential_param[1];

    choices[++k] = "          Bailout";
    values[k].type = 'i';
    old_potential_param[2] = g_potential_params[2];
    values[k].uval.ival = (int) old_potential_param[2];

    choices[++k] = "          16 bit values";
    values[k].type = 'y';
    values[k].uval.ch.val = g_potential_16bit ? 1 : 0;

    choices[++k] = "Distance Estimator (0=off, <0=edge, >0=on):";
    values[k].type = 'L';
    long old_user_distance_estimator_value = g_user_distance_estimator_value;
    values[k].uval.Lval = old_user_distance_estimator_value;

    choices[++k] = "          width factor:";
    values[k].type = 'i';
    int old_distance_estimator_width_factor = g_distance_estimator_width_factor;
    values[k].uval.ival = old_distance_estimator_width_factor;

    choices[++k] = "Inversion radius or \"auto\" (0 means off)";
    choices[++k] = "          center X coordinate or \"auto\"";
    choices[++k] = "          center Y coordinate or \"auto\"";
    k = k - 3;
    for (int i = 0; i < 3; i++)
    {
        values[++k].type = 's';
        old_inversion[i] = g_inversion[i];
        if (g_inversion[i] == AUTO_INVERT)
        {
            std::sprintf(values[k].uval.sval, "auto");
        }
        else
        {
            char buff[80]{};
            std::sprintf(buff, "%-1.15lg", g_inversion[i]);
            buff[std::size(values[k].uval.sval)-1] = 0;
            std::strcpy(values[k].uval.sval, buff);
        }
    }
    choices[++k] = "  (use fixed radius & center when zooming)";
    values[k].type = '*';

    choices[++k] = "Color cycling from color (0 ... 254)";
    values[k].type = 'i';
    int old_rotate_lo = g_color_cycle_range_lo;
    values[k].uval.ival = old_rotate_lo;

    choices[++k] = "              to   color (1 ... 255)";
    values[k].type = 'i';
    int old_rotate_hi = g_color_cycle_range_hi;
    values[k].uval.ival = old_rotate_hi;

    const HelpLabels old_help_mode = g_help_mode;
    g_help_mode = HelpLabels::HELP_Y_OPTIONS;
    {
        int i = full_screen_prompt("Extended Options\n"
                              "(not all combinations make sense)",
                              k+1, choices, values, 0, nullptr);
        g_help_mode = old_help_mode;
        if (i < 0)
        {
            return -1;
        }
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    bool changed = false;

    if ((values[++k].uval.ch.val != 0) != g_finite_attractor)
    {
        g_finite_attractor = values[k].uval.ch.val != 0;
        changed = true;
    }

    g_potential_params[0] = values[++k].uval.ival;
    if (g_potential_params[0] != old_potential_param[0])
    {
        changed = true;
    }

    g_potential_params[1] = values[++k].uval.dval;
    if (g_potential_params[0] != 0.0 && g_potential_params[1] != old_potential_param[1])
    {
        changed = true;
    }

    g_potential_params[2] = values[++k].uval.ival;
    if (g_potential_params[0] != 0.0 && g_potential_params[2] != old_potential_param[2])
    {
        changed = true;
    }

    if ((values[++k].uval.ch.val != 0) != g_potential_16bit)
    {
        g_potential_16bit = values[k].uval.ch.val != 0;
        if (g_potential_16bit)                   // turned it on
        {
            if (g_potential_params[0] != 0.0)
            {
                changed = true;
            }
        }
        else // turned it off
        {
            if (!driver_is_disk())   // ditch the disk video
            {
                end_disk();
            }
            else     // keep disk video, but ditch the fraction part at end
            {
                g_disk_16_bit = false;
            }
        }
    }

    ++k;
    g_user_distance_estimator_value = values[k].uval.Lval;
    if (g_user_distance_estimator_value != old_user_distance_estimator_value)
    {
        changed = true;
    }
    ++k;
    g_distance_estimator_width_factor = values[k].uval.ival;
    if (g_user_distance_estimator_value && g_distance_estimator_width_factor != old_distance_estimator_width_factor)
    {
        changed = true;
    }

    for (int i = 0; i < 3; i++)
    {
        if (values[++k].uval.sval[0] == 'a' || values[k].uval.sval[0] == 'A')
        {
            g_inversion[i] = AUTO_INVERT;
        }
        else
        {
            g_inversion[i] = std::atof(values[k].uval.sval);
        }
        if (old_inversion[i] != g_inversion[i]
            && (i == 0 || g_inversion[0] != 0.0))
        {
            changed = true;
        }
    }
    g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
    ++k;

    g_color_cycle_range_lo = values[++k].uval.ival;
    g_color_cycle_range_hi = values[++k].uval.ival;
    if (g_color_cycle_range_lo < 0 || g_color_cycle_range_hi > 255 || g_color_cycle_range_lo > g_color_cycle_range_hi)
    {
        g_color_cycle_range_lo = old_rotate_lo;
        g_color_cycle_range_hi = old_rotate_hi;
    }

    return changed ? 1 : 0;
}
