#pragma once

enum class bailouts
{
    Mod,
    Real,
    Imag,
    Or,
    And,
    Manh,
    Manr
};

extern bailouts   g_bail_out_test;
extern int      (*g_bailout_float)();
extern int      (*g_bailout_long)();
extern int      (*g_bailout_bignum)();
extern int      (*g_bailout_bigfloat)();

void set_bailout_formula(bailouts test);
