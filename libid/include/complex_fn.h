// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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
