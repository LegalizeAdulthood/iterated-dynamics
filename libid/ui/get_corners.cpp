// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_corners.h"

#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/convert_center_mag.h"
#include "engine/convert_corners.h"
#include "engine/id_data.h"
#include "engine/sticky_orbits.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/double_to_string.h"
#include "ui/id_keys.h"
#include "ui/zoom.h"

#include <config/port.h>

#include <cfloat>
#include <cmath>
#include <cstdlib>

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

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
    return std::abs(old-new_val) < DBL_EPSILON ? 0 : 1; // zero if same
}

int get_corners()
{
    ChoiceBuilder<11> builder;
    char x_prompt[] = "          X";
    char y_prompt[] = "          Y";
    double x_ctr;
    double y_ctr;
    LDouble magnification; // LDouble not really needed here, but used to match function parameters
    double x_mag_factor;
    double rotation;
    double skew;

    const bool old_use_center_mag = g_use_center_mag;
    double old_x_min = g_x_min;
    double old_x_max = g_x_max;
    double old_y_min = g_y_min;
    double old_y_max = g_y_max;
    double old_x_3rd = g_x_3rd;
    double old_y_3rd = g_y_3rd;

gc_loop:
    int use_center_mag = g_use_center_mag ? 1 : 0;
    if (g_draw_mode == OrbitDrawMode::LINE)
    {
        use_center_mag = 0;
    }
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);

    builder.reset();
    // 10 items
    if (use_center_mag)
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
        if (g_draw_mode == OrbitDrawMode::LINE)
        {
            // 6 items
            builder.comment("Left End Point")
                .double_number(x_prompt, g_x_min)
                .double_number(y_prompt, g_y_max)
                .comment("Right End Point")
                .double_number(x_prompt, g_x_max)
                .double_number(y_prompt, g_y_min);
        }
        else
        {
            // 6 items
            builder.comment("Top-Left Corner")
                .double_number(x_prompt, g_x_min)
                .double_number(y_prompt, g_y_max)
                .comment("Bottom-Right Corner")
                .double_number(x_prompt, g_x_max)
                .double_number(y_prompt, g_y_min);
            if (g_x_min == g_x_3rd && g_y_min == g_y_3rd)
            {
                g_y_3rd = 0;
                g_x_3rd = 0;
            }
            // 4 items
            builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
                .double_number(x_prompt, g_x_3rd)
                .double_number(y_prompt, g_y_3rd)
                .comment("Press F7 to switch to \"center-mag\" mode");
        }
    }
    // 1 item
    builder.comment("Press F4 to reset to type default values");

    int prompt_ret;
    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_COORDS};
        prompt_ret = builder.prompt("Image Coordinates", 128 | 16);
    }

    if (prompt_ret < 0)
    {
        g_use_center_mag = old_use_center_mag;
        g_x_min = old_x_min;
        g_x_max = old_x_max;
        g_y_min = old_y_min;
        g_y_max = old_y_max;
        g_x_3rd = old_x_3rd;
        g_y_3rd = old_y_3rd;
        return -1;
    }

    if (prompt_ret == ID_KEY_F4)
    {
        // reset to type defaults
        g_x_min = g_cur_fractal_specific->x_min;
        g_x_3rd = g_x_min;
        g_x_max = g_cur_fractal_specific->x_max;
        g_y_min = g_cur_fractal_specific->y_min;
        g_y_3rd = g_y_min;
        g_y_max = g_cur_fractal_specific->y_max;
        if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
        {
            aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
        }
        if (g_bf_math != BFMathType::NONE)
        {
            id::fractal_float_to_bf();
        }
        goto gc_loop;
    }

    if (use_center_mag)
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
        if (g_draw_mode == OrbitDrawMode::LINE)
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

    if (prompt_ret == ID_KEY_F7 && g_draw_mode != OrbitDrawMode::LINE)
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

    if (!cmp_dbl(old_x_min, g_x_min) && !cmp_dbl(old_x_max, g_x_max) && !cmp_dbl(old_y_min, g_y_min) &&
            !cmp_dbl(old_y_max, g_y_max) && !cmp_dbl(old_x_3rd, g_x_3rd) && !cmp_dbl(old_y_3rd, g_y_3rd))
    {
        // no change, restore values to avoid drift
        g_x_min = old_x_min;
        g_x_max = old_x_max;
        g_y_min = old_y_min;
        g_y_max = old_y_max;
        g_x_3rd = old_x_3rd;
        g_y_3rd = old_y_3rd;
        return 0;
    }

    return 1;
}

