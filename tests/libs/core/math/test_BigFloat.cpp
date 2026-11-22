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
#include <ostream>
#include <string>
#include <string_view>
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

static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().to_float()), LDouble>);
static_assert(std::is_same_v<decltype(std::declval<BigFloat &>() = std::declval<const BigFloat &>()), BigFloat &>);
static_assert(std::is_same_v<decltype(std::declval<BigFloat &>() = LDouble{}), BigFloat &>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>() / std::declval<const BigFloat &>()), BigFloat>);
static_assert(std::is_same_v<decltype(std::declval<BigFloat &>() /= U16{}), BigFloat &>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().exp()), BigFloat>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().log()), BigFloat>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().sin_cos()), std::pair<BigFloat, BigFloat>>);
static_assert(
    std::is_same_v<decltype(std::declval<const BigFloat &>().atan2(std::declval<const BigFloat &>())), BigFloat>);

std::vector<Byte> packed_value(const BigFloat &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bf_length + 2));
    value.pack_bf(result.data(), result.size());
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

    const BigFloat value{0};

    EXPECT_EQ("0.0", value.to_string());
}

TEST_F(TestBigFloatValue, automaticPrecisionPreservesNativeScalarBits)
{
    init_bf_dec(3);
    const LDouble input = std::nextafter(LDouble{1}, LDouble{2});

    const BigFloat value{input};

    EXPECT_EQ(input, value.to_float());
}

TEST_F(TestBigFloatValue, sameByteLayoutPrecisionIncreaseReconfiguresOwningCorners)
{
    init_bf_dec(20);
    g_bf_x_min = 1;
    const int length = g_bf_length;

    init_bf_dec(21);

    ASSERT_EQ(length, g_bf_length);
    const BigFloat updated{"1." + std::string(28, '0') + '1'};
    ASSERT_NE(BigFloat{1}, updated);
    g_bf_x_min = updated;
    EXPECT_EQ(updated, g_bf_x_min);
}

TEST_F(TestBigFloatValue, parsesTheSpecifiedView)
{
    const std::string text{"-1.25x"};

    const BigFloat value{std::string_view{text.data(), 5}};

    EXPECT_EQ("-1.25", value.to_string());
}

enum class BigFloatStorageProcedure
{
    COPY,
    CLEAR
};

struct BigFloatStorageInput
{
    BigFloatStorageProcedure operation;
    LDouble source;
    LDouble expected;
};

class TestBigFloatStorageProcedures : public TestBigFloatValue,
                                      public testing::WithParamInterface<std::tuple<int, BigFloatStorageInput>>
{
};

TEST_P(TestBigFloatStorageProcedures, copiesOrClearsWithoutChangingTheSource)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatStorageInput &input = std::get<1>(GetParam());
    const BigFloat source{input.source};
    const BigFloat original{input.source};
    const BigFloat expected{input.expected};
    BigFloat destination{-2.5L};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};

    switch (input.operation)
    {
    case BigFloatStorageProcedure::COPY:
        EXPECT_EQ(&destination, &(destination = source));
        break;
    case BigFloatStorageProcedure::CLEAR:
        EXPECT_EQ(&destination, &(destination = 0));
        break;
    }

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndOperations, TestBigFloatStorageProcedures,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatStorageInput{BigFloatStorageProcedure::COPY, 1.25L, 1.25L},
            BigFloatStorageInput{BigFloatStorageProcedure::COPY, -1.25L, -1.25L},
            BigFloatStorageInput{BigFloatStorageProcedure::COPY, 0.0L, 0.0L},
            BigFloatStorageInput{BigFloatStorageProcedure::CLEAR, -1.25L, 0.0L})));

class TestBigFloatSign : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigFloatSign, comparesWithZeroWithoutChangingValueOrStorage)
{
    init_bf_length(std::get<0>(GetParam()));
    const int sign = std::get<1>(GetParam());
    const BigFloat value{static_cast<LDouble>(sign) / 2};
    const BigFloat original{value};
    const BigFloat zero{0};

    EXPECT_EQ(sign, value.sign());
    EXPECT_EQ(sign == 0, value.is_zero());
    EXPECT_EQ(sign < 0, value < zero);
    EXPECT_EQ(sign == 0, value == zero);
    EXPECT_EQ(sign > 0, value > zero);
    EXPECT_EQ(sign != 0, value != zero);
    EXPECT_EQ(original, value);
}

INSTANTIATE_TEST_SUITE_P(
    PrecisionsAndSigns, TestBigFloatSign, testing::Combine(testing::Values(4, 64, 708), testing::Values(-1, 0, 1)));

struct BigFloatComparisonInput
{
    LDouble left;
    LDouble right;
};

class TestBigFloatComparison : public TestBigFloatValue,
                               public testing::WithParamInterface<std::tuple<int, BigFloatComparisonInput>>
{
};

TEST_P(TestBigFloatComparison, comparesNumericallyAndPreservesBothOperands)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatComparisonInput &input = std::get<1>(GetParam());
    const BigFloat left{input.left};
    const BigFloat right{input.right};
    const BigFloat original_left{left};
    const BigFloat original_right{right};

    EXPECT_LT(left, right);
    EXPECT_GT(right, left);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigFloatComparison,
    testing::Combine(testing::Values(4, 64, 708),
        testing::Values(BigFloatComparisonInput{-1.25L, 0.0L}, BigFloatComparisonInput{0.5L, 1.25L},
            BigFloatComparisonInput{-512.0L, -1.5L}, BigFloatComparisonInput{1.25L, 2.25L},
            BigFloatComparisonInput{-1.5L, -1.25L})));

class TestBigFloatNegation : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatNegation, preservesSourceAndSupportsAliasedAssignment)
{
    init_bf_length(GetParam());
    BigFloat source{1.0L};
    const BigFloat original{1.0L};

    EXPECT_EQ(-1.0L, (-source).to_float());
    EXPECT_EQ(original, source);
    source = -source;
    EXPECT_EQ(-1.0L, source.to_float());
    source = -source;
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatNegation, testing::Values(4, 64, 708));

struct BigFloatParseCase
{
    std::string_view text;
    LDouble expected;
};

class TestBigFloatParseInterface : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, BigFloatParseCase>>
{
};

