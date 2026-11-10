// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigNum.h>

#include "BigStorage.h"

#include <math/big_init.h>
#include <math/BigFloat.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

using namespace id::misc;

namespace id::math
{
namespace
{

void validate_buffer(BigNumConstPtr buffer, const std::size_t size, const std::size_t required)
{
    if (buffer == nullptr || size < required)
    {
        throw std::invalid_argument("BigNum requires a complete legacy value buffer");
    }
}

/***************************************************************************/
// n1 != n2 ?
// RETURNS:
//  if n1 == n2 returns 0
//  if n1 > n2 returns a positive (bytes left to go when mismatch occured)
//  if n1 < n2 returns a negative (bytes left to go when mismatch occured)
int compare(BigNumConstPtr n1, BigNumConstPtr n2)
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
// r = n1 + n2
BigNumPtr add(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
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

/********************************************************************/
// r = n1 - n2
BigNumPtr subtract(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
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

/********************************************************************/
// r = -n
BigNumPtr negate(BigNumPtr r, BigNumConstPtr n)
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

/************************************************************************/
// r = n1 * n2
// Note: r will be a double wide result, 2*g_bn_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigNumPtr full_multiply(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    BigNumPtr rp2;
    // pointers for r

    const bool sign1 = is_bn_neg(n1);
    if (sign1) // =, not ==
    {
        neg_a_bn(n1);
    }
    const bool same_var = n1 == n2;
    if (!same_var) // check to see if they're the same pointer
    {
        sign2 = is_bn_neg(n2);
        if (sign2) // =, not ==
        {
            neg_a_bn(n2);
        }
    }

    BigNumPtr n1_p = n1;
    const int steps = g_bn_length >> 1; // two bytes at a time
    int carry_steps = double_steps = (steps << 1) - 2;
    g_bn_length <<= 1;
    clear_bn(r);                        // double width
    g_bn_length >>= 1;
    BigNumPtr rp1 = rp2 = r;
    for (int i = 0; i < steps; i++)
    {
        BigNumPtr n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            const U32 prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
            U32 sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
            BIG_SET16(rp2, static_cast<U16>(sum));                // save the lower 2 bytes
            sum >>= 16;                                           // keep just the upper 2 bytes
            BigNumPtr rp3 = rp2 + 2;                              // move over 2 bytes
            sum += BIG_ACCESS16(rp3);                             // add what was the upper two bytes
            BIG_SET16(rp3, static_cast<U16>(sum));                // save what was the upper two bytes
            sum >>= 16;                                           // keep just the overflow
            for (int k = 0; sum != 0 && k < carry_steps; k++)
            {
                rp3 += 2;                                         // move over 2 bytes
                sum += BIG_ACCESS16(rp3);                         // add to what was the overflow
                BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the overflow
                sum >>= 16;                                       // keep just the new overflow
            }
            n2_p += 2;                                            // to next word
            rp2 += 2;
            carry_steps--;                                        // use one less step
        }
        n1_p += 2;                                                // to next word
        rp2 = rp1 += 2;
        carry_steps = --double_steps;                             // decrease doubles steps and reset carry_steps
    }

    // if they were the same or same sign, the product must be positive
    if (!same_var && sign1 != sign2)
    {
        g_bn_length <<= 1; // for a double wide number
        neg_a_bn(r);
        g_bn_length >>= 1; // restore g_bn_length
    }
    return r;
}

/************************************************************************/
// r = n1 * n2 calculating only the top g_r_length bytes
// Note: r will be of length g_r_length
//       2*g_bn_length <= g_r_length < g_bn_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigNumPtr multiply(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    BigNumPtr rp1;

    int bnl = g_bn_length;
    const bool sign1 = is_bn_neg(n1);
    if (sign1 != 0) // =, not ==
    {
        neg_a_bn(n1);
    }
    const bool same_var = n1 == n2;
    if (!same_var) // check to see if they're the same pointer
    {
        sign2 = is_bn_neg(n2);
        if (sign2) // =, not ==
        {
            neg_a_bn(n2);
        }
    }
    BigNumPtr n1_p = n1;
    n2 += (g_bn_length << 1) - g_r_length; // shift n2 over to where it is needed

