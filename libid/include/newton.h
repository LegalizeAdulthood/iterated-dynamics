// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include "cmplx.h"

void invertz2(DComplex *z);

int NewtonFractal2();

bool ComplexNewtonSetup();
int ComplexNewton();
int ComplexBasin();

bool NewtonSetup();
int MPCjulia_per_pixel();
