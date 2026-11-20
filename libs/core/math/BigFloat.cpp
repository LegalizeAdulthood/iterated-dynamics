// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigFloat.h>

#include <math/big.h>
#include <math/big_init.h>
#include <math/BigNum.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace id::misc;

namespace id::math
{
namespace
{

class BigFloatProduct
{
public:
    explicit BigFloatProduct(BigArena &arena);

    Byte *data() noexcept
    {
        return m_storage.data();
    }

    BigArena::Allocation release_reduced_storage() &&;

private:
    // g_r_bf_length mantissa bytes and the exponent in two slots.
    BigArena::Allocation m_storage;
};

BigFloatProduct::BigFloatProduct(BigArena &arena) :
    m_storage(arena.allocate_double())
{
    std::memset(data(), 0, 2 * arena.block_size());
}

BigArena::Allocation BigFloatProduct::release_reduced_storage() &&
{
    m_storage.reduce();
    return std::move(m_storage);
}

Byte *binary_operation(Byte *result, const Byte *left, const Byte *right, Byte *(*operation)(Byte *, Byte *, Byte *) )
{
    copy_bf(g_bf_tmp_copy1.data(), left);
    copy_bf(g_bf_tmp_copy2.data(), right);
    return operation(result, g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
}

Byte *compound_operation(Byte *result, const Byte *right, Byte *(*operation)(Byte *, Byte *))
{
    copy_bf(g_bf_tmp_copy1.data(), right);
    return operation(result, g_bf_tmp_copy1.data());
}

Byte *multiply(Byte *r, Byte *n1, Byte *n2)
{
    if (is_bf_zero(n1) || is_bf_zero(n2))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    multiply_wide(product.data(), n1, n2);
    const BigArena::Allocation canonical = std::move(product).release_reduced_storage();
    copy_bf(r, canonical.data());
    return r;
}

Byte *square_value(Byte *r, Byte *n)
{
    if (is_bf_zero(n))
    {
        return clear_bf(r);
    }
    BigFloatProduct product{bf_arena(BigLifetime::TEMPORARY)};
    square_wide(product.data(), n);
    const BigArena::Allocation canonical = std::move(product).release_reduced_storage();
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

std::string format_bf(
    const Byte *value, const int dec, std::string (*format)(Byte *source, int digits, Byte *decimal))
{
    Byte *const scratch = g_bf_tmp_copy1.data();
    copy_bf(scratch, value);
    if (bf_to_float(scratch) == 0.0)
    {
        return "0.0";
    }

    std::vector<Byte> decimal(static_cast<std::size_t>(dec == 0 ? g_decimals : dec) + 4);
    return format(scratch, dec, decimal.data());
}

} // namespace

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
    str_to_bf(m_storage.data(), text, g_bf_tmp1.data());
}

BigFloat::BigFloat(BigArena &arena, const BigNum &value) :
    BigFloat(value.to_big_float(arena))
{
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

void BigFloat::clear()
{
    std::memset(m_storage.data(), 0, m_storage.blocks() * arena().block_size());
}

void BigFloat::load_pi()
{
    copy_bf_pi(m_storage.data());
}

BigFloat BigFloat::operator-() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    neg_a_bf(result.m_storage.data());
    return result;
}

BigFloat &BigFloat::operator+=(const BigFloat &right)
{
    compound_operation(m_storage.data(), right.m_storage.data(), unsafe_add_a_bf);
    return *this;
}

BigFloat &BigFloat::operator-=(const BigFloat &right)
{
    compound_operation(m_storage.data(), right.m_storage.data(), unsafe_sub_a_bf);
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

BigFloat &BigFloat::operator/=(const U16 right)
{
    div_a_bf_int(m_storage.data(), right);
    return *this;
}

BigFloat BigFloat::operator+(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), unsafe_add_bf);
    return result;
}

BigFloat BigFloat::operator-(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    binary_operation(result.m_storage.data(), m_storage.data(), right.m_storage.data(), unsafe_sub_bf);
    return result;
}

BigFloat BigFloat::operator*(const BigFloat &right) const
{
    BigFloatProduct product{bf_arena(BigLifetime::WORKING)};
    binary_operation(product.data(), m_storage.data(), right.m_storage.data(), multiply_wide);
    BigFloat result;
    result.m_storage = std::move(product).release_reduced_storage();
    return result;
}

BigFloat BigFloat::operator*(const U16 right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    unsafe_mult_bf_int(result.m_storage.data(), g_bf_tmp_copy1.data(), right);
    return result;
}

BigFloat BigFloat::operator/(const BigFloat &right) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    copy_bf(g_bf_tmp_copy2.data(), right.m_storage.data());
    unsafe_div_bf(
        result.m_storage.data(), g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return result;
}

BigFloat BigFloat::abs() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    abs_a_bf(result.m_storage.data());
    return result;
}

