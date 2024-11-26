// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "calcfrac.h"
#include "cmplx.h"
#include "fixed_pt.h"
#include "fpu087.h"
#include "fractals.h"
#include "id_data.h"
#include "parser.h"
#include "sqr.h"
#include "trig_fns.h"

#include <cstdint>
#include <vector>

struct MP
{
    std::int16_t Exp;
    std::uint32_t Mant;
};

struct MPC
{
    MP x;
    MP y;
};

extern bool                  g_log_map_calculate;
extern std::vector<BYTE>     g_log_map_table;
extern long                  g_log_map_table_max_size;
extern bool                  g_mp_overflow;
extern MP                    g_mp_one;
extern MPC                   g_mpc_one;

long exp_float14(long);

inline void f_div(float x, float y, float &z)
{
    *(long*)&z = reg_div_float(*(long*)&x, *(long*)&y);
}
inline void f_mul16(float x, float y, float &z)
{
    *(long*)&z = r16_mul(*(long*)&x, *(long*)&y);
}
inline void f_shift(float x, int shift, float &z)
{
    *(long*)&z = reg_sft_float(*(long*)&x, shift);
}
inline void fg_to_float(int x, long f, float &z)
{
    *(long*)&z = reg_fg_to_float(x, f);
}
inline long float_to_fg(float x, int f)
{
    return reg_float_to_fg(*(long*)&x, f);
}
inline void f_log14(float x, float &z)
{
    *reinterpret_cast<long*>(&z) = reg_fg_to_float(log_float14(*reinterpret_cast<long*>(&x)), 16);
}
inline void f_exp14(float x, float &z)
{
    *(long*)&z = exp_float14(*(long*)&x);
}
inline void f_sqrt14(float x, float &z)
{
    f_log14(x, z);
    f_shift(z, -1, z);
    f_exp14(z, z);
}

union Arg
{
    DComplex d;
    MPC m;
    LComplex l;
};

struct ConstArg
{
    char const *s;
    int len;
    Arg a;
};

extern Arg *g_arg1;
extern Arg *g_arg2;

// --------------------------------------------------------------------
// The following functions allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------
inline long modulus(const LComplex &z)
{
    return lsqr(z.x) + lsqr(z.y);
}
inline void trig0(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    g_ltrig0();
    out = g_arg1->l;
}
inline void trig1(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    g_ltrig1();
    out = g_arg1->l;
}
inline void trig2(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    g_ltrig2();
    out = g_arg1->l;
}
inline void trig3(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    g_ltrig3();
    out = g_arg1->l;
}
inline void cmplx_trig0(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_dtrig0();
    out = g_arg1->d;
}
inline void cmplx_trig1(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_dtrig1();
    out = g_arg1->d;
}
inline void cmplx_trig2(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    g_dtrig2();
    out = g_arg1->d;
}
inline void cmplx_trig3(const DComplex &arg, DComplex &out)
{
    g_arg1->d = (arg);
    g_dtrig3();
    (out) = g_arg1->d;
}
inline void lcmplx_sin(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkSin();
    (out) = g_arg1->l;
}
inline void lcmplx_cos(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkCos();
    out = g_arg1->l;
}
inline void lcmplx_sinh(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkSinh();
    out = g_arg1->l;
}
inline void lcmplx_cosh(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkCosh();
    out = g_arg1->l;
}
inline void lcmplx_log(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkLog();
    out = g_arg1->l;
}
inline void lcmplx_exp(const LComplex &arg, LComplex &out)
{
    g_arg1->l = arg;
    lStkExp();
    out = g_arg1->l;
}
inline void lcmplx_sqr(const LComplex &arg, LComplex &out)
{
    out.x = lsqr(arg.x) - lsqr(arg.y);
    out.y = multiply(arg.x, arg.y, g_bit_shift_less_1);
}
inline void lcmplx_sqr_old(LComplex &out)
{
    out.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    out.x = g_l_temp_sqr_x - g_l_temp_sqr_y;
}
inline void lcmplx_pwr(const LComplex &arg1, const LComplex &arg2, LComplex &out)
{
    g_arg2->l = arg1;
    g_arg1->l = arg2;
    lStkPwr();
    g_arg1++;
    g_arg2++;
    out = g_arg2->l;
}

inline LComplex operator*(const LComplex &lhs, const LComplex &rhs)
{
    const long x = multiply(rhs.x, lhs.x, g_bit_shift) - multiply(rhs.y, lhs.y, g_bit_shift);
    const long y = multiply(rhs.y, lhs.x, g_bit_shift) + multiply(rhs.x, lhs.y, g_bit_shift);
    return {x, y};
}

inline void lcmplx_times_real(const LComplex &arg, long real, LComplex &out)
{
    out.x = multiply(arg.x, real, g_bit_shift);
    out.y = multiply(arg.y, real, g_bit_shift);
}
inline void lcmplx_recip(const LComplex &arg, LComplex &out)
{
    const long denom = lsqr(arg.x) + lsqr(arg.y);
    if (denom == 0L)
    {
        g_overflow = true;
    }
    else
    {
        out.x = divide(arg.x, denom, g_bit_shift);
        out.y = -divide(arg.y, denom, g_bit_shift);
    }
}
inline double cmplx_mod(const DComplex &z)
{
    return sqr(z.x) + sqr(z.y);
}
inline void cmplx_sin(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    dStkSin();
    out = g_arg1->d;
}
inline void cmplx_cos(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    dStkCos();
    out = g_arg1->d;
}
inline void cmplx_sinh(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    dStkSinh();
    out = g_arg1->d;
}
inline void cmplx_cosh(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    dStkCosh();
    out = g_arg1->d;
}
inline void cmplx_log(const DComplex &arg, DComplex &out)
{
    g_arg1->d = arg;
    dStkLog();
    out = g_arg1->d;
}
inline void cmplx_exp(const DComplex &arg, DComplex &out)
{
    fpu_cmplx_exp(&(arg), &(out));
}
inline void cmplx_sqr(const DComplex &arg, DComplex &out)
{
    out.x = sqr(arg.x) - sqr(arg.y);
    out.y = (arg.x + arg.x) * arg.y;
}
inline void cmplx_sqr_old(DComplex &out)
{
    out.y = (g_old_z.x+g_old_z.x) * g_old_z.y;
    out.x = g_temp_sqr_x - g_temp_sqr_y;
}
inline void cmplx_pwr(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    extern DComplex ComplexPower(DComplex, DComplex);
    out = ComplexPower(arg1, arg2);
}
inline void cmplx_mult1(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    g_arg2->d = arg1;
    g_arg1->d = arg2;
    dStkMul();
    g_arg1++;
    g_arg2++;
    out = g_arg2->d;
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
