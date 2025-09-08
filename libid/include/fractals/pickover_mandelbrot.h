// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

#include <complex>

namespace id::fractals
{

int mandel_trig_plus_exponent_orbit();

int mandel_z_power_orbit();
void pascal_triangle();
void mandel_z_power_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void mandel_z_power_ref_pt_bf(const math::BFComplex &center, math::BFComplex &z);
void mandel_z_power_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);

int mandel_z_to_z_plus_z_pwr_orbit();

bool julia_fn_plus_z_sqrd_per_image();

int trig_plus_z_squared_fractal();
int trig_plus_z_squared_orbit();

} // namespace id::fractals