TEST_P(TestBigFloatParseInterface, constructsTheExpectedValueUsingExistingWorkspace)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatParseCase &input = std::get<1>(GetParam());
    const BigFloat expected{input.expected};
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    const BigFloat value{input.text};

    EXPECT_EQ(expected, value);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigFloatParseInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatParseCase{"1.25", 1.25L}, BigFloatParseCase{"-1.25", -1.25L},
            BigFloatParseCase{"+1.25", 1.25L}, BigFloatParseCase{std::string_view{"1.25x", 4}, 1.25L},
            BigFloatParseCase{"1.25e2", 125}, BigFloatParseCase{"1.25E+2tail", 125}, BigFloatParseCase{"125e-2", 1.25L},
            BigFloatParseCase{"0.5", 0.5L}, BigFloatParseCase{"1.00390625", 1.00390625L},
            BigFloatParseCase{"junk1.25", 1.25L}, BigFloatParseCase{"1.25e+", 1.25L}, BigFloatParseCase{"0", 0},
            BigFloatParseCase{"-0", 0}, BigFloatParseCase{"", 0}, BigFloatParseCase{"+", 0}, BigFloatParseCase{"-", 0},
            BigFloatParseCase{".", 0}, BigFloatParseCase{"garbage", 0}, BigFloatParseCase{"1.25x", 0})));

class TestBigFloatScalarConstruction : public TestBigFloatValue,
                                       public testing::WithParamInterface<std::tuple<int, LDouble>>
{
};

TEST_P(TestBigFloatScalarConstruction, preservesTheNativeValue)
{
    const auto [length, input] = GetParam();
    init_bf_length(length);

    const BigFloat value{input};

    EXPECT_EQ(input, value.to_float());
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigFloatScalarConstruction,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values<LDouble>(0, -0.0L, 1, -1, 1.25L, -1.25L, 0.5L, -0.5L, 255, 256, 65536,
            std::numeric_limits<S32>::max(), std::numeric_limits<S32>::min(), std::ldexp(1.0L, 1023),
            -std::ldexp(1.0L, 1023), std::ldexp(1.0L, -1000))));

struct BigFloatOutputInput
{
    const char *text;
    LDouble expected;
};

class TestBigFloatOutputInterface : public TestBigFloatValue,
                                    public testing::WithParamInterface<std::tuple<int, BigFloatOutputInput>>
{
};

TEST_P(TestBigFloatOutputInterface, observesConstValuesWithoutChangingThem)
{
    const auto [length, input] = GetParam();
    init_bf_length(length);
    const BigFloat value{input.text};
    const BigFloat original{value};
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;

    const LDouble actual = value.to_float();

    EXPECT_EQ(input.expected, actual);
    EXPECT_EQ(std::signbit(input.expected), std::signbit(actual));
    EXPECT_EQ(original, value);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigFloatOutputInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatOutputInput{"0", 0}, BigFloatOutputInput{"7.5", 7.5L},
            BigFloatOutputInput{"-7.5", -7.5L},
            BigFloatOutputInput{"1e10000", std::numeric_limits<LDouble>::infinity()},
            BigFloatOutputInput{"-1e10000", -std::numeric_limits<LDouble>::infinity()},
            BigFloatOutputInput{"1e-10000", 0.0L}, BigFloatOutputInput{"-1e-10000", -0.0L})));

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
    EXPECT_EQ(static_cast<std::size_t>(g_bf_length + 2), packed_value(value).size());
}

TEST_F(TestBigFloatValue, supportsTheLargestLegacyLayout)
{
    init_bf_length(708);
    const BigFloat value{"-1.25"};

    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_EQ(static_cast<std::size_t>(g_bf_length + 2), packed_value(value).size());
}

class TestBigFloatHexDump : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, bool>>
{
};

TEST_P(TestBigFloatHexDump, printsAnOwningValueAndPreservesIt)
{
    const int length = std::get<0>(GetParam());
    init_bf_length(length);
    ASSERT_EQ(length + 4, g_bf_length);
    const bool negative = std::get<1>(GetParam());
    const BigFloat value{negative ? -1.25L : 0.0L};
    const BigFloat original{value};
    std::string expected;
    for (int i = 0; i < g_bf_length - 3; ++i)
    {
        expected += "00 ";
    }
    expected += negative ? "C0 FE FF  e 0000\n" : "00 00 00  e 0000\n";

    testing::internal::CaptureStdout();
    value.hex_dump();
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);
    EXPECT_EQ(original, value);
}

INSTANTIATE_TEST_SUITE_P(
    PrecisionsAndSigns, TestBigFloatHexDump, testing::Combine(testing::Values(4, 708), testing::Bool()));

TEST_F(TestBigFloatValue, copiesValuesBelowMachineRange)
{
    constexpr std::string_view TEXT{"-1.23456789012345678901234567890123456789e-10000"};
    BigFloat source{TEXT};
    const BigFloat value{source};

    source = 0;

    EXPECT_LT(value, BigFloat{0});
    EXPECT_EQ(0.0L, value.to_float());
    EXPECT_EQ(BigFloat{TEXT}, value);
}

struct BigFloatUnpackInput
{
    int bn_length;
    int stored_length;
    std::vector<Byte> source;
    std::vector<Byte> expected;
};

class TestBigFloatUnpack : public TestBigFloatValue, public testing::WithParamInterface<BigFloatUnpackInput>
{
};

TEST_P(TestBigFloatUnpack, preservesDestinationStorageSourceCursorAndPrecision)
{
    const BigFloatUnpackInput &input = GetParam();
    init_bf_length(input.bn_length);
    BigFloat destination{0};
    std::vector<Byte> source{0xaa};
    source.insert(source.end(), input.source.begin(), input.source.end());
    source.push_back(0xbb);
    const std::vector<Byte> original = source;
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};

    const Byte *const cursor = BigFloat::unpack_bf(destination, source.data() + 1, input.stored_length);

    EXPECT_EQ(source.data() + source.size() - 1, cursor);
    EXPECT_EQ(input.expected, packed_value(destination));
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatUnpack,
    testing::Values(BigFloatUnpackInput{8, 8, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff, 0x34, 0x12},
                        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff, 0x34, 0x12}},
        BigFloatUnpackInput{4, 12, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xff, 0xfe, 0xff},
            {0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xff, 0xfe, 0xff}},
        BigFloatUnpackInput{4, 8, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff, 0x34, 0x12},
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xff, 0x34, 0x12}}));

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
    const BigFloat expected{original};
    free_bf_vars();

    const BigFloat copy{original};
    BigFloat moved{std::move(original)};
    original = copy;
    moved = std::move(original);

    init_bf_dec(40);
    EXPECT_EQ(expected, copy);
    EXPECT_EQ(expected, moved);
}

