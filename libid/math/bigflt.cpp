// SPDX-License-Identifier: GPL-3.0-only
//
// C routines for big floating point numbers

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include "math/big.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace id::misc;

namespace id::math
{

/********************************************************************/
// bf_hexdump() - for debugging, dumps to stdout

void bf_hex_dump(BigFloat r)
{
    for (int i = 0; i < g_bf_length; i++)
    {
        std::printf("%02X ", *(r+i));
    }
    std::printf(" e %04hX\n", static_cast<S16>(BIG_ACCESS16(r + g_bf_length)));
}

/**********************************************************************/
// strtobf() - converts a string into a bigfloat
//   r - pointer to a bigfloat
//   s - string in the floating point format [+-][dig].[dig]e[+-][dig]
//   note: the string may not be empty or have extra space.
//         It may use scientific notation.
// USES: g_bf_tmp1

BigFloat str_to_bf(BigFloat r, const char *s)
{
    Byte ones_byte;
    bool sign_flag = false;
    const char *l;
    int power_ten = 0;

    clear_bf(r);

    if (s[0] == '+')    // for + sign
    {
        s++;
    }
    else if (s[0] == '-')    // for neg sign
    {
        sign_flag = true;
        s++;
    }

    const char *d = std::strchr(s, '.');
    const char *e = std::strchr(s, 'e');
    if (e == nullptr)
    {
        e = std::strchr(s, 'E');
    }
    if (e != nullptr)
    {
        power_ten = std::atoi(e+1);    // read in the e (x10^) part
        l = e - 1; // just before e
    }
    else
    {
        l = s + std::strlen(s) - 1;  // last digit
    }

    if (d != nullptr) // is there a decimal point?
    {
        while (*l >= '0' && *l <= '9') // while a digit
        {
            ones_byte = static_cast<Byte>(*l-- - '0');
            int_to_bf(g_bf_tmp1, ones_byte);
            unsafe_add_a_bf(r, g_bf_tmp1);
            div_a_bf_int(r, 10);
        }

        if (*l-- == '.') // the digit was found
        {
            bool keep_looping = *l >= '0' && *l <= '9' && l >= s;
            while (keep_looping) // while a digit
            {
                ones_byte = static_cast<Byte>(*l-- - '0');
                int_to_bf(g_bf_tmp1, ones_byte);
                unsafe_add_a_bf(r, g_bf_tmp1);
                keep_looping = *l >= '0' && *l <= '9' && l >= s;
                if (keep_looping)
                {
                    div_a_bf_int(r, 10);
                    power_ten++;    // increase the power of ten
                }
            }
        }
    }
    else
    {
        bool keep_looping = *l >= '0' && *l <= '9' && l >= s;
        while (keep_looping) // while a digit
        {
            ones_byte = static_cast<Byte>(*l-- - '0');
            int_to_bf(g_bf_tmp1, ones_byte);
            unsafe_add_a_bf(r, g_bf_tmp1);
            keep_looping = *l >= '0' && *l <= '9' && l >= s;
            if (keep_looping)
            {
                div_a_bf_int(r, 10);
                power_ten++;    // increase the power of ten
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

/********************************************************************/
// std::strlen_needed_bf() - returns string length needed to hold bigfloat

int strlen_needed_bf()
{
    // first space for integer part
    int length = 1;
    length += g_decimals;  // decimal part
    length += 2;         // decimal point and sign
    length += 2;         // e and sign
    length += 4;         // exponent
    length += 4;         // null and a little extra for safety
    return length;
}

/********************************************************************/
// bftostr() - converts a bigfloat into a scientific notation string
//   s - string, must be large enough to hold the number.
// dec - decimal places, 0 for max
//   r - bigfloat
//   will convert to a floating point notation
//   SIDE-EFFECT: the bigfloat, r, is destroyed.
//                Copy it first if necessary.
// USES: g_bf_tmp1 - g_bf_tmp2
/********************************************************************/

char *unsafe_bf_to_str(char *s, BigFloat r, int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        std::strcpy(s, "0.0");
        return s;
    }

    copy_bf(g_bf_tmp1, r);
    unsafe_bf_to_bf10(g_bf10_tmp, dec, g_bf_tmp1);
    int power = static_cast<S16>(BIG_ACCESS16(g_bf10_tmp + dec + 2)); // where the exponent is stored
    if (power > -4 && power < 6)   // tinker with this
    {
        bf10_to_str_f(s, g_bf10_tmp, dec);
    }
    else
    {
        bf10_to_str_e(s, g_bf10_tmp, dec);
    }
    return s;
}

/********************************************************************/
// the e version puts it in scientific notation, (like printf's %e)
char *unsafe_bf_to_str_e(char *s, BigFloat r, int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        std::strcpy(s, "0.0");
        return s;
    }

    copy_bf(g_bf_tmp1, r);
    unsafe_bf_to_bf10(g_bf10_tmp, dec, g_bf_tmp1);
    bf10_to_str_e(s, g_bf10_tmp, dec);
    return s;
}

/********************************************************************/
// the f version puts it in decimal notation, (like printf's %f)
char *unsafe_bf_to_str_f(char *s, BigFloat r, int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        std::strcpy(s, "0.0");
        return s;
    }

    copy_bf(g_bf_tmp1, r);
    unsafe_bf_to_bf10(g_bf10_tmp, dec, g_bf_tmp1);
    bf10_to_str_f(s, g_bf10_tmp, dec);
    return s;
}

/*********************************************************************/
//  bn = floor(bf)
//  Converts a bigfloat to a bignumber (integer)
//  g_bf_length must be at least g_bn_length+2
BigNum bf_to_bn(BigNum n, BigFloat f)
{
    int f_exp = static_cast<S16>(BIG_ACCESS16(f + g_bf_length));
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
    int move_bytes = g_bn_length - g_int_length + f_exp + 1;
    std::memcpy(n, f+g_bf_length-move_bytes-1, move_bytes);
    Byte hi_byte = *(f + g_bf_length - 1);
    std::memset(n+move_bytes, hi_byte, g_bn_length-move_bytes); // sign extends
    return n;
}

/*********************************************************************/
//  bf = bn
//  Converts a bignumber (integer) to a bigfloat
//  g_bf_length must be at least g_bn_length+2
BigFloat bn_to_bf(BigFloat f, BigNum n)
{
    std::memcpy(f+g_bf_length-g_bn_length-1, n, g_bn_length);
    std::memset(f, 0, g_bf_length - g_bn_length - 1);
    *(f+g_bf_length-1) = static_cast<Byte>(is_bn_neg(n) ? 0xFF : 0x00); // sign extend
    BIG_SET16(f+g_bf_length, static_cast<S16>(g_int_length - 1)); // exp
    norm_bf(f);
    return f;
}

/*********************************************************************/
//  b = l
//  Converts a long to a bigfloat
BigFloat int_to_bf(BigFloat r, long value)
{
    clear_bf(r);
    BIG_SET32(r+g_bf_length-4, static_cast<S32>(value));
    BIG_SET16(r+g_bf_length, static_cast<S16>(2));
    norm_bf(r);
    return r;
}

/*********************************************************************/
//  l = floor(b), floor rounds down
//  Converts a bigfloat to a long
//  note: a bf value of 2.999... will be return a value of 2, not 3
long bf_to_int(BigFloat f)
{
    long result;

    int f_exp = static_cast<S16>(BIG_ACCESS16(f + g_bf_length));
    if (f_exp > 3)
    {
        result = 0x7FFFFFFFL;
        if (is_bf_neg(f))
        {
            result = -result;
        }
        return result;
    }
    result = BIG_ACCESS32(f+g_bf_length-5);
    result >>= 8*(3-f_exp);
    return result;
}

/********************************************************************/
// sign(r)
int sign_bf(BigFloat n)
{
    return is_bf_neg(n) ? -1 : is_bf_not_zero(n) ? 1 : 0;
}

/********************************************************************/
// r = |n|
BigFloat abs_bf(BigFloat r, BigFloat n)
{
    copy_bf(r, n);
    if (is_bf_neg(r))
    {
        neg_a_bf(r);
    }
    return r;
}

/********************************************************************/
// r = |r|
BigFloat abs_a_bf(BigFloat r)
{
    if (is_bf_neg(r))
    {
        neg_a_bf(r);
    }
    return r;
}

/********************************************************************/
// r = 1/n
// uses g_bf_tmp1 - g_bf_tmp2 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n|/256^exp    Make copy first if necessary.
BigFloat unsafe_inv_bf(BigFloat r, BigFloat n)
{
    bool sign_flag = false;

    // use Newton's recursive method for zeroing in on 1/n : r=r(2-rn)

    if (is_bf_neg(n))
    {
        // will be a lot easier to deal with just positives
        sign_flag = true;
        neg_a_bf(n);
    }

    int f_exp = static_cast<S16>(BIG_ACCESS16(n + g_bf_length));
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
    BigFloat orig_r = r;
    BigFloat orig_n = n;
    // orig_bftmp1        = g_bf_tmp1;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>((LDBL_DIG / LOG10_256)) + 1; // round up
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

        unsafe_mult_bf(g_bf_tmp1, r, n); // g_bf_tmp1=rn
        int_to_bf(g_bf_tmp2, 1); // will be used as 1.0

        // There seems to very little difficulty getting g_bf_tmp1 to be EXACTLY 1
        if (g_bf_length == orig_bf_length && cmp_bf(g_bf_tmp1, g_bf_tmp2) == 0)
        {
            break;
        }

        int_to_bf(g_bf_tmp2, 2); // will be used as 2.0
        unsafe_sub_a_bf(g_bf_tmp2, g_bf_tmp1); // g_bf_tmp2=2-rn
        unsafe_mult_bf(g_bf_tmp1, r, g_bf_tmp2); // g_bf_tmp1=r(2-rn)
        copy_bf(r, g_bf_tmp1); // r = g_bf_tmp1
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

/********************************************************************/
// r = n1/n2
//      r - result of length g_bf_length
// uses g_bf_tmp1 - g_bf_tmp2 - global temp bigfloats
//  SIDE-EFFECTS:
//      n1, n2 end up as |n1|/256^x, |n2|/256^x
//      Make copies first if necessary.
BigFloat unsafe_div_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    // first, check for valid data

    int a_exp = static_cast<S16>(BIG_ACCESS16(n1 + g_bf_length));
    BIG_SET16(n1+g_bf_length, static_cast<S16>(0)); // put within LDouble range

    LDouble a = bf_to_float(n1);
    if (a == 0) // division into zero
    {
        clear_bf(r); // return 0
        return r;
    }

    int b_exp = static_cast<S16>(BIG_ACCESS16(n2 + g_bf_length));
    BIG_SET16(n2+g_bf_length, static_cast<S16>(0)); // put within LDouble range

    LDouble b = bf_to_float(n2);
    if (b == 0) // division by zero
    {
        max_bf(r);
        return r;
    }

    unsafe_inv_bf(r, n2);
    unsafe_mult_bf(g_bf_tmp1, n1, r);
    copy_bf(r, g_bf_tmp1); // r = g_bf_tmp1

    int r_exp = static_cast<S16>(BIG_ACCESS16(r + g_bf_length));
    r_exp += a_exp - b_exp;
    BIG_SET16(r+g_bf_length, static_cast<S16>(r_exp)); // adjust result exponent

    return r;
}

/********************************************************************/
// sqrt(r)
// uses g_bf_tmp1 - g_bf_tmp3 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n|
BigFloat unsafe_sqrt_bf(BigFloat r, BigFloat n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on sqrt(n): r=.5(r+n/r)

    if (is_bf_neg(n))
    {
        // sqrt of a neg, return 0
        clear_bf(r);
        return r;
    }

    LDouble f = bf_to_float(n);
    if (f == 0) // division by zero will occur
    {
        clear_bf(r); // sqrt(0) = 0
        return r;
    }
    f = std::sqrt(f); // approximate square root
    // no need to check overflow

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloat orig_r = r;
    BigFloat orig_n = n;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>((LDBL_DIG / LOG10_256)) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;

    float_to_bf(r, f); // start with approximate sqrt

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;

        unsafe_div_bf(g_bf_tmp3, n, r);
        unsafe_add_a_bf(r, g_bf_tmp3);
        half_a_bf(r);
        if (g_bf_length == orig_bf_length)
        {
            const int comp = std::abs(cmp_bf(r, g_bf_tmp3));  // if match or almost match
            if (comp < 8)
            {
                if (comp < 4  // perfect or near perfect match
                    || almost_match == 1)   // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
        }
    }

    // restore original values
    g_bf_length      = orig_bf_length;
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    r             = orig_r;

    return r;
}

/********************************************************************/
// exp(r)
// uses g_bf_tmp1, g_bf_tmp2, g_bf_tmp3 - global temp bigfloats
BigFloat exp_bf(BigFloat r, BigFloat n)
{
    U16 fact = 1;
    S16 *test_exp = reinterpret_cast<S16 *>(g_bf_tmp2 + g_bf_length);
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);

    if (is_bf_zero(n))
    {
        int_to_bf(r, 1);
        return r;
    }

    // use Taylor Series (very slow convergence)
    int_to_bf(r, 1); // start with r=1.0
    copy_bf(g_bf_tmp2, r);
    while (true)
    {
        copy_bf(g_bf_tmp1, n);
        unsafe_mult_bf(g_bf_tmp3, g_bf_tmp2, g_bf_tmp1);
        unsafe_div_bf_int(g_bf_tmp2, g_bf_tmp3, fact);
        if (BIG_ACCESS_S16(test_exp) < BIG_ACCESS_S16(r_exp)-(g_bf_length-2))
        {
            break; // too small to register
        }
        unsafe_add_a_bf(r, g_bf_tmp2);
        fact++;
    }

    return r;
}

/********************************************************************/
// ln(r)
// uses g_bf_tmp1 - g_bf_tmp6 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n|
BigFloat unsafe_ln_bf(BigFloat r, BigFloat n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1

    if (is_bf_neg(n) || is_bf_zero(n))
    {
        // error, return largest neg value
        max_bf(r);
        neg_a_bf(r);
        return r;
    }

    LDouble f = bf_to_float(n);
    f = std::log(f); // approximate ln(x)
    // no need to check overflow
    // appears to be ok, do ln

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloat orig_r = r;
    BigFloat orig_n = n;
    BigFloat orig_bf_tmp5 = g_bf_tmp5;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>((LDBL_DIG / LOG10_256)) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;
    g_bf_tmp5 = orig_bf_tmp5 + orig_bf_length - g_bf_length;

    float_to_bf(r, f); // start with approximate ln
    neg_a_bf(r); // -r
    copy_bf(g_bf_tmp5, r); // -r

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;
        g_bf_tmp5 = orig_bf_tmp5 + orig_bf_length - g_bf_length;

        exp_bf(g_bf_tmp6, r);     // exp(-r)
        unsafe_mult_bf(g_bf_tmp2, g_bf_tmp6, n);  // n*exp(-r)
        int_to_bf(g_bf_tmp4, 1);
        unsafe_sub_a_bf(g_bf_tmp2, g_bf_tmp4);   // n*exp(-r) - 1
        unsafe_sub_a_bf(r, g_bf_tmp2);        // -r - (n*exp(-r) - 1)
        if (g_bf_length == orig_bf_length)
        {
            const int comp = std::abs(cmp_bf(r, g_bf_tmp5));
            if(comp < 8)  // if match or almost match
            {
                if (comp < 4  // perfect or near perfect match
                    || almost_match == 1)   // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
        }
        copy_bf(g_bf_tmp5, r); // -r
    }

    // restore original values
    g_bf_length      = orig_bf_length;
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    r             = orig_r;
    g_bf_tmp5        = orig_bf_tmp5;

    neg_a_bf(r); // -(-r)
    return r;
}

/********************************************************************/
// sincos_bf(r)
// uses g_bf_tmp1 - g_bf_tmp2 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n| mod (pi/4)
BigFloat unsafe_sin_cos_bf(BigFloat s, BigFloat c, BigFloat n)
{
    U16 fact = 2;
    S16 *test_exp = reinterpret_cast<S16 *>(g_bf_tmp1 + g_bf_length);
    S16 *c_exp = reinterpret_cast<S16 *>(c + g_bf_length);
    S16 *s_exp = reinterpret_cast<S16 *>(s + g_bf_length);

#ifndef CALCULATING_BIG_PI
    // assure range 0 <= x < pi/4

    if (is_bf_zero(n))
    {
        clear_bf(s);    // sin(0) = 0
        int_to_bf(c, 1);  // cos(0) = 1
        return s;
    }

    bool sign_sin = false;
    if (is_bf_neg(n))
    {
        sign_sin = !sign_sin; // sin(-x) = -sin(x), odd; cos(-x) = cos(x), even
        neg_a_bf(n);
    }
    // n >= 0

    double_bf(g_bf_tmp1, g_bf_pi); // 2*pi
    // this could be done with remainders, but it would probably be slower
    while (cmp_bf(n, g_bf_tmp1) >= 0) // while n >= 2*pi
    {
        copy_bf(g_bf_tmp2, g_bf_tmp1);
        unsafe_sub_a_bf(n, g_bf_tmp2);
    }
    // 0 <= n < 2*pi

    bool sign_cos = false;
    copy_bf(g_bf_tmp1, g_bf_pi); // pi
    if (cmp_bf(n, g_bf_tmp1) >= 0) // if n >= pi
    {
        unsafe_sub_a_bf(n, g_bf_tmp1);
        sign_sin = !sign_sin;
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi

    half_bf(g_bf_tmp1, g_bf_pi); // pi/2
    if (cmp_bf(n, g_bf_tmp1) > 0) // if n > pi/2
    {
        copy_bf(g_bf_tmp2, g_bf_pi);
        unsafe_sub_bf(n, g_bf_tmp2, n);
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi/2

    bool switch_sin_cos = false;
    half_bf(g_bf_tmp1, g_bf_pi); // pi/2
    half_a_bf(g_bf_tmp1);      // pi/4
    if (cmp_bf(n, g_bf_tmp1) > 0) // if n > pi/4
    {
        copy_bf(g_bf_tmp2, n);
        half_bf(g_bf_tmp1, g_bf_pi); // pi/2
        unsafe_sub_bf(n, g_bf_tmp1, g_bf_tmp2);  // pi/2 - n
        switch_sin_cos = !switch_sin_cos;
    }
    // 0 <= n < pi/4

    // this looks redundant, but n could now be zero when it wasn't before
    if (is_bf_zero(n))
    {
        clear_bf(s);    // sin(0) = 0
        int_to_bf(c, 1);  // cos(0) = 1
        return s;
    }

    // at this point, the double angle trig identities could be used as many
    // times as desired to reduce the range to pi/8, pi/16, etc...  Each time
    // the range is cut in half, the number of iterations required is reduced
    // by "quite a bit."  It's just a matter of testing to see what gives the
    // optimal results.
    // halves = g_bf_length / 10; */ /* this is experimental
    int halves = 1;
    for (int i = 0; i < halves; i++)
    {
        half_a_bf(n);
    }
#endif

    // use Taylor Series (very slow convergence)
    copy_bf(s, n); // start with s=n
    int_to_bf(c, 1); // start with c=1
    copy_bf(g_bf_tmp1, n); // the current x^n/n!
    bool sin_done = false;
    bool cos_done = false;
    bool k = false;
    do
    {
        // even terms for cosine
        copy_bf(g_bf_tmp2, g_bf_tmp1);
        unsafe_mult_bf(g_bf_tmp1, g_bf_tmp2, n);
        div_a_bf_int(g_bf_tmp1, fact++);
        if (!cos_done)
        {
            cos_done =
                BIG_ACCESS_S16(test_exp) < BIG_ACCESS_S16(c_exp) - (g_bf_length - 2); // too small to register
            if (!cos_done)
            {
                if (k)   // alternate between adding and subtracting
                {
                    unsafe_add_a_bf(c, g_bf_tmp1);
                }
                else
                {
                    unsafe_sub_a_bf(c, g_bf_tmp1);
                }
            }
        }

        // odd terms for sine
        copy_bf(g_bf_tmp2, g_bf_tmp1);
        unsafe_mult_bf(g_bf_tmp1, g_bf_tmp2, n);
        div_a_bf_int(g_bf_tmp1, fact++);
        if (!sin_done)
        {
            sin_done =
                BIG_ACCESS_S16(test_exp) < BIG_ACCESS_S16(s_exp) - (g_bf_length - 2); // too small to register
            if (!sin_done)
            {
                if (k)   // alternate between adding and subtracting
                {
                    unsafe_add_a_bf(s, g_bf_tmp1);
                }
                else
                {
                    unsafe_sub_a_bf(s, g_bf_tmp1);
                }
            }
        }
        k = !k; // toggle
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        std::printf("."); // lets you know it's doing something
#endif
    }
    while (!cos_done || !sin_done);

#ifndef CALCULATING_BIG_PI
    // now need to undo what was done by cutting angles in half
    for (int i = 0; i < halves; i++)
    {
        unsafe_mult_bf(g_bf_tmp2, s, c); // no need for safe mult
        double_bf(s, g_bf_tmp2); // sin(2x) = 2*sin(x)*cos(x)
        unsafe_square_bf(g_bf_tmp2, c);
        double_a_bf(g_bf_tmp2);
        int_to_bf(g_bf_tmp1, 1);
        unsafe_sub_bf(c, g_bf_tmp2, g_bf_tmp1); // cos(2x) = 2*cos(x)*cos(x) - 1
    }

    if (switch_sin_cos)
    {
        copy_bf(g_bf_tmp1, s);
        copy_bf(s, c);
        copy_bf(c, g_bf_tmp1);
    }
    if (sign_sin)
    {
        neg_a_bf(s);
    }
    if (sign_cos)
    {
        neg_a_bf(c);
    }
#endif

    return s; // return sine I guess
}

/********************************************************************/
// atan(r)
// uses g_bf_tmp1 - g_bf_tmp5 - global temp bigfloats
//  SIDE-EFFECTS:
//      n ends up as |n| or 1/|n|
BigFloat unsafe_atan_bf(BigFloat r, BigFloat n)
{
    int almost_match = 0;
    bool sign_flag = false;

    // use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r))

    if (is_bf_neg(n))
    {
        sign_flag = true;
        neg_a_bf(n);
    }

    // If n is very large, atanl() won't give enough decimal places to be a
    // good enough initial guess for Newton's Method.  If it is larger than
    // say, 1, atan(n) = pi/2 - acot(n) = pi/2 - atan(1/n).

    LDouble f = bf_to_float(n);
    bool large_arg = f > 1.0;
    if (large_arg)
    {
        unsafe_inv_bf(g_bf_tmp3, n);
        copy_bf(n, g_bf_tmp3);
        f = bf_to_float(n);
    }

    clear_bf(g_bf_tmp3); // not really necessary, but makes things more consistent

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bf_length = g_bf_length;
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    int orig_r_bf_length = g_r_bf_length;
    BigFloat orig_bf_pi = g_bf_pi;
    BigFloat orig_r = r;
    BigFloat orig_n = n;
    BigFloat orig_bf_tmp3 = g_bf_tmp3;

    // calculate new starting values
    g_bn_length = g_int_length + static_cast<int>((LDBL_DIG / LOG10_256)) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bf_length - g_bf_length;
    g_bf_pi = orig_bf_pi + orig_bf_length - g_bf_length;
    g_bf_tmp3 = orig_bf_tmp3 + orig_bf_length - g_bf_length;

    f = atanl(f); // approximate arctangent
    // no need to check overflow

    float_to_bf(r, f); // start with approximate atan
    copy_bf(g_bf_tmp3, r);

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bf_length - g_bf_length;
        n = orig_n + orig_bf_length - g_bf_length;
        g_bf_pi = orig_bf_pi + orig_bf_length - g_bf_length;
        g_bf_tmp3 = orig_bf_tmp3 + orig_bf_length - g_bf_length;

#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        std::printf("\natan() loop #%i, g_bf_length=%i\nsincos() loops\n", i, g_bf_length);
#endif
        unsafe_sin_cos_bf(g_bf_tmp4, g_bf_tmp5, g_bf_tmp3);   // sin(r), cos(r)
        copy_bf(g_bf_tmp3, r); // restore g_bf_tmp3 from sincos_bf()
        copy_bf(g_bf_tmp1, g_bf_tmp5);
        unsafe_mult_bf(g_bf_tmp2, n, g_bf_tmp1);     // n*cos(r)
        unsafe_sub_a_bf(g_bf_tmp4, g_bf_tmp2); // sin(r) - n*cos(r)
        unsafe_mult_bf(g_bf_tmp1, g_bf_tmp5, g_bf_tmp4); // cos(r) * (sin(r) - n*cos(r))
        copy_bf(g_bf_tmp3, r);
        unsafe_sub_a_bf(r, g_bf_tmp1); // r - cos(r) * (sin(r) - n*cos(r))
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
        putchar('\n');
        bf_hexdump(r);
#endif
        if (g_bf_length == orig_bf_length)
        {
            const int comp = std::abs(cmp_bf(r, g_bf_tmp3));
            if (comp < 8)  // if match or almost match
            {
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
                std::printf("atan() loop comp=%i\n", comp);
#endif
                if (comp < 4  // perfect or near perfect match
                    || almost_match == 1)   // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
            else
            {
                std::printf("atan() loop comp=%i\n", comp);
            }
#endif
        }

        copy_bf(g_bf_tmp3, r); // make a copy for later comparison
    }

    // restore original values
    g_bf_length      = orig_bf_length;
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_r_bf_length = orig_r_bf_length;
    g_bf_pi         = orig_bf_pi;
    r             = orig_r;
    g_bf_tmp3        = orig_bf_tmp3;

    if (large_arg)
    {
        half_bf(g_bf_tmp3, g_bf_pi);  // pi/2
        sub_a_bf(g_bf_tmp3, r);     // pi/2 - atan(1/n)
        copy_bf(r, g_bf_tmp3);
    }

    if (sign_flag)
    {
        neg_a_bf(r);
    }
    return r;
}

/********************************************************************/
// atan2(r, ny, nx)
// uses g_bf_tmp1 - g_bf_tmp6 - global temp bigfloats
BigFloat unsafe_atan2_bf(BigFloat r, BigFloat ny, BigFloat nx)
{
    int sign_x = sign_bf(nx);
    int sign_y = sign_bf(ny);

    if (sign_y == 0)
    {
        if (sign_x < 0)
        {
            copy_bf(r, g_bf_pi); // negative x axis, 180 deg
        }
        else        // sign_x >= 0    positive x axis, 0
        {
            clear_bf(r);
        }
        return r;
    }
    if (sign_x == 0)
    {
        copy_bf(r, g_bf_pi); // y axis
        half_a_bf(r);      // +90 deg
        if (sign_y < 0)
        {
            neg_a_bf(r);    // -90 deg
        }
        return r;
    }

    if (sign_y < 0)
    {
        neg_a_bf(ny);
    }
    if (sign_x < 0)
    {
        neg_a_bf(nx);
    }
    unsafe_div_bf(g_bf_tmp6, ny, nx);
    unsafe_atan_bf(r, g_bf_tmp6);
    if (sign_x < 0)
    {
        sub_bf(r, g_bf_pi, r);
    }
    if (sign_y < 0)
    {
        neg_a_bf(r);
    }
    return r;
}

/**********************************************************************/
// The rest of the functions are "safe" versions of the routines that
// have side effects which alter the parameters.
// Most bf routines change values of parameters, not just the sign.
/**********************************************************************/

/**********************************************************************/
BigFloat add_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    copy_bf(g_bf_tmp_copy1, n1);
    copy_bf(g_bf_tmp_copy2, n2);
    unsafe_add_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
BigFloat add_a_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_add_a_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat sub_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    copy_bf(g_bf_tmp_copy1, n1);
    copy_bf(g_bf_tmp_copy2, n2);
    unsafe_sub_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
BigFloat sub_a_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_sub_a_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
// mult and div only change sign
BigFloat full_mult_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    copy_bf(g_bf_tmp_copy1, n1);
    copy_bf(g_bf_tmp_copy2, n2);
    unsafe_full_mult_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
BigFloat mult_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    copy_bf(g_bf_tmp_copy1, n1);
    copy_bf(g_bf_tmp_copy2, n2);
    unsafe_mult_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
BigFloat full_square_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_full_square_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat square_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_square_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat mult_bf_int(BigFloat r, BigFloat n, U16 u)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_mult_bf_int(r, g_bf_tmp_copy1, u);
    return r;
}

/**********************************************************************/
BigFloat div_bf_int(BigFloat r, BigFloat n,  U16 u)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_div_bf_int(r, g_bf_tmp_copy1, u);
    return r;
}

/**********************************************************************/
char *bf_to_str(char *s, BigFloat r, int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    unsafe_bf_to_str(s, g_bf_tmp_copy1, dec);
    return s;
}

/**********************************************************************/
char *bf_to_str_e(char *s, BigFloat r, int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    unsafe_bf_to_str_e(s, g_bf_tmp_copy1, dec);
    return s;
}

/**********************************************************************/
char *bf_to_str_f(char *s, BigFloat r, int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    unsafe_bf_to_str_f(s, g_bf_tmp_copy1, dec);
    return s;
}

/**********************************************************************/
BigFloat inv_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_inv_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat div_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    copy_bf(g_bf_tmp_copy1, n1);
    copy_bf(g_bf_tmp_copy2, n2);
    unsafe_div_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
BigFloat sqrt_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_sqrt_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat ln_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_ln_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat sin_cos_bf(BigFloat s, BigFloat c, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    return unsafe_sin_cos_bf(s, c, g_bf_tmp_copy1);
}

/**********************************************************************/
BigFloat atan_bf(BigFloat r, BigFloat n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_atan_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloat atan2_bf(BigFloat r, BigFloat ny, BigFloat nx)
{
    copy_bf(g_bf_tmp_copy1, ny);
    copy_bf(g_bf_tmp_copy2, nx);
    unsafe_atan2_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
bool is_bf_zero(BigFloat n)
{
    return !is_bf_not_zero(n);
}

/************************************************************************/
// convert_bf  -- convert bigfloat numbers from old to new lengths
int convert_bf(BigFloat new_num, BigFloat old_num, int new_bf_len, int old_bf_len)
{
    // save lengths so not dependent on external environment
    int save_bf_length = g_bf_length;
    g_bf_length      = new_bf_len;
    clear_bf(new_num);
    g_bf_length      = save_bf_length;

    if (new_bf_len > old_bf_len)
    {
        std::memcpy(new_num+new_bf_len-old_bf_len, old_num, old_bf_len+2);
    }
    else
    {
        std::memcpy(new_num, old_num+old_bf_len-new_bf_len, new_bf_len+2);
    }
    return 0;
}

// The following used to be in bigfltc.c
/********************************************************************/
// normalize big float
BigFloat norm_bf(BigFloat r)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);

    // check for overflow
    Byte hi_byte = r[g_bf_length - 1];
    if (hi_byte != 0x00 && hi_byte != 0xFF)
    {
        std::memmove(r, r+1, g_bf_length-1);
        r[g_bf_length-1] = static_cast<Byte>(hi_byte & 0x80 ? 0xFF : 0x00);
        BIG_SET_S16(r_exp, BIG_ACCESS_S16(r_exp) + static_cast<S16>(1));   // exp
    }

    // check for underflow
    else
    {
        int scale;
        for (scale = 2; scale < g_bf_length && r[g_bf_length-scale] == hi_byte; scale++)
        {
            // do nothing
        }
        if (scale == g_bf_length && hi_byte == 0)   // zero
        {
            BIG_SET_S16(r_exp, 0);
        }
        else
        {
            scale -= 2;
            if (scale > 0) // it did underflow
            {
                std::memmove(r+scale, r, g_bf_length-scale-1);
                std::memset(r, 0, scale);
                BIG_SET_S16(r_exp, BIG_ACCESS_S16(r_exp) - static_cast<S16>(scale));    // exp
            }
        }
    }

    return r;
}

/********************************************************************/
// normalize big float with forced sign
// positive = 1, force to be positive
//          = 0, force to be negative
void norm_sign_bf(BigFloat r, bool positive)
{
    norm_bf(r);
    r[g_bf_length-1] = static_cast<Byte>(positive ? 0x00 : 0xFF);
}
/******************************************************/
// adjust n1, n2 for before addition or subtraction
// by forcing exp's to match.
// returns the value of the adjusted exponents
S16 adjust_bf_add(BigFloat n1, BigFloat n2)
{
    int scale;
    int fill_byte;
    S16 r_exp;

    // scale n1 or n2
    // compare exp's
    S16 *n1_exp = reinterpret_cast<S16 *>(n1 + g_bf_length);
    S16 *n2_exp = reinterpret_cast<S16 *>(n2 + g_bf_length);
    if (BIG_ACCESS_S16(n1_exp) > BIG_ACCESS_S16(n2_exp))
    {
        // scale n2
        scale = BIG_ACCESS_S16(n1_exp) - BIG_ACCESS_S16(n2_exp); // n1exp - n2exp
        if (scale < g_bf_length)
        {
            fill_byte = is_bf_neg(n2) ? 0xFF : 0x00;
            std::memmove(n2, n2+scale, g_bf_length-scale);
            std::memset(n2+g_bf_length-scale, fill_byte, scale);
        }
        else
        {
            clear_bf(n2);
        }
        BIG_SET_S16(n2_exp, BIG_ACCESS_S16(n1_exp)); // *n2exp = *n1exp; set exp's =
        r_exp = BIG_ACCESS_S16(n2_exp);
    }
    else if (BIG_ACCESS_S16(n1_exp) < BIG_ACCESS_S16(n2_exp))
    {
        // scale n1
        scale = BIG_ACCESS_S16(n2_exp) - BIG_ACCESS_S16(n1_exp);  // n2exp - n1exp
        if (scale < g_bf_length)
        {
            fill_byte = is_bf_neg(n1) ? 0xFF : 0x00;
            std::memmove(n1, n1+scale, g_bf_length-scale);
            std::memset(n1+g_bf_length-scale, fill_byte, scale);
        }
        else
        {
            clear_bf(n1);
        }
        BIG_SET_S16(n1_exp, BIG_ACCESS_S16(n2_exp)); // *n1exp = *n2exp; set exp's =
        r_exp = BIG_ACCESS_S16(n2_exp);
    }
    else
    {
        r_exp = BIG_ACCESS_S16(n1_exp);
    }
    return r_exp;
}

/********************************************************************/
// r = max positive value
BigFloat max_bf(BigFloat r)
{
    int_to_bf(r, 1);
    BIG_SET16(r+g_bf_length, (S16)(LDBL_MAX_EXP/8));
    return r;
}

/****************************************************************************/
// n1 != n2 ?
// RETURNS:
//  if n1 == n2 returns 0
//  if n1 > n2 returns a positive (bytes left to go when mismatch occurred)
//  if n1 < n2 returns a negative (bytes left to go when mismatch occurred)

int cmp_bf(BigFloat n1, BigFloat n2)
{
    // compare signs
    int sign1 = sign_bf(n1);
    int sign2 = sign_bf(n2);
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
    S16 *n1_exp = reinterpret_cast<S16 *>(n1 + g_bf_length);
    S16 *n2_exp = reinterpret_cast<S16 *>(n2 + g_bf_length);
    if (BIG_ACCESS_S16(n1_exp) > BIG_ACCESS_S16(n2_exp))
    {
        return sign1*g_bf_length;
    }
    if (BIG_ACCESS_S16(n1_exp) < BIG_ACCESS_S16(n2_exp))
    {
        return -sign1*g_bf_length;
    }

    // To get to this point, the signs must match
    // so unsigned comparison is ok.
    // two bytes at a time
    for (int i = g_bf_length-2; i >= 0; i -= 2)
    {
        U16 value1 = BIG_ACCESS16(n1 + i);
        U16 value2 = BIG_ACCESS16(n2 + i);
        if (value1 > value2)
        {
            // now determine which of the two bytes was different
            if ((value1&0xFF00) > (value2&0xFF00))     // compare just high bytes
            {
                return i+2; // high byte was different
            }

            return i+1; // low byte was different
        }
        if (value1 < value2)
        {
            // now determine which of the two bytes was different
            if ((value1&0xFF00) < (value2&0xFF00))     // compare just high bytes
            {
                return -(i+2); // high byte was different
            }

            return -(i+1); // low byte was different
        }
    }
    return 0;
}

/********************************************************************/
// r < 0 ?
// returns 1 if negative, 0 if positive or zero
bool is_bf_neg(BigFloat n)
{
    return static_cast<S8>(n[g_bf_length - 1]) < 0;
}

/********************************************************************/
// n != 0 ?
// RETURNS: if n != 0 returns 1
//          else returns 0
bool is_bf_not_zero(BigFloat n)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    bool result = is_bn_not_zero(n);
    g_bn_length = bnl;

