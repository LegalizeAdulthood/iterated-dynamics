// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// C routines for big floating point numbers

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include <math/big.h>
#include <math/BigFloat.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace id::misc;

namespace id::math
{

/********************************************************************/
// bf_hexdump() - for debugging, dumps to stdout

void bf_hex_dump(const BigFloat &r)
{
    for (int i = 0; i < g_bf_length; i++)
    {
        std::printf("%02X ", *(r.data() + i));
    }
    std::printf(" e %04hX\n", static_cast<S16>(BIG_ACCESS16(r.data() + g_bf_length)));
}

/********************************************************************/
// r = 1/n
// uses g_bf_tmp1 - g_bf_tmp2 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n|/256^exp    Make copy first if necessary.
BigFloatPtr unsafe_inv_bf(BigFloatPtr r, BigFloatPtr n)
{
    bool sign_flag = false;

    // use Newton's recursive method for zeroing in on 1/n : r=r(2-rn)

    if (is_bf_neg(n))
    {
        // will be a lot easier to deal with just positives
        sign_flag = true;
        neg_a_bf(n);
    }

    const int f_exp = static_cast<S16>(BIG_ACCESS16(n + g_bf_length));
    BIG_SET16(n+g_bf_length, static_cast<S16>(0)); // put within LDouble range

    LDouble f = bf_to_float(n);
    if (f == 0) // division by zero
    {
        max_bf(r);
        return r;
    }
    f = 1/f; // approximate inverse

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    // orig_bftmp1 not needed here
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloatPtr orig_r = r;
    BigFloatPtr orig_n = n;
    // orig_bftmp1        = g_bf_tmp1;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>(LDBL_DIG / LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;
    // g_bf_tmp1 = orig_bftmp1 + orig_bf_length - g_bf_length;

    float_to_bf(r, f); // start with approximate inverse

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;
        // g_bf_tmp1 = orig_bftmp1 + orig_bf_length - g_bf_length;

        unsafe_mult_bf(g_bf_tmp1.data(), r, n); // g_bf_tmp1=rn
        int_to_bf(g_bf_tmp2.data(), 1);         // will be used as 1.0

        // There seems to very little difficulty getting g_bf_tmp1 to be EXACTLY 1
        if (g_bf_length == orig_bf_length && cmp_bf(g_bf_tmp1.data(), g_bf_tmp2.data()) == 0)
        {
            break;
        }

        int_to_bf(g_bf_tmp2.data(), 2);                        // will be used as 2.0
        unsafe_sub_a_bf(g_bf_tmp2.data(), g_bf_tmp1.data());   // g_bf_tmp2=2-rn
        unsafe_mult_bf(g_bf_tmp1.data(), r, g_bf_tmp2.data()); // g_bf_tmp1=r(2-rn)
        copy_bf(r, g_bf_tmp1.data());                          // r = g_bf_tmp1
    }

    // restore original values
    g_bf_length      = orig_bf_length;
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    r             = orig_r;
    // g_bf_tmp1        = orig_bftmp1;

    if (sign_flag)
    {
        neg_a_bf(r);
    }
    int r_exp = static_cast<S16>(BIG_ACCESS16(r + g_bf_length));
    r_exp -= f_exp;
    BIG_SET16(r+g_bf_length, static_cast<S16>(r_exp)); // adjust result exponent
    return r;
}






/**********************************************************************/
// The rest of the functions are "safe" versions of the routines that
// have side effects which alter the parameters.
// Most bf routines change values of parameters, not just the sign.
/**********************************************************************/


/**********************************************************************/
BigFloatPtr square_bf(BigFloatPtr r, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    unsafe_square_bf(r, g_bf_tmp_copy1.data());
    return r;
}

/**********************************************************************/
BigFloatPtr mult_bf_int(BigFloatPtr r, BigFloatConstPtr n, const U16 u)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    unsafe_mult_bf_int(r, g_bf_tmp_copy1.data(), u);
    return r;
}

