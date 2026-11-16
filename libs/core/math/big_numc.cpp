// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// C routines equivalent to ASM routines in bignuma.asm

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include <math/big.h>
#include <math/big_init.h>

#include <array>
#include <cfloat>
#include <cstring>

using namespace id::misc;

namespace id::math
{

/********************************************************************
 The following code contains the C versions of the routines from the
 file BIGNUMA.ASM.  It is provided here for portibility and for clarity.
*********************************************************************/

/********************************************************************
 Note:  The C code must be able to detect over/underflow.  Therefore,
 32-bit  integers must be used when doing 16 bit math.  All we really
 need is one more bit, such as is provided in asm with the carry bit.
 Functions that don't need the test for over/underflow, such as cmp_bn()
 and is_bn_not_zero(), can use 32-bit integers as long as g_bn_step
 is set to 4.

 The 16/32 bit combination of integer sizes could be increased to
 32/64 bit to improve efficiency, but since many compilers don't offer
 64-bit integers, this option was not included.

*********************************************************************/

/*****************************************/
// the following used to be in bigfltc.c

/********************************************************************/
// r = 0
BigFloatPtr clear_bf(BigFloatPtr r)
{
    std::memset(r, 0, g_bf_length+2);  // set array to zero
    return r;
}

/********************************************************************/
// r = n
BigFloatPtr copy_bf(BigFloatPtr r, BigFloatConstPtr n)
{
    std::memcpy(r, n, g_bf_length+2);
    return r;
}

/*********************************************************************/
//  f = b
//  Converts a bigfloat to a double
LDouble bf_to_float(BigFloatPtr n)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    int il = g_int_length;
    g_int_length = 2;
    LDouble f = bn_to_float(n);
    g_bn_length = bnl;
    g_int_length = il;

    const int power = static_cast<S16>(BIG_ACCESS16(n + g_bf_length));
    f = scale256(f, power);

    return f;
}

/********************************************************************/
// calculates and returns the value of f*256^n
// sort of like ldexp()
//
// n must be in the range -2^12 <= n < 2^12 (2^12=4096),
// which should not be a problem
LDouble scale256(const LDouble f, const int n)
{
    return scale_value(f, 256, n);
}

} // namespace id::math
