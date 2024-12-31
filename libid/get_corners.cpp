// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_corners.h"

#include "calc_frac_init.h"
#include "ChoiceBuilder.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "convert_corners.h"
#include "double_to_string.h"
#include "fractalp.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "lorenz.h"
#include "port.h"
#include "sticky_orbits.h"
#include "ValueSaver.h"
#include "zoom.h"

#include <cfloat>
#include <cmath>
#include <cstdlib>

/*
   See if double value was changed by input screen. Problem is that the
   conversion from double to string and back can make small changes
   in the value, so will it twill test as "different" even though it
   is not
*/
static int cmp_dbl(double old, double new_val)
{
    char buf[81];

    // change the old value with the same torture the new value had
    double_to_string(buf, old);   // convert "old" to string

    old = std::atof(buf);                // convert back
    return std::fabs(old-new_val) < DBL_EPSILON ? 0 : 1; // zero if same
}

int get_corners()
{
    ChoiceBuilder<11> builder;
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int cmag;
    double x_ctr, y_ctr;
    LDBL magnification; // LDBL not really needed here, but used to match function parameters
    double x_mag_factor, rotation, skew;
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
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);

    builder.reset();
    // 10 items
    if (cmag)
    {
        // 8 items
        builder.double_number("Center X", x_ctr)
            .double_number("Center Y", y_ctr)
            .double_number("Magnification", (double) magnification)
            .double_number("X Magnification Factor", x_mag_factor)
            .double_number("Rotation Angle (degrees)", rotation)
            .double_number("Skew Angle (degrees)", skew)
            .comment("")
            .comment("Press F7 to switch to \"corners\" mode");
    }
    else
    {
        // 10 items
        if (g_draw_mode == 'l')
        {
            // 6 items
            builder.comment("Left End Point")
                .double_number(xprompt, g_x_min)
                .double_number(yprompt, g_y_max)
                .comment("Right End Point")
                .double_number(xprompt, g_x_max)
                .double_number(yprompt, g_y_min);
        }
        else
        {
            // 6 items
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
            // 4 items
            builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
                .double_number(xprompt, g_x_3rd)
                .double_number(yprompt, g_y_3rd)
                .comment("Press F7 to switch to \"center-mag\" mode");
        }
    }
    // 1 item
    builder.comment("Press F4 to reset to type default values");

    int prompt_ret;
    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_COORDS};
        prompt_ret = builder.prompt("Image Coordinates", 128 | 16);
    }

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

    if (prompt_ret == ID_KEY_F4)
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
            aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
        }
        if (g_bf_math != BFMathType::NONE)
        {
            fractal_float_to_bf();
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
        if (cmp_dbl(x_ctr, new_x_center)                          //
            || cmp_dbl(y_ctr, new_y_center)                       //
            || cmp_dbl((double) magnification, new_magnification) //
            || cmp_dbl(x_mag_factor, new_x_mag_factor)            //
            || cmp_dbl(rotation, new_rotation)                    //
            || cmp_dbl(skew, new_skew))
        {
            x_ctr         = new_x_center;
            y_ctr         = new_y_center;
            magnification = new_magnification;
            x_mag_factor  = new_x_mag_factor;
            rotation      = new_rotation;
            skew          = new_skew;
            if (x_mag_factor == 0)
            {
                x_mag_factor = 1;
            }
            cvt_corners(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
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

    if (prompt_ret == ID_KEY_F7 && g_draw_mode != 'l')
    {
        // toggle corners/center-mag mode
        if (!g_use_center_mag)
        {
            cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
            g_use_center_mag = true;
        }
        else
        {
            g_use_center_mag = false;
        }
        goto gc_loop;
    }

    if (!cmp_dbl(oxxmin, g_x_min) && !cmp_dbl(oxxmax, g_x_max) && !cmp_dbl(oyymin, g_y_min) &&
            !cmp_dbl(oyymax, g_y_max) && !cmp_dbl(oxx3rd, g_x_3rd) && !cmp_dbl(oyy3rd, g_y_3rd))
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

int get_screen_corners()
{
    ChoiceBuilder<15> builder;
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int prompt_ret;
    int cmag;
    double x_ctr, y_ctr;
    LDBL magnification; // LDBL not really needed here, but used to match function parameters
    double x_mag_factor, rotation, skew;
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
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);

    builder.reset();
    if (cmag)
    {
        builder.double_number("Center X", x_ctr)
            .double_number("Center Y", y_ctr)
            .double_number("Magnification", static_cast<double>(magnification))
            .double_number("X Magnification Factor", x_mag_factor)
            .double_number("Rotation Angle (degrees)", rotation)
            .double_number("Skew Angle (degrees)", skew)
            .comment("")
            .comment("Press F7 to switch to \"corners\" mode");
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
            .comment("Press F7 to switch to \"center-mag\" mode");
    }
    builder.comment("Press F4 to reset to type default values");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_SCREEN_COORDS};
        prompt_ret = builder.prompt("Screen Coordinates", 128 | 16);
    }

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

    if (prompt_ret == ID_KEY_F4)
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
            aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
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
        if (cmp_dbl(x_ctr, new_x_center)                          //
            || cmp_dbl(y_ctr, new_y_center)                       //
            || cmp_dbl((double) magnification, new_magnification) //
            || cmp_dbl(x_mag_factor, new_x_mag_factor)            //
            || cmp_dbl(rotation, new_rotation)                    //
            || cmp_dbl(skew, new_skew))
        {
            x_ctr          = new_x_center;
            y_ctr          = new_y_center;
            magnification = new_magnification;
            x_mag_factor    = new_x_mag_factor;
            rotation      = new_rotation;
            skew          = new_skew;
            if (x_mag_factor == 0)
            {
                x_mag_factor = 1;
            }
            cvt_corners(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
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

    if (prompt_ret == ID_KEY_F7)
    {
        // toggle corners/center-mag mode
        if (!g_use_center_mag)
        {
            cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
            g_use_center_mag = true;
        }
        else
        {
            g_use_center_mag = false;
        }
        goto gsc_loop;
    }

    if (!cmp_dbl(oxxmin, g_orbit_corner_min_x) && !cmp_dbl(oxxmax, g_orbit_corner_max_x) && !cmp_dbl(oyymin, g_orbit_corner_min_y) &&
            !cmp_dbl(oyymax, g_orbit_corner_max_y) && !cmp_dbl(oxx3rd, g_orbit_corner_3_x) && !cmp_dbl(oyy3rd, g_orbit_corner_3_y))
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
