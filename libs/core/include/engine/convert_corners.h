// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>

#include <config/port.h>

namespace id::engine
{

void cvt_corners(double ctr_x, double ctr_y, LDouble mag, double x_mag_factor, double rot, double skew);
void cvt_corners_bf(const math::BigFloat &ctr_x, const math::BigFloat &ctr_y, LDouble mag, double x_mag_factor,
    double rot, double skew);

} // namespace id::engine