    return result;
}

/********************************************************************/
// r = n1 + n2
// SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision
BigFloat unsafe_add_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    if (is_bf_zero(n1))
    {
        copy_bf(r, n2);
        return r;
    }
    if (is_bf_zero(n2))
    {
        copy_bf(r, n1);
        return r;
    }

    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    BIG_SET_S16(r_exp, adjust_bf_add(n1, n2));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    add_bn(r, n1, n2);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r += n
BigFloat unsafe_add_a_bf(BigFloat r, BigFloat n)
{
    if (is_bf_zero(r))
    {
        copy_bf(r, n);
        return r;
    }
    if (is_bf_zero(n))
    {
        return r;
    }

    adjust_bf_add(r, n);

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    add_a_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);

    return r;
}

/********************************************************************/
// r = n1 - n2
// SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision
BigFloat unsafe_sub_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    if (is_bf_zero(n1))
    {
        neg_bf(r, n2);
        return r;
    }
    if (is_bf_zero(n2))
    {
        copy_bf(r, n1);
        return r;
    }

    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    BIG_SET_S16(r_exp, adjust_bf_add(n1, n2));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    sub_bn(r, n1, n2);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r -= n
BigFloat unsafe_sub_a_bf(BigFloat r, BigFloat n)
{
    if (is_bf_zero(r))
    {
        neg_bf(r, n);
        return r;
    }
    if (is_bf_zero(n))
    {
        return r;
    }

    adjust_bf_add(r, n);

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    sub_a_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r = -n
BigFloat neg_bf(BigFloat r, BigFloat n)
{
    S16 *r_exp = reinterpret_cast<S16 *>(r + g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, BIG_ACCESS_S16(n_exp)); // *r_exp = *n_exp;

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    neg_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r *= -1
BigFloat neg_a_bf(BigFloat r)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    neg_a_bn(r);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/********************************************************************/
// r = 2*n
BigFloat double_bf(BigFloat r, BigFloat n)
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
BigFloat double_a_bf(BigFloat r)
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
BigFloat half_bf(BigFloat r, BigFloat n)
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
// r /= 2
BigFloat half_a_bf(BigFloat r)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    half_a_bn(r);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

/************************************************************************/
// r = n1 * n2
// Note: r will be a double wide result, 2*g_bf_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigFloat unsafe_full_mult_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        g_bf_length <<= 1;
        clear_bf(r);
        g_bf_length >>= 1;
        return r;
    }

    S16 *r_exp = reinterpret_cast<S16 *>(r + 2 * g_bf_length);
    S16 *n1_exp = reinterpret_cast<S16 *>(n1 + g_bf_length);
    S16 *n2_exp = reinterpret_cast<S16 *>(n2 + g_bf_length);
    // add exp's
    BIG_SET_S16(r_exp, (S16)(BIG_ACCESS_S16(n1_exp) + BIG_ACCESS_S16(n2_exp)));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    unsafe_full_mult_bn(r, n1, n2);
    g_bn_length = bnl;

    // handle normalizing full mult on individual basis

    return r;
}

