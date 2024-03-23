#include "unity.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"

#include <cmath>

static long g_xx_one;

int UnityFractal()
{
    g_xx_one = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) + multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    if ((g_xx_one > g_fudge_two) || (labs(g_xx_one - g_fudge_one) < g_l_delta_min))
    {
        return 1;
    }
    g_l_old_z.y = multiply(g_fudge_two - g_xx_one, g_l_old_z.x, g_bit_shift);
    g_l_old_z.x = multiply(g_fudge_two - g_xx_one, g_l_old_z.y, g_bit_shift);
    g_l_new_z = g_l_old_z;
    return 0;
}

int UnityfpFractal()
{
    double XXOne;
    XXOne = sqr(g_old_z.x) + sqr(g_old_z.y);
    if ((XXOne > 2.0) || (std::fabs(XXOne - 1.0) < g_delta_min))
    {
        return 1;
    }
    g_old_z.y = (2.0 - XXOne)* g_old_z.x;
    g_old_z.x = (2.0 - XXOne)* g_old_z.y;
    g_new_z = g_old_z;
    return 0;
}