TEST_F(TestBigFloatValue, copyAssignmentPreservesTheValue)
{
    init_bf_dec(80);
    const BigFloat original{"1.23456789012345678901234567890123456789"};
    const BigFloat expected{original};
    BigFloat assigned{2.5L};

    assigned = original;

    EXPECT_EQ(expected, assigned);
    EXPECT_EQ(expected, original);
}

TEST_F(TestBigFloatValue, destructionAfterContextResetReleasesStorage)
{
    {
        const BigFloat value{"-1.25"};
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

class TestBigFloatAddition : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatAddition, preservesOperandsAndReusesStorageForCompoundAssignment)
{
    init_bf_length(GetParam());
    const BigFloat left{1.25L};
    const BigFloat right{-0.5L};
    const BigFloat original_left{1.25L};
    const BigFloat original_right{-0.5L};
    const BigFloat expected{0.75L};
    const BigFloat expected_self{2.5L};
    BigFloat left_alias{left};
    BigFloat right_alias{right};
    BigFloat result{left};

    EXPECT_EQ(&result, &(result += right));
    EXPECT_EQ(expected, result);
    result = left;
    EXPECT_EQ(&result, &(result += result));
    EXPECT_EQ(expected_self, result);

    EXPECT_EQ(expected, left + right);
    left_alias = left_alias + right;
    right_alias = left + right_alias;
    EXPECT_EQ(expected, left_alias);
    EXPECT_EQ(expected, right_alias);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatAddition, testing::Values(4, 64, 708));

class TestBigFloatSubtraction : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatSubtraction, preservesOperandsAndReusesStorageForCompoundAssignment)
{
    init_bf_length(GetParam());
    const BigFloat left{1.25L};
    const BigFloat right{-0.5L};
    const BigFloat original_left{1.25L};
    const BigFloat original_right{-0.5L};
    const BigFloat expected{1.75L};
    const BigFloat expected_self{0.0L};
    BigFloat left_alias{left};
    BigFloat right_alias{right};
    BigFloat result{left};

    EXPECT_EQ(&result, &(result -= right));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(&result, &(result -= result));
    EXPECT_EQ(expected_self, result);

    EXPECT_EQ(expected, left - right);
    left_alias = left_alias - right;
    right_alias = left - right_alias;
    EXPECT_EQ(expected, left_alias);
    EXPECT_EQ(expected, right_alias);
    EXPECT_EQ(expected, (right - left).abs());
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatSubtraction, testing::Values(4, 64, 708));

class TestBigFloatDoubling : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatDoubling, preservesSourceAndReusesStorageForCompoundAssignment)
{
    init_bf_length(GetParam());
    const BigFloat source{128.0L};
    const BigFloat original{128.0L};
    BigFloat result{source};

    EXPECT_EQ(&result, &(result += result));
    EXPECT_EQ(256.0L, result.to_float());

    EXPECT_EQ(256.0L, (source + source).to_float());
    result = source;
    result = result + result;
    EXPECT_EQ(256.0L, result.to_float());
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatDoubling, testing::Values(4, 64, 708));

class TestBigFloatHalving : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatHalving, preservesSourceAndSupportsAliasedAssignment)
{
    init_bf_length(GetParam());
    const BigFloat source{1.0L};
    const BigFloat original{1.0L};
    const BigFloat expected{0.5L};
    BigFloat result{source};

    EXPECT_EQ(expected, source.half());
    result = result.half();
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatHalving, testing::Values(4, 64, 708));

class TestBigFloatReciprocal : public TestBigFloatValue, public testing::WithParamInterface<LDouble>
{
};

TEST_P(TestBigFloatReciprocal, preservesItsOperand)
{
    const BigFloat source{GetParam()};
    const BigFloat original{GetParam()};
    const BigFloat one{1.0L};
    const BigFloat expected{GetParam() < 0 ? -0.5L : 0.5L};

    const BigFloat result = one / source;

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Signs, TestBigFloatReciprocal, testing::Values(-2.0L, 2.0L));

enum class DivisionDestination
{
    SEPARATE,
    NUMERATOR,
    DENOMINATOR,
    SHARED_INPUT
};

class TestBigFloatDivision : public TestBigFloatValue, public testing::WithParamInterface<DivisionDestination>
{
};

TEST_P(TestBigFloatDivision, supportsAliasedAssignmentAndPreservesOtherOperands)
{
    BigFloat numerator{-8.0L};
    BigFloat denominator{-4.0L};
    BigFloat result{0};
    const DivisionDestination target = GetParam();
    const bool shared_input = target == DivisionDestination::SHARED_INPUT;
    const BigFloat &divisor = shared_input ? numerator : denominator;
    BigFloat &numerator_or_result = target == DivisionDestination::SEPARATE ? result : numerator;
    BigFloat &output = target == DivisionDestination::DENOMINATOR ? denominator : numerator_or_result;
    const BigFloat expected{shared_input ? 1.0L : 2.0L};
    const BigFloat original_numerator{-8.0L};
    const BigFloat original_denominator{-4.0L};

    output = numerator / divisor;

    EXPECT_EQ(expected, output);
    if (&output != &numerator)
    {
        EXPECT_EQ(original_numerator, numerator);
    }
    if (&output != &denominator)
    {
        EXPECT_EQ(original_denominator, denominator);
    }
}

INSTANTIATE_TEST_SUITE_P(Destinations, TestBigFloatDivision,
    testing::Values(DivisionDestination::SEPARATE, DivisionDestination::NUMERATOR, DivisionDestination::DENOMINATOR,
        DivisionDestination::SHARED_INPUT));

struct BigFloatExponentialInput
{
    LDouble input;
    LDouble expected;
    LDouble tolerance;
};