/************************************************************************/
// r = n1 * n2 calculating only the top g_r_length bytes
// Note: r will be of length g_r_length
//       2*g_bf_length <= g_r_length < g_bf_length
//       n1 and n2 can be the same pointer
// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigFloat unsafe_mult_bf(BigFloat r, BigFloat n1, BigFloat n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        clear_bf(r);
        return r;
    }

    S16 *n1_exp = reinterpret_cast<S16 *>(n1 + g_bf_length);
    S16 *n2_exp = reinterpret_cast<S16 *>(n2 + g_bf_length);
    // add exp's
    int r_exp = BIG_ACCESS_S16(n1_exp) + BIG_ACCESS_S16(n2_exp);

    const bool positive = is_bf_neg(n1) == is_bf_neg(n2); // are they the same sign?

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    int rl = g_r_length;
    g_r_length = g_r_bf_length;
    unsafe_mult_bn(r, n1, n2);
    g_bn_length = bnl;
    g_r_length = rl;

    int bfl = g_bf_length;
    g_bf_length = g_r_bf_length;
    BIG_SET16(r+g_bf_length, static_cast<S16>(r_exp + 2)); // adjust after mult
    norm_sign_bf(r, positive);
    g_bf_length = bfl;
    std::memmove(r, r+g_padding, g_bf_length+2); // shift back

    return r;
}

