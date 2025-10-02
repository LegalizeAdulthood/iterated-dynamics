// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::fractals
{

extern LDouble g_b_const;

int divide_brot5_bf_per_pixel();
int divide_brot5_bn_per_pixel();
int divide_brot5_bn_fractal();
int divide_brot5_orbit_bf();
bool divide_brot5_per_image();
int divide_brot5_per_pixel();
int divide_brot5_orbit();

} // namespace id::fractals
