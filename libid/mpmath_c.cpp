// SPDX-License-Identifier: GPL-3.0-only
//
/* (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
     All rights reserved.

   Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     405-C Queen St. Suite #181
     Southington, CT 06489
     (203) 276-9721
*/
#include "mpmath_c.h"

#include "cmdfiles.h"
#include "fpu087.h"
#include "id_data.h"
#include "loadfile.h"

#include <cassert>
#include <cmath>

static MP s_ans{};
static double s_mlf{};
static unsigned long s_lf{};

bool g_mp_overflow{};
MP g_mp_one{};
MPC g_mpc_one{{0x3fff, 0X80000000L}, {0, 0L}};
std::vector<Byte> g_log_map_table;
long g_log_map_table_max_size{};
bool g_log_map_calculate{};

MP *mp_abs(MP x)
{
    s_ans = x;
    s_ans.Exp &= 0x7fff;
    return &s_ans;
}

MPC mpc_sqr(MPC x)
{
    MPC z;

    z.x = *mp_sub(*mp_mul(x.x, x.x), *mp_mul(x.y, x.y));
    z.y = *mp_mul(x.x, x.y);
    z.y.Exp++;
    return z;
}

MPC mpc_mul(MPC x, MPC y)
{
    MPC z;

    z.x = *mp_sub(*mp_mul(x.x, y.x), *mp_mul(x.y, y.y));
    z.y = *mp_add(*mp_mul(x.x, y.y), *mp_mul(x.y, y.x));
    return z;
}

MPC mpc_div(MPC x, MPC y)
{
    MP mod;

    mod = mpc_mod(y);
    y.y.Exp ^= 0x8000;
    y.x = *mp_div(y.x, mod);
    y.y = *mp_div(y.y, mod);
    return mpc_mul(x, y);
}

MPC mpc_add(MPC x, MPC y)
{
    MPC z;

    z.x = *mp_add(x.x, y.x);
    z.y = *mp_add(x.y, y.y);
    return z;
}

MPC mpc_sub(MPC x, MPC y)
{
    MPC z;

    z.x = *mp_sub(x.x, y.x);
    z.y = *mp_sub(x.y, y.y);
    return z;
}

MPC mpc_pow(MPC x, int exp)
{
    MPC z;
    MPC zz;

    if (exp & 1)
    {
        z = x;
    }
    else
    {
        z = g_mpc_one;
    }
    exp >>= 1;
    while (exp)
    {
        zz.x = *mp_sub(*mp_mul(x.x, x.x), *mp_mul(x.y, x.y));
        zz.y = *mp_mul(x.x, x.y);
        zz.y.Exp++;
        x = zz;
        if (exp & 1)
        {
            zz.x = *mp_sub(*mp_mul(z.x, x.x), *mp_mul(z.y, x.y));
            zz.y = *mp_add(*mp_mul(z.x, x.y), *mp_mul(z.y, x.x));
            z = zz;
        }
        exp >>= 1;
    }
    return z;
}

int mpc_cmp(MPC x, MPC y)
{
    MPC z;

    if (mp_cmp(x.x, y.x) || mp_cmp(x.y, y.y))
    {
        z.x = mpc_mod(x);
        z.y = mpc_mod(y);
        return mp_cmp(z.x, z.y);
    }
    else
    {
        return 0;
    }
}

DComplex mpc_to_cmplx(MPC x)
{
    DComplex z;

    z.x = *mp_to_d(x.x);
    z.y = *mp_to_d(x.y);
    return z;
}

MPC cmplx_to_mpc(DComplex z)
{
    MPC x;

    x.x = *d_to_mp(z.x);
    x.y = *d_to_mp(z.y);
    return x;
}

DComplex complex_power(DComplex xx, DComplex yy)
{
    DComplex z;
    DComplex cLog;
    DComplex t;

    if (!g_ld_check)
    {
        if (xx.x == 0 && xx.y == 0)
        {
            z.y = 0.0;
            z.x = z.y;
            return z;
        }
    }

    fpu_cmplx_log(&xx, &cLog);
    fpu_cmplx_mul(&cLog, &yy, &t);
    fpu_cmplx_exp(&t, &z);
    return z;
}

/*

  The following Complex function routines added by Tim Wegner November 1994.

*/

