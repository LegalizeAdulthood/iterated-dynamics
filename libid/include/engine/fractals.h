// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"
#include "math/big.h"

#include <complex>

extern int                   g_basin;
extern int                   g_bit_shift_less_1;
extern int                   g_c_exponent;
extern double                g_cos_x;
extern int                   g_degree;
extern DComplex *            g_float_param;
extern long                  g_fudge_half;
extern int                   g_max_color;
extern DComplex              g_param_z1;
extern DComplex              g_param_z2;
extern DComplex              g_power_z;
extern double                g_quaternion_c;
extern double                g_quaternion_ci;
extern double                g_quaternion_cj;
extern double                g_quaternion_ck;
extern double                g_sin_x;
extern double                g_temp_sqr_x;
extern double                g_temp_sqr_y;

// Raise complex number (base) to the (exp) power, storing the result in complex (result).
void pow(DComplex *base, int exp, DComplex *result);
int pow(LComplex *base, int exp, LComplex *result, int bit_shift);
int julia_fp_fractal();
int burning_ship_fp_fractal();
int float_cmplx_z_power_fractal();
int mandel_fp_per_pixel();
void mandel_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void mandel_ref_pt_bf(const BFComplex &center, BFComplex &z);
void mandel_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);
int julia_fp_per_pixel();
int other_mandel_fp_per_pixel();
int other_julia_fp_per_pixel();
