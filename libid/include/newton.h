// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "cmplx.h"

void invertz2(DComplex *z);

int newton_fractal2();

bool complex_newton_setup();
int complex_newton();
int complex_basin();

bool newton_setup();
int mpc_julia_per_pixel();

int mpc_newton_fractal();
