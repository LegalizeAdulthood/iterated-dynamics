// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// Fixed-point math, e.g. 'long' arithmetic, related data and functions

extern int                   g_bit_shift;
extern long                  g_fudge_factor;
extern bool                  g_overflow;

// TODO: Implement these routines using only integer operations

//       32-bit integer divide routine with an 'n'-bit shift.
//       Overflow condition returns 0x7fffh with overflow = 1;
//       Note: integer division is faked with floating-point division.
//
//       z = divide(x, y, n);       z = x / y;
//
inline long divide(long x, long y, int n)
{
    return (long) (((float) x) / ((float) y)*(float)(1 << n));
}

//  32 bit integer multiply with n bit shift.
//  Note that we fake integer multiplication with floating point
//  multiplication.
//  Overflow condition returns 0x7fffffffh with overflow = 1;
inline long multiply(long x, long y, int n)
{
    long l = (long)(((float) x) * ((float) y)/(float)(1 << n));
    if (l == 0x7fffffff)
    {
        g_overflow = true;
    }
    return l;
}

inline long lsqr(long x)
{
    return multiply(x, x, g_bit_shift);
}
