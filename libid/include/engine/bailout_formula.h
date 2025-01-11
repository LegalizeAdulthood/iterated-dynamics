// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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
extern int      (*g_bailout_long)();
extern int      (*g_bailout_bignum)();
extern int      (*g_bailout_bigfloat)();

void set_bailout_formula(Bailout test);
