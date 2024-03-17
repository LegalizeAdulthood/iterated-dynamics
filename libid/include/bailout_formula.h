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
extern int      (*floatbailout)();
extern int      (*longbailout)();
extern int      (*bignumbailout)();
extern int      (*bigfltbailout)();

void set_bailout_formula(bailouts test);