    g_bn_length = g_r_length;
    clear_bn(r);                           // zero out r, g_r_length width
    g_bn_length = bnl;

    int steps = (g_r_length - g_bn_length) >> 1;
    int skips = (g_bn_length >> 1) - steps;
    int carry_steps = double_steps = (g_r_length >> 1) - 2;
    BigNumPtr rp2 = rp1 = r;
    for (int i = g_bn_length >> 1; i > 0; i--)
    {
        BigNumPtr n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            const U32 prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
            U32 sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
            BIG_SET16(rp2, static_cast<U16>(sum));                // save the lower 2 bytes
            sum >>= 16;                                           // keep just the upper 2 bytes
            BigNumPtr rp3 = rp2 + 2;                              // move over 2 bytes
            sum += BIG_ACCESS16(rp3);                             // add what was the upper two bytes
            BIG_SET16(rp3, static_cast<U16>(sum));                // save what was the upper two bytes
            sum >>= 16;                                           // keep just the overflow
            for (int k = 0; sum != 0 && k < carry_steps; k++)
            {
                rp3 += 2;                                         // move over 2 bytes
                sum += BIG_ACCESS16(rp3);                         // add to what was the overflow
                BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the overflow
                sum >>= 16;                                       // keep just the new overflow
            }
            n2_p += 2;                                            // increase by two bytes
            rp2 += 2;
            carry_steps--;
        }
        n1_p += 2; // increase by two bytes

        if (skips != 0)
        {
            n2 -= 2; // shift n2 back a word
            steps++; // one more step this time
            skips--; // keep track of how many times we've done this
        }
        else
        {
            rp1 += 2;       // shift forward a word
            double_steps--; // reduce the carry steps needed next time
        }
        rp2 = rp1;
        carry_steps = double_steps;
    }

    // if they were the same or same sign, the product must be positive
    if (!same_var && sign1 != sign2)
    {
        g_bn_length = g_r_length;
        neg_a_bn(r); // wider bignumber
        g_bn_length = bnl;
    }
    return r;
}

/************************************************************************/
// r = n^2
//   because of the symetry involved, n^2 is much faster than n*n
//   for a bignumber of length l
//      n*n takes l^2 multiplications
//      n^2 takes (l^2+l)/2 multiplications
//          which is about 1/2 n*n as l gets large
//  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
//
// SIDE-EFFECTS: n is changed to its absolute value
BigNumPtr full_square(BigNumPtr r, BigNumPtr n)
{
    int double_steps;
    BigNumPtr rp1;
    BigNumPtr rp3;
    U32 prod;
    U32 sum;

    if (is_bn_neg(n)) // don't need to keep track of sign since the
    {
        neg_a_bn(n);  // answer must be positive.
    }

    g_bn_length <<= 1;
    clear_bn(r); // zero out r, double width
    g_bn_length >>= 1;

    int steps = (g_bn_length >> 1) - 1;
    int carry_steps = double_steps = (steps << 1) - 1;
    BigNumPtr rp2 = rp1 = r + 2;        // start with second two-byte word
    BigNumPtr n1_p = n;
    if (steps != 0)                     // if zero, then skip all the middle term calculations
    {
        for (int i = steps; i > 0; i--) // steps gets altered, count backwards
        {
            BigNumPtr n2_p = n1_p + 2;  // set n2p pointer to 1 step beyond n1p
            for (int j = 0; j < steps; j++)
            {
                prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
                sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
                BIG_SET16(rp2, static_cast<U16>(sum));            // save the lower 2 bytes
                sum >>= 16;                                       // keep just the upper 2 bytes
                rp3 = rp2 + 2;                                    // move over 2 bytes
                sum += BIG_ACCESS16(rp3);                         // add what was the upper two bytes
                BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the upper two bytes
                sum >>= 16;                                       // keep just the overflow
                for (int k = 0; sum != 0 && k < carry_steps; k++)
                {
                    rp3 += 2;                                     // move over 2 bytes
                    sum += BIG_ACCESS16(rp3);                     // add to what was the overflow
                    BIG_SET16(rp3, static_cast<U16>(sum));        // save what was the overflow
                    sum >>= 16;                                   // keep just the new overflow
                }
                n2_p += 2;                                        // increase by two bytes
                rp2 += 2;
                carry_steps--;
            }
            n1_p += 2;                       // increase by two bytes
            rp2 = rp1 += 4;                  // increase by 2 * two bytes
            carry_steps = double_steps -= 2; // reduce the carry steps needed
            steps--;
        }
        // All the middle terms have been multiplied.  Now double it.
        g_bn_length <<= 1; // double wide bignumber
        double_a_bn(r);
        g_bn_length >>= 1;
        // finished with middle terms
    }

    // Now go back and add in the squared terms.
    n1_p = n;
    steps = g_bn_length >> 1;
    carry_steps = double_steps = (steps << 1) - 2;
    rp1 = r;
    for (int i = 0; i < steps; i++)
    {
        // square it
        prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n1_p)); // U16*U16=U32
        sum = static_cast<U32>(BIG_ACCESS16(rp1)) + prod; // add to previous, including overflow
        BIG_SET16(rp1, static_cast<U16>(sum));            // save the lower 2 bytes
        sum >>= 16;                                       // keep just the upper 2 bytes
        rp3 = rp1 + 2;                                    // move over 2 bytes
        sum += BIG_ACCESS16(rp3);                         // add what was the upper two bytes
        BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the upper two bytes
        sum >>= 16;                                       // keep just the overflow
        for (int k = 0; sum != 0 && k < carry_steps; k++)
        {
            rp3 += 2;                                     // move over 2 bytes
            sum += BIG_ACCESS16(rp3);                     // add to what was the overflow
            BIG_SET16(rp3, static_cast<U16>(sum));        // save what was the overflow
            sum >>= 16;                                   // keep just the new overflow
        }
        n1_p += 2;                                        // increase by 2 bytes
        rp1 += 4;                                         // increase by 4 bytes
        carry_steps = double_steps -= 2;
    }
    return r;
}

