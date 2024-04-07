#include "get_toggles2.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "full_screen_prompt.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"

#include <cstdio>
#include <cstdlib>

/*
        get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
    char const *choices[18];
    fullscreenvalues uvalues[23];
    int old_rotate_lo, old_rotate_hi;
    int old_distestwidth;
    double old_potparam[3], old_inversion[3];
    long old_usr_distest;

    // fill up the choices (and previous values) arrays
    int k = -1;

    choices[++k] = "Look for finite attractor (0=no,>0=yes,<0=phase)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ch.val = g_finite_attractor ? 1 : 0;

    choices[++k] = "Potential Max Color (0 means off)";
    uvalues[k].type = 'i';
    old_potparam[0] = g_potential_params[0];
    uvalues[k].uval.ival = (int) old_potparam[0];

    choices[++k] = "          Slope";
    uvalues[k].type = 'd';
    old_potparam[1] = g_potential_params[1];
    uvalues[k].uval.dval = old_potparam[1];

    choices[++k] = "          Bailout";
    uvalues[k].type = 'i';
    old_potparam[2] = g_potential_params[2];
    uvalues[k].uval.ival = (int) old_potparam[2];

    choices[++k] = "          16 bit values";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_potential_16bit ? 1 : 0;

    choices[++k] = "Distance Estimator (0=off, <0=edge, >0=on):";
    uvalues[k].type = 'L';
    old_usr_distest = g_user_distance_estimator_value;
    uvalues[k].uval.Lval = old_usr_distest;

    choices[++k] = "          width factor:";
    uvalues[k].type = 'i';
    old_distestwidth = g_distance_estimator_width_factor;
    uvalues[k].uval.ival = old_distestwidth;

    choices[++k] = "Inversion radius or \"auto\" (0 means off)";
    choices[++k] = "          center X coordinate or \"auto\"";
    choices[++k] = "          center Y coordinate or \"auto\"";
    k = k - 3;
    for (int i = 0; i < 3; i++)
    {
        uvalues[++k].type = 's';
        old_inversion[i] = g_inversion[i];
        if (g_inversion[i] == AUTO_INVERT)
        {
            std::sprintf(uvalues[k].uval.sval, "auto");
        }
        else
        {
            std::sprintf(uvalues[k].uval.sval, "%-1.15lg", g_inversion[i]);
        }
    }
    choices[++k] = "  (use fixed radius & center when zooming)";
    uvalues[k].type = '*';

    choices[++k] = "Color cycling from color (0 ... 254)";
    uvalues[k].type = 'i';
    old_rotate_lo = g_color_cycle_range_lo;
    uvalues[k].uval.ival = old_rotate_lo;

    choices[++k] = "              to   color (1 ... 255)";
    uvalues[k].type = 'i';
    old_rotate_hi = g_color_cycle_range_hi;
    uvalues[k].uval.ival = old_rotate_hi;

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP_Y_OPTIONS;
    {
        int i = fullscreen_prompt("Extended Options\n"
                              "(not all combinations make sense)",
                              k+1, choices, uvalues, 0, nullptr);
        g_help_mode = old_help_mode;
        if (i < 0)
        {
            return -1;
        }
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    bool changed = false;

    if ((uvalues[++k].uval.ch.val != 0) != g_finite_attractor)
    {
        g_finite_attractor = uvalues[k].uval.ch.val != 0;
        changed = true;
    }

    g_potential_params[0] = uvalues[++k].uval.ival;
    if (g_potential_params[0] != old_potparam[0])
    {
        changed = true;
    }

    g_potential_params[1] = uvalues[++k].uval.dval;
    if (g_potential_params[0] != 0.0 && g_potential_params[1] != old_potparam[1])
    {
        changed = true;
    }

    g_potential_params[2] = uvalues[++k].uval.ival;
    if (g_potential_params[0] != 0.0 && g_potential_params[2] != old_potparam[2])
    {
        changed = true;
    }

    if ((uvalues[++k].uval.ch.val != 0) != g_potential_16bit)
    {
        g_potential_16bit = uvalues[k].uval.ch.val != 0;
        if (g_potential_16bit)                   // turned it on
        {
            if (g_potential_params[0] != 0.0)
            {
                changed = true;
            }
        }
        else // turned it off
        {
            if (!driver_diskp())   // ditch the disk video
            {
                enddisk();
            }
            else     // keep disk video, but ditch the fraction part at end
            {
                g_disk_16_bit = false;
            }
        }
    }

    ++k;
    g_user_distance_estimator_value = uvalues[k].uval.Lval;
    if (g_user_distance_estimator_value != old_usr_distest)
    {
        changed = true;
    }
    ++k;
    g_distance_estimator_width_factor = uvalues[k].uval.ival;
    if (g_user_distance_estimator_value && g_distance_estimator_width_factor != old_distestwidth)
    {
        changed = true;
    }

    for (int i = 0; i < 3; i++)
    {
        if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
        {
            g_inversion[i] = AUTO_INVERT;
        }
        else
        {
            g_inversion[i] = std::atof(uvalues[k].uval.sval);
        }
        if (old_inversion[i] != g_inversion[i]
            && (i == 0 || g_inversion[0] != 0.0))
        {
            changed = true;
        }
    }
    g_invert = (g_inversion[0] == 0.0) ? 0 : 3;
    ++k;

    g_color_cycle_range_lo = uvalues[++k].uval.ival;
    g_color_cycle_range_hi = uvalues[++k].uval.ival;
    if (g_color_cycle_range_lo < 0 || g_color_cycle_range_hi > 255 || g_color_cycle_range_lo > g_color_cycle_range_hi)
    {
        g_color_cycle_range_lo = old_rotate_lo;
        g_color_cycle_range_hi = old_rotate_hi;
    }

    return changed ? 1 : 0;
}