class TestBigFloatExponential : public TestBigFloatValue, public testing::WithParamInterface<BigFloatExponentialInput>
{
};

TEST_P(TestBigFloatExponential, returnsAnOwningResultAndPreservesTheSource)
{
    const BigFloatExponentialInput &test = GetParam();
    const BigFloat source{test.input};
    const BigFloat original{source};

    const BigFloat result = source.exp();

    EXPECT_NEAR(test.expected, result.to_float(), test.tolerance);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatExponential,
    testing::Values(BigFloatExponentialInput{0.0L, 1.0L, 0.0L},
        BigFloatExponentialInput{0.375L, 1.4549914146182013L, 1e-12L},
        BigFloatExponentialInput{-0.625L, 0.5352614285189903L, 1e-12L}));

class TestBigFloatLogarithm : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, bool>>
{
};

TEST_P(TestBigFloatLogarithm, supportsSeparateAndAliasedAssignment)
{
    init_bf_length(64);
    const int input = std::get<0>(GetParam());
    const bool alias = std::get<1>(GetParam());
    BigFloat source{static_cast<LDouble>(input)};
    const BigFloat original{source};
    BigFloat result{0};
    BigFloat &output = alias ? source : result;
    const LDouble expected = input > 0 ? 0.0L : -std::numeric_limits<LDouble>::infinity();

    output = source.log();

    EXPECT_EQ(expected, output.to_float());
    if (!alias)
    {
        EXPECT_EQ(original, source);
    }
}

INSTANTIATE_TEST_SUITE_P(
    InputsAndAliases, TestBigFloatLogarithm, testing::Combine(testing::Values(1, 0, -1), testing::Bool()));

class TestBigFloatSineCosine : public TestBigFloatValue, public testing::WithParamInterface<LDouble>
{
};

TEST_P(TestBigFloatSineCosine, returnsOwningValuesAndPreservesTheSource)
{
    init_big_pi();
    const LDouble angle = GetParam();
    const BigFloat source{angle};
    const BigFloat original{source};

    const std::pair<BigFloat, BigFloat> result = source.sin_cos();

    EXPECT_NEAR(std::sin(angle), result.first.to_float(), 1e-12L);
    EXPECT_NEAR(std::cos(angle), result.second.to_float(), 1e-12L);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Angles, TestBigFloatSineCosine, testing::Values(0.0L, 0.375L, -0.625L));

class TestBigFloatSineCosineAlias : public TestBigFloatValue, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigFloatSineCosineAlias, supportsAssignmentToEitherOperand)
{
    init_big_pi();
    constexpr LDouble angle = -0.625L;
    BigFloat source{angle};
    BigFloat other{0};
    BigFloat &sine = GetParam() ? source : other;
    BigFloat &cosine = GetParam() ? other : source;

    std::tie(sine, cosine) = source.sin_cos();

    EXPECT_NEAR(std::sin(angle), sine.to_float(), 1e-12L);
    EXPECT_NEAR(std::cos(angle), cosine.to_float(), 1e-12L);
}

INSTANTIATE_TEST_SUITE_P(Destinations, TestBigFloatSineCosineAlias, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "Sine" : "Cosine"; });

class TestBigFloatArctangent2 : public TestBigFloatValue,
                                public testing::WithParamInterface<std::tuple<LDouble, LDouble>>
{
};

TEST_P(TestBigFloatArctangent2, returnsAnOwningResultAndPreservesOperands)
{
    init_big_pi();
    const LDouble y = std::get<0>(GetParam());
    const LDouble x = std::get<1>(GetParam());
    const BigFloat ny{y};
    const BigFloat nx{x};
    const BigFloat original_y{ny};
    const BigFloat original_x{nx};

    const BigFloat result = ny.atan2(nx);

    EXPECT_NEAR(std::atan2(y, x), result.to_float(), 1e-11L);
    EXPECT_EQ(original_y, ny);
    EXPECT_EQ(original_x, nx);
}

INSTANTIATE_TEST_SUITE_P(QuadrantsAndAxes, TestBigFloatArctangent2,
    testing::Values(std::make_tuple(1.0L, -2.0L), std::make_tuple(-1.0L, 0.0L), std::make_tuple(2.0L, 1.0L)));

class TestBigFloatArctangent2Alias : public TestBigFloatValue, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigFloatArctangent2Alias, supportsAssignmentToEitherOperand)
{
    init_big_pi();
    BigFloat ny{-1.0L};
    BigFloat nx{-2.0L};
    BigFloat &result = GetParam() ? ny : nx;
    const BigFloat &other = GetParam() ? nx : ny;
    const BigFloat original{other};

    result = ny.atan2(nx);

    EXPECT_NEAR(std::atan2(-1.0L, -2.0L), result.to_float(), 1e-11L);
    EXPECT_EQ(original, other);
}

INSTANTIATE_TEST_SUITE_P(Destinations, TestBigFloatArctangent2Alias, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "Y" : "X"; });

TEST_P(TestBigFloatArithmetic, legacyArithmeticSamplesPreserveOperands)
{
    const BigFloat left{"-7.5"};
    const BigFloat right{"2.5"};
    const BigFloat original_left{left};
    const BigFloat original_right{right};
    BigFloat result{0};

    result = left + right;
    EXPECT_EQ("-5.0", result.to_string(8));
    result = left - right;
    EXPECT_EQ("-10.0", result.to_string(8));
    result = left * right;
    EXPECT_EQ("-18.75", result.to_string(8));
    result = left / right;
    EXPECT_EQ("-3.0", result.to_string(8));

    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigFloatArithmetic, inPlaceAdditionAndSubtractionPreserveTheRightOperand)
{
    BigFloat left{"-7.5"};
    const BigFloat right{"2.5"};
    const BigFloat original_right{"2.5"};

    left += right;
    EXPECT_EQ("-5.0", left.to_string(8));
    left -= right;
    EXPECT_EQ("-7.5", left.to_string(8));
    left += left;
    EXPECT_EQ("-15.0", left.to_string(8));
    left -= left;
    EXPECT_EQ("0.0", left.to_string(8));

    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigFloatArithmetic, arithmeticPreservesOperandsAndReturnsIndependentValues)
{
    const BigFloat left{"-7.5"};
    const BigFloat right{"2.5"};
    const BigFloat original_left{left};
    const BigFloat original_right{right};

    BigFloat sum = left + right;
    EXPECT_EQ("-5.0", sum.to_string(8));
    EXPECT_EQ("-10.0", (left - right).to_string(8));
    EXPECT_EQ("-18.75", (left * right).to_string(8));
    EXPECT_EQ("-3.0", (left / right).to_string(8));
    sum = 0;

    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigFloatArithmetic, negationPreservesItsOperand)
{
    const BigFloat value{"-7.5"};
    const BigFloat original{"-7.5"};
    const BigFloat expected{"7.5"};
    const BigFloat zero{0};

    EXPECT_EQ(expected, -value);
    EXPECT_EQ(original, value);
    EXPECT_EQ(zero, -zero);
    EXPECT_EQ(original, -(-value));
}

