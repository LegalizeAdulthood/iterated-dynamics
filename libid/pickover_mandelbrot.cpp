// SPDX-License-Identifier: GPL-3.0-only
//
#include "pickover_mandelbrot.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "complex_fn.h"
#include "fpu087.h"
#include "fractalb.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"

#include <cmath>

enum
{
    MAX_POWER = 28
};

static long s_pascal_triangle[MAX_POWER]{};

int float_trig_plus_exponent_fractal()
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
    cmplx_trig0(g_old_z, g_new_z);

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

int long_trig_plus_exponent_fractal()
{
    // calculate exp(z)

    // domain check for fast transcendental functions
    if (trig16_check(g_l_old_z.x) || trig16_check(g_l_old_z.y))
    {
        return 1;
    }

    const long longtmp = exp_long(g_l_old_z.x);
    long lcosy;
    long lsiny;
    sin_cos(g_l_old_z.y, &lsiny,  &lcosy);
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += multiply(longtmp,    lcosy,   g_bit_shift) + g_long_param->x;
    g_l_new_z.y += multiply(longtmp,    lsiny,   g_bit_shift) + g_long_param->y;
    return g_bailout_long();
}

int long_z_power_fractal()
{
    if (pow(&g_l_old_z, g_c_exponent, &g_l_new_z, g_bit_shift))
    {
        g_l_new_z.y = 8L << g_bit_shift;
        g_l_new_z.x = g_l_new_z.y;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int float_z_power_fractal()
{
    pow(&g_old_z, g_c_exponent, &g_new_z);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

// Generate Pascal's Triangle coefficients
void pascal_triangle()
{
    long c = 1L;

    for (long j = 0; j <= g_c_exponent; j++)
    {
        if (j == 0)
        {
            c = 1;
        }
        else
        {
            c = c * (g_c_exponent - j + 1) / j;
        }
        s_pascal_triangle[j] = c;
    }
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

void mandel_z_power_ref_pt_bf(const BFComplex &center, BFComplex &z)
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
            cmplx_mul_bf(&tmp, &tmp, &z);
        }
        add_bf(z.x, tmp.x, center.x);
        add_bf(z.y, tmp.y, center.y);
    }
}

void mandel_z_power_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0)
{
    if (g_c_exponent == 3)
    {
        const double r{ref.real()};
        const double i{ref.imag()};
        const double a{delta_n.real()};
        const double b{delta_n.imag()};
        const double a0{delta0.real()};
        const double b0{delta0.imag()};
        const double dnr{       //
            3 * r * r * a       //
            - 6 * r * i * b     //
            - 3 * i * i * a     //
            + 3 * r * a * a     //
            - 3 * r * b * b     //
            - 3 * i * 2 * a * b //
            + a * a * a         //
            - 3 * a * b * b     //
            + a0};
        const double dni{       //
            3 * r * r * b       //
            + 6 * r * i * a     //
            - 3 * i * i * b     //
            + 3 * r * 2 * a * b //
            + 3 * i * a * a     //
            - 3 * i * b * b     //
            + 3 * a * a * b     //
            - b * b * b         //
            + b0};
        delta_n.imag(dni);
        delta_n.real(dnr);
    }
    else
    {
        std::complex<double> zp(1.0, 0.0);
        std::complex<double> sum(0.0, 0.0);
        for (int j = 0; j < g_c_exponent; j++)
        {
            sum += zp * (double) s_pascal_triangle[j];
            sum *= delta_n;
            zp *= ref;
        }
        delta_n = sum;
        delta_n += delta0;
    }
}

int float_z_to_z_plus_z_pwr_fractal()
{
    pow(&g_old_z, (int)g_params[2], &g_new_z);
    g_old_z = complex_power(g_old_z, g_old_z);
    g_new_z.x = g_new_z.x + g_old_z.x +g_float_param->x;
    g_new_z.y = g_new_z.y + g_old_z.y +g_float_param->y;
    return g_bailout_float();
}

bool julia_fn_plus_z_sqrd_setup()
{
    //   static char fnpluszsqrd[] =
    // fn1 ->  sin   cos    sinh  cosh   sqr    exp   log
    // sin    {NONE, ORIGIN, NONE, ORIGIN, ORIGIN, NONE, NONE};

    switch (g_trig_index[0]) // fix sqr symmetry & add additional functions
    {
    case TrigFn::COSXX: // cosxx
    case TrigFn::COSH:
    case TrigFn::SQR:
    case TrigFn::COS:
    case TrigFn::TAN:
    case TrigFn::TANH:
        g_symmetry = SymmetryType::ORIGIN;
        break;
        // default is for NONE symmetry

    default:
        break;
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return julia_long_setup();
    }
    else
    {
        return julia_fp_setup();
    }
}

int trig_plus_z_squared_fractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C
    trig0(g_l_old_z, g_l_new_z);
    g_l_new_z.x += g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y += multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int trig_plus_z_squared_fp_fractal()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C

    cmplx_trig0(g_old_z, g_new_z);
    g_new_z.x += g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y += 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}
