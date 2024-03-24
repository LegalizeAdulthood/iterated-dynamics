/*
        Various routines that prompt for things.
*/
#include "port.h"
#include "prototyp.h"

#include "prompts2.h"

#include "ant.h"
#include "calcfrac.h"
#include "choice_builder.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "double_to_string.h"
#include "drivers.h"
#include "evolve.h"
#include "expand_dirname.h"
#include "field_prompt.h"
#include "find_file.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "full_screen_choice.h"
#include "full_screen_prompt.h"
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
#include "miscres.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "prompts1.h"
#include "realdos.h"
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

static  int check_f6_key(int curkey, int choice);
static  int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match);
static  int get_screen_corners();

// speed key state values
#define MATCHING         0      // string matches list - speed key mode
#define TEMPLATE        -2      // wild cards present - buiding template
#define SEARCHPATH      -3      // no match - building path search name

#define GETPARM    3

static char commandmask[13] = {"*.par"};

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
            g_inversion[i] = atof(uvalues[k].uval.sval);
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

/*
    get_cmd_string() is called from FRACTINT.C whenever the 'g' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  parameter changed, no need to regenerate
        >0  parameter changed, regenerate
*/

int get_cmd_string()
{
    int i;
    static char cmdbuf[61];

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPCOMMANDS;
    i = field_prompt("Enter command string to use.", nullptr, cmdbuf, 60, nullptr);
    g_help_mode = old_help_mode;
    if (i >= 0 && cmdbuf[0] != 0)
    {
        i = cmdarg(cmdbuf, cmd_file::AT_AFTER_STARTUP);
        if (g_debug_flag == debug_flags::write_formula_debug_information)
        {
            backwards_v18();
            backwards_v19();
            backwards_v20();
        }
    }

    return i;
}


// ---------------------------------------------------------------------

long g_concentration;


static double starfield_values[4] =
{
    30.0, 100.0, 5.0, 0.0
};

std::string const g_gray_map_file{"altern.map"};

int starfield()
{   
    int c;
    g_busy = true;
    if (starfield_values[0] <   1.0)
    {
        starfield_values[0] =   1.0;
    }
    if (starfield_values[0] > 100.0)
    {
        starfield_values[0] = 100.0;
    }
    if (starfield_values[1] <   1.0)
    {
        starfield_values[1] =   1.0;
    }
    if (starfield_values[1] > 100.0)
    {
        starfield_values[1] = 100.0;
    }
    if (starfield_values[2] <   1.0)
    {
        starfield_values[2] =   1.0;
    }
    if (starfield_values[2] > 100.0)
    {
        starfield_values[2] = 100.0;
    }

    g_distribution = (int)(starfield_values[0]);
    g_concentration  = (long)(((starfield_values[1]) / 100.0) * (1L << 16));
    g_slope = (int)(starfield_values[2]);

    if (ValidateLuts(g_gray_map_file.c_str()))
    {
        stopmsg(STOPMSG_NONE, "Unable to load ALTERN.MAP");
        g_busy = false;
        return -1;
    }
    spindac(0, 1);                 // load it, but don't spin
    for (g_row = 0; g_row < g_logical_screen_y_dots; g_row++)
    {
        for (g_col = 0; g_col < g_logical_screen_x_dots; g_col++)
        {
            if (driver_key_pressed())
            {
                driver_buzzer(buzzer_codes::INTERRUPT);
                g_busy = false;
                return 1;
            }
            c = getcolor(g_col, g_row);
            if (c == g_inside_color)
            {
                c = g_colors-1;
            }
            g_put_color(g_col, g_row, GausianNumber(c, g_colors));
        }
    }
    driver_buzzer(buzzer_codes::COMPLETE);
    g_busy = false;
    return 0;
}

int get_starfield_params()
{
    if (g_colors < 255)
    {
        stopmsg(STOPMSG_NONE, "starfield requires 256 color mode");
        return -1;
    }

    ChoiceBuilder<3> builder;
    builder.float_number("Star Density in Pixels per Star", starfield_values[0])
        .float_number("Percent Clumpiness", starfield_values[1])
        .float_number("Ratio of Dim stars to Bright", starfield_values[2]);
    driver_stack_screen();
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPSTARFLD;
    int const choice = builder.prompt("Starfield Parameters");
    g_help_mode = old_help_mode;
    driver_unstack_screen();
    if (choice < 0)
    {
        return -1;
    }
    for (int i = 0; i < 3; i++)
    {
        starfield_values[i] = builder.read_float_number();
    }

    return 0;
}

