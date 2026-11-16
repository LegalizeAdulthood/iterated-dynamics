// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigNum.h>

#include "BigStorage.h"

#include <math/big.h>
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
    unsafe_mult_bn(g_bn_tmp1.data(), n1, r);
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

BigNum::BigNum(BigNumProduct &&value) :
    m_storage(std::move(value).release_reduced_storage())
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
        str_to_bn(data(), text);
    }
}

BigNum::BigNum(BigArena &arena, const BigFloat &value) :
    BigNum(arena)
{
    value.copy_to_legacy_num(data(), encoded_size());
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
    std::memset(data(), 0, m_storage.blocks() * arena().block_size());
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
            std::memcpy(data(), value.data(), encoded_size());
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
        std::memset(data(), 0, arena().block_size());
    }
    float_to_bn(data(), value);
    return *this;
}

BigNum BigNum::operator-() const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    neg_bn(result.data(), data());
    return result;
}

BigNum &BigNum::operator+=(const BigNum &right)
{
    add_bn(data(), data(), right.data());
    return *this;
}

BigNum &BigNum::operator-=(const BigNum &right)
{
    sub_bn(data(), data(), right.data());
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
    div_a_bn_int(data(), right);
    return *this;
}

BigNum BigNum::operator+(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    add_bn(result.data(), data(), right.data());
    return result;
}

BigNum BigNum::operator-(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    sub_bn(result.data(), data(), right.data());
    return result;
}

BigNum BigNum::operator*(const BigNum &right) const
{
    BigNumProduct result{bn_arena(BigLifetime::WORKING)};
    binary_operation(result.data(), data(), right.data(), unsafe_mult_bn);
    return BigNum{std::move(result)};
}

BigNum BigNum::operator*(const U16 right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    mult_bn_int(result.data(), data(), right);
    return result;
}

BigNum BigNum::operator/(const BigNum &right) const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    binary_operation(result.data(), data(), right.data(), divide);
    return result;
}

BigNum BigNum::sqr() const
{
    BigNumProduct result{bn_arena(BigLifetime::WORKING)};
    copy_bn(g_bn_tmp_copy1.data(), data());
    unsafe_square_bn(result.data(), g_bn_tmp_copy1.data());
    return BigNum{std::move(result)};
}

BigNumProduct::BigNumProduct() noexcept = default;

BigNumProduct::BigNumProduct(BigArena &arena) :
    m_storage(arena.allocate_double())
{
    std::memset(data(), 0, 2 * arena.block_size());
}

BigNumProduct::BigNumProduct(const BigNumProduct &value) :
    m_storage(value.m_storage.clone())
{
}

BigNumProduct::BigNumProduct(BigArena &arena, const BigNumProduct &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigNumProduct::BigNumProduct(BigNumProduct &&value) noexcept = default;

BigNumProduct &BigNumProduct::operator=(const BigNumProduct &value)
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
                m_storage = destination.allocate_double();
            }
            std::memcpy(data(), value.data(), g_r_length);
        }
    }
    return *this;
}

BigNumProduct &BigNumProduct::operator=(BigNumProduct &&value)
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
            *this = static_cast<const BigNumProduct &>(value);
        }
    }
    return *this;
}

BigArena::Allocation BigNumProduct::release_reduced_storage() &&
{
    BigArena::Allocation result = std::move(m_storage);
    if (result)
    {
        result.reduce();
    }
    return result;
}

void square_bn(BigNumProduct &result, BigNum &source)
{
    const bool sign = is_bn_neg(source.data());
    unsafe_square_bn(result.data(), source.data());
    if (sign)
    {
        neg_a_bn(source.data());
    }
}

void mult_bn(BigNum &result, BigNum &left, BigNum &right)
{
    const bool sign1 = is_bn_neg(left.data());
    const bool sign2 = is_bn_neg(right.data());
    BigNumProduct product{bn_arena(BigLifetime::TEMPORARY)};
    unsafe_mult_bn(product.data(), left.data(), right.data());
    if (sign1)
    {
        neg_a_bn(left.data());
    }
    if (sign2)
    {
        neg_a_bn(right.data());
    }
    copy_bn(result.data(), product.data() + g_shift_factor);
}

bool BigNum::operator==(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) == 0;
}

bool BigNum::operator!=(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) != 0;
}

bool BigNum::operator<(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) < 0;
}

bool BigNum::operator<=(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) <= 0;
}

bool BigNum::operator>(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) > 0;
}

bool BigNum::operator>=(const BigNum &right) const
{
    return cmp_bn(data(), right.data()) >= 0;
}

