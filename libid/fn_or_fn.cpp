// SPDX-License-Identifier: GPL-3.0-only
//
#include "fn_or_fn.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "fractals.h"
#include "fractype.h"
#include "get_julia_attractor.h"
#include "mpmath.h"
#include "trig_fns.h"

bool mandel_trig_or_trig_setup()
{
    // default symmetry is X_AXIS_NO_PARAM
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if ((g_trig_index[0] == trig_fn::FLIP) || (g_trig_index[1] == trig_fn::FLIP))
    {
        g_symmetry = symmetry_type::NONE;
    }
    return true;
}

bool man_lam_trig_or_trig_setup()
{
    // psuedo
    // default symmetry is X_AXIS
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if (g_trig_index[0] == trig_fn::SQR)
    {
        g_symmetry = symmetry_type::NONE;
    }
    if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[1] == trig_fn::LOG))
    {
        g_symmetry = symmetry_type::NONE;
    }
    return true;
}

bool lambda_trig_or_trig_setup()
{
    // default symmetry is ORIGIN
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    if ((g_trig_index[0] == trig_fn::EXP) || (g_trig_index[1] == trig_fn::EXP))
    {
        g_symmetry = symmetry_type::NONE;
    }
    if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[1] == trig_fn::LOG))
    {
        g_symmetry = symmetry_type::X_AXIS;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  */

int lambda_trig_or_trig_fractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if ((modulus(g_l_old_z)) < g_l_param2.x)
    {
        trig0(g_l_old_z, g_l_temp);
        g_l_new_z = *g_long_param * g_l_temp;
    }
    else
    {
        trig1(g_l_old_z, g_l_temp);
        g_l_new_z = *g_long_param * g_l_temp;
    }
    return g_bailout_long();
}

int lambda_trig_or_trig_fp_fractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if (cmplx_mod(g_old_z) < g_param_z2.x)
    {
        cmplx_trig0(g_old_z, g_old_z);
        fpu_cmplx_mul(g_float_param, &g_old_z, &g_new_z);
    }
    else
    {
        cmplx_trig1(g_old_z, g_old_z);
        fpu_cmplx_mul(g_float_param, &g_old_z, &g_new_z);
    }
    return g_bailout_float();
}

bool julia_trig_or_trig_setup()
{
    // default symmetry is X_AXIS
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    if (g_param_z1.y != 0.0)
    {
        g_symmetry = symmetry_type::NONE;
    }
    if (g_trig_index[0] == trig_fn::FLIP || g_trig_index[1] == trig_fn::FLIP)
    {
        g_symmetry = symmetry_type::NONE;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

int julia_trig_or_trig_fractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (modulus(g_l_old_z) < g_l_param2.x)
    {
        trig0(g_l_old_z, g_l_temp);
        g_l_new_z = *g_long_param + g_l_temp;
    }
    else
    {
        trig1(g_l_old_z, g_l_temp);
        g_l_new_z = *g_long_param + g_l_temp;
    }
    return g_bailout_long();
}

int julia_trig_or_trig_fp_fractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (cmplx_mod(g_old_z) < g_param_z2.x)
    {
        cmplx_trig0(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    else
    {
        cmplx_trig1(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    return g_bailout_float();
}
