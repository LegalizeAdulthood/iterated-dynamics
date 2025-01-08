// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/volterra_lotka.h"

#include <algorithm>

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"

// Beauty of Fractals pp. 125 - 127
int vl_fp_fractal()
{
    double half = g_params[0] / 2.0;
    double xy = g_old_z.x * g_old_z.y;
    double u = g_old_z.x - xy;
    double w = -g_old_z.y + xy;
    double a = g_old_z.x + g_params[1] * u;
    double b = g_old_z.y + g_params[1] * w;
    double ab = a * b;
    g_new_z.x = g_old_z.x + half * (u + (a - ab));
    g_new_z.y = g_old_z.y + half * (w + (-b + ab));
    return g_bailout_float();
}

bool vl_setup()
{
    g_params[0] = std::max(g_params[0], 0.0);
    g_params[1] = std::max(g_params[1], 0.0);
    g_params[0] = std::min(g_params[0], 1.0);
    g_params[1] = std::min(g_params[1], 1.0);
    g_float_param = &g_param_z1;
    return true;
}