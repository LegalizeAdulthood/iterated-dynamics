// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/fixed_pt.h"

int g_bit_shift{};     // fudgefactor
long g_fudge_factor{}; // 2**fudgefactor
double g_fudge_limit{};
bool g_overflow{};