static char const *masks[] = {"*.pot", "*.gif"};

int get_a_number(double *x, double *y)
{
    driver_stack_screen();

    // fill up the previous values arrays
    ChoiceBuilder<2> builder;
    builder.double_number("X coordinate at cursor", *x);
    builder.double_number("Y coordinate at cursor", *y);

    const int i = builder.prompt("Set Cursor Coordinates", 16 | 8 | 1);
    if (i < 0)
    {
        driver_unstack_screen();
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    *x = builder.read_double_number();
    *y = builder.read_double_number();

    driver_unstack_screen();
    return i;
}

// ---------------------------------------------------------------------

int get_commands()              // execute commands from file
{
    int ret;
    std::FILE *parmfile;
    ret = 0;
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPPARMFILE;
    long point = get_file_entry(GETPARM, "Parameter Set", commandmask, g_command_file, g_command_name);
    if (point >= 0 && (parmfile = std::fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        std::fseek(parmfile, point, SEEK_SET);
        ret = load_commands(parmfile);
    }
    g_help_mode = old_help_mode;
    return ret;
}

// ---------------------------------------------------------------------

void goodbye()                  // we done.  Bail out
{
    end_resume();
    ReleaseParamBox();
    if (!g_ifs_definition.empty())
    {
        g_ifs_definition.clear();
    }
    free_grid_pointers();
    free_ant_storage();
    enddisk();
    ExitCheck();
    if (!g_make_parameter_file)
    {
        driver_set_for_text();
    }
#if 0 && defined(XFRACT)
    UnixDone();
    std::printf("\n\n\n%s\n", "   Thank You for using " FRACTINT); // printf takes pointer
#endif
    if (!g_make_parameter_file)
    {
        driver_move_cursor(6, 0);
    }
    stopslideshow();
    end_help();
    int ret = 0;
    if (g_init_batch == batch_modes::BAILOUT_ERROR_NO_SAVE) // exit with error code for batch file
    {
        ret = 2;
    }
    else if (g_init_batch == batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE)
    {
        ret = 1;
    }
    close_drivers();
#if defined(_WIN32)
    _CrtDumpMemoryLeaks();
#endif
    exit(ret);
}


// ---------------------------------------------------------------------

struct CHOICE
{
    char name[13];
    char full_name[FILE_MAX_PATH];
    char type;
};

int lccompare(void *arg1, void *arg2) // for sort
{
    char **choice1 = (char **) arg1;
    char **choice2 = (char **) arg2;

    return stricmp(*choice1, *choice2);
}

#define MAXNUMFILES 2977L

static int speedstate;
bool getafilename(char const *hdg, char const *file_template, char *flname)
{
    char user_file_template[FILE_MAX_PATH] = { 0 };
    int rds;  // if getting an RDS image map
    char instr[80];
    int masklen;
    char filename[FILE_MAX_PATH]; // 13 is big enough for Fractint, but not Xfractint
    char speedstr[81];
    char tmpmask[FILE_MAX_PATH];   // used to locate next file in list
    char old_flname[FILE_MAX_PATH];
    int out;
    int retried;
    // Only the first 13 characters of file names are displayed...
    CHOICE storage[MAXNUMFILES];
    CHOICE *choices[MAXNUMFILES];
    int attributes[MAXNUMFILES];
    int filecount;   // how many files
    int dircount;    // how many directories
    bool notroot;     // not the root directory
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];

    static int numtemplates = 1;
    static bool dosort = true;

    rds = (g_stereo_map_filename == flname) ? 1 : 0;
    for (int i = 0; i < MAXNUMFILES; i++)
    {
        attributes[i] = 1;
        choices[i] = &storage[i];
    }
    // save filename
    std::strcpy(old_flname, flname);

restart:  // return here if template or directory changes
    tmpmask[0] = 0;
    if (flname[0] == 0)
    {
        std::strcpy(flname, DOTSLASH);
    }
    splitpath(flname , drive, dir, fname, ext);
    make_fname_ext(filename, fname, ext);
    retried = 0;

retry_dir:
    if (dir[0] == 0)
    {
        std::strcpy(dir, ".");
    }
    expand_dirname(dir, drive);
    make_drive_dir(tmpmask, drive, dir);
    fix_dirname(tmpmask);
    if (retried == 0 && std::strcmp(dir, SLASH) && std::strcmp(dir, DOTSLASH))
    {
        int j = (int) std::strlen(tmpmask) - 1;
        tmpmask[j] = 0; // strip trailing backslash
        if (std::strchr(tmpmask, '*') || std::strchr(tmpmask, '?')
            || fr_findfirst(tmpmask) != 0
            || (DTA.attribute & SUBDIR) == 0)
        {
            std::strcpy(dir, DOTSLASH);
            ++retried;
            goto retry_dir;
        }
        tmpmask[j] = SLASHC;
    }
    if (file_template[0])
    {
        numtemplates = 1;
        split_fname_ext(file_template, fname, ext);
    }
    else
    {
        numtemplates = sizeof(masks)/sizeof(masks[0]);
    }
    filecount = -1;
    dircount  = 0;
    notroot   = false;
    masklen = (int) std::strlen(tmpmask);
    std::strcat(tmpmask, "*.*");
    out = fr_findfirst(tmpmask);
    while (out == 0 && filecount < MAXNUMFILES)
    {
        if ((DTA.attribute & SUBDIR) && DTA.filename != ".")
        {
            if (DTA.filename != "..")
            {
                DTA.filename += SLASH;
            }
            std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
            choices[filecount]->name[12] = 0;
            choices[filecount]->type = 1;
            std::strcpy(choices[filecount]->full_name, DTA.filename.c_str());
            dircount++;
            if (DTA.filename == "..")
            {
                notroot = true;
            }
        }
        out = fr_findnext();
    }
    tmpmask[masklen] = 0;
    if (file_template[0])
    {
        make_path(tmpmask, drive, dir, fname, ext);
    }
    int j = 0;
    do
    {
        if (numtemplates > 1)
        {
            std::strcpy(&(tmpmask[masklen]), masks[j]);
        }
        out = fr_findfirst(tmpmask);
        while (out == 0 && filecount < MAXNUMFILES)
        {
            if (!(DTA.attribute & SUBDIR))
            {
                if (rds)
                {
                    putstringcenter(2, 0, 80, C_GENERAL_INPUT, DTA.filename.c_str());

                    split_fname_ext(DTA.filename, fname, ext);
                    // just using speedstr as a handy buffer
                    make_path(speedstr, drive, dir, fname, ext);
                    std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
                    choices[filecount]->type = 0;
                }
                else
                {
                    std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
                    choices[filecount]->type = 0;
                    std::strcpy(choices[filecount]->full_name, DTA.filename.c_str());
                }
            }
            out = fr_findnext();
        }
    }
    while (++j < numtemplates);
    if (++filecount == 0)
    {
        std::strcpy(choices[filecount]->name, "*nofiles*");
        choices[filecount]->type = 0;
        ++filecount;
    }

    std::strcpy(instr, "Press " FK_F6 " for default directory, " FK_F4 " to toggle sort ");
    if (dosort)
    {
        std::strcat(instr, "off");
        shell_sort(&choices, filecount, sizeof(CHOICE *), lccompare); // sort file list
    }
    else
    {
        std::strcat(instr, "on");
    }
    if (!notroot && dir[0] && dir[0] != SLASHC) // must be in root directory
    {
        splitpath(tmpmask, drive, dir, fname, ext);
        std::strcpy(dir, SLASH);
        make_path(tmpmask, drive, dir, fname, ext);
    }
    if (numtemplates > 1)
    {
        std::strcat(tmpmask, " ");
        std::strcat(tmpmask, masks[0]);
    }

    std::string const heading{std::string{hdg} + "\n"
        + "Template: " + tmpmask};
    std::strcpy(speedstr, filename);
    int i = 0;
    if (speedstr[0] == 0)
    {
        for (i = 0; i < filecount; i++) // find first file
        {
            if (choices[i]->type == 0)
            {
                break;
            }
        }
        if (i >= filecount)
        {
            i = 0;
        }
    }

    i = fullscreen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
        heading.c_str(), nullptr, instr, filecount, (char const **) choices,
        attributes, 5, 99, 12, i, nullptr, speedstr, filename_speedstr, check_f6_key);
    if (i == -FIK_F4)
    {
        dosort = !dosort;
        goto restart;
    }
    if (i == -FIK_F6)
    {
        static int lastdir = 0;
        if (lastdir == 0)
        {
            std::strcpy(dir, g_fractal_search_dir1.c_str());
        }
        else
        {
            std::strcpy(dir, g_fractal_search_dir2.c_str());
        }
        fix_dirname(dir);
        make_drive_dir(flname, drive, dir);
        lastdir = 1 - lastdir;
        goto restart;
    }
    if (i < 0)
    {
        // restore filename
        std::strcpy(flname, old_flname);
        return true;
    }
    if (speedstr[0] == 0 || speedstate == MATCHING)
    {
        if (choices[i]->type)
        {
            if (std::strcmp(choices[i]->name, "..") == 0) // go up a directory
            {
                if (std::strcmp(dir, DOTSLASH) == 0)
                {
                    std::strcpy(dir, DOTDOTSLASH);
                }
                else
                {
                    char *s = std::strrchr(dir, SLASHC);
                    if (s != nullptr) // trailing slash
                    {
                        *s = 0;
                        s = std::strrchr(dir, SLASHC);
                        if (s != nullptr)
                        {
                            *(s + 1) = 0;
                        }
                    }
                }
            }
            else  // go down a directory
            {
                std::strcat(dir, choices[i]->full_name);
            }
            fix_dirname(dir);
            make_drive_dir(flname, drive, dir);
            goto restart;
        }
        split_fname_ext(choices[i]->full_name, fname, ext);
        make_path(flname, drive, dir, fname, ext);
    }
    else
    {
        if (speedstate == SEARCHPATH
            && std::strchr(speedstr, '*') == nullptr && std::strchr(speedstr, '?') == nullptr
            && ((fr_findfirst(speedstr) == 0 && (DTA.attribute & SUBDIR))
                || std::strcmp(speedstr, SLASH) == 0)) // it is a directory
        {
            speedstate = TEMPLATE;
        }

        if (speedstate == TEMPLATE)
        {
            /* extract from tempstr the pathname and template information,
                being careful not to overwrite drive and directory if not
                newly specified */
            char drive1[FILE_MAX_DRIVE];
            char dir1[FILE_MAX_DIR];
            char fname1[FILE_MAX_FNAME];
            char ext1[FILE_MAX_EXT];
            splitpath(speedstr, drive1, dir1, fname1, ext1);
            if (drive1[0])
            {
                std::strcpy(drive, drive1);
            }
            if (dir1[0])
            {
                std::strcpy(dir, dir1);
            }
            make_path(flname, drive, dir, fname1, ext1);
            if (std::strchr(fname1, '*') || std::strchr(fname1, '?') ||
                    std::strchr(ext1,   '*') || std::strchr(ext1,   '?'))
            {
                make_fname_ext(user_file_template, fname1, ext1);
                // cppcheck-suppress uselessAssignmentPtrArg
                file_template = user_file_template;
            }
            else if (isadirectory(flname))
            {
                fix_dirname(flname);
            }
            goto restart;
        }
        else // speedstate == SEARCHPATH
        {
            const std::string full_path{find_path(speedstr)};
            if (!full_path.empty())
            {
                std::strcpy(flname, full_path.c_str());
            }
            else
            {
                // failed, make diagnostic useful:
                std::strcpy(flname, speedstr);
                if (std::strchr(speedstr, SLASHC) == nullptr)
                {
                    split_fname_ext(speedstr, fname, ext);
                    make_path(flname, drive, dir, fname, ext);
                }
            }
        }
    }
    g_browse_name = std::string{fname} + ext;
    return false;
}

