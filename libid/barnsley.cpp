// SPDX-License-Identifier: GPL-3.0-only
//
#include "barnsley.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fixed_pt.h"
#include "fractals.h"

int barnsley1_fractal()
{
    // Barnsley's Mandelbrot type M1 from "Fractals Everywhere" by Michael Barnsley, p. 322

    // calculate intermediate products
    const long old_x_init_x   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    const long old_y_init_y   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    const long old_x_init_y   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    const long old_y_init_x   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);
    // orbit calculation
    if (g_l_old_z.x >= 0)
    {
        g_l_new_z.x = (old_x_init_x - g_long_param->x - old_y_init_y);
        g_l_new_z.y = (old_y_init_x - g_long_param->y + old_x_init_y);
    }
    else
    {
        g_l_new_z.x = (old_x_init_x + g_long_param->x - old_y_init_y);
        g_l_new_z.y = (old_y_init_x + g_long_param->y + old_x_init_y);
    }
    return g_bailout_long();
}

int barnsley1_fp_fractal()
{
    // Barnsley's Mandelbrot type M1 from "Fractals Everywhere" by Michael Barnsley, p. 322

    // calculate intermediate products
    const double old_x_init_x = g_old_z.x * g_float_param->x;
    const double old_y_init_y = g_old_z.y * g_float_param->y;
    const double old_x_init_y = g_old_z.x * g_float_param->y;
    const double old_y_init_x = g_old_z.y * g_float_param->x;
    // orbit calculation
    if (g_old_z.x >= 0)
    {
        g_new_z.x = (old_x_init_x - g_float_param->x - old_y_init_y);
        g_new_z.y = (old_y_init_x - g_float_param->y + old_x_init_y);
    }
    else
    {
        g_new_z.x = (old_x_init_x + g_float_param->x - old_y_init_y);
        g_new_z.y = (old_y_init_x + g_float_param->y + old_x_init_y);
    }
    return g_bailout_float();
}

int barnsley2_fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 331,
    // example 4.2

    // calculate intermediate products
    const long old_x_init_x   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    const long old_y_init_y   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    const long old_x_init_y   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    const long old_y_init_x   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);

    // orbit calculation
    if (old_x_init_y + old_y_init_x >= 0)
    {
        g_l_new_z.x = old_x_init_x - g_long_param->x - old_y_init_y;
        g_l_new_z.y = old_y_init_x - g_long_param->y + old_x_init_y;
    }
    else
    {
        g_l_new_z.x = old_x_init_x + g_long_param->x - old_y_init_y;
        g_l_new_z.y = old_y_init_x + g_long_param->y + old_x_init_y;
    }
    return g_bailout_long();
}

int barnsley2_fp_fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 331,
    // example 4.2

    // calculate intermediate products
    const double old_x_init_x = g_old_z.x * g_float_param->x;
    const double old_y_init_y = g_old_z.y * g_float_param->y;
    const double old_x_init_y = g_old_z.x * g_float_param->y;
    const double old_y_init_x = g_old_z.y * g_float_param->x;

    // orbit calculation
    if (old_x_init_y + old_y_init_x >= 0)
    {
        g_new_z.x = old_x_init_x - g_float_param->x - old_y_init_y;
        g_new_z.y = old_y_init_x - g_float_param->y + old_x_init_y;
    }
    else
    {
        g_new_z.x = old_x_init_x + g_float_param->x - old_y_init_y;
        g_new_z.y = old_y_init_x + g_float_param->y + old_x_init_y;
    }
    return g_bailout_float();
}

int barnsley3_fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 292,
    // example 4.1

    // calculate intermediate products
    const long old_x_init_x   = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    const long old_y_init_y   = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    const long old_x_init_y   = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);

    // orbit calculation
    if (g_l_old_z.x > 0)
    {
        g_l_new_z.x = old_x_init_x   - old_y_init_y - g_fudge_factor;
        g_l_new_z.y = old_x_init_y << 1;
    }
    else
    {
        g_l_new_z.x = old_x_init_x - old_y_init_y - g_fudge_factor
                 + multiply(g_long_param->x, g_l_old_z.x, g_bit_shift);
        g_l_new_z.y = old_x_init_y <<1;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_l_new_z.y += multiply(g_long_param->y, g_l_old_z.x, g_bit_shift);
    }
    return g_bailout_long();
}

int barnsley3_fp_fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 292,
    // example 4.1

    // calculate intermediate products
    const double old_x_init_x  = g_old_z.x * g_old_z.x;
    const double old_y_init_y  = g_old_z.y * g_old_z.y;
    const double old_x_init_y  = g_old_z.x * g_old_z.y;

    // orbit calculation
    if (g_old_z.x > 0)
    {
        g_new_z.x = old_x_init_x - old_y_init_y - 1.0;
        g_new_z.y = old_x_init_y * 2;
    }
    else
    {
        g_new_z.x = old_x_init_x - old_y_init_y -1.0 + g_float_param->x * g_old_z.x;
        g_new_z.y = old_x_init_y * 2;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_new_z.y += g_float_param->y * g_old_z.x;
    }
    return g_bailout_float();
}