TEST_P(TestBigFloatArithmetic, compoundAssignmentChangesOnlyItsDestination)
{
    BigFloat value{"-7.5"};
    const BigFloat right{"2.5"};
    const BigFloat original_right{right};
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
    EXPECT_EQ(original_right, right);
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
    const BigFloat original_left{left};
    const BigFloat original_right{right};
    const BigFloat original_divisor{divisor};
    BigFloat expected{left};
    expected += right;
    expected /= divisor;
    expected *= left;
    expected -= right;

    const BigFloat result = ((left + right) / divisor) * left - right;
    const BigFloat unrelated = BigFloat{"-8"} * BigFloat{"-9"};

    EXPECT_EQ(expected, result);
    EXPECT_EQ("72.0", unrelated.to_string(8));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    EXPECT_EQ(original_divisor, divisor);
}

TEST_P(TestBigFloatArithmetic, zeroOperandKeepsLegacyResults)
{
    const BigFloat zero{0};
    const BigFloat value{"-7.5"};

    EXPECT_EQ(value, value + zero);
    EXPECT_EQ(value, value - zero);
    EXPECT_EQ(-value, zero - value);
    EXPECT_EQ(zero, value * zero);
    EXPECT_EQ(zero, zero / value);
    EXPECT_EQ(zero, zero / zero);
}

class TestBigFloatScalarMultiply : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, LDouble, U16>>
{
};

TEST_P(TestBigFloatScalarMultiply, returnsExactResultAndPreservesSource)
{
    init_bf_length(std::get<0>(GetParam()));
    const LDouble input = std::get<1>(GetParam());
    const U16 factor = std::get<2>(GetParam());
    const BigFloat source{input};
    const BigFloat original{input};

    const BigFloat result = source * factor;

    EXPECT_EQ(input * factor, result.to_float());
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsValuesAndFactors, TestBigFloatScalarMultiply,
    testing::Combine(testing::Values(4, 32, 708), testing::Values(0.0L, -0.5L, 1.25L),
        testing::Values(U16{1}, U16{2}, U16{255}, U16{256}, U16{32768}, U16{65535})));

TEST_P(TestBigFloatArithmetic, unsignedScalingSupportsAliasedAssignment)
{
    const BigFloat source{2.0L};
    const BigFloat original{2.0L};
    const BigFloat expected{131070.0L};
    const BigFloat zero{0.0L};
    BigFloat result{source};

    EXPECT_EQ(expected, source * U16{65535});
    EXPECT_EQ(zero, source * U16{0});
    result = result * U16{65535};

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

class TestBigFloatScalarDivision : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, LDouble, U16>>
{
};

TEST_P(TestBigFloatScalarDivision, dividesInPlaceAndReusesStorage)
{
    init_bf_length(std::get<0>(GetParam()));
    const LDouble quotient = std::get<1>(GetParam());
    const U16 divisor = std::get<2>(GetParam());
    const BigFloat source{quotient * divisor};
    const BigFloat original{source};
    BigFloat value{source};

    EXPECT_EQ(&value, &(value /= divisor));

    EXPECT_EQ(quotient, value.to_float());
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatScalarDivision,
    testing::Combine(testing::Values(4, 32, 708), testing::Values(0.0L, std::ldexp(-3.0L, -403), std::ldexp(5.0L, 398)),
        testing::Values(U16{1}, U16{2}, U16{255}, U16{256}, U16{32768}, U16{65535})));

class TestBigFloatAbs : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, const char *>>
{
};

TEST_P(TestBigFloatAbs, absoluteValuePreservesItsOperand)
{
    init_bf_length(std::get<0>(GetParam()));
    const char *text = std::get<1>(GetParam());
    const BigFloat source{text};
    const BigFloat original{text};
    const BigFloat expected{*text == '-' ? text + 1 : text};

    const BigFloat member_result = source.abs();
    const BigFloat result = abs(source);
    BigFloat assigned{source};
    assigned = abs(assigned);

    EXPECT_EQ(expected, member_result);
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected, assigned);
    EXPECT_EQ(original, source);
}

TEST_P(TestBigFloatAbs, absoluteValueSupportsAliasedAssignment)
{
    init_bf_length(std::get<0>(GetParam()));
    const char *text = std::get<1>(GetParam());
    const BigFloat source{text};
    const BigFloat original{text};
    const BigFloat expected{*text == '-' ? text + 1 : text};
    BigFloat result{source};

    EXPECT_EQ(expected, source.abs());
    result = result.abs();

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndValues, TestBigFloatAbs,
    testing::Combine(testing::Values(4, 32, 708),
        testing::Values("0", "2.5", "-2.5", "1.2345678901234567890123456789e-400",
            "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400",
            "-1.2345678901234567890123456789e400")));

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatArithmetic, testing::Values(4, 32, 708));

class TestBigFloatOrdering : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigFloatOrdering, comparisonsUseNumericalValuesWithoutChangingOperands)
{
    constexpr std::array<const char *, 8> ORDERED{
        "-1e400", "-2", "-1e-400", "0", "1e-400", "1", "1.000000000000000000000000000001", "1e400"};
    const auto [i, j] = GetParam();
    const BigFloat left{ORDERED[i]};
    const BigFloat right{ORDERED[j]};
    const BigFloat original_left{left};
    const BigFloat original_right{right};

