// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigFloat.h>

#include "BigStorage.h"

#include <math/big_init.h>
#include <math/BigNum.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
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

BigFloatPtr binary_operation(BigFloatPtr result, BigFloatConstPtr left, BigFloatConstPtr right,
    BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1.data(), left);
    copy_bf(g_bf_tmp_copy2.data(), right);
    return operation(result, g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
}

BigFloatPtr compound_operation(
    BigFloatPtr result, BigFloatConstPtr right, BigFloatPtr (*operation)(BigFloatPtr, BigFloatPtr))
{
    copy_bf(g_bf_tmp_copy1.data(), right);
    return operation(result, g_bf_tmp_copy1.data());
}

BigFloatPtr multiply(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    multiply_wide(product.data(), n1, n2);
    const BigFloat canonical{std::move(product)};
    copy_bf(r, canonical.data());
    return r;
}

BigFloatPtr square_value(BigFloatPtr r, BigFloatPtr n)
{
    if (is_bf_zero(n))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    square_wide(product.data(), n);
    const BigFloat canonical{std::move(product)};
    copy_bf(r, canonical.data());
    return r;
}

void validate_scalar(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigFloat requires a finite scalar value");
    }
}

BigArena::Allocation native_storage(BigArena &arena, const LDouble value)
{
    validate_scalar(value);
    auto storage = arena.allocate_single();
    std::memset(storage.data(), 0, arena.block_size());
    float_to_bf(storage.data(), value);
    return storage;
}

} // namespace

void reduce_bf_product(Byte *value)
{
    std::memmove(value, value + g_padding, g_bf_length + 2);
}

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

#if defined(CALCULATING_BIG_PI) && !defined(_WIN32)
void bf_hex_dump_snapshot(const Byte *r)
{
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigArena dump_arena{size, 2, reduce_bf_product};
    bf_hex_dump(BigFloat::from_legacy(dump_arena, r, size));
}
#endif

BigFloat::BigFloat() noexcept = default;

BigFloat::BigFloat(const LDouble value) :
    BigFloat(bf_arena(BigLifetime::WORKING), value)
{
}

BigFloat::BigFloat(const std::string_view text) :
    BigFloat(bf_arena(BigLifetime::WORKING), text)
{
}

BigFloat::BigFloat(const BigNum &value) :
    BigFloat(bf_arena(BigLifetime::WORKING), value)
{
}

BigFloat::BigFloat(BigFloatProduct &&value) :
    m_storage(std::move(value).release_reduced_storage())
{
}

BigFloat::BigFloat(BigArena &arena) :
    BigFloat(arena.allocate_single())
{
}

BigFloat::BigFloat(BigArena &arena, const LDouble value) :
    m_storage(native_storage(arena, value))
{
}

BigFloat::BigFloat(BigArena &arena, const std::string_view text) :
    BigFloat(arena)
{
    str_to_bf(*this, text);
}

BigFloat::BigFloat(BigArena &arena, const BigNum &value) :
    BigFloat(arena)
{
    value.copy_to_legacy_float(m_storage.data(), encoded_size());
}

BigFloat::BigFloat(const BigFloat &value) :
    m_storage(value.m_storage.clone())
{
}

BigFloat::BigFloat(BigArena &arena, const BigFloat &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigFloat::BigFloat(BigFloat &&value) noexcept = default;

BigFloat::BigFloat(BigArena::Allocation storage) :
    m_storage(std::move(storage))
{
    std::memset(m_storage.data(), 0, m_storage.blocks() * arena().block_size());
}

BigFloat &BigFloat::operator=(const BigFloat &value)
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
                BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
                m_storage = destination.allocate_single();
            }
            std::memcpy(m_storage.data(), value.m_storage.data(), encoded_size());
        }
    }
    return *this;
}

BigFloat &BigFloat::operator=(BigFloat &&value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else if ((m_storage.arena_ptr() != nullptr ? &arena() : &bf_arena(BigLifetime::WORKING)) == &value.arena())
        {
            m_storage = std::move(value.m_storage);
        }
        else
        {
            *this = static_cast<const BigFloat &>(value);
        }
    }
    return *this;
}

