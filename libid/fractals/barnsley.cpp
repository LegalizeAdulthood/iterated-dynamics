// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/barnsley.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"

using namespace id::engine;

namespace id::fractals
{

int barnsley1_orbit()
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

int barnsley2_orbit()
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

int barnsley3_orbit()
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
           is uninteresting.) */
        g_new_z.y += g_float_param->y * g_old_z.x;
    }
    return g_bailout_float();
}

} // namespace id::fractals
