// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/burning_ship.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "math/biginit.h"
#include "math/complex_fn.h"

#include <cmath>
#include <stdexcept>
#include <string>

int burning_ship_orbit()
{
    DComplex q;
    const int degree = (int) g_params[2];
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
    const int degree = (int) g_params[2];
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
    const int degree = (int) g_params[2];
    if (degree == 2)
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
    const int degree = (int) g_params[2];
    BigStackSaver saved;
    BigFloat temp_real = alloc_stack(g_r_bf_length + 2);
    BigFloat real_sqr = alloc_stack(g_r_bf_length + 2);
    BigFloat imag_sqr = alloc_stack(g_r_bf_length + 2);
    BigFloat real_imag = alloc_stack(g_r_bf_length + 2);
    BFComplex z2;
    z2.x = alloc_stack(g_r_bf_length + 2);
    z2.y = alloc_stack(g_r_bf_length + 2);
    double_bf(z2.x, z.x);
    double_bf(z2.y, z.y);
    if (degree == 2)
    {
        square_bf(real_sqr, z.x);
        square_bf(imag_sqr, z.y);
        sub_bf(temp_real, real_sqr, imag_sqr);
        add_bf(z.x, temp_real, center.x);
        mult_bf(real_imag, z2.x, z.y);
        abs_bf(real_imag, real_imag);
        add_bf(z.y, real_imag, center.y);
    }
    else if (degree > 2)
    {
        abs_bf(z2.x, z.x);
        abs_bf(z2.y, z.y);
        power(g_new_z_bf, z2, degree);
        add_bf(z.x, g_new_z_bf.x, center.x);
        add_bf(z.y, g_new_z_bf.y, center.y);
    }
}

int burning_ship_bf_fractal()
{
    const int degree = (int) g_params[2];
    BigStackSaver saved;

    if (degree == 2) // Burning Ship
    {
        square_bf(g_tmp_sqr_x_bf, g_old_z_bf.x);
        square_bf(g_tmp_sqr_y_bf, g_old_z_bf.y);
        sub_bf(g_bf_tmp, g_tmp_sqr_x_bf, g_tmp_sqr_y_bf);
        add_bf(g_new_z_bf.x, g_bf_tmp, g_param_z_bf.x);
        mult_bf(g_bf_tmp, g_old_z_bf.x, g_old_z_bf.y);
        double_a_bf(g_bf_tmp);
        abs_a_bf(g_bf_tmp);
        sub_bf(g_new_z_bf.y, g_bf_tmp, g_param_z_bf.y);
    }
    else if (degree > 2) // Burning Ship to higher power
    {
        BFComplex big_z;
        big_z.x = alloc_stack(g_bf_length + 2);
        big_z.y = alloc_stack(g_bf_length + 2);
        abs_bf(big_z.x, g_old_z_bf.x);
        abs_bf(big_z.y, g_old_z_bf.y);
        neg_a_bf(big_z.y);
        power(g_new_z_bf, big_z, degree);
        add_a_bf(g_new_z_bf.x, g_param_z_bf.x);
        add_a_bf(g_new_z_bf.y, g_param_z_bf.y);
    }
    return g_bailout_bigfloat();
}
