// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/MPConversions.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::math
{
namespace
{

class TestMPConversions : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    MPReal power(int exponent) const;
    BigFloat unpack(std::vector<Byte> bytes, int exponent) const;
    std::vector<Byte> pack(const BigFloat &value) const;

    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test, Bailout::MOD};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::MANDEL};
    const MPPrecision m_precision{200};
};

void TestMPConversions::SetUp()
{
    init_bf_length(32);
}

void TestMPConversions::TearDown()
{
    free_bf_vars();
}

MPReal TestMPConversions::power(const int exponent) const
{
    MPReal result = m_precision.real("1");
    mpfr_mul_2si(result.backend().data(), result.backend().data(), exponent, MPFR_RNDN);
    return result;
}

BigFloat TestMPConversions::unpack(std::vector<Byte> bytes, const int exponent) const
{
    bytes.resize(g_bf_length + 2);
    const auto encoded = static_cast<std::uint16_t>(exponent);
    bytes[g_bf_length] = static_cast<Byte>(encoded & 0xff);
    bytes[g_bf_length + 1] = static_cast<Byte>(encoded >> 8);
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    BigFloat::unpack_bf(result, bytes.data(), g_bf_length);
    return result;
}

std::vector<Byte> TestMPConversions::pack(const BigFloat &value) const
{
    std::vector<Byte> result(g_bf_length + 2);
    value.pack_bf(result.data(), result.size());
    return result;
}

TEST_F(TestMPConversions, importsLegacyValuesAndExportsToSelectedArenas)
{
    const MPPrecision low{1};
    BigArena bf{static_cast<std::size_t>(g_bf_length + 2), 4, reduce_bf_product};
    for (const char *text : {"0", "1.25", "-1.25"})
    {
        SCOPED_TRACE(text);
        const auto expected = low.real(text);
        EXPECT_EQ(expected, MPConversions::from_big_float(BigFloat{text}, m_precision));
        const auto floating = MPConversions::to_big_float(expected, bf);
        EXPECT_EQ(&bf, &floating.arena());
        EXPECT_EQ(pack(BigFloat{text}), pack(floating));
    }
}

TEST_F(TestMPConversions, decodesSignedMantissaAndSubDoubleCoordinateBits)
{
    std::vector<Byte> bytes(g_bf_length);
    bytes[g_bf_length - 2] = 1;
    bytes[g_bf_length - 12] = 1;
    const auto positive = unpack(bytes, 0);
    const auto expected = m_precision.real("1") + power(-80);
    EXPECT_EQ(expected, MPConversions::from_big_float(positive, m_precision));
    EXPECT_EQ(pack(positive), pack(MPConversions::to_big_float(expected, positive.arena())));
    // Two's complement of the independently assembled positive mantissa.
    unsigned carry = 1;
    for (Byte &byte : bytes)
    {
        carry += static_cast<Byte>(~byte);
        byte = static_cast<Byte>(carry);
        carry >>= 8;
    }
    EXPECT_EQ(-expected, MPConversions::from_big_float(unpack(bytes, 0), m_precision));
}

TEST_F(TestMPConversions, importsZeroRegardlessOfStoredExponent)
{
    EXPECT_EQ(0, MPConversions::from_big_float(unpack({}, 32767), m_precision));
    EXPECT_EQ(0, MPConversions::from_big_float(unpack({}, -32768), m_precision));
}

TEST_F(TestMPConversions, preservesExtremeExponentsAndLowestSubnormalBit)
{
    std::vector<Byte> bytes(g_bf_length);
    bytes[g_bf_length - 2] = 1;
    for (const int exponent : {-32768, 32767})
    {
        SCOPED_TRACE(exponent);
        const auto source = unpack(bytes, exponent);
        const auto expected = power(8 * exponent);
        EXPECT_EQ(expected, MPConversions::from_big_float(source, m_precision));
        EXPECT_EQ(pack(source), pack(MPConversions::to_big_float(expected, source.arena())));
        const auto negative = -expected;
        EXPECT_EQ(negative,
            MPConversions::from_big_float(MPConversions::to_big_float(negative, source.arena()), m_precision));
    }
    std::fill(bytes.begin(), bytes.end(), 0);
    bytes[0] = 1;
    const auto source = unpack(bytes, -32768);
    const auto expected = power(8 * (-32768 + 2 - g_bf_length));
    EXPECT_EQ(expected, MPConversions::from_big_float(source, m_precision));
    EXPECT_EQ(pack(source), pack(MPConversions::to_big_float(expected, source.arena())));
}

TEST_F(TestMPConversions, roundsImportsToNearestEvenAtRequestedPrecision)
{
    const MPPrecision low{1};
    auto one = low.real("1");
    const int bits = static_cast<int>(mpfr_get_prec(one.backend().data()));
    const auto half = power(-bits);
    auto lower = MPConversions::to_big_float(m_precision.real("1") + half, bf_arena(BigLifetime::WORKING));
    auto upper = MPConversions::to_big_float(m_precision.real("1") + 3 * half, lower.arena());
    EXPECT_EQ(one, MPConversions::from_big_float(lower, low));
    mpfr_nextabove(one.backend().data());
    mpfr_nextabove(one.backend().data());
    EXPECT_EQ(one, MPConversions::from_big_float(upper, low));
    const auto sticky = MPConversions::to_big_float(m_precision.real("1") + half + power(-bits - 8), lower.arena());
    mpfr_nextbelow(one.backend().data());
    EXPECT_EQ(one, MPConversions::from_big_float(sticky, low));
}

