// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/MPConversions.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace id::math
{
namespace
{

class TestMPConversions : public testing::Test
{
protected:
    MPReal power(int exponent) const;
    std::vector<Byte> record(std::vector<Byte> bytes, int exponent) const;
    MPReal unpack(std::vector<Byte> bytes, int exponent) const;
    MPReal decode(const std::vector<Byte> &bytes) const;
    std::vector<Byte> pack(const MPReal &value) const;

    int m_length{36};
    const MPPrecision m_precision{200};
};

MPReal TestMPConversions::power(const int exponent) const
{
    return boost::multiprecision::ldexp(m_precision.real("1"), exponent);
}

std::vector<Byte> TestMPConversions::record(std::vector<Byte> bytes, const int exponent) const
{
    bytes.resize(m_length + 2);
    const auto encoded = static_cast<std::uint16_t>(exponent);
    bytes[m_length] = static_cast<Byte>(encoded & 0xff);
    bytes[m_length + 1] = static_cast<Byte>(encoded >> 8);
    return bytes;
}

MPReal TestMPConversions::unpack(std::vector<Byte> bytes, const int exponent) const
{
    return decode(record(std::move(bytes), exponent));
}

MPReal TestMPConversions::decode(const std::vector<Byte> &bytes) const
{
    return MPConversions::from_legacy(bytes.data(), m_length, m_precision);
}

std::vector<Byte> TestMPConversions::pack(const MPReal &value) const
{
    std::vector<Byte> bytes(m_length + 2);
    MPConversions::to_legacy(value, bytes.data(), m_length);
    return bytes;
}

TEST_F(TestMPConversions, roundTripsCanonicalValues)
{
    for (const char *text : {"0", "1.25", "-1.25"})
    {
        SCOPED_TRACE(text);
        const auto expected = m_precision.real(text);
        EXPECT_EQ(expected, decode(pack(expected)));
    }
}

TEST_F(TestMPConversions, decodesSignedMantissaAndSubDoubleCoordinateBits)
{
    std::vector<Byte> bytes(m_length);
    bytes[m_length - 2] = 1;
    bytes[m_length - 12] = 1;
    const auto expected = m_precision.real("1") + power(-80);
    EXPECT_EQ(expected, unpack(bytes, 0));
    EXPECT_EQ(record(bytes, 0), pack(expected));
    unsigned carry = 1;
    for (Byte &byte : bytes)
    {
        carry += static_cast<Byte>(~byte);
        byte = static_cast<Byte>(carry);
        carry >>= 8;
    }
    EXPECT_EQ(-expected, unpack(bytes, 0));
}

TEST_F(TestMPConversions, importsZeroRegardlessOfStoredExponent)
{
    EXPECT_EQ(0, unpack({}, 32767));
    EXPECT_EQ(0, unpack({}, -32768));
}

TEST_F(TestMPConversions, preservesExtremeExponentsAndLowestSubnormalBit)
{
    std::vector<Byte> bytes(m_length);
    bytes[m_length - 2] = 1;
    for (const int exponent : {-32768, 32767})
    {
        SCOPED_TRACE(exponent);
        const auto expected = power(8 * exponent);
        EXPECT_EQ(expected, unpack(bytes, exponent));
        EXPECT_EQ(record(bytes, exponent), pack(expected));
        EXPECT_EQ(-expected, decode(pack(-expected)));
    }
    std::fill(bytes.begin(), bytes.end(), 0);
    bytes[0] = 1;
    const auto expected = power(8 * (-32768 + 2 - m_length));
    EXPECT_EQ(expected, unpack(bytes, -32768));
    EXPECT_EQ(record(bytes, -32768), pack(expected));
}

TEST_F(TestMPConversions, roundsImportsToNearestEvenAtRequestedPrecision)
{
    const MPPrecision low{1};
    auto one = low.real("1");
    const int bits = static_cast<int>(mpfr_get_prec(one.backend().data()));
    const auto half = power(-bits);
    const auto decode_low = [this, &low](const MPReal &value)
    {
        const auto bytes = pack(value);
        return MPConversions::from_legacy(bytes.data(), m_length, low);
    };

    EXPECT_EQ(one, decode_low(m_precision.real("1") + half));
    mpfr_nextabove(one.backend().data());
    mpfr_nextabove(one.backend().data());
    EXPECT_EQ(one, decode_low(m_precision.real("1") + 3 * half));
    mpfr_nextbelow(one.backend().data());
    EXPECT_EQ(one, decode_low(m_precision.real("1") + half + power(-bits - 8)));
}

TEST_F(TestMPConversions, roundsFloatingPointTiesAndCarry)
{
    const auto quantum = power(-8 * (m_length - 2));
    EXPECT_EQ(pack(m_precision.real("1")), pack(m_precision.real("1") + quantum / 2));
    EXPECT_EQ(m_precision.real("1") + 2 * quantum, decode(pack(m_precision.real("1") + 3 * quantum / 2)));
    std::vector<Byte> bytes(m_length);
    bytes[m_length - 2] = 1;
    const auto carried = pack(m_precision.real("256") - quantum / 2);
    EXPECT_EQ(record(bytes, 1), carried);
    EXPECT_EQ(m_precision.real("256"), decode(carried));
    EXPECT_EQ(m_precision.real("-1"), decode(pack(m_precision.real("-1"))));
}

TEST_F(TestMPConversions, handlesFloatingPointUnderflowAndAsymmetricOverflow)
{
    const auto quantum = power(8 * (-32768 + 2 - m_length));
    const auto limit = power(8 * 32768);
    for (const int sign : {-1, 1})
    {
        SCOPED_TRACE(sign);
        EXPECT_EQ(pack(m_precision.real("0")), pack(sign * quantum / 2));
        EXPECT_EQ(sign * 2 * quantum, decode(pack(sign * 3 * quantum / 2)));
        EXPECT_THROW(pack(sign * 256 * limit), PrecisionError);
    }
    EXPECT_THROW(pack(limit), PrecisionError);
    EXPECT_EQ(-limit, decode(pack(-limit)));
    const auto top_quantum = power(8 * (32767 + 2 - m_length));
    EXPECT_THROW(pack(limit - top_quantum / 2), PrecisionError);
    EXPECT_EQ(limit - top_quantum, decode(pack(limit - top_quantum)));
}

TEST_F(TestMPConversions, rejectsNonFiniteAndIncompleteRecords)
{
    for (const char *text : {"nan", "inf", "-inf"})
    {
        SCOPED_TRACE(text);
        EXPECT_THROW(pack(m_precision.real(text)), std::invalid_argument);
    }
    std::vector<Byte> bytes(m_length + 2);
    EXPECT_THROW(MPConversions::from_legacy(nullptr, m_length, m_precision), std::invalid_argument);
    EXPECT_THROW(MPConversions::from_legacy(bytes.data(), 1, m_precision), std::invalid_argument);
    EXPECT_THROW(MPConversions::to_legacy(m_precision.real(), nullptr, m_length), std::invalid_argument);
    EXPECT_THROW(MPConversions::to_legacy(m_precision.real(), bytes.data(), 1), std::invalid_argument);
}

TEST_F(TestMPConversions, canonicalizesSignedZeroWithoutChangingDefaultsOrInput)
{
    const auto defaults = MPReal::default_precision();
    const auto thread_defaults = MPReal::thread_default_precision();
    const auto source = m_precision.real("-1.25");
    const auto original = source;
    const auto bytes = pack(source);
    const auto retained = decode(bytes);

    EXPECT_EQ(pack(m_precision.real("0")), pack(m_precision.real("-0")));
    EXPECT_EQ(original, source);
    EXPECT_EQ(source, retained);
    EXPECT_EQ(bytes, pack(retained));
    EXPECT_EQ(defaults, MPReal::default_precision());
    EXPECT_EQ(thread_defaults, MPReal::thread_default_precision());
    EXPECT_EQ(m_precision.working_digits(), retained.precision());
}

TEST_F(TestMPConversions, preservesAllSignificandBitsAtMaximumStoredPrecision)
{
    m_length = 712;
    const MPPrecision precision{2000};
    std::vector<Byte> bytes(m_length - 1, 0xff);
    bytes.push_back(0);
    bytes = record(std::move(bytes), 0);
    const auto imported = MPConversions::from_legacy(bytes.data(), m_length, precision);
    const auto expected =
        precision.real("256") - boost::multiprecision::ldexp(precision.real("1"), -8 * (m_length - 2));

    EXPECT_EQ(expected, imported);
    EXPECT_EQ(precision.working_digits(), imported.precision());
    EXPECT_EQ(bytes, pack(imported));
}

} // namespace
} // namespace id::math
