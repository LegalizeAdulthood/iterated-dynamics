// SPDX-License-Identifier: GPL-3.0-only
//
// Tierazon definitions
#pragma once

#include    <complex>
#include    "fractype.h"
#include    "cmplx.h"
#include    "bailout_formula.h"
#include    "pixel_grid.h"
#include    "fractype.h"
#include    "id.h"
#include    "complex_fn.h"

#define MAXSIZE 1.0e+6
#define MINSIZE 1.0e-6

extern double g_params[];
extern bailouts g_bail_out_test;
extern double g_magnitude_limit;
extern DComplex g_old_z, g_new_z;
extern long g_max_iterations; // try this many iterations
extern long g_color_iter;
extern int g_periodicity_check;

