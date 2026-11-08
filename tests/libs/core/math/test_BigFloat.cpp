// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
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

static_assert(!std::is_convertible_v<BigFloat, BigFloatPtr>);
static_assert(!std::is_convertible_v<BigFloatPtr, BigFloat>);

std::vector<Byte> encoded_value(const BigFloat &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bf_length + 2));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

} // namespace

class TestBigFloatValue : public testing::Test
{
protected:
    TestBigFloatValue();
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

TestBigFloatValue::TestBigFloatValue() :
    m_saved_bailout_value(g_user.bailout_value, 0),
    m_saved_bailout_test(g_bailout_test, Bailout::MOD),
    m_saved_bf_digits(g_bf_digits, 0),
    m_saved_fractal_type(g_fractal_type, FractalType::MANDEL_Z_POWER)
{
}

void TestBigFloatValue::SetUp()
{
    init_bf_dec(40);
}

void TestBigFloatValue::TearDown()
{
    free_bf_vars();
}

TEST_F(TestBigFloatValue, zeroConstructionDoesNotAllocateArenaStorage)
{
    const int marker = save_stack();

    const BigFloat value;

    EXPECT_EQ(marker, save_stack());
    EXPECT_EQ("0.0", value.to_string());
}

TEST_F(TestBigFloatValue, parsesTheSpecifiedView)
{
    const std::string text{"-1.25x"};

    const BigFloat value{std::string_view{text.data(), 5}};

    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigFloatValue, assignsNativeScalarValues)
{
    BigFloat value{1.25L};

    value = -7;

    EXPECT_EQ("-7.0", value.to_string());
}

TEST_F(TestBigFloatValue, supportsTheSmallestLegacyLayout)
{
    init_bf_length(4);
    const BigFloat value{"-1.25"};

    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_EQ(static_cast<std::size_t>(g_bf_length + 2), encoded_value(value).size());
}

TEST_F(TestBigFloatValue, supportsTheLargestLegacyLayout)
{
    init_bf_length(708);
    const BigFloat value{"-1.25"};

    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_EQ(static_cast<std::size_t>(g_bf_length + 2), encoded_value(value).size());
}

TEST_F(TestBigFloatValue, importsEncodedBytesWithoutMachineFloatConversion)
{
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloatPtr source = alloc_stack(size);
    str_to_bf(source, "-1.23456789012345678901234567890123456789e-400");
    const std::vector<Byte> expected(source, source + size);

    const BigFloat value = BigFloat::from_legacy(source, size);
    std::fill_n(source, size, Byte{0});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigFloatValue, valueSurvivesArenaRewindAndReuse)
{
    const int marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloatPtr source = alloc_stack(size);
    str_to_bf(source, "1.23456789012345678901234567890123456789");
    const BigFloat value = BigFloat::from_legacy(source, size);
    const auto expected = encoded_value(value);

    restore_stack(marker);
    BigFloatPtr reused = alloc_stack(size);
    ASSERT_EQ(source, reused);
    std::fill_n(reused, size, Byte{0xff});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigFloatValue, exportedBytesAreIndependentOfTheValue)
{
    const BigFloat value{"-1.25"};
    auto exported = encoded_value(value);

    std::fill(exported.begin(), exported.end(), Byte{0});

    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigFloatValue, exportDoesNotRequireAnActiveArena)
{
    const BigFloat value{"1.23456789012345678901234567890123456789e-400"};
    const auto expected = encoded_value(value);
    free_bf_vars();
    std::vector<Byte> exported(expected.size());

    value.copy_to_legacy(exported.data(), exported.size());

    EXPECT_EQ(expected, exported);
}

TEST_F(TestBigFloatValue, copiesDoNotShareWritableValueStorage)
{
    BigFloat original{"-1.25"};
    const BigFloat copy{original};

    original = 2.5L;

    EXPECT_EQ("-1.25", copy.to_string());
    EXPECT_EQ("2.5", original.to_string());
}

TEST_F(TestBigFloatValue, copyingAndMovingRemainValidWithoutAnActiveArena)
{
    BigFloat original{"1.23456789012345678901234567890123456789"};
    const auto expected = encoded_value(original);
    free_bf_vars();

    const BigFloat copy{original};
    BigFloat moved{std::move(original)};
    original = copy;
    moved = std::move(original);

    init_bf_dec(40);
    EXPECT_EQ(expected, encoded_value(copy));
    EXPECT_EQ(expected, encoded_value(moved));
}

TEST_F(TestBigFloatValue, copyAssignmentUsesTheOwnedByteCount)
{
    const BigFloat original{"1.23456789012345678901234567890123456789"};
    const auto expected = encoded_value(original);
    init_bf_dec(80);
    BigFloat assigned{2.5L};

    assigned = original;

    std::vector<Byte> exported(expected.size());
    assigned.copy_to_legacy(exported.data(), exported.size());
    EXPECT_EQ(expected, exported);
    init_bf_dec(40);
    EXPECT_EQ(expected, encoded_value(assigned));
}

TEST_F(TestBigFloatValue, destructionDoesNotRequireTheOriginalPrecision)
{
    {
        const BigFloat value{"-1.25"};
        init_bf_dec(80);
        free_bf_vars();
    }

    init_bf_dec(40);
    const BigFloat value{2.5L};
    EXPECT_EQ("2.5", value.to_string());
}

TEST_F(TestBigFloatValue, movedFromValueCanBeReassigned)
{
    BigFloat original{"-1.25"};
    const BigFloat moved{std::move(original)};

    original = moved;

    EXPECT_EQ("-1.25", original.to_string());
}

TEST_F(TestBigFloatValue, adaptersRejectNullAndUndersizedBuffers)
{
    const BigFloat value{"-1.25"};
    std::vector<Byte> buffer(static_cast<std::size_t>(g_bf_length + 2), Byte{0x5a});
    const auto unchanged = buffer;

    EXPECT_THROW(BigFloat::from_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(BigFloat::from_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(unchanged, buffer);
}

TEST_F(TestBigFloatValue, rejectsNonfiniteScalarInputWithoutChangingTheValue)
{
    BigFloat value{"-1.25"};

    EXPECT_THROW((BigFloat{std::numeric_limits<LDouble>::infinity()}), std::invalid_argument);
    EXPECT_THROW(value = std::numeric_limits<LDouble>::quiet_NaN(), std::invalid_argument);
    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigFloatValue, formattingRejectsNegativeDigitCounts)
{
    const BigFloat value{"-1.25"};

    EXPECT_THROW(value.to_string(-1), std::invalid_argument);
}

} // namespace id::test
