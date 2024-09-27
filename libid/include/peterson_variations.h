// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include "cmplx.h"

extern DComplex              g_marks_coefficient;

bool MarksJuliaSetup();
bool MarksJuliafpSetup();
int MarksLambdaFractal();
int MarksLambdafpFractal();
int MarksCplxMand();
int MarksMandelPwrfpFractal();
int MarksMandelPwrFractal();
int TimsErrorfpFractal();
int TimsErrorFractal();
int marks_mandelpwr_per_pixel();
int marksmandel_per_pixel();
int marksmandelfp_per_pixel();
int marks_mandelpwrfp_per_pixel();
int MarksCplxMandperp();
