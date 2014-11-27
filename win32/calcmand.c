/* calcmand.c
 * This file contains routines to replace calcmand.asm.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "externs.h"
#include "drivers.h"

#define FUDGE_FACTOR_BITS 29
#define FUDGE_FACTOR ((1L << FUDGE_FACTOR_BITS)-1)
#define FUDGE_MUL(x_,y_) MulDiv(x_, y_, FUDGE_FACTOR)

#define KEYPRESSDELAY 32767
#define ABS(x) ((x)<0?-(x):(x))

extern unsigned long lm;
extern int atan_colors;
extern long firstsavedand;

static int inside_color;
static int periodicity_color;

static unsigned long savedmask = 0;

static int x = 0;
static int y = 0;
static int savedx = 0;
static int savedy = 0;
static int k = 0;
static int savedand = 0;
static int savedincr = 0;
static int period = 0;
static long firstsavedand = 0;

long cdecl
calcmandasm(void)
{
    static int been_here = 0;
    if (!been_here)
    {
        stopmsg(0, "This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = 1;
    }
    return 0;
}

static long cdecl
calc_mand_floating_point(void)
{
    long cx;
    long savedand;
    int savedincr;
    long tmpfsd;
    long x,y,x2, y2, xy, Cx, Cy, savedx, savedy;

    oldcoloriter = (periodicitycheck == 0) ? 0 : (maxit - 250);
    tmpfsd = maxit - firstsavedand;
    if (oldcoloriter > tmpfsd)
    {
        oldcoloriter = tmpfsd;
    }

    savedx = 0;
    savedy = 0;
    orbit_ptr = 0;
    savedand = firstsavedand;
    savedincr = 1;             /* start checking the very first time */
    kbdcount--;                /* Only check the keyboard sometimes */
    if (kbdcount < 0)
    {
        int key;
        kbdcount = 1000;
        key = driver_key_pressed();
        if (key)
        {
            if (key=='o' || key=='O')
            {
                driver_get_key();
                show_orbit = 1-show_orbit;
            }
            else
            {
                coloriter = -1;
                return -1;
            }
        }
    }

    cx = maxit;
    if (fractype != JULIA)
    {
        /* Mandelbrot_87 */
        Cx = linitx;
        Cy = linity;
        x = lparm.x+Cx;
        y = lparm.y+Cy;
    }
    else
    {
        /* dojulia_87 */
        Cx = lparm.x;
        Cy = lparm.y;
        x = linitx;
        y = linity;
        x2 = FUDGE_MUL(x, x);
        y2 = FUDGE_MUL(y, y);
        xy = FUDGE_MUL(x, y);
        x = x2-y2+Cx;
        y = 2*xy+Cy;
    }
    x2 = FUDGE_MUL(x, x);
    y2 = FUDGE_MUL(y, y);
    xy = FUDGE_MUL(x, y);

    /* top_of_cs_loop_87 */
    while (--cx > 0)
    {
        x = x2-y2+Cx;
        y = 2*xy+Cy;
        x2 = FUDGE_MUL(x, x);
        y2 = FUDGE_MUL(y, y);
        xy = FUDGE_MUL(x, y);
        magnitude = x2+y2;

        if (magnitude >= lm)
        {
            if (outside <= -2)
            {
                lnew.x = x;
                lnew.y = y;
            }
            if (cx - 10 > 0)
            {
                oldcoloriter = cx - 10;
            }
            else
            {
                oldcoloriter = 0;
            }
            realcoloriter = maxit - cx;
            coloriter = realcoloriter;

            if (coloriter == 0)
            {
                coloriter = 1;
            }
            kbdcount -= realcoloriter;
            if (outside == -1)
            {
            }
            else if (outside > -2)
            {
                coloriter = outside;
            }
            else
            {
                /* special_outside */
                if (outside == REAL)
                {
                    coloriter += lnew.x + 7;
                }
                else if (outside == IMAG)
                {
                    coloriter += lnew.y + 7;
                }
                else if (outside == MULT && lnew.y != 0)
                {
                    coloriter = FUDGE_MUL(coloriter, lnew.x) / lnew.y;
                }
                else if (outside == SUM)
                {
                    coloriter += lnew.x + lnew.y;
                }
                else if (outside == ATAN)
                {
                    coloriter = (long) fabs(atan2(lnew.y, lnew.x)*atan_colors/PI);
                }
                /* check_color */
                if ((coloriter <= 0 || coloriter > maxit) && outside != FMOD)
                {
                    coloriter = (save_release < 1961) ? 0 : 1;
                }
            }

            goto pop_stack;
        }

        /* no_save_new_xy_87 */
        if (cx < oldcoloriter)   /* check periodicity */
        {
            if (((maxit - cx) & savedand) == 0)
            {
                savedx = x;
                savedy = y;
                savedincr--;
                if (savedincr == 0)
                {
                    savedand = (savedand << 1) + 1;
                    savedincr = nextsavedincr;
                }
            }
            else if (ABS(savedx-x) < lclosenuff && ABS(savedy-y) < lclosenuff)
            {
                /* oldcoloriter = 65535;  */
                oldcoloriter = maxit;
                realcoloriter = maxit;
                kbdcount = kbdcount - (maxit - cx);
                coloriter = periodicity_color;
                goto pop_stack;
            }
        }
        /* no_periodicity_check_87 */
        if (show_orbit != 0)
        {
            plot_orbit(x, y, -1);
        }
        /* no_show_orbit_87 */
    } /* while (--cx > 0) */

    /* reached maxit */
    /* oldcoloriter = 65535;  */
    /* check periodicity immediately next time, remember we count down from maxit */
    oldcoloriter = maxit;
    kbdcount -= maxit;
    realcoloriter = maxit;

    coloriter = inside_color;

pop_stack:
    if (orbit_ptr)
    {
        scrub_orbit();
    }

    return coloriter;
}
