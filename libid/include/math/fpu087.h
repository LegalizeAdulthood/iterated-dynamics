// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

constexpr double ID_INFINITY{1.0e+300};

void fpu_cmplx_mul(const DComplex *x, const DComplex *y, DComplex *z); // z = x * y
void fpu_cmplx_div(const DComplex *x, const DComplex *y, DComplex *z); // z = x / y
void fpu_cmplx_log(const DComplex *x, DComplex *z);                    // z = log(x)
void fpu_cmplx_exp(const DComplex *x, DComplex *z);                    // z = e^x
void sin_cos(const double *angle, double *sine, double *cosine);
void sinh_cosh(const double *angle, double *sine, double *cosine);

void sin_cos(long angle, long *sine, long *cosine);
void sinh_cosh(long angle, long *sine, long *cosine);
long r16_mul(long x, long y);
long reg_float_to_fg(long x, int fudge);
long exp_long(long x);
unsigned long exp_fudged(long x, int fudge);
long reg_div_float(long x, long y);
long log_float14(unsigned long x);
long reg_fg_to_float(long x, int fudge_factor);
long reg_sft_float(long x, int shift);
