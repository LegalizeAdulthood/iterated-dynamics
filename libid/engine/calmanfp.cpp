// SPDX-License-Identifier: GPL-3.0-only
//
// This file contains routines to replace calmanfp.asm.
//
// This file Copyright 1992 Ken Shirriff.  It may be used according to the
// fractint license conditions, blah blah blah.
//
#include "engine/calmanfp.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/orbit.h"
#include "fractals/fractype.h"
#include "misc/id.h"

#include <algorithm>
#include <cmath>

using namespace id::engine;
using namespace id::fractals;

namespace id
{

static int s_inside_color{};
static int s_periodicity_color{};

void calc_mandelbrot_init()
{
    s_inside_color = (g_inside_color < COLOR_BLACK) ? g_max_iterations : g_inside_color;
    s_periodicity_color = (g_periodicity_check < 0) ? 7 : s_inside_color;
    g_old_color_iter = 0;
}

long mandelbrot_orbit()
{
    double x;
    double y;
    double x2;
    double y2;
    double xy;
    double c_x;
    double c_y;

    if (g_periodicity_check == 0)
    {
        g_old_color_iter = 0;      // don't check periodicity
    }
    else if (g_reset_periodicity)
    {
        g_old_color_iter = g_max_iterations - 255;
    }

    long tmp_fsd = g_max_iterations - g_first_saved_and;
    // this defeats checking periodicity immediately
    // but matches the code in standard_fractal()
    g_old_color_iter = std::min(g_old_color_iter, tmp_fsd);

    // initparms
    double saved_x = 0;
    double saved_y = 0;
    g_orbit_save_index = 0;
    long saved_and = g_first_saved_and;
    int saved_incr = 1;             // start checking the very first time

    long cx = g_max_iterations;
    if (g_fractal_type != FractalType::JULIA)
    {
        // Mandelbrot_87
        c_x = g_init.x;
        c_y = g_init.y;
        x = g_param_z1.x+c_x;
        y = g_param_z1.y+c_y;
    }
    else
    {
        // dojulia_87
        c_x = g_param_z1.x;
        c_y = g_param_z1.y;
        x = g_init.x;
        y = g_init.y;
        x2 = x*x;
        y2 = y*y;
        xy = x*y;
        x = x2-y2+c_x;
        y = 2*xy+c_y;
    }
    x2 = x*x;
    y2 = y*y;
    xy = x*y;

    // top_of_cs_loop_87
    while (--cx > 0)
    {
        x = x2-y2+c_x;
        y = 2*xy+c_y;
        x2 = x*x;
        y2 = y*y;
        xy = x*y;
        g_magnitude = x2+y2;

        if (g_magnitude >= g_magnitude_limit)
        {
            goto over_bailout_87;
        }

        // no_save_new_xy_87
        if (cx < g_old_color_iter)  // check periodicity
        {
            if (((g_max_iterations - cx) & saved_and) == 0)
            {
                saved_x = x;
                saved_y = y;
                saved_incr--;
                if (saved_incr == 0)
                {
                    saved_and = (saved_and << 1) + 1;
                    saved_incr = g_periodicity_next_saved_incr;
                }
            }
            else
            {
                if (std::abs(saved_x-x) < g_close_enough && std::abs(saved_y-y) < g_close_enough)
                {
                    //          oldcoloriter = 65535;
                    g_old_color_iter = g_max_iterations;
                    g_real_color_iter = g_max_iterations;
                    g_keyboard_check_interval = g_keyboard_check_interval -(g_max_iterations-cx);
                    g_color_iter = s_periodicity_color;
                    goto pop_stack;
                }
            }
        }
        // no_periodicity_check_87
        if (g_show_orbit)
        {
            plot_orbit(x, y, -1);
        }
        // no_show_orbit_87
    } // while (--cx > 0)

    // reached maxit
    // check periodicity immediately next time, remember we count down from maxit
    g_old_color_iter = g_max_iterations;
    g_keyboard_check_interval -= g_max_iterations;
    g_real_color_iter = g_max_iterations;
    g_color_iter = s_inside_color;

pop_stack:
    if (g_orbit_save_index)
    {
        scrub_orbit();
    }
    return g_color_iter;

over_bailout_87:
    if (g_outside_color <= REAL)
    {
        g_new_z.x = x;
        g_new_z.y = y;
    }
    if (cx-10 > 0)
    {
        g_old_color_iter = cx-10;
    }
    else
    {
        g_old_color_iter = 0;
    }
    g_real_color_iter = g_max_iterations-cx;
    g_color_iter = g_real_color_iter;
    if (g_color_iter == 0)
    {
        g_color_iter = 1;
    }
    g_keyboard_check_interval -= g_real_color_iter;
    if (g_outside_color == ITER)
    {
    }
    else if (g_outside_color > REAL)
    {
        g_color_iter = g_outside_color;
    }
    else
    {
        // special_outside
        if (g_outside_color == REAL)
        {
            g_color_iter += (long) g_new_z.x + 7;
        }
        else if (g_outside_color == IMAG)
        {
            g_color_iter += (long) g_new_z.y + 7;
        }
        else if (g_outside_color == MULT && g_new_z.y != 0.0)
        {
            g_color_iter = (long)((double) g_color_iter * (g_new_z.x/g_new_z.y));
        }
        else if (g_outside_color == SUM)
        {
            g_color_iter += (long)(g_new_z.x + g_new_z.y);
        }
        else if (g_outside_color == ATAN)
        {
            g_color_iter = (long) std::abs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
        }
        // check_color
        if ((g_color_iter <= 0 || g_color_iter > g_max_iterations) && g_outside_color != FMOD)
        {
            g_color_iter = 1;
        }
    }

    goto pop_stack;
}

} // namespace id