/**********************************************************************/
BigFloatPtr ln_bf(BigFloatPtr r, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    unsafe_ln_bf(r, g_bf_tmp_copy1.data());
    return r;
}

/**********************************************************************/
BigFloatPtr sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    return unsafe_sin_cos_bf(s, c, g_bf_tmp_copy1.data());
}

/**********************************************************************/
BigFloatPtr atan2_bf(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx)
{
    copy_bf(g_bf_tmp_copy1.data(), ny);
    copy_bf(g_bf_tmp_copy2.data(), nx);
    unsafe_atan2_bf(r, g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
    return r;
}

/********************************************************************/
// r = 2*n
BigFloatPtr double_bf(BigFloatPtr r, BigFloatPtr n)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    double_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r *= 2
BigFloatPtr double_a_bf(BigFloatPtr r)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    double_a_bn(r);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r = n/2
BigFloatPtr half_bf(BigFloatPtr r, BigFloatPtr n)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    half_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}


/********************************************************************/
// r = n * u  where u is an unsigned integer
// SIDE-EFFECTS: n can be "de-normalized" and lose precision
BigFloatPtr unsafe_mult_bf_int(BigFloatPtr r, BigFloatPtr n, const U16 u)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    const bool positive = !is_bf_neg(n);

    /*
    if u > 0x00FF, then the integer part of the mantissa will overflow the
    2 byte (16 bit) integer size.  Therefore, make adjustment before
    multiplication is performed.
    */
    if (u > 0x00FF)
    {
        // un-normalize n
        std::memmove(n, n+1, g_bf_length-1);  // this sign extends as well
        BIG_SET_S16(r_exp, BIG_ACCESS_S16(r_exp) + static_cast<S16>(1));
    }

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    mult_bn_int(r, n, u);
    g_bn_length = bnl;

    norm_sign_bf(r, positive);
    return r;
}

/********************************************************************/
// r *= u  where u is an unsigned integer
BigFloatPtr mult_a_bf_int(BigFloatPtr r, const U16 u)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    const bool positive = !is_bf_neg(r);

    /*
    if u > 0x00FF, then the integer part of the mantissa will overflow the
    2 byte (16 bit) integer size.  Therefore, make adjustment before
    multiplication is performed.
    */
    if (u > 0x00FF)
    {
        // un-normalize n
        std::memmove(r, r+1, g_bf_length-1);  // this sign extends as well
        BIG_SET_S16(r_exp, BIG_ACCESS_S16(r_exp) + static_cast<S16>(1));
    }

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    mult_a_bn_int(r, u);
    g_bn_length = bnl;

    norm_sign_bf(r, positive);
    return r;
}

/********************************************************************/
// r = n / u  where u is an unsigned integer
BigFloatPtr unsafe_div_bf_int(BigFloatPtr r, BigFloatPtr n, const U16 u)
{
    if (u == 0) // division by zero
    {
        max_bf(r);
        if (is_bf_neg(n))
        {
            neg_a_bf(r);
        }
        return r;
    }

    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    unsafe_div_bn_int(r, n, u);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r /= u  where u is an unsigned integer
BigFloatPtr div_a_bf_int(BigFloatPtr r, const U16 u)
{
    if (u == 0) // division by zero
    {
        if (is_bf_neg(r))
        {
            max_bf(r);
            neg_a_bf(r);
        }
        else
        {
            max_bf(r);
        }
        return r;
    }

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    div_a_bn_int(r, u);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// extracts the mantissa and exponent of f
// finds m and n such that 1<=|m|<10 and f = m*10^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract_10(LDouble f, int *exp_ptr)
{
    return extract_value(f, 10, exp_ptr);
}

/********************************************************************/
// calculates and returns the value of f*10^n
// sort of like ldexp()
LDouble scale_10(LDouble f, const int n)
{
    return scale_value(f, 10, n);
}

} // namespace id::math