int get_screen_corners()
{
    ChoiceBuilder<15> builder;
    char x_prompt[] = "          X";
    char y_prompt[] = "          Y";
    int prompt_ret;
    double x_ctr;
    double y_ctr;
    LDouble magnification; // LDouble not really needed here, but used to match function parameters
    double x_mag_factor;
    double rotation;
    double skew;

    const bool old_use_center_mag = g_use_center_mag;

    double save_x_min = g_x_min;  // save these for later since cvtcorners modifies them
    double save_x_max = g_x_max;  // and we need to set them for cvtcentermag to work
    double save_x_3rd = g_x_3rd;
    double save_y_min = g_y_min;
    double save_y_max = g_y_max;
    double save_y_3rd = g_y_3rd;

    if (!g_set_orbit_corners && !g_keep_screen_coords)
    {
        g_orbit_corner_min_x = g_x_min;
        g_orbit_corner_max_x = g_x_max;
        g_orbit_corner_3rd_x = g_x_3rd;
        g_orbit_corner_min_y = g_y_min;
        g_orbit_corner_max_y = g_y_max;
        g_orbit_corner_3rd_y = g_y_3rd;
    }

    double old_x_min = g_orbit_corner_min_x;
    double old_x_max = g_orbit_corner_max_x;
    double old_y_min = g_orbit_corner_min_y;
    double old_y_max = g_orbit_corner_max_y;
    double old_x_3rd = g_orbit_corner_3rd_x;
    double old_y_3rd = g_orbit_corner_3rd_y;

    g_x_min = g_orbit_corner_min_x;
    g_x_max = g_orbit_corner_max_x;
    g_y_min = g_orbit_corner_min_y;
    g_y_max = g_orbit_corner_max_y;
    g_x_3rd = g_orbit_corner_3rd_x;
    g_y_3rd = g_orbit_corner_3rd_y;

gsc_loop:
    int use_center_mag = g_use_center_mag ? 1 : 0;
    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);

    builder.reset();
    if (use_center_mag)
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
            .double_number(x_prompt, g_orbit_corner_min_x)
            .double_number(y_prompt, g_orbit_corner_max_y)
            .comment("Bottom-Right Corner")
            .double_number(x_prompt, g_orbit_corner_max_x)
            .double_number(y_prompt, g_orbit_corner_min_y);
        if (g_orbit_corner_min_x == g_orbit_corner_3rd_x && g_orbit_corner_min_y == g_orbit_corner_3rd_y)
        {
            g_orbit_corner_3rd_y = 0;
            g_orbit_corner_3rd_x = 0;
        }
        builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
            .double_number(x_prompt, g_orbit_corner_3rd_x)
            .double_number(y_prompt, g_orbit_corner_3rd_y)
            .comment("Press F7 to switch to \"center-mag\" mode");
    }
    builder.comment("Press F4 to reset to type default values");

    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_SCREEN_COORDS};
        prompt_ret = builder.prompt("Screen Coordinates", 128 | 16);
    }

    if (prompt_ret < 0)
    {
        g_use_center_mag = old_use_center_mag;
        g_orbit_corner_min_x = old_x_min;
        g_orbit_corner_max_x = old_x_max;
        g_orbit_corner_min_y = old_y_min;
        g_orbit_corner_max_y = old_y_max;
        g_orbit_corner_3rd_x = old_x_3rd;
        g_orbit_corner_3rd_y = old_y_3rd;
        // restore corners
        g_x_min = save_x_min;
        g_x_max = save_x_max;
        g_y_min = save_y_min;
        g_y_max = save_y_max;
        g_x_3rd = save_x_3rd;
        g_y_3rd = save_y_3rd;
        return -1;
    }

    if (prompt_ret == ID_KEY_F4)
    {
        // reset to type defaults
        g_orbit_corner_min_x = g_cur_fractal_specific->x_min;
        g_orbit_corner_3rd_x = g_orbit_corner_min_x;
        g_orbit_corner_max_x = g_cur_fractal_specific->x_max;
        g_orbit_corner_min_y = g_cur_fractal_specific->y_min;
        g_orbit_corner_3rd_y = g_orbit_corner_min_y;
        g_orbit_corner_max_y = g_cur_fractal_specific->y_max;
        g_x_min = g_orbit_corner_min_x;
        g_x_max = g_orbit_corner_max_x;
        g_y_min = g_orbit_corner_min_y;
        g_y_max = g_orbit_corner_max_y;
        g_x_3rd = g_orbit_corner_3rd_x;
        g_y_3rd = g_orbit_corner_3rd_y;
        if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
        {
            aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
        }

        g_orbit_corner_min_x = g_x_min;
        g_orbit_corner_max_x = g_x_max;
        g_orbit_corner_min_y = g_y_min;
        g_orbit_corner_max_y = g_y_max;
        g_orbit_corner_3rd_x = g_x_min;
        g_orbit_corner_3rd_y = g_y_min;
        goto gsc_loop;
    }

    if (use_center_mag)
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
            g_orbit_corner_3rd_x = g_x_3rd;
            g_orbit_corner_3rd_y = g_y_3rd;
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
        g_orbit_corner_3rd_x = builder.read_double_number();
        g_orbit_corner_3rd_y = builder.read_double_number();
        if (g_orbit_corner_3rd_x == 0 && g_orbit_corner_3rd_y == 0)
        {
            g_orbit_corner_3rd_x = g_orbit_corner_min_x;
            g_orbit_corner_3rd_y = g_orbit_corner_min_y;
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

    if (!cmp_dbl(old_x_min, g_orbit_corner_min_x) && !cmp_dbl(old_x_max, g_orbit_corner_max_x) &&
        !cmp_dbl(old_y_min, g_orbit_corner_min_y) && !cmp_dbl(old_y_max, g_orbit_corner_max_y) &&
        !cmp_dbl(old_x_3rd, g_orbit_corner_3rd_x) && !cmp_dbl(old_y_3rd, g_orbit_corner_3rd_y))
    {
        // no change, restore values to avoid drift
        g_orbit_corner_min_x = old_x_min;
        g_orbit_corner_max_x = old_x_max;
        g_orbit_corner_min_y = old_y_min;
        g_orbit_corner_max_y = old_y_max;
        g_orbit_corner_3rd_x = old_x_3rd;
        g_orbit_corner_3rd_y = old_y_3rd;
        // restore corners
        g_x_min = save_x_min;
        g_x_max = save_x_max;
        g_y_min = save_y_min;
        g_y_max = save_y_max;
        g_x_3rd = save_x_3rd;
        g_y_3rd = save_y_3rd;
        return 0;
    }
    g_set_orbit_corners = true;
    g_keep_screen_coords = true;
    // restore corners
    g_x_min = save_x_min;
    g_x_max = save_x_max;
    g_y_min = save_y_min;
    g_y_max = save_y_max;
    g_x_3rd = save_x_3rd;
    g_y_3rd = save_y_3rd;
    return 1;
}

} // namespace id::ui
