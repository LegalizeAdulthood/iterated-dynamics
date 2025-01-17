// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/unity.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "math/fixed_pt.h"
#include "math/sqr.h"

#include <cmath>

static long s_fudge_one;
static long s_fudge_two;

bool unity_setup()
{
    g_periodicity_check = 0;
    s_fudge_one = (1L << g_bit_shift);
    s_fudge_two = s_fudge_one + s_fudge_one;
    return true;
}

int unity_fractal()
{
    const long xx_one = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) + multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    if ((xx_one > s_fudge_two) || (std::abs(xx_one - s_fudge_one) < g_l_delta_min))
    {
        return 1;
    }
    g_l_old_z.y = multiply(s_fudge_two - xx_one, g_l_old_z.x, g_bit_shift);
    g_l_old_z.x = multiply(s_fudge_two - xx_one, g_l_old_z.y, g_bit_shift);
    g_l_new_z = g_l_old_z;
    return 0;
}

int unity_fp_fractal()
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