bool getafilename(char const *hdg, char const *file_template, std::string &flname)
{
    char buff[FILE_MAX_PATH];
    std::strncpy(buff, flname.c_str(), FILE_MAX_PATH);
    bool const result = getafilename(hdg, file_template, buff);
    flname = buff;
    return result;
}

// choice is used by other routines called by fullscreen_choice()
static int check_f6_key(int curkey, int /*choice*/)
{
    if (curkey == FIK_F6)
    {
        return 0-FIK_F6;
    }
    else if (curkey == FIK_F4)
    {
        return 0-FIK_F4;
    }
    return 0;
}

static int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match)
{
    char const *prompt;
    if (std::strchr(speedstring, ':')
        || std::strchr(speedstring, '*') || std::strchr(speedstring, '*')
        || std::strchr(speedstring, '?'))
    {
        speedstate = TEMPLATE;  // template
        prompt = "File Template";
    }
    else if (speed_match)
    {
        speedstate = SEARCHPATH; // does not match list
        prompt = "Search Path for";
    }
    else
    {
        speedstate = MATCHING;
        prompt = g_speed_prompt.c_str();
    }
    driver_put_string(row, col, vid, prompt);
    return (int) std::strlen(prompt);
}

void make_path(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext)
{
    if (template_str == nullptr)
    {
        assert(template_str != nullptr);
        return;
    }

    fs::path result;
    auto not_empty = [](const char *ptr) { return ptr != nullptr && ptr[0] != 0; };
#ifndef XFRACT
    if (not_empty(drive))
    {
        result = drive;
    }
#endif
    if (not_empty(dir))
    {
        result /= dir;
        result.make_preferred();
        if (result.string().back() != fs::path::preferred_separator)
        {
            result += '/';
        }
    }
    if (not_empty(fname))
    {
        result /= fname;
    }
    if (not_empty(ext))
    {
        result.replace_extension(ext);
    }
    strcpy(template_str, result.lexically_normal().make_preferred().string().c_str());
}

