// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_browse_params.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/evolve.h"
#include "ui/id_keys.h"

#include <config/string_case_compare.h>

#include <algorithm>
#include <string>

// get browse parameters, returns 3 if anything changes.
int get_browse_params()
{
    ChoiceBuilder<10> choices;
    int i;

    bool old_auto_browse = g_auto_browse;
    bool old_browse_check_fractal_type = g_browse_check_fractal_type;
    bool old_browse_check_params = g_browse_check_fractal_params;
    bool old_double_caution  = g_confirm_file_deletes;
    int old_smallest_box_size_shown = g_smallest_box_size_shown;
    double old_smallest_window_display_size = g_smallest_window_display_size;
    std::string old_browse_mask = g_browse_mask;

get_brws_restart:
    choices.reset()
        .yes_no("Autobrowsing? (y/n)", g_auto_browse)
        .yes_no("Ask about GIF video mode? (y/n)", g_ask_video)
        .yes_no("Check fractal type? (y/n)", g_browse_check_fractal_type)
        .yes_no("Check fractal parameters (y/n)", g_browse_check_fractal_params)
        .yes_no("Confirm file deletes (y/n)", g_confirm_file_deletes)
        .float_number("Smallest window to display (size in pixels)", g_smallest_window_display_size)
        .int_number("Smallest box size shown before crosshairs used (pix)", g_smallest_box_size_shown)
        .string("Browse search filename mask ", g_browse_mask.c_str())
        .comment("")
        .comment("Press F4 to reset browse parameters to defaults.");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_BROWSE_PARAMETERS};
        i = choices.prompt("Browse ('L'ook) Mode Options", 16);
    }
    if (i < 0)
    {
        return 0;
    }

    if (i == ID_KEY_F4)
    {
        g_smallest_window_display_size = 6;
        g_auto_browse = false;
        g_ask_video = true;
        g_browse_check_fractal_params = true;
        g_browse_check_fractal_type = true;
        g_confirm_file_deletes = true;
        g_smallest_box_size_shown = 3;
        g_browse_mask = "*.gif";
        goto get_brws_restart;
    }

    g_auto_browse = choices.read_yes_no();
    g_ask_video = choices.read_yes_no();
    g_browse_check_fractal_type = choices.read_yes_no();
    g_browse_check_fractal_params = choices.read_yes_no();
    g_confirm_file_deletes = choices.read_yes_no();
    g_smallest_window_display_size = choices.read_float_number();
    g_smallest_window_display_size = std::max(g_smallest_window_display_size, 0.0);
    g_smallest_box_size_shown = choices.read_int_number();
    g_smallest_box_size_shown = std::max(g_smallest_box_size_shown, 1);
    g_smallest_box_size_shown = std::min(g_smallest_box_size_shown, 10);
    g_browse_mask = choices.read_string();

    i = 0;
    if (g_auto_browse != old_auto_browse                                      //
        || g_browse_check_fractal_type != old_browse_check_fractal_type       //
        || g_browse_check_fractal_params != old_browse_check_params           //
        || g_confirm_file_deletes != old_double_caution                       //
        || g_smallest_window_display_size != old_smallest_window_display_size //
        || g_smallest_box_size_shown != old_smallest_box_size_shown           //
        || g_browse_mask != old_browse_mask)
    {
        i = -3;
    }

    if (g_evolving != EvolutionModeFlags::NONE)
    {
        // can't browse
        g_auto_browse = false;
        i = 0;
    }

    return i;
}
