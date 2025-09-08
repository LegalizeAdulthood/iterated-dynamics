// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/quartic_mandelbrot.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"

using namespace id::engine;

namespace id::fractals
{

int mandel4_orbit()
{
    // first, compute (x + iy)**2
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y;
    g_new_z.y = g_old_z.x*g_old_z.y*2;
    if (g_bailout_float())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y =  g_old_z.x*g_old_z.y*2 + g_float_param->y;
    return g_bailout_float();
}

} // namespace id::fractals
