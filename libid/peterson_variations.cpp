// SPDX-License-Identifier: GPL-3.0-only
//
#include "peterson_variations.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "fpu087.h"
#include "fractals.h"
#include "fractype.h"
#include "get_julia_attractor.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "newton.h"
#include "pixel_grid.h"

DComplex g_marks_coefficient{};

static LComplex s_l_coefficient{};

bool marks_julia_setup()
{
    if (g_params[2] < 1)
    {
        g_params[2] = 1;
    }
    g_c_exponent = (int)g_params[2];
    g_long_param = &g_l_param;
    g_l_old_z = *g_long_param;
    if (g_c_exponent > 3)
    {
        lcpower(&g_l_old_z, g_c_exponent-1, &s_l_coefficient, g_bit_shift);
    }
    else if (g_c_exponent == 3)
    {
        s_l_coefficient.x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) - multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        s_l_coefficient.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    }
    else if (g_c_exponent == 2)
    {
        s_l_coefficient = g_l_old_z;
    }
    else if (g_c_exponent < 2)
    {
        s_l_coefficient.x = 1L << g_bit_shift;
        s_l_coefficient.y = 0L;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool marks_julia_fp_setup()
{
    if (g_params[2] < 1)
    {
        g_params[2] = 1;
    }
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    g_old_z = *g_float_param;
    if (g_c_exponent > 3)
    {
        cpower(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
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

int marks_lambda_fractal()
{
    // Mark Peterson's variation of "lambda" function

    // Z1 = (C^(exp-1) * Z**2) + C
    g_l_temp.x = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_temp.y = multiply(g_l_old_z.x , g_l_old_z.y , g_bit_shift_less_1);

    g_l_new_z.x = multiply(s_l_coefficient.x, g_l_temp.x, g_bit_shift)
             - multiply(s_l_coefficient.y, g_l_temp.y, g_bit_shift) + g_long_param->x;
    g_l_new_z.y = multiply(s_l_coefficient.x, g_l_temp.y, g_bit_shift)
             + multiply(s_l_coefficient.y, g_l_temp.x, g_bit_shift) + g_long_param->y;

    return g_bailout_long();
}

int marks_lambda_fp_fractal()
{
    // Mark Peterson's variation of "lambda" function

    // Z1 = (C^(exp-1) * Z**2) + C
    g_tmp_z.x = g_temp_sqr_x - g_temp_sqr_y;
    g_tmp_z.y = g_old_z.x * g_old_z.y *2;

    g_new_z.x = g_marks_coefficient.x * g_tmp_z.x - g_marks_coefficient.y * g_tmp_z.y + g_float_param->x;
    g_new_z.y = g_marks_coefficient.x * g_tmp_z.y + g_marks_coefficient.y * g_tmp_z.x + g_float_param->y;

    return g_bailout_float();
}

int marks_cplx_mand()
{
    g_tmp_z.x = g_temp_sqr_x - g_temp_sqr_y;
    g_tmp_z.y = 2*g_old_z.x*g_old_z.y;
    fpu_cmplx_mul(&g_tmp_z, &g_marks_coefficient, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

/*
   MarksMandelPwr (XAXIS) {
      z = pixel, c = z ^ (z - 1):
         z = c * sqr(z) + pixel,
      |z| <= 4
   }
*/

int marks_mandel_pwr_fp_fractal()
{
    cmplx_trig0(g_old_z, g_new_z);
    cmplx_mult(g_tmp_z, g_new_z, g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

int marks_mandel_pwr_fractal()
{
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z = g_l_temp * g_l_new_z;
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

/* I was coding Marksmandelpower and failed to use some temporary
   variables. The result was nice, and since my name is not on any fractal,
   I thought I would immortalize myself with this error!
                Tim Wegner */

int tims_error_fp_fractal()
{
    cmplx_trig0(g_old_z, g_new_z);
    g_new_z.x = g_new_z.x * g_tmp_z.x - g_new_z.y * g_tmp_z.y;
    g_new_z.y = g_new_z.x * g_tmp_z.y - g_new_z.y * g_tmp_z.x;
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

int tims_error_fractal()
{
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x = multiply(g_l_new_z.x, g_l_temp.x, g_bit_shift)-multiply(g_l_new_z.y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y = multiply(g_l_new_z.x, g_l_temp.y, g_bit_shift)-multiply(g_l_new_z.y, g_l_temp.x, g_bit_shift);
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int marks_mandel_pwr_per_pixel()
{
    mandel_per_pixel();
    g_l_temp = g_l_old_z;
    g_l_temp.x -= g_fudge_factor;
    lcmplx_pwr(g_l_old_z, g_l_temp, g_l_temp);
    return 1;
}

int marks_mandel_per_pixel()
{
    // marksmandel
    if (g_invert != 0)
    {
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }
    else
    {
        g_l_init.x = g_l_x_pixel();
        g_l_init.y = g_l_y_pixel();
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;

    if (g_c_exponent > 3)
    {
        lcpower(&g_l_old_z, g_c_exponent-1, &s_l_coefficient, g_bit_shift);
    }
    else if (g_c_exponent == 3)
    {
        s_l_coefficient.x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift)
                         - multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        s_l_coefficient.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    }
    else if (g_c_exponent == 2)
    {
        s_l_coefficient = g_l_old_z;
    }
    else if (g_c_exponent < 2)
    {
        s_l_coefficient.x = 1L << g_bit_shift;
        s_l_coefficient.y = 0L;
    }

    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    return 1; // 1st iteration has been done
}

int marks_mandel_fp_per_pixel()
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

    if (g_use_init_orbit == init_orbit_mode::value)
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
        cpower(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
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

int marks_mandel_pwr_fp_per_pixel()
{
    mandel_fp_per_pixel();
    g_tmp_z = g_old_z;
    g_tmp_z.x -= 1;
    cmplx_pwr(g_old_z, g_tmp_z, g_tmp_z);
    return 1;
}

int marks_cplx_mand_perp()
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
