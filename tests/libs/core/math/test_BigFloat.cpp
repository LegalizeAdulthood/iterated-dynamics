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
#include <cstring>
#include <limits>
#include <ostream>
#include <string>
#include <tuple>
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
static_assert(std::is_same_v<decltype(&bf_hex_dump), void (*)(const BigFloat &)>);

std::vector<Byte> encoded_value(const BigFloat &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bf_length + 2));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

void expect_hex_dump(const BigFloat &value, const std::string &expected)
{
    const auto original = encoded_value(value);
    testing::internal::CaptureStdout();

    bf_hex_dump(value);

    const std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(expected + '\n', output);
    EXPECT_EQ(original, encoded_value(value));
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

TEST_F(TestBigFloatValue, hexDumpPrintsZeroAndPreservesInput)
{
    for (const int length : {4, 708})
    {
        SCOPED_TRACE(length);
        init_bf_length(length);
        ASSERT_EQ(length + 4, g_bf_length);
        std::vector<Byte> bytes(static_cast<std::size_t>(g_bf_length + 2));
        std::string expected;
        for (int i = 0; i < g_bf_length; ++i)
        {
            expected += "00 ";
        }

        expect_hex_dump(BigFloat::from_legacy(bytes.data(), bytes.size()), expected + " e 0000");
    }
}

TEST_F(TestBigFloatValue, hexDumpPrintsNegativeValueAndPreservesInput)
{
    for (const int length : {4, 708})
    {
        SCOPED_TRACE(length);
        init_bf_length(length);
        ASSERT_EQ(length + 4, g_bf_length);
        std::vector<Byte> bytes(static_cast<std::size_t>(g_bf_length + 2));
        bytes[bytes.size() - 4] = 0xc0;
        bytes[bytes.size() - 3] = 0xfe;
        std::string expected;
        for (int i = 0; i < g_bf_length - 2; ++i)
        {
            expected += "00 ";
        }

        expect_hex_dump(BigFloat::from_legacy(bytes.data(), bytes.size()), expected + "C0 FE  e 0000");
    }
}

TEST_F(TestBigFloatValue, hexDumpPrintsHighBitMantissaAndSignedExponents)
{
    const std::array<Byte, 4> pattern{0x00, 0x09, 0x8a, 0xff};

    struct Case
    {
        std::array<Byte, 2> exponent;
        const char *expected;
    };

    const Case cases[]{{{0x34, 0x12}, "1234"}, {{0xfe, 0xff}, "FFFE"}};
    for (const int length : {4, 708})
    {
        SCOPED_TRACE(length);
        init_bf_length(length);
        ASSERT_EQ(length + 4, g_bf_length);
        std::vector<Byte> bytes(static_cast<std::size_t>(g_bf_length + 2));
        std::string expected;
        for (int i = 0; i < g_bf_length; i += 4)
        {
            std::copy(pattern.begin(), pattern.end(), bytes.begin() + i);
            expected += "00 09 8A FF ";
        }
        for (const Case &test : cases)
        {
            SCOPED_TRACE(test.expected);
            std::copy(test.exponent.begin(), test.exponent.end(), bytes.end() - 2);

            expect_hex_dump(BigFloat::from_legacy(bytes.data(), bytes.size()), expected + " e " + test.expected);
        }
    }
}

TEST_F(TestBigFloatValue, hexDumpPrintsShiftedShortenedWorkspace)
{
    const int original_length = g_bf_length;
    ASSERT_GT(original_length, 8);
    std::vector<Byte> workspace(static_cast<std::size_t>(original_length + 2), 0xcc);
    const ValueSaver<int> active_length{g_bf_length, 8};
    BigFloatPtr shortened = workspace.data() + original_length - g_bf_length;
    const std::array<Byte, 10> bytes{0x00, 0x09, 0x8a, 0xff, 0x12, 0x34, 0x80, 0xfe, 0xfe, 0xff};
    std::copy(bytes.begin(), bytes.end(), shortened);
    const auto original = workspace;
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    ASSERT_GT(bf_arena(BigLifetime::WORKING).value_size(), size);
    BigArena dump_arena{size, 2, [](Byte *) {}};

    expect_hex_dump(BigFloat::from_legacy(dump_arena, shortened, size), "00 09 8A FF 12 34 80 FE  e FFFE");

    EXPECT_EQ(original, workspace);
    EXPECT_EQ(8, g_bf_length);
}

TEST_F(TestBigFloatValue, importsEncodedBytesWithoutMachineFloatConversion)
{
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloat source{bf_arena(BigLifetime::TEMPORARY)};
    str_to_bf(source.data(), "-1.23456789012345678901234567890123456789e-400");
    const std::vector<Byte> expected(source.data(), source.data() + size);

    const BigFloat value = BigFloat::from_legacy(source.data(), size);
    std::fill_n(source.data(), size, Byte{0});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigFloatValue, valueSurvivesArenaRewindAndReuse)
{
    // Borrow BigStack buffers here to test importing bytes across a real rewind.
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

namespace
{

static_assert(!std::is_same_v<BigFloatProduct, BigFloat>);
static_assert(!std::is_convertible_v<BigFloatProduct, BigFloat>);
static_assert(std::is_constructible_v<BigFloat, BigFloatProduct &&>);
static_assert(!std::is_constructible_v<BigFloat, BigFloatProduct &>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloatProduct &>().data()), BigFloatConstPtr>);
static_assert(std::is_invocable_v<decltype(&BigFloatProduct::release_reduced_storage), BigFloatProduct &&>);
static_assert(!std::is_invocable_v<decltype(&BigFloatProduct::release_reduced_storage), BigFloatProduct &>);

std::vector<Byte> wide_float_fixture(const int length, const bool negative, const int guard)
{
    std::vector<Byte> bytes(length + 2);
    bytes[length - 2] = negative ? 0xee : 0x12;
    bytes[length - 1] = negative ? 0xff : 0;
    bytes[length] = negative ? 0xdc : 0x34;
    bytes[length + 1] = negative ? 0xfe : 0x12;
    if (guard >= 0 && guard < length - 2)
    {
        bytes[guard] = 0x5a;
    }
    return bytes;
}

std::vector<Byte> wide_float_pattern()
{
    std::vector<Byte> bytes(g_r_bf_length + 2);
    for (int i = 0; i < g_r_bf_length + 2; ++i)
    {
        bytes[i] = static_cast<Byte>(37 * i + 19);
    }
    return bytes;
}

std::vector<Byte> product_bytes(const BigFloatProduct &value)
{
    return {value.data(), value.data() + g_r_bf_length + 2};
}

} // namespace

class TestBigFloatProductRelocation : public TestBigFloatValue,
                                      public testing::WithParamInterface<std::tuple<int, bool, BigLifetime>>
{
protected:
    void SetUp() override;
};

void TestBigFloatProductRelocation::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatProductRelocation, relocatesTheWideMantissaExponentAndFractionalGuard)
{
    const int old_length = g_r_bf_length;
    const int guard = g_padding - 1;
    const bool negative = std::get<1>(GetParam());
    BigArena &arena = bf_arena(std::get<2>(GetParam()));
    BigFloatProduct product{arena};
    const std::vector<Byte> bytes = wide_float_fixture(old_length, negative, guard);
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigFloat canonical{arena, -7.5L};
    const Byte *const address = product.data();
    const std::size_t capacity = arena.capacity();

    init_bf_length(std::get<0>(GetParam()) <= 56 ? 64 : 12);

    const std::vector<Byte> expected = wide_float_fixture(g_r_bf_length, negative, guard + g_r_bf_length - old_length);
    EXPECT_NE(address, product.data());
    EXPECT_EQ(&arena, &product.arena());
    EXPECT_EQ(capacity, arena.capacity());
    EXPECT_EQ(expected, std::vector<Byte>(product.data(), product.data() + g_r_bf_length + 2));
    EXPECT_EQ(std::vector<Byte>(2 * arena.block_size() - g_r_bf_length - 2),
        std::vector<Byte>(product.data() + g_r_bf_length + 2, product.data() + 2 * arena.block_size()));
    EXPECT_EQ(BigFloat{-7.5L}, canonical);
    const Byte *const relocated = product.data();
    const BigFloat reduced{std::move(product)};
    EXPECT_EQ(relocated, reduced.data());
    EXPECT_EQ(std::vector<Byte>(expected.begin() + g_padding, expected.end()), encoded_value(reduced));
    BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(relocated + arena.block_size(), companion.data());
}

