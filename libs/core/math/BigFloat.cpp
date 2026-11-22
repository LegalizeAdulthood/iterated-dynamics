// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>

#include <math/big.h>
#include <math/big_init.h>
#include <math/MPConversions.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace id::math
{
namespace
{

void validate_scalar(const LDouble value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("BigFloat requires a finite scalar value");
    }
}

bool digit(const char value)
{
    return value >= '0' && value <= '9';
}

// Preserve the existing permissive decimal input syntax without byte math.
std::string decimal_input(std::string_view text)
{
    bool negative{};
    if (!text.empty() && (text.front() == '+' || text.front() == '-'))
    {
        negative = text.front() == '-';
        text.remove_prefix(1);
    }
    int exponent{};
    const auto e = text.find_first_of("eE");
    if (e != std::string_view::npos)
    {
        auto tail = text.substr(e + 1);
        if (!tail.empty() && tail.front() == '+')
        {
            tail.remove_prefix(1);
        }
        std::from_chars(tail.data(), tail.data() + tail.size(), exponent);
        text = text.substr(0, e);
    }
    const std::size_t end = text.size();
    std::size_t start = end;
    while (start > 0 && digit(text[start - 1]))
    {
        --start;
    }
    if (text.find('.') != std::string_view::npos && start > 0 && text[start - 1] == '.')
    {
        --start;
        while (start > 0 && digit(text[start - 1]))
        {
            --start;
        }
    }
    if (start == end || text.substr(start) == ".")
    {
        return "0";
    }
    std::string result{negative ? "-" : ""};
    result += text.substr(start);
    result += 'e';
    result += std::to_string(exponent);
    return result;
}

void legacy_limit(MPReal &value, const bool negative)
{
    // Preserve the finite error sentinel returned by legacy division and log.
    value = boost::multiprecision::ldexp(
        MPReal{1, value.precision()}, 8 * (std::numeric_limits<LDouble>::max_exponent / 8));
    if (negative)
    {
        value = -value;
    }
}

std::string format_value(const MPReal &value, const int requested, const bool scientific)
{
    if (requested < 0)
    {
        throw std::invalid_argument("BigFloat formatting requires a nonnegative digit count");
    }
    if (value.convert_to<LDouble>() == 0)
    {
        return "0.0";
    }
    if (!boost::multiprecision::isfinite(value))
    {
        return value.str();
    }
    const int digits = requested == 0
        ? std::max(1, static_cast<int>(value.precision()) - static_cast<int>(MPPrecision::GUARD_DIGITS))
        : requested;
    std::vector<char> buffer(static_cast<std::size_t>(digits) + 3);
    mpfr_exp_t exponent{};
    mpfr_get_str(buffer.data(), &exponent, 10, digits, value.backend().data(), MPFR_RNDZ);
    const mpfr_exp_t original_exponent = exponent;
    mpfr_get_str(buffer.data(), &exponent, 10, digits, value.backend().data(), MPFR_RNDN);
    if (exponent != original_exponent)
    {
        // Retain the existing decimal carry truncation convention.
        mpfr_get_str(buffer.data(), &exponent, 10, digits, value.backend().data(), MPFR_RNDZ);
    }
    std::string mantissa{buffer.data()};
    const bool negative = mantissa.front() == '-';
    if (negative)
    {
        mantissa.erase(0, 1);
    }
    std::string result{negative ? "-" : ""};
    if (scientific || exponent <= -3 || exponent > 6)
    {
        result += mantissa.front();
        result += '.';
        result += mantissa.substr(1);
    }
    else if (exponent <= 0)
    {
        result += "0.";
        result.append(static_cast<std::size_t>(-exponent), '0');
        result += mantissa;
    }
    else
    {
        mantissa.resize(std::max(mantissa.size(), static_cast<std::size_t>(exponent)), '0');
        result += mantissa.substr(0, static_cast<std::size_t>(exponent));
        result += '.';
        result += mantissa.substr(static_cast<std::size_t>(exponent));
    }
    while (result.back() == '0')
    {
        result.pop_back();
    }
    if (result.back() == '.')
    {
        result += '0';
    }
    if (scientific || exponent <= -3 || exponent > 6)
    {
        result += 'e';
        result += std::to_string(exponent - 1);
    }
    return result;
}

} // namespace

