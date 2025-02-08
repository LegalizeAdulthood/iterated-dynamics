// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

void invertz2(DComplex *z);

int newton_fractal2();

bool complex_newton_setup();
int complex_newton();
int complex_basin();

bool newton_setup();
