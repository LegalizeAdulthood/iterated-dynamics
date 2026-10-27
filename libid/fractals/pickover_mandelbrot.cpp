// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "fractals/pickover_mandelbrot.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "engine/trig_fns.h"
#include "fractals/frasetup.h"
#include "math/arg.h"
#include "math/biginit.h"
#include "math/complex_fn.h"
#include "math/fpu087.h"

#include <cmath>

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

namespace
{

std::complex<double> integer_power(const std::complex<double> &z, const int exponent)
{
    if (exponent >= 0)
    {
        return power(z, exponent);
    }
    const std::complex<double> value{power(z, -(exponent + 1)) * z};
    const DComplex powered{value.real(), value.imag()};
    DComplex inverse;
    cmplx_recip(powered, inverse);
    return {inverse.x, inverse.y};
}

std::complex<double> reciprocal_power(const std::complex<double> &z)
{
    if (g_bf_math != BFMathType::NONE)
    {
        return z == std::complex<double>{} ? std::complex<double>{} : std::pow(z, static_cast<double>(g_c_exponent));
    }
    return integer_power(z, g_c_exponent);
}

} // namespace

int mandel_trig_plus_exponent_orbit()
{
    // another Scientific American biomorph type
    // z(n+1) = e**z(n) + trig(z(n)) + C

    if (std::abs(g_old_z.x) >= 6.4e2)
    {
        return 1; // DOMAIN errors
    }
    const double tmp_exp = std::exp(g_old_z.x);
    double sin_y;
    double cos_y;
    sin_cos(g_old_z.y, sin_y, cos_y);
    cmplx_trig0(g_old_z, g_new_z);

    //new =   trig(old) + e**old + C
    g_new_z.x += tmp_exp*cos_y + g_float_param->x;
    g_new_z.y += tmp_exp*sin_y + g_float_param->y;
    return g_bailout_float();
}

int mandel_z_power_orbit()
{
    pow(&g_old_z, g_c_exponent, &g_new_z);
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
        z = integer_power(z, g_c_exponent) + center;
    }
}

void mandel_z_power_ref_pt_bf(const BFComplex &center, BFComplex &z)
{
    BigStackSaver saved;
    BFComplex tmp;
    tmp.x = alloc_stack(g_r_bf_length + 2);
    tmp.y = alloc_stack(g_r_bf_length + 2);
    // The ordinary arbitrary-precision power kernel defines 0^n as 0,
    // including zero and negative exponents.
    if (is_bf_zero(z.x) && is_bf_zero(z.y))
    {
        clear_bf(tmp.x);
        clear_bf(tmp.y);
    }
    else if (g_c_exponent == 3)
    {
        cube(tmp, z);
    }
    else if (g_c_exponent >= 0)
    {
        power(tmp, z, g_c_exponent);
    }
    else
    {
        const BigFloat denominator{alloc_stack(g_r_bf_length + 2)};
        BFComplex powered{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
        power(tmp, z, -(g_c_exponent + 1));
        cmplx_mul_bf(&powered, &tmp, &z);
        square_bf(tmp.x, powered.x);
        square_bf(tmp.y, powered.y);
        add_bf(denominator, tmp.x, tmp.y);
        div_bf(tmp.x, powered.x, denominator);
        div_bf(tmp.y, powered.y, denominator);
        neg_a_bf(tmp.y);
    }
    add_bf(z.x, tmp.x, center.x);
    add_bf(z.y, tmp.y, center.y);
}

void mandel_z_power_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0)
{
    if (delta_n == std::complex<double>{})
    {
        delta_n = delta0;
        return;
    }
    if (g_c_exponent == 0)
    {
        double difference{};
        if (g_bf_math != BFMathType::NONE)
        {
            difference = static_cast<double>(ref + delta_n != std::complex<double>{}) -
                static_cast<double>(ref != std::complex<double>{});
        }
        delta_n = delta0 + difference;
        return;
    }
    if (g_c_exponent == 1)
    {
        delta_n += delta0;
        return;
    }
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
        // Track (ref + delta)^n - ref^n without subtracting nearby powers
        // or materializing binomial coefficients that can overflow.
        std::complex<double> base{ref};
        std::complex<double> difference{delta_n};
        std::complex<double> result{1.0, 0.0};
        std::complex<double> result_difference{};
        const unsigned int degree{
            g_c_exponent < 0 ? 0U - static_cast<unsigned int>(g_c_exponent) : static_cast<unsigned int>(g_c_exponent)};
        for (unsigned int exponent{degree}; exponent > 0; exponent >>= 1)
        {
            if (exponent & 1)
            {
                result_difference = result_difference * (base + difference) + result * difference;
                result *= base;
            }
            if (exponent > 1)
            {
                difference *= base + base + difference;
                base *= base;
            }
        }
        if (g_c_exponent < 0)
        {
            const std::complex<double> value{ref + delta_n};
            const std::complex<double> powered{power(value, -(g_c_exponent + 1)) * value};
            const double reference_norm{std::norm(result)};
            const double value_norm{std::norm(powered)};
            if (reference_norm == 0.0 || value_norm == 0.0 || !std::isfinite(reference_norm) ||
                !std::isfinite(value_norm))
            {
                result_difference = reciprocal_power(ref + delta_n) - reciprocal_power(ref);
            }
            else
            {
                // 1/(R + d) - 1/R, without subtracting nearby reciprocals.
                result_difference = -result_difference / result / powered;
            }
        }
        delta_n = result_difference + delta0;
    }
}

int mandel_z_to_z_plus_z_pwr_orbit()
{
    pow(&g_old_z, static_cast<int>(g_params[2]), &g_new_z);
    g_old_z = pow(g_old_z, g_old_z);
    g_new_z.x = g_new_z.x + g_old_z.x +g_float_param->x;
    g_new_z.y = g_new_z.y + g_old_z.y +g_float_param->y;
    return g_bailout_float();
}

bool julia_fn_plus_z_sqrd_per_image()
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
    return julia_per_image();
}

int trig_plus_z_squared_orbit()
{
    // From Scientific American, July 1989
    // A Biomorph
    // z(n+1) = trig(z(n))+z(n)**2+C

    cmplx_trig0(g_old_z, g_new_z);
    g_new_z.x += g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y += 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}

} // namespace id::fractals
