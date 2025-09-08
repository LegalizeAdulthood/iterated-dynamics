// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/phoenix.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/newton.h"
#include "math/cmplx.h"
#include "math/fpu087.h"

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

static DComplex s_tmp2{};

int phoenix_orbit()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_fractal_cplx_orbit()
{
    // z(n+1) = z(n)^2 + p1 + p2*y(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x + (g_param_z2.x * s_tmp2.x) - (g_param_z2.y * s_tmp2.y);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + g_float_param->y + (g_param_z2.x * s_tmp2.y) + (g_param_z2.y * s_tmp2.x);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_plus_fractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex old_plus;
    DComplex new_minus;
    old_plus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        fpu_cmplx_mul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    old_plus.x += g_float_param->x;
    fpu_cmplx_mul(&g_tmp_z, &old_plus, &new_minus);
    g_new_z.x = new_minus.x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = new_minus.y + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_minus_fractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex old_sqr;
    DComplex new_minus;
    fpu_cmplx_mul(&g_old_z, &g_old_z, &old_sqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        fpu_cmplx_mul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    old_sqr.x += g_float_param->x;
    fpu_cmplx_mul(&g_tmp_z, &old_sqr, &new_minus);
    g_new_z.x = new_minus.x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = new_minus.y + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_cplx_plus_fractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex new_minus;
    DComplex old_plus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        fpu_cmplx_mul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    old_plus.x += g_float_param->x;
    old_plus.y += g_float_param->y;
    fpu_cmplx_mul(&g_tmp_z, &old_plus, &new_minus);
    fpu_cmplx_mul(&g_param_z2, &s_tmp2, &g_tmp_z);
    g_new_z.x = new_minus.x + g_tmp_z.x;
    g_new_z.y = new_minus.y + g_tmp_z.y;
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_cplx_minus_fractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex old_sqr;
    DComplex new_minus;
    fpu_cmplx_mul(&g_old_z, &g_old_z, &old_sqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        fpu_cmplx_mul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    old_sqr.x += g_float_param->x;
    old_sqr.y += g_float_param->y;
    fpu_cmplx_mul(&g_tmp_z, &old_sqr, &new_minus);
    fpu_cmplx_mul(&g_param_z2, &s_tmp2, &g_tmp_z);
    g_new_z.x = new_minus.x + g_tmp_z.x;
    g_new_z.y = new_minus.y + g_tmp_z.y;
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int phoenix_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value
    g_temp_sqr_y = sqr(g_old_z.y);
    s_tmp2.x = 0; // use tmp2 as the complex Y value
    s_tmp2.y = 0;
    return 0;
}

int mand_phoenix_per_pixel()
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
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value
    g_temp_sqr_y = sqr(g_old_z.y);
    s_tmp2.x = 0;
    s_tmp2.y = 0;
    return 1; // 1st iteration has been done
}

bool phoenix_per_image()
{
    g_float_param = &g_param_z1;
    g_degree = (int)g_param_z2.x;
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[2] = (double)g_degree;
    if (g_degree == 0)
    {
        g_cur_fractal_specific->orbit_calc = phoenix_orbit;
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        g_cur_fractal_specific->orbit_calc = phoenix_plus_fractal;
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        g_cur_fractal_specific->orbit_calc = phoenix_minus_fractal;
    }

    return true;
}

bool phoenix_cplx_per_image()
{
    g_float_param = &g_param_z1;
    g_degree = (int)g_params[4];
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[4] = (double)g_degree;
    if (g_degree == 0)
    {
        if (g_param_z2.x != 0 || g_param_z2.y != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        else
        {
            g_symmetry = SymmetryType::ORIGIN;
        }
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        g_cur_fractal_specific->orbit_calc = phoenix_fractal_cplx_orbit;
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        else
        {
            g_symmetry = SymmetryType::NONE;
        }
        g_cur_fractal_specific->orbit_calc = phoenix_cplx_plus_fractal;
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        else
        {
            g_symmetry = SymmetryType::NONE;
        }
        g_cur_fractal_specific->orbit_calc = phoenix_cplx_minus_fractal;
    }

    return true;
}

bool mand_phoenix_per_image()
{
    g_float_param = &g_init;
    g_degree = (int)g_param_z2.x;
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[2] = (double)g_degree;
    if (g_degree == 0)
    {
        g_cur_fractal_specific->orbit_calc = phoenix_orbit;
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        g_cur_fractal_specific->orbit_calc = phoenix_plus_fractal;
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        g_cur_fractal_specific->orbit_calc = phoenix_minus_fractal;
    }

    return true;
}

bool mand_phoenix_cplx_per_image()
{
    g_float_param = &g_init;
    g_degree = (int)g_params[4];
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[4] = (double)g_degree;
    if (g_param_z1.y != 0 || g_param_z2.y != 0)
    {
        g_symmetry = SymmetryType::NONE;
    }
    if (g_degree == 0)
    {
        g_cur_fractal_specific->orbit_calc = phoenix_fractal_cplx_orbit;
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        g_cur_fractal_specific->orbit_calc = phoenix_cplx_plus_fractal;
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        g_cur_fractal_specific->orbit_calc = phoenix_cplx_minus_fractal;
    }

    return true;
}

} // namespace id::fractals