    EXPECT_EQ(i == j, left == right);
    EXPECT_EQ(i != j, left != right);
    EXPECT_EQ(i < j, left < right);
    EXPECT_EQ(i <= j, left <= right);
    EXPECT_EQ(i > j, left > right);
    EXPECT_EQ(i >= j, left >= right);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatOrdering, testing::Combine(testing::Range(0, 8), testing::Range(0, 8)));

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
    bool shared{};
    bool squared{};
};

void PrintTo(const BigFloatOperatorProductInput &input, std::ostream *out)
{
    *out << input.name;
}

BigFloat scaled_product_sample(const int value, const int exponent)
{
    return BigFloat{std::ldexp(static_cast<LDouble>(value), 8 * exponent)};
}

} // namespace

class TestBigFloatOperatorProducts : public TestBigFloatValue,
                                     public testing::WithParamInterface<std::tuple<int, BigFloatOperatorProductInput>>
{
protected:
    void SetUp() override;
};

void TestBigFloatOperatorProducts::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatOperatorProducts, preservesOperandsAndReturnsIndependentResults)
{
    const BigFloatOperatorProductInput &input = std::get<1>(GetParam());
    const BigFloat left = scaled_product_sample(input.left, input.left_exponent);
    const BigFloat distinct_right = scaled_product_sample(input.right, input.right_exponent);
    const BigFloat &right = input.shared ? left : distinct_right;
    const BigFloat original_left{left};
    const BigFloat original_right{distinct_right};
    const BigFloat guard{-7};
    const BigFloat expected = scaled_product_sample(input.product, input.product_exponent);
    {
        const BigFloat result = input.squared ? left.sqr() : left * right;

        EXPECT_EQ(expected, result);
        EXPECT_EQ(original_left, left);
        EXPECT_EQ(original_right, distinct_right);

        const BigFloat companion{-8};
        EXPECT_EQ(-7.0L, guard.to_float());
        EXPECT_EQ(-8.0L, companion.to_float());
        EXPECT_EQ(expected, result);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndInputs, TestBigFloatOperatorProducts,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigFloatOperatorProductInput{"Positive", 2, 3, 0, 0, 6, 0},
            BigFloatOperatorProductInput{"NegativeLeft", -2, 3, 0, 0, -6, 0},
            BigFloatOperatorProductInput{"NegativeRight", 2, -3, 0, 0, -6, 0},
            BigFloatOperatorProductInput{"NegativeBoth", -2, -3, 0, 0, 6, 0},
            BigFloatOperatorProductInput{"LargeExponents", 2, 3, 40, 60, 6, 100},
            BigFloatOperatorProductInput{"SmallExponents", -2, 3, -40, -60, -6, -100},
            BigFloatOperatorProductInput{"ZeroLeft", 0, 3, 0, 60, 0, 0},
            BigFloatOperatorProductInput{"ZeroRight", -2, 0, -40, 0, 0, 0},
            BigFloatOperatorProductInput{"Shared", -3, -3, 60, 60, 9, 120, true},
            BigFloatOperatorProductInput{"SquarePositive", 2, 0, 0, 0, 4, 0, false, true},
            BigFloatOperatorProductInput{"SquareNegative", -2, 0, 0, 0, 4, 0, false, true},
            BigFloatOperatorProductInput{"SquareLargeExponent", 2, 0, 40, 0, 4, 80, false, true},
            BigFloatOperatorProductInput{"SquareSmallExponent", -2, 0, -40, 0, 4, -80, false, true},
            BigFloatOperatorProductInput{"SquareZero", 0, 0, 0, 0, 0, 0, false, true},
            BigFloatOperatorProductInput{"SquareSharedSample", -3, 0, 60, 0, 9, 120, false, true})));

namespace
{

enum class LegacyMultiplyCall
{
    SAFE,
    SAFE_LEFT,
    SAFE_RIGHT
};

struct LegacyMultiplyInput
{
    const char *name;
    int left;
    int right;
    int left_exponent;
    int right_exponent;
    int product;
    int product_exponent;
    bool shared;
};

void PrintTo(const LegacyMultiplyInput &input, std::ostream *out)
{
    *out << input.name;
}

} // namespace

class TestBigFloatLegacyMultiply
    : public TestBigFloatValue,
      public testing::WithParamInterface<std::tuple<int, LegacyMultiplyCall, LegacyMultiplyInput>>
{
protected:
    void SetUp() override;
};

void TestBigFloatLegacyMultiply::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatLegacyMultiply, preservesOperandsAndTemporaryLifetime)
{
    const LegacyMultiplyCall call = std::get<1>(GetParam());
    const LegacyMultiplyInput &input = std::get<2>(GetParam());
    BigFloat left = scaled_product_sample(input.left, input.left_exponent);
    BigFloat right = scaled_product_sample(input.right, input.right_exponent);
    const BigFloat original_left{left};
    const BigFloat original_right{right};
    const BigFloat expected = scaled_product_sample(input.product, input.product_exponent);
    BigFloat separate{-5};
    BigFloat *destination = &separate;
    if (call == LegacyMultiplyCall::SAFE_LEFT)
    {
        destination = &left;
    }
    else if (call == LegacyMultiplyCall::SAFE_RIGHT)
    {
        destination = input.shared ? &left : &right;
    }
    const BigFloat survivor{-7};

    *destination = left * (input.shared ? left : right);

    EXPECT_EQ(expected, *destination);
    EXPECT_EQ(destination == &left ? expected : original_left, left);
    EXPECT_EQ(destination == &right ? expected : original_right, right);
    if (destination != &separate)
    {
        EXPECT_EQ(-5.0L, separate.to_float());
    }
    EXPECT_EQ(-7.0L, survivor.to_float());
}