TEST_F(TestMPConversions, roundsFloatingPointTiesAndCarry)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const auto quantum = power(-8 * (g_bf_length - 2));
    EXPECT_EQ(pack(BigFloat{"1"}), pack(MPConversions::to_big_float(m_precision.real("1") + quantum / 2, arena)));
    EXPECT_EQ(m_precision.real("1") + 2 * quantum,
        MPConversions::from_big_float(
            MPConversions::to_big_float(m_precision.real("1") + 3 * quantum / 2, arena), m_precision));
    std::vector<Byte> bytes(g_bf_length);
    bytes[g_bf_length - 2] = 1;
    const auto carried = MPConversions::to_big_float(m_precision.real("256") - quantum / 2, arena);
    EXPECT_EQ(pack(unpack(bytes, 1)), pack(carried));
    EXPECT_EQ(m_precision.real("256"), MPConversions::from_big_float(carried, m_precision));
    EXPECT_EQ(pack(BigFloat{"-1"}), pack(MPConversions::to_big_float(m_precision.real("-1"), arena)));
}

TEST_F(TestMPConversions, handlesFloatingPointUnderflowAndAsymmetricOverflow)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const auto quantum = power(8 * (-32768 + 2 - g_bf_length));
    const auto limit = power(8 * 32768);
    for (const int sign : {-1, 1})
    {
        SCOPED_TRACE(sign);
        EXPECT_EQ(pack(BigFloat{"0"}), pack(MPConversions::to_big_float(sign * quantum / 2, arena)));
        EXPECT_EQ(sign * 2 * quantum,
            MPConversions::from_big_float(MPConversions::to_big_float(sign * 3 * quantum / 2, arena), m_precision));
        EXPECT_THROW(MPConversions::to_big_float(sign * 256 * limit, arena), PrecisionError);
    }
    EXPECT_THROW(MPConversions::to_big_float(limit, arena), PrecisionError);
    EXPECT_EQ(-limit, MPConversions::from_big_float(MPConversions::to_big_float(-limit, arena), m_precision));
    const auto top_quantum = power(8 * (32767 + 2 - g_bf_length));
    EXPECT_THROW(MPConversions::to_big_float(limit - top_quantum / 2, arena), PrecisionError);
    EXPECT_EQ(limit - top_quantum,
        MPConversions::from_big_float(MPConversions::to_big_float(limit - top_quantum, arena), m_precision));
}

TEST_F(TestMPConversions, rejectsNonFiniteNullAndMismatchedLayouts)
{
    for (const char *text : {"nan", "inf", "-inf"})
    {
        SCOPED_TRACE(text);
        const auto value = m_precision.real(text);
        EXPECT_THROW(MPConversions::to_big_float(value, bf_arena(BigLifetime::WORKING)), std::invalid_argument);
    }
    EXPECT_THROW(MPConversions::from_big_float(BigFloat{}, m_precision), std::invalid_argument);
    BigArena wrong{8, 4, reduce_bf_product};
    EXPECT_THROW(MPConversions::to_big_float(m_precision.real(), wrong), std::invalid_argument);
    EXPECT_THROW(MPConversions::from_big_float(BigFloat{wrong}, m_precision), std::invalid_argument);
}

TEST_F(TestMPConversions, canonicalizesSignedZeroWithoutChangingDefaultsOrLegacyState)
{
    const auto defaults = MPReal::default_precision();
    const auto thread_defaults = MPReal::thread_default_precision();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const auto source = BigFloat{"-1.25"};
    const auto zero = pack(BigFloat{0.0L});
    const auto before = pack(source);
    const auto scratch = pack(g_bf_tmp1);
    const auto value = MPConversions::from_big_float(source, m_precision);
    EXPECT_EQ(zero, pack(MPConversions::to_big_float(m_precision.real("-0"), source.arena())));
    EXPECT_EQ(before, pack(source));
    EXPECT_EQ(scratch, pack(g_bf_tmp1));
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(defaults, MPReal::default_precision());
    EXPECT_EQ(thread_defaults, MPReal::thread_default_precision());
    EXPECT_EQ(m_precision.working_digits(), value.precision());
}

TEST_F(TestMPConversions, importedValueOutlivesLegacyContext)
{
    MPReal retained = m_precision.real();
    {
        const BigFloat source{"-1.25"};
        retained = MPConversions::from_big_float(source, m_precision);
    }
    free_bf_vars();
    EXPECT_EQ(m_precision.real("-1.25"), retained);
}

TEST_F(TestMPConversions, preservesAllSignificandBitsAtMaximumPrecision)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(708);
    const MPPrecision precision{2000};
    std::vector<Byte> bytes(g_bf_length, 0xff);
    bytes.back() = 0;
    const auto source = unpack(bytes, 0);
    const auto imported = MPConversions::from_big_float(source, precision);
    auto expected = precision.real("1");
    mpfr_mul_2si(expected.backend().data(), expected.backend().data(), -8 * (g_bf_length - 2), MPFR_RNDN);
    mpfr_ui_sub(expected.backend().data(), 256, expected.backend().data(), MPFR_RNDN);
    EXPECT_EQ(expected, imported);
    EXPECT_EQ(precision.working_digits(), imported.precision());
    EXPECT_EQ(pack(source), pack(MPConversions::to_big_float(imported, source.arena())));
}

} // namespace
} // namespace id::math
