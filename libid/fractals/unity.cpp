// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/unity.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "math/sqr.h"

#include <cmath>

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

int unity_orbit()
{
    const double XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
    if ((XXOne > 2.0) || (std::abs(XXOne - 1.0) < g_delta_min))
    {
        return 1;
    }
    g_old_z.y = (2.0 - XXOne)* g_old_z.x;
    g_old_z.x = (2.0 - XXOne)* g_old_z.y;
    g_new_z = g_old_z;
    return 0;
}

} // namespace id::fractals
