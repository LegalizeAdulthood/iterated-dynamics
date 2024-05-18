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
#include "port.h"
#include "prototyp.h"

#include "mpmath.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fpu087.h"
#include "fractals.h"
#include "id_data.h"
#include "jiim.h"
#include "loadfile.h"
#include "mpmath_c.h"
#include "parser.h"

#include <algorithm>
#include <cassert>
#include <cmath>

static MP Ans = { 0 };

int g_mp_overflow = 0;

MP *MPsub(MP x, MP y)
{
    y.Exp ^= 0x8000;
    return MPadd(x, y);
}

MP *MPabs(MP x)
{
    Ans = x;
    Ans.Exp &= 0x7fff;
    return &Ans;
}

MPC MPCsqr(MPC x)
{
    MPC z;

    z.x = *MPsub(*MPmul386(x.x, x.x), *MPmul386(x.y, x.y));
    z.y = *MPmul386(x.x, x.y);
    z.y.Exp++;
    return z;
}

MPC MPCmul(MPC x, MPC y)
{
    MPC z;

    z.x = *MPsub(*MPmul386(x.x, y.x), *MPmul386(x.y, y.y));
    z.y = *MPadd(*MPmul386(x.x, y.y), *MPmul386(x.y, y.x));
    return z;
}

MPC MPCdiv(MPC x, MPC y)
{
    MP mod;

    mod = MPCmod(y);
    y.y.Exp ^= 0x8000;
    y.x = *MPdiv386(y.x, mod);
    y.y = *MPdiv386(y.y, mod);
    return MPCmul(x, y);
}

MPC MPCadd(MPC x, MPC y)
{
    MPC z;

    z.x = *MPadd(x.x, y.x);
    z.y = *MPadd(x.y, y.y);
    return z;
}

MPC MPCsub(MPC x, MPC y)
{
    MPC z;

    z.x = *MPsub(x.x, y.x);
    z.y = *MPsub(x.y, y.y);
    return z;
}

MPC g_mpc_one =
{
    {0x3fff, 0x80000000l},
    {0, 0l}
};

MPC MPCpow(MPC x, int exp)
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
        zz.x = *MPsub(*MPmul386(x.x, x.x), *MPmul386(x.y, x.y));
        zz.y = *MPmul386(x.x, x.y);
        zz.y.Exp++;
        x = zz;
        if (exp & 1)
        {
            zz.x = *MPsub(*MPmul386(z.x, x.x), *MPmul386(z.y, x.y));
            zz.y = *MPadd(*MPmul386(z.x, x.y), *MPmul386(z.y, x.x));
            z = zz;
        }
        exp >>= 1;
    }
    return z;
}

int MPCcmp(MPC x, MPC y)
{
    MPC z;

    if (MPcmp386(x.x, y.x) || MPcmp386(x.y, y.y))
    {
        z.x = MPCmod(x);
        z.y = MPCmod(y);
        return MPcmp386(z.x, z.y);
    }
    else
    {
        return 0;
    }
}

DComplex MPC2cmplx(MPC x)
{
    DComplex z;

    z.x = *MP2d386(x.x);
    z.y = *MP2d386(x.y);
    return z;
}

MPC cmplx2MPC(DComplex z)
{
    MPC x;

    x.x = *d2MP(z.x);
    x.y = *d2MP(z.y);
    return x;
}

DComplex ComplexPower(DComplex xx, DComplex yy)
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

    FPUcplxlog(&xx, &cLog);
    FPUcplxmul(&cLog, &yy, &t);
    FPUcplxexp(&t, &z);
    return z;
}

/*

  The following Complex function routines added by Tim Wegner November 1994.

*/

#define Sqrtz(z, rz) (*(rz) = ComplexSqrtFloat((z).x, (z).y))

