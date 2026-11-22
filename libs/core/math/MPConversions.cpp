// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/MPConversions.h>

#include <math/big.h>
#include <math/BigFloat.h>

#include <boost/multiprecision/gmp.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace id::math
{
namespace
{

using Integer = boost::multiprecision::mpz_int;
constexpr int MIN_EXPONENT = std::numeric_limits<std::int16_t>::min();
constexpr int MAX_EXPONENT = std::numeric_limits<std::int16_t>::max();

Integer decode(const Byte *bytes, const int length)
{
    Integer result;
    mpz_import(result.backend().data(), length, -1, 1, 0, 0, bytes);
    if ((bytes[length - 1] & 0x80) != 0)
    {
        result -= Integer{1} << (8 * length);
    }
    return result;
}

std::vector<Byte> encode(Integer value, const int length)
{
    if (value < 0)
    {
        value += Integer{1} << (8 * length);
    }
    std::vector<Byte> bytes(length, 0);
    std::size_t count{};
    mpz_export(bytes.data(), &count, -1, 1, 0, 0, value.backend().data());
    return bytes;
}

MPReal import(const Integer &significand, const mpfr_exp_t scale, const MPPrecision &precision)
{
    MPReal result = precision.real();
    mpfr_set_z_2exp(result.backend().data(), significand.backend().data(), scale, MPFR_RNDN);
    if (!boost::multiprecision::isfinite(result) || (significand != 0 && result == 0))
    {
        throw PrecisionError("Legacy value outside the MPFR exponent range");
    }
    return result;
}

void validate_finite(const MPReal &value)
{
    if (!boost::multiprecision::isfinite(value))
    {
        throw std::invalid_argument("Cannot encode a non-finite legacy number");
    }
}

// Round the exact MPFR dyadic to an integer multiple of 2^scale. Integer
// arithmetic avoids double rounding and never constructs a huge half-quantum.
Integer quantize(const MPReal &value, const mpfr_exp_t scale)
{
    if (value == 0 || mpfr_get_exp(value.backend().data()) < scale)
    {
        return 0;
    }
    Integer result;
    const mpfr_exp_t exponent = mpfr_get_z_2exp(result.backend().data(), value.backend().data());
    if (exponent >= scale)
    {
        mpz_mul_2exp(result.backend().data(), result.backend().data(), exponent - scale);
        return result;
    }
    const bool negative = result < 0;
    mpz_abs(result.backend().data(), result.backend().data());
    const auto shift = static_cast<mp_bitcnt_t>(scale - exponent);
    const bool round = mpz_tstbit(result.backend().data(), shift - 1) != 0;
    const bool sticky = mpz_scan1(result.backend().data(), 0) < shift - 1;
    mpz_fdiv_q_2exp(result.backend().data(), result.backend().data(), shift);
    if (round && (sticky || mpz_odd_p(result.backend().data())))
    {
        ++result;
    }
    if (negative)
    {
        result = -result;
    }
    return result;
}

} // namespace

MPReal MPConversions::from_big_float(const BigFloat &value, const MPPrecision &precision)
{
    if (!value)
    {
        throw std::invalid_argument("Cannot convert a null BigFloat");
    }
    return MPReal{*value.m_value, precision.working_digits()};
}

MPReal MPConversions::from_legacy(const Byte *bytes, const int length, const MPPrecision &precision)
{
    if (bytes == nullptr || length < 2)
    {
        throw std::invalid_argument("Incomplete legacy value buffer");
    }
    const int encoded = bytes[length] + 256 * bytes[length + 1];
    const int exponent = encoded >= 32768 ? encoded - 65536 : encoded;
    return import(decode(bytes, length), 8 * (exponent + 2 - length), precision);
}

BigFloat MPConversions::to_big_float(const MPReal &value)
{
    std::vector<Byte> bytes(static_cast<std::size_t>(g_bf_length) + 2);
    to_legacy(value, bytes.data(), g_bf_length);
    BigFloat result{0};
    BigFloat::unpack_bf(result, bytes.data(), g_bf_length);
    return result;
}

void MPConversions::to_legacy(const MPReal &value, Byte *record, const int length)
{
    if (record == nullptr || length < 2)
    {
        throw std::invalid_argument("Incomplete legacy value buffer");
    }
    validate_finite(value);
    mpfr_exp_t exponent{};
    if (value != 0)
    {
        const mpfr_exp_t binary = mpfr_get_exp(value.backend().data()) - 1;
        exponent = binary / 8 - (binary < 0 && binary % 8 != 0 ? 1 : 0);
    }
    if (exponent > MAX_EXPONENT + 1)
    {
        throw PrecisionError("BigFloat conversion overflow");
    }
    exponent = std::clamp(exponent, mpfr_exp_t{MIN_EXPONENT}, mpfr_exp_t{MAX_EXPONENT});
    Integer significand = quantize(value, 8 * (exponent + 2 - length));
    const Integer bound = Integer{1} << (8 * (length - 1));
    if (significand == bound && exponent < MAX_EXPONENT)
    {
        significand >>= 8;
        ++exponent;
    }
    if (significand < -bound || significand >= bound)
    {
        throw PrecisionError("BigFloat conversion overflow");
    }
    // Negative exact powers need an extra leading 0xff byte in canonical form.
    if (significand < 0 && significand >= -(bound >> 8) && exponent > MIN_EXPONENT)
    {
        significand <<= 8;
        --exponent;
    }
    if (significand == 0)
    {
        exponent = 0;
    }
    const auto bytes = encode(significand, length);
    std::memcpy(record, bytes.data(), bytes.size());
    const auto encoded = static_cast<std::uint16_t>(exponent);
    record[length] = static_cast<Byte>(encoded & 0xff);
    record[length + 1] = static_cast<Byte>(encoded >> 8);
}

} // namespace id::math
