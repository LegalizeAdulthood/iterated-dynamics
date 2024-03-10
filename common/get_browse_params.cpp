#include "get_browse_params.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "evolve.h"
#include "helpdefs.h"
#include "id_data.h"
#include "prompts1.h"

#include <cstring>
#include <string>

// get browse parameters, returns 3 if anything changes.
int get_browse_params()
{
    char const *choices[10];
    fullscreenvalues uvalues[25];
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

    choices[++k] = "Autobrowsing? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_auto_browse ? 1 : 0;

    choices[++k] = "Ask about GIF video mode? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_ask_video ? 1 : 0;

    choices[++k] = "Check fractal type? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_browse_check_fractal_type ? 1 : 0;

    choices[++k] = "Check fractal parameters (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_browse_check_fractal_params ? 1 : 0;

    choices[++k] = "Confirm file deletes (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_confirm_file_deletes ? 1 : 0;

    choices[++k] = "Smallest window to display (size in pixels)";
    uvalues[k].type = 'f';
    uvalues[k].uval.dval = g_smallest_window_display_size;

    choices[++k] = "Smallest box size shown before crosshairs used (pix)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_smallest_box_size_shown;
    choices[++k] = "Browse search filename mask ";
    uvalues[k].type = 's';
    std::strcpy(uvalues[k].uval.sval, g_browse_mask.c_str());

    choices[++k] = "";
    uvalues[k].type = '*';

    choices[++k] = "Press " FK_F4 " to reset browse parameters to defaults.";
    uvalues[k].type = '*';

    help_labels const old_help_mode = g_help_mode;     // this prevents HELP from activating
    g_help_mode = help_labels::HELPBRWSPARMS;
    i = fullscreen_prompt("Browse ('L'ook) Mode Options", k+1, choices, uvalues, 16, nullptr);
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

    g_auto_browse = uvalues[++k].uval.ch.val != 0;
    g_ask_video = uvalues[++k].uval.ch.val != 0;
    g_browse_check_fractal_type = uvalues[++k].uval.ch.val != 0;
    g_browse_check_fractal_params = uvalues[++k].uval.ch.val != 0;
    g_confirm_file_deletes = uvalues[++k].uval.ch.val != 0;
    g_smallest_window_display_size = uvalues[++k].uval.dval;
    if (g_smallest_window_display_size < 0)
    {
        g_smallest_window_display_size = 0 ;
    }
    g_smallest_box_size_shown = uvalues[++k].uval.ival;
    if (g_smallest_box_size_shown < 1)
    {
        g_smallest_box_size_shown = 1;
    }
    if (g_smallest_box_size_shown > 10)
    {
        g_smallest_box_size_shown = 10;
    }

    g_browse_mask = uvalues[++k].uval.sval;

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