INSTANTIATE_TEST_SUITE_P(LayoutsSignsAndLifetimes, TestBigFloatProductRelocation,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708), testing::Bool(),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

TEST(TestBigFloatProductLifetime, nullConstructionCopyMoveAndReductionNeedNoArena)
{
    static_assert(std::is_nothrow_default_constructible_v<BigFloatProduct>);
    BigFloatProduct original;
    const BigFloatProduct copy{original};
    BigFloatProduct moved{std::move(original)};
    const BigFloat reduced{std::move(moved)};
    const BigArena::Allocation released = std::move(original).release_reduced_storage();
    original = copy;
    original = std::move(moved);

    EXPECT_FALSE(original);
    EXPECT_FALSE(copy);
    EXPECT_FALSE(moved);
    EXPECT_FALSE(reduced);
    EXPECT_FALSE(released);
}

class TestBigFloatProductOwnership : public TestBigFloatValue,
                                     public testing::WithParamInterface<std::tuple<int, BigLifetime>>
{
protected:
    void SetUp() override;
};

void TestBigFloatProductOwnership::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatProductOwnership, reservesAndReleasesTwoZeroedSlotsWithoutChangingTheStack)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    const BigStackMark mark = save_stack();
    BigArena::Allocation previous = arena.allocate_double();
    Byte *const address = previous.data();
    std::fill_n(address, 2 * arena.block_size(), Byte{0xa5});
    previous.reset();
    {
        const BigFloatProduct product{arena};
        ASSERT_TRUE(product);
        EXPECT_EQ(&arena, &product.arena());
        EXPECT_EQ(address, product.data());
        EXPECT_EQ(std::vector<Byte>(2 * arena.block_size()),
            std::vector<Byte>(product.data(), product.data() + 2 * arena.block_size()));
        EXPECT_EQ(mark, save_stack());
    }
    BigArena::Allocation reused = arena.allocate_double();
    EXPECT_EQ(address, reused.data());
    EXPECT_EQ(2U, reused.blocks());
    EXPECT_EQ(mark, save_stack());
}

