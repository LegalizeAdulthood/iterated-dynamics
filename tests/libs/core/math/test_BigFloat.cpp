// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
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

TEST_F(TestBigFloatValue, zeroConstructionLeavesTheLegacyStackUnchanged)
{
    const auto marker = save_stack();

    const BigFloat value{0};

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
    BigFloatPtr source = bf_stack().allocate_single();
    str_to_bf(source, "-1.23456789012345678901234567890123456789e-400");
    const std::vector<Byte> expected(source, source + size);

    const BigFloat value = BigFloat::from_legacy(source, size);
    std::fill_n(source, size, Byte{0});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigFloatValue, valueSurvivesArenaRewindAndReuse)
{
    const auto marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloatPtr source = bf_stack().allocate_single();
    str_to_bf(source, "1.23456789012345678901234567890123456789");
    const BigFloat value = BigFloat::from_legacy(source, size);
    const auto expected = encoded_value(value);

    restore_stack(marker);
    BigFloatPtr reused = bf_stack().allocate_single();
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

TEST_F(TestBigFloatValue, copyAssignmentUsesTheRelocatedArenaLayout)
{
    const BigFloat original{"1.23456789012345678901234567890123456789"};
    const auto expected = encoded_value(original);
    const int old_length = g_bf_length;
    init_bf_dec(80);
    BigFloat assigned{2.5L};

    assigned = original;

    std::vector<Byte> exported(g_bf_length + 2);
    std::vector<Byte> widened(g_bf_length + 2);
    convert_bf(widened.data(), expected.data(), g_bf_length, old_length);
    assigned.copy_to_legacy(exported.data(), exported.size());
    EXPECT_EQ(widened, exported);
    EXPECT_EQ(widened, encoded_value(original));
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

TEST_F(TestBigFloatValue, legacyBorrowsAccessOwnedStorageAndResolveRelocatedHandles)
{
    static_assert(std::is_same_v<decltype(std::declval<BigFloat &>().data()), BigFloatPtr>);
    static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().data()), BigFloatConstPtr>);
    BigFloat value{-1.25L};
    const auto *before = std::as_const(value).data();
    EXPECT_EQ(before, value.data());
    EXPECT_EQ(encoded_value(value), std::vector<Byte>(before, before + g_bf_length + 2));

    float_to_bf(value.data(), 2.5L);
    EXPECT_EQ(BigFloat{2.5L}, value);
    init_bf_dec(80);

    EXPECT_NE(before, std::as_const(value).data());
    EXPECT_EQ(BigFloat{2.5L}, value);
    EXPECT_EQ(encoded_value(value), std::vector<Byte>(value.data(), value.data() + g_bf_length + 2));
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

class TestBigFloatArithmetic : public TestBigFloatValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigFloatArithmetic::SetUp()
{
    TestBigFloatValue::SetUp();
    init_bf_length(GetParam());
}

TEST_P(TestBigFloatArithmetic, legacyArithmeticSamplesPreserveOperands)
{
    auto left = encoded_value(BigFloat{"-7.5"});
    auto right = encoded_value(BigFloat{"2.5"});
    const auto original_left = left;
    const auto original_right = right;
    std::vector<Byte> result(static_cast<std::size_t>(g_r_bf_length + 2));

    add_bf(result.data(), left.data(), right.data());
    EXPECT_EQ("-5.0", bf_to_string(result.data(), 8));
    sub_bf(result.data(), left.data(), right.data());
    EXPECT_EQ("-10.0", bf_to_string(result.data(), 8));
    mult_bf(result.data(), left.data(), right.data());
    EXPECT_EQ("-18.75", bf_to_string(result.data(), 8));
    div_bf(result.data(), left.data(), right.data());
    EXPECT_EQ("-3.0", bf_to_string(result.data(), 8));

    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigFloatArithmetic, legacyInPlaceSamplesPreserveTheRightOperand)
{
    auto left = encoded_value(BigFloat{"-7.5"});
    auto right = encoded_value(BigFloat{"2.5"});
    const auto original_right = right;

    add_a_bf(left.data(), right.data());
    EXPECT_EQ("-5.0", bf_to_string(left.data(), 8));
    sub_a_bf(left.data(), right.data());
    EXPECT_EQ("-7.5", bf_to_string(left.data(), 8));
    add_a_bf(left.data(), left.data());
    EXPECT_EQ("-15.0", bf_to_string(left.data(), 8));
    sub_a_bf(left.data(), left.data());
    EXPECT_EQ("0.0", bf_to_string(left.data(), 8));

    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigFloatArithmetic, arithmeticPreservesOperandsAndReturnsIndependentValues)
{
    const BigFloat left{"-7.5"};
    const BigFloat right{"2.5"};
    const auto original_left = encoded_value(left);
    const auto original_right = encoded_value(right);

    BigFloat sum = left + right;
    EXPECT_EQ("-5.0", sum.to_string(8));
    EXPECT_EQ("-10.0", (left - right).to_string(8));
    EXPECT_EQ("-18.75", (left * right).to_string(8));
    EXPECT_EQ("-3.0", (left / right).to_string(8));
    sum = 0;

    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigFloatArithmetic, negationPreservesItsOperand)
{
    const BigFloat value{"-7.5"};
    const auto original = encoded_value(value);

    EXPECT_EQ("7.5", (-value).to_string(8));
    EXPECT_EQ(original, encoded_value(value));
    EXPECT_EQ("0.0", (-BigFloat{0}).to_string(8));
    EXPECT_EQ(original, encoded_value(-(-value)));
}

TEST_P(TestBigFloatArithmetic, compoundAssignmentChangesOnlyItsDestination)
{
    BigFloat value{"-7.5"};
    const BigFloat right{"2.5"};
    const auto original_right = encoded_value(right);
    const BigFloat copy = value;

    EXPECT_EQ(&value, &(value += right));
    EXPECT_EQ("-5.0", value.to_string(8));
    EXPECT_EQ(&value, &(value -= right));
    EXPECT_EQ("-7.5", value.to_string(8));
    EXPECT_EQ(&value, &(value *= right));
    EXPECT_EQ("-18.75", value.to_string(8));
    EXPECT_EQ(&value, &(value /= right));
    EXPECT_EQ("-7.5", value.to_string(8));

    EXPECT_EQ("-7.5", copy.to_string(8));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigFloatArithmetic, repeatedOperandsAndSelfAssignmentAreSafe)
{
    BigFloat value{"-7.5"};

    EXPECT_EQ("56.25", (value * value).to_string(8));
    EXPECT_EQ("-7.5", value.to_string(8));
    value *= value;
    EXPECT_EQ("56.25", value.to_string(8));
    value /= value;
    EXPECT_EQ("1.0", value.to_string(8));
    value += value;
    EXPECT_EQ("2.0", value.to_string(8));
    value -= value;
    EXPECT_EQ("0.0", value.to_string(8));
}

TEST_P(TestBigFloatArithmetic, assignmentCanAlsoSupplyTheRightOperand)
{
    const BigFloat left{"3.5"};
    BigFloat value{"-2.5"};

    value = left + value;
    EXPECT_EQ("1.0", value.to_string(8));
    value = left - value;
    EXPECT_EQ("2.5", value.to_string(8));
    value = left * value;
    EXPECT_EQ("8.75", value.to_string(8));
    value = left / value;
    EXPECT_EQ("0.4", value.to_string(8));
}

TEST_P(TestBigFloatArithmetic, expressionsMatchLegacySequencesAndOutliveTheirTemporaries)
{
    const BigFloat left{"-1.234567890123456789e-400"};
    const BigFloat right{"2.345678901234567890e-400"};
    const BigFloat divisor{"-3.456789012345678901e-400"};
    const auto original_left = encoded_value(left);
    const auto original_right = encoded_value(right);
    const auto original_divisor = encoded_value(divisor);
    auto raw_left = original_left;
    auto raw_right = original_right;
    auto raw_divisor = original_divisor;
    std::vector<Byte> expected(static_cast<std::size_t>(g_r_bf_length + 2));
    add_bf(expected.data(), raw_left.data(), raw_right.data());
    div_bf(expected.data(), expected.data(), raw_divisor.data());
    mult_bf(expected.data(), expected.data(), raw_left.data());
    sub_bf(expected.data(), expected.data(), raw_right.data());
    expected.resize(static_cast<std::size_t>(g_bf_length + 2));
    const auto marker = save_stack();

    const BigFloat result = ((left + right) / divisor) * left - right;
    const BigFloat unrelated = BigFloat{"-8"} * BigFloat{"-9"};
    bf_stack().allocate_single();
    restore_stack(marker);

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ("72.0", unrelated.to_string(8));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
    EXPECT_EQ(original_divisor, encoded_value(divisor));
}

TEST_P(TestBigFloatArithmetic, zeroAndDivisionByZeroKeepLegacyResults)
{
    const BigFloat zero{0};
    const BigFloat value{"-7.5"};
    auto raw_value = encoded_value(value);
    auto raw_zero = encoded_value(zero);
    std::vector<Byte> expected(static_cast<std::size_t>(g_r_bf_length + 2));
    div_bf(expected.data(), raw_value.data(), raw_zero.data());
    expected.resize(static_cast<std::size_t>(g_bf_length + 2));

    EXPECT_EQ(encoded_value(value), encoded_value(value + zero));
    EXPECT_EQ(encoded_value(value), encoded_value(value - zero));
    EXPECT_EQ(encoded_value(-value), encoded_value(zero - value));
    EXPECT_EQ(encoded_value(zero), encoded_value(value * zero));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / value));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / zero));
    EXPECT_EQ(expected, encoded_value(value / zero));
}

