#include "lambda_fn.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "mpmath.h"

#include <cmath>

bool LambdaTrigSetup()
{
    bool const isinteger = g_cur_fractal_specific->isinteger != 0;
    if (isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal;
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal;
    }
    switch (g_trig_index[0])
    {
    case trig_fn::SIN:
    case trig_fn::COSXX:
    case trig_fn::COS:
        g_symmetry = symmetry_type::PI_SYM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal1;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal1;
        }
        break;
    case trig_fn::SINH:
    case trig_fn::COSH:
        g_symmetry = symmetry_type::ORIGIN;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal2;
        }
        break;
    case trig_fn::SQR:
        g_symmetry = symmetry_type::ORIGIN;
        break;
    case trig_fn::EXP:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LongLambdaexponentFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaexponentFractal;
        }
        g_symmetry = symmetry_type::NONE;
        break;
    case trig_fn::LOG:
        g_symmetry = symmetry_type::NONE;
        break;
    default:   // default for additional functions
        g_symmetry = symmetry_type::ORIGIN;
        break;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    if (isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

int LambdaTrigFractal()
{
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2 || labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);           // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z);   // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal()
{
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2 || fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

// bailouts are different for different trig functions
int LambdaTrigFractal1()
{
    // sin,cos
    if (labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);               // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z); // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal1()
{
    // sin,cos
    if (std::fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);                // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z); // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

int LambdaTrigFractal2()
{
    // sinh,cosh
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);               // ltmp = trig(lold)
    LCMPLXmult(*g_long_param, g_l_temp, g_l_new_z); // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal2()
{
    // sinh,cosh
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}
