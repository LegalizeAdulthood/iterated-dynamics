// SPDX-License-Identifier: GPL-3.0-only
//
#include "lambda_fn.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "get_julia_attractor.h"
#include "mpmath.h"

#include <cmath>

int lambda_fractal()
{
    // variation of classical Mandelbrot/Julia

    // in complex math) temp = Z * (1-Z)
    g_l_temp_sqr_x = g_l_old_z.x - g_l_temp_sqr_x + g_l_temp_sqr_y;
    g_l_temp_sqr_y = g_l_old_z.y
                - multiply(g_l_old_z.y, g_l_old_z.x, g_bit_shift_less_1);
    // (in complex math) Z = Lambda * Z
    g_l_new_z.x = multiply(g_long_param->x, g_l_temp_sqr_x, g_bit_shift)
             - multiply(g_long_param->y, g_l_temp_sqr_y, g_bit_shift);
    g_l_new_z.y = multiply(g_long_param->x, g_l_temp_sqr_y, g_bit_shift)
             + multiply(g_long_param->y, g_l_temp_sqr_x, g_bit_shift);
    return g_bailout_long();
}

int lambda_fp_fractal()
{
    // variation of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    g_temp_sqr_x = g_old_z.x - g_temp_sqr_x + g_temp_sqr_y;
    g_temp_sqr_y = -(g_old_z.y * g_old_z.x);
    g_temp_sqr_y += g_temp_sqr_y + g_old_z.y;

    g_new_z.x = g_float_param->x * g_temp_sqr_x - g_float_param->y * g_temp_sqr_y;
    g_new_z.y = g_float_param->x * g_temp_sqr_y + g_float_param->y * g_temp_sqr_x;
    return g_bailout_float();
}

int lambda_trig_fractal()
{
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2 || labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    trig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int lambda_trig_fp_fractal()
{
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2 || std::fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    cmplx_trig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    cmplx_mult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

// bailouts are different for different trig functions
static int lambda_trig_fractal1()
{
    // sin,cos
    if (labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    trig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

static int lambda_trigfp_fractal1()
{
    // sin,cos
    if (std::fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    cmplx_trig0(g_old_z, g_tmp_z);                // tmp = trig(old)
    cmplx_mult(*g_float_param, g_tmp_z, g_new_z); // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

static int lambda_trig_fractal2()
{
    // sinh,cosh
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    trig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

static int lambda_trigfp_fractal2()
{
    // sinh,cosh
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2)
    {
        return 1;
    }
    cmplx_trig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    cmplx_mult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

static int long_lambda_exponent_fractal()
{
    // found this in  "Science of Fractal Images"
    if (labs(g_l_old_z.y) >= (1000L << g_bit_shift))
    {
        return 1;
    }
    if (labs(g_l_old_z.x) >= (8L << g_bit_shift))
    {
        return 1;
    }

    long lsiny;
    long lcosy;
    sin_cos(g_l_old_z.y, &lsiny,  &lcosy);

    if (g_l_old_z.x >= g_l_magnitude_limit && lcosy >= 0L)
    {
        return 1;
    }
    long longtmp = exp_long(g_l_old_z.x);

    g_l_temp.x = multiply(longtmp,      lcosy,   g_bit_shift);
    g_l_temp.y = multiply(longtmp,      lsiny,   g_bit_shift);

    g_l_new_z.x  = multiply(g_long_param->x, g_l_temp.x, g_bit_shift)
              - multiply(g_long_param->y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y  = multiply(g_long_param->x, g_l_temp.y, g_bit_shift)
              + multiply(g_long_param->y, g_l_temp.x, g_bit_shift);
    g_l_old_z = g_l_new_z;
    return 0;
}

static int lambda_exponent_fractal()
{
    // found this in  "Science of Fractal Images"
    if (std::fabs(g_old_z.y) >= 1.0e3)
    {
        return 1;
    }
    if (std::fabs(g_old_z.x) >= 8)
    {
        return 1;
    }
    double siny;
    double cosy;
    sin_cos(&g_old_z.y, &siny, &cosy);

    if (g_old_z.x >= g_magnitude_limit && cosy >= 0.0)
    {
        return 1;
    }
    const double tmpexp = std::exp(g_old_z.x);
    g_tmp_z.x = tmpexp*cosy;
    g_tmp_z.y = tmpexp*siny;

    //multiply by lamda
    g_new_z.x = g_float_param->x*g_tmp_z.x - g_float_param->y*g_tmp_z.y;
    g_new_z.y = g_float_param->y*g_tmp_z.x + g_float_param->x*g_tmp_z.y;
    g_old_z = g_new_z;
    return 0;
}

bool lambda_trig_setup()
{
    bool const isinteger = g_cur_fractal_specific->isinteger != 0;
    if (isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal;
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  lambda_trig_fp_fractal;
    }
    switch (g_trig_index[0])
    {
    case TrigFn::SIN:
    case TrigFn::COSXX:
    case TrigFn::COS:
        g_symmetry = SymmetryType::PI_SYM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal1;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trigfp_fractal1;
        }
        break;
    case TrigFn::SINH:
    case TrigFn::COSH:
        g_symmetry = SymmetryType::ORIGIN;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trigfp_fractal2;
        }
        break;
    case TrigFn::SQR:
        g_symmetry = SymmetryType::ORIGIN;
        break;
    case TrigFn::EXP:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  long_lambda_exponent_fractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_exponent_fractal;
        }
        g_symmetry = SymmetryType::NONE;
        break;
    case TrigFn::LOG:
        g_symmetry = SymmetryType::NONE;
        break;
    default:   // default for additional functions
        g_symmetry = SymmetryType::ORIGIN;
        break;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    if (isinteger)
    {
        return julia_long_setup();
    }
    else
    {
        return julia_fp_setup();
    }
}

bool mandel_trig_setup()
{
    bool const isinteger = g_cur_fractal_specific->isinteger != 0;
    if (isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal;
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  lambda_trig_fp_fractal;
    }
    g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
    switch (g_trig_index[0])
    {
    case TrigFn::SIN:
    case TrigFn::COSXX:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal1;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trigfp_fractal1;
        }
        break;
    case TrigFn::SINH:
    case TrigFn::COSH:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trig_fractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_trigfp_fractal2;
        }
        break;
    case TrigFn::EXP:
        g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  long_lambda_exponent_fractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  lambda_exponent_fractal;
        }
        break;
    case TrigFn::LOG:
        g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        break;
    default:
        g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        break;
    }
    if (isinteger)
    {
        return mandel_long_setup();
    }
    else
    {
        return mandel_fp_setup();
    }
}
