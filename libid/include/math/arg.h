// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/parser.h"
#include "math/cmplx.h"
#include "math/fpu087.h"

namespace id::math
{

struct Arg
{
    DComplex d;
};

extern Arg                  *g_arg1;
extern Arg                  *g_arg2;
extern void                (*g_d_trig0)();
extern void                (*g_d_trig1)();
extern void                (*g_d_trig2)();
extern void                (*g_d_trig3)();

// --------------------------------------------------------------------
// The following functions allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------
inline void cmplx_trig0(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_d_trig0();
    out = g_arg1->d;
}
inline void cmplx_trig1(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_d_trig1();
    out = g_arg1->d;
}
inline void cmplx_trig2(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_d_trig2();
    out = g_arg1->d;
}
inline void cmplx_trig3(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_d_trig3();
    out = g_arg1->d;
}

inline double cmplx_mod(const DComplex &z)
{
    return sqr(z.x) + sqr(z.y);
}
inline void cmplx_sin(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    fractals::d_stk_sin();
    out = g_arg1->d;
}
inline void cmplx_cos(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    fractals::d_stk_cos();
    out = g_arg1->d;
}
inline void cmplx_sinh(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    fractals::d_stk_sinh();
    out = g_arg1->d;
}
inline void cmplx_cosh(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    fractals::d_stk_cosh();
    out = g_arg1->d;
}
inline void cmplx_log(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    fractals::d_stk_log();
    out = g_arg1->d;
}
inline void cmplx_exp(const DComplex &arg, DComplex &out)
{
    fpu_cmplx_exp(arg, out);
}
inline void cmplx_sqr(const DComplex &arg, DComplex &out)
{
    out.x = sqr(arg.x) - sqr(arg.y);
    out.y = (arg.x + arg.x) * arg.y;
}
inline void cmplx_pwr(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    out = complex_power(arg1, arg2);
}
inline void cmplx_mult(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    DComplex tmp;
    tmp.x = arg1.x*arg2.x - arg1.y*arg2.y;
    tmp.y = arg1.x*arg2.y + arg1.y*arg2.x;
    out = tmp;
}
inline void cmplx_recip(const DComplex &arg, DComplex &out)
{
    const double denom = sqr(arg.x) + sqr(arg.y);
    if (denom == 0.0)
    {
        out = {1.0e10, 1.0e10};
    }
    else
    {
        out.x = arg.x / denom;
        out.y = -arg.y / denom;
    }
}

} // namespace id::math
