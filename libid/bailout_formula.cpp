#include "bailout_formula.h"

#include "port.h"
#include "prototyp.h"

#include "fracsuba.h"
#include "fractals.h"
#include "merge_path_names.h"

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
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpMODbailout;
        }
        else
        {
            g_bailout_float = fpMODbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lMODbailout;
        }
        else
        {
            g_bailout_long = asmlMODbailout;
        }
        g_bailout_bignum = bnMODbailout;
        g_bailout_bigfloat = bfMODbailout;
        break;

    case bailouts::Real:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpREALbailout;
        }
        else
        {
            g_bailout_float = fpREALbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lREALbailout;
        }
        else
        {
            g_bailout_long = asmlREALbailout;
        }
        g_bailout_bignum = bnREALbailout;
        g_bailout_bigfloat = bfREALbailout;
        break;

    case bailouts::Imag:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpIMAGbailout;
        }
        else
        {
            g_bailout_float = fpIMAGbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lIMAGbailout;
        }
        else
        {
            g_bailout_long = asmlIMAGbailout;
        }
        g_bailout_bignum = bnIMAGbailout;
        g_bailout_bigfloat = bfIMAGbailout;
        break;

    case bailouts::Or:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpORbailout;
        }
        else
        {
            g_bailout_float = fpORbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lORbailout;
        }
        else
        {
            g_bailout_long = asmlORbailout;
        }
        g_bailout_bignum = bnORbailout;
        g_bailout_bigfloat = bfORbailout;
        break;

    case bailouts::And:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpANDbailout;
        }
        else
        {
            g_bailout_float = fpANDbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lANDbailout;
        }
        else
        {
            g_bailout_long = asmlANDbailout;
        }
        g_bailout_bignum = bnANDbailout;
        g_bailout_bigfloat = bfANDbailout;
        break;

    case bailouts::Manh:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpMANHbailout;
        }
        else
        {
            g_bailout_float = fpMANHbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lMANHbailout;
        }
        else
        {
            g_bailout_long = asmlMANHbailout;
        }
        g_bailout_bignum = bnMANHbailout;
        g_bailout_bigfloat = bfMANHbailout;
        break;

    case bailouts::Manr:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            g_bailout_float = asmfpMANRbailout;
        }
        else
        {
            g_bailout_float = fpMANRbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            g_bailout_long = asm386lMANRbailout;
        }
        else
        {
            g_bailout_long = asmlMANRbailout;
        }
        g_bailout_bignum = bnMANRbailout;
        g_bailout_bigfloat = bfMANRbailout;
        break;
    }
}