BigFloat::BigFloat() noexcept = default;

BigFloat BigFloat::from_value(MPReal value)
{
    BigFloat result;
    result.m_value.emplace(std::move(value));
    if (result.is_zero())
    {
        result.clear();
    }
    return result;
}

BigFloat::BigFloat(const LDouble value) :
    m_value(bf_precision().real())
{
    *this = value;
}

BigFloat::BigFloat(const std::string_view text) :
    m_value(bf_precision().real(decimal_input(text)))
{
    if (is_zero())
    {
        clear();
    }
}

BigFloat::BigFloat(const BigFloat &value) = default;

BigFloat::BigFloat(BigFloat &&value) noexcept :
    m_value(std::move(value.m_value))
{
    value.m_value.reset();
}

BigFloat &BigFloat::operator=(const BigFloat &value) = default;

BigFloat &BigFloat::operator=(BigFloat &&value)
{
    if (this != &value)
    {
        m_value = std::move(value.m_value);
        value.m_value.reset();
    }
    return *this;
}

BigFloat &BigFloat::operator=(const LDouble value)
{
    validate_scalar(value);
    if (!m_value)
    {
        m_value.emplace(bf_precision().real());
    }
    *m_value = value;
    if (value == 0)
    {
        clear();
    }
    return *this;
}

void BigFloat::clear()
{
    *m_value = 0;
}

void BigFloat::load_pi()
{
    *m_value = boost::multiprecision::acos(MPReal{-1, m_value->precision()});
}

BigFloat BigFloat::operator-() const
{
    BigFloat result{*this};
    *result.m_value = -*m_value;
    if (result.is_zero())
    {
        result.clear();
    }
    return result;
}

BigFloat &BigFloat::operator+=(const BigFloat &right)
{
    *m_value += *right.m_value;
    return *this;
}

BigFloat &BigFloat::operator-=(const BigFloat &right)
{
    *m_value -= *right.m_value;
    return *this;
}

BigFloat &BigFloat::operator*=(const BigFloat &right)
{
    *m_value *= *right.m_value;
    if (is_zero())
    {
        clear();
    }
    return *this;
}

BigFloat &BigFloat::operator/=(const BigFloat &right)
{
    if (is_zero())
    {
        clear();
    }
    else if (right.is_zero())
    {
        legacy_limit(*m_value, false);
    }
    else
    {
        *m_value /= *right.m_value;
    }
    return *this;
}

BigFloat &BigFloat::operator/=(const misc::U16 right)
{
    if (right == 0)
    {
        legacy_limit(*m_value, sign() < 0);
    }
    else
    {
        *m_value /= right;
    }
    return *this;
}

BigFloat BigFloat::operator+(const BigFloat &right) const
{
    BigFloat result{*this};
    result += right;
    return result;
}

BigFloat BigFloat::operator-(const BigFloat &right) const
{
    BigFloat result{*this};
    result -= right;
    return result;
}

BigFloat BigFloat::operator*(const BigFloat &right) const
{
    BigFloat result{*this};
    result *= right;
    return result;
}

BigFloat BigFloat::operator*(const misc::U16 right) const
{
    BigFloat result{*this};
    *result.m_value *= right;
    if (result.is_zero())
    {
        result.clear();
    }
    return result;
}

BigFloat BigFloat::operator/(const BigFloat &right) const
{
    BigFloat result{*this};
    result /= right;
    return result;
}

BigFloat BigFloat::abs() const
{
    BigFloat result{*this};
    *result.m_value = boost::multiprecision::abs(*m_value);
    return result;
}

