// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_corners.h"

#include "engine/calc_frac_init.h"
#include "engine/convert_center_mag.h"
#include "engine/convert_corners.h"
#include "engine/ImageRegion.h"
#include "engine/sticky_orbits.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/double_to_string.h"
#include "ui/help.h"
#include "ui/id_keys.h"
#include "ui/zoom.h"

#include <config/port.h>

#include <cfloat>
#include <cmath>
#include <cstdlib>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

/*
   See if double value was changed by input screen. Problem is that the
   conversion from double to string and back can make small changes
   in the value, so will it twill test as "different" even though it
   is not
*/
static int cmp_dbl(double old, const double new_val)
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
    constexpr char x_prompt[] = "          X";
    constexpr char y_prompt[] = "          Y";
    double x_ctr;
    double y_ctr;
    LDouble magnification; // LDouble not really needed here, but used to match function parameters
    double x_mag_factor;
    double rotation;
    double skew;

    const bool old_use_center_mag = g_use_center_mag;
    double old_x_min = g_image_region.m_min.x;
    double old_x_max = g_image_region.m_max.x;
    double old_y_min = g_image_region.m_min.y;
    double old_y_max = g_image_region.m_max.y;
    double old_x_3rd = g_image_region.m_3rd.x;
    double old_y_3rd = g_image_region.m_3rd.y;

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
            .double_number("Magnification", static_cast<double>(magnification))
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
                .double_number(x_prompt, g_image_region.m_min.x)
                .double_number(y_prompt, g_image_region.m_max.y)
                .comment("Right End Point")
                .double_number(x_prompt, g_image_region.m_max.x)
                .double_number(y_prompt, g_image_region.m_min.y);
        }
        else
        {
            // 6 items
            builder.comment("Top-Left Corner")
                .double_number(x_prompt, g_image_region.m_min.x)
                .double_number(y_prompt, g_image_region.m_max.y)
                .comment("Bottom-Right Corner")
                .double_number(x_prompt, g_image_region.m_max.x)
                .double_number(y_prompt, g_image_region.m_min.y);
            if (g_image_region.m_min.x == g_image_region.m_3rd.x && g_image_region.m_min.y == g_image_region.m_3rd.y)
            {
                g_image_region.m_3rd.y = 0;
                g_image_region.m_3rd.x = 0;
            }
            // 4 items
            builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
                .double_number(x_prompt, g_image_region.m_3rd.x)
                .double_number(y_prompt, g_image_region.m_3rd.y)
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
        g_use_center_mag = old_use_center_mag;
        g_image_region.m_min.x = old_x_min;
        g_image_region.m_max.x = old_x_max;
        g_image_region.m_min.y = old_y_min;
        g_image_region.m_max.y = old_y_max;
        g_image_region.m_3rd.x = old_x_3rd;
        g_image_region.m_3rd.y = old_y_3rd;
        return -1;
    }

    if (prompt_ret == ID_KEY_F4)
    {
        // reset to type defaults
        g_image_region.m_min.x = g_cur_fractal_specific->x_min;
        g_image_region.m_3rd.x = g_image_region.m_min.x;
        g_image_region.m_max.x = g_cur_fractal_specific->x_max;
        g_image_region.m_min.y = g_cur_fractal_specific->y_min;
        g_image_region.m_3rd.y = g_image_region.m_min.y;
        g_image_region.m_max.y = g_cur_fractal_specific->y_max;
        if (g_viewport.crop && g_viewport.final_aspect_ratio != g_screen_aspect)
        {
            aspect_ratio_crop(g_screen_aspect, g_viewport.final_aspect_ratio);
        }
        if (g_bf_math != BFMathType::NONE)
        {
            fractal_float_to_bf();
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
            || cmp_dbl(static_cast<double>(magnification), new_magnification) //
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
            g_image_region.m_min.x = builder.read_double_number();
            g_image_region.m_max.y = builder.read_double_number();
            builder.read_comment();
            g_image_region.m_max.x = builder.read_double_number();
            g_image_region.m_min.y = builder.read_double_number();
        }
        else
        {
            builder.read_comment();
            g_image_region.m_min.x = builder.read_double_number();
            g_image_region.m_max.y = builder.read_double_number();
            builder.read_comment();
            g_image_region.m_max.x = builder.read_double_number();
            g_image_region.m_min.y = builder.read_double_number();
            builder.read_comment();
            g_image_region.m_3rd.x = builder.read_double_number();
            g_image_region.m_3rd.y = builder.read_double_number();
            if (g_image_region.m_3rd.x == 0 && g_image_region.m_3rd.y == 0)
            {
                g_image_region.m_3rd.x = g_image_region.m_min.x;
                g_image_region.m_3rd.y = g_image_region.m_min.y;
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

    if (!cmp_dbl(old_x_min, g_image_region.m_min.x) && !cmp_dbl(old_x_max, g_image_region.m_max.x) && !cmp_dbl(old_y_min, g_image_region.m_min.y) &&
            !cmp_dbl(old_y_max, g_image_region.m_max.y) && !cmp_dbl(old_x_3rd, g_image_region.m_3rd.x) && !cmp_dbl(old_y_3rd, g_image_region.m_3rd.y))
    {
        // no change, restore values to avoid drift
        g_image_region.m_min.x = old_x_min;
        g_image_region.m_max.x = old_x_max;
        g_image_region.m_min.y = old_y_min;
        g_image_region.m_max.y = old_y_max;
        g_image_region.m_3rd.x = old_x_3rd;
        g_image_region.m_3rd.y = old_y_3rd;
        return 0;
    }

    return 1;
}

int get_screen_corners()
{
    ChoiceBuilder<15> builder;
    constexpr char x_prompt[] = "          X";
    constexpr char y_prompt[] = "          Y";
    int prompt_ret;
    double x_ctr;
    double y_ctr;
    LDouble magnification; // LDouble not really needed here, but used to match function parameters
    double x_mag_factor;
    double rotation;
    double skew;

    const bool old_use_center_mag = g_use_center_mag;

    const ImageRegion save_region{g_image_region}; // save these for later since cvtcorners modifies them
                                                   // and we need to set them for cvtcentermag to work

    if (!g_set_orbit_corners && !g_keep_screen_coords)
    {
        g_orbit_corner = g_image_region;
    }

    const ImageRegion save_corner{g_orbit_corner};

    g_image_region = g_orbit_corner;

gsc_loop:
    const bool use_center_mag = g_use_center_mag;
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
            .double_number(x_prompt, g_orbit_corner.m_min.x)
            .double_number(y_prompt, g_orbit_corner.m_max.y)
            .comment("Bottom-Right Corner")
            .double_number(x_prompt, g_orbit_corner.m_max.x)
            .double_number(y_prompt, g_orbit_corner.m_min.y);
        if (g_orbit_corner.m_min.x == g_orbit_corner.m_3rd.x && g_orbit_corner.m_min.y == g_orbit_corner.m_3rd.y)
        {
            g_orbit_corner.m_3rd.y = 0;
            g_orbit_corner.m_3rd.x = 0;
        }
        builder.comment("Bottom-left (zeros for top-left X, bottom-right Y)")
            .double_number(x_prompt, g_orbit_corner.m_3rd.x)
            .double_number(y_prompt, g_orbit_corner.m_3rd.y)
            .comment("Press F7 to switch to \"center-mag\" mode");
    }
    builder.comment("Press F4 to reset to type default values");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_SCREEN_COORDS};
        prompt_ret = builder.prompt("Screen Coordinates", 128 | 16);
    }

    if (prompt_ret < 0)
    {
        // restore corners
        g_use_center_mag = old_use_center_mag;
        g_orbit_corner = save_corner;
        g_image_region = g_save_image_region;
        return -1;
    }

    if (prompt_ret == ID_KEY_F4)
    {
        // reset to type defaults
        g_orbit_corner.m_min.x = g_cur_fractal_specific->x_min;
        g_orbit_corner.m_min.y = g_cur_fractal_specific->y_min;
        g_orbit_corner.m_max.x = g_cur_fractal_specific->x_max;
        g_orbit_corner.m_max.y = g_cur_fractal_specific->y_max;
        g_orbit_corner.m_3rd = g_orbit_corner.m_min;
        g_image_region = g_orbit_corner;
        if (g_viewport.crop && g_viewport.final_aspect_ratio != g_screen_aspect)
        {
            aspect_ratio_crop(g_screen_aspect, g_viewport.final_aspect_ratio);
            g_orbit_corner = g_image_region;
        }
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
            || cmp_dbl(static_cast<double>(magnification), new_magnification) //
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
            g_orbit_corner = g_image_region;
        }
    }
    else
    {
        builder.read_comment();
        g_orbit_corner.m_min.x = builder.read_double_number();
        g_orbit_corner.m_max.y = builder.read_double_number();
        builder.read_comment();
        g_orbit_corner.m_max.x = builder.read_double_number();
        g_orbit_corner.m_min.y = builder.read_double_number();
        builder.read_comment();
        g_orbit_corner.m_3rd.x = builder.read_double_number();
        g_orbit_corner.m_3rd.y = builder.read_double_number();
        if (g_orbit_corner.m_3rd.x == 0.0 && g_orbit_corner.m_3rd.y == 0.0)
        {
            g_orbit_corner.m_3rd = g_orbit_corner.m_min;
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

    const auto cmp_cmplx = [](const DComplex &lhs, const DComplex &rhs)
    { return !cmp_dbl(lhs.x, rhs.x) && !cmp_dbl(lhs.y, rhs.y); };
    if (!cmp_cmplx(save_corner.m_min, g_orbit_corner.m_min) && //
        !cmp_cmplx(save_corner.m_max, g_orbit_corner.m_max) && //
        !cmp_cmplx(save_corner.m_3rd, g_orbit_corner.m_3rd))
    {
        // no change, restore values to avoid drift
        // restore corners
        g_orbit_corner = save_corner;
        g_image_region = save_region;
        return 0;
    }

    g_set_orbit_corners = true;
    g_keep_screen_coords = true;
    g_image_region = save_region;
    return 1;
}

} // namespace id::ui
