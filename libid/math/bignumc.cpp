// SPDX-License-Identifier: GPL-3.0-only
//
// C routines equivalent to ASM routines in bignuma.asm

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include "math/big.h"

#include <array>
#include <cfloat>
#include <cstdio>
#include <cstring>

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
BigNum clear_bn(BigNum r)
{
    std::memset(r, 0, g_bn_length);  // set array to zero
    return r;
}

/********************************************************************/
// r = max positive value
BigNum max_bn(BigNum r)
{
    std::memset(r, 0xFF, g_bn_length-1);  // set to max values
    r[g_bn_length-1] = 0x7F;  // turn off the sign bit
    return r;
}

/********************************************************************/
// r = n
BigNum copy_bn(BigNum r, BigNum n)
{
    std::memcpy(r, n, g_bn_length);
    return r;
}

/***************************************************************************/
// n1 != n2 ?
// RETURNS:
//  if n1 == n2 returns 0
//  if n1 > n2 returns a positive (bytes left to go when mismatch occured)
//  if n1 < n2 returns a negative (bytes left to go when mismatch occured)
int cmp_bn(BigNum n1, BigNum n2)
{
    // two bytes at a time
    // signed comparison for msb
    S16 value1 = BIG_ACCESS_S16((S16 *) (n1 + g_bn_length - 2));
    S16 value2 = BIG_ACCESS_S16((S16 *) (n2 + g_bn_length - 2));
    if (value1 > value2)
    {
        // now determine which of the two bytes was different
        if ((S16) (value1 & 0xFF00) > (S16) (value2 & 0xFF00)) // compare just high bytes
        {
            return g_bn_length; // high byte was different
        }

        return g_bn_length - 1; // low byte was different
    }
    if (value1 < value2)
    {
        // now determine which of the two bytes was different
        if ((S16) (value1 & 0xFF00) < (S16) (value2 & 0xFF00)) // compare just high bytes
        {
            return -(g_bn_length); // high byte was different
        }

        return -(g_bn_length - 1); // low byte was different
    }

    // unsigned comparison for the rest
    for (int i = g_bn_length-4; i >= 0; i -= 2)
    {
        U16 n1_value = BIG_ACCESS16(n1 + i);
        U16 n2_value = BIG_ACCESS16(n2 + i);
        if (n1_value > n2_value)
        {
            // now determine which of the two bytes was different
            if ((n1_value & 0xFF00) > (n2_value & 0xFF00)) // compare just high bytes
            {
                return i + 2; // high byte was different
            }

            return i + 1; // low byte was different
        }
        if (n1_value < n2_value)
        {
            // now determine which of the two bytes was different
            if ((n1_value & 0xFF00) < (n2_value & 0xFF00)) // compare just high bytes
            {
                return -(i + 2); // high byte was different
            }

            return -(i + 1); // low byte was different
        }
    }
    return 0;
}

/********************************************************************/
// r < 0 ?
// returns 1 if negative, 0 if positive or zero
bool is_bn_neg(BigNum n)
{
    return (S8)n[g_bn_length-1] < 0;
}

/********************************************************************/
// n != 0 ?
// RETURNS: true if n != 0
bool is_bn_not_zero(BigNum n)
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
// r = n1 + n2
BigNum add_bn(BigNum r, BigNum n1, BigNum n2)
{
    U32 sum = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        sum += (U32)BIG_ACCESS16(n1+i) + (U32)BIG_ACCESS16(n2+i); // add 'em up
        BIG_SET16(r+i, (U16)sum);   // store the lower 2 bytes
        sum >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r += n
BigNum add_a_bn(BigNum r, BigNum n)
{
    U32 sum = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        sum += (U32)BIG_ACCESS16(r+i) + (U32)BIG_ACCESS16(n+i); // add 'em up
        BIG_SET16(r+i, (U16)sum);   // store the lower 2 bytes
        sum >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n1 - n2
BigNum sub_bn(BigNum r, BigNum n1, BigNum n2)
{
    U32 diff = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        diff = (U32)BIG_ACCESS16(n1+i) - ((U32)BIG_ACCESS16(n2+i)-(S32)(S16)diff); // subtract with borrow
        BIG_SET16(r+i, (U16)diff);   // store the lower 2 bytes
        diff >>= 16; // shift the underflow for next time
    }
    return r;
}

/********************************************************************/
// r -= n
BigNum sub_a_bn(BigNum r, BigNum n)
{
    U32 diff = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        diff = (U32)BIG_ACCESS16(r+i) - ((U32)BIG_ACCESS16(n+i)-(S32)(S16)diff); // subtract with borrow
        BIG_SET16(r+i, (U16)diff);   // store the lower 2 bytes
        diff >>= 16; // shift the underflow for next time
    }
    return r;
}

