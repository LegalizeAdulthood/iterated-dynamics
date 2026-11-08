// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>
#include <math/BigNum.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

static_assert(!std::is_same_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigNum, BigNumPtr>);
static_assert(!std::is_convertible_v<BigNumPtr, BigNum>);
static_assert(!std::is_convertible_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigFloat, BigNum>);
static_assert(!std::is_convertible_v<LDouble, BigNum>);

std::vector<Byte> encoded_value(const BigNum &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bn_length));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

std::vector<Byte> encoded_value(const BigFloat &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bf_length + 2));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

} // namespace

class TestBigNumValue : public testing::Test
{
protected:
    TestBigNumValue();
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

TestBigNumValue::TestBigNumValue() :
    m_saved_bailout_value(g_user.bailout_value, 0),
    m_saved_bailout_test(g_bailout_test, Bailout::MOD),
    m_saved_bf_digits(g_bf_digits, 0),
    m_saved_fractal_type(g_fractal_type, FractalType::MANDEL)
{
}

void TestBigNumValue::SetUp()
{
    init_bf_dec(40);
}

void TestBigNumValue::TearDown()
{
    free_bf_vars();
}

class TestBigNumLayout : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigNumLayout::SetUp()
{
    if (GetParam() == 2)
    {
        g_bailout_test = Bailout::REAL;
    }
    else if (GetParam() == 4)
    {
        g_user.bailout_value = 100;
    }
    init_bf_length(32);
    ASSERT_EQ(GetParam(), g_int_length);
}

TEST_P(TestBigNumLayout, legacyConversionsPreserveEncodedValuesAndInputs)
{
    std::vector<Byte> fixed(static_cast<std::size_t>(g_bn_length));
    str_to_bn(fixed.data(), "-1.23456789012345678901234567890123456789");
    const auto original_fixed = fixed;
    std::vector<Byte> floating(static_cast<std::size_t>(g_bf_length + 2));
    std::vector<Byte> restored(fixed.size());

    bn_to_bf(floating.data(), fixed.data());
    const auto original_float = floating;
    bf_to_bn(restored.data(), floating.data());

    EXPECT_EQ(original_fixed, fixed);
    EXPECT_EQ(original_float, floating);
    EXPECT_EQ(original_fixed, restored);
}

TEST_P(TestBigNumLayout, legacyFormattingPreservesInputBytes)
{
    std::vector<Byte> value(static_cast<std::size_t>(g_bn_length));
    str_to_bn(value.data(), "-12.5");
    const auto original = value;

    EXPECT_EQ("-12.5", bn_to_string(value.data(), 8));
    EXPECT_EQ("-12.5", bn_to_string(value.data(), 0));
    EXPECT_EQ(original, value);
}

TEST_P(TestBigNumLayout, legacyFloatConversionRetainsOverflowAndUnderflowRules)
{
    for (const char *text : {"1e400", "-1e400", "1e-400", "-1e-400"})
    {
        SCOPED_TRACE(text);
        std::vector<Byte> floating(static_cast<std::size_t>(g_bf_length + 2));
        str_to_bf(floating.data(), text);
        const auto original = floating;
        std::vector<Byte> result(static_cast<std::size_t>(g_bn_length));
        std::vector<Byte> expected(result.size());
        if (std::string_view{text}.find("e-") == std::string_view::npos)
        {
            max_bn(expected.data());
            if (text[0] == '-')
            {
                neg_a_bn(expected.data());
            }
        }

        bf_to_bn(result.data(), floating.data());

        EXPECT_EQ(expected, result);
        EXPECT_EQ(original, floating);
    }
}

TEST_F(TestBigNumValue, zeroConstructionDoesNotAllocateArenaStorage)
{
    const int marker = save_stack();

    const BigNum value;

    EXPECT_EQ(marker, save_stack());
    EXPECT_EQ("0.", value.to_string());
}

TEST_F(TestBigNumValue, parsesTheSpecifiedView)
{
    const std::string text{"-1.25x"};

    const BigNum value{std::string_view{text.data(), 5}};

    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_EQ("0.", BigNum{std::string_view{}}.to_string());
}

TEST_F(TestBigNumValue, assignsNativeScalarsWithoutSharingCopies)
{
    BigNum value{1.25L};
    const BigNum copy = value;

    value = -7;

    EXPECT_EQ("1.25", copy.to_string());
    EXPECT_EQ("-7.", value.to_string());
}

TEST_F(TestBigNumValue, supportsSmallestAndLargestLegacyLayouts)
{
    for (const int length : {4, 672})
    {
        SCOPED_TRACE(length);
        init_bf_length(length);

        const BigNum value{"-1.25"};

        EXPECT_EQ("-1.25", value.to_string(8));
        EXPECT_EQ(static_cast<std::size_t>(length), encoded_value(value).size());
        EXPECT_EQ(encoded_value(value), encoded_value(BigNum{BigFloat{value}}));
    }
}

TEST_F(TestBigNumValue, importedBytesSurviveArenaRewindAndReuse)
{
    const int marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bn_length);
    BigNumPtr source = alloc_stack(size);
    str_to_bn(source, "-1.23456789012345678901234567890123456789");
    const std::vector<Byte> expected(source, source + size);
    const BigNumConstPtr input = source;