LDouble BigNum::to_float() const
{
    copy_bn(g_bn_tmp_copy1.data(), data());
    return bn_to_float(g_bn_tmp_copy1.data());
}

std::string BigNum::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigNum formatting requires a nonnegative digit count");
    }
    return bn_to_string(data(), digits);
}

BigNum BigNum::from_legacy(BigNumConstPtr source, const std::size_t size)
{
    return from_legacy(bn_arena(BigLifetime::WORKING), source, size);
}

BigNum BigNum::from_legacy(BigArena &arena, BigNumConstPtr source, const std::size_t size)
{
    BigNum result{arena};
    validate_buffer(source, size, result.encoded_size());
    std::copy_n(source, result.encoded_size(), result.data());
    return result;
}

void BigNum::copy_to_legacy(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, encoded_size());
    std::copy_n(data(), encoded_size(), destination);
}

void BigNum::copy_to_legacy_float(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bf_length) + 2);
    bn_to_bf(destination, data());
}

BigNum sqr(const BigNum &value)
{
    return value.sqr();
}

int cmp_bn(const BigNum &n1, const BigNum &n2)
{
    return cmp_bn(n1.data(), n2.data());
}

BigNum &add_bn(BigNum &r, const BigNum &n1, const BigNum &n2)
{
    add_bn(r.data(), n1.data(), n2.data());
    return r;
}

BigNum add_bn(const BigNum &left, const BigNum &right)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    add_bn(result.data(), left.data(), right.data());
    return result;
}

BigNum &sub_bn(BigNum &r, const BigNum &n1, const BigNum &n2)
{
    sub_bn(r.data(), n1.data(), n2.data());
    return r;
}

BigNum sub_bn(const BigNum &left, const BigNum &right)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    sub_bn(result.data(), left.data(), right.data());
    return result;
}

void add_a_bn(BigNum &result, const BigNum &source)
{
    add_bn(result.data(), result.data(), source.data());
}

void sub_a_bn(BigNum &result, const BigNum &source)
{
    sub_bn(result.data(), result.data(), source.data());
}

BigNumPtr div_bn(BigNumPtr r, BigNumConstPtr n1, BigNumConstPtr n2)
{
    return binary_operation(r, n1, n2, divide);
}

void copy_bn(BigNum &result, const BigNum &source)
{
    copy_bn(result.data(), source.data());
}

BigNum mult_bn_int(const BigNum &source, const U16 factor)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    mult_bn_int(result.data(), source.data(), factor);
    return result;
}

BigNum &mult_bn_int(BigNum &r, const BigNum &source, const U16 factor)
{
    mult_bn_int(r.data(), source.data(), factor);
    return r;
}

BigNum abs_bn(const BigNum &source)
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    copy_bn(result.data(), source.data());
    if (is_bn_neg(result.data()))
    {
        neg_a_bn(result.data());
    }
    return result;
}

BigNum &abs_bn(BigNum &r, const BigNum &source)
{
    abs_bn(r.data(), source.data());
    return r;
}

BigNum &abs_a_bn(BigNum &r)
{
    if (is_bn_neg(r.data()))
    {
        neg_a_bn(r.data());
    }
    return r;
}

BigNum &clear_bn(BigNum &r)
{
    clear_bn(r.data());
    return r;
}

BigNum &max_bn(BigNum &r)
{
    max_bn(r.data());
    return r;
}

bool is_bn_neg(const BigNum &n)
{
    return is_bn_neg(n.data());
}

bool is_bn_zero(const BigNum &n)
{
    return is_bn_zero(n.data());
}

bool is_bn_not_zero(const BigNum &n)
{
    return is_bn_not_zero(n.data());
}

BigNum &neg_bn(BigNum &r, const BigNum &n)
{
    neg_bn(r.data(), n.data());
    return r;
}

BigNum &neg_a_bn(BigNum &r)
{
    neg_a_bn(r.data());
    return r;
}

BigNum &double_bn(BigNum &r, const BigNum &n)
{
    double_bn(r.data(), n.data());
    return r;
}

BigNum &double_a_bn(BigNum &r)
{
    double_a_bn(r.data());
    return r;
}

BigNum &half_bn(BigNum &r, const BigNum &n)
{
    half_bn(r.data(), n.data());
    return r;
}

BigNum &half_a_bn(BigNum &r)
{
    half_a_bn(r.data());
    return r;
}

BigNum &mult_a_bn_int(BigNum &r, const U16 u)
{
    mult_a_bn_int(r.data(), u);
    return r;
}

} // namespace id::math