/************************************************************************/
// r = n^2
//   because of the symetry involved, n^2 is much faster than n*n
//   for a bignumber of length l
//      n*n takes l^2 multiplications
//      n^2 takes (l^2+l)/2 multiplications
//          which is about 1/2 n*n as l gets large
//  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
//
// Note: r will be of length g_r_length
//       2*g_bn_length >= g_r_length > g_bn_length
// SIDE-EFFECTS: n is changed to its absolute value
BigNumPtr square(BigNumPtr r, BigNumPtr n)
{
    int double_steps;
    BigNumPtr n2_p;
    BigNumPtr rp1;
    BigNumPtr rp3;
    U32 prod;
    U32 sum;

    // This whole procedure would be a great deal simpler if we could assume that
    // g_r_length < 2*g_bn_length (that is, not =).  Therefore, we will take the
    // easy way out and call full_square_bn() if it is.
    if (g_r_length == g_bn_length << 1) // g_r_length == 2*g_bn_length
    {
        return full_square(r, n);       // call full_square_bn() and quit
    }

    if (is_bn_neg(n))                   // don't need to keep track of sign since the
    {
        neg_a_bn(n);                    // answer must be positive.
    }

    int bnl = g_bn_length;
    g_bn_length = g_r_length;
    clear_bn(r); // zero out r, of width g_r_length
    g_bn_length = bnl;

    // determine whether r is on an odd or even two-byte word in the number
    const int rodd = static_cast<U16>(((g_bn_length << 1) - g_r_length) >> 1) & 0x0001;
    int i = (g_bn_length >> 1) - 1;
    int steps = (g_r_length - g_bn_length) >> 1;
    int carry_steps = double_steps = (g_bn_length >> 1) + steps - 2;
    int skips = (i - steps) >> 1;                                       // how long to skip over pointer shifts
    BigNumPtr rp2 = rp1 = r;
    BigNumPtr n1_p = n;
    BigNumPtr n3_p = n2_p = n1_p + (((g_bn_length >> 1) - steps) << 1); // n2p = n1p + 2*(g_bn_length/2 - steps)
    if (i != 0)                                                         // if zero, skip middle term calculations
    {
        // i is already set
        for (; i > 0; i--)
        {
            for (int j = 0; j < steps; j++)
            {
                prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
                sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
                BIG_SET16(rp2, static_cast<U16>(sum));            // save the lower 2 bytes
                sum >>= 16;                                       // keep just the upper 2 bytes
                rp3 = rp2 + 2;                                    // move over 2 bytes
                sum += BIG_ACCESS16(rp3);                         // add what was the upper two bytes
                BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the upper two bytes
                sum >>= 16;                                       // keep just the overflow
                for (int k = 0; sum != 0 && k < carry_steps; k++)
                {
                    rp3 += 2;                                     // move over 2 bytes
                    sum += BIG_ACCESS16(rp3);                     // add to what was the overflow
                    BIG_SET16(rp3, static_cast<U16>(sum));        // save what was the overflow
                    sum >>= 16;                                   // keep just the new overflow
                }
                n2_p += 2;                                        // increase by 2-byte word size
                rp2 += 2;
                carry_steps--;
            }
            n1_p += 2; // increase by 2-byte word size
            if (skips > 0)
            {
                n2_p = n3_p -= 2;
                steps++;
                skips--;
            }
            else if (skips == 0) // only gets executed once
            {
                steps -= rodd;   // rodd is 1 or 0
                double_steps -= rodd + 1;
                rp1 += (rodd + 1) << 1;
                n2_p = n1_p + 2;
                skips--;
            }
            else // skips < 0
            {
                steps--;
                double_steps -= 2;
                rp1 += 4; // add two 2-byte words
                n2_p = n1_p + 2;
            }
            rp2 = rp1;
            carry_steps = double_steps;
        }
        // All the middle terms have been multiplied.  Now double it.
        g_bn_length = g_r_length;
        double_a_bn(r);
        g_bn_length = bnl;
    }
    // Now go back and add in the squared terms.

    // be careful, the next dozen or so lines are confusing!
    // determine whether r is on an odd or even word in the number
    // using i as a temporary variable here
    i = (g_bn_length << 1) - g_r_length;
    rp1 = r + (static_cast<U16>(i) & static_cast<U16>(0x0002));
    i = static_cast<U16>((i >> 1) + 1) & static_cast<U16>(0xFFFE);
    n1_p = n + i;
    // i here is no longer a temp var., but will be used as a loop counter
    i = (g_bn_length - i) >> 1;
    carry_steps = double_steps = (i << 1) - 2;
    // i is already set
    for (; i > 0; i--)
    {
        // square it
        prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n1_p)); // U16*U16=U32
        sum = static_cast<U32>(BIG_ACCESS16(rp1)) + prod; // add to previous, including overflow
        BIG_SET16(rp1, static_cast<U16>(sum));            // save the lower 2 bytes
        sum >>= 16;                                       // keep just the upper 2 bytes
        rp3 = rp1 + 2;                                    // move over 2 bytes
        sum += BIG_ACCESS16(rp3);                         // add what was the upper two bytes
        BIG_SET16(rp3, static_cast<U16>(sum));            // save what was the upper two bytes
        sum >>= 16;                                       // keep just the overflow
        for (int k = 0; sum != 0 && k < carry_steps; k++)
        {
            rp3 += 2;                                     // move over 2 bytes
            sum += BIG_ACCESS16(rp3);                     // add to what was the overflow
            BIG_SET16(rp3, static_cast<U16>(sum));        // save what was the overflow
            sum >>= 16;                                   // keep just the new overflow
        }
        n1_p += 2;
        rp1 += 4;
        carry_steps = double_steps -= 2;
    }
    return r;
}

