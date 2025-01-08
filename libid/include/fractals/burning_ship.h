// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

#include <complex>

int burning_ship_bf_fractal();
int burning_ship_fp_fractal();
void burning_ship_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void burning_ship_ref_pt_bf(const BFComplex &center, BFComplex &z);
void burning_ship_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);
