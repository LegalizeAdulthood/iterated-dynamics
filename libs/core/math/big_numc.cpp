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

/********************************************************************/
// r = 0
BigNumPtr clear_bn(BigNumPtr r)
{
    std::memset(r, 0, g_bn_length);  // set array to zero
    return r;
}

/********************************************************************/
// r = max positive value
BigNumPtr max_bn(BigNumPtr r)
{
    std::memset(r, 0xFF, g_bn_length-1);  // set to max values
    r[g_bn_length-1] = 0x7F;  // turn off the sign bit
    return r;
}

/********************************************************************/
// r < 0 ?
// returns 1 if negative, 0 if positive or zero
bool is_bn_neg(BigNumConstPtr n)
{
    return static_cast<S8>(n[g_bn_length - 1]) < 0;
}

/********************************************************************/
// n != 0 ?
// RETURNS: true if n != 0
bool is_bn_not_zero(BigNumPtr n)
{
    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        if (BIG_ACCESS16(n+i) != 0)
        {
            return true;
        }
    }
    return false;
}

/********************************************************************/
// r = 2*n
BigNumPtr double_bn(BigNumPtr r, BigNumPtr n)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(n + i)) << 1; // double it
        BIG_SET16(r+i, static_cast<U16>(prod));   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r *= 2
BigNumPtr double_a_bn(BigNumPtr r)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(r + i)) << 1; // double it
        BIG_SET16(r+i, static_cast<U16>(prod));   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n/2
BigNumPtr half_bn(BigNumPtr r, BigNumPtr n)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += static_cast<U32>((static_cast<S32>(static_cast<S16>(BIG_ACCESS16(n + i))) << 16) >>
            1);                                                       // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += static_cast<U32>(BIG_ACCESS16(n + i)) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                   // store the upper 2 bytes
        quot <<= 16;                                            // shift the underflow for next time
    }

    return r;
}

/********************************************************************/
// r /= 2
BigNumPtr half_a_bn(BigNumPtr r)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += static_cast<U32>(
            (static_cast<S32>(static_cast<S16>(BIG_ACCESS16(r + i))) << 16) >> 1); // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += static_cast<U32>(BIG_ACCESS16(r + i)) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }
    return r;
}

/********************************************************************/
// r *= u  where u is an unsigned integer
BigNumPtr mult_a_bn_int(BigNumPtr r, const U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(r + i)) * u; // r*u
        BIG_SET16(r+i, static_cast<U16>(prod));   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n / u  where u is an unsigned integer
BigNumPtr unsafe_div_bn_int(BigNumPtr r, BigNumPtr n, const U16 u)
{
    U16 rem = 0;

    const bool sign = is_bn_neg(n);
    if (sign)
    {
        neg_a_bn(n);
    }

    if (u == 0) // division by zero
    {
        max_bn(r);
        if (sign)
        {
            neg_a_bn(r);
        }
        return r;
    }

    // two bytes at a time
    for (int i = g_bn_length-2; i >= 0; i -= 2)
    {
        const U32 full_number = (static_cast<U32>(rem) << 16) + static_cast<U32>(BIG_ACCESS16(n + i));
        const U16 quot = static_cast<U16>(full_number / u);
        rem  = static_cast<U16>(full_number % u);
        BIG_SET16(r+i, quot);
    }

    if (sign)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r /= u  where u is an unsigned integer
BigNumPtr div_a_bn_int(BigNumPtr r, const U16 u)
{
    U16 rem = 0;

    const bool sign = is_bn_neg(r);
    if (sign)
    {
        neg_a_bn(r);
    }

    if (u == 0) // division by zero
    {
        max_bn(r);
        if (sign)
        {
            neg_a_bn(r);
        }
        return r;
    }

    // two bytes at a time
    for (int i = g_bn_length-2; i >= 0; i -= 2)
    {
        const U32 full_number = (static_cast<U32>(rem) << 16) + static_cast<U32>(BIG_ACCESS16(r + i));
        const U16 quot = static_cast<U16>(full_number / u);
        rem  = static_cast<U16>(full_number % u);
        BIG_SET16(r+i, quot);
    }

    if (sign)
    {
        neg_a_bn(r);
    }
    return r;
}

/*********************************************************************/
//  f = b
//  Converts a bignumber to a double
LDouble bn_to_float(BigNumPtr n)
{
    LDouble f = 0;

    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        sign_flag = true;
        neg_a_bn(n);
    }

    int exponent = g_int_length - 1;
    BigNumPtr get_byte = n + g_bn_length - 1;
    while (*get_byte == 0 && get_byte >= n)
    {
        get_byte--;
        exponent--;
    }

    // There is no need to use all g_bn_length bytes.  To get the full
    // precision of LDouble, all you need is LDBL_MANT_DIG/8+1.
    for (int i = 0; i < LDBL_MANT_DIG / 8 + 1 && get_byte >= n; i++, get_byte--)
    {
        f += scale256(*get_byte, -i);
    }

    f = scale256(f, exponent);

    if (sign_flag)
    {
        f = -f;
        neg_a_bn(n);
    }
    return f;
}

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
//  b = f
//  Converts a double to a bigfloat
BigFloatPtr float_to_bf(BigFloatPtr r, LDouble f)
{
    int power;
    if (f == 0)
    {
        clear_bf(r);
        return r;
    }

    // remove the exp part
    f = extract256(f, &power);

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    int il = g_int_length;
    g_int_length = 2;
    float_to_bn(r, f);
    g_bn_length = bnl;
    g_int_length = il;

    BIG_SET16(r + g_bf_length, static_cast<S16>(power)); // exp

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
// extracts the mantissa and exponent of f
// finds m and n such that 1<=|m|<256 and f = m*256^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract256(const LDouble f, int *exp_ptr)
{
    return extract_value(f, 256, exp_ptr);
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