/********************************************************************/
// r = n1/n2
//      r - result of length g_bn_length
// uses g_bn_tmp1 - g_bn_tmp3 - global temp bignumbers
//  SIDE-EFFECTS:
//      n1, n2 can end up as GARBAGE
//      Make copies first if necessary.
BigNumPtr divide(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    // first, check for valid data
    const LDouble a = bn_to_float(n1);
    if (a == 0)      // division into zero
    {
        clear_bn(r); // return 0
        return r;
    }
    const LDouble b = bn_to_float(n2);
    if (b == 0) // division by zero
    {
        max_bn(r);
        return r;
    }
    const LDouble f = a / b; // approximate quotient
    const long max_val = (1L << ((g_int_length << 3) - 1)) - 1;
    if (f > max_val)         // check for overflow
    {
        max_bn(r);
        return r;
    }
    if (f <= -max_val)
    {
        max_bn(r);
        neg_a_bn(r);
        return r;
    }
    // appears to be ok, do division

    bool sign = false;
    if (is_bn_neg(n1))
    {
        neg_a_bn(n1);
        sign = !sign;
    }
    if (is_bn_neg(n2))
    {
        neg_a_bn(n2);
        sign = !sign;
    }

    // scale n1 and n2 so: |n| >= 1/256
    // scale = (int)(log(1/fabs(a))/LOG_256) = LOG_256(1/|a|)
    int i = g_bn_length - 1;
    while (i >= 0 && n1[i] == 0)
    {
        i--;
    }
    int scale1 = g_bn_length - i - 2;
    scale1 = std::max(scale1, 0);
    i = g_bn_length - 1;
    while (i >= 0 && n2[i] == 0)
    {
        i--;
    }
    int scale2 = g_bn_length - i - 2;
    scale2 = std::max(scale2, 0);

    // shift n1, n2
    // important!, use std::memmove(), not std::memcpy()
    std::memmove(n1 + scale1, n1, g_bn_length - scale1); // shift bytes over
    std::memset(n1, 0, scale1);                          // zero out the rest
    std::memmove(n2 + scale2, n2, g_bn_length - scale2); // shift bytes over
    std::memset(n2, 0, scale2);                          // zero out the rest

    unsafe_inv_bn(r, n2);
    multiply(g_bn_tmp1.data(), n1, r);
    copy_bn(r, g_bn_tmp1.data() + g_shift_factor); // r = g_bn_tmp1

    if (scale1 != scale2)
    {
        // Rescale r back to what it should be.  Overflow has already been checked
        if (scale1 > scale2)                                 // answer is too big, adjust it
        {
            const int scale = scale1 - scale2;
            std::memmove(r, r + scale, g_bn_length - scale); // shift bytes over
            std::memset(r + g_bn_length - scale, 0, scale);  // zero out the rest
        }
        else if (scale1 < scale2)                            // answer is too small, adjust it
        {
            const int scale = scale2 - scale1;
            std::memmove(r + scale, r, g_bn_length - scale); // shift bytes over
            std::memset(r, 0, scale);                        // zero out the rest
        }
        // else scale1 == scale2
    }

    if (sign)
    {
        neg_a_bn(r);
    }

    return r;
}