static void dir_name(char *target, char const *dir, char const *name)
{
    *target = 0;
    if (*dir != 0)
    {
        std::strcpy(target, dir);
    }
    std::strcat(target, name);
}

// removes file in dir directory
int dir_remove(char const *dir, char const *filename)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::remove(tmp);
}

// fopens file in dir directory
std::FILE *dir_fopen(char const *dir, char const *filename, char const *mode)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::fopen(tmp, mode);
}

/*
   See if double value was changed by input screen. Problem is that the
   conversion from double to string and back can make small changes
   in the value, so will it twill test as "different" even though it
   is not
*/
int cmpdbl(double old, double new_val)
{
    char buf[81];
    fullscreenvalues val;

    // change the old value with the same torture the new value had
    double_to_string(buf, old);   // convert "old" to string

    old = atof(buf);                // convert back
    return std::fabs(old-new_val) < DBL_EPSILON ? 0 : 1; // zero if same
}

int get_corners()
{
    ChoiceBuilder<15> builder;
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int cmag;
    double Xctr, Yctr;
    LDBL Magnification; // LDBL not really needed here, but used to match function parameters
    double Xmagfactor, Rotation, Skew;
    double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;

    bool const ousemag = g_use_center_mag;
    oxxmin = g_x_min;
    oxxmax = g_x_max;
    oyymin = g_y_min;
    oyymax = g_y_max;
    oxx3rd = g_x_3rd;
    oyy3rd = g_y_3rd;

gc_loop:
    cmag = g_use_center_mag ? 1 : 0;
    if (g_draw_mode == 'l')
    {
        cmag = 0;
    }
    cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

    if (cmag)
    {
        builder.double_number("Center X", Xctr)
            .double_number("Center Y", Yctr)
            .double_number("Magnification", (double) Magnification)
            .double_number("X Magnification Factor", Xmagfactor)
            .double_number("Rotation Angle (degrees)", Rotation)
            .double_number("Skew Angle (degrees)", Skew)
            .comment("")
            .comment("Press " FK_F7 " to switch to \"corners\" mode");
    }

    else
    {
        if (g_draw_mode == 'l')
        {
            builder.comment("Left End Point")
                .double_number(xprompt, g_x_min)
                .double_number(yprompt, g_y_max)
                .comment("Right End Point")
                .double_number(xprompt, g_x_max)
                .double_number(yprompt, g_y_min);
        }
        else
        {
            builder.comment("Top-Left Corner")
                .double_number(xprompt, g_x_min)
                .double_number(yprompt, g_y_max)
                .comment("Bottom-Right Corner")
                .double_number(xprompt, g_x_max)
                .double_number(yprompt, g_y_min);
            if (g_x_min == g_x_3rd && g_y_min == g_y_3rd)
            {
                g_y_3rd = 0;
                g_x_3rd = g_y_3rd;
            }
            builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
                .double_number(xprompt, g_x_3rd)
                .double_number(yprompt, g_y_3rd)
                .comment("Press " FK_F7 " to switch to \"center-mag\" mode");
        }
    }
    builder.comment("Press " FK_F4 " to reset to type default values");

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPCOORDS;
    const int prompt_ret = builder.prompt("Image Coordinates", 128 | 16);
    g_help_mode = old_help_mode;

    if (prompt_ret < 0)
    {
        g_use_center_mag = ousemag;
        g_x_min = oxxmin;
        g_x_max = oxxmax;
        g_y_min = oyymin;
        g_y_max = oyymax;
        g_x_3rd = oxx3rd;
        g_y_3rd = oyy3rd;
        return -1;
    }

    if (prompt_ret == FIK_F4)
    {
        // reset to type defaults
        g_x_min = g_cur_fractal_specific->xmin;
        g_x_3rd = g_x_min;
        g_x_max = g_cur_fractal_specific->xmax;
        g_y_min = g_cur_fractal_specific->ymin;
        g_y_3rd = g_y_min;
        g_y_max = g_cur_fractal_specific->ymax;
        if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
        {
            aspectratio_crop(g_screen_aspect, g_final_aspect_ratio);
        }
        if (bf_math != bf_math_type::NONE)
        {
            fractal_floattobf();
        }
        goto gc_loop;
    }

    if (cmag)
    {
        const double new_x_center = builder.read_double_number();
        const double new_y_center = builder.read_double_number();
        const double new_magnification = builder.read_double_number();
        const double new_x_mag_factor = builder.read_double_number();
        const double new_rotation = builder.read_double_number();
        const double new_skew = builder.read_double_number();
        if (cmpdbl(Xctr, new_x_center)                           //
            || cmpdbl(Yctr, new_y_center)                        //
            || cmpdbl((double) Magnification, new_magnification) //
            || cmpdbl(Xmagfactor, new_x_mag_factor)              //
            || cmpdbl(Rotation, new_rotation)                    //
            || cmpdbl(Skew, new_skew))
        {
            Xctr          = new_x_center;
            Yctr          = new_y_center;
            Magnification = new_magnification;
            Xmagfactor    = new_x_mag_factor;
            Rotation      = new_rotation;
            Skew          = new_skew;
            if (Xmagfactor == 0)
            {
                Xmagfactor = 1;
            }
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        }
    }
    else
    {
        if (g_draw_mode == 'l')
        {
            builder.read_comment();
            g_x_min = builder.read_double_number();
            g_y_max = builder.read_double_number();
            builder.read_comment();
            g_x_max = builder.read_double_number();
            g_y_min = builder.read_double_number();
        }
        else
        {
            builder.read_comment();
            g_x_min = builder.read_double_number();
            g_y_max = builder.read_double_number();
            builder.read_comment();
            g_x_max = builder.read_double_number();
            g_y_min = builder.read_double_number();
            builder.read_comment();
            g_x_3rd = builder.read_double_number();
            g_y_3rd = builder.read_double_number();
            if (g_x_3rd == 0 && g_y_3rd == 0)
            {
                g_x_3rd = g_x_min;
                g_y_3rd = g_y_min;
            }
        }
    }

    if (prompt_ret == FIK_F7 && g_draw_mode != 'l')
    {
        // toggle corners/center-mag mode
        if (!g_use_center_mag)
        {
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            g_use_center_mag = true;
        }
        else
        {
            g_use_center_mag = false;
        }
        goto gc_loop;
    }

    if (!cmpdbl(oxxmin, g_x_min) && !cmpdbl(oxxmax, g_x_max) && !cmpdbl(oyymin, g_y_min) &&
            !cmpdbl(oyymax, g_y_max) && !cmpdbl(oxx3rd, g_x_3rd) && !cmpdbl(oyy3rd, g_y_3rd))
    {
        // no change, restore values to avoid drift
        g_x_min = oxxmin;
        g_x_max = oxxmax;
        g_y_min = oyymin;
        g_y_max = oyymax;
        g_x_3rd = oxx3rd;
        g_y_3rd = oyy3rd;
        return 0;
    }

    return 1;
}