/************************************************************************/
// r = n^2
//   because of the symmetry involved, n^2 is much faster than n*n
//   for a bignumber of length l
//      n*n takes l^2 multiplications
//      n^2 takes (l^2+l)/2 multiplications
//          which is about 1/2 n*n as l gets large
//  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
/*                                                                      */
// SIDE-EFFECTS: n is changed to its absolute value
BigFloat unsafe_full_square_bf(BigFloat r, BigFloat n)
{
    if (is_bf_zero(n))
    {
        g_bf_length <<= 1;
        clear_bf(r);
        g_bf_length >>= 1;
        return r;
    }

    S16 *r_exp = reinterpret_cast<S16 *>(r + 2 * g_bf_length);
    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    BIG_SET_S16(r_exp, 2 * BIG_ACCESS_S16(n_exp));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    unsafe_full_square_bn(r, n);
    g_bn_length = bnl;

    // handle normalizing full mult on individual basis

    return r;
}

/************************************************************************/
// r = n^2
//   because of the symmetry involved, n^2 is much faster than n*n
//   for a bignumber of length l
//      n*n takes l^2 multiplications
//      n^2 takes (l^2+l)/2 multiplications
//          which is about 1/2 n*n as l gets large
//  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)
/*                                                                      */
// Note: r will be of length g_r_length
//       2*g_bf_length >= g_r_length > g_bf_length
// SIDE-EFFECTS: n is changed to its absolute value
BigFloat unsafe_square_bf(BigFloat r, BigFloat n)
{
    if (is_bf_zero(n))
    {
        clear_bf(r);
        return r;
    }

    S16 *n_exp = reinterpret_cast<S16 *>(n + g_bf_length);
    int r_exp = static_cast<S16>(2 * BIG_ACCESS_S16(n_exp));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    int rl = g_r_length;
    g_r_length = g_r_bf_length;
    unsafe_square_bn(r, n);
    g_bn_length = bnl;
    g_r_length = rl;

    int bfl = g_bf_length;
    g_bf_length = g_r_bf_length;
    BIG_SET16(r+g_bf_length, static_cast<S16>(r_exp + 2)); // adjust after mult

    norm_sign_bf(r, true);
    g_bf_length = bfl;
    std::memmove(r, r+g_padding, g_bf_length+2); // shift back

    return r;
}

