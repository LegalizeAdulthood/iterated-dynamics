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
#include "fractals/TestPoint.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "engine/resume.h"

using namespace id::engine;

namespace id::fractals
{

TestPoint::TestPoint() :
    num_passes(g_std_calc_mode == CalcMode::ONE_PASS ? 0 : 1),
    passes(start_pass)
{
    g_col = 0;
    g_row = start_row;
}

void TestPoint::resume()
{
    start_resume();
    get_resume(start_row, start_pass);
    passes = start_pass;
    g_row = start_row;
    end_resume();
}

void TestPoint::suspend()
{
    finish();
    alloc_resume(20, 1);
    put_resume(g_row, passes);
}

bool TestPoint::done()
{
    return passes > num_passes;
}

void TestPoint::iterate()
{
    if (g_row <= g_i_stop_pt.y)
    {
        if (g_col <= g_i_stop_pt.x) // look at each point on screen
        {
            g_init.x = dx_pixel();
            g_init.y = dy_pixel();
            int color =
                per_pixel(g_init.x, g_init.y, g_param_z1.x, g_param_z1.y, g_max_iterations, g_inside_color);
            if (color >= g_colors)
            {
                // avoid trouble if color is 0
                if (g_colors < 16)
                {
                    color &= g_and_color;
                }
                else
                {
                    color = (color - 1) % g_and_color + 1; // skip color zero
                }
            }
            g_plot(g_col, g_row, color);
            if (num_passes && passes == 0)
            {
                g_plot(g_col, g_row + 1, color);
            }
            ++g_col;
        }
        else
        {
            g_col = 0;
            g_row += 1 + num_passes;
        }
    }
    else
    {
        start_row = passes + 1;
        g_row = start_row;
        passes++;
    }
}


// this routine is called once for every pixel
// (note: possibly using the dual-pass / solid-guessing options
int TestPoint::per_pixel(const double init_real, const double init_imag, //
    const double param1, const double param2,                            //
    const long max_iter, const int inside)
{
    double old_real = param1;
    double old_imag = param2;
    double magnitude = 0.0;
    long iter = 0;
    while (magnitude < 4.0 && iter < max_iter)
    {
        const double new_real = old_real * old_real - old_imag * old_imag + init_real;
        const double new_imag = 2 * old_real * old_imag + init_imag;
        iter++;
        old_real = new_real;
        old_imag = new_imag;
        magnitude = new_real * new_real + new_imag * new_imag;
    }
    if (iter >= max_iter)
    {
        iter = inside;
    }
    return static_cast<int>(iter);
}

// this routine is called just before the fractal starts
bool TestPoint::start()
{
    return false;
}

// this routine is called just after the fractal ends
void TestPoint::finish()
{
}

} // namespace id::fractals
