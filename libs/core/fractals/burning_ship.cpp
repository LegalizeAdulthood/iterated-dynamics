// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/burning_ship.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/complex_fn.h>

#include <cmath>
#include <stdexcept>
#include <string>

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

int burning_ship_orbit()
{
    DComplex q;
    const int degree = static_cast<int>(g_params[2]);
    q.x = g_init.x;
    q.y = g_init.y;
    if (degree == 2)
    {
        // (|x| + i|y|)^2 = (|x| + i|y|)*(|x| + i|y|)
        //                = (|x|*|x| - |y|*|y|) + i(|x|*|y| + |x|*|y|)
        //                = (|x|^2 - |y|^2) + i(2*|x|*|y|)
        g_temp_sqr_x = sqr(g_old_z.x);                            // real(z)^2 = |real(z)|^2
        g_temp_sqr_y = sqr(g_old_z.y);                            // imag(z)^2 = |imag(z)|^2
        const double real_imag = std::abs(g_old_z.x * g_old_z.y); // |real(z)*imag(z)| = |real(z)|*|imag(z)|
        g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + q.x;            // real(z)^2 - imag(z)^2 + real(c)
        g_new_z.y = real_imag + real_imag - q.y;                  // |real(z)*imag(z)|*2 - imag(c)
    }
    else if (degree > 2)
    {
        DComplex z;
        z.x = std::abs(g_old_z.x);
        z.y = -std::abs(g_old_z.y);
        pow(&z, degree, &z);
        g_new_z.x = z.x + q.x;
        g_new_z.y = z.y + q.y;
    }
    return g_bailout_float();
}

static double diff_abs(const double c, const double d)
{
    const double cd = c + d;

    if (c >= 0.0)
    {
        return cd >= 0.0 ? d : -d - 2.0 * c;
    }
    return cd > 0.0 ? d + 2.0 * c : -d;
}

void burning_ship_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0)
{
    const int degree = static_cast<int>(g_params[2]);
    const double r{ref.real()};
    const double i{ref.imag()};
    const double r2 = r * r;
    const double i2 = i * i;
    const double a{delta_n.real()};
    const double b{delta_n.imag()};
    const double a2 = a * a;
    const double b2 = b * b;
    const double a0{delta0.real()};
    const double b0{delta0.imag()};

    switch (degree)
    {
    case 2:
        delta_n.real(2.0 * a * r + a2 - 2.0 * b * i - b2);
        delta_n.imag(diff_abs(r * i, r * b + i * a + a * b) * 2);
        delta_n += delta0;
        break;

    case 3:
    {
        double dnr = diff_abs(r, a);
        double ab = r + a;
        dnr = (r * r - 3 * i * i) * dnr                            //
            + (2 * a * r + a2 - 6 * i * b - 3 * b2) * std::abs(ab) //
            + a0;
        double dni = diff_abs(i, b);
        ab = i + b;
        dni = (3 * r * r - i * i) * dni                             //
            + (6 * r * a + 3 * a2 - 2 * i * b - b2) * std::abs(ab) //
            + b0;
        delta_n.imag(dni);
        delta_n.real(dnr);
        break;
    }

    case 4:
    {
        const double dnr = 4 * r2 * r * a //
            + 6 * r2 * a2                 //
            + 4 * r * a2 * a              //
            + a2 * a2                     //
            + 4 * i2 * i * b              //
            + 6 * i2 * b2                 //
            + 4 * i * b2 * b              //
            + b2 * b2                     //
            - 12 * r2 * i * b             //
            - 6 * r2 * b2                 //
            - 12 * r * a * i2             //
            - 24 * r * a * i * b          //
            - 12 * r * a * b2             //
            - 6 * a2 * i2                 //
            - 12 * a2 * i * b             //
            - 6 * a2 * b2                 //
            + a0;
        double dni = diff_abs(r * i, r * b + a * i + a * b);
        dni = 4 * (r2 - i2) * dni                                                              //
            + 4 * std::abs(r * i + r * b + a * i + a * b) * (2 * a * r + a2 - 2 * b * i - b2) //
            + b0;
        delta_n.imag(dni);
        delta_n.real(dnr);
        break;
    }

    case 5:
    {
        double dnr = diff_abs(r, a);
        dnr = dnr * (r * r * r * r - 10 * r * r * i * i + 5 * i * i * i * i) //
            + std::abs(r + a) *
                (4 * r * r * r * a       //
                    + 6 * r * r * a2     //
                    + 4 * r * a2 * a     //
                    + a2 * a2            //
                    - 20 * r2 * i * b    //
                    - 10 * r2 * b2       //
                    - 20 * r * a * i2    //
                    - 40 * r * a * i * b //
                    - 20 * r * a * b2    //
                    - 10 * a2 * i2       //
                    - 20 * a2 * i * b    //
                    - 10 * a2 * b2       //
                    + 20 * i2 * i * b    //
                    + 30 * i2 * b2       //
                    + 20 * i * b2 * b    //
                    + 5 * b2 * b2)       //
            + a0;
        const double dni = diff_abs(i, b) * (5 * r2 * r2 - 10 * r2 * i2 + i2 * i2) + //
            std::abs(i + b) *
                (20 * r2 * r * a         //
                    + 30 * r2 * a2       //
                    + 20 * r * a2 * a    //
                    + 5 * a2 * a2        //
                    - 20 * r2 * i * b    //
                    - 10 * r2 * b2       //
                    - 20 * r * a * i2    //
                    - 40 * r * a * i * b //
                    - 20 * r * a * b2    //
                    - 10 * a2 * i2       //
                    - 20 * a2 * i * b    //
                    - 10 * a2 * b2       //
                    + 4 * i2 * i * b     //
                    + 6 * i2 * b2        //
                    + 4 * i * b2 * b     //
                    + b2 * b2)           //
            + b0;
        delta_n.imag(dni);
        delta_n.real(dnr);
        break;
    }

    default:
        throw std::runtime_error("Unexpected degree " + std::to_string(degree));
    }
}

