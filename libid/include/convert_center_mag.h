// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"
#include "big.h"

void cvt_center_mag(double &ctr_x, double &ctr_y, LDBL &mag, double &x_mag_factor, double &rot, double &skew);
void cvt_center_mag_bf(bf_t ctr_x, bf_t ctr_y, LDBL &mag, double &x_mag_factor, double &rot, double &skew);