BigFloat BigFloat::sqr() const
{
    BigFloatProduct product{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    square_wide(product.data(), g_bf_tmp_copy1.data());
    BigFloat result;
    result.m_storage = std::move(product).release_reduced_storage();
    return result;
}

BigFloat BigFloat::half() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING), *this};
    half_a_bf(result.m_storage.data());
    return result;
}

BigFloat BigFloat::log() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    unsafe_ln_bf(result.m_storage.data(), g_bf_tmp_copy1.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(),
        g_bf_tmp4.data(), g_bf_tmp5.data(), g_bf_tmp6.data());
    return result;
}

BigFloat BigFloat::exp() const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    exp_bf(result.m_storage.data(), m_storage.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data());
    return result;
}

std::pair<BigFloat, BigFloat> BigFloat::sin_cos() const
{
    std::pair<BigFloat, BigFloat> result{
        BigFloat{bf_arena(BigLifetime::WORKING)}, BigFloat{bf_arena(BigLifetime::WORKING)}};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    unsafe_sin_cos_bf(result.first.m_storage.data(), result.second.m_storage.data(), g_bf_tmp_copy1.data(),
        g_bf_pi.data(), g_bf_tmp1.data(), g_bf_tmp2.data());
    return result;
}

BigFloat BigFloat::atan2(const BigFloat &x) const
{
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    copy_bf(g_bf_tmp_copy1.data(), m_storage.data());
    copy_bf(g_bf_tmp_copy2.data(), x.m_storage.data());
    unsafe_atan2_bf(result.m_storage.data(), g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data(), g_bf_pi.data(),
        g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(), g_bf_tmp4.data(), g_bf_tmp5.data(), g_bf_tmp6.data(),
        g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
    return result;
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

int BigFloat::sign() const
{
    return sign_bf(m_storage.data());
}

bool BigFloat::is_zero() const
{
    return is_bf_zero(m_storage.data());
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
    return format_bf(data(), digits == 0 ? default_digits : digits, unsafe_bf_to_string);
}

std::string BigFloat::to_scientific_string(const int digits) const
{
    return format_bf(data(), digits, unsafe_bf_to_string_e);
}

// Debugging aid that dumps the mantissa and exponent to stdout.
void BigFloat::hex_dump() const
{
    bf_hex_dump_snapshot(m_storage.data());
}

BigFloat BigFloat::convert_precision(BigArena &arena, const int new_length, const int old_length) const
{
    BigFloat result{arena};
    convert_bf(result.m_storage.data(), m_storage.data(), new_length, old_length);
    return result;
}

void BigFloat::pack_bf(Byte *record, const std::size_t length) const
{
    const std::size_t size = encoded_size();
    if (record == nullptr || length < size)
    {
        throw std::invalid_argument("BigFloat requires a complete legacy value buffer");
    }
    std::copy_n(data(), size, record);
}

const Byte *BigFloat::unpack_bf(BigFloat &destination, const Byte *source, const int stored_length)
{
    return id::math::unpack_bf(destination.data(), source, stored_length);
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
