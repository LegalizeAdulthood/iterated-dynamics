// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "cmplx.h"

extern DComplex              g_marks_coefficient;

bool marks_julia_setup();
bool marks_julia_fp_setup();
int marks_lambda_fractal();
int marks_lambda_fp_fractal();
int marks_cplx_mand();
int marks_mandel_pwr_fp_fractal();
int marks_mandel_pwr_fractal();
int tims_error_fp_fractal();
int tims_error_fractal();
int marks_mandelpwr_per_pixel();
int marksmandel_per_pixel();
int marksmandelfp_per_pixel();
int marks_mandelpwrfp_per_pixel();
int marks_cplx_mand_perp();