TEST_P(TestBigFloatArithmetic, unsignedScalingRetainsLegacyNormalizationAndOperands)
{
    const std::array<U16, 7> factors{0, 1, 2, 255, 256, 32768, 65535};
    auto &persistent = bf_arena(BigLifetime::PERSISTENT);
    for (const char *text : {"0", "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400"})
    {
        const BigFloat source{persistent, text};
        const auto original = encoded_value(source);
        for (const U16 factor : factors)
        {
            SCOPED_TRACE(text);
            SCOPED_TRACE(factor);
            std::vector<Byte> expected(g_r_bf_length + 2);
            mult_bf_int(expected.data(), original.data(), factor);
            expected.resize(g_bf_length + 2);

            const BigFloat result = source * factor;

            EXPECT_EQ(expected, encoded_value(result));
            EXPECT_EQ(original, encoded_value(source));
            EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &result.arena());
        }
    }
}

TEST_P(TestBigFloatArithmetic, unsignedDivisionAssignmentRetainsRoundingAndZeroDivisorSaturation)
{
    const std::array<U16, 7> divisors{0, 1, 2, 255, 256, 32768, 65535};
    auto &persistent = bf_arena(BigLifetime::PERSISTENT);
    for (const char *text : {"0", "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400"})
    {
        const BigFloat source{persistent, text};
        const auto original = encoded_value(source);
        for (const U16 divisor : divisors)
        {
            SCOPED_TRACE(text);
            SCOPED_TRACE(divisor);
            auto expected = original;
            div_a_bf_int(expected.data(), divisor);
            BigFloat value{source};

            EXPECT_EQ(&value, &(value /= divisor));

            EXPECT_EQ(expected, encoded_value(value));
            EXPECT_EQ(original, encoded_value(source));
            EXPECT_EQ(&persistent, &value.arena());
        }
    }
}