// rz=Arcsin(z)=-i*Log{i*z+sqrt(1-z*z)}
void asin_z(DComplex z, DComplex *rz)
{
    DComplex tempz1, tempz2;

    fpu_cmplx_mul(&z, &z, &tempz1);
    tempz1.x = 1 - tempz1.x;
    tempz1.y = -tempz1.y;  // tempz1 = 1 - tempz1
    tempz1 = complex_sqrt_float(tempz1);

    tempz2.x = -z.y;
    tempz2.y = z.x;                // tempz2 = i*z
    tempz1.x += tempz2.x;
    tempz1.y += tempz2.y;    // tempz1 += tempz2
    fpu_cmplx_log(&tempz1, &tempz1);
    rz->x = tempz1.y;
    rz->y = -tempz1.x;           // rz = (-i)*tempz1
}   // end. Arcsinz

// rz=Arccos(z)=-i*Log{z+sqrt(z*z-1)}
void acos_z(DComplex z, DComplex *rz)
{
    DComplex temp;

    fpu_cmplx_mul(&z, &z, &temp);
    temp.x -= 1;                                 // temp = temp - 1
    temp = complex_sqrt_float(temp);

    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp

    fpu_cmplx_log(&temp, &temp);
    rz->x = temp.y;
    rz->y = -temp.x;              // rz = (-i)*tempz1
}   // end. Arccosz

void asinh_z(DComplex z, DComplex *rz)
{
    DComplex temp;

    fpu_cmplx_mul(&z, &z, &temp);
    temp.x += 1;                                 // temp = temp + 1
    temp = complex_sqrt_float(temp);
    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp
    fpu_cmplx_log(&temp, rz);
}  // end. Arcsinhz

// rz=Arccosh(z)=Log(z+sqrt(z*z-1)}
void acosh_z(DComplex z, DComplex *rz)
{
    DComplex tempz;
    fpu_cmplx_mul(&z, &z, &tempz);
    tempz.x -= 1;                              // tempz = tempz - 1
    tempz = complex_sqrt_float(tempz);
    tempz.x = z.x + tempz.x;
    tempz.y = z.y + tempz.y;  // tempz = z + tempz
    fpu_cmplx_log(&tempz, rz);
}   // end. Arccoshz

// rz=Arctanh(z)=1/2*Log{(1+z)/(1-z)}
void atanh_z(DComplex z, DComplex *rz)
{
    DComplex temp0, temp1, temp2;

    if (z.x == 0.0)
    {
        rz->x = 0;
        rz->y = std::atan(z.y);
    }
    else
    {
        if (std::fabs(z.x) == 1.0 && z.y == 0.0)
        {
        }
        else if (std::fabs(z.x) < 1.0 && z.y == 0.0)
        {
            rz->x = std::log((1+z.x)/(1-z.x))/2;
            rz->y = 0;
        }
        else
        {
            temp0.x = 1 + z.x;
            temp0.y = z.y;             // temp0 = 1 + z
            temp1.x = 1 - z.x;
            temp1.y = -z.y;            // temp1 = 1 - z
            fpu_cmplx_div(&temp0, &temp1, &temp2);
            fpu_cmplx_log(&temp2, &temp2);
            rz->x = .5*temp2.x;
            rz->y = .5*temp2.y;       // rz = .5*temp2
        }
    }
}   // end. Arctanhz

// rz=Arctan(z)=i/2*Log{(1-i*z)/(1+i*z)}
void atan_z(DComplex z, DComplex *rz)
{
    DComplex temp0, temp1, temp2, temp3;
    if (z.x == 0.0 && z.y == 0.0)
    {
        rz->y = 0;
        rz->x = rz->y;
    }
    else if (z.x != 0.0 && z.y == 0.0)
    {
        rz->x = std::atan(z.x);
        rz->y = 0;
    }
    else if (z.x == 0.0 && z.y != 0.0)
    {
        temp0.x = z.y;
        temp0.y = 0.0;
        atanh_z(temp0, &temp0);
        rz->x = -temp0.y;
        rz->y = temp0.x;              // i*temp0
    }
    else if (z.x != 0.0 && z.y != 0.0)
    {

        temp0.x = -z.y;
        temp0.y = z.x;                  // i*z
        temp1.x = 1 - temp0.x;
        temp1.y = -temp0.y;      // temp1 = 1 - temp0
        temp2.x = 1 + temp0.x;
        temp2.y = temp0.y;       // temp2 = 1 + temp0

        fpu_cmplx_div(&temp1, &temp2, &temp3);
        fpu_cmplx_log(&temp3, &temp3);
        rz->x = -temp3.y*.5;
        rz->y = .5*temp3.x;           // .5*i*temp0
    }
}   // end. Arctanz

