#include "fn_or_fn.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "fractals.h"
#include "fractype.h"
#include "get_julia_attractor.h"
#include "mpmath.h"
#include "trig_fns.h"

bool MandelTrigOrTrigSetup()
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

bool ManlamTrigOrTrigSetup()
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

bool LambdaTrigOrTrigSetup()
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

int LambdaTrigOrTrigFractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if ((LCMPLXmod(g_l_old_z)) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);
    }
    return g_bailout_long();
}

int LambdaTrigOrTrigfpFractal()
{
    /* z = trig0(z)*p1 if mod(old) < p2.x and
           trig1(z)*p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        FPUcplxmul(g_float_param, &g_old_z, &g_new_z);
    }
    return g_bailout_float();
}

bool JuliaTrigOrTrigSetup()
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

int JuliaTrigOrTrigFractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (LCMPLXmod(g_l_old_z) < g_l_param2.x)
    {
        LCMPLXtrig0(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    else
    {
        LCMPLXtrig1(g_l_old_z, g_l_temp);
        LCMPLXadd(*g_long_param, g_l_temp, g_l_new_z);
    }
    return g_bailout_long();
}

int JuliaTrigOrTrigfpFractal()
{
    /* z = trig0(z)+p1 if mod(old) < p2.x and
           trig1(z)+p1 if mod(old) >= p2.x */
    if (CMPLXmod(g_old_z) < g_param_z2.x)
    {
        CMPLXtrig0(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    else
    {
        CMPLXtrig1(g_old_z, g_old_z);
        g_new_z = *g_float_param + g_old_z;
    }
    return g_bailout_float();
}
