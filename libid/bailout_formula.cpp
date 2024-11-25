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
        g_bailout_bignum = bn_mod_bailout;
        g_bailout_bigfloat = bf_mod_bailout;
        break;

    case bailouts::Real:
        g_bailout_float = fp_real_bailout;
        g_bailout_long = long_real_bailout;
        g_bailout_bignum = bn_real_bailout;
        g_bailout_bigfloat = bf_real_bailout;
        break;

    case bailouts::Imag:
        g_bailout_float = fp_imag_bailout;
        g_bailout_long = long_imag_bailout;
        g_bailout_bignum = bn_imag_bailout;
        g_bailout_bigfloat = bf_imag_bailout;
        break;

    case bailouts::Or:
        g_bailout_float = fp_or_bailout;
        g_bailout_long = long_or_bailout;
        g_bailout_bignum = bn_or_bailout;
        g_bailout_bigfloat = bf_or_bailout;
        break;

    case bailouts::And:
        g_bailout_float = fp_and_bailout;
        g_bailout_long = long_and_bailout;
        g_bailout_bignum = bn_and_bailout;
        g_bailout_bigfloat = bf_and_bailout;
        break;

    case bailouts::Manh:
        g_bailout_float = fp_manh_bailout;
        g_bailout_long = long_manh_bailout;
        g_bailout_bignum = bn_manh_bailout;
        g_bailout_bigfloat = bf_manh_bailout;
        break;

    case bailouts::Manr:
        g_bailout_float = fp_manr_bailout;
        g_bailout_long = long_manr_bailout;
        g_bailout_bignum = bn_manr_bailout;
        g_bailout_bigfloat = bf_manr_bailout;
        break;
    }
}
