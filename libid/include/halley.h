#pragma once

#include "port.h"
#include "mpmath.h"

extern int                   g_halley_a_plus_one_times_degree;
extern int                   g_halley_a_plus_one;
extern MP                    g_halley_mp_a_plus_one_times_degree;
extern MP                    g_halley_mp_a_plus_one;

bool HalleySetup();
int HalleyFractal();
int Halley_per_pixel();
int MPCHalleyFractal();
int MPCHalley_per_pixel();
