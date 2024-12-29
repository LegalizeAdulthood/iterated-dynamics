// SPDX-License-Identifier: GPL-3.0-only
//
/*

Write your fractal program here. initreal and initimag are the values in
the complex plane; parm1, and parm2 are paramaters to be entered with the
"params=" option (if needed). The function should return the color associated
with initreal and initimag.  Id will repeatedly call your function with
the values of initreal and initimag ranging over the rectangle defined by the
"corners=" option. Assuming your formula is iterative, "maxit" is the maximum
iteration. If "maxit" is hit, color "inside" should be returned.

Note that this routine could be sped up using external variables/arrays
rather than the current parameter-passing scheme.  The goal, however was
to make it as easy as possible to add fractal types, and this looked like
the easiest way.

This module is part of an overlay, with calcfrac.c.  The routines in it
must not be called by any part of Id other than calcfrac.

The sample code below is a straightforward Mandelbrot routine.

*/
#include "testpt.h"

#include "calcfrac.h"
#include "drivers.h"
#include "fractals.h"
#include "id_data.h"
#include "pixel_grid.h"
#include "resume.h"

// standalone engine for "test"
int test()
{
    int startpass = 0;
    int startrow = startpass;
    if (g_resuming)
    {
        start_resume();
        get_resume(startrow, startpass);
        end_resume();
    }
    if (test_start())   // assume it was stand-alone, doesn't want passes logic
    {
        return 0;
    }
    int numpasses = (g_std_calc_mode == '1') ? 0 : 1;
    for (int passes = startpass; passes <= numpasses ; passes++)
    {
        for (g_row = startrow; g_row <= g_i_y_stop; g_row = g_row+1+numpasses)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)       // look at each point on screen
            {
                int color;
                g_init.x = g_dx_pixel();
                g_init.y = g_dy_pixel();
                if (driver_key_pressed())
                {
                    test_end();
                    alloc_resume(20, 1);
                    put_resume(g_row, passes);
                    return -1;
                }
                color = test_pt(g_init.x, g_init.y, g_param_z1.x, g_param_z1.y, g_max_iterations, g_inside_color);
                if (color >= g_colors)
                {
                    // avoid trouble if color is 0
                    if (g_colors < 16)
                    {
                        color &= g_and_color;
                    }
                    else
                    {
                        color = ((color-1) % g_and_color) + 1; // skip color zero
                    }
                }
                (*g_plot)(g_col, g_row, color);
                if (numpasses && (passes == 0))
                {
                    (*g_plot)(g_col, g_row+1, color);
                }
            }
        }
        startrow = passes + 1;
    }
    test_end();
    return 0;
}

int test_start()     // this routine is called just before the fractal starts
{
    return 0;
}

void test_end()       // this routine is called just after the fractal ends
{
}

// this routine is called once for every pixel
// (note: possibly using the dual-pass / solid-guessing options
int test_pt(double initreal, double initimag, double parm1, double parm2, long maxit, int inside)
{
    double oldreal;
    double oldimag;
    double newreal;
    double newimag;
    double magnitude;
    long color;
    oldreal = parm1;
    oldimag = parm2;
    magnitude = 0.0;
    color = 0;
    while ((magnitude < 4.0) && (color < maxit))
    {
        newreal = oldreal * oldreal - oldimag * oldimag + initreal;
        newimag = 2 * oldreal * oldimag + initimag;
        color++;
        oldreal = newreal;
        oldimag = newimag;
        magnitude = newreal * newreal + newimag * newimag;
    }
    if (color >= maxit)
    {
        color = inside;
    }
    return (int)color;
}
