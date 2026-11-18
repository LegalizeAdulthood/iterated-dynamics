// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/cmplx.h>
#include <math/sqr.h>

#include <algorithm>
#include <complex>
#include <utility>

namespace id::math
{

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

template <typename T>
Complex<T> cube(const Complex<T> &z)
{
    const T real_squared = z.x * z.x;
    const T imag_squared = z.y * z.y;
    const T three{3};
    T real = z.x * (real_squared - three * imag_squared);
    T imag = z.y * (three * real_squared - imag_squared);
    return {std::move(real), std::move(imag)};
}

// base^exp
inline std::complex<double> power(const std::complex<double> &base, int exp)
{
    double xt = base.real();
    double yt = base.imag();

    std::complex<double> result;
    exp = std::max(exp, 0);
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

template <typename T>
Complex<T> power(const Complex<T> &base, int degree)
{
    degree = std::max(degree, 0);
    Complex<T> result;
    if (degree & 1)
    {
        result = base;
    }
    else
    {
        result.x = T{1};
        result.y = T{0};
    }
    degree >>= 1;
    if (degree == 0)
    {
        return result;
    }
    Complex<T> factor;
    factor = base;
    while (degree)
    {
        const T difference = factor.x - factor.y;
        const T sum = factor.x + factor.y;
        T real = difference * sum;
        const T cross = factor.y * factor.x;
        factor.y = cross + cross;
        factor.x = std::move(real);
        if (degree & 1)
        {
            result = factor * result;
        }
        degree >>= 1;
    }
    return result;
}

} // namespace id::math