static int get_screen_corners()
{
    ChoiceBuilder<15> builder;
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int prompt_ret;
    int cmag;
    double Xctr, Yctr;
    LDBL Magnification; // LDBL not really needed here, but used to match function parameters
    double Xmagfactor, Rotation, Skew;
    double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;
    double svxxmin, svxxmax, svyymin, svyymax, svxx3rd, svyy3rd;

    bool const ousemag = g_use_center_mag;

    svxxmin = g_x_min;  // save these for later since cvtcorners modifies them
    svxxmax = g_x_max;  // and we need to set them for cvtcentermag to work
    svxx3rd = g_x_3rd;
    svyymin = g_y_min;
    svyymax = g_y_max;
    svyy3rd = g_y_3rd;

    if (!g_set_orbit_corners && !g_keep_screen_coords)
    {
        g_orbit_corner_min_x = g_x_min;
        g_orbit_corner_max_x = g_x_max;
        g_orbit_corner_3_x = g_x_3rd;
        g_orbit_corner_min_y = g_y_min;
        g_orbit_corner_max_y = g_y_max;
        g_orbit_corner_3_y = g_y_3rd;
    }

    oxxmin = g_orbit_corner_min_x;
    oxxmax = g_orbit_corner_max_x;
    oyymin = g_orbit_corner_min_y;
    oyymax = g_orbit_corner_max_y;
    oxx3rd = g_orbit_corner_3_x;
    oyy3rd = g_orbit_corner_3_y;

    g_x_min = g_orbit_corner_min_x;
    g_x_max = g_orbit_corner_max_x;
    g_y_min = g_orbit_corner_min_y;
    g_y_max = g_orbit_corner_max_y;
    g_x_3rd = g_orbit_corner_3_x;
    g_y_3rd = g_orbit_corner_3_y;

gsc_loop:
    cmag = g_use_center_mag ? 1 : 0;
    cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

    builder.reset();
    if (cmag)
    {
        builder.double_number("Center X", Xctr)
            .double_number("Center Y", Yctr)
            .double_number("Magnification", static_cast<double>(Magnification))
            .double_number("X Magnification Factor", Xmagfactor)
            .double_number("Rotation Angle (degrees)", Rotation)
            .double_number("Skew Angle (degrees)", Skew)
            .comment("")
            .comment("Press " FK_F7 " to switch to \"corners\" mode");
    }
    else
    {
        builder.comment("Top-Left Corner")
            .double_number(xprompt, g_orbit_corner_min_x)
            .double_number(yprompt, g_orbit_corner_max_y)
            .comment("Bottom-Right Corner")
            .double_number(xprompt, g_orbit_corner_max_x)
            .double_number(yprompt, g_orbit_corner_min_y);
        if (g_orbit_corner_min_x == g_orbit_corner_3_x && g_orbit_corner_min_y == g_orbit_corner_3_y)
        {
            g_orbit_corner_3_y = 0;
            g_orbit_corner_3_x = g_orbit_corner_3_y;
        }
        builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
            .double_number(xprompt, g_orbit_corner_3_x)
            .double_number(yprompt, g_orbit_corner_3_y)
            .comment("Press " FK_F7 " to switch to \"center-mag\" mode");
    }
    builder.comment("Press " FK_F4 " to reset to type default values");

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPSCRNCOORDS;
    prompt_ret = builder.prompt("Screen Coordinates", 128 | 16);
    g_help_mode = old_help_mode;

    if (prompt_ret < 0)
    {
        g_use_center_mag = ousemag;
        g_orbit_corner_min_x = oxxmin;
        g_orbit_corner_max_x = oxxmax;
        g_orbit_corner_min_y = oyymin;
        g_orbit_corner_max_y = oyymax;
        g_orbit_corner_3_x = oxx3rd;
        g_orbit_corner_3_y = oyy3rd;
        // restore corners
        g_x_min = svxxmin;
        g_x_max = svxxmax;
        g_y_min = svyymin;
        g_y_max = svyymax;
        g_x_3rd = svxx3rd;
        g_y_3rd = svyy3rd;
        return -1;
    }

    if (prompt_ret == FIK_F4)
    {
        // reset to type defaults
        g_orbit_corner_min_x = g_cur_fractal_specific->xmin;
        g_orbit_corner_3_x = g_orbit_corner_min_x;
        g_orbit_corner_max_x = g_cur_fractal_specific->xmax;
        g_orbit_corner_min_y = g_cur_fractal_specific->ymin;
        g_orbit_corner_3_y = g_orbit_corner_min_y;
        g_orbit_corner_max_y = g_cur_fractal_specific->ymax;
        g_x_min = g_orbit_corner_min_x;
        g_x_max = g_orbit_corner_max_x;
        g_y_min = g_orbit_corner_min_y;
        g_y_max = g_orbit_corner_max_y;
        g_x_3rd = g_orbit_corner_3_x;
        g_y_3rd = g_orbit_corner_3_y;
        if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
        {
            aspectratio_crop(g_screen_aspect, g_final_aspect_ratio);
        }

        g_orbit_corner_min_x = g_x_min;
        g_orbit_corner_max_x = g_x_max;
        g_orbit_corner_min_y = g_y_min;
        g_orbit_corner_max_y = g_y_max;
        g_orbit_corner_3_x = g_x_min;
        g_orbit_corner_3_y = g_y_min;
        goto gsc_loop;
    }

    if (cmag)
    {
        const double new_x_center = builder.read_double_number();
        const double new_y_center = builder.read_double_number();
        const double new_magnification = builder.read_double_number();
        const double new_x_mag_factor = builder.read_double_number();
        const double new_rotation = builder.read_double_number();
        const double new_skew = builder.read_double_number();
        if (cmpdbl(Xctr, new_x_center)                           //
            || cmpdbl(Yctr, new_y_center)                        //
            || cmpdbl((double) Magnification, new_magnification) //
            || cmpdbl(Xmagfactor, new_x_mag_factor)              //
            || cmpdbl(Rotation, new_rotation)                    //
            || cmpdbl(Skew, new_skew))
        {
            Xctr          = new_x_center;
            Yctr          = new_y_center;
            Magnification = new_magnification;
            Xmagfactor    = new_x_mag_factor;
            Rotation      = new_rotation;
            Skew          = new_skew;
            if (Xmagfactor == 0)
            {
                Xmagfactor = 1;
            }
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
            // set screen corners
            g_orbit_corner_min_x = g_x_min;
            g_orbit_corner_max_x = g_x_max;
            g_orbit_corner_min_y = g_y_min;
            g_orbit_corner_max_y = g_y_max;
            g_orbit_corner_3_x = g_x_3rd;
            g_orbit_corner_3_y = g_y_3rd;
        }
    }
    else
    {
        builder.read_comment();
        g_orbit_corner_min_x = builder.read_double_number();
        g_orbit_corner_max_y = builder.read_double_number();
        builder.read_comment();
        g_orbit_corner_max_x = builder.read_double_number();
        g_orbit_corner_min_y = builder.read_double_number();
        builder.read_comment();
        g_orbit_corner_3_x = builder.read_double_number();
        g_orbit_corner_3_y = builder.read_double_number();
        if (g_orbit_corner_3_x == 0 && g_orbit_corner_3_y == 0)
        {
            g_orbit_corner_3_x = g_orbit_corner_min_x;
            g_orbit_corner_3_y = g_orbit_corner_min_y;
        }
    }

    if (prompt_ret == FIK_F7)
    {
        // toggle corners/center-mag mode
        if (!g_use_center_mag)
        {
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            g_use_center_mag = true;
        }
        else
        {
            g_use_center_mag = false;
        }
        goto gsc_loop;
    }

    if (!cmpdbl(oxxmin, g_orbit_corner_min_x) && !cmpdbl(oxxmax, g_orbit_corner_max_x) && !cmpdbl(oyymin, g_orbit_corner_min_y) &&
            !cmpdbl(oyymax, g_orbit_corner_max_y) && !cmpdbl(oxx3rd, g_orbit_corner_3_x) && !cmpdbl(oyy3rd, g_orbit_corner_3_y))
    {
        // no change, restore values to avoid drift
        g_orbit_corner_min_x = oxxmin;
        g_orbit_corner_max_x = oxxmax;
        g_orbit_corner_min_y = oyymin;
        g_orbit_corner_max_y = oyymax;
        g_orbit_corner_3_x = oxx3rd;
        g_orbit_corner_3_y = oyy3rd;
        // restore corners
        g_x_min = svxxmin;
        g_x_max = svxxmax;
        g_y_min = svyymin;
        g_y_max = svyymax;
        g_x_3rd = svxx3rd;
        g_y_3rd = svyy3rd;
        return 0;
    }
    else
    {
        g_set_orbit_corners = true;
        g_keep_screen_coords = true;
        // restore corners
        g_x_min = svxxmin;
        g_x_max = svxxmax;
        g_y_min = svyymin;
        g_y_max = svyymax;
        g_x_3rd = svxx3rd;
        g_y_3rd = svyy3rd;
        return 1;
    }
}

// extract just the filename/extension portion of a path
std::string extract_filename(char const *source)
{
    return fs::path(source).filename().string();
}

// tells if filename has extension
// returns pointer to period or nullptr
char const *has_ext(char const *source)
{
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT] = { 0 };
    split_fname_ext(source, fname, ext);
    char const *ret = nullptr;
    if (ext[0] != 0)
    {
        ret = std::strrchr(source, '.');
    }
    return ret;
}

void shell_sort(void *v1, int n, unsigned sz, int (*fct)(void *arg1, void *arg2))
{
    void *temp;
    char *v;
    v = (char *)v1;
    for (int gap = n/2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < n; i++)
        {
            for (int j = i-gap; j >= 0; j -= gap)
            {
                if (fct((char **)(v+j*sz), (char **)(v+(j+gap)*sz)) <= 0)
                {
                    break;
                }
                temp = *(char **)(v+j*sz);
                *(char **)(v+j*sz) = *(char **)(v+(j+gap)*sz);
                *(char **)(v+(j+gap)*sz) = static_cast<char *>(temp);
            }
        }
    }
}
