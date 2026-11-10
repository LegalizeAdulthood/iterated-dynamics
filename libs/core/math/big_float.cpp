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
#include <string>
#include <vector>

using namespace id::misc;

namespace id::math
{

namespace
{

using BigFloat10 = Byte *;

BigFloat10 unsafe_bf_to_bf10(BigFloat10 r, int dec, BigFloatPtr n);
BigFloat10 mult_a_bf10_int(BigFloat10 r, int dec, U16 n);
BigFloat10 div_a_bf10_int(BigFloat10 r, int dec, U16 n);
std::string unsafe_bf_to_string(BigFloatPtr r, int dec);
std::string unsafe_bf_to_string_e(BigFloatPtr r, int dec);
std::string unsafe_bf_to_string_f(BigFloatPtr r, int dec);
std::string bf10_to_string_e(BigFloat10 n, int dec);
std::string bf10_to_string_f(BigFloat10 n, int dec);

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

} // namespace

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

/**********************************************************************/
// strtobf() - converts a string into a bigfloat
//   r - pointer to a bigfloat
//   s - string in the floating point format [+-][dig].[dig]e[+-][dig]
//   note: the string may not be empty or have extra space.
//         It may use scientific notation.
// USES: g_bf_tmp1

BigFloatPtr str_to_bf(BigFloatPtr r, std::string_view s)
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
            int_to_bf(g_bf_tmp1, ones_byte);
            unsafe_add_a_bf(r, g_bf_tmp1);
            div_a_bf_int(r, 10);
        }

        if (l > 0 && s[l - 1] == '.') // the digit was found
        {
            --l;
            bool keep_looping = l > 0 && is_digit(s[l - 1]);
            while (keep_looping) // while a digit
            {
                ones_byte = static_cast<Byte>(s[--l] - '0');
                int_to_bf(g_bf_tmp1, ones_byte);
                unsafe_add_a_bf(r, g_bf_tmp1);
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
            int_to_bf(g_bf_tmp1, ones_byte);
            unsafe_add_a_bf(r, g_bf_tmp1);
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

/********************************************************************/
// bftostring() - converts a bigfloat into a scientific notation string
// dec - decimal places, 0 for max
//   r - bigfloat
//   will convert to a floating point notation
//   SIDE-EFFECT: the bigfloat, r, is destroyed.
//                Copy it first if necessary.
// USES: g_bf_tmp1 - g_bf_tmp2
/********************************************************************/

namespace
{

std::string unsafe_bf_to_string(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    int power = static_cast<S16>(BIG_ACCESS16(decimal + dec + 2));    // where the exponent is stored
    if (power > -4 && power < 6)                                      // tinker with this
    {
        return bf10_to_string_f(decimal, dec);
    }
    return bf10_to_string_e(decimal, dec);
}

std::string unsafe_bf_to_string_e(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    return bf10_to_string_e(decimal, dec);
}

std::string unsafe_bf_to_string_f(BigFloatPtr r, const int dec)
{
    LDouble value = bf_to_float(r);
    if (value == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> storage(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    BigFloat10 decimal{storage.data()};
    unsafe_bf_to_bf10(decimal, dec, r);
    return bf10_to_string_f(decimal, dec);
}

} // namespace

/*********************************************************************/
//  bn = floor(bf)
//  Converts a bigfloat to a bignumber (integer)
//  g_bf_length must be at least g_bn_length+2
BigNumPtr bf_to_bn(BigNumPtr n, BigFloatConstPtr f)
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
    std::memset(n+move_bytes, hi_byte, g_bn_length-move_bytes); // sign extends
    return n;
}

/*********************************************************************/
//  bf = bn
//  Converts a bignumber (integer) to a bigfloat
//  g_bf_length must be at least g_bn_length+2
BigFloatPtr bn_to_bf(BigFloatPtr f, BigNumConstPtr n)
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
BigFloatPtr int_to_bf(BigFloatPtr r, const long value)
{
    clear_bf(r);
    BIG_SET32(r+g_bf_length-4, static_cast<S32>(value));
    BIG_SET16(r+g_bf_length, static_cast<S16>(2));
    norm_bf(r);
    return r;
}

/********************************************************************/
// sign(r)
int sign_bf(BigFloatConstPtr n)
{
    return is_bf_neg(n) ? -1 : is_bf_not_zero(n) ? 1 : 0;
}

/********************************************************************/
// r = |n|
BigFloatPtr abs_bf(BigFloatPtr r, BigFloatPtr n)
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
BigFloatPtr abs_a_bf(BigFloatPtr r)
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






/**********************************************************************/
// The rest of the functions are "safe" versions of the routines that
// have side effects which alter the parameters.
// Most bf routines change values of parameters, not just the sign.
/**********************************************************************/


/**********************************************************************/
BigFloatPtr square_bf(BigFloatPtr r, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_square_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloatPtr mult_bf_int(BigFloatPtr r, BigFloatConstPtr n, const U16 u)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_mult_bf_int(r, g_bf_tmp_copy1, u);
    return r;
}

/**********************************************************************/
std::string bf_to_string(BigFloatConstPtr r, const int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    return unsafe_bf_to_string(g_bf_tmp_copy1, dec);
}

/**********************************************************************/
std::string bf_to_string_e(BigFloatConstPtr r, const int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    return unsafe_bf_to_string_e(g_bf_tmp_copy1, dec);
}

/**********************************************************************/
std::string bf_to_string_f(BigFloatConstPtr r, const int dec)
{
    copy_bf(g_bf_tmp_copy1, r);
    return unsafe_bf_to_string_f(g_bf_tmp_copy1, dec);
}

/**********************************************************************/
BigFloatPtr ln_bf(BigFloatPtr r, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1, n);
    unsafe_ln_bf(r, g_bf_tmp_copy1);
    return r;
}

/**********************************************************************/
BigFloatPtr sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1, n);
    return unsafe_sin_cos_bf(s, c, g_bf_tmp_copy1);
}

