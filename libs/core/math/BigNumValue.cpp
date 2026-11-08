// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigNum.h>

#include <math/BigFloat.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

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

} // namespace

BigNum::BigNum() :
    m_storage(static_cast<std::size_t>(g_bn_length))
{
}

BigNum::BigNum(const LDouble value)
{
    *this = value;
}

BigNum::BigNum(const std::string_view text) :
    BigNum()
{
    if (!text.empty())
    {
        str_to_bn(m_storage.data(), text);
    }
}

BigNum::BigNum(const BigFloat &value) :
    BigNum()
{
    value.copy_to_legacy_num(m_storage.data(), m_storage.size());
}

BigNum &BigNum::operator=(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigNum requires a finite scalar value");
    }
    m_storage.resize(static_cast<std::size_t>(g_bn_length));
    float_to_bn(m_storage.data(), value);
    return *this;
}

std::string BigNum::to_string(const int digits) const
{
    if (digits < 0)
    {
        throw std::invalid_argument("BigNum formatting requires a nonnegative digit count");
    }
    return bn_to_string(m_storage.data(), digits);
}

BigNum BigNum::from_legacy(BigNumConstPtr source, const std::size_t size)
{
    BigNum result;
    validate_buffer(source, size, result.m_storage.size());
    std::copy_n(source, result.m_storage.size(), result.m_storage.data());
    return result;
}

void BigNum::copy_to_legacy(BigNumPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, m_storage.size());
    std::copy(m_storage.begin(), m_storage.end(), destination);
}

void BigNum::copy_to_legacy_float(BigFloatPtr destination, const std::size_t size) const
{
    validate_buffer(destination, size, static_cast<std::size_t>(g_bf_length) + 2);
    bn_to_bf(destination, m_storage.data());
}

} // namespace id::math
