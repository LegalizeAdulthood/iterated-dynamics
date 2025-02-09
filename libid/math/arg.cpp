// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/arg.h"

#include "math/fpu087.h"

long exp_float14(long xx)
{
    static float fLogTwo = 0.6931472F;
    int f = 23 - (int) reg_float_to_fg(reg_div_float(xx, *(long *) &fLogTwo), 0);
    long ans = exp_fudged(reg_float_to_fg(xx, 16), f);
    return reg_fg_to_float(ans, (char)f);
}