INSTANTIATE_TEST_SUITE_P(LayoutsCallsAndInputs, TestBigFloatLegacyMultiply,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(LegacyMultiplyCall::SAFE, LegacyMultiplyCall::SAFE_LEFT, LegacyMultiplyCall::SAFE_RIGHT),
        testing::Values(LegacyMultiplyInput{"Positive", 2, 3, 0, 0, 6, 0, false},
            LegacyMultiplyInput{"NegativeLeft", -2, 3, 0, 0, -6, 0, false},
            LegacyMultiplyInput{"NegativeRight", 2, -3, 0, 0, -6, 0, false},
            LegacyMultiplyInput{"NegativeBoth", -2, -3, 0, 0, 6, 0, false},
            LegacyMultiplyInput{"LargeExponents", 2, 3, 40, 60, 6, 100, false},
            LegacyMultiplyInput{"SmallExponents", -2, 3, -40, -60, -6, -100, false},
            LegacyMultiplyInput{"ZeroLeft", 0, 3, 0, 60, 0, 0, false},
            LegacyMultiplyInput{"ZeroRight", -2, 0, -40, 0, 0, 0, false})));

INSTANTIATE_TEST_SUITE_P(LayoutsAndSharedCalls, TestBigFloatLegacyMultiply,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(LegacyMultiplyCall::SAFE, LegacyMultiplyCall::SAFE_LEFT),
        testing::Values(LegacyMultiplyInput{"Shared", -3, -3, 60, 60, 9, 120, true})));

namespace
{

enum class LegacySqrCall
{
    SAFE,
    SAFE_IN_PLACE,
    CACHE_X,
    CACHE_Y
};

struct LegacySqrInput
{
    const char *name;
    int source;
    int exponent;
    int squared;
    int squared_exponent;
};

void PrintTo(const LegacySqrInput &input, std::ostream *out)
{
    *out << input.name;
}

} // namespace

class TestBigFloatLegacySqr : public TestBigFloatValue,
                              public testing::WithParamInterface<std::tuple<int, LegacySqrCall, LegacySqrInput>>
{
protected:
    void SetUp() override;
};

void TestBigFloatLegacySqr::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatLegacySqr, preservesSourceCachesAndTemporaryLifetime)
{
    const LegacySqrCall call = std::get<1>(GetParam());
    const LegacySqrInput &input = std::get<2>(GetParam());
    BigFloat source = scaled_product_sample(input.source, input.exponent);
    const BigFloat original{source};
    const BigFloat expected = scaled_product_sample(input.squared, input.squared_exponent);
    g_tmp_sqr_x_bf = -2;
    g_tmp_sqr_y_bf = -3;
    BigFloat separate{-5};
    BigFloat *destination = &separate;
    if (call == LegacySqrCall::SAFE_IN_PLACE)
    {
        destination = &source;
    }
    else if (call == LegacySqrCall::CACHE_X)
    {
        destination = &g_tmp_sqr_x_bf;
    }
    else if (call == LegacySqrCall::CACHE_Y)
    {
        destination = &g_tmp_sqr_y_bf;
    }
    const BigFloat survivor{-7};

    *destination = source.sqr();

    EXPECT_EQ(expected, *destination);
    EXPECT_EQ(destination == &source ? expected : original, source);
    if (destination != &separate)
    {
        EXPECT_EQ(-5.0L, separate.to_float());
    }
    if (destination != &g_tmp_sqr_x_bf)
    {
        EXPECT_EQ(-2.0L, g_tmp_sqr_x_bf.to_float());
    }
    if (destination != &g_tmp_sqr_y_bf)
    {
        EXPECT_EQ(-3.0L, g_tmp_sqr_y_bf.to_float());
    }
    EXPECT_EQ(-7.0L, survivor.to_float());
}

INSTANTIATE_TEST_SUITE_P(LayoutsCallsAndInputs, TestBigFloatLegacySqr,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(LegacySqrCall::SAFE, LegacySqrCall::SAFE_IN_PLACE, LegacySqrCall::CACHE_X,
            LegacySqrCall::CACHE_Y),
        testing::Values(LegacySqrInput{"Positive", 2, 0, 4, 0}, LegacySqrInput{"Negative", -3, 0, 9, 0},
            LegacySqrInput{"LargeExponent", -2, 40, 4, 80}, LegacySqrInput{"SmallExponent", 3, -60, 9, -120},
            LegacySqrInput{"Zero", 0, 0, 0, 0})));

namespace
{

class TestBigFloat : public testing::Test
{
protected:
    TestBigFloat() :
        m_saved_bailout_value(g_user.bailout_value, 0),
        m_saved_bailout_test(g_bailout_test, Bailout::MOD),
        m_saved_bf_digits(g_bf_digits, 0),
        m_saved_fractal_type(g_fractal_type, FractalType::MANDEL)
    {
    }

    void SetUp() override
    {
        init_bf_dec(20);
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

} // namespace

struct BigFloatRampInput
{
    const char *name;
    bool logarithm;
    const char *input;
    const char *expected;
};

class TestBigFloatWorkspaceRamp : public TestBigFloat,
                                  public testing::WithParamInterface<std::tuple<int, BigFloatRampInput>>
{
};

TEST_P(TestBigFloatWorkspaceRamp, preservesOperandsAndRestoresPrecision)
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    init_big_pi();
    const BigFloatRampInput &test = std::get<1>(GetParam());
    const BigFloat input{test.input};
    const BigFloat original{input};
    const BigFloat one{1};
    const BigFloat original_pi{g_bf_pi};
    const int length = g_bf_length;
    const int bn_length = g_bn_length;