#define SinCosFudge 0x10000L
#ifdef LONGSQRT
long lsqrt(long f)
{
    int N;
    unsigned long y0, z;
    static long a = 0, b = 0, c = 0;                  // constant factors

    if (f == 0)
    {
        return f;
    }
    if (f <  0)
    {
        return 0;
    }

    if (a == 0)                                   // one-time compute consts
    {
        a = (long)(g_fudge_factor * .41731);
        b = (long)(g_fudge_factor * .59016);
        c = (long)(g_fudge_factor * .7071067811);
    }

    N  = 0;
    while (f & 0xff000000L)                     // shift arg f into the
    {
        // range: 0.5 <= f < 1
        N++;
        f /= 2;
    }
    while (!(f & 0xff800000L))
    {
        N--;
        f *= 2;
    }

    y0 = a + multiply(b, f,  g_bit_shift);         // Newton's approximation

    z  = y0 + divide(f, y0, g_bit_shift);
    y0 = (z >> 2) + divide(f, z,  g_bit_shift);

    if (N % 2)
    {
        N++;
        y0 = multiply(c, y0, g_bit_shift);
    }
    N /= 2;
    if (N >= 0)
    {
        return y0 <<  N;    // correct for shift above
    }
    else
    {
        return y0 >> -N;
    }
}
#endif

LComplex complex_sqrt_long(long x, long y)
{
    double theta;
    long      maglong, thetalong;
    LComplex    result;

#ifndef LONGSQRT
    double mag = std::sqrt(std::sqrt(((double) multiply(x, x, g_bit_shift))/g_fudge_factor +
                           ((double) multiply(y, y, g_bit_shift))/ g_fudge_factor));
    maglong   = (long)(mag * g_fudge_factor);
#else
    maglong   = lsqrt(lsqrt(multiply(x, x, g_bit_shift)+multiply(y, y, g_bit_shift)));
#endif
    theta     = std::atan2((double) y/g_fudge_factor, (double) x/g_fudge_factor)/2;
    thetalong = (long)(theta * SinCosFudge);
    sin_cos(thetalong, &result.y, &result.x);
    result.x  = multiply(result.x << (g_bit_shift - 16), maglong, g_bit_shift);
    result.y  = multiply(result.y << (g_bit_shift - 16), maglong, g_bit_shift);
    return result;
}

DComplex complex_sqrt_float(double x, double y)
{
    DComplex  result;

    if (x == 0.0 && y == 0.0)
    {
        result.x = 0.0;
        result.y = 0.0;
    }
    else
    {
        double mag = std::sqrt(std::sqrt(x*x + y*y));
        double theta = std::atan2(y, x) / 2;
        sin_cos(&theta, &result.y, &result.x);
        result.x *= mag;
        result.y *= mag;
    }
    return result;
}

/* int LogFlag;
   LogFlag == 1  -- standard log palettes
   LogFlag == -1 -- 'old' log palettes
   LogFlag >  1  -- compress counts < LogFlag into color #1
   LogFlag < -1  -- use quadratic palettes based on square roots && compress
*/
void setup_log_table()
{
    float l, f, c, m;
    unsigned long limit;

    // set up on-the-fly variables
    if (g_log_map_flag > 0)
    {
        // new log function
        s_lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        s_mlf = (g_colors - (s_lf?2:1)) / std::log(static_cast<double>(g_log_map_table_max_size - s_lf));
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        s_mlf = (g_colors - 1) / std::log(static_cast<double>(g_log_map_table_max_size));
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        s_lf = 0 - g_log_map_flag;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        s_mlf = (g_colors - 2) / std::sqrt(static_cast<double>(g_log_map_table_max_size - s_lf));
    }

    if (g_log_map_calculate)
    {
        return; // LogTable not defined, bail out now
    }

    if (!g_log_map_calculate)
    {
        g_log_map_calculate = true;   // turn it on
        for (unsigned long prev = 0U; prev <= (unsigned long)g_log_map_table_max_size; prev++)
        {
            g_log_map_table[prev] = (Byte)log_table_calc((long)prev);
        }
        g_log_map_calculate = false;   // turn it off, again
        return;
    }

    if (g_log_map_flag > -2)
    {
        s_lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        fg_to_float((long)(g_log_map_table_max_size-s_lf), 0, m);
        f_log14(m, m);
        fg_to_float((long)(g_colors-(s_lf?2:1)), 0, c);
        f_div(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = (s_lf ? 2U : 1U); n < (unsigned int)g_colors; n++)
        {
            fg_to_float((long)n, 0, f);
            f_mul16(f, m, f);
            f_exp14(f, l);
            limit = (unsigned long)float_to_fg(l, 0) + s_lf;
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (Byte)n;
            }
        }
    }
    else
    {
        s_lf = 0 - g_log_map_flag;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        fg_to_float((long)(g_log_map_table_max_size-s_lf), 0, m);
        f_sqrt14(m, m);
        fg_to_float((long)(g_colors-2), 0, c);
        f_div(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = 2U; n < (unsigned int)g_colors; n++)
        {
            fg_to_float((long)n, 0, f);
            f_mul16(f, m, f);
            f_mul16(f, f, l);
            limit = (unsigned long)(float_to_fg(l, 0) + s_lf);
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (Byte)n;
            }
        }
    }
    g_log_map_table[0] = 0;
    if (g_log_map_flag != -1)
    {
        for (unsigned long sptop = 1U; sptop < (unsigned long)g_log_map_table_max_size; sptop++)   // spread top to incl unused colors
        {
            if (g_log_map_table[sptop] > g_log_map_table[sptop-1])
            {
                g_log_map_table[sptop] = (Byte)(g_log_map_table[sptop-1]+1);
            }
        }
    }
}

