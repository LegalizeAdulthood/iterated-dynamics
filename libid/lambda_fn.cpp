#include "lambda_fn.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "get_julia_attractor.h"
#include "mpmath.h"
#include "trig_fns.h"

#include <cmath>

int LambdaFractal()
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

int LambdaFPFractal()
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

int LambdaTrigFractal()
{
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2 || labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);     // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

int LambdaTrigfpFractal()
{
    if (std::fabs(g_old_z.x) >= g_magnitude_limit2 || std::fabs(g_old_z.y) >= g_magnitude_limit2)
    {
        return 1;
    }
    CMPLXtrig0(g_old_z, g_tmp_z);              // tmp = trig(old)
    CMPLXmult(*g_float_param, g_tmp_z, g_new_z);   // new = longparm*trig(old)
    g_old_z = g_new_z;
    return 0;
}

// bailouts are different for different trig functions
static int LambdaTrigFractal1()
{
    // sin,cos
    if (labs(g_l_old_z.y) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);     // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

static int LambdaTrigfpFractal1()
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

static int LambdaTrigFractal2()
{
    // sinh,cosh
    if (labs(g_l_old_z.x) >= g_l_magnitude_limit2)
    {
        return 1;
    }
    LCMPLXtrig0(g_l_old_z, g_l_temp);     // ltmp = trig(lold)
    g_l_new_z = *g_long_param * g_l_temp; // lnew = longparm*trig(lold)
    g_l_old_z = g_l_new_z;
    return 0;
}

static int LambdaTrigfpFractal2()
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

static int LongLambdaexponentFractal()
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
    long longtmp = Exp086(g_l_old_z.x);

    g_l_temp.x = multiply(longtmp,      lcosy,   g_bit_shift);
    g_l_temp.y = multiply(longtmp,      lsiny,   g_bit_shift);

    g_l_new_z.x  = multiply(g_long_param->x, g_l_temp.x, g_bit_shift)
              - multiply(g_long_param->y, g_l_temp.y, g_bit_shift);
    g_l_new_z.y  = multiply(g_long_param->x, g_l_temp.y, g_bit_shift)
              + multiply(g_long_param->y, g_l_temp.x, g_bit_shift);
    g_l_old_z = g_l_new_z;
    return 0;
}

static int LambdaexponentFractal()
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

bool MandelTrigSetup()
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
    g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
    switch (g_trig_index[0])
    {
    case trig_fn::SIN:
    case trig_fn::COSXX:
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
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal2;
        }
        break;
    case trig_fn::EXP:
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LongLambdaexponentFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaexponentFractal;
        }
        break;
    case trig_fn::LOG:
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    default:
        g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        break;
    }
    if (isinteger)
    {
        return MandellongSetup();
    }
    else
    {
        return MandelfpSetup();
    }
}
