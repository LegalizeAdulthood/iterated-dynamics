#include "bailout_formula.h"

#include "debug_flags.h"
#include "fracsuba.h"
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
        g_bailout_float = fpMODbailout;
        g_bailout_long = asmlMODbailout;
        g_bailout_bignum = bnMODbailout;
        g_bailout_bigfloat = bfMODbailout;
        break;

    case bailouts::Real:
        g_bailout_float = fpREALbailout;
        g_bailout_long = asmlREALbailout;
        g_bailout_bignum = bnREALbailout;
        g_bailout_bigfloat = bfREALbailout;
        break;

    case bailouts::Imag:
        g_bailout_float = fpIMAGbailout;
        g_bailout_long = asmlIMAGbailout;
        g_bailout_bignum = bnIMAGbailout;
        g_bailout_bigfloat = bfIMAGbailout;
        break;

    case bailouts::Or:
        g_bailout_float = fpORbailout;
        g_bailout_long = asmlORbailout;
        g_bailout_bignum = bnORbailout;
        g_bailout_bigfloat = bfORbailout;
        break;

    case bailouts::And:
        g_bailout_float = fpANDbailout;
        g_bailout_long = asmlANDbailout;
        g_bailout_bignum = bnANDbailout;
        g_bailout_bigfloat = bfANDbailout;
        break;

    case bailouts::Manh:
        g_bailout_float = fpMANHbailout;
        g_bailout_long = asmlMANHbailout;
        g_bailout_bignum = bnMANHbailout;
        g_bailout_bigfloat = bfMANHbailout;
        break;

    case bailouts::Manr:
        g_bailout_float = fpMANRbailout;
        g_bailout_long = asmlMANRbailout;
        g_bailout_bignum = bnMANRbailout;
        g_bailout_bigfloat = bfMANRbailout;
        break;
    }
}