TEST_P(TestBigFloatArithmetic, absoluteValuePreservesItsOperandAndDestinationArena)
{
    auto &persistent = bf_arena(BigLifetime::PERSISTENT);
    for (const char *text :
        {"0", "2.5", "-2.5", "1.2345678901234567890123456789e-400", "-1.2345678901234567890123456789e-400",
            "1.2345678901234567890123456789e400", "-1.2345678901234567890123456789e400"})
    {
        SCOPED_TRACE(text);
        const BigFloat source{persistent, text};
        const auto original = encoded_value(source);
        auto raw_source = original;
        std::vector<Byte> expected(original.size());
        abs_bf(expected.data(), raw_source.data());

        const BigFloat member_result = source.abs();
        const BigFloat result = abs(source);
        BigFloat assigned{persistent, source};
        assigned = abs(assigned);

        EXPECT_EQ(expected, encoded_value(member_result));
        EXPECT_EQ(expected, encoded_value(result));
        EXPECT_EQ(expected, encoded_value(assigned));
        EXPECT_EQ(original, encoded_value(source));
        EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &member_result.arena());
        EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &result.arena());
        EXPECT_EQ(&persistent, &assigned.arena());
    }
}

TEST_P(TestBigFloatArithmetic, constMachineConversionRetainsLegacyResultsAndInputBytes)
{
    for (const char *text :
        {"0", "7.5", "-7.5", "1.2345678901234567890123456789e-400", "-1.2345678901234567890123456789e-400",
            "1.2345678901234567890123456789e400", "-1.2345678901234567890123456789e400"})
    {
        SCOPED_TRACE(text);
        const BigFloat source{bf_arena(BigLifetime::PERSISTENT), text};
        const auto original = encoded_value(source);
        auto legacy = original;
        const LDouble expected{bf_to_float(legacy.data())};

        const LDouble actual{source.to_float()};

        EXPECT_EQ(expected, actual);
        EXPECT_EQ(std::signbit(expected), std::signbit(actual));
        EXPECT_EQ(original, encoded_value(source));
    }
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatArithmetic, testing::Values(4, 32, 708));

TEST_F(TestBigFloatValue, comparisonsUseNumericalValuesWithoutChangingOperands)
{
    const BigFloat values[]{BigFloat{"-1e400"}, BigFloat{"-2"}, BigFloat{"-1e-400"}, BigFloat{0}, BigFloat{"1e-400"},
        BigFloat{"1"}, BigFloat{"1.000000000000000000000000000001"}, BigFloat{"1e400"}};

    for (std::size_t i = 0; i < std::size(values); ++i)
    {
        for (std::size_t j = 0; j < std::size(values); ++j)
        {
            const auto left = encoded_value(values[i]);
            const auto right = encoded_value(values[j]);
            const BigFloat copy = values[j];

            EXPECT_EQ(i == j, values[i] == copy);
            EXPECT_EQ(i != j, values[i] != copy);
            EXPECT_EQ(i < j, values[i] < copy);
            EXPECT_EQ(i <= j, values[i] <= copy);
            EXPECT_EQ(i > j, values[i] > copy);
            EXPECT_EQ(i >= j, values[i] >= copy);
            EXPECT_EQ(left, encoded_value(values[i]));
            EXPECT_EQ(right, encoded_value(values[j]));
            EXPECT_EQ(right, encoded_value(copy));
        }
    }
}

} // namespace id::test
