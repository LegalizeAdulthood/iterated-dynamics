// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/big.h>
#include <math/big_init.h>

#include <algorithm>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>

using namespace id::misc;

namespace id::math
{

namespace
{

bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

int parse_exponent(std::string_view text)
{
    bool negative = false;
    if (!text.empty() && text.front() == '+')
    {
        text.remove_prefix(1);
    }
    else if (!text.empty() && text.front() == '-')
    {
        negative = true;
        text.remove_prefix(1);
    }

    int exponent = 0;
    for (const char c : text)
    {
        if (!is_digit(c))
        {
            break;
        }
        exponent = exponent * 10 + c - '0';
    }
    return negative ? -exponent : exponent;
}

long parse_long(const std::string_view text)
{
    long value{};
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

/************************************************************************/
// r = n1 * n2
// Note: r will be a double wide result, 2*g_bn_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
Byte *full_multiply(Byte *r, Byte *n1, Byte *n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    Byte *rp2;
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

    Byte *n1_p = n1;
    const int steps = g_bn_length >> 1; // two bytes at a time
    int carry_steps = double_steps = (steps << 1) - 2;
    g_bn_length <<= 1;
    clear_bn(r);                        // double width
    g_bn_length >>= 1;
    Byte *rp1 = rp2 = r;
    for (int i = 0; i < steps; i++)
    {
        Byte *n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            const U32 prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
            U32 sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
            BIG_SET16(rp2, static_cast<U16>(sum));                // save the lower 2 bytes
            sum >>= 16;                                           // keep just the upper 2 bytes
            Byte *rp3 = rp2 + 2;                                  // move over 2 bytes
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
Byte *multiply(Byte *r, Byte *n1, Byte *n2)
{
    bool sign2 = false;
    int double_steps;
    // pointers for n1, n2
    Byte *rp1;

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
    Byte *n1_p = n1;
    n2 += (g_bn_length << 1) - g_r_length; // shift n2 over to where it is needed

    g_bn_length = g_r_length;
    clear_bn(r);                           // zero out r, g_r_length width
    g_bn_length = bnl;

    int steps = (g_r_length - g_bn_length) >> 1;
    int skips = (g_bn_length >> 1) - steps;
    int carry_steps = double_steps = (g_r_length >> 1) - 2;
    Byte *rp2 = rp1 = r;
    for (int i = g_bn_length >> 1; i > 0; i--)
    {
        Byte *n2_p = n2;
        for (int j = 0; j < steps; j++)
        {
            const U32 prod = static_cast<U32>(BIG_ACCESS16(n1_p)) * static_cast<U32>(BIG_ACCESS16(n2_p)); // U16*U16=U32
            U32 sum = static_cast<U32>(BIG_ACCESS16(rp2)) + prod; // add to previous, including overflow
            BIG_SET16(rp2, static_cast<U16>(sum));                // save the lower 2 bytes
            sum >>= 16;                                           // keep just the upper 2 bytes
            Byte *rp3 = rp2 + 2;                                  // move over 2 bytes
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
Byte *full_square(Byte *r, Byte *n)
{
    int double_steps;
    Byte *rp1;
    Byte *rp3;
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
    Byte *rp2 = rp1 = r + 2;            // start with second two-byte word
    Byte *n1_p = n;
    if (steps != 0)                     // if zero, then skip all the middle term calculations
    {
        for (int i = steps; i > 0; i--) // steps gets altered, count backwards
        {
            Byte *n2_p = n1_p + 2;      // set n2p pointer to 1 step beyond n1p
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
Byte *square(Byte *r, Byte *n)
{
    int double_steps;
    Byte *n2_p;
    Byte *rp1;
    Byte *rp3;
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
    int skips = (i - steps) >> 1;                                   // how long to skip over pointer shifts
    Byte *rp2 = rp1 = r;
    Byte *n1_p = n;
    Byte *n3_p = n2_p = n1_p + (((g_bn_length >> 1) - steps) << 1); // n2p = n1p + 2*(g_bn_length/2 - steps)
    if (i != 0)                                                     // if zero, skip middle term calculations
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

} // namespace

/************************************************************************/
// convert_bn  -- convert bignum numbers from old to new lengths
int convert_bn(Byte *new_num, const Byte *old_num, const int new_bn_len, const int new_int_len, const int old_bn_len,
    const int old_int_len)
{
    const int new_fraction = new_bn_len - new_int_len;
    const int old_fraction = old_bn_len - old_int_len;
    const int fraction = std::min(new_fraction, old_fraction);
    const int integer = std::min(new_int_len, old_int_len);
    std::memset(new_num, 0, new_bn_len);
    std::memcpy(new_num + new_fraction - fraction, old_num + old_fraction - fraction, fraction + integer);
    return 0;
}

void convert_bn_signed(Byte *new_num, const Byte *old_num, const int new_bn_len, const int new_int_len,
    const int old_bn_len, const int old_int_len)
{
    convert_bn(new_num, old_num, new_bn_len, new_int_len, old_bn_len, old_int_len);
    // Legacy scratch conversion zero-fills new integer bytes;
    // live signed values require sign extension instead.
    if (new_int_len > old_int_len && (old_num[old_bn_len - 1] & 0x80) != 0)
    {
        const int extension = new_int_len - old_int_len;
        std::memset(new_num + new_bn_len - extension, 0xff, extension);
    }
}

void migrate_bn(Byte *destination, const Byte *source, const std::size_t blocks, const int new_bn_length,
    const int new_int_length, const int new_padding, const int old_bn_length, const int old_int_length,
    const int old_padding)
{
    const bool product = blocks == 2;
    const int old_length = product ? old_bn_length + old_padding : old_bn_length;
    const int old_integer = product ? 2 * old_int_length : old_int_length;
    const int new_length = product ? new_bn_length + new_padding : new_bn_length;
    const int new_integer = product ? 2 * new_int_length : new_int_length;
    convert_bn_signed(destination, source, new_length, new_integer, old_length, old_integer);
}

/*********************************************************************/
//  bn = floor(bf)
//  Converts a bigfloat to a bignumber (integer)
//  g_bf_length must be at least g_bn_length+2
Byte *bf_to_bn(Byte *n, const Byte *f)
{
    const int f_exp = static_cast<S16>(BIG_ACCESS16(f + g_bf_length));
    if (f_exp >= g_int_length)
    {
        // if it's too big, use max value
        max_bn(n);
        if (is_bf_neg(f))
        {
            neg_a_bn(n);
        }
        return n;
    }

    if (-f_exp > g_bn_length - g_int_length) // too small, return zero
    {
        clear_bn(n);
        return n;
    }

    // already checked for over/underflow, this should be ok
    const int move_bytes = g_bn_length - g_int_length + f_exp + 1;
    std::memcpy(n, f + g_bf_length - move_bytes - 1, move_bytes);
    const Byte hi_byte = *(f + g_bf_length - 1);
    std::memset(n + move_bytes, hi_byte, g_bn_length - move_bytes); // sign extends
    return n;
}

/*********************************************************************/
//  bf = bn
//  Converts a bignumber (integer) to a bigfloat
//  g_bf_length must be at least g_bn_length+2
Byte *bn_to_bf(Byte *f, const Byte *n)
{
    std::memcpy(f + g_bf_length - g_bn_length - 1, n, g_bn_length);
    std::memset(f, 0, g_bf_length - g_bn_length - 1);
    *(f + g_bf_length - 1) = static_cast<Byte>(is_bn_neg(n) ? 0xFF : 0x00); // sign extend
    BIG_SET16(f + g_bf_length, static_cast<S16>(g_int_length - 1));         // exp
    norm_bf(f);
    return f;
}

/************************************************************************/
// convert_bf  -- convert bigfloat numbers from old to new lengths
int convert_bf(Byte *new_num, const Byte *old_num, const int new_bf_len, const int old_bf_len)
{
    const int retained = std::min(new_bf_len, old_bf_len);
    std::memset(new_num, 0, new_bf_len + 2);
    std::memcpy(new_num + new_bf_len - retained, old_num + old_bf_len - retained, retained + 2);
    return 0;
}

const Byte *unpack_bf(Byte *destination, const Byte *source, const int stored_length)
{
    convert_bf(destination, source, g_bf_length, stored_length);
    return source + stored_length + 2;
}

void migrate_bf(Byte *destination, const Byte *source, const std::size_t blocks, const int new_bf_length,
    const int new_padding, const int old_bf_length, const int old_padding)
{
    const bool product = blocks == 2;
    const int old_length = product ? old_bf_length + old_padding : old_bf_length;
    const int new_length = product ? new_bf_length + new_padding : new_bf_length;
    convert_bf(destination, source, new_length, old_length);
}

Byte *unsafe_mult_bn(Byte *r, Byte *n1, Byte *n2)
{
    return multiply(r, n1, n2);
}

Byte *full_mult_bn(Byte *r, Byte *n1, Byte *n2)
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

Byte *mult_bn(Byte *r, Byte *n1, Byte *n2)
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

Byte *unsafe_square_bn(Byte *r, Byte *n)
{
    return square(r, n);
}

Byte *full_square_bn(Byte *r, Byte *n)
{
    const bool sign = is_bn_neg(n);
    full_square(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

Byte *square_bn(Byte *r, Byte *n)
{
    const bool sign = is_bn_neg(n);
    square(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

Byte *msb_bn(Byte *value)
{
    return value + g_shift_factor;
}

const Byte *msb_bn(const Byte *value)
{
    return value + g_shift_factor;
}

/*********************************************************************/
//  l = floor(b), floor rounds down
//  Converts the integer part a bignumber to a long
long bn_to_int(const Byte *n)
{
    long value = 0;

    const Byte *ones_byte = n + g_bn_length - g_int_length;
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        value = *ones_byte;
        break;
    case 2:
        value = BIG_ACCESS16(ones_byte);
        break;
    case 4:
        value = BIG_ACCESS32(ones_byte);
        break;
    }
    return value;
}

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

/********************************************************************/
// r = n / u  where u is an unsigned integer
Byte *unsafe_div_bn_int(Byte *r, Byte *n, const U16 u)
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
    for (int i = g_bn_length - 2; i >= 0; i -= 2)
    {
        const U32 full_number = (static_cast<U32>(rem) << 16) + static_cast<U32>(BIG_ACCESS16(n + i));
        const U16 quot = static_cast<U16>(full_number / u);
        rem = static_cast<U16>(full_number % u);
        BIG_SET16(r + i, quot);
    }

    if (sign)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r /= u  where u is an unsigned integer
Byte *div_a_bn_int(Byte *r, const U16 u)
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
    for (int i = g_bn_length - 2; i >= 0; i -= 2)
    {
        const U32 full_number = (static_cast<U32>(rem) << 16) + static_cast<U32>(BIG_ACCESS16(r + i));
        const U16 quot = static_cast<U16>(full_number / u);
        rem = static_cast<U16>(full_number % u);
        BIG_SET16(r + i, quot);
    }

    if (sign)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r = 1/n
// uses tmp1 - tmp3 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n|    Make copy first if necessary.
Byte *unsafe_inv_bn(Byte *r, Byte *n, Byte *tmp1, Byte *tmp2, Byte *tmp3)
{
    // use Newton's recursive method for zeroing in on 1/n : r=r(2-rn)

    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        // will be a lot easier to deal with just positives
        sign_flag = true;
        neg_a_bn(n);
    }

    LDouble f = bn_to_float(n);
    if (f == 0) // division by zero
    {
        max_bn(r);
        return r;
    }
    f = 1 / f;       // approximate inverse
    const long max_val = (1L << ((g_int_length << 3) - 1)) - 1;
    if (f > max_val) // check for overflow
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

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    // orig_bntmp1 not needed here
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    Byte *orig_r = r;
    Byte *orig_n = n;
    // orig_bntmp1        = tmp1;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>(LDBL_DIG / LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bn_length - g_bn_length;
    // tmp1 = orig_bntmp1 + orig_bn_length - g_bn_length;

    float_to_bn(r, f);           // start with approximate inverse
    clear_bn(tmp2);              // will be used as 1.0 and 2.0

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bn_length - g_bn_length;
        n = orig_n + orig_bn_length - g_bn_length;
        // tmp1 = orig_bntmp1 + orig_bn_length - g_bn_length;

        unsafe_mult_bn(tmp1, r, n);                                                    // tmp1=rn
        int_to_bn(tmp2, 1);                                                            // tmp2 = 1.0
        if (g_bn_length == orig_bn_length && cmp_bn(tmp2, tmp1 + g_shift_factor) == 0) // if not different
        {
            break;                                                                     // they must be the same
        }
        int_to_bn(tmp2, 2);                                                            // tmp2 = 2.0
        sub_bn(tmp3, tmp2, tmp1 + g_shift_factor);                                     // tmp3=2-rn
        unsafe_mult_bn(tmp1, r, tmp3);                                                 // tmp1=r(2-rn)
        copy_bn(r, tmp1 + g_shift_factor);                                             // r = tmp1
    }

    // restore original values
    g_bn_length = orig_bn_length;
    g_padding = orig_padding;
    g_r_length = orig_r_length;
    g_shift_factor = orig_shift_factor;
    r = orig_r;

    if (sign_flag)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r = n1/n2
//      r - result of length g_bn_length
// uses tmp1 - tmp3 - global temp bignumbers
//  SIDE-EFFECTS:
//      n1, n2 can end up as GARBAGE
//      Make copies first if necessary.
Byte *divide(Byte *r, Byte *n1, Byte *n2, Byte *tmp1, Byte *tmp2, Byte *tmp3)
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

    unsafe_inv_bn(r, n2, tmp1, tmp2, tmp3);
    unsafe_mult_bn(tmp1, n1, r);
    copy_bn(r, tmp1 + g_shift_factor); // r = tmp1

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

Byte *binary_operation(Byte *result, const Byte *left, const Byte *right, Byte *copy1, Byte *copy2,
    Byte *(*operation)(Byte *, Byte *, Byte *) )
{
    copy_bn(copy1, left);
    copy_bn(copy2, right);
    return operation(result, copy1, copy2);
}

Byte *div_bn(Byte *r, const Byte *n1, const Byte *n2, Byte *tmp1, Byte *tmp2, Byte *tmp3, Byte *copy1, Byte *copy2)
{
    copy_bn(copy1, n1);
    copy_bn(copy2, n2);
    return divide(r, copy1, copy2, tmp1, tmp2, tmp3);
}

void set_integer_part(Byte *ones_byte, const long value)
{
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(value);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(value));
        break;
    case 4:
        BIG_SET32(ones_byte, value);
        break;
    }
}

std::string unsafe_bn_to_string(Byte *r, int dec)
{
    long value = 0;

    if (dec == 0)
    {
        dec = g_decimals;
    }
    Byte *ones_byte = r + g_bn_length - g_int_length;

    std::string result;
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
        result += '-';
    }
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        value = *ones_byte;
        break;
    case 2:
        value = BIG_ACCESS16(ones_byte);
        break;
    case 4:
        value = BIG_ACCESS32(ones_byte);
        break;
    }
    result += std::to_string(value);
    result += '.';
    for (int d = 0; d < dec; d++)
    {
        *ones_byte = 0; // clear out highest byte
        mult_a_bn_int(r, 10);
        if (is_bn_zero(r))
        {
            break;
        }
        result += static_cast<char>(*ones_byte + '0');
    }

    return result;
}

/********************************************************************/
// calculates and returns the value of f*b^n
// sort of like ldexp()
LDouble scale_value(LDouble f, LDouble b, const int n)
{
    LDouble total = 1;

    if (b == 0 || f == 0)
    {
        return 0;
    }

    if (n == 0)
    {
        return f;
    }

    int an = std::abs(n);

    while (an != 0)
    {
        if (an & 0x0001)
        {
            total *= b;
        }
        b *= b;
        an >>= 1;
    }

    if (n > 0)
    {
        f *= total;
    }
    else // n < 0
    {
        f /= total;
    }
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

/*********************************************************************/
//  f = b
//  Converts a bigfloat to a double
LDouble bf_to_float(Byte *n)
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
// finds m and n such that 1<=|m|<b and f = m*b^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract_value(LDouble f, LDouble b, int *exp_ptr)
{
    LDouble value[15];

    if (b <= 0 || f == 0)
    {
        *exp_ptr = 0;
        return 0;
    }

    const LDouble orig_b = b;
    const LDouble af = f >= 0 ? f : -f; // abs value
    LDouble ff = af > 1 ? af : 1 / af;
    int n = 0;
    unsigned power_two = 1;
    while (b < ff)
    {
        value[n] = b;
        n++;
        power_two <<= 1;
        b *= b;
    }

    *exp_ptr = 0;
    for (; n > 0; n--)
    {
        power_two >>= 1;
        if (value[n - 1] < ff)
        {
            ff /= value[n - 1];
            *exp_ptr += power_two;
        }
    }
    if (f < 0)
    {
        ff = -ff;
    }
    if (af < 1)
    {
        ff = orig_b / ff;
        *exp_ptr = -*exp_ptr - 1;
    }

    return ff;
}

/********************************************************************/
// extracts the mantissa and exponent of f
// finds m and n such that 1<=|m|<256 and f = m*256^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract256(const LDouble f, int *exp_ptr)
{
    return extract_value(f, 256, exp_ptr);
}

/*********************************************************************/
//  b = f
//  Converts a double to a bigfloat
Byte *float_to_bf(Byte *r, LDouble f)
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

/********************************************************************/
// r = 0
Byte *clear_bf(Byte *r)
{
    std::memset(r, 0, g_bf_length + 2); // set array to zero
    return r;
}

/********************************************************************/
// r = n
Byte *copy_bf(Byte *r, const Byte *n)
{
    std::memcpy(r, n, g_bf_length + 2);
    return r;
}

/********************************************************************/
// r = max positive value
Byte *max_bf(Byte *r)
{
    int_to_bf(r, 1);
    BIG_SET16(r + g_bf_length, static_cast<S16>(LDBL_MAX_EXP / 8));
    return r;
}

// r < 0 ?
bool is_bf_neg(const Byte *n)
{
    return static_cast<S8>(n[g_bf_length - 1]) < 0;
}

// n != 0 ?
bool is_bf_not_zero(const Byte *n)
{
    return std::any_of(n, n + g_bf_length, [](const Byte digit) { return digit != 0; });
}

bool is_bf_zero(const Byte *n)
{
    return !is_bf_not_zero(n);
}

// sign(r)
int sign_bf(const Byte *n)
{
    return is_bf_neg(n) ? -1 : is_bf_not_zero(n) ? 1 : 0;
}

// Keep the legacy signed mismatch position.
int cmp_bf(const Byte *n1, const Byte *n2)
{
    // compare signs
    const int sign1 = sign_bf(n1);
    const int sign2 = sign_bf(n2);
    if (sign1 > sign2)
    {
        return g_bf_length;
    }
    if (sign1 < sign2)
    {
        return -g_bf_length;
    }
    // signs are the same

    // compare exponents, using signed comparisons
    const S16 *n1_exp = reinterpret_cast<const S16 *>(n1 + g_bf_length);
    const S16 *n2_exp = reinterpret_cast<const S16 *>(n2 + g_bf_length);
    if (BIG_ACCESS_S16(n1_exp) > BIG_ACCESS_S16(n2_exp))
    {
        return sign1 * g_bf_length;
    }
    if (BIG_ACCESS_S16(n1_exp) < BIG_ACCESS_S16(n2_exp))
    {
        return -sign1 * g_bf_length;
    }

    // To get to this point, the signs must match
    // so unsigned comparison is ok.
    // two bytes at a time
    for (int i = g_bf_length - 2; i >= 0; i -= 2)
    {
        const U16 value1 = BIG_ACCESS16(n1 + i);
        const U16 value2 = BIG_ACCESS16(n2 + i);
        if (value1 > value2)
        {
            // now determine which of the two bytes was different
            if ((value1 & 0xFF00) > (value2 & 0xFF00)) // compare just high bytes
            {
                return i + 2;                          // high byte was different
            }

            return i + 1;                              // low byte was different
        }
        if (value1 < value2)
        {
            // now determine which of the two bytes was different
            if ((value1 & 0xFF00) < (value2 & 0xFF00)) // compare just high bytes
            {
                return -(i + 2);                       // high byte was different
            }

            return -(i + 1);                           // low byte was different
        }
    }
    return 0;
}

// r = -n
Byte *neg_bf(Byte *r, const Byte *n)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    const S16 *n_exp = reinterpret_cast<const S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    neg_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// r *= -1
Byte *neg_a_bf(Byte *r)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    neg_a_bn(r);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// r = |n|
Byte *abs_bf(Byte *r, const Byte *n)
{
    copy_bf(r, n);
    if (is_bf_neg(r))
    {
        neg_a_bf(r);
    }
    return r;
}

// r = |r|
Byte *abs_a_bf(Byte *r)
{
    if (is_bf_neg(r))
    {
        neg_a_bf(r);
    }
    return r;
}

/*********************************************************************/
//  b = l
//  Converts a long to a bigfloat
Byte *int_to_bf(Byte *r, const long value)
{
    clear_bf(r);
    BIG_SET32(r + g_bf_length - 4, static_cast<S32>(value));
    BIG_SET16(r + g_bf_length, static_cast<S16>(2));
    norm_bf(r);
    return r;
}

/**********************************************************************/
// strtobf() - converts a string into a bigfloat
//   r - pointer to a bigfloat
//   s - string in the floating point format [+-][dig].[dig]e[+-][dig]
//   note: the string may not be empty or have extra space.
//         It may use scientific notation.
// USES: tmp1

Byte *str_to_bf(Byte *r, std::string_view s, Byte *tmp1)
{
    Byte ones_byte;
    bool sign_flag = false;
    int power_ten = 0;

    clear_bf(r);
    if (s.empty())
    {
        return r;
    }

    if (s[0] == '+') // for + sign
    {
        s.remove_prefix(1);
    }
    else if (s[0] == '-') // for neg sign
    {
        sign_flag = true;
        s.remove_prefix(1);
    }
    if (s.empty())
    {
        return r;
    }

    const std::size_t d = s.find('.');
    const std::size_t e = s.find_first_of("eE");
    std::size_t l = s.size();
    if (e != std::string_view::npos)
    {
        power_ten = parse_exponent(s.substr(e + 1)); // read in the e (x10^) part
        l = e;                                       // just after last mantissa char
    }

    if (d != std::string_view::npos)                 // is there a decimal point?
    {
        while (l > 0 && is_digit(s[l - 1]))          // while a digit
        {
            ones_byte = static_cast<Byte>(s[--l] - '0');
            int_to_bf(tmp1, ones_byte);
            unsafe_add_a_bf(r, tmp1);
            div_a_bf_int(r, 10);
        }

        if (l > 0 && s[l - 1] == '.') // the digit was found
        {
            --l;
            bool keep_looping = l > 0 && is_digit(s[l - 1]);
            while (keep_looping) // while a digit
            {
                ones_byte = static_cast<Byte>(s[--l] - '0');
                int_to_bf(tmp1, ones_byte);
                unsafe_add_a_bf(r, tmp1);
                keep_looping = l > 0 && is_digit(s[l - 1]);
                if (keep_looping)
                {
                    div_a_bf_int(r, 10);
                    power_ten++; // increase the power of ten
                }
            }
        }
    }
    else
    {
        bool keep_looping = l > 0 && is_digit(s[l - 1]);
        while (keep_looping) // while a digit
        {
            ones_byte = static_cast<Byte>(s[--l] - '0');
            int_to_bf(tmp1, ones_byte);
            unsafe_add_a_bf(r, tmp1);
            keep_looping = l > 0 && is_digit(s[l - 1]);
            if (keep_looping)
            {
                div_a_bf_int(r, 10);
                power_ten++; // increase the power of ten
            }
        }
    }

    if (power_ten > 0)
    {
        for (; power_ten > 0; power_ten--)
        {
            mult_a_bf_int(r, 10);
        }
    }
    else if (power_ten < 0)
    {
        for (; power_ten < 0; power_ten++)
        {
            div_a_bf_int(r, 10);
        }
    }
    if (sign_flag)
    {
        neg_a_bf(r);
    }

    return r;
}

/**********************************************************************/
// strtobn() - converts a string into a bignumer
//   r - pointer to a bignumber
//   s - string in the floating point format [-][digits].[digits]
//   note: the string may not be empty or have extra space and may
//   not use scientific notation (2.3e4).

Byte *str_to_bn(Byte *r, std::string_view s)
{
    bool sign_flag = false;

    clear_bn(r);
    Byte *ones_byte = r + g_bn_length - g_int_length;

    if (s[0] == '+') // for + sign
    {
        s.remove_prefix(1);
    }
    else if (s[0] == '-') // for neg sign
    {
        sign_flag = true;
        s.remove_prefix(1);
    }

    if (s.find('.') != std::string_view::npos)              // is there a decimal point?
    {
        std::size_t l = s.size();                           // start just past the last digit
        while (l > 0 && s[l - 1] >= '0' && s[l - 1] <= '9') // while a digit
        {
            *ones_byte = static_cast<Byte>(s[--l] - '0');
            div_a_bn_int(r, 10);
        }

        if (l > 0 && s[l - 1] == '.')
        {
            set_integer_part(ones_byte, parse_long(s));
        }
    }
    else
    {
        set_integer_part(ones_byte, parse_long(s));
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/********************************************************************/
// bntostr() - converts a bignumber into a string
//   s - string, must be large enough to hold the number.
//   r - bignumber
//   will covert to a floating point notation
//   SIDE-EFFECT: the bignumber, r, is destroyed.
//                Copy it first if necessary.

//  b = l
//  Converts a long to a bignumber
Byte *int_to_bn(Byte *r, const long value)
{
    clear_bn(r);
    Byte *ones_byte = r + g_bn_length - g_int_length;
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(value);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(value));
        break;
    case 4:
        BIG_SET32(ones_byte, value);
        break;
    }
    return r;
}

/*********************************************************************/
//  b = f
//  Converts a double to a bignumber
Byte *float_to_bn(Byte *r, LDouble f)
{
    bool sign_flag = false;

    clear_bn(r);
    Byte *ones_byte = r + g_bn_length - g_int_length;

    if (f < 0)
    {
        sign_flag = true;
        f = -f;
    }

    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(f);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(f));
        break;
    case 4:
        BIG_SET32(ones_byte, static_cast<U32>(f));
        break;
    }

    f -= static_cast<long>(f); // keep only the decimal part
    for (int i = g_bn_length - g_int_length - 1; i >= 0 && f != 0.0; i--)
    {
        f *= 256;
        r[i] = static_cast<Byte>(f); // keep use the integer part
        f -= static_cast<Byte>(f);   // now throw away the integer part
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/*********************************************************************/
//  f = b
//  Converts a bignumber to a double
LDouble bn_to_float(Byte *n)
{
    LDouble f = 0;

    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        sign_flag = true;
        neg_a_bn(n);
    }

    int exponent = g_int_length - 1;
    Byte *get_byte = n + g_bn_length - 1;
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

// big10flt.c - C routines for base 10 big floating point numbers

/**********************************************************
(Just when you thought it was safe to go back in the water.)
Just when you thought you have seen every type of format possible,
16 bit integer, 32 bit integer, double, long double, mpmath,
BigNum, BigFloat, I now give you BigFloat10 (big float base 10)!

Why, because this is the only way (I can think of) to properly do a
bftostr() without rounding errors.  Without this, then
   -1.9999999999( > LDBL_DIG of 9's)9999999123456789...
will round to -2.0.  The good news is that we only need to do two
mathematical operations: multiplication and division by integers

BigFloat10 format: (notice the position of the MSB and LSB)

MSB                                         LSB
  _  _  _  _  _  _  _  _  _  _  _  _ _ _ _ _
n <><------------- dec --------------><> <->
  1 byte pad            1 byte rounding   2 byte exponent.

  total length = dec + 4

***********************************************************/

/**********************************************************************/
// unsafe_bftobf10() - converts a bigfloat into a bigfloat10
//   n - pointer to a bigfloat
//   r - result array of Byte big enough to hold the BigFloat10 number
// dec - number of decimals, not including the one extra for rounding
//  SIDE-EFFECTS: n is changed to |n|.  Make copy of n if necessary.

Byte *unsafe_bf_to_bf10(Byte *r, int dec, Byte *n)
{
    if (is_bf_zero(n))
    {
        // in scientific notation, the leading digit can't be zero
        r[1] = static_cast<Byte>(0); // unless the number is zero
        return r;
    }

    Byte *ones_byte = n + g_bf_length - 1;                                    // really it's n+g_bf_length-2
    const int power256 = static_cast<S16>(BIG_ACCESS16(n + g_bf_length)) + 1; // so adjust power256 by 1

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++; // one extra byte for rounding
    Byte *power10 = r + dec + 1;

    if (is_bf_neg(n))
    {
        neg_a_bf(n);
        r[0] = 1; // sign flag
    }
    else
    {
        r[0] = 0;
    }

    int p = -1; // multiply by 10 right away
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    for (int d = 1; d <= dec; d++)
    {
        // pretend it's a BigNum instead of a BigFloat
        // this leaves n un-normalized, which is what we want here
        mult_a_bn_int(n, 10);

        r[d] = *ones_byte;
        if (d == 1 && r[d] == 0)
        {
            d = 0; // back up a digit
            p--;   // and decrease by a factor of 10
        }
        *ones_byte = 0;
    }
    g_bn_length = bnl;
    BIG_SET16(power10, static_cast<U16>(p)); // save power of ten

    // the digits are all read in, now scale it by 256^power256
    if (power256 > 0)
    {
        for (int d = 0; d < power256; d++)
        {
            mult_a_bf10_int(r, dec, 256);
        }
    }
    else if (power256 < 0)
    {
        for (int d = 0; d > power256; d--)
        {
            div_a_bf10_int(r, dec, 256);
        }
    }

    // else power256 is zero, don't do anything

    // round the last digit
    if (r[dec] >= 5)
    {
        int d = dec - 1;
        while (d > 0)   // stop before you get to the sign flag
        {
            r[d]++;     // round up
            if (r[d] < 10)
            {
                d = -1; // flag for below
                break;  // finished rounding
            }
            r[d] = 0;
            d--;
        }
        if (d == 0) // rounding went back to the first digit and it overflowed
        {
            r[1] = 0;
            std::memmove(r + 2, r + 1, dec - 1);
            r[1] = 1;
            p = static_cast<S16>(BIG_ACCESS16(power10));
            BIG_SET16(power10, static_cast<U16>(p + 1));
        }
    }
    r[dec] = 0; // truncate the rounded digit

    return r;
}

/**********************************************************************/
// mult_a_bf10_int()
// r *= n
// dec - number of decimals, including the one extra for rounding

Byte *mult_a_bf10_int(Byte *r, const int dec, const U16 n)
{
    if (r[1] == 0 || n == 0)
    {
        r[1] = 0;
        return r;
    }

    Byte *power10 = r + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    const int sign_flag = r[0]; // r[0] to be used as a padding
    unsigned overflow = 0;
    for (int d = dec; d > 0; d--)
    {
        const unsigned value = r[d] * n + overflow;
        r[d] = static_cast<Byte>(value % 10);
        overflow = value / 10;
    }
    while (overflow)
    {
        p++;
        std::memmove(r + 2, r + 1, dec - 1);
        r[1] = static_cast<Byte>(overflow % 10);
        overflow = overflow / 10;
    }
    BIG_SET16(power10, static_cast<U16>(p)); // save power of ten
    r[0] = static_cast<Byte>(sign_flag);     // restore sign flag
    return r;
}

/**********************************************************************/
// div_a_bf10_int()
// r /= n
// dec - number of decimals, including the one extra for rounding

Byte *div_a_bf10_int(Byte *r, const int dec, const U16 n)
{
    unsigned value;

    if (r[1] == 0 || n == 0)
    {
        r[1] = 0;
        return r;
    }

    Byte *power10 = r + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    unsigned remainder = 0;
    int dest = 1;
    for (int src = 1; src <= dec; dest++, src++)
    {
        value = 10 * remainder + r[src];
        r[dest] = static_cast<Byte>(value / n);
        remainder = value % n;
        if (dest == 1 && r[dest] == 0)
        {
            dest = 0; // back up a digit
            p--;      // and decrease by a factor of 10
        }
    }
    for (; dest <= dec; dest++)
    {
        value = 10 * remainder;
        r[dest] = static_cast<Byte>(value / n);
        remainder = value % n;
        if (dest == 1 && r[dest] == 0)
        {
            dest = 0; // back up a digit
            p--;      // and decrease by a factor of 10
        }
    }

    BIG_SET16(power10, static_cast<U16>(p)); // save power of ten
    return r;
}

/*************************************************************************/
// bf10tostring_e()
// Takes a bf10 number and converts it to an ascii string, sci. notation
// dec - number of decimals, not including the one extra for rounding

std::string bf10_to_string_e(const Byte *n, int dec)
{
    if (n[1] == 0)
    {
        return "0.0";
    }

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++; // one extra byte for rounding
    const Byte *power10 = n + dec + 1;
    const int p = static_cast<S16>(BIG_ACCESS16(power10));

    // if p is negative, it is not necessary to show all the decimal places
    if (p < 0 && dec > 8)       // 8 sounds like a reasonable value
    {
        dec = dec + p;
        dec = std::max(dec, 8); // let's keep at least a few
    }

    std::string text;
    if (n[0] == 1) // sign flag
    {
        text.push_back('-');
    }
    text.push_back(static_cast<char>(n[1] + '0'));
    text.push_back('.');
    for (int d = 2; d <= dec; d++)
    {
        text.push_back(static_cast<char>(n[d] + '0'));
    }
    // clean up trailing 0's
    while (text.back() == '0')
    {
        text.pop_back();
    }
    if (text.back() == '.') // put at least one 0 after the decimal
    {
        text.push_back('0');
    }
    text.push_back('e');
    text += std::to_string(p);
    return text;
}

/****************************************************************************/
// bf10tostring_f()
// Takes a bf10 number and converts it to an ascii string, decimal notation

std::string bf10_to_string_f(const Byte *n, int dec)
{
    if (n[1] == 0)
    {
        return "0.0";
    }

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++; // one extra byte for rounding
    const Byte *power10 = n + dec + 1;
    const int p = static_cast<S16>(BIG_ACCESS16(power10));

    // if p is negative, it is not necessary to show all the decimal places
    if (p < 0 && dec > 8)       // 8 sounds like a reasonable value
    {
        dec = dec + p;
        dec = std::max(dec, 8); // let's keep at least a few
    }

    std::string text;
    if (n[0] == 1) // sign flag
    {
        text.push_back('-');
    }
    if (p >= 0)
    {
        int d;
        for (d = 1; d <= p + 1; d++)
        {
            text.push_back(d < dec ? static_cast<char>(n[d] + '0') : '0');
        }
        text.push_back('.');
        for (; d <= dec; d++)
        {
            text.push_back(static_cast<char>(n[d] + '0'));
        }
    }
    else
    {
        text.push_back('0');
        text.push_back('.');
        for (int d = 0; d > p + 1; d--)
        {
            text.push_back('0');
        }
        for (int d = 1; d <= dec; d++)
        {
            text.push_back(static_cast<char>(n[d] + '0'));
        }
    }

    // clean up trailing 0's
    while (text.back() == '0')
    {
        text.pop_back();
    }
    if (text.back() == '.') // put at least one 0 after the decimal
    {
        text.push_back('0');
    }
    return text;
}

} // namespace id::math