BigFloat &BigFloat::operator=(const LDouble value)
{
    validate_scalar(value);
    if (!m_storage)
    {
        BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
        m_storage = destination.allocate_single();
        std::memset(m_storage.data(), 0, arena().block_size());
    }
    float_to_bf(m_storage.data(), value);
    return *this;
}

BigFloat BigFloat::operator-() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    neg_a_bf(result);
    return result;
}

BigFloat &BigFloat::operator+=(const BigFloat &right)
{
    return add_a_bf(*this, right);
}

BigFloat &BigFloat::operator-=(const BigFloat &right)
{
    return sub_a_bf(*this, right);
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

BigFloat &BigFloat::operator/=(const U16 right)
{
    return div_a_bf_int(*this, right);
}

BigFloat BigFloat::operator+(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    add_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::operator-(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    sub_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::operator*(const BigFloat &right) const
{
    BigFloatProduct result{bf_arena(BigLifetime::WORKING)};
    binary_operation(result.data(), m_storage.data(), right.m_storage.data(), multiply_wide);
    return BigFloat{std::move(result)};
}

BigFloat BigFloat::operator*(const U16 right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    mult_bf_int(result, *this, right);
    return result;
}

BigFloat BigFloat::operator/(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    div_bf(result, *this, right);
    return result;
}

BigFloat BigFloat::abs() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    abs_a_bf(result);
    return result;
}

BigFloat BigFloat::sqr() const
{
    BigFloatProduct result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    square_wide(result.data(), g_bf_tmp_copy1.data());
    return BigFloat{std::move(result)};
}

BigFloat BigFloat::half() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    half_a_bf(result);
    return result;
}

BigFloat BigFloat::log() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    ln_bf(result, *this);
    return result;
}

BigFloat BigFloat::exp() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    exp_bf(result, *this);
    return result;
}

std::pair<BigFloat, BigFloat> BigFloat::sin_cos() const
{
    std::pair<BigFloat, BigFloat> result{
        BigFloat{bf_arena(BigLifetime::WORKING)}, BigFloat{bf_arena(BigLifetime::WORKING)}};
    sin_cos_bf(result.first, result.second, *this);
    return result;
}

BigFloat BigFloat::atan2(const BigFloat &x) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    atan2_bf(result, *this, x);
    return result;
}

BigFloat &clear_bf(BigFloat &r)
{
    clear_bf(r.data());
    return r;
}

BigFloat &copy_bf(BigFloat &r, const BigFloat &n)
{
    copy_bf(r.data(), n.data());
    return r;
}

BigFloat &max_bf(BigFloat &r)
{
    max_bf(r.data());
    return r;
}

int sign_bf(const BigFloat &n)
{
    return sign_bf(n.data());
}

bool is_bf_zero(const BigFloat &n)
{
    return is_bf_zero(n.data());
}

bool is_bf_neg(const BigFloat &n)
{
    return is_bf_neg(n.data());
}

bool is_bf_not_zero(const BigFloat &n)
{
    return is_bf_not_zero(n.data());
}

int cmp_bf(const BigFloat &n1, const BigFloat &n2)
{
    return cmp_bf(n1.data(), n2.data());
}

BigFloat &neg_bf(BigFloat &r, const BigFloat &n)
{
    neg_bf(r.data(), n.data());
    return r;
}

BigFloat &neg_a_bf(BigFloat &r)
{
    neg_a_bf(r.data());
    return r;
}

BigFloat &abs_bf(BigFloat &r, const BigFloat &n)
{
    abs_bf(r.data(), n.data());
    return r;
}

BigFloat &abs_a_bf(BigFloat &r)
{
    abs_a_bf(r.data());
    return r;
}

BigFloat &add_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), unsafe_add_bf);
    return r;
}

BigFloat &add_a_bf(BigFloat &r, const BigFloat &n)
{
    compound_operation(r.data(), n.data(), unsafe_add_a_bf);
    return r;
}

BigFloat &sub_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), unsafe_sub_bf);
    return r;
}

BigFloat &sub_a_bf(BigFloat &r, const BigFloat &n)
{
    compound_operation(r.data(), n.data(), unsafe_sub_a_bf);
    return r;
}

BigFloat &mult_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    binary_operation(r.data(), n1.data(), n2.data(), multiply);
    return r;
}

