// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/fn_or_fn.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "math/mpmath.h"
#include "ui/trig_fns.h"

bool mandel_trig_or_trig_setup()
{
    // default symmetry is X_AXIS_NO_PARAM
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if ((g_trig_index[0] == TrigFn::FLIP) || (g_trig_index[1] == TrigFn::FLIP))
    {
        g_symmetry = SymmetryType::NONE;
    }
    return true;
}

bool man_lam_trig_or_trig_setup()
{
    // pseudo
    // default symmetry is X_AXIS
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if (g_trig_index[0] == TrigFn::SQR)
    {
        g_symmetry = SymmetryType::NONE;
    }
    if ((g_trig_index[0] == TrigFn::LOG) || (g_trig_index[1] == TrigFn::LOG))
    {
        g_symmetry = SymmetryType::NONE;
    }
    return true;
}

bool lambda_trig_or_trig_setup()
{
    // default symmetry is ORIGIN
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    if ((g_trig_index[0] == TrigFn::EXP) || (g_trig_index[1] == TrigFn::EXP))
    {
        g_symmetry = SymmetryType::NONE;
    }
    if ((g_trig_index[0] == TrigFn::LOG) || (g_trig_index[1] == TrigFn::LOG))
    {
        g_symmetry = SymmetryType::X_AXIS;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

/* The following four fractals are based on the idea of parallel
   or alternate calculations.  The shift is made when the mod
   reaches a given value.  */

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
        g_symmetry = SymmetryType::NONE;
    }
    if (g_trig_index[0] == TrigFn::FLIP || g_trig_index[1] == TrigFn::FLIP)
    {
        g_symmetry = SymmetryType::NONE;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
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
