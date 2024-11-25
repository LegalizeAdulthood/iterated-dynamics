// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "fracsuba.h"

#include "calcfrac.h"
#include "fixed_pt.h"
#include "fractals.h"

#include <cmath>
#include <cstdlib>

int long_mod_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2
        || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_real_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_imag_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_y >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_or_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if (g_l_temp_sqr_x >= g_l_magnitude_limit || g_l_temp_sqr_y >= g_l_magnitude_limit || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_and_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    if ((g_l_temp_sqr_x >= g_l_magnitude_limit && g_l_temp_sqr_y >= g_l_magnitude_limit) || g_overflow)
    {
        g_overflow = false;
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_manh_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = std::fabs(g_new_z.x) + std::fabs(g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_manr_bailout()
{
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_magnitude = std::fabs(g_new_z.x + g_new_z.y);
    if (g_magnitude*g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}