/********************************************************************/
// r = -n
BigNum neg_bn(BigNum r, BigNum n)
{
    int i;
    U32 neg = 1; // to get the 2's complement started

    // two bytes at a time
    for (i = 0; neg != 0 && i < g_bn_length; i += 2)
    {
        U16 t_short = ~BIG_ACCESS16(n + i);
        neg += ((U32)t_short); // two's complement
        BIG_SET16(r+i, (U16)neg);   // store the lower 2 bytes
        neg >>= 16; // shift the sign bit for next time
    }
    // if neg was 0, then just "not" the rest
    for (; i < g_bn_length; i += 2)
    {
        // notice that BIG_ACCESS16() and BIG_SET16() are not needed here
        *(U16 *)(r+i) = ~*(U16 *)(n+i); // toggle all the bits
    }
    return r;
}

/********************************************************************/
// r *= -1
BigNum neg_a_bn(BigNum r)
{
    int i;
    U32 neg = 1; // to get the 2's complement started

    // two bytes at a time
    for (i = 0; neg != 0 && i < g_bn_length; i += 2)
    {
        U16 t_short = ~BIG_ACCESS16(r + i);
        neg += ((U32)t_short); // two's complement
        BIG_SET16(r+i, (U16)neg);   // store the lower 2 bytes
        neg >>= 16; // shift the sign bit for next time
    }
    // if neg was 0, then just "not" the rest
    for (; i < g_bn_length; i += 2)
    {
        // notice that BIG_ACCESS16() and BIG_SET16() are not needed here
        *(U16 *)(r+i) = ~*(U16 *)(r+i); // toggle all the bits
    }
    return r;
}

/********************************************************************/
// r = 2*n
BigNum double_bn(BigNum r, BigNum n)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += (U32)BIG_ACCESS16(n+i) << 1; // double it
        BIG_SET16(r+i, (U16)prod);   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r *= 2
BigNum double_a_bn(BigNum r)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += (U32)BIG_ACCESS16(r+i) << 1; // double it
        BIG_SET16(r+i, (U16)prod);   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n/2
BigNum half_bn(BigNum r, BigNum n)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += (U32) (((S32) (S16) BIG_ACCESS16(n + i) << 16) >> 1); // shift to upper 2 bytes and half it
        BIG_SET16(r + i, (U16) (quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += (U32) BIG_ACCESS16(n + i) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, (U16) (quot >> 16));                   // store the upper 2 bytes
        quot <<= 16;                                            // shift the underflow for next time
    }

    return r;
}

