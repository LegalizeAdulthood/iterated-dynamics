// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/popcorn.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/orbit.h"
#include "engine/resume.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "misc/id.h"

#include <cmath>

// standalone engine for "popcorn"
// subset of std engine
int popcorn_type()
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
    g_temp_sqr_x = 0;
    for (g_row = start_row; g_row <= g_i_stop_pt.y; g_row++)
    {
        g_reset_periodicity = true;
        for (g_col = 0; g_col <= g_i_stop_pt.x; g_col++)
        {
            if (standard_fractal_type() == -1) // interrupted
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
        || std::abs(g_new_z.x) > g_magnitude_limit2
        || std::abs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// Popcorn generalization

int popcorn_orbit()
{
    DComplex tmp_x;
    DComplex tmp_y;

    // tmpx contains the generalized value of the old real "x" equation
    g_tmp_z = g_param_z2 * g_old_z.y;       // tmp = (C * old.y)
    cmplx_trig1(g_tmp_z, tmp_x);            // tmpx = trig1(tmp)
    tmp_x.x += g_old_z.y;                   // tmpx = old.y + trig1(tmp)
    cmplx_trig0(tmp_x, g_tmp_z);            // tmp = trig0(tmpx)
    cmplx_mult(g_tmp_z, g_param_z1, tmp_x); // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    g_tmp_z = g_param_z2 * g_old_z.x;       // tmp = (C * old.x)
    cmplx_trig3(g_tmp_z, tmp_y);            // tmpy = trig3(tmp)
    tmp_y.x += g_old_z.x;                   // tmpy = old.x + trig1(tmp)
    cmplx_trig2(tmp_y, g_tmp_z);            // tmp = trig2(tmpy)
    cmplx_mult(g_tmp_z, g_param_z1, tmp_y); // tmpy = tmp * h

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
        || std::abs(g_new_z.x) > g_magnitude_limit2
        || std::abs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}
