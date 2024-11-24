// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "sqr.h"

#include <complex>

inline double mag_squared(const std::complex<double> &z)
{
    return sqr(z.real()) + sqr(z.imag());
}

// Cube: (a + jb) * (a + jb) * (a + jb)
inline std::complex<double> cube(const std::complex<double> &z)
{
    const double r2{z.real() * z.real()};
    const double i2{z.imag() * z.imag()};
    return {z.real() * (r2 - (i2 + i2 + i2)), z.imag() * (r2 + r2 + r2 - i2)};
}

class CComplexFn
{
public:
    void complex_power(std::complex<double> &result, std::complex<double> &base, int exp);
    std::complex<double> complex_polynomial(std::complex<double> z, int degree);
    std::complex<double> complex_invert(std::complex<double> z);
    std::complex<double> complex_square(std::complex<double> z);
};
