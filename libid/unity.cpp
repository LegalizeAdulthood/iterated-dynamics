#include "unity.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "id_data.h"
#include "sqr.h"

#include <cmath>

bool UnitySetup()
{
    g_periodicity_check = 0;
    g_fudge_one = (1L << g_bit_shift);
    g_fudge_two = g_fudge_one + g_fudge_one;
    return true;
}

int UnityFractal()
{
    const long xx_one = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) + multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    if ((xx_one > g_fudge_two) || (labs(xx_one - g_fudge_one) < g_l_delta_min))
    {
        return 1;
    }
    g_l_old_z.y = multiply(g_fudge_two - xx_one, g_l_old_z.x, g_bit_shift);
    g_l_old_z.x = multiply(g_fudge_two - xx_one, g_l_old_z.y, g_bit_shift);
    g_l_new_z = g_l_old_z;
    return 0;
}

int UnityfpFractal()
{
    const double XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
    if ((XXOne > 2.0) || (std::fabs(XXOne - 1.0) < g_delta_min))
    {
        return 1;
    }
    g_old_z.y = (2.0 - XXOne)* g_old_z.x;
    g_old_z.x = (2.0 - XXOne)* g_old_z.y;
    g_new_z = g_old_z;
    return 0;
}
