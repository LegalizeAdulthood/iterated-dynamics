#include "bailout_formula.h"

#include "debug_flags.h"
#include "fracsuba.h"
#include "fractals.h"

bailouts g_bail_out_test; // test used for determining bailout
int (*g_bailout_float)();
int (*g_bailout_long)();
int (*g_bailout_bignum)();
int (*g_bailout_bigfloat)();

void set_bailout_formula(bailouts test)
{
    switch (test)
    {
    case bailouts::Mod:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpMODbailout;
        }
        else
        {
            g_bailout_float = asmfpMODbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlMODbailout;
        }
        else
        {
            g_bailout_long = asm386lMODbailout;
        }
        g_bailout_bignum = bnMODbailout;
        g_bailout_bigfloat = bfMODbailout;
        break;

    case bailouts::Real:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpREALbailout;
        }
        else
        {
            g_bailout_float = asmfpREALbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlREALbailout;
        }
        else
        {
            g_bailout_long = asm386lREALbailout;
        }
        g_bailout_bignum = bnREALbailout;
        g_bailout_bigfloat = bfREALbailout;
        break;

    case bailouts::Imag:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpIMAGbailout;
        }
        else
        {
            g_bailout_float = asmfpIMAGbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlIMAGbailout;
        }
        else
        {
            g_bailout_long = asm386lIMAGbailout;
        }
        g_bailout_bignum = bnIMAGbailout;
        g_bailout_bigfloat = bfIMAGbailout;
        break;

    case bailouts::Or:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpORbailout;
        }
        else
        {
            g_bailout_float = asmfpORbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlORbailout;
        }
        else
        {
            g_bailout_long = asm386lORbailout;
        }
        g_bailout_bignum = bnORbailout;
        g_bailout_bigfloat = bfORbailout;
        break;

    case bailouts::And:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpANDbailout;
        }
        else
        {
            g_bailout_float = asmfpANDbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlANDbailout;
        }
        else
        {
            g_bailout_long = asm386lANDbailout;
        }
        g_bailout_bignum = bnANDbailout;
        g_bailout_bigfloat = bfANDbailout;
        break;

    case bailouts::Manh:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpMANHbailout;
        }
        else
        {
            g_bailout_float = asmfpMANHbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlMANHbailout;
        }
        else
        {
            g_bailout_long = asm386lMANHbailout;
        }
        g_bailout_bignum = bnMANHbailout;
        g_bailout_bigfloat = bfMANHbailout;
        break;

    case bailouts::Manr:
        if (g_debug_flag == debug_flags::prevent_287_math)
        {
            g_bailout_float = fpMANRbailout;
        }
        else
        {
            g_bailout_float = asmfpMANRbailout;
        }
        if (g_debug_flag == debug_flags::prevent_386_math)
        {
            g_bailout_long = asmlMANRbailout;
        }
        else
        {
            g_bailout_long = asm386lMANRbailout;
        }
        g_bailout_bignum = bnMANRbailout;
        g_bailout_bigfloat = bfMANRbailout;
        break;
    }
}
