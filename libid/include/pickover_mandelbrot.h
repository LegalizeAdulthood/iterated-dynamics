// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"

#include <complex>

int FloatTrigPlusExponentFractal();
int LongTrigPlusExponentFractal();

int longZpowerFractal();
int floatZpowerFractal();
void pascal_triangle();
void mandel_z_power_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void mandel_z_power_ref_pt(const BFComplex &center, BFComplex &z);
void mandel_z_power_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);

int floatZtozPluszpwrFractal();

bool JuliafnPlusZsqrdSetup();
