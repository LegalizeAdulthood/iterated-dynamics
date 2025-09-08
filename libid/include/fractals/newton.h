// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::fractals
{

void invertz2(math::DComplex *z);

int newton_orbit();

bool complex_newton_per_image();
int complex_newton_orbit();
int complex_basin_orbit();

bool newton_per_image();

} // namespace id::fractals
