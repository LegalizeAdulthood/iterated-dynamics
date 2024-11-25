// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"
#include "sqr.h"

#include <complex>

// |z|^2
inline double mag_squared(const std::complex<double> &z)
{
    return sqr(z.real()) + sqr(z.imag());
}

// Cube: z^3 = (a + jb) * (a + jb) * (a + jb)
inline std::complex<double> cube(const std::complex<double> &z)
{
    const double r2{z.real() * z.real()};
    const double i2{z.imag() * z.imag()};
    return {z.real() * (r2 - (i2 + i2 + i2)), z.imag() * (r2 + r2 + r2 - i2)};
}

// Cube c + jd = (a + jb) * (a + jb) * (a + jb)
inline void cube(BFComplex &out, const BFComplex &in)
{
    BigStackSaver saved;
    bf_t t = alloc_stack(g_r_bf_length + 2);
    bf_t t1 = alloc_stack(g_r_bf_length + 2);
    bf_t t2 = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_real = alloc_stack(g_r_bf_length + 2);
    bf_t sqr_imag = alloc_stack(g_r_bf_length + 2);

    mult_bf(sqr_real, in.x, in.x); // sqr_real = x * x;
    mult_bf(sqr_imag, in.y, in.y); // sqr_imag = y * y;
    inttobf(t, 3);
    mult_bf(t1, t, sqr_imag); // sqr_real + sqr_real + sqr_real
    sub_bf(t2, sqr_real, t1); // sqr_real - (sqr_imag + sqr_imag + sqr_imag)
    mult_bf(out.x, in.x, t2); // c = x * (sqr_real - (sqr_imag + sqr_imag + sqr_imag))

    mult_bf(t1, t, sqr_real); // sqr_imag + sqr_imag + sqr_imag
    sub_bf(t2, t1, sqr_imag); // (sqr_real + sqr_real + sqr_real) - sqr_imag
    mult_bf(out.y, in.y, t2); // d = y * ((sqr_real + sqr_real + sqr_real) - sqr_imag)
}

// base^exp
inline std::complex<double> power(const std::complex<double> &base, int exp)
{
    double xt = base.real();
    double yt = base.imag();

    std::complex<double> result;
    if (exp < 0)
    {
        exp = 0;
    }
    if (exp & 1)
    {
        result.real(xt);
        result.imag(yt);
    }
    else
    {
        result.real(1.0);
        result.imag(0.0);
    }

    exp >>= 1;
    while (exp)
    {
        double t2 = (xt + yt) * (xt - yt);
        yt = xt * yt;
        yt = yt + yt;
        xt = t2;

        if (exp & 1)
        {
            t2 = xt * result.real() - yt * result.imag();
            result.imag(result.imag() * xt + yt * result.real());
            result.real(t2);
        }
        exp >>= 1;
    }
    return result;
}

inline void power(BFComplex &result, const BFComplex &z, int degree)
{
    BigStackSaver saved;
    bf_t t = alloc_stack(g_r_bf_length + 2);
    bf_t t1 = alloc_stack(g_r_bf_length + 2);
    bf_t t2 = alloc_stack(g_r_bf_length + 2);
    bf_t t3 = alloc_stack(g_r_bf_length + 2);
    bf_t t4 = alloc_stack(g_r_bf_length + 2);

    if (degree < 0)
    {
        degree = 0;
    }

    copy_bf(t1, z.x); // BigTemp1 = xt
    copy_bf(t2, z.y); // BigTemp2 = yt

    if (degree & 1)
    {
        copy_bf(result.x, t1); // new.x = result real
        copy_bf(result.y, t2); // new.y = result imag
    }
    else
    {
        inttobf(result.x, 1);
        inttobf(result.y, 0);
    }

    degree >>= 1;
    while (degree)
    {
        sub_bf(t, t1, t2);  // (xt - yt)
        add_bf(t3, t1, t2); // (xt + yt)
        mult_bf(t4, t, t3); // t2 = (xt + yt) * (xt - yt)
        copy_bf(t, t2);
        mult_bf(t3, t, t1); // yt = xt * yt
        add_bf(t2, t3, t3); // yt = yt + yt
        copy_bf(t1, t4);

        if (degree & 1)
        {
            mult_bf(t, t1, result.x);  // xt * result->x
            mult_bf(t3, t2, result.y); // yt * result->y
            sub_bf(t4, t, t3);         // t2 = xt * result->x - yt * result->y
            mult_bf(t, t1, result.y);  // xt * result->y
            mult_bf(t3, t2, result.x); // yt * result->x
            add_bf(result.y, t, t3);   // result->y = result->y * xt + yt * result->x
            copy_bf(result.x, t4);     // result->x = t2
        }
        degree >>= 1;
    }
}