TEST_P(TestBigFloatProductOwnership, copiesWideBytesIndependentlyAndPreservesDestinationBindingAndTails)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    BigArena &other =
        bf_arena(std::get<1>(GetParam()) == BigLifetime::PERSISTENT ? BigLifetime::WORKING : BigLifetime::PERSISTENT);
    const std::vector<Byte> bytes = wide_float_pattern();
    BigFloatProduct source{arena};
    std::copy(bytes.begin(), bytes.end(), source.data());
    const BigFloatProduct copy{source};
    const BigFloatProduct selected{other, source};
    BigFloatProduct destination{other};
    std::fill_n(destination.data(), 2 * other.block_size(), Byte{0xa5});
    const Byte *const address = destination.data();

    EXPECT_EQ(&destination, &(destination = source));

    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_EQ(&other, &selected.arena());
    EXPECT_EQ(&other, &destination.arena());
    EXPECT_EQ(address, destination.data());
    EXPECT_NE(source.data(), copy.data());
    EXPECT_EQ(bytes, product_bytes(copy));
    EXPECT_EQ(bytes, product_bytes(selected));
    EXPECT_EQ(bytes, product_bytes(destination));
    EXPECT_EQ(std::vector<Byte>(2 * other.block_size() - bytes.size(), 0xa5),
        std::vector<Byte>(destination.data() + bytes.size(), destination.data() + 2 * other.block_size()));
    EXPECT_EQ(&source, &(source = source));
    destination.data()[0] ^= 0xff;
    EXPECT_EQ(bytes, product_bytes(source));
    EXPECT_EQ(bytes, product_bytes(copy));
}

