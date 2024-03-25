/*
        Various routines that prompt for things.
*/
#include "port.h"
#include "prototyp.h"

#include "prompts2.h"

#include "ant.h"
#include "calcfrac.h"
#include "calc_frac_init.h"
#include "choice_builder.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "convert_corners.h"
#include "diskvid.h"
#include "double_to_string.h"
#include "drivers.h"
#include "evolve.h"
#include "expand_dirname.h"
#include "field_prompt.h"
#include "find_file.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "fractalp.h"
#include "full_screen_choice.h"
#include "full_screen_prompt.h"
#include "get_corners.h"
#include "get_file_entry.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "is_directory.h"
#include "loadfile.h"
#include "loadmap.h"
#include "lorenz.h"
#include "make_batch_file.h"
#include "make_path.h"
#include "memory.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "pixel_grid.h"
#include "prompts1.h"
#include "put_string_center.h"
#include "resume.h"
#include "shell_sort.h"
#include "slideshw.h"
#include "spindac.h"
#include "split_path.h"
#include "stereo.h"
#include "stop_msg.h"
#include "video_mode.h"
#include "zoom.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// speed key state values
#define MATCHING         0      // string matches list - speed key mode
#define TEMPLATE        -2      // wild cards present - buiding template
#define SEARCHPATH      -3      // no match - building path search name

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
    g_help_mode = help_labels::HELPYOPTS;
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


/*
     passes_options invoked by <p> key
*/

int passes_options()
{
    char const *choices[20];
    char const *passcalcmodes[] = {"rect", "line"};

    fullscreenvalues uvalues[25];
    int i, j, k;
    int ret;

    int old_periodicity, old_orbit_delay, old_orbit_interval;
    bool const old_keep_scrn_coords = g_keep_screen_coords;
    char old_drawmode;

    ret = 0;

pass_option_restart:
    // fill up the choices (and previous values) arrays
    k = -1;

    choices[++k] = "Periodicity (0=off, <0=show, >0=on, -255..+255)";
    uvalues[k].type = 'i';
    old_periodicity = g_user_periodicity_value;
    uvalues[k].uval.ival = old_periodicity;

    choices[++k] = "Orbit delay (0 = none)";
    uvalues[k].type = 'i';
    old_orbit_delay = g_orbit_delay;
    uvalues[k].uval.ival = old_orbit_delay;

    choices[++k] = "Orbit interval (1 ... 255)";
    uvalues[k].type = 'i';
    old_orbit_interval = (int)g_orbit_interval;
    uvalues[k].uval.ival = old_orbit_interval;

    choices[++k] = "Maintain screen coordinates";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_keep_screen_coords ? 1 : 0;

    choices[++k] = "Orbit pass shape (rect, line)";
    //   choices[++k] = "Orbit pass shape (rect,line,func)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 5;
    uvalues[k].uval.ch.llen = sizeof(passcalcmodes)/sizeof(*passcalcmodes);
    uvalues[k].uval.ch.list = passcalcmodes;
    uvalues[k].uval.ch.val = (g_draw_mode == 'r') ? 0
                             : (g_draw_mode == 'l') ? 1
                             :   /* function */    2;
    old_drawmode = g_draw_mode;

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPPOPTS;
    i = fullscreen_prompt("Passes Options\n"
                          "(not all combinations make sense)\n"
                          "(Press " FK_F2 " for corner parameters)\n"
                          "(Press " FK_F6 " for calculation parameters)", k+1, choices, uvalues, 64 | 4, nullptr);
    g_help_mode = old_help_mode;
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    j = 0;   // return code

    g_user_periodicity_value = uvalues[++k].uval.ival;
    if (g_user_periodicity_value > 255)
    {
        g_user_periodicity_value = 255;
    }
    if (g_user_periodicity_value < -255)
    {
        g_user_periodicity_value = -255;
    }
    if (g_user_periodicity_value != old_periodicity)
    {
        j = 1;
    }


    g_orbit_delay = uvalues[++k].uval.ival;
    if (g_orbit_delay != old_orbit_delay)
    {
        j = 1;
    }


    g_orbit_interval = uvalues[++k].uval.ival;
    if (g_orbit_interval > 255)
    {
        g_orbit_interval = 255;
    }
    if (g_orbit_interval < 1)
    {
        g_orbit_interval = 1;
    }
    if (g_orbit_interval != old_orbit_interval)
    {
        j = 1;
    }

    g_keep_screen_coords = uvalues[++k].uval.ch.val != 0;
    if (g_keep_screen_coords != old_keep_scrn_coords)
    {
        j = 1;
    }
    if (!g_keep_screen_coords)
    {
        g_set_orbit_corners = false;
    }

    {
        int tmp = uvalues[++k].uval.ch.val;
        switch (tmp)
        {
        default:
        case 0:
            g_draw_mode = 'r';
            break;
        case 1:
            g_draw_mode = 'l';
            break;
        case 2:
            g_draw_mode = 'f';
            break;
        }
    }
    if (g_draw_mode != old_drawmode)
    {
        j = 1;
    }

    if (i == FIK_F2)
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

    if (i == FIK_F6)
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


// for videomodes added new options "virtual x/y" that change "sx/ydots"
// for diskmode changed "viewx/ydots" to "virtual x/y" that do as above
// (since for diskmode they were updated by x/ydots that should be the
// same as sx/ydots for that mode)
// g_video_table and g_video_entry are now updated even for non-disk modes

// ---------------------------------------------------------------------
/*
    get_view_params() is called from FRACTINT.C whenever the 'v' key
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

    if (i == FIK_F4 && !driver_diskp())
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

// ---------------------------------------------------------------------

std::string const g_gray_map_file{"altern.map"};
