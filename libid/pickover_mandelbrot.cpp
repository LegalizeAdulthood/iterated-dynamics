// SPDX-License-Identifier: GPL-3.0-only
//
#include "pickover_mandelbrot.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "complex_fn.h"
#include "fpu087.h"
#include "fractalb.h"
#include "fractalp.h"
#include "fractype.h"
#include "frasetup.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "trig_fns.h"

#include <cmath>

int FloatTrigPlusExponentFractal()
{
    // another Scientific American biomorph type
    // z(n+1) = e**z(n) + trig(z(n)) + C

    if (std::fabs(g_old_z.x) >= 6.4e2)
    {
        return 1; // DOMAIN errors
    }
    const double tmpexp = std::exp(g_old_z.x);
    double siny;
    double cosy;
    sin_cos(&g_old_z.y, &siny, &cosy);
    CMPLXtrig0(g_old_z, g_new_z);

    //new =   trig(old) + e**old + C
    g_new_z.x += tmpexp*cosy + g_float_param->x;
    g_new_z.y += tmpexp*siny + g_float_param->y;
    return g_bailout_float();
}

inline bool trig16_check(long val)
{
    static constexpr long l16triglim = 8L << 16; // domain limit of fast trig functions

    return labs(val) > l16triglim;
}

int LongTrigPlusExponentFractal()
{
    // calculate exp(z)

    // domain check for fast transcendental functions
    if (trig16_check(g_l_old_z.x) || trig16_check(g_l_old_z.y))
    {
        return 1;
    }

    const long longtmp = Exp086(g_l_old_z.x);
    long lcosy;
    long lsiny;
    sin_cos(g_l_old_z.y, &lsiny,  &lcosy);
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += multiply(longtmp,    lcosy,   g_bit_shift) + g_long_param->x;
    g_l_new_z.y += multiply(longtmp,    lsiny,   g_bit_shift) + g_long_param->y;
    return g_bailout_long();
}

int longZpowerFractal()
{
    if (lcpower(&g_l_old_z, g_c_exponent, &g_l_new_z, g_bit_shift))
    {
        g_l_new_z.y = 8L << g_bit_shift;
        g_l_new_z.x = g_l_new_z.y;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int floatZpowerFractal()
{
    cpower(&g_old_z, g_c_exponent, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

void mandel_z_power_ref_pt(const std::complex<double> &center, std::complex<double> &z)
{
    if (g_c_exponent == 3)
    {
        z = cube(z) + center;
    }
    else
    {
        std::complex<double> tmp{z};
        for (int k = 0; k < g_c_exponent - 1; k++)
        {
            tmp *= z;
        }
        z = tmp + center;
    }
}

void mandel_z_power_ref_pt(const BFComplex &center, BFComplex &z)
{
    BigStackSaver saved;
    BFComplex tmp;
    tmp.x = alloc_stack(g_r_bf_length + 2);
    tmp.y = alloc_stack(g_r_bf_length + 2);
    if (g_c_exponent == 3)
    {
        cube(tmp, z);
        add_bf(z.x, tmp.x, center.x);
        add_bf(z.y, tmp.y, center.y);
    }
    else
    {
        copy_bf(tmp.x, z.x);
        copy_bf(tmp.y, z.y);
        for (int k = 0; k < g_c_exponent - 1; k++)
        {
            cplxmul_bf(&tmp, &tmp, &z);
        }
        add_bf(z.x, tmp.x, center.x);
        add_bf(z.y, tmp.y, center.y);
    }
}

int floatZtozPluszpwrFractal()
{
    cpower(&g_old_z, (int)g_params[2], &g_new_z);
    g_old_z = ComplexPower(g_old_z, g_old_z);
    g_new_z.x = g_new_z.x + g_old_z.x +g_float_param->x;
    g_new_z.y = g_new_z.y + g_old_z.y +g_float_param->y;
    return g_bailout_float();
}

bool JuliafnPlusZsqrdSetup()
{
    //   static char fnpluszsqrd[] =
    // fn1 ->  sin   cos    sinh  cosh   sqr    exp   log
    // sin    {NONE, ORIGIN, NONE, ORIGIN, ORIGIN, NONE, NONE};

    switch (g_trig_index[0]) // fix sqr symmetry & add additional functions
    {
    case trig_fn::COSXX: // cosxx
    case trig_fn::COSH:
    case trig_fn::SQR:
    case trig_fn::COS:
    case trig_fn::TAN:
    case trig_fn::TANH:
        g_symmetry = symmetry_type::ORIGIN;
        break;
        // default is for NONE symmetry

    default:
        break;
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

int TrigPlusZsquaredFractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y += multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int TrigPlusZsquaredfpFractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C

    CMPLXtrig0(g_old_z, g_new_z);
    g_new_z.x += g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y += 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}
