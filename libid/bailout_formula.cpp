#include "bailout_formula.h"

#include "port.h"
#include "prototyp.h"

#include "fracsuba.h"
#include "fractals.h"
#include "merge_path_names.h"

bailouts g_bail_out_test; // test used for determining bailout
int (*floatbailout)();
int (*longbailout)();
int (*bignumbailout)();
int (*bigfltbailout)();

void set_bailout_formula(bailouts test)
{
    switch (test)
    {
    case bailouts::Mod:
    default:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpMODbailout;
        }
        else
        {
            floatbailout = fpMODbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lMODbailout;
        }
        else
        {
            longbailout = asmlMODbailout;
        }
        bignumbailout = bnMODbailout;
        bigfltbailout = bfMODbailout;
        break;

    case bailouts::Real:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpREALbailout;
        }
        else
        {
            floatbailout = fpREALbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lREALbailout;
        }
        else
        {
            longbailout = asmlREALbailout;
        }
        bignumbailout = bnREALbailout;
        bigfltbailout = bfREALbailout;
        break;

    case bailouts::Imag:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpIMAGbailout;
        }
        else
        {
            floatbailout = fpIMAGbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lIMAGbailout;
        }
        else
        {
            longbailout = asmlIMAGbailout;
        }
        bignumbailout = bnIMAGbailout;
        bigfltbailout = bfIMAGbailout;
        break;

    case bailouts::Or:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpORbailout;
        }
        else
        {
            floatbailout = fpORbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lORbailout;
        }
        else
        {
            longbailout = asmlORbailout;
        }
        bignumbailout = bnORbailout;
        bigfltbailout = bfORbailout;
        break;

    case bailouts::And:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpANDbailout;
        }
        else
        {
            floatbailout = fpANDbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lANDbailout;
        }
        else
        {
            longbailout = asmlANDbailout;
        }
        bignumbailout = bnANDbailout;
        bigfltbailout = bfANDbailout;
        break;

    case bailouts::Manh:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpMANHbailout;
        }
        else
        {
            floatbailout = fpMANHbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lMANHbailout;
        }
        else
        {
            longbailout = asmlMANHbailout;
        }
        bignumbailout = bnMANHbailout;
        bigfltbailout = bfMANHbailout;
        break;

    case bailouts::Manr:
        if (g_debug_flag != debug_flags::prevent_287_math)
        {
            floatbailout = asmfpMANRbailout;
        }
        else
        {
            floatbailout = fpMANRbailout;
        }
        if (g_debug_flag != debug_flags::prevent_386_math)
        {
            longbailout = asm386lMANRbailout;
        }
        else
        {
            longbailout = asmlMANRbailout;
        }
        bignumbailout = bnMANRbailout;
        bigfltbailout = bfMANRbailout;
        break;
    }
}
