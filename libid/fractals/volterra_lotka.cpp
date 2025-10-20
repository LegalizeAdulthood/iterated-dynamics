// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/volterra_lotka.h"

#include <algorithm>

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"

using namespace id::engine;

namespace id::fractals
{

// Beauty of Fractals pp. 125 - 127
int vl_orbit()
{
    const double half = g_params[0] / 2.0;
    const double xy = g_old_z.x * g_old_z.y;
    const double u = g_old_z.x - xy;
    const double w = -g_old_z.y + xy;
    const double a = g_old_z.x + g_params[1] * u;
    const double b = g_old_z.y + g_params[1] * w;
    const double ab = a * b;
    g_new_z.x = g_old_z.x + half * (u + (a - ab));
    g_new_z.y = g_old_z.y + half * (w + (-b + ab));
    return g_bailout_float();
}

bool vl_per_image()
{
    g_params[0] = std::max(g_params[0], 0.0);
    g_params[1] = std::max(g_params[1], 0.0);
    g_params[0] = std::min(g_params[0], 1.0);
    g_params[1] = std::min(g_params[1], 1.0);
    g_float_param = &g_param_z1;
    return true;
}

} // namespace id::fractals
