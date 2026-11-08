// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigFloat.h>

#include <math/BigNum.h>

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

void validate_buffer(BigFloatConstPtr buffer, const std::size_t size, const std::size_t required)
{
    if (buffer == nullptr || size < required)
    {
        throw std::invalid_argument("BigFloat requires a complete legacy value buffer");
    }
}

// Align both mantissas in place; return their common exponent.
S16 align_exponents(BigFloatPtr n1, BigFloatPtr n2)
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
            std::memmove(n2, n2 + scale, g_bf_length - scale);
            std::memset(n2 + g_bf_length - scale, fill_byte, scale);
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
        scale = BIG_ACCESS_S16(n2_exp) - BIG_ACCESS_S16(n1_exp); // n2exp - n1exp
        if (scale < g_bf_length)
        {
            fill_byte = is_bf_neg(n1) ? 0xFF : 0x00;
            std::memmove(n1, n1 + scale, g_bf_length - scale);
            std::memset(n1 + g_bf_length - scale, fill_byte, scale);
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

// SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision
BigFloatPtr add(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
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
    BIG_SET_S16(r_exp, align_exponents(n1, n2));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    add_bn(r, n1, n2);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// r += n
BigFloatPtr add_assign(BigFloatPtr r, BigFloatPtr n)
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

    align_exponents(r, n);

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    add_a_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);

    return r;
}

// SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision
BigFloatPtr subtract(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
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
    BIG_SET_S16(r_exp, align_exponents(n1, n2));

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    sub_bn(r, n1, n2);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// r -= n
BigFloatPtr subtract_assign(BigFloatPtr r, BigFloatPtr n)
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

    align_exponents(r, n);

    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    sub_a_bn(r, n);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// SIDE-EFFECTS: n1 and n2 are changed to their absolute values
BigFloatPtr multiply(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        clear_bf(r);
        return r;
    }

    S16 *n1_exp = reinterpret_cast<S16 *>(n1 + g_bf_length);
    S16 *n2_exp = reinterpret_cast<S16 *>(n2 + g_bf_length);
    // add exp's
    const int r_exp = BIG_ACCESS_S16(n1_exp) + BIG_ACCESS_S16(n2_exp);

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
    BIG_SET16(r + g_bf_length, static_cast<S16>(r_exp + 2)); // adjust after mult
    norm_sign_bf(r, positive);
    g_bf_length = bfl;
    std::memmove(r, r + g_padding, g_bf_length + 2);         // shift back

    return r;
}

// Division destroys both operands; the result needs only canonical bytes.
BigFloatPtr divide(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    // first, check for valid data

    const int a_exp = static_cast<S16>(BIG_ACCESS16(n1 + g_bf_length));
    BIG_SET16(n1 + g_bf_length, static_cast<S16>(0)); // put within LDouble range

    LDouble a = bf_to_float(n1);
    if (a == 0)                                       // division into zero
    {
        clear_bf(r);                                  // return 0
        return r;
    }

    const int b_exp = static_cast<S16>(BIG_ACCESS16(n2 + g_bf_length));
    BIG_SET16(n2 + g_bf_length, static_cast<S16>(0)); // put within LDouble range

    LDouble b = bf_to_float(n2);
    if (b == 0)                                       // division by zero
    {
        max_bf(r);
        return r;
    }

    unsafe_inv_bf(r, n2);
    multiply(g_bf_tmp1, n1, r);
    copy_bf(r, g_bf_tmp1); // r = g_bf_tmp1

    int r_exp = static_cast<S16>(BIG_ACCESS16(r + g_bf_length));
    r_exp += a_exp - b_exp;
    BIG_SET16(r + g_bf_length, static_cast<S16>(r_exp)); // adjust result exponent

    return r;
}

// r *= -1
BigFloatPtr negate(BigFloatPtr r)
{
    int bnl = g_bn_length;
    g_bn_length = g_bf_length;
    neg_a_bn(r);
    g_bn_length = bnl;

    norm_bf(r);
    return r;
}

// Keep the legacy signed mismatch position for raw comparison callers.
int compare(BigFloatConstPtr n1, BigFloatConstPtr n2)
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

BigFloatPtr binary_operation(BigFloatPtr result, BigFloatConstPtr left, BigFloatConstPtr right,
    BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1, left);
    copy_bf(g_bf_tmp_copy2, right);
    return operation(result, g_bf_tmp_copy1, g_bf_tmp_copy2);
}

BigFloatPtr compound_operation(
    BigFloatPtr result, BigFloatConstPtr right, BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1, right);
    return operation(result, g_bf_tmp_copy1);
}

} // namespace

BigFloat::BigFloat() :
    m_storage(static_cast<std::size_t>(g_bf_length) + 2)
{
}

BigFloat::BigFloat(const LDouble value)
{
    *this = value;
}

