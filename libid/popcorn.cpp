// SPDX-License-Identifier: GPL-3.0-only
//
#include "popcorn.h"

#include "calcfrac.h"
#include "fpu087.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "mpmath.h"
#include "orbit.h"
#include "resume.h"

#include <cmath>

// standalone engine for "popcorn"
// subset of std engine
int popcorn()
{
    int start_row = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(start_row);
        end_resume();
    }
    g_keyboard_check_interval = g_max_keyboard_check_interval;
    g_plot = no_plot;
    g_l_temp_sqr_x = 0;
    g_temp_sqr_x = g_l_temp_sqr_x;
    for (g_row = start_row; g_row <= g_i_y_stop; g_row++)
    {
        g_reset_periodicity = true;
        for (g_col = 0; g_col <= g_i_x_stop; g_col++)
        {
            if (standard_fractal() == -1) // interrupted
            {
                alloc_resume(10, 1);
                put_resume(g_row);
                return -1;
            }
            g_reset_periodicity = false;
        }
    }
    g_calc_status = CalcStatus::COMPLETED;
    return 0;
}

int popcorn_fractal_old()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    double sin_y;
    double cos_y;
    sin_cos(&g_tmp_z.y, &sin_y, &cos_y);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = sin_y/cos_y + g_old_z.y;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    sin_cos(&g_tmp_z.y, &sin_y, &cos_y);
    g_new_z.x = g_old_z.x - g_param_z1.x*sin_y;
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
    double sin_y;
    double cos_y;
    sin_cos(&g_tmp_z.y, &sin_y, &cos_y);
    g_tmp_z.x = g_sin_x/g_cos_x + g_old_z.x;
    g_tmp_z.y = sin_y/cos_y + g_old_z.y;
    sin_cos(&g_tmp_z.x, &g_sin_x, &g_cos_x);
    sin_cos(&g_tmp_z.y, &sin_y, &cos_y);
    g_new_z.x = g_old_z.x - g_param_z1.x*sin_y;
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
    static constexpr long L16_TRIG_LIM = 8L << 16; // domain limit of fast trig functions

    return labs(val) > L16_TRIG_LIM;
}

static void ltrig_arg(long &val)
{
    if (trig16_check(val))
    {
        double tmp = val;
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
    long l_cos_x;
    long l_sin_x;
    sin_cos(g_l_temp.x, &l_sin_x, &l_cos_x);
    long l_cos_y;
    long l_sin_y;
    sin_cos(g_l_temp.y, &l_sin_y, &l_cos_y);
    g_l_temp.x = divide(l_sin_x, l_cos_x, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(l_sin_y, l_cos_y, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    sin_cos(g_l_temp.x, &l_sin_x, &l_cos_x);
    sin_cos(g_l_temp.y, &l_sin_y, &l_cos_y);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, l_sin_y, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, l_sin_x, g_bit_shift);
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
    long l_cos_x;
    long l_sin_x;
    sin_cos(g_l_temp.x, &l_sin_x, &l_cos_x);
    long l_cos_y;
    long l_sin_y;
    sin_cos(g_l_temp.y, &l_sin_y, &l_cos_y);
    g_l_temp.x = divide(l_sin_x, l_cos_x, g_bit_shift) + g_l_old_z.x;
    g_l_temp.y = divide(l_sin_y, l_cos_y, g_bit_shift) + g_l_old_z.y;
    ltrig_arg(g_l_temp.x);
    ltrig_arg(g_l_temp.y);
    sin_cos(g_l_temp.x, &l_sin_x, &l_cos_x);
    sin_cos(g_l_temp.y, &l_sin_y, &l_cos_y);
    g_l_new_z.x = g_l_old_z.x - multiply(g_l_param.x, l_sin_y, g_bit_shift);
    g_l_new_z.y = g_l_old_z.y - multiply(g_l_param.x, l_sin_x, g_bit_shift);
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
    DComplex tmp_x;
    DComplex tmp_y;

    // tmpx contains the generalized value of the old real "x" equation
    g_tmp_z = g_param_z2*g_old_z.y;  // tmp = (C * old.y)
    cmplx_trig1(g_tmp_z, tmp_x);             // tmpx = trig1(tmp)
    tmp_x.x += g_old_z.y;                  // tmpx = old.y + trig1(tmp)
    cmplx_trig0(tmp_x, g_tmp_z);             // tmp = trig0(tmpx)
    cmplx_mult(g_tmp_z, g_param_z1, tmp_x);         // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    g_tmp_z = g_param_z2*g_old_z.x;  // tmp = (C * old.x)
    cmplx_trig3(g_tmp_z, tmp_y);             // tmpy = trig3(tmp)
    tmp_y.x += g_old_z.x;                  // tmpy = old.x + trig1(tmp)
    cmplx_trig2(tmp_y, g_tmp_z);             // tmp = trig2(tmpy)

    cmplx_mult(g_tmp_z, g_param_z1, tmp_y);         // tmpy = tmp * h

    g_new_z.x = g_old_z.x - tmp_x.x - tmp_y.y;
    g_new_z.y = g_old_z.y - tmp_y.x - tmp_x.y;

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
    LComplex l_tmp_x;
    LComplex l_tmp_y;

    g_overflow = false;

    // ltmpx contains the generalized value of the old real "x" equation
    lcmplx_times_real(g_l_param2, g_l_old_z.y, g_l_temp); // tmp = (C * old.y)
    trig1(g_l_temp, l_tmp_x);                             // tmpx = trig1(tmp)
    fix_overflow(l_tmp_x);                                //
    l_tmp_x.x += g_l_old_z.y;                             // tmpx = old.y + trig1(tmp)
    trig0(l_tmp_x, g_l_temp);                             // tmp = trig0(tmpx)
    fix_overflow(g_l_temp);                             //
    l_tmp_x = g_l_temp * g_l_param;                       // tmpx = tmp * h

    // ltmpy contains the generalized value of the old real "y" equation
    lcmplx_times_real(g_l_param2, g_l_old_z.x, g_l_temp); // tmp = (C * old.x)
    trig3(g_l_temp, l_tmp_y);                             // tmpy = trig3(tmp)
    fix_overflow(l_tmp_y);                                //
    l_tmp_y.x += g_l_old_z.x;                             // tmpy = old.x + trig1(tmp)
    trig2(l_tmp_y, g_l_temp);                             // tmp = trig2(tmpy)
    fix_overflow(g_l_temp);                             //
    l_tmp_y = g_l_temp * g_l_param;                       // tmpy = tmp * h

    g_l_new_z.x = g_l_old_z.x - l_tmp_x.x - l_tmp_y.y;
    g_l_new_z.y = g_l_old_z.y - l_tmp_y.x - l_tmp_x.y;

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
