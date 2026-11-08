// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

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

} // namespace id::math
