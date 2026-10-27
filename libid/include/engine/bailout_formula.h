// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{

enum class Bailout
{
    MOD,
    REAL,
    IMAG,
    OR,
    AND,
    MANH,
    MANR
};

extern Bailout    g_bailout_test;
extern int      (*g_bailout_float)();
extern int      (*g_bailout_bignum)();
extern int      (*g_bailout_bigfloat)();

void set_bailout_formula(Bailout test);
bool bailout_reached(Bailout test, double x, double y, double limit, bool strict = false);

} // namespace id::engine