BigNumPtr binary_operation(BigNumPtr result, BigNumConstPtr left, BigNumConstPtr right,
    BigNumPtr (*operation)(BigNumPtr, BigNumPtr, BigNumPtr))
{
    copy_bn(g_bn_tmp_copy1.data(), left);
    copy_bn(g_bn_tmp_copy2.data(), right);
    return operation(result, g_bn_tmp_copy1.data(), g_bn_tmp_copy2.data());
}

void validate_scalar(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigNum requires a finite scalar value");
    }
}

BigArena::Allocation native_storage(BigArena &arena, const LDouble value)
{
    validate_scalar(value);
    auto storage = arena.allocate_single();
    std::memset(storage.data(), 0, arena.block_size());
    float_to_bn(storage.data(), value);
    return storage;
}

} // namespace

void detail::reduce_bn_product(Byte *value)
{
    std::memmove(value, value + g_shift_factor, g_bn_length);
}

BigNum::BigNum() noexcept = default;

BigNum::BigNum(const LDouble value) :
    BigNum(bn_arena(BigLifetime::WORKING), value)
{
}

BigNum::BigNum(const std::string_view text) :
    BigNum(bn_arena(BigLifetime::WORKING), text)
{
}

BigNum::BigNum(const BigFloat &value) :
    BigNum(bn_arena(BigLifetime::WORKING), value)
{
}

BigNum::BigNum(BigArena &arena) :
    BigNum(arena.allocate_single())
{
}