BigFloat &square_bf(BigFloat &r, const BigFloat &n)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    square_value(r.data(), g_bf_tmp_copy1.data());
    return r;
}

BigFloat &mult_bf_int(BigFloat &r, const BigFloat &n, const U16 u)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    unsafe_mult_bf_int(r.data(), g_bf_tmp_copy1.data(), u);
    return r;
}

BigFloat &mult_a_bf_int(BigFloat &r, const U16 u)
{
    mult_a_bf_int(r.data(), u);
    return r;
}

BigFloat &double_bf(BigFloat &r, const BigFloat &n)
{
    double_bf(r.data(), n.data());
    return r;
}

BigFloat &double_a_bf(BigFloat &r)
{
    double_a_bf(r.data());
    return r;
}

BigFloat &half_bf(BigFloat &r, const BigFloat &n)
{
    half_bf(r.data(), n.data());
    return r;
}

BigFloat &half_a_bf(BigFloat &r)
{
    half_a_bf(r.data());
    return r;
}

BigFloat &unsafe_inv_bf(BigFloat &r, BigFloat &n)
{
    unsafe_inv_bf(r.data(), n.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return r;
}

BigFloat &div_bf(BigFloat &r, const BigFloat &n1, const BigFloat &n2)
{
    copy_bf(g_bf_tmp_copy1.data(), n1.data());
    copy_bf(g_bf_tmp_copy2.data(), n2.data());
    unsafe_div_bf(r.data(), g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return r;
}

BigFloat &unsafe_div_bf_int(BigFloat &r, BigFloat &n, const U16 u)
{
    unsafe_div_bf_int(r.data(), n.data(), u);
    return r;
}

BigFloat &div_a_bf_int(BigFloat &r, const U16 u)
{
    div_a_bf_int(r.data(), u);
    return r;
}

BigFloat &exp_bf(BigFloat &r, const BigFloat &n)
{
    exp_bf(r.data(), n.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data());
    return r;
}

BigFloat &ln_bf(BigFloat &r, const BigFloat &n)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    unsafe_ln_bf(r.data(), g_bf_tmp_copy1.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(),
        g_bf_tmp4.data(), g_bf_tmp5.data(), g_bf_tmp6.data());
    return r;
}

BigFloat &sin_cos_bf(BigFloat &s, BigFloat &c, const BigFloat &n)
{
    copy_bf(g_bf_tmp_copy1.data(), n.data());
    unsafe_sin_cos_bf(s.data(), c.data(), g_bf_tmp_copy1.data(), g_bf_pi.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return s;
}

BigFloat &atan2_bf(BigFloat &r, const BigFloat &ny, const BigFloat &nx)
{
    copy_bf(g_bf_tmp_copy1.data(), ny.data());
    copy_bf(g_bf_tmp_copy2.data(), nx.data());
    unsafe_atan2_bf(r.data(), g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data(), g_bf_pi.data(), g_bf_tmp1.data(),
        g_bf_tmp2.data(), g_bf_tmp3.data(), g_bf_tmp4.data(), g_bf_tmp5.data(), g_bf_tmp6.data(), g_bf_tmp_copy1.data(),
        g_bf_tmp_copy2.data());
    return r;
}

BigFloat &str_to_bf(BigFloat &r, const std::string_view text)
{
    str_to_bf(r.data(), text, g_bf_tmp1.data());
    return r;
}

BigFloat &int_to_bf(BigFloat &r, const long value)
{
    int_to_bf(r.data(), value);
    return r;
}

BigFloat &float_to_bf(BigFloat &r, const LDouble value)
{
    float_to_bf(r.data(), value);
    return r;
}

LDouble bf_to_float(BigFloat &value)
{
    return bf_to_float(value.data());
}

const Byte *unpack_bf(BigFloat &destination, const Byte *source, const int stored_length)
{
    return unpack_bf(destination.data(), source, stored_length);
}

/**********************************************************************/
std::string bf_to_string(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string(g_bf_tmp_copy1.data(), dec);
}

/**********************************************************************/
std::string bf_to_string_e(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string_e(g_bf_tmp_copy1.data(), dec);
}

/**********************************************************************/
std::string bf_to_string_f(const BigFloat &r, const int dec)
{
    copy_bf(g_bf_tmp_copy1.data(), r.data());
    return unsafe_bf_to_string_f(g_bf_tmp_copy1.data(), dec);
}

BigFloat abs(const BigFloat &value)
{
    return value.abs();
}

BigFloat sqr(const BigFloat &value)
{
    return value.sqr();
}

BigFloat log(const BigFloat &value)
{
    return value.log();
}

BigFloat exp(const BigFloat &value)
{
    return value.exp();
}

std::pair<BigFloat, BigFloat> sin_cos(const BigFloat &value)
{
    return value.sin_cos();
}

BigFloat atan2(const BigFloat &y, const BigFloat &x)
{
    return y.atan2(x);
}

bool BigFloat::operator==(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) == 0;
}

bool BigFloat::operator!=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) != 0;
}