TEST_P(TestBigFloatProductOwnership, movesAndNullAssignmentsRetainExistingAllocatorSemantics)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    BigArena &other =
        bf_arena(std::get<1>(GetParam()) == BigLifetime::PERSISTENT ? BigLifetime::WORKING : BigLifetime::PERSISTENT);
    const std::vector<Byte> bytes = wide_float_pattern();
    BigFloatProduct source{arena};
    std::copy(bytes.begin(), bytes.end(), source.data());
    const Byte *const address = source.data();
    BigFloatProduct moved{std::move(source)};
    EXPECT_FALSE(source);
    EXPECT_EQ(&arena, &source.arena());
    EXPECT_EQ(address, moved.data());
    BigFloatProduct destination{arena};
    EXPECT_EQ(&destination, &(destination = std::move(moved)));
    EXPECT_FALSE(moved);
    EXPECT_EQ(address, destination.data());
    EXPECT_EQ(&destination, &(destination = std::move(destination)));
    BigFloatProduct foreign{other};
    EXPECT_EQ(&foreign, &(foreign = std::move(destination)));
    EXPECT_TRUE(destination);
    EXPECT_EQ(address, destination.data());
    EXPECT_EQ(&other, &foreign.arena());
    EXPECT_EQ(bytes, product_bytes(foreign));
    source = destination;
    EXPECT_EQ(&arena, &source.arena());
    EXPECT_EQ(bytes, product_bytes(source));
    source = BigFloatProduct{};
    EXPECT_FALSE(source);
    EXPECT_EQ(&arena, &source.arena());
    source = foreign;
    EXPECT_EQ(&arena, &source.arena());
    EXPECT_EQ(bytes, product_bytes(source));
    BigFloatProduct unbound;
    unbound = destination;
    EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &unbound.arena());
    EXPECT_EQ(bytes, product_bytes(unbound));
    const BigFloatProduct null;
    const BigFloatProduct selected_null{arena, null};
    EXPECT_FALSE(selected_null);
    EXPECT_EQ(&arena, &selected_null.arena());
}

TEST_P(TestBigFloatProductOwnership, consumingReductionKeepsTheFirstSlotAndReleasesTheCompanion)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    const std::vector<Byte> bytes = wide_float_pattern();
    const std::vector<Byte> expected(bytes.begin() + g_padding, bytes.end());
    BigFloatProduct product{arena};
    std::copy(bytes.begin(), bytes.end(), product.data());
    const Byte *const address = product.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    const BigFloat canonical{std::move(product)};

    EXPECT_FALSE(product);
    EXPECT_EQ(&arena, &product.arena());
    EXPECT_EQ(&arena, &canonical.arena());
    EXPECT_EQ(address, canonical.data());
    EXPECT_EQ(expected, encoded_value(canonical));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(address + arena.block_size(), companion.data());
    EXPECT_EQ(1U, companion.blocks());
    std::fill_n(companion.data(), arena.block_size(), Byte{0xa5});
    EXPECT_EQ(expected, encoded_value(canonical));
}

TEST_P(TestBigFloatProductOwnership, rvalueTransferReturnsReducedSingleSlotOwnership)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    const std::vector<Byte> bytes = wide_float_pattern();
    const std::vector<Byte> expected(bytes.begin() + g_padding, bytes.end());
    BigFloatProduct product{arena};
    std::copy(bytes.begin(), bytes.end(), product.data());
    const Byte *const address = product.data();
    const std::size_t high_water = arena.high_water_mark();

    const BigArena::Allocation released = std::move(product).release_reduced_storage();

    ASSERT_TRUE(released);
    EXPECT_FALSE(product);
    EXPECT_EQ(address, released.data());
    EXPECT_EQ(&arena, &released.arena());
    EXPECT_EQ(1U, released.blocks());
    EXPECT_EQ(expected, std::vector<Byte>(released.data(), released.data() + expected.size()));
    EXPECT_EQ(high_water, arena.high_water_mark());
    BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(address + arena.block_size(), companion.data());
}

