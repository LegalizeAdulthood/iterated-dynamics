// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <complex>

class CComplexFn
{
public:
    double sum_squared(std::complex<double> z);
    std::complex<double> complex_cube(std::complex<double> z);
    void complex_power(std::complex<double> &result, std::complex<double> &base, int exp);
    std::complex<double> complex_polynomial(std::complex<double> z, int degree);
    std::complex<double> complex_invert(std::complex<double> z);
    std::complex<double> complex_square(std::complex<double> z);
};
