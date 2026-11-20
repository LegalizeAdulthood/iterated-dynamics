// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/BigNum.h>

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

class BigNumProduct
{
public:
    BigNumProduct() noexcept = default;
    explicit BigNumProduct(BigArena &arena);

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_storage);
    }

    BigNumPtr data() noexcept
    {
        return m_storage.data();
    }

    BigNumConstPtr data() const noexcept
    {
        return m_storage.data();
    }

    void reset() noexcept
    {
        m_storage.reset();
    }

    BigArena::Allocation release_reduced_storage() &&;

private:
    // g_r_length retained bytes in two slots, including fractional guards.
    BigArena::Allocation m_storage;
};

BigNumProduct::BigNumProduct(BigArena &arena) :
    m_storage(arena.allocate_double())
{
    std::memset(data(), 0, 2 * arena.block_size());
}

BigArena::Allocation BigNumProduct::release_reduced_storage() &&
{
    m_storage.reduce();
    return std::move(m_storage);
}

BigNumProduct &bn_product_scratch()
{
    // First use follows arena owner initialization, so this releases its
    // allocation before the working arena is destroyed.
    static BigNumProduct scratch;
    return scratch;
}

void validate_buffer(BigNumConstPtr buffer, const std::size_t size, const std::size_t required)
{
    if (buffer == nullptr || size < required)
    {
        throw std::invalid_argument("BigNum requires a complete legacy value buffer");
    }
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

void init_bn_product_scratch(BigArena &working)
{
    BigNumProduct &scratch = bn_product_scratch();
    if (!scratch)
    {
        scratch = BigNumProduct{working};
    }
    else
    {
        std::memset(scratch.data(), 0, 2 * working.block_size());
    }
}

bool is_bn_product_scratch(const Byte *source)
{
    const BigNumProduct &scratch = bn_product_scratch();
    return scratch && source == scratch.data();
}

void free_bn_product_scratch()
{
    bn_product_scratch().reset();
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

BigNum &BigNum::operator=(const BigFloat &value)
{
    if (m_storage)
    {
        value.copy_to_legacy_num(data(), encoded_size());
    }
    else
    {
        BigArena &destination = m_storage.arena_ptr() != nullptr ? arena() : bn_arena(BigLifetime::WORKING);
        *this = BigNum{destination, value};
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
    BigNumProduct product{bn_arena(BigLifetime::WORKING)};
    binary_operation(
        product.data(), data(), right.data(), g_bn_tmp_copy1.data(), g_bn_tmp_copy2.data(), unsafe_mult_bn);
    BigNum result;
    result.m_storage = std::move(product).release_reduced_storage();
    return result;
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
    div_bn(result.data(), data(), right.data(), bn_product_scratch().data(), g_bn_tmp2.data(), g_bn_tmp3.data(),
        g_bn_tmp_copy1.data(), g_bn_tmp_copy2.data());
    return result;
}

BigNum BigNum::sqr() const
{
    BigNumProduct product{bn_arena(BigLifetime::WORKING)};
    copy_bn(g_bn_tmp_copy1.data(), data());
    unsafe_square_bn(product.data(), g_bn_tmp_copy1.data());
    BigNum result;
    result.m_storage = std::move(product).release_reduced_storage();
    return result;
}

BigNum BigNum::abs() const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    abs_bn(result.data(), data());
    return result;
}

BigNum BigNum::half() const
{
    BigNum result{bn_arena(BigLifetime::WORKING)};
    half_bn(result.data(), data());
    return result;
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

bool BigNum::is_negative() const
{
    return is_bn_neg(data());
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
    return bn_to_string(*this, digits);
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

BigFloat BigNum::to_big_float(BigArena &arena) const
{
    BigFloat result{arena};
    Byte *const destination = result.data();
    validate_buffer(destination, arena.value_size(), static_cast<std::size_t>(g_bf_length) + 2);
    bn_to_bf(destination, data());
    return result;
}

BigNum sqr(const BigNum &value)
{
    return value.sqr();
}

BigNum &str_to_bn(BigNum &r, const std::string_view text)
{
    str_to_bn(r.data(), text);
    return r;
}

BigNum &int_to_bn(BigNum &r, const long value)
{
    int_to_bn(r.data(), value);
    return r;
}

LDouble bn_to_float(BigNum &value)
{
    return bn_to_float(value.data());
}

std::string bn_to_string(const BigNum &value, const int digits)
{
    return unsafe_bn_to_string(copy_bn(g_bn_tmp_copy2.data(), value.data()), digits);
}

long bn_to_int(const BigNum &n)
{
    return bn_to_int(n.data());
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

bool is_bn_zero(const BigNum &n)
{
    return is_bn_zero(n.data());
}

bool is_bn_not_zero(const BigNum &n)
{
    return is_bn_not_zero(n.data());
}

BigNum &double_bn(BigNum &r, const BigNum &n)
{
    double_bn(r.data(), n.data());
    return r;
}

BigNum &mult_a_bn_int(BigNum &r, const U16 u)
{
    mult_a_bn_int(r.data(), u);
    return r;
}

/********************************************************************/
// bn_hexdump() - for debugging, dumps to stdout

void BigNum::hex_dump() const
{
    bn_hex_dump(data());
}

} // namespace id::math
