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
extern int nextsavedincr;

static int inside_color, periodicity_color;

void calcmandfpasmstart()
{
    inside_color = (g_inside < COLOR_BLACK) ? maxit : g_inside;
    periodicity_color = (periodicitycheck < 0) ? 7 : inside_color;
    oldcoloriter = 0;
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
        oldcoloriter = 0;      // don't check periodicity
    }
    else if (reset_periodicity)
    {
        oldcoloriter = maxit - 255;
    }

    tmpfsd = maxit - firstsavedand;
    if (oldcoloriter > tmpfsd) // this defeats checking periodicity immediately
    {
        oldcoloriter = tmpfsd; // but matches the code in standard_fractal()
    }

    // initparms
#if USE_NEW
    savedmag = 0;
#else
    savedx = 0;
    savedy = 0;
#endif
    orbit_ptr = 0;
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

    cx = maxit;
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
        if (cx < oldcoloriter)  // check periodicity
        {
            if (((maxit - cx) & savedand) == 0)
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
                    savedincr = nextsavedincr;
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
                    oldcoloriter = maxit;
                    realcoloriter = maxit;
                    g_keyboard_check_interval = g_keyboard_check_interval -(maxit-cx);
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
    oldcoloriter = maxit;
    g_keyboard_check_interval -= maxit;
    realcoloriter = maxit;
    g_color_iter = inside_color;

pop_stack:
    if (orbit_ptr)
    {
        scrub_orbit();
    }
    return g_color_iter;

over_bailout_87:
    if (outside <= REAL)
    {
        g_new.x = x;
        g_new.y = y;
    }
    if (cx-10 > 0)
    {
        oldcoloriter = cx-10;
    }
    else
    {
        oldcoloriter = 0;
    }
    realcoloriter = maxit-cx;
    g_color_iter = realcoloriter;
    if (g_color_iter == 0)
    {
        g_color_iter = 1;
    }
    g_keyboard_check_interval -= realcoloriter;
    if (outside == ITER)
    {
    }
    else if (outside > REAL)
    {
        g_color_iter = outside;
    }
    else
    {
        // special_outside
        if (outside == REAL)
        {
            g_color_iter += (long) g_new.x + 7;
        }
        else if (outside == IMAG)
        {
            g_color_iter += (long) g_new.y + 7;
        }
        else if (outside == MULT && g_new.y != 0.0)
        {
            g_color_iter = (long)((double) g_color_iter * (g_new.x/g_new.y));
        }
        else if (outside == SUM)
        {
            g_color_iter += (long)(g_new.x + g_new.y);
        }
        else if (outside == ATAN)
        {
            g_color_iter = (long) fabs(atan2(g_new.y, g_new.x)*atan_colors/PI);
        }
        // check_color
        if ((g_color_iter <= 0 || g_color_iter > maxit) && outside != FMOD)
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
