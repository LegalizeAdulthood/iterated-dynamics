// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

#include <config/port.h>

void cvt_corners(double ctr_x, double ctr_y, LDouble mag, double x_mag_factor, double rot, double skew);
void cvt_corners_bf(BigFloat ctr_x, BigFloat ctr_y, LDouble mag, double x_mag_factor, double rot, double skew);
