// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lambda_fn.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "fractals/fractalp.h"
#include "fractals/frasetup.h"
#include "math/arg.h"
#include "ui/trig_fns.h"

#include <cmath>

namespace id::fractals
{

int lambda_orbit()
{
    // variation of classical Mandelbrot/Julia
    // note that fast >= 287 equiv in fracsuba.asm must be kept in step

    g_temp_sqr_x = g_old_z.x - g_temp_sqr_x + g_temp_sqr_y;
    g_temp_sqr_y = -(g_old_z.y * g_old_z.x);
    g_temp_sqr_y += g_temp_sqr_y + g_old_z.y;

    g_new_z.x = g_float_param->x * g_temp_sqr_x - g_float_param->y * g_temp_sqr_y;
    g_new_z.y = g_float_param->x * g_temp_sqr_y + g_float_param->y * g_temp_sqr_x;
    return id::g_bailout_float();
}

int lambda_trig_orbit()
{
    if (std::abs(g_old_z.x) >= g_magnitude_limit2 || std::abs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    cmplx_trig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    cmplx_mult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

static int lambda_trig_fractal1()
{
    // sin,cos
    if (std::abs(g_old_z.y) >= g_magnitude_limit2)
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
    if (std::abs(g_old_z.x) >= g_magnitude_limit2)
    {
        return 1;
    }
    cmplx_trig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    cmplx_mult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

static int lambda_exponent_fractal()
{
    // found this in  "Science of Fractal Images"
    if (std::abs(g_old_z.y) >= 1.0e3)
    {
        return 1;
    }
    if (std::abs(g_old_z.x) >= 8)
    {
        return 1;
    }
    double sin_y;
    double cos_y;
    sin_cos(g_old_z.y, &sin_y, &cos_y);

    if (g_old_z.x >= g_magnitude_limit && cos_y >= 0.0)
    {
        return 1;
    }
    const double tmp_exp = std::exp(g_old_z.x);
    g_tmp_z.x = tmp_exp*cos_y;
    g_tmp_z.y = tmp_exp*sin_y;

    //multiply by lamda
    g_new_z.x = g_float_param->x*g_tmp_z.x - g_float_param->y*g_tmp_z.y;
    g_new_z.y = g_float_param->y*g_tmp_z.x + g_float_param->x*g_tmp_z.y;
    g_old_z = g_new_z;
    return 0;
}

bool lambda_trig_per_image()
{
    g_cur_fractal_specific->orbit_calc = lambda_trig_orbit;
    switch (g_trig_index[0])
    {
    case TrigFn::SIN:
    case TrigFn::COSXX:
    case TrigFn::COS:
        g_symmetry = SymmetryType::PI_SYM;
        g_cur_fractal_specific->orbit_calc = lambda_trig_fractal1;
        break;
    case TrigFn::SINH:
    case TrigFn::COSH:
        g_symmetry = SymmetryType::ORIGIN;
        g_cur_fractal_specific->orbit_calc = lambda_trig_fractal2;
        break;
    case TrigFn::SQR:
        g_symmetry = SymmetryType::ORIGIN;
        break;
    case TrigFn::EXP:
        g_cur_fractal_specific->orbit_calc = lambda_exponent_fractal;
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
    return julia_per_image();
}

bool mandel_trig_per_image()
{
    g_cur_fractal_specific->orbit_calc = lambda_trig_orbit;
    g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
    switch (g_trig_index[0])
    {
    case TrigFn::SIN:
    case TrigFn::COSXX:
        g_cur_fractal_specific->orbit_calc = lambda_trig_fractal1;
        break;
    case TrigFn::SINH:
    case TrigFn::COSH:
        g_cur_fractal_specific->orbit_calc = lambda_trig_fractal2;
        break;
    case TrigFn::EXP:
        g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        g_cur_fractal_specific->orbit_calc = lambda_exponent_fractal;
        break;
    case TrigFn::LOG:
        g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        break;
    default:
        g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        break;
    }
    return mandel_per_image();
}

} // namespace id::fractals