BigFloat::BigFloat(const std::string_view text) :
    BigFloat()
{
    str_to_bf(m_storage.data(), text);
}

BigFloat::BigFloat(const BigNum &value) :
    BigFloat()
{
    value.copy_to_legacy_float(m_storage.data(), m_storage.size());
}

BigFloat &BigFloat::operator=(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigFloat requires a finite scalar value");
    }
    m_storage.resize(static_cast<std::size_t>(g_bf_length) + 2);
    float_to_bf(m_storage.data(), value);
    return *this;
}

BigFloat::BigFloat(std::vector<Byte> storage) :
    m_storage(std::move(storage))
{
}

BigFloat BigFloat::operator-() const
{
    BigFloat result{*this};
    negate(result.m_storage.data());
    return result;
}

BigFloat &BigFloat::operator+=(const BigFloat &right)
{
    compound_operation(m_storage.data(), right.m_storage.data(), add_assign);
    return *this;
}

BigFloat &BigFloat::operator-=(const BigFloat &right)
{
    compound_operation(m_storage.data(), right.m_storage.data(), subtract_assign);
    return *this;
}

BigFloat &BigFloat::operator*=(const BigFloat &right)
{
    *this = *this * right;
    return *this;
}

BigFloat &BigFloat::operator/=(const BigFloat &right)
{
    *this = *this / right;
    return *this;
}

BigFloat BigFloat::operator+(const BigFloat &right) const
{
    BigFloat result;
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), add);
    return result;
}

BigFloat BigFloat::operator-(const BigFloat &right) const
{
    BigFloat result;
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), subtract);
    return result;
}

BigFloat BigFloat::operator*(const BigFloat &right) const
{
    BigFloat result{std::vector<Byte>(static_cast<std::size_t>(g_r_bf_length) + 2)};
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), multiply);
    result.m_storage.resize(static_cast<std::size_t>(g_bf_length) + 2);
    return result;
}

BigFloat BigFloat::operator/(const BigFloat &right) const
{
    BigFloat result;
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), divide);
    return result;
}

bool BigFloat::operator==(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) == 0;
}

bool BigFloat::operator!=(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) != 0;
}

bool BigFloat::operator<(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) < 0;
}

bool BigFloat::operator<=(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) <= 0;
}

bool BigFloat::operator>(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) > 0;
}

bool BigFloat::operator>=(const BigFloat &right) const
{
    return compare(m_storage.data(), right.m_storage.data()) >= 0;
}

std::string BigFloat::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigFloat formatting requires a nonnegative digit count");
    }
    const int default_digits = g_decimals > 0 ? g_decimals : static_cast<int>((g_bf_length - 2) * LOG10_256);
    return bf_to_string(m_storage.data(), digits == 0 ? default_digits : digits);
}

BigFloat BigFloat::from_legacy(BigFloatConstPtr source, const std::size_t size)
{
    BigFloat result;
    validate_buffer(source, size, result.m_storage.size());
    std::copy_n(source, result.m_storage.size(), result.m_storage.data());
    return result;
}

void BigFloat::copy_to_legacy(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, m_storage.size());
    std::copy(m_storage.begin(), m_storage.end(), destination);
}

void BigFloat::copy_to_legacy_num(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bn_length));
    bf_to_bn(destination, m_storage.data());
}

// Raw compatibility adapters for callers not yet using owning values.
BigFloatPtr unsafe_add_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return add(r, n1, n2);
}

BigFloatPtr unsafe_add_a_bf(BigFloatPtr r, BigFloatPtr n)
{
    return add_assign(r, n);
}

BigFloatPtr unsafe_sub_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return subtract(r, n1, n2);
}

BigFloatPtr unsafe_sub_a_bf(BigFloatPtr r, BigFloatPtr n)
{
    return subtract_assign(r, n);
}

BigFloatPtr unsafe_mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return multiply(r, n1, n2);
}

BigFloatPtr unsafe_div_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return divide(r, n1, n2);
}

BigFloatPtr neg_a_bf(BigFloatPtr r)
{
    return negate(r);
}

int cmp_bf(BigFloatConstPtr n1, BigFloatConstPtr n2)
{
    return compare(n1, n2);
}

BigFloatPtr add_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return binary_operation(r, n1, n2, add);
}

BigFloatPtr sub_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return binary_operation(r, n1, n2, subtract);
}

BigFloatPtr mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return binary_operation(r, n1, n2, multiply);
}

BigFloatPtr div_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    return binary_operation(r, n1, n2, divide);
}

BigFloatPtr add_a_bf(BigFloatPtr r, BigFloatPtr n)
{
    return compound_operation(r, n, add_assign);
}

BigFloatPtr sub_a_bf(BigFloatPtr r, BigFloatPtr n)
{
    return compound_operation(r, n, subtract_assign);
}

} // namespace id::math