/**********************************************************************/
BigFloatPtr atan2_bf(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx)
{
    copy_bf(g_bf_tmp_copy1, ny);
    copy_bf(g_bf_tmp_copy2, nx);
    unsafe_atan2_bf(r, g_bf_tmp_copy1, g_bf_tmp_copy2);
    return r;
}

/**********************************************************************/
bool is_bf_zero(BigFloatConstPtr n)
{
    return !is_bf_not_zero(n);
}

/************************************************************************/
// convert_bf  -- convert bigfloat numbers from old to new lengths
int convert_bf(BigFloatPtr new_num, BigFloatConstPtr old_num, const int new_bf_len, const int old_bf_len)
{
    const int retained = std::min(new_bf_len, old_bf_len);
    std::memset(new_num, 0, new_bf_len + 2);
    std::memcpy(new_num + new_bf_len - retained, old_num + old_bf_len - retained, retained + 2);
    return 0;
}

// The following used to be in bigfltc.c
/********************************************************************/
// normalize big float
BigFloatPtr norm_bf(BigFloatPtr r)
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
void norm_sign_bf(BigFloatPtr r, const bool positive)
{
    norm_bf(r);
    r[g_bf_length-1] = static_cast<Byte>(positive ? 0x00 : 0xFF);
}

/********************************************************************/
// r = max positive value
BigFloatPtr max_bf(BigFloatPtr r)
{
    int_to_bf(r, 1);
    BIG_SET16(r+g_bf_length, static_cast<S16>(LDBL_MAX_EXP/8));
    return r;
}

/********************************************************************/
// r < 0 ?
// returns 1 if negative, 0 if positive or zero
bool is_bf_neg(BigFloatConstPtr n)
{
    return static_cast<S8>(n[g_bf_length - 1]) < 0;
}

/********************************************************************/
// n != 0 ?
// RETURNS: if n != 0 returns 1
//          else returns 0
bool is_bf_not_zero(BigFloatConstPtr n)
{
    return std::any_of(n, n + g_bf_length, [](const Byte digit) { return digit != 0; });
}

/********************************************************************/
// r = -n
BigFloatPtr neg_bf(BigFloatPtr r, BigFloatPtr n)
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
    const LDouble af = f >= 0 ? f : -f;     // abs value
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
LDouble scale_value(LDouble f, LDouble b , const int n)
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
LDouble scale_10(LDouble f, const int n)
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

namespace
{

BigFloat10 unsafe_bf_to_bf10(BigFloat10 r, int dec, BigFloatPtr n)
{
    if (is_bf_zero(n))
    {
        // in scientific notation, the leading digit can't be zero
        r[1] = static_cast<Byte>(0); // unless the number is zero
        return r;
    }

    BigFloatPtr ones_byte = n + g_bf_length - 1;                              // really it's n+g_bf_length-2
    const int power256 = static_cast<S16>(BIG_ACCESS16(n + g_bf_length)) + 1; // so adjust power256 by 1

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

BigFloat10 mult_a_bf10_int(BigFloat10 r, const int dec, const U16 n)
{
    if (r[1] == 0 || n == 0)
    {
        r[1] = 0;
        return r;
    }

    BigFloat10 power10 = r + dec + 1;
    int p = static_cast<S16>(BIG_ACCESS16(power10));

    const int sign_flag = r[0];  // r[0] to be used as a padding
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

BigFloat10 div_a_bf10_int(BigFloat10 r, const int dec, const U16 n)
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
// bf10tostring_e()
// Takes a bf10 number and converts it to an ascii string, sci. notation
// dec - number of decimals, not including the one extra for rounding

std::string bf10_to_string_e(BigFloat10 n, int dec)
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
    BigFloat10 power10 = n + dec + 1;
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

std::string bf10_to_string_f(BigFloat10 n, int dec)
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
    BigFloat10 power10 = n + dec + 1;
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

} // namespace

} // namespace id::math