void burning_ship_ref_pt(const std::complex<double> &center, std::complex<double> &z)
{
    if (const int degree = static_cast<int>(g_params[2]); degree == 2)
    {
        const double real_sqr = sqr(z.real());
        const double imag_sqr = sqr(z.imag());
        const double real = real_sqr - imag_sqr + center.real();
        const double temp = std::abs(2.0 * z.real() * z.imag());
        const double imag = temp + center.imag();
        z.real(real);
        z.imag(imag);
    }
    else if (degree > 2)
    {
        DComplex temp;
        DComplex temp_z;
        temp.x = std::abs(z.real());
        temp.y = std::abs(z.imag());
        pow(&temp, degree, &temp_z);
        z.real(temp_z.x);
        z.imag(temp_z.y);
        z += center;
    }
}

void burning_ship_ref_pt_bf(const BFComplex &center, BFComplex &z)
{
    const int degree = static_cast<int>(g_params[2]);
    BigStackSaver saved;
    BigFloat temp_real{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat real_sqr{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat imag_sqr{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat real_imag{bf_arena(BigLifetime::TEMPORARY)};
    BFComplex z2;
    z2.x = BigFloat{bf_arena(BigLifetime::TEMPORARY)};
    z2.y = BigFloat{bf_arena(BigLifetime::TEMPORARY)};
    z2.x = z.x + z.x;
    z2.y = z.y + z.y;
    if (degree == 2)
    {
        real_sqr = sqr(z.x);
        imag_sqr = sqr(z.y);
        temp_real = real_sqr - imag_sqr;
        z.x = temp_real + center.x;
        real_imag = z2.x * z.y;
        real_imag = abs(real_imag);
        z.y = real_imag + center.y;
    }
    else if (degree > 2)
    {
        z2.x = abs(z.x);
        z2.y = abs(z.y);
        g_new_z_bf = power(z2, degree);
        z.x = g_new_z_bf.x + center.x;
        z.y = g_new_z_bf.y + center.y;
    }
}

int burning_ship_bf_fractal()
{
    const int degree = static_cast<int>(g_params[2]);
    BigStackSaver saved;

    if (degree == 2) // Burning Ship
    {
        g_tmp_sqr_x_bf = sqr(g_old_z_bf.x);
        g_tmp_sqr_y_bf = sqr(g_old_z_bf.y);
        g_bf_tmp = g_tmp_sqr_x_bf - g_tmp_sqr_y_bf;
        g_new_z_bf.x = g_bf_tmp + g_param_z_bf.x;
        g_bf_tmp = g_old_z_bf.x * g_old_z_bf.y;
        g_bf_tmp += g_bf_tmp;
        g_bf_tmp = abs(g_bf_tmp);
        g_new_z_bf.y = g_bf_tmp - g_param_z_bf.y;
    }
    else if (degree > 2) // Burning Ship to higher power
    {
        BFComplex big_z;
        big_z.x = BigFloat{bf_arena(BigLifetime::TEMPORARY)};
        big_z.y = BigFloat{bf_arena(BigLifetime::TEMPORARY)};
        big_z.x = abs(g_old_z_bf.x);
        big_z.y = abs(g_old_z_bf.y);
        big_z.y = -big_z.y;
        g_new_z_bf = power(big_z, degree);
        g_new_z_bf.x += g_param_z_bf.x;
        g_new_z_bf.y += g_param_z_bf.y;
    }
    return g_bailout_bigfloat();
}

} // namespace id::fractals