/********************************************************************/
// r = n * u  where u is an unsigned integer
// SIDE-EFFECTS: n can be "de-normalized" and lose precision
BigFloat unsafe_mult_bf_int(BigFloat r, BigFloat n, U16 u)
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
BigFloat mult_a_bf_int(BigFloat r, U16 u)
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
BigFloat unsafe_div_bf_int(BigFloat r, BigFloat n,  U16 u)
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
BigFloat div_a_bf_int(BigFloat r, U16 u)
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

    LDouble orig_b = b;
    LDouble af = f >= 0 ? f : -f;     // abs value
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
        if (value[n-1] < ff)
        {
            ff /= value[n-1];
            *exp_ptr += power_two;
        }
    }
    if (f < 0)
    {
        ff = -ff;
    }
    if (af < 1)
    {
        ff = orig_b/ff;
        *exp_ptr = -*exp_ptr - 1;
    }

    return ff;
}

/********************************************************************/
// calculates and returns the value of f*b^n
// sort of like ldexp()
LDouble scale_value(LDouble f, LDouble b , int n)
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
    else     // n < 0
    {
        f /= total;
    }
    return f;
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
LDouble scale_10(LDouble f, int n)
{
    return scale_value(f, 10, n);
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

BigFloat10 unsafe_bf_to_bf10(BigFloat10 r, int dec, BigFloat n)
{
    if (is_bf_zero(n))
    {
        // in scientific notation, the leading digit can't be zero
        r[1] = static_cast<Byte>(0); // unless the number is zero
        return r;
    }

    BigFloat ones_byte = n + g_bf_length - 1;           // really it's n+g_bf_length-2
    int power256 = static_cast<S16>(BIG_ACCESS16(n + g_bf_length)) + 1; // so adjust power256 by 1

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++;  // one extra byte for rounding
    BigFloat10 power10 = r + dec + 1;

    if (is_bf_neg(n))
    {
        neg_a_bf(n);
        r[0] = 1; // sign flag
    }
    else
    {
        r[0] = 0;
    }

    int p = -1;  // multiply by 10 right away
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
            p--; // and decrease by a factor of 10
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
        while (d > 0) // stop before you get to the sign flag
        {
            r[d]++;  // round up
            if (r[d] < 10)
            {
                d = -1; // flag for below
                break; // finished rounding
            }
            r[d] = 0;
            d--;
        }
        if (d == 0) // rounding went back to the first digit and it overflowed
        {
            r[1] = 0;
            std::memmove(r+2, r+1, dec-1);
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

BigFloat10 mult_a_bf10_int(BigFloat10 r, int dec, U16 n)
{
    if (r[1] == 0 || n == 0)
    {
        r[1] = 0;
        return r;
    }

    BigFloat10 power10 = r + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    int sign_flag = r[0];  // r[0] to be used as a padding
    unsigned overflow = 0;
    for (int d = dec; d > 0; d--)
    {
        unsigned value = r[d] * n + overflow;
        r[d] = static_cast<Byte>(value % 10);
        overflow = value / 10;
    }
    while (overflow)
    {
        p++;
        std::memmove(r+2, r+1, dec-1);
        r[1] = static_cast<Byte>(overflow % 10);
        overflow = overflow / 10;
    }
    BIG_SET16(power10, static_cast<U16>(p)); // save power of ten
    r[0] = static_cast<Byte>(sign_flag); // restore sign flag
    return r;
}

/**********************************************************************/
// div_a_bf10_int()
// r /= n
// dec - number of decimals, including the one extra for rounding

BigFloat10 div_a_bf10_int(BigFloat10 r, int dec, U16 n)
{
    unsigned value;

    if (r[1] == 0 || n == 0)
    {
        r[1] = 0;
        return r;
    }

    BigFloat10 power10 = r + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    unsigned remainder = 0;
    int dest = 1;
    for (int src = 1; src <= dec; dest++, src++)
    {
        value = 10*remainder + r[src];
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
        value = 10*remainder;
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
// bf10tostr_e()
// Takes a bf10 number and converts it to an ascii string, sci. notation
// dec - number of decimals, not including the one extra for rounding

char *bf10_to_str_e(char *s, BigFloat10 n, int dec)
{
    if (n[1] == 0)
    {
        std::strcpy(s, "0.0");
        return s;
    }

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++;  // one extra byte for rounding
    BigFloat10 power10 = n + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    // if p is negative, it is not necessary to show all the decimal places
    if (p < 0 && dec > 8) // 8 sounds like a reasonable value
    {
        dec = dec + p;
        dec = std::max(dec, 8); // let's keep at least a few
    }

    if (n[0] == 1)   // sign flag
    {
        *s++ = '-';
    }
    *s++ = static_cast<char>(n[1] + '0');
    *s++ = '.';
    for (int d = 2; d <= dec; d++)
    {
        *s++ = static_cast<char>(n[d] + '0');
    }
    // clean up trailing 0's
    while (*(s-1) == '0')
    {
        s--;
    }
    if (*(s-1) == '.')   // put at least one 0 after the decimal
    {
        *s++ = '0';
    }
    std::sprintf(s, "e%d", p);
    return s;
}

/****************************************************************************/
// bf10tostr_f()
// Takes a bf10 number and converts it to an ascii string, decimal notation

char *bf10_to_str_f(char *s, BigFloat10 n, int dec)
{
    if (n[1] == 0)
    {
        std::strcpy(s, "0.0");
        return s;
    }

    if (dec == 0)
    {
        dec = g_decimals;
    }
    dec++;  // one extra byte for rounding
    BigFloat10 power10 = n + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    // if p is negative, it is not necessary to show all the decimal places
    if (p < 0 && dec > 8) // 8 sounds like a reasonable value
    {
        dec = dec + p;
        dec = std::max(dec, 8); // let's keep at least a few
    }

    if (n[0] == 1)   // sign flag
    {
        *s++ = '-';
    }
    if (p >= 0)
    {
        int d;
        for (d = 1; d <= p+1; d++)
        {
            *s++ = static_cast<char>(n[d] + '0');
        }
        *s++ = '.';
        for (; d <= dec; d++)
        {
            *s++ = static_cast<char>(n[d] + '0');
        }
    }
    else
    {
        *s++ = '0';
        *s++ = '.';
        for (int d = 0; d > p+1; d--)
        {
            *s++ = '0';
        }
        for (int d = 1; d <= dec; d++)
        {
            *s++ = static_cast<char>(n[d] + '0');
        }
    }

    // clean up trailing 0's
    while (*(s-1) == '0')
    {
        s--;
    }
    if (*(s-1) == '.')   // put at least one 0 after the decimal
    {
        *s++ = '0';
    }
    *s = '\0'; // terminating nul
    return s;
}

} // namespace id::math
