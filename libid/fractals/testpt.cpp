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
#include "fractals/testpt.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "engine/resume.h"
#include "misc/Driver.h"

// standalone engine for "test"
int test()
{
    int start_pass = 0;
    int start_row = start_pass;
    if (g_resuming)
    {
        start_resume();
        get_resume(start_row, start_pass);
        end_resume();
    }
    if (test_start())   // assume it was stand-alone, doesn't want passes logic
    {
        return 0;
    }
    int num_passes = (g_std_calc_mode == '1') ? 0 : 1;
    for (int passes = start_pass; passes <= num_passes ; passes++)
    {
        for (g_row = start_row; g_row <= g_i_y_stop; g_row = g_row+1+num_passes)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)       // look at each point on screen
            {
                g_init.x = g_dx_pixel();
                g_init.y = g_dy_pixel();
                if (driver_key_pressed())
                {
                    test_end();
                    alloc_resume(20, 1);
                    put_resume(g_row, passes);
                    return -1;
                }
                int color =
                    test_pt(g_init.x, g_init.y, g_param_z1.x, g_param_z1.y, g_max_iterations, g_inside_color);
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
                if (num_passes && (passes == 0))
                {
                    (*g_plot)(g_col, g_row+1, color);
                }
            }
        }
        start_row = passes + 1;
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
int test_pt(double init_real, double init_imag, double param1, double param2, long max_iter, int inside)
{
    double old_real = param1;
    double old_imag = param2;
    double magnitude = 0.0;
    long color = 0;
    while ((magnitude < 4.0) && (color < max_iter))
    {
        double new_real = old_real * old_real - old_imag * old_imag + init_real;
        double new_imag = 2 * old_real * old_imag + init_imag;
        color++;
        old_real = new_real;
        old_imag = new_imag;
        magnitude = new_real * new_real + new_imag * new_imag;
    }
    if (color >= max_iter)
    {
        color = inside;
    }
    return (int)color;
}
