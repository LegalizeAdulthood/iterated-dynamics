// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"
#include "big.h"

void cvtcorners(double Xctr, double Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew);
void cvtcornersbf(bf_t Xctr, bf_t Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew);
