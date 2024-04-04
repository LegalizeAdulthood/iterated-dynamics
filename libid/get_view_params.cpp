#include "get_view_params.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "full_screen_prompt.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "video_mode.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>

// for videomodes added new options "virtual x/y" that change "sx/ydots"
// for diskmode changed "viewx/ydots" to "virtual x/y" that do as above
// (since for diskmode they were updated by x/ydots that should be the
// same as sx/ydots for that mode)
// g_video_table and g_video_entry are now updated even for non-disk modes

// ---------------------------------------------------------------------
/*
    get_view_params() is called whenever the 'v' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  minor variable changed.  No need to re-generate the image.
         1  View changed.  Re-generate the image.
*/

int get_view_params()
{
    char const *choices[16];
    fullscreenvalues uvalues[25];
    int i, k;
    float old_viewreduction, old_aspectratio;
    int old_viewxdots, old_viewydots, old_sxdots, old_sydots;
    int xmax, ymax;
    char dim1[50];
    char dim2[50];

    driver_get_max_screen(&xmax, &ymax);

    bool const old_viewwindow    = g_view_window;
    old_viewreduction = g_view_reduction;
    old_aspectratio   = g_final_aspect_ratio;
    old_viewxdots     = g_view_x_dots;
    old_viewydots     = g_view_y_dots;
    old_sxdots        = g_screen_x_dots;
    old_sydots        = g_screen_y_dots;

get_view_restart:
    // fill up the previous values arrays
    k = -1;

    if (!driver_diskp())
    {
        choices[++k] = "Preview display? (no for full screen)";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = g_view_window ? 1 : 0;

        choices[++k] = "Auto window size reduction factor";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = g_view_reduction;

        choices[++k] = "Final media overall aspect ratio, y/x";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = g_final_aspect_ratio;

        choices[++k] = "Crop starting coordinates to new aspect ratio?";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = g_view_crop ? 1 : 0;

        choices[++k] = "Explicit size x pixels (0 for auto size)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_view_x_dots;

        choices[++k] = "              y pixels (0 to base on aspect ratio)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_view_y_dots;
    }

    choices[++k] = "";
    uvalues[k].type = '*';

    choices[++k] = "Virtual screen total x pixels";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_screen_x_dots;

    choices[++k] = driver_diskp() ?
                   "                     y pixels" :
                   "                     y pixels (0: by aspect ratio)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_screen_y_dots;

    choices[++k] = "Keep aspect? (cuts both x & y when either too big)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_keep_aspect_ratio ? 1 : 0;

    {
        char const *scrolltypes[] = {"fixed", "relaxed"};
        choices[++k] = "Zoombox scrolling (f[ixed], r[elaxed])";
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = sizeof(scrolltypes)/sizeof(*scrolltypes);
        uvalues[k].uval.ch.list = scrolltypes;
        uvalues[k].uval.ch.val = g_z_scroll ? 1 : 0;
    }

    choices[++k] = "";
    uvalues[k].type = '*';

    std::sprintf(dim1, "Video memory limits: (for y = %4d) x <= %d", ymax,  xmax);
    choices[++k] = dim1;
    uvalues[k].type = '*';

    std::sprintf(dim2, "                     (for x = %4d) y <= %d", xmax, ymax);
    choices[++k] = dim2;
    uvalues[k].type = '*';

    choices[++k] = "";
    uvalues[k].type = '*';

    if (!driver_diskp())
    {
        choices[++k] = "Press F4 to reset view parameters to defaults.";
        uvalues[k].type = '*';
    }

    help_labels const old_help_mode = g_help_mode;     // this prevents HELP from activating
    g_help_mode = help_labels::HELPVIEW;
    i = fullscreen_prompt("View Window Options", k+1, choices, uvalues, 16, nullptr);
    g_help_mode = old_help_mode;     // re-enable HELP
    if (i < 0)
    {
        return -1;
    }

    if (i == ID_KEY_F4 && !driver_diskp())
    {
        g_view_window = false;
        g_view_x_dots = 0;
        g_view_y_dots = 0;
        g_view_reduction = 4.2F;
        g_view_crop = true;
        g_final_aspect_ratio = g_screen_aspect;
        g_screen_x_dots = old_sxdots;
        g_screen_y_dots = old_sydots;
        g_keep_aspect_ratio = true;
        g_z_scroll = true;
        goto get_view_restart;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    if (!driver_diskp())
    {
        g_view_window = uvalues[++k].uval.ch.val != 0;
        g_view_reduction = (float) uvalues[++k].uval.dval;
        g_final_aspect_ratio = (float) uvalues[++k].uval.dval;
        g_view_crop = uvalues[++k].uval.ch.val != 0;
        g_view_x_dots = uvalues[++k].uval.ival;
        g_view_y_dots = uvalues[++k].uval.ival;
    }

    ++k;

    g_screen_x_dots = uvalues[++k].uval.ival;
    g_screen_y_dots = uvalues[++k].uval.ival;
    g_keep_aspect_ratio = uvalues[++k].uval.ch.val != 0;
    g_z_scroll = uvalues[++k].uval.ch.val != 0;

    if ((xmax != -1) && (g_screen_x_dots > xmax))
    {
        g_screen_x_dots = (int) xmax;
    }
    if (g_screen_x_dots < 2)
    {
        g_screen_x_dots = 2;
    }
    if (g_screen_y_dots == 0) // auto by aspect ratio request
    {
        if (g_final_aspect_ratio == 0.0)
        {
            g_final_aspect_ratio = (g_view_window && g_view_x_dots != 0 && g_view_y_dots != 0) ?
                               ((float) g_view_y_dots)/((float) g_view_x_dots) : old_aspectratio;
        }
        g_screen_y_dots = (int)(g_final_aspect_ratio*g_screen_x_dots + 0.5);
    }
    if ((ymax != -1) && (g_screen_y_dots > ymax))
    {
        g_screen_y_dots = ymax;
    }
    if (g_screen_y_dots < 2)
    {
        g_screen_y_dots = 2;
    }

    if (driver_diskp())
    {
        g_video_entry.xdots = g_screen_x_dots;
        g_video_entry.ydots = g_screen_y_dots;
        std::memcpy(&g_video_table[g_adapter], &g_video_entry, sizeof(g_video_entry));
        if (g_final_aspect_ratio == 0.0)
        {
            g_final_aspect_ratio = ((float) g_screen_y_dots)/((float) g_screen_x_dots);
        }
    }

    if (g_view_x_dots != 0 && g_view_y_dots != 0 && g_view_window && g_final_aspect_ratio == 0.0)
    {
        g_final_aspect_ratio = ((float) g_view_y_dots)/((float) g_view_x_dots);
    }
    else if (g_final_aspect_ratio == 0.0 && (g_view_x_dots == 0 || g_view_y_dots == 0))
    {
        g_final_aspect_ratio = old_aspectratio;
    }

    if (g_final_aspect_ratio != old_aspectratio && g_view_crop)
    {
        aspectratio_crop(old_aspectratio, g_final_aspect_ratio);
    }

    return (g_view_window != old_viewwindow
            || g_screen_x_dots != old_sxdots || g_screen_y_dots != old_sydots
            || (g_view_window
                && (g_view_reduction != old_viewreduction
                    || g_final_aspect_ratio != old_aspectratio
                    || g_view_x_dots != old_viewxdots
                    || (g_view_y_dots != old_viewydots && g_view_x_dots)))) ? 1 : 0;
}