bool BigFloat::operator<(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) < 0;
}

bool BigFloat::operator<=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) <= 0;
}

bool BigFloat::operator>(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) > 0;
}

bool BigFloat::operator>=(const BigFloat &right) const
{
    return cmp_bf(m_storage.data(), right.m_storage.data()) >= 0;
}

LDouble BigFloat::to_float() const
{
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    return bf_to_float(g_bf_tmp_copy1.data());
}

std::string BigFloat::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigFloat formatting requires a nonnegative digit count");
    }
    const int default_digits = g_decimals > 0 ? g_decimals : static_cast<int>((g_bf_length - 2) * LOG10_256);
    return bf_to_string(*this, digits == 0 ? default_digits : digits);
}

BigFloat BigFloat::from_legacy(BigFloatConstPtr source, const std::size_t size)
{
    return from_legacy(bf_arena(BigLifetime::WORKING), source, size);
}

BigFloat BigFloat::from_legacy(BigArena &arena, BigFloatConstPtr source, const std::size_t size)
{
    BigFloat result{arena};
    validate_buffer(source, size, result.encoded_size());
    std::copy_n(source, result.encoded_size(), result.m_storage.data());
    return result;
}

void BigFloat::copy_to_legacy(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, encoded_size());
    std::copy_n(m_storage.data(), encoded_size(), destination);
}

void BigFloat::copy_to_legacy_num(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bn_length));
    bf_to_bn(destination, m_storage.data());
}

BigFloatProduct::BigFloatProduct() noexcept = default;

BigFloatProduct::BigFloatProduct(BigArena &arena) :
    m_storage(arena.allocate_double())
{
    std::memset(data(), 0, 2 * arena.block_size());
}

BigFloatProduct::BigFloatProduct(const BigFloatProduct &value) :
    m_storage(value.m_storage.clone())
{
}

BigFloatProduct::BigFloatProduct(BigArena &arena, const BigFloatProduct &value) :
    m_storage(value.m_storage.clone(arena))
{
}

BigFloatProduct::BigFloatProduct(BigFloatProduct &&value) noexcept = default;

BigFloatProduct &BigFloatProduct::operator=(const BigFloatProduct &value)
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
                BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bf_arena(BigLifetime::WORKING);
                m_storage = destination.allocate_double();
            }
            std::memcpy(data(), value.data(), g_r_bf_length + 2);
        }
    }
    return *this;
}

BigFloatProduct &BigFloatProduct::operator=(BigFloatProduct &&value)
{
    if (this != &value)
    {
        if (!value.m_storage)
        {
            m_storage.reset();
        }
        else if ((m_storage.arena_ptr() != nullptr ? &arena() : &bf_arena(BigLifetime::WORKING)) == &value.arena())
        {
            m_storage = std::move(value.m_storage);
        }
        else
        {
            *this = static_cast<const BigFloatProduct &>(value);
        }
    }
    return *this;
}

BigArena::Allocation BigFloatProduct::release_reduced_storage() &&
{
    BigArena::Allocation result = std::move(m_storage);
    if (result)
    {
        result.reduce();
    }
    return result;
}

Byte *unsafe_square_bf(Byte *r, Byte *n)
{
    return square_value(r, n);
}

Byte *unsafe_mult_bf(Byte *r, Byte *n1, Byte *n2)
{
    return multiply(r, n1, n2);
}

} // namespace id::math