/********************************************************************/
// r /= 2
BigNum half_a_bn(BigNum r)
{
    U32 quot = 0;

    // two bytes at a time

    // start with an arithmetic shift
    {
        const int i = g_bn_length - 2;
        quot += (U32) (((S32) (S16) BIG_ACCESS16(r + i) << 16) >> 1); // shift to upper 2 bytes and half it
        BIG_SET16(r + i, (U16) (quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }

    for (int i = g_bn_length - 4; i >= 0; i -= 2)
    {
        // looks wierd, but properly sign extends argument
        quot += (U32) BIG_ACCESS16(r + i) << 16 >> 1; // shift to upper 2 bytes and half it
        BIG_SET16(r + i, (U16) (quot >> 16));                         // store the upper 2 bytes
        quot <<= 16;                                                  // shift the underflow for next time
    }
    return r;
}

/************************************************************************/
// r = n1 * n2
// Note: r will be a double wide result, 2*g_bn_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigNum unsafe_full_mult_bn(BigNum r, BigNum n1, BigNum n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    BigNum rp2;
    // pointers for r

    bool sign1 = is_bn_neg(n1);
    if (sign1) // =, not ==
    {
        neg_a_bn(n1);
    }
    const bool same_var = (n1 == n2);
    if (!same_var) // check to see if they're the same pointer
    {
        sign2 = is_bn_neg(n2);
        if (sign2) // =, not ==
        {
            neg_a_bn(n2);
        }
    }

    BigNum n1_p = n1;
    int steps = g_bn_length >> 1; // two bytes at a time
    int carry_steps = double_steps = (steps << 1) - 2;
    g_bn_length <<= 1;
    clear_bn(r);        // double width
    g_bn_length >>= 1;
    BigNum rp1 = rp2 = r;
    for (int i = 0; i < steps; i++)
    {
        BigNum n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            U32 prod = (U32) BIG_ACCESS16(n1_p) * (U32) BIG_ACCESS16(n2_p); // U16*U16=U32
            U32 sum = (U32) BIG_ACCESS16(rp2) + prod; // add to previous, including overflow
            BIG_SET16(rp2, (U16)sum); // save the lower 2 bytes
            sum >>= 16;             // keep just the upper 2 bytes
            BigNum rp3 = rp2 + 2;          // move over 2 bytes
            sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
            BIG_SET16(rp3 , (U16)sum); // save what was the upper two bytes
            sum >>= 16;             // keep just the overflow
            for (int k = 0; sum != 0 && k < carry_steps; k++)
            {
                rp3 += 2;               // move over 2 bytes
                sum += BIG_ACCESS16(rp3);     // add to what was the overflow
                BIG_SET16(rp3, (U16)sum); // save what was the overflow
                sum >>= 16;             // keep just the new overflow
            }
            n2_p += 2;       // to next word
            rp2 += 2;
            carry_steps--;  // use one less step
        }
        n1_p += 2;           // to next word
        rp2 = rp1 += 2;
        carry_steps = --double_steps; // decrease doubles steps and reset carry_steps
    }

    // if they were the same or same sign, the product must be positive
    if (!same_var && sign1 != sign2)
    {
        g_bn_length <<= 1;         // for a double wide number
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
BigNum unsafe_mult_bn(BigNum r, BigNum n1, BigNum n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    BigNum rp1;

    int bnl = g_bn_length;
    bool sign1 = is_bn_neg(n1);
    if (sign1 != 0)   // =, not ==
    {
        neg_a_bn(n1);
    }
    const bool same_var = (n1 == n2);
    if (!same_var) // check to see if they're the same pointer
    {
        sign2 = is_bn_neg(n2);
        if (sign2)   // =, not ==
        {
            neg_a_bn(n2);
        }
    }
    BigNum n1_p = n1;
    n2 += (g_bn_length << 1) - g_r_length;  // shift n2 over to where it is needed

    g_bn_length = g_r_length;
    clear_bn(r);        // zero out r, g_r_length width
    g_bn_length = bnl;

    int steps = (g_r_length - g_bn_length) >> 1;
    int skips = (g_bn_length >> 1) - steps;
    int carry_steps = double_steps = (g_r_length >> 1) - 2;
    BigNum rp2 = rp1 = r;
    for (int i = g_bn_length >> 1; i > 0; i--)
    {
        BigNum n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            U32 prod = (U32) BIG_ACCESS16(n1_p) * (U32) BIG_ACCESS16(n2_p); // U16*U16=U32
            U32 sum = (U32) BIG_ACCESS16(rp2) + prod; // add to previous, including overflow
            BIG_SET16(rp2, (U16)sum); // save the lower 2 bytes
            sum >>= 16;             // keep just the upper 2 bytes
            BigNum rp3 = rp2 + 2;          // move over 2 bytes
            sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
            BIG_SET16(rp3, (U16)sum); // save what was the upper two bytes
            sum >>= 16;             // keep just the overflow
            for (int k = 0; sum != 0 && k < carry_steps; k++)
            {
                rp3 += 2;               // move over 2 bytes
                sum += BIG_ACCESS16(rp3);     // add to what was the overflow
                BIG_SET16(rp3, (U16)sum); // save what was the overflow
                sum >>= 16;             // keep just the new overflow
            }
            n2_p += 2;                   // increase by two bytes
            rp2 += 2;
            carry_steps--;
        }
        n1_p += 2;   // increase by two bytes

        if (skips != 0)
        {
            n2 -= 2;    // shift n2 back a word
            steps++;    // one more step this time
            skips--;    // keep track of how many times we've done this
        }
        else
        {
            rp1 += 2;           // shift forward a word
            double_steps--;      // reduce the carry steps needed next time
        }
        rp2 = rp1;
        carry_steps = double_steps;
    }

    // if they were the same or same sign, the product must be positive
    if (!same_var && sign1 != sign2)
    {
        g_bn_length = g_r_length;
        neg_a_bn(r);            // wider bignumber
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
BigNum unsafe_full_square_bn(BigNum r, BigNum n)
{
    int double_steps;
    BigNum rp1;
    BigNum rp3;
    U32 prod;
    U32 sum;

    if (is_bn_neg(n))    // don't need to keep track of sign since the
    {
        neg_a_bn(n);   // answer must be positive.
    }

    g_bn_length <<= 1;
    clear_bn(r);        // zero out r, double width
    g_bn_length >>= 1;

    int steps = (g_bn_length >> 1) - 1;
    int carry_steps = double_steps = (steps << 1) - 1;
    BigNum rp2 = rp1 = r + 2;  // start with second two-byte word
    BigNum n1_p = n;
    if (steps != 0) // if zero, then skip all the middle term calculations
    {
        for (int i = steps; i > 0; i--) // steps gets altered, count backwards
        {
            BigNum n2_p = n1_p + 2;  // set n2p pointer to 1 step beyond n1p
            for (int j = 0; j < steps; j++)
            {
                prod = (U32)BIG_ACCESS16(n1_p) * (U32)BIG_ACCESS16(n2_p); // U16*U16=U32
                sum = (U32)BIG_ACCESS16(rp2) + prod; // add to previous, including overflow
                BIG_SET16(rp2, (U16)sum); // save the lower 2 bytes
                sum >>= 16;             // keep just the upper 2 bytes
                rp3 = rp2 + 2;          // move over 2 bytes
                sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
                BIG_SET16(rp3, (U16)sum); // save what was the upper two bytes
                sum >>= 16;             // keep just the overflow
                for (int k = 0; sum != 0 && k < carry_steps; k++)
                {
                    rp3 += 2;               // move over 2 bytes
                    sum += BIG_ACCESS16(rp3);     // add to what was the overflow
                    BIG_SET16(rp3, (U16)sum); // save what was the overflow
                    sum >>= 16;             // keep just the new overflow
                }
                n2_p += 2;       // increase by two bytes
                rp2 += 2;
                carry_steps--;
            }
            n1_p += 2;           // increase by two bytes
            rp2 = rp1 += 4;     // increase by 2 * two bytes
            carry_steps = double_steps -= 2;   // reduce the carry steps needed
            steps--;
        }
        // All the middle terms have been multiplied.  Now double it.
        g_bn_length <<= 1;     // double wide bignumber
        double_a_bn(r);
        g_bn_length >>= 1;
        // finished with middle terms
    }

    // Now go back and add in the squared terms.
    n1_p = n;
    steps = (g_bn_length >> 1);
    carry_steps = double_steps = (steps << 1) - 2;
    rp1 = r;
    for (int i = 0; i < steps; i++)
    {
        // square it
        prod = (U32)BIG_ACCESS16(n1_p) * (U32)BIG_ACCESS16(n1_p); // U16*U16=U32
        sum = (U32)BIG_ACCESS16(rp1) + prod; // add to previous, including overflow
        BIG_SET16(rp1, (U16)sum); // save the lower 2 bytes
        sum >>= 16;             // keep just the upper 2 bytes
        rp3 = rp1 + 2;          // move over 2 bytes
        sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
        BIG_SET16(rp3, (U16)sum); // save what was the upper two bytes
        sum >>= 16;             // keep just the overflow
        for (int k = 0; sum != 0 && k < carry_steps; k++)
        {
            rp3 += 2;               // move over 2 bytes
            sum += BIG_ACCESS16(rp3);     // add to what was the overflow
            BIG_SET16(rp3, (U16)sum); // save what was the overflow
            sum >>= 16;             // keep just the new overflow
        }
        n1_p += 2;       // increase by 2 bytes
        rp1 += 4;       // increase by 4 bytes
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
BigNum unsafe_square_bn(BigNum r, BigNum n)
{
    int double_steps;
    BigNum n2_p;
    BigNum rp1;
    BigNum rp3;
    U32 prod;
    U32 sum;

    // This whole procedure would be a great deal simpler if we could assume that
    // g_r_length < 2*g_bn_length (that is, not =).  Therefore, we will take the
    // easy way out and call full_square_bn() if it is.
    if (g_r_length == (g_bn_length << 1))   // g_r_length == 2*g_bn_length
    {
        return unsafe_full_square_bn(r, n);    // call full_square_bn() and quit
    }

    if (is_bn_neg(n))    // don't need to keep track of sign since the
    {
        neg_a_bn(n);   // answer must be positive.
    }

    int bnl = g_bn_length;
    g_bn_length = g_r_length;
    clear_bn(r);        // zero out r, of width g_r_length
    g_bn_length = bnl;

    // determine whether r is on an odd or even two-byte word in the number
    int rodd = (U16) (((g_bn_length << 1) - g_r_length) >> 1) & 0x0001;
    int i = (g_bn_length >> 1)-1;
    int steps = (g_r_length - g_bn_length) >> 1;
    int carry_steps = double_steps = (g_bn_length >> 1) + steps - 2;
    int skips = (i - steps) >> 1;     // how long to skip over pointer shifts
    BigNum rp2 = rp1 = r;
    BigNum n1_p = n;
    BigNum n3_p = n2_p = n1_p + (((g_bn_length >> 1) - steps) << 1);    // n2p = n1p + 2*(g_bn_length/2 - steps)
    if (i != 0) // if zero, skip middle term calculations
    {
        // i is already set
        for (; i > 0; i--)
        {
            for (int j = 0; j < steps; j++)
            {
                prod = (U32)BIG_ACCESS16(n1_p) * (U32)BIG_ACCESS16(n2_p); // U16*U16=U32
                sum = (U32)BIG_ACCESS16(rp2) + prod; // add to previous, including overflow
                BIG_SET16(rp2, (U16)sum); // save the lower 2 bytes
                sum >>= 16;             // keep just the upper 2 bytes
                rp3 = rp2 + 2;          // move over 2 bytes
                sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
                BIG_SET16(rp3, (U16)sum); // save what was the upper two bytes
                sum >>= 16;             // keep just the overflow
                for (int k = 0; sum != 0 && k < carry_steps; k++)
                {
                    rp3 += 2;               // move over 2 bytes
                    sum += BIG_ACCESS16(rp3);     // add to what was the overflow
                    BIG_SET16(rp3, (U16)sum); // save what was the overflow
                    sum >>= 16;             // keep just the new overflow
                }
                n2_p += 2;       // increase by 2-byte word size
                rp2 += 2;
                carry_steps--;
            }
            n1_p += 2;       // increase by 2-byte word size
            if (skips > 0)
            {
                n2_p = n3_p -= 2;
                steps++;
                skips--;
            }
            else if (skips == 0)    // only gets executed once
            {
                steps -= rodd;  // rodd is 1 or 0
                double_steps -= rodd+1;
                rp1 += (rodd+1) << 1;
                n2_p = n1_p+2;
                skips--;
            }
            else // skips < 0
            {
                steps--;
                double_steps -= 2;
                rp1 += 4;           // add two 2-byte words
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
    i = (g_bn_length << 1)-g_r_length;
    rp1 = r + ((U16)i & (U16)0x0002);
    i = (U16)((i >> 1)+1) & (U16)0xFFFE;
    n1_p = n + i;
    // i here is no longer a temp var., but will be used as a loop counter
    i = (g_bn_length - i) >> 1;
    carry_steps = double_steps = (i << 1)-2;
    // i is already set
    for (; i > 0; i--)
    {
        // square it
        prod = (U32)BIG_ACCESS16(n1_p) * (U32)BIG_ACCESS16(n1_p); // U16*U16=U32
        sum = (U32)BIG_ACCESS16(rp1) + prod; // add to previous, including overflow
        BIG_SET16(rp1, (U16)sum); // save the lower 2 bytes
        sum >>= 16;             // keep just the upper 2 bytes
        rp3 = rp1 + 2;          // move over 2 bytes
        sum += BIG_ACCESS16(rp3);     // add what was the upper two bytes
        BIG_SET16(rp3, (U16)sum); // save what was the upper two bytes
        sum >>= 16;             // keep just the overflow
        for (int k = 0; sum != 0 && k < carry_steps; k++)
        {
            rp3 += 2;               // move over 2 bytes
            sum += BIG_ACCESS16(rp3);     // add to what was the overflow
            BIG_SET16(rp3, (U16)sum); // save what was the overflow
            sum >>= 16;             // keep just the new overflow
        }
        n1_p += 2;
        rp1 += 4;
        carry_steps = double_steps -= 2;
    }
    return r;
}

/********************************************************************/
// r = n * u  where u is an unsigned integer
BigNum mult_bn_int(BigNum r, BigNum n, U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += (U32)BIG_ACCESS16(n+i) * u; // n*u
        BIG_SET16(r+i, (U16)prod);   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r *= u  where u is an unsigned integer
BigNum mult_a_bn_int(BigNum r, U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += (U32)BIG_ACCESS16(r+i) * u; // r*u
        BIG_SET16(r+i, (U16)prod);   // store the lower 2 bytes
        prod >>= 16; // shift the overflow for next time
    }
    return r;
}

/********************************************************************/
// r = n / u  where u is an unsigned integer
BigNum unsafe_div_bn_int(BigNum r, BigNum n,  U16 u)
{
    U16 rem = 0;

    bool sign = is_bn_neg(n);
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
        U32 full_number = ((U32) rem << 16) + (U32) BIG_ACCESS16(n + i);
        U16 quot = (U16) (full_number / u);
        rem  = (U16)(full_number % u);
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
BigNum div_a_bn_int(BigNum r, U16 u)
{
    U16 rem = 0;

    bool sign = is_bn_neg(r);
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
        U32 full_number = ((U32) rem << 16) + (U32) BIG_ACCESS16(r + i);
        U16 quot = (U16) (full_number / u);
        rem  = (U16)(full_number % u);
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
LDouble bn_to_float(BigNum n)
{
    LDouble f = 0;

    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        sign_flag = true;
        neg_a_bn(n);
    }

    int exponent = g_int_length - 1;
    BigNum get_byte = n + g_bn_length - 1;
    while (*get_byte == 0 && get_byte >= n)
    {
        get_byte--;
        exponent--;
    }

    // There is no need to use all g_bn_length bytes.  To get the full
    // precision of LDouble, all you need is LDBL_MANT_DIG/8+1.
    for (int i = 0; i < (LDBL_MANT_DIG/8+1) && get_byte >= n; i++, get_byte--)
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
BigFloat clear_bf(BigFloat r)
{
    std::memset(r, 0, g_bf_length+2);  // set array to zero
    return r;
}

/********************************************************************/
// r = n
BigFloat copy_bf(BigFloat r, BigFloat n)
{
    std::memcpy(r, n, g_bf_length+2);
    return r;
}

/*********************************************************************/
//  b = f
//  Converts a double to a bigfloat
BigFloat float_to_bf(BigFloat r, LDouble f)
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

    BIG_SET16(r + g_bf_length, (S16)power); // exp

    return r;
}

/*********************************************************************/
//  b = f
//  Converts a long double to a bigfloat
BigFloat float_to_bf1(BigFloat r, LDouble f)
{
    char msg[80];
    std::snprintf(msg, std::size(msg), "%-.22Le", f);
    str_to_bf(r, msg);
    return r;
}

/*********************************************************************/
//  f = b
//  Converts a bigfloat to a double
LDouble bf_to_float(BigFloat n)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    int il = g_int_length;
    g_int_length = 2;
    LDouble f = bn_to_float(n);
    g_bn_length = bnl;
    g_int_length = il;

    int power = (S16) BIG_ACCESS16(n + g_bf_length);
    f = scale256(f, power);

    return f;
}

/********************************************************************/
// extracts the mantissa and exponent of f
// finds m and n such that 1<=|m|<256 and f = m*256^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract256(LDouble f, int *exp_ptr)
{
    return extract_value(f, 256, exp_ptr);
}

/********************************************************************/
// calculates and returns the value of f*256^n
// sort of like ldexp()
//
// n must be in the range -2^12 <= n < 2^12 (2^12=4096),
// which should not be a problem
LDouble scale256(LDouble f, int n)
{
    return scale_value(f, 256, n);
}
