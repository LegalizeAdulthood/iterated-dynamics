// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_browse_params.h"

#include "engine/Browse.h"
#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/big_while_loop.h"
#include "ui/ChoiceBuilder.h"
#include "ui/evolve.h"
#include "ui/help.h"
#include "ui/id_keys.h"

#include <algorithm>
#include <filesystem>
#include <string>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

// get browse parameters, returns 3 if anything changes.
int get_browse_params()
{
    ChoiceBuilder<10> choices;
    int i;

    bool old_auto_browse = g_browse.auto_browse;
    bool old_browse_check_fractal_type = g_browse.check_fractal_type;
    bool old_browse_check_params = g_browse.check_fractal_params;
    bool old_double_caution  = g_browse.confirm_delete;
    int old_smallest_box_size_shown = g_browse.smallest_box;
    double old_smallest_window_display_size = g_browse.smallest_window;
    std::filesystem::path old_browse_mask = g_browse.mask;

get_brws_restart:
    choices.reset()
        .yes_no("Autobrowsing? (y/n)", g_browse.auto_browse)
        .yes_no("Ask about GIF video mode? (y/n)", g_ask_video)
        .yes_no("Check fractal type? (y/n)", g_browse.check_fractal_type)
        .yes_no("Check fractal parameters (y/n)", g_browse.check_fractal_params)
        .yes_no("Confirm file deletes (y/n)", g_browse.confirm_delete)
        .float_number("Smallest window to display (size in pixels)", g_browse.smallest_window)
        .int_number("Smallest box size shown before crosshairs used (pix)", g_browse.smallest_box)
        .string("Browse search filename mask ", g_browse.mask.string().c_str())
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
        g_browse.smallest_window = 6;
        g_browse.auto_browse = false;
        g_ask_video = true;
        g_browse.check_fractal_params = true;
        g_browse.check_fractal_type = true;
        g_browse.confirm_delete = true;
        g_browse.smallest_box = 3;
        g_browse.mask = "*.gif";
        goto get_brws_restart;
    }

    g_browse.auto_browse = choices.read_yes_no();
    g_ask_video = choices.read_yes_no();
    g_browse.check_fractal_type = choices.read_yes_no();
    g_browse.check_fractal_params = choices.read_yes_no();
    g_browse.confirm_delete = choices.read_yes_no();
    g_browse.smallest_window = choices.read_float_number();
    g_browse.smallest_window = std::max(g_browse.smallest_window, 0.0);
    g_browse.smallest_box = choices.read_int_number();
    g_browse.smallest_box = std::max(g_browse.smallest_box, 1);
    g_browse.smallest_box = std::min(g_browse.smallest_box, 10);
    g_browse.mask = std::filesystem::path{choices.read_string()}.filename().string();

    i = 0;
    if (g_browse.auto_browse != old_auto_browse                         //
        || g_browse.check_fractal_type != old_browse_check_fractal_type //
        || g_browse.check_fractal_params != old_browse_check_params     //
        || g_browse.confirm_delete != old_double_caution                //
        || g_browse.smallest_window != old_smallest_window_display_size //
        || g_browse.smallest_box != old_smallest_box_size_shown         //
        || g_browse.mask != old_browse_mask)
    {
        i = -3;
    }

    if (g_evolving != EvolutionModeFlags::NONE)
    {
        // can't browse
        g_browse.auto_browse = false;
        i = 0;
    }

    return i;
}

} // namespace id::ui
