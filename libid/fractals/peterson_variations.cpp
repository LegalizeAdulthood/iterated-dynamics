// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/peterson_variations.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "fractals/newton.h"
#include "math/arg.h"
#include "math/fpu087.h"

#include <algorithm>

namespace id::fractals
{

DComplex g_marks_coefficient{};

bool marks_julia_per_image()
{
    g_params[2] = std::max(g_params[2], 1.0);
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    g_old_z = g_param_z1;
    if (g_c_exponent > 3)
    {
        pow(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
    }
    else if (g_c_exponent == 3)
    {
        g_marks_coefficient.x = sqr(g_old_z.x) - sqr(g_old_z.y);
        g_marks_coefficient.y = g_old_z.x * g_old_z.y * 2;
    }
    else if (g_c_exponent == 2)
    {
        g_marks_coefficient = g_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_marks_coefficient.x = 1.0;
        g_marks_coefficient.y = 0.0;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

int marks_lambda_orbit()
{
    // Mark Peterson's variation of "lambda" function

    // Z1 = (C^(exp-1) * Z**2) + C
    g_tmp_z.x = g_temp_sqr_x - g_temp_sqr_y;
    g_tmp_z.y = g_old_z.x * g_old_z.y *2;

    g_new_z.x = g_marks_coefficient.x * g_tmp_z.x - g_marks_coefficient.y * g_tmp_z.y + g_float_param->x;
    g_new_z.y = g_marks_coefficient.x * g_tmp_z.y + g_marks_coefficient.y * g_tmp_z.x + g_float_param->y;

    return id::g_bailout_float();
}

int marks_cplx_mand_orbit()
{
    g_tmp_z.x = g_temp_sqr_x - g_temp_sqr_y;
    g_tmp_z.y = 2*g_old_z.x*g_old_z.y;
    fpu_cmplx_mul(&g_tmp_z, &g_marks_coefficient, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return id::g_bailout_float();
}

/*
   MarksMandelPwr (XAXIS) {
      z = pixel, c = z ^ (z - 1):
         z = c * sqr(z) + pixel,
      |z| <= 4
   }
*/

int marks_mandel_pwr_orbit()
{
    cmplx_trig0(g_old_z, g_new_z);
    cmplx_mult(g_tmp_z, g_new_z, g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return id::g_bailout_float();
}

/* I was coding Marksmandelpower and failed to use some temporary
   variables. The result was nice, and since my name is not on any fractal,
   I thought I would immortalize myself with this error!
                Tim Wegner */

int tims_error_orbit()
{
    cmplx_trig0(g_old_z, g_new_z);
    g_new_z.x = g_new_z.x * g_tmp_z.x - g_new_z.y * g_tmp_z.y;
    g_new_z.y = g_new_z.x * g_tmp_z.y - g_new_z.y * g_tmp_z.x;
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return id::g_bailout_float();
}

int marks_mandel_per_pixel()
{
    // marksmandel

    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }

    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;

    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);

    if (g_c_exponent > 3)
    {
        pow(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
    }
    else if (g_c_exponent == 3)
    {
        g_marks_coefficient.x = g_temp_sqr_x - g_temp_sqr_y;
        g_marks_coefficient.y = g_old_z.x * g_old_z.y * 2;
    }
    else if (g_c_exponent == 2)
    {
        g_marks_coefficient = g_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_marks_coefficient.x = 1.0;
        g_marks_coefficient.y = 0.0;
    }

    return 1; // 1st iteration has been done
}

int marks_mandel_pwr_per_pixel()
{
    mandel_per_pixel();
    g_tmp_z = g_old_z;
    g_tmp_z.x -= 1;
    cmplx_pwr(g_old_z, g_tmp_z, g_tmp_z);
    return 1;
}

int marks_cplx_mand_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }
    g_old_z.x = g_init.x + g_param_z1.x; // initial perturbation of parameters set
    g_old_z.y = g_init.y + g_param_z1.y;
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value
    g_temp_sqr_y = sqr(g_old_z.y);
    g_marks_coefficient = complex_power(g_init, g_power_z);
    return 1;
}

} // namespace id::fractals
