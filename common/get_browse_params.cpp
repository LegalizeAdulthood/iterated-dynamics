#include "get_browse_params.h"

#include "port.h"
#include "prototyp.h"

#include "choice_builder.h"
#include "cmdfiles.h"
#include "evolve.h"
#include "helpdefs.h"
#include "id_data.h"

#include <cstring>
#include <string>

// get browse parameters, returns 3 if anything changes.
int get_browse_params()
{
    ChoiceBuilder<10> choices;
    int i, k;

    bool old_auto_browse = g_auto_browse;
    bool old_browse_check_fractal_type = g_browse_check_fractal_type;
    bool old_brwscheckparms = g_browse_check_fractal_params;
    bool old_doublecaution  = g_confirm_file_deletes;
    int old_smallest_box_size_shown = g_smallest_box_size_shown;
    double old_smallest_window_display_size = g_smallest_window_display_size;
    std::string old_browse_mask = g_browse_mask;

get_brws_restart:
    // fill up the previous values arrays
    k = -1;

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
        .comment("Press " FK_F4 " to reset browse parameters to defaults.");

    help_labels const old_help_mode = g_help_mode;     // this prevents HELP from activating
    g_help_mode = help_labels::HELPBRWSPARMS;
    i = choices.prompt("Browse ('L'ook) Mode Options", 16);
    g_help_mode = old_help_mode;     // re-enable HELP
    if (i < 0)
    {
        return 0;
    }

    if (i == FIK_F4)
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

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    g_auto_browse = choices.read_yes_no();
    g_ask_video = choices.read_yes_no();
    g_browse_check_fractal_type = choices.read_yes_no();
    g_browse_check_fractal_params = choices.read_yes_no();
    g_confirm_file_deletes = choices.read_yes_no();
    g_smallest_window_display_size = choices.read_float_number();
    if (g_smallest_window_display_size < 0)
    {
        g_smallest_window_display_size = 0;
    }
    g_smallest_box_size_shown = choices.read_int_number();
    if (g_smallest_box_size_shown < 1)
    {
        g_smallest_box_size_shown = 1;
    }
    if (g_smallest_box_size_shown > 10)
    {
        g_smallest_box_size_shown = 10;
    }

    g_browse_mask = choices.read_string();

    i = 0;
    if (g_auto_browse != old_auto_browse ||
            g_browse_check_fractal_type != old_browse_check_fractal_type ||
            g_browse_check_fractal_params != old_brwscheckparms ||
            g_confirm_file_deletes != old_doublecaution ||
            g_smallest_window_display_size != old_smallest_window_display_size ||
            g_smallest_box_size_shown != old_smallest_box_size_shown ||
            !stricmp(g_browse_mask.c_str(), old_browse_mask.c_str()))
    {
        i = -3;
    }

    if (g_evolving)
    {
        // can't browse
        g_auto_browse = false;
        i = 0;
    }

    return i;
}
