// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_toggles2.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/Inversion.h"
#include "engine/Potential.h"
#include "engine/spindac.h"
#include "engine/UserData.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/diskvid.h"
#include "ui/help.h"

#include <fmt/format.h>

#include <array>
#include <cstdlib>
#include <string>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

/*
        get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
    ChoiceBuilder<13> choices;
    std::array<double, 3> old_potential_param;
    std::array<double, 3> old_inversion;

    // fill up the choices (and previous values) arrays
    old_potential_param[0] = g_potential.params[0];
    old_potential_param[1] = g_potential.params[1];
    old_potential_param[2] = g_potential.params[2];

    long old_user_distance_estimator_value = g_user.distance_estimator_value;
    int old_distance_estimator_width_factor = g_distance_estimator_width_factor;

    choices.int_number("Look for finite attractor (0=no,>0=yes,<0=phase)", g_attractor.enabled ? 1 : 0)
        .int_number("Potential Max Color (0 means off)", static_cast<int>(old_potential_param[0]))
        .double_number("          Slope", old_potential_param[1])
        .int_number("          Bailout", static_cast<int>(old_potential_param[2]))
        .yes_no("          16 bit values", g_potential.store_16bit)
        .long_number("Distance Estimator (0=off, <0=edge, >0=on):", old_user_distance_estimator_value)
        .int_number("          width factor:", old_distance_estimator_width_factor);
    const char *inversion_choices[] = {"Inversion radius or \"auto\" (0 means off)",
        "          center X coordinate or \"auto\"", "          center Y coordinate or \"auto\""};
    for (int i = 0; i < 3; i++)
    {
        old_inversion[i] = g_inversion.params[i];
        if (g_inversion.params[i] == AUTO_INVERT)
        {
            choices.string(inversion_choices[i], "auto");
        }
        else
        {
            const std::string value{fmt::format("{:<1.15Lg}", g_inversion.params[i])};
            choices.string(inversion_choices[i], value.c_str());
        }
    }
    int old_rotate_lo = g_color_cycle_range_lo;
    int old_rotate_hi = g_color_cycle_range_hi;
    choices.comment("  (use fixed radius & center when zooming)")
        .int_number("Color cycling from color (0 ... 254)", old_rotate_lo)
        .int_number("              to   color (1 ... 255)", old_rotate_hi);

    const HelpLabels old_help_mode = g_help_mode;
    g_help_mode = HelpLabels::HELP_Y_OPTIONS;
    {
        const int i = choices.prompt("Extended Options\n"
                                     "(not all combinations make sense)");
        g_help_mode = old_help_mode;
        if (i < 0)
        {
            return -1;
        }
    }

    // now check out the results (*hopefully* in the same order <grin>)
    bool changed = false;

    const bool attractor_enabled{choices.read_int_number() != 0};
    if (attractor_enabled != g_attractor.enabled)
    {
        g_attractor.enabled = attractor_enabled;
        changed = true;
    }

    g_potential.params[0] = choices.read_int_number();
    if (g_potential.params[0] != old_potential_param[0])
    {
        changed = true;
    }

    g_potential.params[1] = choices.read_double_number();
    if (g_potential.params[0] != 0.0 && g_potential.params[1] != old_potential_param[1])
    {
        changed = true;
    }

    g_potential.params[2] = choices.read_int_number();
    if (g_potential.params[0] != 0.0 && g_potential.params[2] != old_potential_param[2])
    {
        changed = true;
    }

    const bool store_16bit{choices.read_yes_no()};
    if (store_16bit != g_potential.store_16bit)
    {
        g_potential.store_16bit = store_16bit;
        if (g_potential.store_16bit) // turned it on
        {
            if (g_potential.params[0] != 0.0)
            {
                changed = true;
            }
        }
        else                       // turned it off
        {
            if (!driver_is_disk()) // ditch the disk video
            {
                end_disk();
            }
            else // keep disk video, but ditch the fraction part at end
            {
                g_disk_16_bit = false;
            }
        }
    }

    g_user.distance_estimator_value = choices.read_long_number();
    if (g_user.distance_estimator_value != old_user_distance_estimator_value)
    {
        changed = true;
    }
    g_distance_estimator_width_factor = choices.read_int_number();
    if (g_user.distance_estimator_value && g_distance_estimator_width_factor != old_distance_estimator_width_factor)
    {
        changed = true;
    }

    for (int i = 0; i < 3; i++)
    {
        const char *value{choices.read_string()};
        if (value[0] == 'a' || value[0] == 'A')
        {
            g_inversion.params[i] = AUTO_INVERT;
        }
        else
        {
            g_inversion.params[i] = std::atof(value);
        }
        if (old_inversion[i] != g_inversion.params[i] && (i == 0 || g_inversion.params[0] != 0.0))
        {
            changed = true;
        }
    }
    g_inversion.invert = g_inversion.params[0] == 0.0 ? 0 : 3;
    choices.read_comment();

    g_color_cycle_range_lo = choices.read_int_number();
    g_color_cycle_range_hi = choices.read_int_number();
    if (g_color_cycle_range_lo < 0 || g_color_cycle_range_hi > 255 || g_color_cycle_range_lo > g_color_cycle_range_hi)
    {
        g_color_cycle_range_lo = old_rotate_lo;
        g_color_cycle_range_hi = old_rotate_hi;
    }

    return changed ? 1 : 0;
}

} // namespace id::ui