    const BigNum value = BigNum::from_legacy(input, size);
    restore_stack(marker);
    BigNumPtr reused = alloc_stack(size);
    ASSERT_EQ(source, reused);
    std::fill_n(reused, size, Byte{0xff});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigNumValue, exportedBytesAreIndependentOfTheValue)
{
    const BigNum value{"-1.25"};
    auto exported = encoded_value(value);

    std::fill(exported.begin(), exported.end(), Byte{0});

    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigNumValue, copyingMovingAndExportDoNotRequireAnActiveArena)
{
    BigNum original{"-1.23456789012345678901234567890123456789"};
    const auto expected = encoded_value(original);
    free_bf_vars();

    const BigNum copy{original};
    BigNum moved{std::move(original)};
    original = copy;
    moved = std::move(original);
    std::vector<Byte> exported(expected.size());
    moved.copy_to_legacy(exported.data(), exported.size());

    EXPECT_EQ(expected, exported);
    init_bf_dec(40);
    EXPECT_EQ(expected, encoded_value(copy));
    EXPECT_EQ(expected, encoded_value(moved));
}

TEST_F(TestBigNumValue, copyAssignmentAndDestructionUseTheOwnedByteCount)
{
    const BigNum original{"-1.25"};
    const auto expected = encoded_value(original);
    g_bailout_test = Bailout::REAL;
    init_bf_dec(80);

    {
        BigNum assigned{2.5L};
        assigned = original;
        std::vector<Byte> exported(expected.size());
        assigned.copy_to_legacy(exported.data(), exported.size());

        EXPECT_EQ(expected, exported);
        free_bf_vars();
    }

    g_bailout_test = Bailout::MOD;
    init_bf_dec(40);
    EXPECT_EQ("-1.25", original.to_string());
}

TEST_F(TestBigNumValue, movedFromValueCanBeReassigned)
{
    BigNum original{"-1.25"};
    const BigNum moved{std::move(original)};

    original = 2.5L;

    EXPECT_EQ("-1.25", moved.to_string());
    EXPECT_EQ("2.5", original.to_string());
}

TEST_F(TestBigNumValue, adaptersRejectNullAndUndersizedBuffers)
{
    const BigNum value{"-1.25"};
    std::vector<Byte> buffer(static_cast<std::size_t>(g_bn_length), Byte{0x5a});
    const auto unchanged = buffer;

    EXPECT_THROW(BigNum::from_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(BigNum::from_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(unchanged, buffer);
}

TEST_F(TestBigNumValue, rejectsNonfiniteScalarInputWithoutChangingTheValue)
{
    BigNum value{"-1.25"};

    EXPECT_THROW((BigNum{std::numeric_limits<LDouble>::infinity()}), std::invalid_argument);
    EXPECT_THROW(value = std::numeric_limits<LDouble>::quiet_NaN(), std::invalid_argument);
    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigNumValue, formattingPreservesInputAndRejectsNegativeDigitCounts)
{
    const BigNum value{"-1.25"};
    const auto original = encoded_value(value);

    EXPECT_EQ("-1.25", value.to_string(8));
    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_THROW(value.to_string(-1), std::invalid_argument);
    EXPECT_EQ(original, encoded_value(value));
}

TEST_P(TestBigNumLayout, constructorsUseTheActiveFixedPointLayout)
{
    const std::size_t ones_byte = static_cast<std::size_t>(g_bn_length - g_int_length);
    std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
    expected[ones_byte] = 1;
    expected[ones_byte - 1] = 0x40;

    EXPECT_EQ(expected, encoded_value(BigNum{1.25L}));
    EXPECT_EQ(expected, encoded_value(BigNum{"1.25"}));
}

TEST_P(TestBigNumLayout, explicitConversionsPreserveBitsBeyondMachinePrecision)
{
    std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
    expected.front() = 1;
    expected[static_cast<std::size_t>(g_bn_length - g_int_length)] = 1;
    const BigNum fixed = BigNum::from_legacy(expected.data(), expected.size());

    const BigFloat floating{fixed};
    const auto original_float = encoded_value(floating);
    const BigNum restored{floating};

    EXPECT_EQ(expected, encoded_value(fixed));
    EXPECT_EQ(expected, encoded_value(restored));
    EXPECT_EQ(original_float, encoded_value(floating));
}

TEST_P(TestBigNumLayout, owningConversionsKeepLegacyOverflowAndUnderflowResults)
{
    for (const char *text : {"1e400", "-1e400", "1e-400", "-1e-400"})
    {
        SCOPED_TRACE(text);
        const BigFloat floating{text};
        const auto original = encoded_value(floating);
        auto raw = original;
        std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
        bf_to_bn(expected.data(), raw.data());

        const BigNum fixed{floating};

        EXPECT_EQ(expected, encoded_value(fixed));
        EXPECT_EQ(original, encoded_value(floating));
    }
}

TEST_F(TestBigNumValue, conversionAdaptersValidateDestinationBuffers)
{
    const BigNum fixed{"-1.25"};
    const BigFloat floating{fixed};
    std::vector<Byte> bn_buffer(static_cast<std::size_t>(g_bn_length), Byte{0x5a});
    std::vector<Byte> bf_buffer(static_cast<std::size_t>(g_bf_length + 2), Byte{0x5a});
    const auto original_bn = bn_buffer;
    const auto original_bf = bf_buffer;

    EXPECT_THROW(fixed.copy_to_legacy_float(nullptr, bf_buffer.size()), std::invalid_argument);
    EXPECT_THROW(fixed.copy_to_legacy_float(bf_buffer.data(), bf_buffer.size() - 1), std::invalid_argument);
    EXPECT_THROW(floating.copy_to_legacy_num(nullptr, bn_buffer.size()), std::invalid_argument);
    EXPECT_THROW(floating.copy_to_legacy_num(bn_buffer.data(), bn_buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(original_bn, bn_buffer);
    EXPECT_EQ(original_bf, bf_buffer);
}

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumLayout, testing::Values(1, 2, 4));

} // namespace id::test