    const BigFloat result = test.logarithm ? input.log() : input.atan2(one);

    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(original_pi, g_bf_pi);
    EXPECT_EQ(original, input);
    EXPECT_EQ(test.expected, result.to_string(12));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndOperations, TestBigFloatWorkspaceRamp,
    testing::Combine(testing::Values(32, 64, 708),
        testing::Values(BigFloatRampInput{"LogTwo", true, "2", "0.69314718056"},
            BigFloatRampInput{"AtanHalf", false, "0.5", "0.463647609001"},
            BigFloatRampInput{"AtanTwo", false, "2", "1.10714871779"})),
    [](const testing::TestParamInfo<TestBigFloatWorkspaceRamp::ParamType> &info)
    { return std::string{std::get<1>(info.param).name} + '_' + std::to_string(std::get<0>(info.param)); });

struct BigFloatParseInput
{
    const char *name;
    std::string storage;
    std::size_t length;
    LDouble expected;
};

class TestBigFloatParse : public TestBigFloat, public testing::WithParamInterface<BigFloatParseInput>
{
};

TEST_P(TestBigFloatParse, parsesTheSpecifiedText)
{
    const std::string_view text{GetParam().storage.data(), GetParam().length};
    {
        const BigFloat value{text};
        EXPECT_EQ(GetParam().expected, value.to_float());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatParse,
    testing::Values(BigFloatParseInput{"PlainDecimal", "1.25", 4, 1.25L},
        BigFloatParseInput{"UnterminatedView", "1.25x", 4, 1.25L}, BigFloatParseInput{"Exponent", "1.25e2", 6, 125.0L}),
    [](const testing::TestParamInfo<BigFloatParseInput> &info) { return info.param.name; });

class TestBigFloatFormat : public TestBigFloat
{
protected:
    static BigFloat make_value(std::string_view text);
};

BigFloat TestBigFloatFormat::make_value(const std::string_view text)
{
    return BigFloat{text};
}

TEST_F(TestBigFloatFormat, generalFormatHandlesZero)
{
    const BigFloat value = make_value("0");

    EXPECT_EQ("0.0", value.to_string(5));
}

TEST_F(TestBigFloatFormat, resetClearsTheOwningValue)
{
    BigFloat value = make_value("1.25");
    EXPECT_EQ("1.25", value.to_string(5));

    value = BigFloat{};
    ASSERT_FALSE(value);
}

struct BigFloatFormatInput
{
    const char *name;
    const char *input;
    int decimals;
    const char *expected;
};

class TestBigFloatGeneralFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
};

TEST_P(TestBigFloatGeneralFormat, selectsNotationAndFormatsDecimal)
{
    const BigFloatFormatInput &input = GetParam();
    const BigFloat value = make_value(input.input);
    const BigFloat original{value};

    EXPECT_EQ(input.expected, value.to_string(input.decimals));
    EXPECT_EQ(original, value);
}

// Preserve the legacy carry truncation while refactoring decimal storage.
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatGeneralFormat,
    testing::Values(BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234"}, BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999"},
        BigFloatFormatInput{"SmallFixed", "0.00125", 8, "0.00125"},
        BigFloatFormatInput{"SmallScientific", "0.000125", 8, "1.25e-4"},
        BigFloatFormatInput{"LargeFixed", "125000", 8, "125000.0"},
        BigFloatFormatInput{"FewDigitsLargeFixed", "125000", 3, "125000.0"},
        BigFloatFormatInput{"LargeScientific", "1250000", 8, "1.25e6"},
        BigFloatFormatInput{"DefaultPrecision", "1.0000123456789012345", 0, "1.0000123456789012345"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

class TestBigFloatScientificFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
};

TEST_P(TestBigFloatScientificFormat, formatsScientificDecimal)
{
    const BigFloatFormatInput &input = GetParam();
    const BigFloat value = make_value(input.input);
    const BigFloat original{value};

    EXPECT_EQ(input.expected, value.to_scientific_string(input.decimals));
    EXPECT_EQ(original, value);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatScientificFormat,
    testing::Values(BigFloatFormatInput{"Zero", "0", 5, "0.0"},
        BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25e0"},
        BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25e0"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234e0"},
        BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235e0"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999e0"},
        BigFloatFormatInput{"SmallExponent", "0.000125", 8, "1.25e-4"},
        BigFloatFormatInput{"LargeExponent", "1250000", 8, "1.25e6"},
        BigFloatFormatInput{"SingleDigit", "1.25", 1, "1.0e0"},
        BigFloatFormatInput{"DefaultPrecision", "1.234567890123456789", 0, "1.234567890123456789e0"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

enum class BigFloatFormatMode
{
    GENERAL,
    SCIENTIFIC
};

class TestBigFloatFormatterStorage : public TestBigFloatFormat,
                                     public testing::WithParamInterface<std::tuple<int, BigFloatFormatMode, bool>>
{
};

TEST_P(TestBigFloatFormatterStorage, preservesValueAndPrecision)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatFormatMode mode = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    const BigFloat value = make_value(negative ? "-1.25" : "1.25");
    const BigFloat original{value};
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;
    std::string expected{negative ? "-1.25" : "1.25"};
    std::string actual;

    switch (mode)
    {
    case BigFloatFormatMode::GENERAL:
        actual = value.to_string(12);
        break;
    case BigFloatFormatMode::SCIENTIFIC:
        expected += "e0";
        actual = value.to_scientific_string(12);
        break;
    }

    EXPECT_EQ(expected, actual);
    EXPECT_EQ(original, value);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsModesAndSigns, TestBigFloatFormatterStorage,
    testing::Combine(testing::Values(4, 12, 64, 708),
        testing::Values(BigFloatFormatMode::GENERAL, BigFloatFormatMode::SCIENTIFIC),
        testing::Bool()));

class TestBigFloatFormatterUnderflow : public TestBigFloatFormat,
                                       public testing::WithParamInterface<decltype(&BigFloat::to_string)>
{
};

TEST_P(TestBigFloatFormatterUnderflow, formatsNonzeroValueBelowNativeRangeAsZero)
{
    const BigFloat value = make_value("1e-5000");
    const BigFloat original{value};

    ASSERT_NE(BigFloat{0}, value);
    EXPECT_EQ("0.0", (value.*GetParam())(5));
    EXPECT_EQ(original, value);
}

INSTANTIATE_TEST_SUITE_P(
    Formatters, TestBigFloatFormatterUnderflow, testing::Values(&BigFloat::to_string, &BigFloat::to_scientific_string));

TEST_F(TestBigFloatFormat, formatsAtMaximumSupportedPrecision)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(708);
    const BigFloat value = make_value("-1.25");
    const BigFloat original{value};

    EXPECT_EQ("-1.25", value.to_string(g_bf_decimals));
    EXPECT_EQ("-1.25e0", value.to_scientific_string(g_bf_decimals));
    EXPECT_EQ(original, value);
}

TEST_F(TestBigFloatFormat, repeatedFormattingPreservesTheValue)
{
    const BigFloat value = make_value("-1.234567890123456789");
    const BigFloat original{value};

    EXPECT_EQ("-1.23", value.to_string(3));
    EXPECT_EQ("-1.234567890123456789e0", value.to_scientific_string(0));
    EXPECT_EQ(original, value);
}

} // namespace id::test