// rz=Arcsin(z)=-i*Log{i*z+sqrt(1-z*z)}
void Arcsinz(DComplex z, DComplex *rz)
{
    DComplex tempz1, tempz2;

    FPUcplxmul(&z, &z, &tempz1);
    tempz1.x = 1 - tempz1.x;
    tempz1.y = -tempz1.y;  // tempz1 = 1 - tempz1
    Sqrtz(tempz1, &tempz1);

    tempz2.x = -z.y;
    tempz2.y = z.x;                // tempz2 = i*z
    tempz1.x += tempz2.x;
    tempz1.y += tempz2.y;    // tempz1 += tempz2
    FPUcplxlog(&tempz1, &tempz1);
    rz->x = tempz1.y;
    rz->y = -tempz1.x;           // rz = (-i)*tempz1
}   // end. Arcsinz


// rz=Arccos(z)=-i*Log{z+sqrt(z*z-1)}
void Arccosz(DComplex z, DComplex *rz)
{
    DComplex temp;

    FPUcplxmul(&z, &z, &temp);
    temp.x -= 1;                                 // temp = temp - 1
    Sqrtz(temp, &temp);

    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp

    FPUcplxlog(&temp, &temp);
    rz->x = temp.y;
    rz->y = -temp.x;              // rz = (-i)*tempz1
}   // end. Arccosz

void Arcsinhz(DComplex z, DComplex *rz)
{
    DComplex temp;

    FPUcplxmul(&z, &z, &temp);
    temp.x += 1;                                 // temp = temp + 1
    Sqrtz(temp, &temp);
    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp
    FPUcplxlog(&temp, rz);
}  // end. Arcsinhz

// rz=Arccosh(z)=Log(z+sqrt(z*z-1)}
void Arccoshz(DComplex z, DComplex *rz)
{
    DComplex tempz;
    FPUcplxmul(&z, &z, &tempz);
    tempz.x -= 1;                              // tempz = tempz - 1
    Sqrtz(tempz, &tempz);
    tempz.x = z.x + tempz.x;
    tempz.y = z.y + tempz.y;  // tempz = z + tempz
    FPUcplxlog(&tempz, rz);
}   // end. Arccoshz

// rz=Arctanh(z)=1/2*Log{(1+z)/(1-z)}
void Arctanhz(DComplex z, DComplex *rz)
{
    DComplex temp0, temp1, temp2;

    if (z.x == 0.0)
    {
        rz->x = 0;
        rz->y = std::atan(z.y);
        return;
    }
    else
    {
        if (std::fabs(z.x) == 1.0 && z.y == 0.0)
        {
            return;
        }
        else if (std::fabs(z.x) < 1.0 && z.y == 0.0)
        {
            rz->x = std::log((1+z.x)/(1-z.x))/2;
            rz->y = 0;
            return;
        }
        else
        {
            temp0.x = 1 + z.x;
            temp0.y = z.y;             // temp0 = 1 + z
            temp1.x = 1 - z.x;
            temp1.y = -z.y;            // temp1 = 1 - z
            FPUcplxdiv(&temp0, &temp1, &temp2);
            FPUcplxlog(&temp2, &temp2);
            rz->x = .5*temp2.x;
            rz->y = .5*temp2.y;       // rz = .5*temp2
            return;
        }
    }
}   // end. Arctanhz

// rz=Arctan(z)=i/2*Log{(1-i*z)/(1+i*z)}
void Arctanz(DComplex z, DComplex *rz)
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
        Arctanhz(temp0, &temp0);
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

        FPUcplxdiv(&temp1, &temp2, &temp3);
        FPUcplxlog(&temp3, &temp3);
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

LComplex ComplexSqrtLong(long x, long y)
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
    SinCos086(thetalong, &result.y, &result.x);
    result.x  = multiply(result.x << (g_bit_shift - 16), maglong, g_bit_shift);
    result.y  = multiply(result.y << (g_bit_shift - 16), maglong, g_bit_shift);
    return result;
}

DComplex ComplexSqrtFloat(double x, double y)
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
        FPUsincos(&theta, &result.y, &result.x);
        result.x *= mag;
        result.y *= mag;
    }
    return result;
}


#ifndef TESTING_MATH

std::vector<BYTE> g_log_map_table;
long g_log_map_table_max_size;
bool g_log_map_calculate = false;
static double mlf;
static unsigned long lf;

