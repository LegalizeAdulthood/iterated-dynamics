// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/big.h>

#include <cstring>

using namespace id::misc;

namespace id::math
{

// r = n
Byte *copy_bn(Byte *r, const Byte *n)
{
    std::memcpy(r, n, g_bn_length);
    return r;
}

// r = n1 + n2
Byte *add_bn(Byte *r, const Byte *n1, const Byte *n2)
{
    U32 sum = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        sum += static_cast<U32>(BIG_ACCESS16(n1 + i)) + static_cast<U32>(BIG_ACCESS16(n2 + i)); // add 'em up
        BIG_SET16(r + i, static_cast<U16>(sum)); // store the lower 2 bytes
        sum >>= 16;                              // shift the overflow for next time
    }
    return r;
}

// r = n1 - n2
Byte *sub_bn(Byte *r, const Byte *n1, const Byte *n2)
{
    U32 diff = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        diff = static_cast<U32>(BIG_ACCESS16(n1 + i)) -
            (static_cast<U32>(BIG_ACCESS16(n2 + i)) - static_cast<S32>(static_cast<S16>(diff))); // subtract with borrow
        BIG_SET16(r + i, static_cast<U16>(diff)); // store the lower 2 bytes
        diff >>= 16;                              // shift the underflow for next time
    }
    return r;
}

// r += n
Byte *add_a_bn(Byte *r, const Byte *n)
{
    return add_bn(r, r, n);
}

// r -= n
Byte *sub_a_bn(Byte *r, const Byte *n)
{
    return sub_bn(r, r, n);
}

// r = n * u
Byte *mult_bn_int(Byte *r, const Byte *n, const U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(n + i)) * u; // n*u
        BIG_SET16(r + i, static_cast<U16>(prod));          // store the lower 2 bytes
        prod >>= 16;                                       // shift the overflow for next time
    }
    return r;
}

// r = |n|
Byte *abs_bn(Byte *r, const Byte *n)
{
    copy_bn(r, n);
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

// r = |r|
Byte *abs_a_bn(Byte *r)
{
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

/***************************************************************************/
// n1 != n2 ?
// RETURNS:
//  if n1 == n2 returns 0
//  if n1 > n2 returns a positive (bytes left to go when mismatch occured)
//  if n1 < n2 returns a negative (bytes left to go when mismatch occured)
int cmp_bn(const Byte *n1, const Byte *n2)
{
    // two bytes at a time
    // signed comparison for msb
    const S16 value1 = BIG_ACCESS_S16(reinterpret_cast<const S16 *>(n1 + g_bn_length - 2));
    const S16 value2 = BIG_ACCESS_S16(reinterpret_cast<const S16 *>(n2 + g_bn_length - 2));
    if (value1 > value2)
    {
        // now determine which of the two bytes was different
        if (static_cast<S16>(value1 & 0xFF00) > static_cast<S16>(value2 & 0xFF00)) // compare just high bytes
        {
            return g_bn_length;                                                    // high byte was different
        }

        return g_bn_length - 1;                                                    // low byte was different
    }
    if (value1 < value2)
    {
        // now determine which of the two bytes was different
        if (static_cast<S16>(value1 & 0xFF00) < static_cast<S16>(value2 & 0xFF00)) // compare just high bytes
        {
            return -g_bn_length;                                                   // high byte was different
        }

        return -(g_bn_length - 1);                                                 // low byte was different
    }

    // unsigned comparison for the rest
    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        const U16 n1_value = BIG_ACCESS16(n1 + i);
        const U16 n2_value = BIG_ACCESS16(n2 + i);
        if (n1_value > n2_value)
        {
            // now determine which of the two bytes was different
            if ((n1_value & 0xFF00) > (n2_value & 0xFF00)) // compare just high bytes
            {
                return i + 2;                              // high byte was different
            }

            return i + 1;                                  // low byte was different
        }
        if (n1_value < n2_value)
        {
            // now determine which of the two bytes was different
            if ((n1_value & 0xFF00) < (n2_value & 0xFF00)) // compare just high bytes
            {
                return -(i + 2);                           // high byte was different
            }

            return -(i + 1);                               // low byte was different
        }
    }
    return 0;
}

/********************************************************************/
// r = -n
Byte *neg_bn(Byte *r, const Byte *n)
{
    int i;
    U32 neg = 1; // to get the 2's complement started

    // two bytes at a time
    for (i = 0; neg != 0 && i < g_bn_length; i += 2)
    {
        const U16 t_short = ~BIG_ACCESS16(n + i);
        neg += static_cast<U32>(t_short);        // two's complement
        BIG_SET16(r + i, static_cast<U16>(neg)); // store the lower 2 bytes
        neg >>= 16;                              // shift the sign bit for next time
    }
    // if neg was 0, then just "not" the rest
    for (; i < g_bn_length; i += 2)
    {
        // notice that BIG_ACCESS16() and BIG_SET16() are not needed here
        *reinterpret_cast<U16 *>(r + i) = ~*reinterpret_cast<const U16 *>(n + i); // toggle all the bits
    }
    return r;
}

Byte *neg_a_bn(Byte *r)
{
    return neg_bn(r, r);
}

/********************************************************************/
// r = 0
Byte *clear_bn(Byte *r)
{
    std::memset(r, 0, g_bn_length); // set array to zero
    return r;
}

/********************************************************************/
// r = max positive value
Byte *max_bn(Byte *r)
{
    std::memset(r, 0xFF, g_bn_length - 1); // set to max values
    r[g_bn_length - 1] = 0x7F;             // turn off the sign bit
    return r;
}

/********************************************************************/
// r < 0 ?
// returns 1 if negative, 0 if positive or zero
bool is_bn_neg(const Byte *n)
{
    return static_cast<S8>(n[g_bn_length - 1]) < 0;
}

/********************************************************************/
// n != 0 ?
// RETURNS: true if n != 0
bool is_bn_not_zero(const Byte *n)
{
    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        if (BIG_ACCESS16(n + i) != 0)
        {
            return true;
        }
    }
    return false;
}