TEST_P(TestBigFloatProductOwnership, consumingReductionUsesTheConfiguredCallbackWithoutSpareArenaSpace)
{
    BigArena &working = bf_arena(std::get<1>(GetParam()));
    BigArena arena{working.value_size(), working.block_size(), 2, [](Byte *value)
        {
            std::memmove(value, value + g_padding, g_bf_length + 2);
            value[0] ^= 0xff;
        }};
    const std::vector<Byte> bytes = wide_float_pattern();
    std::vector<Byte> expected(bytes.begin() + g_padding, bytes.end());
    expected[0] ^= 0xff;
    BigFloatProduct product{arena};
    std::copy(bytes.begin(), bytes.end(), product.data());
    const Byte *const address = product.data();
    ASSERT_EQ(2U, arena.high_water_mark());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);

    const BigFloat canonical{std::move(product)};

    EXPECT_EQ(address, canonical.data());
    EXPECT_EQ(expected, encoded_value(canonical));
    BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(address + arena.block_size(), companion.data());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
}

TEST_P(TestBigFloatProductOwnership, temporaryRewindsPreserveLiveProductsAndReleaseLocalOwners)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    const std::vector<Byte> bytes = wide_float_pattern();
    BigFloatProduct product{arena};
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigStackMark mark = save_stack();
    {
        BigStackSaver saved;
        bn_stack().allocate_double();
        bf_stack().allocate_double();
        const BigFloatProduct local{temporary, product};
        EXPECT_EQ(bytes, product_bytes(local));
    }
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bytes, product_bytes(product));
    const std::size_t high_water = temporary.high_water_mark();
    BigArena::Allocation first = temporary.allocate_double();
    BigArena::Allocation second = temporary.allocate_double();
    EXPECT_EQ(high_water, temporary.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLifetimes, TestBigFloatProductOwnership,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

namespace
{

struct BigFloatOperatorProductInput
{
    const char *name;
    int left;
    int right;
    int left_exponent;
    int right_exponent;
    int product;
    int product_exponent;
    int squared;
    int squared_exponent;
    bool shared;
};

void PrintTo(const BigFloatOperatorProductInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<Byte> integer_float_bytes(const int length, const int value, const int exponent)
{
    std::vector<Byte> bytes(length + 2);
    bytes[length - 2] = static_cast<Byte>(value);
    bytes[length - 1] = value < 0 ? 0xff : 0;
    bytes[length] = static_cast<Byte>(exponent);
    bytes[length + 1] = static_cast<Byte>(static_cast<unsigned>(exponent) >> 8);
    return bytes;
}

} // namespace

class TestBigFloatOperatorProducts
    : public TestBigFloatValue,
      public testing::WithParamInterface<std::tuple<int, bool, BigFloatOperatorProductInput>>
{
protected:
    void SetUp() override;
};

void TestBigFloatOperatorProducts::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatOperatorProducts, preservesExactBytesOperandsAndPairedAllocationReductionAndRelease)
{
    const bool squared = std::get<1>(GetParam());
    const BigFloatOperatorProductInput &input = std::get<2>(GetParam());
    BigArena &working = bf_arena(BigLifetime::WORKING);
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    const std::vector<Byte> left_bytes = integer_float_bytes(g_bf_length, input.left, input.left_exponent);
    const std::vector<Byte> right_bytes = integer_float_bytes(g_bf_length, input.right, input.right_exponent);
    BigFloat left = BigFloat::from_legacy(persistent, left_bytes.data(), left_bytes.size());
    BigFloat distinct_right = BigFloat::from_legacy(persistent, right_bytes.data(), right_bytes.size());
    const BigFloat &right = input.shared ? left : distinct_right;
    std::fill(left.data() + left_bytes.size(), left.data() + persistent.block_size(), Byte{0x5a});
    std::fill(distinct_right.data() + right_bytes.size(), distinct_right.data() + persistent.block_size(), Byte{0x6c});
    const std::vector<Byte> original_left(left.data(), left.data() + persistent.block_size());
    const std::vector<Byte> original_right(distinct_right.data(), distinct_right.data() + persistent.block_size());
    BigArena::Allocation guard = working.allocate_single();
    std::fill_n(guard.data(), working.block_size(), Byte{0x3c});
    std::fill_n(g_bf_tmp_copy1.data(), 2 * working.block_size(), Byte{0x69});
    std::fill_n(g_bf_tmp_copy2.data(), 2 * working.block_size(), Byte{0x96});
    BigArena::Allocation reservation = working.allocate_double();
    Byte *const address = reservation.data();
    std::fill_n(address, 2 * working.block_size(), Byte{0xa5});
    reservation.reset();
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();
    const int expected_value = squared ? input.squared : input.product;
    const int expected_exponent = squared ? input.squared_exponent : input.product_exponent;
    const std::vector<Byte> expected = integer_float_bytes(g_bf_length, expected_value, expected_exponent);
    const std::vector<Byte> wide = integer_float_bytes(g_r_bf_length, expected_value, expected_exponent);
    std::vector<Byte> expected_slot(2 * working.block_size());
    std::copy(wide.begin(), wide.end(), expected_slot.begin());
    std::copy(expected.begin(), expected.end(), expected_slot.begin());
    expected_slot.resize(working.block_size());
    {
        const BigFloat result = squared ? left.sqr() : left * right;

        EXPECT_EQ(address, result.data());
        EXPECT_EQ(&working, &result.arena());
        EXPECT_EQ(high_water, working.high_water_mark());
        EXPECT_EQ(mark, save_stack());
        EXPECT_EQ(expected, encoded_value(result));
        EXPECT_EQ(expected_slot, std::vector<Byte>(result.data(), result.data() + working.block_size()));
        EXPECT_EQ(original_left, std::vector<Byte>(left.data(), left.data() + persistent.block_size()));
        EXPECT_EQ(
            original_right, std::vector<Byte>(distinct_right.data(), distinct_right.data() + persistent.block_size()));
        EXPECT_EQ(std::vector<Byte>(working.block_size(), 0x3c),
            std::vector<Byte>(guard.data(), guard.data() + working.block_size()));
        EXPECT_EQ(std::vector<Byte>(2 * working.block_size() - expected.size(), 0x69),
            std::vector<Byte>(
                g_bf_tmp_copy1.data() + expected.size(), g_bf_tmp_copy1.data() + 2 * working.block_size()));
        EXPECT_EQ(std::vector<Byte>(2 * working.block_size() - expected.size(), 0x96),
            std::vector<Byte>(
                g_bf_tmp_copy2.data() + expected.size(), g_bf_tmp_copy2.data() + 2 * working.block_size()));
        BigArena::Allocation companion = working.allocate_single();
        EXPECT_EQ(address + working.block_size(), companion.data());
        EXPECT_EQ(high_water, working.high_water_mark());
        std::fill_n(companion.data(), working.block_size(), Byte{0xc3});
        EXPECT_EQ(expected, encoded_value(result));
    }
    BigArena::Allocation reused = working.allocate_double();
    EXPECT_EQ(address, reused.data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndInputs, TestBigFloatOperatorProducts,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708), testing::Bool(),
        testing::Values(BigFloatOperatorProductInput{"Positive", 2, 3, 0, 0, 6, 0, 4, 0, false},
            BigFloatOperatorProductInput{"NegativeLeft", -2, 3, 0, 0, -6, 0, 4, 0, false},
            BigFloatOperatorProductInput{"NegativeRight", 2, -3, 0, 0, -6, 0, 4, 0, false},
            BigFloatOperatorProductInput{"NegativeBoth", -2, -3, 0, 0, 6, 0, 4, 0, false},
            BigFloatOperatorProductInput{"LargeExponents", 2, 3, 500, 700, 6, 1200, 4, 1000, false},
            BigFloatOperatorProductInput{"SmallExponents", -2, 3, -500, -700, -6, -1200, 4, -1000, false},
            BigFloatOperatorProductInput{"ZeroLeft", 0, 3, 0, 700, 0, 0, 0, 0, false},
            BigFloatOperatorProductInput{"ZeroRight", -2, 0, -500, 0, 0, 0, 4, -1000, false},
            BigFloatOperatorProductInput{"Shared", -3, -3, 1000, 1000, 9, 2000, 9, 2000, true})));

} // namespace id::test
