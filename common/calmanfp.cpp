/* calmanfp.c
 * This file contains routines to replace calmanfp.asm.
 *
 * This file Copyright 1992 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include <float.h>
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

extern int atan_colors;
extern long firstsavedand;
extern int g_periodicity_next_saved_incr;

static int inside_color, periodicity_color;

void calcmandfpasmstart()
{
    inside_color = (g_inside_color < COLOR_BLACK) ? g_max_iterations : g_inside_color;
    periodicity_color = (periodicitycheck < 0) ? 7 : inside_color;
    g_old_color_iter = 0;
}

#define ABS(x) ((x) < 0?-(x):(x))

/* If USE_NEW is 1, the magnitude is used for periodicity checking instead
   of the x and y values.  This is experimental. */
#define USE_NEW 0

long calcmandfpasm()
{
    long cx;
    long savedand;
    int savedincr;
    long tmpfsd;
#if USE_NEW
    double x, y, x2, y2, xy, Cx, Cy, savedmag;
#else
    double x, y, x2, y2, xy, Cx, Cy, savedx, savedy;
#endif

    if (periodicitycheck == 0)
    {
        g_old_color_iter = 0;      // don't check periodicity
    }
    else if (reset_periodicity)
    {
        g_old_color_iter = g_max_iterations - 255;
    }

    tmpfsd = g_max_iterations - firstsavedand;
    if (g_old_color_iter > tmpfsd) // this defeats checking periodicity immediately
    {
        g_old_color_iter = tmpfsd; // but matches the code in standard_fractal()
    }

    // initparms
#if USE_NEW
    savedmag = 0;
#else
    savedx = 0;
    savedy = 0;
#endif
    g_orbit_save_index = 0;
    savedand = firstsavedand;
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
                show_orbit = !show_orbit;
            }
            else
            {
                g_color_iter = -1;
                return -1;
            }
        }
    }

    cx = g_max_iterations;
    if (fractype != fractal_type::JULIAFP && fractype != fractal_type::JULIA)
    {
        // Mandelbrot_87
        Cx = g_init.x;
        Cy = g_init.y;
        x = parm.x+Cx;
        y = parm.y+Cy;
    }
    else
    {
        // dojulia_87
        Cx = parm.x;
        Cy = parm.y;
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

        if (g_magnitude >= rqlim)
        {
            goto over_bailout_87;
        }

        // no_save_new_xy_87
        if (cx < g_old_color_iter)  // check periodicity
        {
            if (((g_max_iterations - cx) & savedand) == 0)
            {
#if USE_NEW
                savedmag = magnitude;
#else
                savedx = x;
                savedy = y;
#endif
                savedincr--;
                if (savedincr == 0)
                {
                    savedand = (savedand << 1) + 1;
                    savedincr = g_periodicity_next_saved_incr;
                }
            }
            else
            {
#if USE_NEW
                if (ABS(magnitude-savedmag) < g_close_enough)
                {
#else
                if (ABS(savedx-x) < g_close_enough && ABS(savedy-y) < g_close_enough)
                {
#endif
                    //          oldcoloriter = 65535;
                    g_old_color_iter = g_max_iterations;
                    realcoloriter = g_max_iterations;
                    g_keyboard_check_interval = g_keyboard_check_interval -(g_max_iterations-cx);
                    g_color_iter = periodicity_color;
                    goto pop_stack;
                }
            }
        }
        // no_periodicity_check_87
        if (show_orbit)
        {
            plot_orbit(x, y, -1);
        }
        // no_show_orbit_87
    } // while (--cx > 0)

    // reached maxit
    // check periodicity immediately next time, remember we count down from maxit
    g_old_color_iter = g_max_iterations;
    g_keyboard_check_interval -= g_max_iterations;
    realcoloriter = g_max_iterations;
    g_color_iter = inside_color;

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
    realcoloriter = g_max_iterations-cx;
    g_color_iter = realcoloriter;
    if (g_color_iter == 0)
    {
        g_color_iter = 1;
    }
    g_keyboard_check_interval -= realcoloriter;
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
            g_color_iter = (long) fabs(atan2(g_new_z.y, g_new_z.x)*atan_colors/PI);
        }
        // check_color
        if ((g_color_iter <= 0 || g_color_iter > g_max_iterations) && g_outside_color != FMOD)
        {
            if (save_release < 1961)
            {
                g_color_iter = 0;
            }
            else
            {
                g_color_iter = 1;
            }
        }
    }

    goto pop_stack;
}
