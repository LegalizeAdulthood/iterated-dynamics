// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"
#include "big.h"

void cvt_center_mag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew);
void cvt_center_mag_bf(bf_t Xctr, bf_t Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew);
