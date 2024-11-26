// SPDX-License-Identifier: GPL-3.0-only
//
#include "popcorn.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fpu087.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "mpmath.h"
#include "orbit.h"

#include <cmath>

int popcorn_fractal_old()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    double siny;
    double cosy;
    sin_cos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    sin_cos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == no_plot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    else
    {
        g_temp_sqr_x = sqr(g_new_z.x);
    }
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int popcorn_fractal()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    double siny;
    double cosy;
    sin_cos(&g_tmp_z.y, &siny, &cosy);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = siny/cosy + g_old_z.y;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    sin_cos(&g_tmp_z.y, &siny, &cosy);
    g_new_z.x = g_old_z.x - g_param_z1.x*siny;
    g_new_z.y = g_old_z.y - g_param_z1.x*g_sin_x;
    if (g_plot == no_plot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit
        || std::fabs(g_new_z.x) > g_magnitude_limit2
        || std::fabs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

inline bool trig16_check(long val)
{
    static constexpr long l16triglim = 8L << 16; // domain limit of fast trig functions

    return labs(val) > l16triglim;
}

static void ltrig_arg(long &val)
{
    if (trig16_check(val))
    {
        double tmp;
        tmp = val;
        tmp /= g_fudge_factor;
        tmp = std::fmod(tmp, PI * 2.0);
        tmp *= g_fudge_factor;
        val = (long) tmp;
    }
}

int long_popcorn_fractal_old()
{
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    long lcosx;
    long lsinx;
    sin_cos(g_l_temp.x, &lsinx, &lcosx);
    long lcosy;
    long lsiny;
    sin_cos(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    sin_cos(g_l_temp.x, &lsinx, &lcosx);
    sin_cos(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == no_plot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    else
    {
        g_l_temp_sqr_x = lsqr(g_l_new_z.x);
        g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    }
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

int long_popcorn_fractal()
{
    g_l_temp = g_l_old_z;
    g_l_temp.x *= 3L;
    g_l_temp.y *= 3L;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    long lcosx;
    long lsinx;
    sin_cos(g_l_temp.x, &lsinx, &lcosx);
    long lcosy;
    long lsiny;
    sin_cos(g_l_temp.y, &lsiny, &lcosy);
    g_l_temp.x = divide(lsinx, lcosx, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(lsiny, lcosy, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    sin_cos(g_l_temp.x, &lsinx, &lcosx);
    sin_cos(g_l_temp.y, &lsiny, &lcosy);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, lsiny, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, lsinx, g_bit_shift);
    if (g_plot == no_plot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    // else
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}

// Popcorn generalization

int popcorn_fractal_fn()
{
    DComplex tmpx;
    DComplex tmpy;

    // tmpx contains the generalized value of the old real "x" equation
    g_tmp_z = g_param_z2*g_old_z.y;  // tmp = (C * old.y)
    CMPLXtrig1(g_tmp_z, tmpx);             // tmpx = trig1(tmp)
    tmpx.x += g_old_z.y;                  // tmpx = old.y + trig1(tmp)
    CMPLXtrig0(tmpx, g_tmp_z);             // tmp = trig0(tmpx)
    CMPLXmult(g_tmp_z, g_param_z1, tmpx);         // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    g_tmp_z = g_param_z2*g_old_z.x;  // tmp = (C * old.x)
    CMPLXtrig3(g_tmp_z, tmpy);             // tmpy = trig3(tmp)
    tmpy.x += g_old_z.x;                  // tmpy = old.x + trig1(tmp)
    CMPLXtrig2(tmpy, g_tmp_z);             // tmp = trig2(tmpy)

    CMPLXmult(g_tmp_z, g_param_z1, tmpy);         // tmpy = tmp * h

    g_new_z.x = g_old_z.x - tmpx.x - tmpy.y;
    g_new_z.y = g_old_z.y - tmpy.x - tmpx.y;

    if (g_plot == no_plot)
    {
        plot_orbit(g_new_z.x, g_new_z.y, 1+g_row%g_colors);
        g_old_z = g_new_z;
    }

    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit
        || std::fabs(g_new_z.x) > g_magnitude_limit2
        || std::fabs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

inline void fix_overflow(LComplex &arg)
{
    if (g_overflow)
    {
        arg.x = g_fudge_factor;
        arg.y = 0;
        g_overflow = false;
    }
}

int long_popcorn_fractal_fn()
{
    LComplex ltmpx;
    LComplex ltmpy;

    g_overflow = false;

    // ltmpx contains the generalized value of the old real "x" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.y, g_l_temp); // tmp = (C * old.y)
    trig1(g_l_temp, ltmpx);                             // tmpx = trig1(tmp)
    fix_overflow(ltmpx);                                //
    ltmpx.x += g_l_old_z.y;                             // tmpx = old.y + trig1(tmp)
    trig0(ltmpx, g_l_temp);                             // tmp = trig0(tmpx)
    fix_overflow(g_l_temp);                             //
    ltmpx = g_l_temp * g_l_param;                       // tmpx = tmp * h

    // ltmpy contains the generalized value of the old real "y" equation
    LCMPLXtimesreal(g_l_param2, g_l_old_z.x, g_l_temp); // tmp = (C * old.x)
    trig3(g_l_temp, ltmpy);                             // tmpy = trig3(tmp)
    fix_overflow(ltmpy);                                //
    ltmpy.x += g_l_old_z.x;                             // tmpy = old.x + trig1(tmp)
    trig2(ltmpy, g_l_temp);                             // tmp = trig2(tmpy)
    fix_overflow(g_l_temp);                             //
    ltmpy = g_l_temp * g_l_param;                       // tmpy = tmp * h

    g_l_new_z.x = g_l_old_z.x - ltmpx.x - ltmpy.y;
    g_l_new_z.y = g_l_old_z.y - ltmpy.x - ltmpx.y;

    if (g_plot == no_plot)
    {
        iplot_orbit(g_l_new_z.x, g_l_new_z.y, 1+g_row%g_colors);
        g_l_old_z = g_l_new_z;
    }
    g_l_temp_sqr_x = lsqr(g_l_new_z.x);
    g_l_temp_sqr_y = lsqr(g_l_new_z.y);
    g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
    if (g_l_magnitude >= g_l_magnitude_limit
        || g_l_magnitude < 0
        || labs(g_l_new_z.x) > g_l_magnitude_limit2
        || labs(g_l_new_z.y) > g_l_magnitude_limit2)
    {
        return 1;
    }
    g_l_old_z = g_l_new_z;
    return 0;
}