BigNum::BigNum(BigArena &arena, const LDouble value) :
    m_storage(native_storage(arena, value))
{
}

BigNum::BigNum(BigArena &arena, const std::string_view text) :
    BigNum(arena)
{
    if (!text.empty())
    {
        str_to_bn(m_storage.data(), text);
    }
}

BigNum::BigNum(BigArena &arena, const BigFloat &value) :
    BigNum(arena)
{
    value.copy_to_legacy_num(m_storage.data(), encoded_size());
}

BigNum::BigNum(const BigNum &value) :
    m_storage(value.m_storage.clone())
{
}

BigNum::BigNum(BigArena &arena, const BigNum &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigNum::BigNum(BigNum &&value) noexcept = default;

BigNum::BigNum(BigArena::Allocation storage) :
    m_storage(std::move(storage))
{
    std::memset(m_storage.data(), 0, m_storage.blocks() * arena().block_size());
}

BigNum &BigNum::operator=(const BigNum &value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else
        {
            if (!m_storage)
            {
                BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bn_arena(BigLifetime::WORKING);
                m_storage = destination.allocate_single();
            }
            std::memcpy(m_storage.data(), value.m_storage.data(), encoded_size());
        }
    }
    return *this;
}

BigNum &BigNum::operator=(BigNum &&value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else if ((m_storage.arena_ptr() != nullptr ? &arena() : &bn_arena(BigLifetime::WORKING)) == &value.arena())
        {
            m_storage = std::move(value.m_storage);
        }
        else
        {
            *this = static_cast<const BigNum &>(value);
        }
    }
    return *this;
}

BigNum &BigNum::operator=(const LDouble value)
{
    validate_scalar(value);
    if (!m_storage)
    {
        BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bn_arena(BigLifetime::WORKING);
        m_storage = destination.allocate_single();
        std::memset(m_storage.data(), 0, arena().block_size());
    }
    float_to_bn(m_storage.data(), value);
    return *this;
}

BigNum BigNum::operator-() const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    negate(result.m_storage.data(), m_storage.data());
    return result;
}

BigNum &BigNum::operator+=(const BigNum &right)
{
    add(m_storage.data(), m_storage.data(), right.m_storage.data());
    return *this;
}

BigNum &BigNum::operator-=(const BigNum &right)
{
    subtract(m_storage.data(), m_storage.data(), right.m_storage.data());
    return *this;
}

BigNum &BigNum::operator*=(const BigNum &right)
{
    *this = *this * right;
    return *this;
}

BigNum &BigNum::operator/=(const BigNum &right)
{
    *this = *this / right;
    return *this;
}

BigNum &BigNum::operator/=(const U16 right)
{
    div_a_bn_int(m_storage.data(), right);
    return *this;
}

BigNum BigNum::operator+(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    add(result.m_storage.data(), m_storage.data(), right.m_storage.data());
    return result;
}

BigNum BigNum::operator-(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    subtract(result.m_storage.data(), m_storage.data(), right.m_storage.data());
    return result;
}

BigNum BigNum::operator*(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING).allocate_double()};
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), multiply);
    result.m_storage.reduce();
    return result;
}

BigNum BigNum::operator*(const U16 right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    mult_bn_int(result.m_storage.data(), m_storage.data(), right);
    return result;
}

BigNum BigNum::operator/(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), divide);
    return result;
}

BigNum BigNum::sqr() const
{
    BigNum result{bn_arena(BigLifetime::WORKING).allocate_double()};
    copy_bn(g_bn_tmp_copy1.data(), m_storage.data());
    square(result.m_storage.data(), g_bn_tmp_copy1.data());
    result.m_storage.reduce();
    return result;
}

void square_bn(BigNum &result, BigNum &source)
{
    const bool sign = is_bn_neg(source.data());
    square(result.data(), source.data());
    if (sign)
    {
        neg_a_bn(source.data());
    }
}

void mult_bn(BigNum &result, BigNum &left, BigNum &right)
{
    const bool sign1 = is_bn_neg(left.data());
    const bool sign2 = is_bn_neg(right.data());
    multiply(result.data(), left.data(), right.data());
    if (sign1)
    {
        neg_a_bn(left.data());
    }
    if (sign2)
    {
        neg_a_bn(right.data());
    }
}

