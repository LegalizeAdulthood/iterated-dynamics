// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_view_params.h"

#include "engine/cmdfiles.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/full_screen_prompt.h"
#include "ui/help.h"
#include "ui/id_keys.h"
#include "ui/zoom.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

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
    const char *choices[16];
    FullScreenValues values[25];
    int i;
    int x_max;
    int y_max;
    char dim1[50];
    char dim2[50];

    driver_get_max_screen(x_max, y_max);

    const bool old_view_window    = g_viewport.enabled;
    float old_view_reduction = g_viewport.reduction;
    float old_aspect_ratio = g_viewport.final_aspect_ratio;
    int old_view_x_dots = g_viewport.x_dots;
    int old_view_y_dots = g_viewport.y_dots;
    int old_screen_x_dots = g_screen_x_dots;
    int old_screen_y_dots = g_screen_y_dots;

get_view_restart:
    // fill up the previous values arrays
    int k = -1;

    if (!driver_is_disk())
    {
        choices[++k] = "Preview display? (no for full screen)";
        values[k].type = 'y';
        values[k].uval.ch.val = g_viewport.enabled ? 1 : 0;

        choices[++k] = "Auto window size reduction factor";
        values[k].type = 'f';
        values[k].uval.dval = g_viewport.reduction;

        choices[++k] = "Final media overall aspect ratio, y/x";
        values[k].type = 'f';
        values[k].uval.dval = g_viewport.final_aspect_ratio;

        choices[++k] = "Crop starting coordinates to new aspect ratio?";
        values[k].type = 'y';
        values[k].uval.ch.val = g_viewport.crop ? 1 : 0;

        choices[++k] = "Explicit size x pixels (0 for auto size)";
        values[k].type = 'i';
        values[k].uval.ival = g_viewport.x_dots;

        choices[++k] = "              y pixels (0 to base on aspect ratio)";
        values[k].type = 'i';
        values[k].uval.ival = g_viewport.y_dots;
    }

    choices[++k] = "";
    values[k].type = '*';

    choices[++k] = "Virtual screen total x pixels";
    values[k].type = 'i';
    values[k].uval.ival = g_screen_x_dots;

    choices[++k] = driver_is_disk() ?
                   "                     y pixels" :
                   "                     y pixels (0: by aspect ratio)";
    values[k].type = 'i';
    values[k].uval.ival = g_screen_y_dots;

    choices[++k] = "Keep aspect? (cuts both x & y when either too big)";
    values[k].type = 'y';
    values[k].uval.ch.val = g_viewport.keep_aspect_ratio ? 1 : 0;

    {
        static const char *scroll_types[] = {"fixed", "relaxed"};
        choices[++k] = "Zoombox scrolling (f[ixed], r[elaxed])";
        values[k].type = 'l';
        values[k].uval.ch.vlen = 7;
        values[k].uval.ch.list_len = std::size(scroll_types);
        values[k].uval.ch.list = scroll_types;
        values[k].uval.ch.val = g_viewport.z_scroll ? 1 : 0;
    }

    choices[++k] = "";
    values[k].type = '*';

    *fmt::format_to(dim1, "Video memory limits: (for y = {:4d}) x <= {:d}", y_max, x_max).out = '\0';
    choices[++k] = dim1;
    values[k].type = '*';

    *fmt::format_to(dim2, "                     (for x = {:4d}) y <= {:d}", x_max, y_max).out = '\0';
    choices[++k] = dim2;
    values[k].type = '*';

    choices[++k] = "";
    values[k].type = '*';

    if (!driver_is_disk())
    {
        choices[++k] = "Press F4 to reset view parameters to defaults.";
        values[k].type = '*';
    }

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_VIEW};
        i = full_screen_prompt("View Window Options", k+1, choices, values, 16, nullptr);
    }
    if (i < 0)
    {
        return -1;
    }

    if (i == ID_KEY_F4 && !driver_is_disk())
    {
        g_viewport.enabled = false;
        g_viewport.x_dots = 0;
        g_viewport.y_dots = 0;
        g_viewport.reduction = 4.2F;
        g_viewport.crop = true;
        g_viewport.final_aspect_ratio = g_screen_aspect;
        g_screen_x_dots = old_screen_x_dots;
        g_screen_y_dots = old_screen_y_dots;
        g_viewport.keep_aspect_ratio = true;
        g_viewport.z_scroll = true;
        goto get_view_restart;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    if (!driver_is_disk())
    {
        g_viewport.enabled = values[++k].uval.ch.val != 0;
        g_viewport.reduction = static_cast<float>(values[++k].uval.dval);
        g_viewport.final_aspect_ratio = static_cast<float>(values[++k].uval.dval);
        g_viewport.crop = values[++k].uval.ch.val != 0;
        g_viewport.x_dots = values[++k].uval.ival;
        g_viewport.y_dots = values[++k].uval.ival;
    }

    ++k;

    g_screen_x_dots = values[++k].uval.ival;
    g_screen_y_dots = values[++k].uval.ival;
    g_viewport.keep_aspect_ratio = values[++k].uval.ch.val != 0;
    g_viewport.z_scroll = values[++k].uval.ch.val != 0;

    if (x_max != -1 && g_screen_x_dots > x_max)
    {
        g_screen_x_dots = x_max;
    }
    g_screen_x_dots = std::max(g_screen_x_dots, 2);
    if (g_screen_y_dots == 0) // auto by aspect ratio request
    {
        if (g_viewport.final_aspect_ratio == 0.0)
        {
            g_viewport.final_aspect_ratio = g_viewport.enabled && g_viewport.x_dots != 0 && g_viewport.y_dots != 0 ?
                               static_cast<float>(g_viewport.y_dots) / static_cast<float>(g_viewport.x_dots) : old_aspect_ratio;
        }
        g_screen_y_dots = static_cast<int>(std::lround(g_viewport.final_aspect_ratio * g_screen_x_dots));
    }
    if (y_max != -1 && g_screen_y_dots > y_max)
    {
        g_screen_y_dots = y_max;
    }
    g_screen_y_dots = std::max(g_screen_y_dots, 2);

    if (driver_is_disk())
    {
        g_video_entry.x_dots = g_screen_x_dots;
        g_video_entry.y_dots = g_screen_y_dots;
        g_video_table[g_adapter] = g_video_entry;
        if (g_viewport.final_aspect_ratio == 0.0)
        {
            g_viewport.final_aspect_ratio = static_cast<float>(g_screen_y_dots) / static_cast<float>(g_screen_x_dots);
        }
    }

    if (g_viewport.x_dots != 0 && g_viewport.y_dots != 0 && g_viewport.enabled && g_viewport.final_aspect_ratio == 0.0)
    {
        g_viewport.final_aspect_ratio = static_cast<float>(g_viewport.y_dots) / static_cast<float>(g_viewport.x_dots);
    }
    else if (g_viewport.final_aspect_ratio == 0.0 && (g_viewport.x_dots == 0 || g_viewport.y_dots == 0))
    {
        g_viewport.final_aspect_ratio = old_aspect_ratio;
    }

    if (g_viewport.final_aspect_ratio != old_aspect_ratio && g_viewport.crop)
    {
        aspect_ratio_crop(old_aspect_ratio, g_viewport.final_aspect_ratio);
    }

    return g_viewport.enabled != old_view_window || g_screen_x_dots != old_screen_x_dots ||
            g_screen_y_dots != old_screen_y_dots ||
            (g_viewport.enabled &&
                (g_viewport.reduction != old_view_reduction || g_viewport.final_aspect_ratio != old_aspect_ratio ||
                    g_viewport.x_dots != old_view_x_dots || (g_viewport.y_dots != old_view_y_dots && g_viewport.x_dots))) ? 1 : 0;
}

} // namespace id::ui