long log_table_calc(long citer)
{
    long ret = 0;

    if (g_log_map_flag == 0 && !g_iteration_ranges_len)   // Oops, how did we get here?
    {
        return citer;
    }
    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        return g_log_map_table[(long)std::min(citer, g_log_map_table_max_size)];
    }

    if (g_log_map_flag > 0)
    {
        // new log function
        if ((unsigned long)citer <= s_lf + 1)
        {
            ret = 1;
        }
        else if ((citer - s_lf)/std::log(static_cast<double>(citer - s_lf)) <= s_mlf)
        {
            ret = (long)(citer - s_lf);
        }
        else
        {
            ret = (long)(s_mlf * std::log(static_cast<double>(citer - s_lf))) + 1;
        }
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        if (citer == 0)
        {
            ret = 1;
        }
        else
        {
            ret = (long)(s_mlf * std::log(static_cast<double>(citer))) + 1;
        }
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        if ((unsigned long)citer <= s_lf)
        {
            ret = 1;
        }
        else if ((unsigned long)(citer - s_lf) <= (unsigned long)(s_mlf * s_mlf))
        {
            ret = (long)(citer - s_lf + 1);
        }
        else
        {
            ret = (long)(s_mlf * std::sqrt(static_cast<double>(citer - s_lf))) + 1;
        }
    }
    return ret;
}

long exp_float14(long xx)
{
    static float fLogTwo = 0.6931472F;
    int f;
    long Ans;

    f = 23 - (int)reg_float_to_fg(reg_div_float(xx, *(long*)&fLogTwo), 0);
    Ans = exp_fudged(reg_float_to_fg(xx, 16), f);
    return reg_fg_to_float(Ans, (char)f);
}

/*
d2MP386     PROC     uses si di, x:QWORD
   mov   si, WORD PTR [x+6]
.386
   mov   edx, DWORD PTR [x+4]
   mov   eax, DWORD PTR [x]

   mov   ebx, edx
   shl   ebx, 1
   or    ebx, eax
   jnz   NonZero

   xor   si, si
   xor   edx, edx
   jmp   StoreAns

NonZero:
   shl   si, 1
   pushf
   shr   si, 4
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   shld  edx, eax, 12
   stc
   rcr   edx, 1

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, edx

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
d2MP386     ENDP
*/
MP *d_to_mp(double x)
{
    // TODO: implement
    assert(!"d2MP386 called.");
    return &s_ans;
}

/*
MP2d386     PROC     uses si di, xExp:WORD, xMant:DWORD
   sub   xExp, (1 SHL 14) - (1 SHL 10)
.386
   jo    Overflow

   mov   bx, xExp
   and   bx, 0111100000000000b
   jz    InRangeOfDouble

Overflow:
   mov   g_mp_overflow, 1
   xor   eax, eax
   xor   edx, edx
   jmp   StoreAns

InRangeOfDouble:
   mov   bx, xExp
   mov   ax, bx
   shl   bx, 5
   shl   ax, 1
   rcr   bx, 1
   shr   bx, 4

   mov   edx, xMant
   shl   edx, 1
   xor   eax, eax
   shrd  eax, edx, 12
   shrd  edx, ebx, 12

StoreAns:
   mov   DWORD PTR Double+4, edx
   mov   DWORD PTR Double, eax

   lea   ax, Double
   mov   dx, ds
.8086
   ret
MP2d386     ENDP
*/
double *mp_to_d(MP x)
{
    // TODO: implement
    static double ans = 0.0;
    assert(!"MP2d386 called.");
    return &ans;
}