/* int LogFlag;
   LogFlag == 1  -- standard log palettes
   LogFlag == -1 -- 'old' log palettes
   LogFlag >  1  -- compress counts < LogFlag into color #1
   LogFlag < -1  -- use quadratic palettes based on square roots && compress
*/

void SetupLogTable()
{
    float l, f, c, m;
    unsigned long limit;

    // set up on-the-fly variables
    if (g_log_map_flag > 0)
    {
        // new log function
        lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        mlf = (g_colors - (lf?2:1)) / std::log(static_cast<double>(g_log_map_table_max_size - lf));
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        mlf = (g_colors - 1) / std::log(static_cast<double>(g_log_map_table_max_size));
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        lf = 0 - g_log_map_flag;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        mlf = (g_colors - 2) / std::sqrt(static_cast<double>(g_log_map_table_max_size - lf));
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
            g_log_map_table[prev] = (BYTE)logtablecalc((long)prev);
        }
        g_log_map_calculate = false;   // turn it off, again
        return;
    }

    if (g_log_map_flag > -2)
    {
        lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        Fg2Float((long)(g_log_map_table_max_size-lf), 0, m);
        fLog14(m, m);
        Fg2Float((long)(g_colors-(lf?2:1)), 0, c);
        fDiv(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = (lf ? 2U : 1U); n < (unsigned int)g_colors; n++)
        {
            Fg2Float((long)n, 0, f);
            fMul16(f, m, f);
            fExp14(f, l);
            limit = (unsigned long)Float2Fg(l, 0) + lf;
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (BYTE)n;
            }
        }
    }
    else
    {
        lf = 0 - g_log_map_flag;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        Fg2Float((long)(g_log_map_table_max_size-lf), 0, m);
        fSqrt14(m, m);
        Fg2Float((long)(g_colors-2), 0, c);
        fDiv(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = 2U; n < (unsigned int)g_colors; n++)
        {
            Fg2Float((long)n, 0, f);
            fMul16(f, m, f);
            fMul16(f, f, l);
            limit = (unsigned long)(Float2Fg(l, 0) + lf);
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (BYTE)n;
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
                g_log_map_table[sptop] = (BYTE)(g_log_map_table[sptop-1]+1);
            }
        }
    }
}

long logtablecalc(long citer)
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
        if ((unsigned long)citer <= lf + 1)
        {
            ret = 1;
        }
        else if ((citer - lf)/std::log(static_cast<double>(citer - lf)) <= mlf)
        {
            ret = (long)(citer - lf);
        }
        else
        {
            ret = (long)(mlf * std::log(static_cast<double>(citer - lf))) + 1;
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
            ret = (long)(mlf * std::log(static_cast<double>(citer))) + 1;
        }
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        if ((unsigned long)citer <= lf)
        {
            ret = 1;
        }
        else if ((unsigned long)(citer - lf) <= (unsigned long)(mlf * mlf))
        {
            ret = (long)(citer - lf + 1);
        }
        else
        {
            ret = (long)(mlf * std::sqrt(static_cast<double>(citer - lf))) + 1;
        }
    }
    return ret;
}

long ExpFloat14(long xx)
{
    static float fLogTwo = 0.6931472F;
    int f;
    long Ans;

    f = 23 - (int)RegFloat2Fg(RegDivFloat(xx, *(long*)&fLogTwo), 0);
    Ans = ExpFudged(RegFloat2Fg(xx, 16), f);
    return RegFg2Float(Ans, (char)f);
}
#endif

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
MP *d2MP(double x)
{
    // TODO: implement
    assert(!"d2MP386 called.");
    return &Ans;
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
double *MP2d386(MP x)
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
MP *MPadd(MP x, MP y)
{
    // TODO: implement
    assert(!"MPadd386 called.");
    return &Ans;
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
int MPcmp386(MP x, MP y)
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
MP *MPdiv386(MP x, MP y)
{
    // TODO: implement
    assert(!"MPdiv386 called.");
    return &Ans;
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
MP *MPmul386(MP x, MP y)
{
    // TODO: implement
    assert(!"MPmul386 called.");
    return &Ans;
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
MP *fg2MP(long x, int fg)
{
    assert(!"fg2MP386 called");
    return &Ans;
}