BigFloat BigFloat::sqr() const
{
    BigFloat result{*this};
    *result.m_value = *m_value * *m_value;
    return result;
}

BigFloat BigFloat::half() const
{
    BigFloat result{*this};
    result /= misc::U16{2};
    return result;
}

BigFloat BigFloat::log() const
{
    BigFloat result{*this};
    if (sign() <= 0)
    {
        legacy_limit(*result.m_value, true);
    }
    else
    {
        *result.m_value = boost::multiprecision::log(*m_value);
    }
    return result;
}

BigFloat BigFloat::exp() const
{
    BigFloat result{*this};
    *result.m_value = boost::multiprecision::exp(*m_value);
    return result;
}

std::pair<BigFloat, BigFloat> BigFloat::sin_cos() const
{
    std::pair<BigFloat, BigFloat> result{BigFloat{*this}, BigFloat{*this}};
    // Boost has no combined operation; share MPFR's argument reduction.
    mpfr_sin_cos(result.first.m_value->backend().data(), result.second.m_value->backend().data(),
        m_value->backend().data(), MPFR_RNDN);
    return result;
}

BigFloat BigFloat::atan2(const BigFloat &x) const
{
    BigFloat result{*this};
    *result.m_value = boost::multiprecision::atan2(*m_value, *x.m_value);
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
    return *m_value == *right.m_value;
}

bool BigFloat::operator!=(const BigFloat &right) const
{
    return *m_value != *right.m_value;
}

bool BigFloat::operator<(const BigFloat &right) const
{
    return *m_value < *right.m_value;
}

bool BigFloat::operator<=(const BigFloat &right) const
{
    return *m_value <= *right.m_value;
}

bool BigFloat::operator>(const BigFloat &right) const
{
    return *m_value > *right.m_value;
}

bool BigFloat::operator>=(const BigFloat &right) const
{
    return *m_value >= *right.m_value;
}

int BigFloat::sign() const
{
    return m_value->sign();
}

bool BigFloat::is_zero() const
{
    return *m_value == 0;
}

LDouble BigFloat::to_float() const
{
    return m_value->convert_to<LDouble>();
}

std::string BigFloat::to_string(const int digits) const
{
    return format_value(*m_value, digits == 0 && g_decimals > 0 ? g_decimals : digits, false);
}

std::string BigFloat::to_scientific_string(const int digits) const
{
    return format_value(*m_value, digits == 0 && g_decimals > 0 ? g_decimals : digits, true);
}

void BigFloat::hex_dump() const
{
    std::vector<Byte> record(static_cast<std::size_t>(g_bf_length + 2));
    pack_bf(record.data(), record.size());
    bf_hex_dump_snapshot(record.data());
}

void BigFloat::set_precision(const MPPrecision &precision)
{
    if (m_value)
    {
        m_value->precision(precision.working_digits());
    }
}

void BigFloat::pack_bf(Byte *record, const std::size_t length) const
{
    if (record == nullptr || g_bf_length <= 0 || length < static_cast<std::size_t>(g_bf_length + 2))
    {
        throw std::invalid_argument("BigFloat requires a complete legacy value buffer");
    }
    MPConversions::to_legacy(*m_value, record, g_bf_length);
}

const Byte *BigFloat::unpack_bf(BigFloat &destination, const Byte *source, const int stored_length)
{
    const int length = g_bf_length;
    const int retained = std::min(length, stored_length);
    std::vector<Byte> bytes(static_cast<std::size_t>(length) + 2, 0);
    std::copy_n(source + stored_length - retained, retained, bytes.data() + length - retained);
    std::copy_n(source + stored_length, 2, bytes.data() + length);
    const MPPrecision precision{std::max(1, static_cast<int>(std::ceil((length - 1) * LOG10_256)))};
    destination.m_value = MPConversions::from_legacy(bytes.data(), length, precision);
    return source + stored_length + 2;
}

} // namespace id::math