/*
MPadd386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   si, xExp
.386
   mov   eax, xMant

   mov   di, yExp
   mov   edx, yMant

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  eax, edx

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SameMag:
   add   eax, edx
   jnc   StoreAns

   rcr   eax, 1
   add   si, 1
   jno   StoreAns

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   si, si
   xor   edx, edx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   eax, edx
   jg    SubtractNumbers
   je    ZeroAns

   xor   si, 8000h
   xchg  eax, edx
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  eax, edx
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SubtractNumbers:
   sub   eax, edx
   bsr   ecx, eax
   neg   cx
   add   cx, 31
   shl   eax, cl
   sub   si, cx
   jo    Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, eax

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPadd386    ENDP
*/
MP *mp_add(MP x, MP y)
{
    // TODO: implement
    assert(!"MPadd386 called.");
    return &s_ans;
}

/*
MPcmp386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, 0
   mov   di, si
.386
   mov   ax, xExp
   mov   edx, xMant
   mov   bx, yExp
   mov   ecx, yMant
   or    ax, ax
   jns   AtLeastOnePos

   or    bx, bx
   jns   AtLeastOnePos

   mov   si, 1
   and   ah, 7fh
   and   bh, 7fh

AtLeastOnePos:
   cmp   ax, bx
   jle   Cmp1

   mov   di, 1
   jmp   ChkRev

Cmp1:
   je    Cmp2

   mov   di, -1
   jmp   ChkRev

Cmp2:
   cmp   edx, ecx
   jbe   Cmp3

   mov   di, 1
   jmp   ChkRev

Cmp3:
   je    ChkRev

   mov   di, -1

ChkRev:
        or    si, si
   jz    ExitCmp

   neg   di

ExitCmp:
   mov   ax, di
.8086
   ret
MPcmp386    ENDP
*/
int mp_cmp(MP x, MP y)
{
    // TODO: implement
    assert(!"MPcmp386 called.");
    return 0;
}

/*
MPdiv386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   sub   ax, bx
   jno   NoOverflow

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   eax, eax
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   xor   eax, eax
   mov   edx, xMant
   mov   ecx, yMant
   or    edx, edx
   jz    ZeroAns

   or    ecx, ecx
   jz    Overflow

   cmp   edx, ecx
   jl    Divide

   shr   edx, 1
   rcr   eax, 1
   add   Ans.Exp, 1
   jo    Overflow

Divide:
   div   ecx

StoreMant:
   mov   Ans.Mant, eax
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPdiv386    ENDP
*/
MP *mp_div(MP x, MP y)
{
    // TODO: implement
    assert(!"MPdiv386 called.");
    return &s_ans;
}

/*
MPmul386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    WORD PTR [xMant+2], 0
   jz    ZeroAns
   or    WORD PTR [yMant+2], 0
   jz    ZeroAns

   mov   g_mp_overflow, 1

ZeroAns:
   xor   edx, edx
   mov   Ans.Exp, dx
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   eax, xMant
   mov   edx, yMant
   or    eax, eax
   jz    ZeroAns

   or    edx, edx
   jz    ZeroAns

   mul   edx

   or    edx, edx
   js    StoreMant

   shld  edx, eax, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   Ans.Mant, edx
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPmul386    ENDP
*/
MP *mp_mul(MP x, MP y)
{
    // TODO: implement
    assert(!"MPmul386 called.");
    return &s_ans;
}

/*
fg2MP386    PROC     x:DWORD, fg:WORD
   mov   bx, 1 SHL 14 + 30
   sub   bx, fg
.386
   mov   edx, x

   or    edx, edx
   jnz   ChkNegMP

   mov   bx, dx
   jmp   StoreAns

ChkNegMP:
   jns   BitScanRight

   or    bh, 80h
   neg   edx

BitScanRight:
   bsr   ecx, edx
   neg   cx
   add   cx, 31
   sub   bx, cx
   shl   edx, cl

StoreAns:
   mov   Ans.Exp, bx
   mov   Ans.Mant, edx
.8086
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP386    ENDP
*/
MP *fg_to_mp(long x, int fg)
{
    assert(!"fg2MP386 called");
    return &s_ans;
}
