// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "sqr.h"

#include <complex>

inline double mag_squared(const std::complex<double> &z)
{
    return sqr(z.real()) + sqr(z.imag());
}

class CComplexFn
{
public:
    std::complex<double> complex_cube(std::complex<double> z);
    void complex_power(std::complex<double> &result, std::complex<double> &base, int exp);
    std::complex<double> complex_polynomial(std::complex<double> z, int degree);
    std::complex<double> complex_invert(std::complex<double> z);
    std::complex<double> complex_square(std::complex<double> z);
};
