// SPDX-License-Identifier: GPL-3.0-only
//
#include "bailout_formula.h"

#include "fracsuba.h"
#include "fractalb.h"
#include "fractals.h"

bailouts g_bail_out_test{}; // test used for determining bailout
int (*g_bailout_float)(){};
int (*g_bailout_long)(){};
int (*g_bailout_bignum)(){};
int (*g_bailout_bigfloat)(){};

void set_bailout_formula(bailouts test)
{
    switch (test)
    {
    case bailouts::Mod:
        g_bailout_float = fp_mod_bailout;
        g_bailout_long = long_mod_bailout;
        g_bailout_bignum = bnMODbailout;
        g_bailout_bigfloat = bfMODbailout;
        break;

    case bailouts::Real:
        g_bailout_float = fp_real_bailout;
        g_bailout_long = long_real_bailout;
        g_bailout_bignum = bnREALbailout;
        g_bailout_bigfloat = bfREALbailout;
        break;

    case bailouts::Imag:
        g_bailout_float = fp_imag_bailout;
        g_bailout_long = long_imag_bailout;
        g_bailout_bignum = bnIMAGbailout;
        g_bailout_bigfloat = bfIMAGbailout;
        break;

    case bailouts::Or:
        g_bailout_float = fp_or_bailout;
        g_bailout_long = long_or_bailout;
        g_bailout_bignum = bnORbailout;
        g_bailout_bigfloat = bfORbailout;
        break;

    case bailouts::And:
        g_bailout_float = fp_and_bailout;
        g_bailout_long = long_and_bailout;
        g_bailout_bignum = bnANDbailout;
        g_bailout_bigfloat = bfANDbailout;
        break;

    case bailouts::Manh:
        g_bailout_float = fp_manh_bailout;
        g_bailout_long = long_manh_bailout;
        g_bailout_bignum = bnMANHbailout;
        g_bailout_bigfloat = bfMANHbailout;
        break;

    case bailouts::Manr:
        g_bailout_float = fp_manr_bailout;
        g_bailout_long = long_manr_bailout;
        g_bailout_bignum = bnMANRbailout;
        g_bailout_bigfloat = bfMANRbailout;
        break;
    }
}
