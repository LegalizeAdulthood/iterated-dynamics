// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"

#include <complex>

int float_trig_plus_exponent_fractal();
int long_trig_plus_exponent_fractal();

int long_z_power_fractal();
int float_z_power_fractal();
void pascal_triangle();
void mandel_z_power_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void mandel_z_power_ref_pt_bf(const BFComplex &center, BFComplex &z);
void mandel_z_power_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);

int float_z_to_z_plus_z_pwr_fractal();

bool julia_fn_plus_z_sqrd_setup();

int trig_plus_z_squared_fractal();
int trig_plus_z_squared_fp_fractal();
