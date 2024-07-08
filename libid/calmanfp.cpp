// This file contains routines to replace calmanfp.asm.
//
// This file Copyright 1992 Ken Shirriff.  It may be used according to the
// fractint license conditions, blah blah blah.
//
#include "calmanfp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fractals.h"
#include "fractype.h"
#include "id.h"
#include "id_data.h"
#include "orbit.h"
#include "port.h"

#include <cmath>

static int s_inside_color{};
static int s_periodicity_color{};

void calcmandfpasmstart()
{
    s_inside_color = (g_inside_color < COLOR_BLACK) ? g_max_iterations : g_inside_color;
    s_periodicity_color = (g_periodicity_check < 0) ? 7 : s_inside_color;
    g_old_color_iter = 0;
}

long calcmandfpasm()
{
    long cx;
    long savedand;
    int savedincr;
    long tmpfsd;
    double x;
    double y;
    double x2;
    double y2;
    double xy;
    double Cx;
    double Cy;
    double savedx;
    double savedy;

    if (g_periodicity_check == 0)
    {
        g_old_color_iter = 0;      // don't check periodicity
    }
    else if (g_reset_periodicity)
    {
        g_old_color_iter = g_max_iterations - 255;
    }

    tmpfsd = g_max_iterations - g_first_saved_and;
    if (g_old_color_iter > tmpfsd) // this defeats checking periodicity immediately
    {
        g_old_color_iter = tmpfsd; // but matches the code in standard_fractal()
    }

    // initparms
    savedx = 0;
    savedy = 0;
    g_orbit_save_index = 0;
    savedand = g_first_saved_and;
    savedincr = 1;             // start checking the very first time
    g_keyboard_check_interval--;                // Only check the keyboard sometimes
    if (g_keyboard_check_interval < 0)
    {
        int key;
        g_keyboard_check_interval = 1000;
        key = driver_key_pressed();
        if (key)
        {
            if (key == 'o' || key == 'O')
            {
                driver_get_key();
                g_show_orbit = !g_show_orbit;
            }
            else
            {
                g_color_iter = -1;
                return -1;
            }
        }
    }

    cx = g_max_iterations;
    if (g_fractal_type != fractal_type::JULIAFP && g_fractal_type != fractal_type::JULIA)
    {
        // Mandelbrot_87
        Cx = g_init.x;
        Cy = g_init.y;
        x = g_param_z1.x+Cx;
        y = g_param_z1.y+Cy;
    }
    else
    {
        // dojulia_87
        Cx = g_param_z1.x;
        Cy = g_param_z1.y;
        x = g_init.x;
        y = g_init.y;
        x2 = x*x;
        y2 = y*y;
        xy = x*y;
        x = x2-y2+Cx;
        y = 2*xy+Cy;
    }
    x2 = x*x;
    y2 = y*y;
    xy = x*y;

    // top_of_cs_loop_87
    while (--cx > 0)
    {
        x = x2-y2+Cx;
        y = 2*xy+Cy;
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
            if (((g_max_iterations - cx) & savedand) == 0)
            {
                savedx = x;
                savedy = y;
                savedincr--;
                if (savedincr == 0)
                {
                    savedand = (savedand << 1) + 1;
                    savedincr = g_periodicity_next_saved_incr;
                }
            }
            else
            {
                if (std::abs(savedx-x) < g_close_enough && std::abs(savedy-y) < g_close_enough)
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
            g_color_iter = (long) std::fabs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
        }
        // check_color
        if ((g_color_iter <= 0 || g_color_iter > g_max_iterations) && g_outside_color != FMOD)
        {
            g_color_iter = 1;
        }
    }

    goto pop_stack;
}
