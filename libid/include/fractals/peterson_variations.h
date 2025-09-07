// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::fractals
{

extern DComplex              g_marks_coefficient;

bool marks_julia_per_image();
int marks_lambda_orbit();
int marks_cplx_mand_orbit();
int marks_mandel_pwr_orbit();
int tims_error_orbit();
int marks_mandel_per_pixel();
int marks_mandel_pwr_per_pixel();
int marks_cplx_mand_per_pixel();

} // namespace id::fractals
