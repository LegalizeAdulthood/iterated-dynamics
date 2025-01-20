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
#include "math/mpmath.h"

#include "engine/id_data.h"
#include "io/loadfile.h"
#include "math/fpu087.h"
#include "ui/cmdfiles.h"

#include <cassert>
#include <cmath>

static MP s_ans{};

bool g_mp_overflow{};
MP g_mp_one{};
MPC g_mpc_one{{0x3fff, 0X80000000L}, {0, 0L}};

MP *mp_abs(MP x)
{
    s_ans = x;
    s_ans.exp &= 0x7fff;
    return &s_ans;
}

MPC mpc_sqr(MPC x)
{
    MPC z;

    z.x = *mp_sub(*mp_mul(x.x, x.x), *mp_mul(x.y, x.y));
    z.y = *mp_mul(x.x, x.y);
    z.y.exp++;
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
    MP mod = mpc_mod(y);
    y.y.exp ^= 0x8000;
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
        zz.y.exp++;
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
    if (mp_cmp(x.x, y.x) || mp_cmp(x.y, y.y))
    {
        MPC z;
        z.x = mpc_mod(x);
        z.y = mpc_mod(y);
        return mp_cmp(z.x, z.y);
    }
    return 0;
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

long exp_float14(long xx)
{
    static float fLogTwo = 0.6931472F;
    int f = 23 - (int) reg_float_to_fg(reg_div_float(xx, *(long *) &fLogTwo), 0);
    long ans = exp_fudged(reg_float_to_fg(xx, 16), f);
    return reg_fg_to_float(ans, (char)f);
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