/********************************************************************/
// r = 2*n
Byte *double_bn(Byte *r, const Byte *n)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(n + i)) << 1; // double it
        BIG_SET16(r + i, static_cast<U16>(prod));           // store the lower 2 bytes
        prod >>= 16;                                        // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r *= 2
Byte *double_a_bn(Byte *r)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(r + i)) << 1; // double it
        BIG_SET16(r + i, static_cast<U16>(prod));           // store the lower 2 bytes
        prod >>= 16;                                        // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n/2
Byte *half_bn(Byte *r, const Byte *n)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += static_cast<U32>(
            (static_cast<S32>(static_cast<S16>(BIG_ACCESS16(n + i))) << 16) >> 1); // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                            // store the upper 2 bytes
        quot <<= 16;                                                               // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += static_cast<U32>(BIG_ACCESS16(n + i)) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));           // store the upper 2 bytes
        quot <<= 16;                                              // shift the underflow for next time
    }

    return r;
}

/********************************************************************/
// r /= 2
Byte *half_a_bn(Byte *r)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += static_cast<U32>(
            (static_cast<S32>(static_cast<S16>(BIG_ACCESS16(r + i))) << 16) >> 1); // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));                            // store the upper 2 bytes
        quot <<= 16;                                                               // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += static_cast<U32>(BIG_ACCESS16(r + i)) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, static_cast<U16>(quot >> 16));           // store the upper 2 bytes
        quot <<= 16;                                              // shift the underflow for next time
    }
    return r;
}

/********************************************************************/
// r *= u  where u is an unsigned integer
Byte *mult_a_bn_int(Byte *r, const U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(r + i)) * u; // r*u
        BIG_SET16(r + i, static_cast<U16>(prod));          // store the lower 2 bytes
        prod >>= 16;                                       // shift the overflow for next time
    }
    return r;
}

bool is_bn_zero(const Byte *n)
{
    return !is_bn_not_zero(n);
}

} // namespace id::math
