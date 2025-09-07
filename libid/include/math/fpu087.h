// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::math
{

constexpr double ID_INFINITY{1.0e+300};

void fpu_cmplx_mul(const DComplex *x, const DComplex *y, DComplex *z); // z = x * y
void fpu_cmplx_div(const DComplex *x, const DComplex *y, DComplex *z); // z = x / y
void fpu_cmplx_log(const DComplex *x, DComplex *z);                    // z = log(x)
void fpu_cmplx_exp(const DComplex *x, DComplex *z);                    // z = e^x
void sin_cos(double angle, double *sine, double *cosine);
void sinh_cosh(double angle, double *sine, double *cosine);

} // namespace id::math