bool BigNum::operator==(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) == 0;
}

bool BigNum::operator!=(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) != 0;
}

bool BigNum::operator<(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) < 0;
}

bool BigNum::operator<=(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) <= 0;
}

bool BigNum::operator>(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) > 0;
}

bool BigNum::operator>=(const BigNum &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) >= 0;
}

LDouble BigNum::to_float() const
{
    copy_bn(g_bn_tmp_copy1.data(), m_storage.data());
    return bn_to_float(g_bn_tmp_copy1.data());
}

std::string BigNum::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigNum formatting requires a nonnegative digit count");
    }
    return bn_to_string(m_storage.data(), digits);
}

BigNum BigNum::from_legacy(BigNumConstPtr source, const std::size_t size)
{
    return from_legacy(bn_arena(BigLifetime::WORKING), source, size);
}

BigNum BigNum::from_legacy(BigArena &arena, BigNumConstPtr source, const std::size_t size)
{
    BigNum result{arena};
    validate_buffer(source, size, result.encoded_size());
    std::copy_n(source, result.encoded_size(), result.m_storage.data());
    return result;
}

void BigNum::copy_to_legacy(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, encoded_size());
    std::copy_n(m_storage.data(), encoded_size(), destination);
}

void BigNum::copy_to_legacy_float(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bf_length) + 2);
    bn_to_bf(destination, m_storage.data());
}

BigNum sqr(const BigNum &value)
{
    return value.sqr();
}

// Raw compatibility adapters for callers not yet using owning values.
int cmp_bn(BigNumConstPtr n1, BigNumConstPtr n2)
{
    return compare(n1, n2);
}

BigNumPtr add_bn(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
{
    return add(r, n1, n2);
}

BigNum add_bn(const BigNum &left, const BigNum &right)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    add(result.data(), left.data(), right.data());
    return result;
}

BigNumPtr sub_bn(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
{
    return subtract(r, n1, n2);
}

BigNum sub_bn(const BigNum &left, const BigNum &right)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    subtract(result.data(), left.data(), right.data());
    return result;
}

BigNumPtr neg_bn(BigNumPtr r, BigNumConstPtr n)
{
    return negate(r, n);
}

BigNumPtr unsafe_mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    return multiply(r, n1, n2);
}

BigNumPtr unsafe_square_bn(BigNumPtr r, BigNumPtr n)
{
    return square(r, n);
}

BigNumPtr add_a_bn(BigNumPtr r, BigNumConstPtr n)
{
    return add(r, r, n);
}

void add_a_bn(BigNum &result, const BigNum &source)
{
    add(result.data(), result.data(), source.data());
}

BigNumPtr sub_a_bn(BigNumPtr r, BigNumConstPtr n)
{
    return subtract(r, r, n);
}

void sub_a_bn(BigNum &result, const BigNum &source)
{
    subtract(result.data(), result.data(), source.data());
}

BigNumPtr neg_a_bn(BigNumPtr r)
{
    return negate(r, r);
}

BigNumPtr full_mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    const bool sign1 = is_bn_neg(n1);
    const bool sign2 = is_bn_neg(n2);
    full_multiply(r, n1, n2);
    if (sign1)
    {
        neg_a_bn(n1);
    }
    if (sign2)
    {
        neg_a_bn(n2);
    }
    return r;
}

BigNumPtr mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2)
{
    const bool sign1 = is_bn_neg(n1);
    const bool sign2 = is_bn_neg(n2);
    multiply(r, n1, n2);
    if (sign1)
    {
        neg_a_bn(n1);
    }
    if (sign2)
    {
        neg_a_bn(n2);
    }
    return r;
}

BigNumPtr full_square_bn(BigNumPtr r, BigNumPtr n)
{
    const bool sign = is_bn_neg(n);
    full_square(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

BigNumPtr square_bn(BigNumPtr r, BigNumPtr n)
{
    const bool sign = is_bn_neg(n);
    square(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

BigNumPtr div_bn(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
{
    return binary_operation(r, n1, n2, divide);
}

} // namespace id::math
