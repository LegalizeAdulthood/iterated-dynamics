// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/bif_may.h"

#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/population.h"

#include <cmath>

using namespace id::engine;

namespace id::fractals
{

static long s_beta{};

int bifurc_may_orbit()
{
    /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    g_tmp_z.x = 1.0 + g_population;
    g_tmp_z.x = std::pow(g_tmp_z.x, -s_beta); // pow in math.h included with math/mpmath.h
    g_population = g_rate * g_population * g_tmp_z.x;
    return population_orbit();
}

bool bifurc_may_per_image()
{
    g_params[2] = std::max(g_params[2], 2.0);
    s_beta = static_cast<long>(g_params[2]);
    engine_timer(g_cur_fractal_specific->calc_type);
    return false;
}

} // namespace id::fractals
