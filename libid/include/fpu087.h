// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "cmplx.h"

constexpr double ID_INFINITY{1.0e+300};

void fpu_cmplx_mul(DComplex const *x, DComplex const *y, DComplex *z); // z = x * y
void fpu_cmplx_div(DComplex const *x, DComplex const *y, DComplex *z); // z = x / y
void fpu_cmplx_log(DComplex const *x, DComplex *z);                    // z = log(x)
void fpu_cmplx_exp(const DComplex *x, DComplex *z);                    // z = e^x
void sin_cos(double const *angle, double *sine, double *cosine);
void sinh_cosh(double const *angle, double *sine, double *cosine);
void sin_cos(long angle, long *sine, long *cosine);
void sinh_cosh(long angle, long *sine, long *cosine);
long r16_mul(long, long);
long reg_float_to_fg(long, int);
long exp_long(long);
unsigned long exp_fudged(long, int);
long reg_div_float(long, long);
long log_fudged(unsigned long, int);
long log_float14(unsigned long);
long reg_fg_to_float(long, int);
long reg_sft_float(long, int);
