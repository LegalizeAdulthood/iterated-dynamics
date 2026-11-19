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

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <stdexcept>
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

static_assert(!std::is_same_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigFloat, BigNum>);
static_assert(!std::is_convertible_v<LDouble, BigNum>);
static_assert(std::is_same_v<decltype(&BigNum::hex_dump), void (BigNum::*)() const>);
static_assert(std::is_void_v<decltype(square_bn(std::declval<BigNum &>(), std::declval<BigNum &>()))>);
static_assert(std::is_same_v<decltype(std::declval<const BigNum &>() * std::declval<const BigNum &>()), BigNum>);
static_assert(std::is_same_v<decltype(std::declval<const BigNum &>().sqr()), BigNum>);
static_assert(std::is_same_v<decltype(add_bn(std::declval<const BigNum &>(), std::declval<const BigNum &>())), BigNum>);
static_assert(std::is_same_v<decltype(sub_bn(std::declval<const BigNum &>(), std::declval<const BigNum &>())), BigNum>);
static_assert(std::is_same_v<decltype(mult_bn_int(std::declval<const BigNum &>(), U16{})), BigNum>);
static_assert(std::is_same_v<decltype(abs_bn(std::declval<const BigNum &>())), BigNum>);
static_assert(std::is_same_v<decltype(add_bn(std::declval<BigNum &>(), std::declval<const BigNum &>(),
                                 std::declval<const BigNum &>())),
    BigNum &>);
static_assert(
    std::is_same_v<decltype(mult_bn_int(std::declval<BigNum &>(), std::declval<const BigNum &>(), U16{})), BigNum &>);
static_assert(std::is_same_v<decltype(abs_bn(std::declval<BigNum &>(), std::declval<const BigNum &>())), BigNum &>);
static_assert(!std::is_void_v<decltype(abs_a_bn(std::declval<BigNum &>()))>);
static_assert(std::is_same_v<decltype(std::declval<BigNum &>() = std::declval<const BigNum &>()), BigNum &>);
static_assert(std::is_same_v<decltype(std::declval<BigNum &>() += std::declval<const BigNum &>()), BigNum &>);
static_assert(std::is_same_v<decltype(std::declval<BigNum &>() -= std::declval<const BigNum &>()), BigNum &>);
static_assert(
    std::is_void_v<decltype(mult_bn(std::declval<BigNum &>(), std::declval<BigNum &>(), std::declval<BigNum &>()))>);

static_assert(std::is_same_v<decltype(clear_bn(std::declval<BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(max_bn(std::declval<BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(neg_a_bn(std::declval<BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(double_a_bn(std::declval<BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(half_a_bn(std::declval<BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(neg_bn(std::declval<BigNum &>(), std::declval<const BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(double_bn(std::declval<BigNum &>(), std::declval<const BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(half_bn(std::declval<BigNum &>(), std::declval<const BigNum &>())), BigNum &>);
static_assert(std::is_same_v<decltype(mult_a_bn_int(std::declval<BigNum &>(), U16{})), BigNum &>);
static_assert(std::is_same_v<decltype(is_bn_neg(std::declval<const BigNum &>())), bool>);
static_assert(std::is_same_v<decltype(is_bn_zero(std::declval<const BigNum &>())), bool>);
static_assert(std::is_same_v<decltype(is_bn_not_zero(std::declval<const BigNum &>())), bool>);
static_assert(std::is_same_v<decltype(bn_to_int(std::declval<const BigNum &>())), long>);
static_assert(std::is_same_v<decltype(div_bn(std::declval<BigNum &>(), std::declval<const BigNum &>(),
                                 std::declval<const BigNum &>())),
    BigNum &>);
static_assert(std::is_same_v<decltype(unsafe_inv_bn(std::declval<BigNum &>(), std::declval<BigNum &>())), BigNum &>);
static_assert(
    std::is_same_v<decltype(unsafe_div_bn_int(std::declval<BigNum &>(), std::declval<BigNum &>(), U16{})), BigNum &>);
static_assert(std::is_same_v<decltype(div_a_bn_int(std::declval<BigNum &>(), U16{})), BigNum &>);
static_assert(std::is_same_v<decltype(str_to_bn(std::declval<BigNum &>(), std::string_view{})), BigNum &>);
static_assert(std::is_same_v<decltype(int_to_bn(std::declval<BigNum &>(), long{})), BigNum &>);
static_assert(std::is_same_v<decltype(std::declval<BigNum &>() = LDouble{}), BigNum &>);
static_assert(std::is_same_v<decltype(bn_to_float(std::declval<BigNum &>())), LDouble>);
static_assert(std::is_same_v<decltype(bn_to_string(std::declval<const BigNum &>(), int{})), std::string>);

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

namespace
{

struct BigNumIntegerInput
{
    LDouble value;
    std::array<long, 3> expected;
};

} // namespace

class TestBigNumIntegerObservation : public TestBigNumValue,
                                     public testing::WithParamInterface<std::tuple<int, int, BigNumIntegerInput>>
{
protected:
    void SetUp() override;
};

void TestBigNumIntegerObservation::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumIntegerObservation, extractsIntegerBitsWithoutChangingTheValueOrAllocating)
{
    const BigNumIntegerInput &input = std::get<2>(GetParam());
    const BigStackMark mark = save_stack();
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigArena::Allocation reservation = temporary.allocate_single();
    const BigArena::Index index = reservation.index();
    reservation.reset();
    const BigNum original{input.value};
    const std::size_t working_high_water = bn_arena(BigLifetime::WORKING).high_water_mark();
    {
        const BigNum value{temporary, input.value};
        const std::size_t high_water = temporary.high_water_mark();

        const long result = bn_to_int(value);

        const int expected_index = g_int_length == 1 ? 0 : g_int_length == 2 ? 1 : 2;
        EXPECT_EQ(input.expected[expected_index], result);
        EXPECT_EQ(original, value);
        EXPECT_EQ(working_high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
        EXPECT_EQ(high_water, temporary.high_water_mark());
        EXPECT_EQ(mark, save_stack());
    }
    const BigArena::Allocation reused = temporary.allocate_single();
    EXPECT_EQ(index, reused.index());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigNumIntegerObservation,
    testing::Combine(testing::Values(4, 8, 12, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(BigNumIntegerInput{0, {0, 0, 0}}, BigNumIntegerInput{1, {1, 1, 1}},
            BigNumIntegerInput{1.5L, {1, 1, 1}},
            BigNumIntegerInput{-2, {254, 65534, static_cast<long>(std::uint32_t{0xfffffffe})}})));

namespace
{

enum class BigNumScalarCall
{
    PARSE,
    INTEGER,
    FLOAT
};

struct BigNumScalarInput
{
    const char *text;
    long magnitude;
    bool half;
    bool negative;
    const char *fractional_text;
    const char *integer_text;
};

void PrintTo(const BigNumScalarInput &input, std::ostream *out)
{
    *out << input.text;
}


} // namespace

class TestBigNumScalarInterfaces
    : public TestBigNumValue,
      public testing::WithParamInterface<std::tuple<int, int, BigNumScalarInput, BigNumScalarCall>>
{
protected:
    void SetUp() override;
};

void TestBigNumScalarInterfaces::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumScalarInterfaces, observesConstructedValuesWithoutChangingTheSourceOrAllocating)
{
    const BigNumScalarInput &input = std::get<2>(GetParam());
    const BigNumScalarCall call = std::get<3>(GetParam());
    const bool half = call != BigNumScalarCall::INTEGER && input.half;
    const bool has_fraction = half && g_bn_length > g_int_length;
    const LDouble native = (input.negative ? -1 : 1) * (input.magnitude + (half ? 0.5L : 0.0L));
    LDouble expected = (input.negative ? -1 : 1) * (input.magnitude + (has_fraction ? 0.5L : 0.0L));
    if (expected == 0)
    {
        expected = 0;
    }
    const BigNum value = call == BigNumScalarCall::PARSE ? BigNum{std::string_view{input.text}} : BigNum{native};
    const BigNum original{value};
    const std::size_t high_water = value.arena().high_water_mark();
    const BigStackMark mark = save_stack();

    const LDouble observed = value.to_float();

    EXPECT_EQ(expected, observed);
    EXPECT_EQ(std::signbit(expected), std::signbit(observed));
    EXPECT_EQ(has_fraction ? input.fractional_text : input.integer_text, value.to_string(8));
    EXPECT_EQ(original, value);
    EXPECT_EQ(high_water, value.arena().high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndCalls, TestBigNumScalarInterfaces,
    testing::Combine(testing::Values(4, 12, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(BigNumScalarInput{"0", 0, false, false, "0.", "0."},
            BigNumScalarInput{"42", 42, false, false, "42.", "42."},
            BigNumScalarInput{"12.5", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"-12.5", 12, true, true, "-12.5", "-12."},
            BigNumScalarInput{"-0.5", 0, true, true, "-0.5", "0."},
            BigNumScalarInput{"+12.5", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"12.500", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"12.5x", 0, false, false, "0.", "0."},
            BigNumScalarInput{"12e3", 12, false, false, "12.", "12."}),
        testing::Values(BigNumScalarCall::PARSE, BigNumScalarCall::INTEGER, BigNumScalarCall::FLOAT)));

namespace
{

void init_layout_precision(const int integer_length)
{
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    init_bf_length(32);
}

} // namespace

class TestBigNumLayout : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigNumLayout::SetUp()
{
    init_layout_precision(GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
}


TEST_P(TestBigNumLayout, formattingPreservesTheValue)
{
    const BigNum value{"-12.5"};
    const BigNum original{value};

    EXPECT_EQ("-12.5", value.to_string(8));
    EXPECT_EQ("-12.5", value.to_string(0));
    EXPECT_EQ(original, value);
}

TEST_F(TestBigNumValue, zeroConstructionLeavesTheLegacyStackUnchanged)
{
    const auto marker = save_stack();

    const BigNum value{0};

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

class TestBigNumConstructionPrecision : public TestBigNumValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigNumConstructionPrecision, constructsAndConvertsAtTheSelectedPrecision)
{
    const int length = GetParam();
    init_bf_length(length);

    const BigNum value{"-1.25"};

    EXPECT_EQ("-1.25", value.to_string(8));
    EXPECT_EQ(static_cast<std::size_t>(length), value.arena().value_size());
    EXPECT_EQ(value, BigNum{BigFloat{value}});
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumConstructionPrecision, testing::Values(4, 672));

TEST_F(TestBigNumValue, owningCopySurvivesTemporaryStackRewindAndReuse)
{
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigArena &working = bn_arena(BigLifetime::WORKING);
    constexpr std::string_view text{"-1.23456789012345678901234567890123456789"};
    const BigStackMark marker = save_stack();
    const BigNum expected{text};
    BigNum copy;
    {
        const BigNum source{temporary, text};
        copy = BigNum{working, source};
    }
    bn_stack().allocate_single();
    const std::size_t high_water = temporary.high_water_mark();
    restore_stack(marker);
    bn_stack().allocate_single();

    EXPECT_EQ(high_water, temporary.high_water_mark());
    EXPECT_EQ(&working, &copy.arena());
    EXPECT_EQ(expected, copy);
    restore_stack(marker);
}

TEST_F(TestBigNumValue, copyingAndMovingDoNotRequireAnActiveArena)
{
    BigNum original{"-1.23456789012345678901234567890123456789"};
    const BigNum expected{original};
    free_bf_vars();

    const BigNum copy{original};
    BigNum moved{std::move(original)};
    original = copy;
    moved = std::move(original);

    EXPECT_FALSE(original);
    init_bf_dec(40);
    EXPECT_EQ(expected, copy);
    EXPECT_EQ(expected, moved);
}

TEST_F(TestBigNumValue, copyAssignmentUsesTheCurrentArenaLayout)
{
    g_bailout_test = Bailout::REAL;
    init_bf_dec(80);
    const BigNum original{"-1.25"};
    BigArena &persistent = bn_arena(BigLifetime::PERSISTENT);
    BigNum assigned{persistent, 2.5L};
    const BigNum expected{original};

    assigned = original;
    assigned += BigNum{2};

    EXPECT_EQ(&persistent, &assigned.arena());
    EXPECT_EQ(static_cast<std::size_t>(g_bn_length), assigned.arena().value_size());
    EXPECT_EQ(BigNum{0.75L}, assigned);
    EXPECT_EQ(expected, original);
}

TEST_F(TestBigNumValue, scalarAssignmentReusesItsSlotInAFullArena)
{
    BigArena arena{static_cast<std::size_t>(g_bn_length), 2, [](Byte *) {}};
    BigNum value{arena, -1.25L};
    const BigNum neighbor{arena, -4};
    ASSERT_EQ(arena.capacity(), arena.high_water_mark());

    EXPECT_EQ(&value, &(value = 2.5L));

    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(BigNum{2.5L}, value);
    EXPECT_EQ(BigNum{-4}, neighbor);
}

TEST_F(TestBigNumValue, movedFromValueCanBeReassigned)
{
    BigNum original{"-1.25"};
    const BigNum moved{std::move(original)};

    original = 2.5L;

    EXPECT_EQ("-1.25", moved.to_string());
    EXPECT_EQ("2.5", original.to_string());
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
    const BigNum original{value};

    EXPECT_EQ("-1.25", value.to_string(8));
    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_THROW(value.to_string(-1), std::invalid_argument);
    EXPECT_EQ(original, value);
}

TEST_P(TestBigNumLayout, constructorsUseTheActiveFixedPointLayout)
{
    EXPECT_EQ(1.25L, BigNum{1.25L}.to_float());
    EXPECT_EQ(1.25L, BigNum{"1.25"}.to_float());
}

TEST_P(TestBigNumLayout, explicitConversionsPreserveBitsBeyondMachinePrecision)
{
    const BigNum fixed{"1.000000000000000000000000000000000000001"};
    ASSERT_GT(fixed, BigNum{1});
    ASSERT_EQ(1.0L, fixed.to_float());

    const BigFloat floating{fixed};

    EXPECT_GT(floating, BigFloat{1});
    EXPECT_EQ(fixed, BigNum{floating});
}

TEST_P(TestBigNumLayout, inPlaceBigFloatConversionPreservesSourceAndDestinationStorage)
{
    const BigFloat source{"-1.25"};
    const BigFloat expected_source{-1.25L};
    const BigNum expected{-1.25L};
    BigArena &arena = bn_arena(BigLifetime::PERSISTENT);
    const BigArena &working = bn_arena(BigLifetime::WORKING);
    BigNum destination{arena, 23};
    const std::size_t high_water = arena.high_water_mark();
    const std::size_t working_high_water = working.high_water_mark();

    static_assert(std::is_same_v<decltype(destination = source), BigNum &>);
    BigNum &assigned = (destination = source);

    EXPECT_EQ(&destination, &assigned);
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(working_high_water, working.high_water_mark());
    EXPECT_EQ(expected, destination);
    EXPECT_EQ(expected_source, source);
}

TEST_P(TestBigNumLayout, bigNumConversionUsesTheSelectedBigFloatArena)
{
    const BigNum source{"-1.25"};
    const BigNum expected_source{-1.25L};
    const BigFloat expected{-1.25L};
    const std::size_t size = bf_arena(BigLifetime::WORKING).value_size();
    BigArena arena{size, 2, [](Byte *) {}};
    const BigFloat occupied{arena, 23};

    static_assert(std::is_same_v<decltype(source.to_big_float(arena)), BigFloat>);
    const BigFloat destination = source.to_big_float(arena);

    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(expected, destination);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(23.0L, occupied.to_float());
}

TEST_F(TestBigNumValue, bigFloatConversionAssignsIntoEmptyOwners)
{
    const BigFloat source{"-1.25"};
    const BigNum expected{-1.25L};
    BigNum unbound;
    BigArena &arena = bn_arena(BigLifetime::PERSISTENT);
    BigNum bound{arena, 23};
    const BigNum moved{std::move(bound)};

    unbound = source;
    bound = source;

    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &unbound.arena());
    EXPECT_EQ(&arena, &bound.arena());
    EXPECT_EQ(expected, unbound);
    EXPECT_EQ(expected, bound);
    EXPECT_EQ(23.0L, moved.to_float());
}

class TestBigNumLayoutConversion : public TestBigNumValue,
                                   public testing::WithParamInterface<std::tuple<int, std::pair<const char *, int>>>
{
};

TEST_P(TestBigNumLayoutConversion, owningConversionsKeepLegacyOverflowAndUnderflowResults)
{
    const int integer_length = std::get<0>(GetParam());
    init_layout_precision(integer_length);
    ASSERT_EQ(integer_length, g_int_length);
    const std::pair<const char *, int> &input = std::get<1>(GetParam());
    const BigFloat floating{input.first};
    const BigFloat original{floating};
    const int saturation = input.second;

    const BigNum fixed{floating};
    BigNum assigned{23};
    assigned = floating;

    EXPECT_EQ(fixed, assigned);
    EXPECT_EQ(original, floating);
    if (saturation == 0)
    {
        EXPECT_EQ(BigNum{0}, fixed);
    }
    else if (saturation > 0)
    {
        EXPECT_GT(fixed, BigNum{64});
    }
    else
    {
        EXPECT_LT(fixed, BigNum{-64});
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLimits, TestBigNumLayoutConversion,
    testing::Combine(testing::Values(1, 2, 4),
        testing::Values(std::make_pair("1e400", 1), std::make_pair("-1e400", -1), std::make_pair("1e-400", 0),
            std::make_pair("-1e-400", 0))));

TEST_F(TestBigNumValue, bigFloatConversionRejectsAnUndersizedBigNumArena)
{
    const BigFloat floating{"-1.25"};
    BigArena arena{static_cast<std::size_t>(g_bn_length - 1), 2, [](Byte *) {}};

    EXPECT_THROW((BigNum{arena, floating}), std::invalid_argument);
    EXPECT_EQ(BigFloat{-1.25L}, floating);
}

TEST_F(TestBigNumValue, bigNumConversionRejectsAnUndersizedBigFloatArena)
{
    const BigNum source{"-1.25"};
    BigArena arena{static_cast<std::size_t>(g_bf_length + 1), 2, [](Byte *) {}};

    EXPECT_THROW(source.to_big_float(arena), std::invalid_argument);
    EXPECT_THROW((BigFloat{arena, source}), std::invalid_argument);
}

class TestBigNumLayoutScaling : public TestBigNumValue,
                                public testing::WithParamInterface<std::tuple<int, LDouble, U16>>
{
};

TEST_P(TestBigNumLayoutScaling, unsignedScalingPreservesTheSourceAndReturnsWorkingStorage)
{
    const auto [integer_length, input, factor] = GetParam();
    init_layout_precision(integer_length);
    BigArena &persistent = bn_arena(BigLifetime::PERSISTENT);
    const BigNum source{persistent, input};

    const BigNum result = source * factor;

    EXPECT_EQ(BigNum{input * factor}, result);
    EXPECT_EQ(BigNum{input}, source);
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &result.arena());
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndFactors, TestBigNumLayoutScaling,
    testing::Combine(testing::Values(1, 2, 4), testing::Values(-0.001953125L, 0.0L, 0.001953125L),
        testing::Values<U16>(0, 1, 2, 255, 256, 32768, 65535)));

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumLayout, testing::Values(1, 2, 4));

TEST_P(TestBigNumLayout, divisionWritesTheDestinationAndPreservesDistinctOperands)
{
    const BigNum numerator{"-7.5"};
    const BigNum denominator{"0.5"};
    const BigNum original_numerator{numerator};
    const BigNum original_denominator{denominator};
    BigNum quotient{bn_arena(BigLifetime::WORKING)};

    EXPECT_EQ(&quotient, &div_bn(quotient, numerator, denominator));

    EXPECT_EQ(BigNum{-15}, quotient);
    EXPECT_EQ(original_numerator, numerator);
    EXPECT_EQ(original_denominator, denominator);
}

TEST_P(TestBigNumLayout, legacyDivisionKeepsSaturationAndZeroRules)
{
    BigNum maximum{0};
    max_bn(maximum);
    const BigNum negative_maximum = -maximum;
    const BigNum quarter{"0.25"};
    const BigNum zero{0};
    BigNum result{0};

    div_bn(result, maximum, quarter);
    EXPECT_EQ(maximum, result);
    div_bn(result, negative_maximum, quarter);
    EXPECT_EQ(negative_maximum, result);
    div_bn(result, negative_maximum, zero);
    EXPECT_EQ(maximum, result);
    div_bn(result, zero, zero);
    EXPECT_EQ(zero, result);
}

class TestBigNumProducts : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigNumProducts::SetUp()
{
    init_bf_length(GetParam());
}

TEST_P(TestBigNumLayout, arithmeticPreservesOperandsAndUsesCanonicalResults)
{
    const BigNum left{"-7.5"};
    const BigNum right{"2.5"};
    const BigNum original_left{left};
    const BigNum original_right{right};

    EXPECT_EQ("-5.", (left + right).to_string(8));
    EXPECT_EQ("-10.", (left - right).to_string(8));
    EXPECT_EQ("-18.75", (left * right).to_string(8));
    EXPECT_EQ("-15.", (left / BigNum{"0.5"}).to_string(8));
    EXPECT_EQ("7.5", (-left).to_string(8));
    EXPECT_EQ("56.25", left.sqr().to_string(8));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigNumLayout, compoundAssignmentChangesOnlyItsDestination)
{
    BigNum value{"1.5"};
    const BigNum right{2};
    const BigNum copy = value;
    const BigNum original_right{right};

    EXPECT_EQ(&value, &(value += right));
    EXPECT_EQ("3.5", value.to_string(8));
    EXPECT_EQ(&value, &(value -= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ(&value, &(value *= right));
    EXPECT_EQ("3.", value.to_string(8));
    EXPECT_EQ(&value, &(value /= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ("1.5", copy.to_string(8));
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigNumLayout, repeatedNegativeOperandsAndSelfAssignmentAreSafe)
{
    BigNum value{-2};
    const BigNum original{value};

    EXPECT_EQ("4.", (value * value).to_string(8));
    EXPECT_EQ("4.", value.sqr().to_string(8));
    EXPECT_EQ(original, value);
    value *= value;
    EXPECT_EQ("4.", value.to_string(8));
    value /= value;
    EXPECT_EQ("1.", value.to_string(8));
    value += value;
    EXPECT_EQ("2.", value.to_string(8));
    value -= value;
    EXPECT_EQ("0.", value.to_string(8));
}

TEST_P(TestBigNumLayout, assignmentCanAlsoSupplyTheRightOperand)
{
    const BigNum left{8};
    BigNum value{2};

    value = left + value;
    EXPECT_EQ("10.", value.to_string(8));
    value = left - value;
    EXPECT_EQ("-2.", value.to_string(8));
    value = left * value;
    EXPECT_EQ("-16.", value.to_string(8));
    value = left / value;
    EXPECT_EQ("-0.5", value.to_string(8));
}

TEST_P(TestBigNumLayout, expressionsMatchDestinationProceduresAndOutliveTheirTemporaries)
{
    const BigNum left{"-1.234567890123456789"};
    const BigNum right{"2.345678901234567890"};
    const BigNum divisor{"-0.5"};
    const BigNum original_left{left};
    const BigNum original_right{right};
    const BigNum original_divisor{divisor};
    BigArena &working = bn_arena(BigLifetime::WORKING);
    BigNum numerator{working};
    BigNum quotient{working};
    BigNum product{working};
    BigNum expected{working};
    BigNum left_copy{left};
    // Use fresh destinations so division starts with zero low bytes.
    add_bn(numerator, left, right);
    div_bn(quotient, numerator, divisor);
    mult_bn(product, quotient, left_copy);
    sub_bn(expected, product, right);
    const BigStackMark marker = save_stack();

    const BigNum result = ((left + right) / divisor) * left - right;
    const BigNum unrelated = BigNum{-8} * BigNum{-9};

    EXPECT_EQ(marker, save_stack());
    bn_stack().allocate_single();
    restore_stack(marker);
    EXPECT_EQ(expected, result);
    EXPECT_EQ("72.", unrelated.to_string(8));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    EXPECT_EQ(original_divisor, divisor);
}

TEST_P(TestBigNumLayout, divisionAssignmentMatchesAFreshResult)
{
    BigNum value = BigNum{"-1.234567890123456789"} + BigNum{"2.345678901234567890"};
    const BigNum divisor{"-0.5"};
    const BigNum expected = value / divisor;

    value /= divisor;

    EXPECT_EQ(expected, value);
}

TEST_P(TestBigNumLayout, divisionKeepsLegacySaturationAndZeroResults)
{
    BigNum maximum{0};
    max_bn(maximum);
    const BigNum negative_maximum = -maximum;
    const BigNum quarter{"0.25"};
    const BigNum zero{0};

    EXPECT_EQ(maximum, maximum / quarter);
    EXPECT_EQ(maximum, negative_maximum / zero);
    EXPECT_EQ(negative_maximum, negative_maximum / quarter);
    EXPECT_EQ(zero, zero / zero);
    EXPECT_EQ(zero, zero / quarter);
}

TEST_P(TestBigNumProducts, productsAndSquaresReturnCanonicalValues)
{
    const BigNum value{"-7.5"};
    const BigNum expected{"56.25"};

    EXPECT_EQ(expected, value * value);
    EXPECT_EQ(expected, value.sqr());
    EXPECT_EQ(BigNum{"-7.5"}, value);
}

class TestBigNumComparisons : public TestBigNumValue, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigNumComparisons, ordersConstructedValuesWithoutChangingOperandsOrAllocating)
{
    const auto [left_index, right_index] = GetParam();
    constexpr std::array<const char *, 8> ordered{"-7.5", "-1", "-0.000000000000000000000000000000000001", "0",
        "0.000000000000000000000000000000000001", "1", "1.000000000000000000000000000000000001", "7.5"};
    const BigNum left{std::string_view{ordered[left_index]}};
    const BigNum right{std::string_view{ordered[right_index]}};
    const BigNum original_left{left};
    const BigNum original_right{right};
    const BigArena &working = bn_arena(BigLifetime::WORKING);
    const std::size_t high_water = working.high_water_mark();

    EXPECT_EQ(left_index == right_index, left == right);
    EXPECT_EQ(left_index != right_index, left != right);
    EXPECT_EQ(left_index < right_index, left < right);
    EXPECT_EQ(left_index <= right_index, left <= right);
    EXPECT_EQ(left_index > right_index, left > right);
    EXPECT_EQ(left_index >= right_index, left >= right);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    EXPECT_EQ(high_water, working.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigNumComparisons, testing::Combine(testing::Range(0, 8), testing::Range(0, 8)));

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumProducts, testing::Values(4, 8, 12, 672));

class TestBigNumRetainedSqr : public TestBigNumValue, public testing::WithParamInterface<std::tuple<int, int, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumRetainedSqr::SetUp()
{
    const int length = std::get<0>(GetParam());
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(length);
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumRetainedSqr, canonicalSqrPreservesDestinationSourceAndScratchLifetime)
{
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum source{std::get<2>(GetParam()) ? -2.0L : 2.0L};
    BigNum destination{arena};
    const BigNum expected{4};
    const BigNum original_source{source};
    BigArena::Allocation scratch = temporary.allocate_double();
    const BigArena::Index scratch_index = scratch.index();
    scratch.reset();
    const std::size_t working_high_water = arena.high_water_mark();
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const BigStackMark stack_mark = save_stack();

    square_bn(destination, source);

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(working_high_water, arena.high_water_mark());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(stack_mark, save_stack());
    EXPECT_EQ(original_source, source);
    BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(scratch_index, reused.index());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
}

TEST_P(TestBigNumRetainedSqr, canonicalSqrPreservesFixedWidthOverflow)
{
    // Squaring the largest signed integer wraps to 1 at the canonical width.
    const LDouble magnitude = std::ldexp(LDouble{1}, 8 * g_int_length - 1) - 1;
    BigNum source{std::get<2>(GetParam()) ? -magnitude : magnitude};
    const BigNum original_source{source};
    BigNum destination{bn_arena(BigLifetime::WORKING)};
    const BigNum expected{1};

    square_bn(destination, source);

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(original_source, source);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigNumRetainedSqr,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4), testing::Bool()));


namespace
{

struct RetainedMultiplyInput
{
    const char *name;
    int left;
    int right;
    bool shared;
};

void PrintTo(const RetainedMultiplyInput &input, std::ostream *out)
{
    *out << input.name;
}

std::array<int, 9> kernel_lengths()
{
    return {g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding, g_shift_factor, g_bf_length, g_r_bf_length,
        g_bf_decimals};
}

} // namespace

class TestBigNumRetainedMultiply : public TestBigNumValue,
                                   public testing::WithParamInterface<std::tuple<int, int, RetainedMultiplyInput>>
{
protected:
    void SetUp() override;
};

void TestBigNumRetainedMultiply::SetUp()
{
    const int length = std::get<0>(GetParam());
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(length);
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumRetainedMultiply, owningMultiplyPreservesDestinationArenaOperandsAndScratchReuse)
{
    const RetainedMultiplyInput input = std::get<2>(GetParam());
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum left{static_cast<LDouble>(input.left)};
    BigNum distinct_right{static_cast<LDouble>(input.right)};
    BigNum &right = input.shared ? left : distinct_right;
    BigNum destination{arena};
    const int product = input.left * (input.shared ? input.left : input.right);
    const BigNum expected{static_cast<LDouble>(product)};
    const int restored_left = input.shared && input.left < 0 ? -input.left : input.left;
    const BigNum expected_left{static_cast<LDouble>(restored_left)};
    const BigNum original_right{distinct_right};
    BigArena::Allocation scratch = temporary.allocate_double();
    const BigArena::Index scratch_index = scratch.index();
    scratch.reset();
    const std::size_t working_high_water = arena.high_water_mark();
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const BigStackMark stack_mark = save_stack();
    const std::array<int, 9> lengths = kernel_lengths();

    mult_bn(destination, left, right);

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(working_high_water, arena.high_water_mark());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(stack_mark, save_stack());
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(expected_left, left);
    EXPECT_EQ(original_right, distinct_right);
    const BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(scratch_index, reused.index());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(LayoutsSignsAndAliasing, TestBigNumRetainedMultiply,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(RetainedMultiplyInput{"PositivePositive", 2, 3, false},
            RetainedMultiplyInput{"PositiveNegative", 2, -3, false},
            RetainedMultiplyInput{"NegativePositive", -2, 3, false},
            RetainedMultiplyInput{"NegativeNegative", -2, -3, false},
            RetainedMultiplyInput{"SharedPositive", 2, 3, true},
            RetainedMultiplyInput{"SharedNegative", -2, 3, true})));

namespace
{

enum class CanonicalValue
{
    ADD,
    SUB,
    MULT,
    ABS_NEGATIVE,
    ABS_POSITIVE
};

enum class CanonicalProcedure
{
    COPY,
    COPY_SELF,
    ADD,
    ADD_SELF,
    SUB,
    SUB_SELF,
    ABS_NEGATIVE,
    ABS_POSITIVE,
    ADD_TO,
    ADD_TO_SELF,
    SUB_TO,
    SUB_TO_SELF,
    MULT_TO,
    MULT_TO_SELF,
    ABS_TO,
    ABS_TO_SELF,
    ABS_TO_POSITIVE
};

} // namespace

class TestBigNumCanonicalValues : public TestBigNumValue,
                                  public testing::WithParamInterface<std::tuple<int, int, CanonicalValue>>
{
protected:
    void SetUp() override;
};

void TestBigNumCanonicalValues::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumCanonicalValues, returnsIndependentCanonicalStorageAndPreservesInputs)
{
    const CanonicalValue operation = std::get<2>(GetParam());
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    BigNum left{operation == CanonicalValue::ABS_POSITIVE ? 7.0L : -7.0L};
    BigNum right{3};
    const BigNum original_left{left};
    const BigNum original_right{right};
    long expected_integer{};
    BigNum result;

    switch (operation)
    {
    case CanonicalValue::ADD:
        expected_integer = -4;
        result = left + right;
        break;
    case CanonicalValue::SUB:
        expected_integer = -10;
        result = left - right;
        break;
    case CanonicalValue::MULT:
        expected_integer = g_int_length == 4 ? -7L * 65535 : 7;
        result = left * U16{65535};
        break;
    case CanonicalValue::ABS_NEGATIVE:
    case CanonicalValue::ABS_POSITIVE:
        expected_integer = 7;
        result = abs_bn(left);
        break;
    }

    const BigNum expected{static_cast<LDouble>(expected_integer)};
    ASSERT_TRUE(result);
    EXPECT_EQ(&arena, &result.arena());
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    left = 0;
    right = 0;
    EXPECT_EQ(expected, result);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndOperations, TestBigNumCanonicalValues,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(CanonicalValue::ADD, CanonicalValue::SUB, CanonicalValue::MULT, CanonicalValue::ABS_NEGATIVE,
            CanonicalValue::ABS_POSITIVE)));

class TestBigNumCanonicalProcedures : public TestBigNumValue,
                                      public testing::WithParamInterface<std::tuple<int, int, CanonicalProcedure>>
{
protected:
    void SetUp() override;
};

void TestBigNumCanonicalProcedures::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumCanonicalProcedures, preservesDestinationIdentityAliasesAndUnchangedSources)
{
    const CanonicalProcedure operation = std::get<2>(GetParam());
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    const long source_integer = operation == CanonicalProcedure::ABS_TO_POSITIVE ? 7 : -7;
    const long separate_integer = operation == CanonicalProcedure::ABS_NEGATIVE ? -2 : 2;
    BigNum source{static_cast<LDouble>(source_integer)};
    BigNum separate{static_cast<LDouble>(separate_integer)};
    const bool self = operation == CanonicalProcedure::COPY_SELF || operation == CanonicalProcedure::ADD_SELF ||
        operation == CanonicalProcedure::SUB_SELF || operation == CanonicalProcedure::ADD_TO_SELF ||
        operation == CanonicalProcedure::SUB_TO_SELF || operation == CanonicalProcedure::MULT_TO_SELF ||
        operation == CanonicalProcedure::ABS_TO_SELF;
    BigNum &destination = self ? source : separate;
    const BigNum original_source{source};
    const long initial = self ? source_integer : separate_integer;
    long expected_integer{};
    const std::size_t high_water = arena.high_water_mark();

    switch (operation)
    {
    case CanonicalProcedure::COPY:
    case CanonicalProcedure::COPY_SELF:
        expected_integer = source_integer;
        EXPECT_EQ(&destination, &(destination = source));
        break;
    case CanonicalProcedure::ADD:
    case CanonicalProcedure::ADD_SELF:
        expected_integer = initial + source_integer;
        EXPECT_EQ(&destination, &(destination += source));
        break;
    case CanonicalProcedure::SUB:
    case CanonicalProcedure::SUB_SELF:
        expected_integer = initial - source_integer;
        EXPECT_EQ(&destination, &(destination -= source));
        break;
    case CanonicalProcedure::ABS_NEGATIVE:
    case CanonicalProcedure::ABS_POSITIVE:
        expected_integer = std::abs(initial);
        abs_a_bn(destination);
        break;
    case CanonicalProcedure::ADD_TO:
    case CanonicalProcedure::ADD_TO_SELF:
        expected_integer = initial + source_integer;
        EXPECT_EQ(&destination, &add_bn(destination, destination, source));
        break;
    case CanonicalProcedure::SUB_TO:
    case CanonicalProcedure::SUB_TO_SELF:
        expected_integer = initial - source_integer;
        EXPECT_EQ(&destination, &sub_bn(destination, destination, source));
        break;
    case CanonicalProcedure::MULT_TO:
    case CanonicalProcedure::MULT_TO_SELF:
        expected_integer = source_integer * 3;
        EXPECT_EQ(&destination, &mult_bn_int(destination, source, U16{3}));
        break;
    case CanonicalProcedure::ABS_TO:
    case CanonicalProcedure::ABS_TO_SELF:
    case CanonicalProcedure::ABS_TO_POSITIVE:
        expected_integer = std::abs(source_integer);
        EXPECT_EQ(&destination, &abs_bn(destination, source));
        break;
    }

    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    const BigNum expected{static_cast<LDouble>(expected_integer)};
    EXPECT_EQ(expected, destination);
    if (!self)
    {
        EXPECT_EQ(original_source, source);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndAliases, TestBigNumCanonicalProcedures,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(CanonicalProcedure::COPY, CanonicalProcedure::COPY_SELF, CanonicalProcedure::ADD,
            CanonicalProcedure::ADD_SELF, CanonicalProcedure::SUB, CanonicalProcedure::SUB_SELF,
            CanonicalProcedure::ABS_NEGATIVE, CanonicalProcedure::ABS_POSITIVE, CanonicalProcedure::ADD_TO,
            CanonicalProcedure::ADD_TO_SELF, CanonicalProcedure::SUB_TO, CanonicalProcedure::SUB_TO_SELF,
            CanonicalProcedure::MULT_TO, CanonicalProcedure::MULT_TO_SELF, CanonicalProcedure::ABS_TO,
            CanonicalProcedure::ABS_TO_SELF, CanonicalProcedure::ABS_TO_POSITIVE)));

namespace
{

enum class UnaryOperation
{
    CLEAR,
    NEGATE,
    NEGATE_ASSIGN,
    DOUBLE,
    DOUBLE_ASSIGN,
    HALF,
    HALF_ASSIGN,
    MULT_ASSIGN
};

struct UnaryInput
{
    UnaryOperation operation;
    LDouble source;
    LDouble expected;
    U16 factor{};
};

} // namespace

class TestBigNumUnary : public TestBigNumValue,
                        public testing::WithParamInterface<std::tuple<int, int, UnaryInput, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumUnary::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumUnary, preservesDestinationIdentityAliasesAndAllocation)
{
    const UnaryInput &input = std::get<2>(GetParam());
    const bool self = std::get<3>(GetParam());
    BigNum source{input.source};
    BigNum separate{input.source};
    BigNum &destination = self ? source : separate;
    const BigArena &arena = destination.arena();
    const BigNum original_source{source};
    const BigNum expected{input.expected};
    const std::size_t high_water = arena.high_water_mark();

    switch (input.operation)
    {
    case UnaryOperation::CLEAR:
        EXPECT_EQ(&destination, &clear_bn(destination));
        break;
    case UnaryOperation::NEGATE:
        EXPECT_EQ(&destination, &neg_bn(destination, std::as_const(source)));
        break;
    case UnaryOperation::NEGATE_ASSIGN:
        EXPECT_EQ(&destination, &neg_a_bn(destination));
        break;
    case UnaryOperation::DOUBLE:
        EXPECT_EQ(&destination, &double_bn(destination, std::as_const(source)));
        break;
    case UnaryOperation::DOUBLE_ASSIGN:
        EXPECT_EQ(&destination, &double_a_bn(destination));
        break;
    case UnaryOperation::HALF:
        EXPECT_EQ(&destination, &half_bn(destination, std::as_const(source)));
        break;
    case UnaryOperation::HALF_ASSIGN:
        EXPECT_EQ(&destination, &half_a_bn(destination));
        break;
    case UnaryOperation::MULT_ASSIGN:
        EXPECT_EQ(&destination, &mult_a_bn_int(destination, input.factor));
        break;
    }

    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(expected, destination);
    if (!self)
    {
        EXPECT_EQ(original_source, source);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndAliases, TestBigNumUnary,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(UnaryInput{UnaryOperation::CLEAR, -3, 0}, UnaryInput{UnaryOperation::NEGATE, 3, -3},
            UnaryInput{UnaryOperation::NEGATE_ASSIGN, -3, 3}, UnaryInput{UnaryOperation::DOUBLE, 3, 6},
            UnaryInput{UnaryOperation::DOUBLE_ASSIGN, -3, -6}, UnaryInput{UnaryOperation::HALF, 6, 3},
            UnaryInput{UnaryOperation::HALF_ASSIGN, -6, -3}, UnaryInput{UnaryOperation::MULT_ASSIGN, -3, 0, 0},
            UnaryInput{UnaryOperation::MULT_ASSIGN, -3, -6, 2}),
        testing::Bool()));

enum class IntegerDivisionCall
{
    DISTINCT,
    ALIASED,
    IN_PLACE
};

class TestBigNumIntegerDivision
    : public TestBigNumValue,
      public testing::WithParamInterface<std::tuple<int, int, U16, bool, IntegerDivisionCall>>
{
protected:
    void SetUp() override;
};

void TestBigNumIntegerDivision::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumIntegerDivision, preservesResultIdentityMutationAliasesAndStorage)
{
    const U16 divisor = std::get<2>(GetParam());
    const bool negative = std::get<3>(GetParam());
    const IntegerDivisionCall call = std::get<4>(GetParam());
    BigNum source{negative ? -6.0L : 6.0L};
    BigNum destination{-17};
    BigNum &result = call == IntegerDivisionCall::DISTINCT ? destination : source;
    BigNum expected{0};
    if (divisor == 0)
    {
        max_bn(expected);
        if (negative)
        {
            expected = -expected;
        }
    }
    else
    {
        expected = (negative ? -6.0L : 6.0L) / divisor;
    }
    const BigNum expected_source = call == IntegerDivisionCall::DISTINCT ? BigNum{6} : BigNum{expected};
    const BigNum untouched{destination};
    const BigArena &arena = result.arena();
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const std::size_t high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    if (call == IntegerDivisionCall::IN_PLACE)
    {
        EXPECT_EQ(&result, &(result /= divisor));
    }
    else
    {
        EXPECT_EQ(&result, &unsafe_div_bn_int(result, source, divisor));
    }

    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(&arena, &result.arena());
    if (call != IntegerDivisionCall::DISTINCT)
    {
        EXPECT_EQ(untouched, destination);
    }
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(LayoutsDivisorsSignsAndAliases, TestBigNumIntegerDivision,
    testing::Combine(testing::Values(12, 32, 672), testing::Values(1, 2, 4), testing::Values<U16>(0, 1, 2, 256),
        testing::Bool(),
        testing::Values(IntegerDivisionCall::DISTINCT, IntegerDivisionCall::ALIASED, IntegerDivisionCall::IN_PLACE)));

class TestBigNumKernelWindows : public TestBigNumValue, public testing::WithParamInterface<std::tuple<int, int, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumKernelWindows::SetUp()
{
    const auto [length, integer_length, flag] = GetParam();
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(length);
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumKernelWindows, reciprocalRestoresPrecisionAndReturnsTheOriginalDestination)
{
    BigNum source{std::get<2>(GetParam()) ? -2.0L : 2.0L};
    const BigNum expected{std::get<2>(GetParam()) ? -0.5L : 0.5L};
    const BigNum expected_source{2};
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum result{temporary};
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();
    const auto high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(&temporary, &result.arena());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
}

TEST_P(TestBigNumKernelWindows, divisionCapturesBothInputsBeforeAnAliasedWrite)
{
    BigNum left{-6};
    BigNum right{2};
    const bool alias_left = std::get<2>(GetParam());
    BigNum &output = alias_left ? left : right;
    const BigNum expected{-3};
    const BigNum other{alias_left ? right : left};
    const BigArena &arena = output.arena();
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();

    EXPECT_EQ(&output, &div_bn(output, left, right));
    EXPECT_EQ(&arena, &output.arena());

    EXPECT_EQ(expected, output);
    EXPECT_EQ(other, alias_left ? right : left);
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, reciprocalZeroKeepsSaturationAndPrecision)
{
    BigNum source{0};
    BigNum result{bn_arena(BigLifetime::WORKING)};
    BigNum expected{0};
    max_bn(expected);
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const BigArena &arena = result.arena();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(BigNum{0}, source);
    EXPECT_EQ(&arena, &result.arena());
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, divisionCopiesASharedInputBeforeWritingItsDestination)
{
    BigNum value{std::get<2>(GetParam()) ? -2.0L : 2.0L};
    const BigNum expected{1};
    const BigArena &arena = value.arena();
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const std::size_t high_water = value.arena().high_water_mark();

    EXPECT_EQ(&value, &div_bn(value, value, value));

    EXPECT_EQ(expected, value);
    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, value.arena().high_water_mark());
}

TEST_P(TestBigNumKernelWindows, reciprocalOverflowKeepsEarlyReturnSignAndSourceMutation)
{
    const LDouble magnitude = std::ldexp(LDouble{1}, -40);
    BigNum source{std::get<2>(GetParam()) ? -magnitude : magnitude};
    BigNum result{bn_arena(BigLifetime::WORKING)};
    BigNum expected{0};
    max_bn(expected);
    const BigNum expected_source{magnitude};
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, bigFloatMantissaProductsRestoreTheBigNumContext)
{
    const bool negative = std::get<2>(GetParam());
    BigFloat left{negative ? -0.75L : 0.75L};
    BigFloat right{0.5L};
    const BigFloat original_left{left};
    const BigFloat original_right{right};
    const BigFloat expected_product{negative ? -0.375L : 0.375L};
    const BigFloat expected_sqr{0.5625L};
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();

    BigFloat product{bf_arena(BigLifetime::WORKING)};
    EXPECT_EQ(&product, &mult_bf(product, left, right));

    EXPECT_EQ(expected_product, product);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    EXPECT_EQ(lengths, kernel_lengths());

    EXPECT_EQ(&product, &square_bf(product, left));

    EXPECT_EQ(expected_sqr, product);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndLayouts, TestBigNumKernelWindows,
    testing::Combine(testing::Values(12, 32, 672), testing::Values(1, 2, 4), testing::Bool()));

class TestBigNumKernelStorage : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
    static void expect_fresh_scratch();
};

void TestBigNumKernelStorage::SetUp()
{
    init_bf_length(GetParam());
}

void TestBigNumKernelStorage::expect_fresh_scratch()
{
    static_assert(std::is_same_v<decltype(g_bn_tmp2), BigNum>);
    static_assert(std::is_same_v<decltype(g_bn_tmp3), BigNum>);
    static_assert(std::is_same_v<decltype(g_bn_tmp_copy1), BigArena::Allocation>);
    static_assert(std::is_same_v<decltype(g_bn_tmp_copy2), BigArena::Allocation>);
    ASSERT_TRUE(g_bn_tmp2);
    ASSERT_TRUE(g_bn_tmp3);
    ASSERT_TRUE(g_bn_tmp_copy1);
    ASSERT_TRUE(g_bn_tmp_copy2);
    const BigArena &working = bn_arena(BigLifetime::WORKING);
    EXPECT_EQ(&working, &g_bn_tmp2.arena());
    EXPECT_EQ(&working, &g_bn_tmp3.arena());
    EXPECT_EQ(&working, &g_bn_tmp_copy1.arena());
    EXPECT_EQ(&working, &g_bn_tmp_copy2.arena());
    EXPECT_EQ(2U, g_bn_tmp_copy1.blocks());
    EXPECT_EQ(2U, g_bn_tmp_copy2.blocks());
    EXPECT_EQ(0.0L, g_bn_tmp2.to_float());
    EXPECT_EQ(0.0L, g_bn_tmp3.to_float());

    const BigNum numerator{6};
    const BigNum denominator{2};
    BigNum quotient{0};
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&quotient, &div_bn(quotient, numerator, denominator));
    EXPECT_EQ(3.0L, quotient.to_float());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

TEST_P(TestBigNumKernelStorage, rebindingClearsScratchAndKeepsTheReservations)
{
    expect_fresh_scratch();
    g_bn_tmp2 = 17;
    g_bn_tmp3 = -23;
    const BigArena::Index copy1 = g_bn_tmp_copy1.index();
    const BigArena::Index copy2 = g_bn_tmp_copy2.index();
    const std::size_t high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    init_bf_length(GetParam());

    EXPECT_EQ(copy1, g_bn_tmp_copy1.index());
    EXPECT_EQ(copy2, g_bn_tmp_copy2.index());
    EXPECT_EQ(high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
    expect_fresh_scratch();
}

TEST_P(TestBigNumKernelStorage, precisionChangesRebuildUsableScratchAndEmptyTheStack)
{
    expect_fresh_scratch();
    g_bn_tmp2 = 17;
    g_bn_tmp3 = -23;
    bn_stack().allocate_double();
    const int other_length = GetParam() == 672 ? 4 : 672;

    init_bf_length(other_length);

    EXPECT_EQ(static_cast<std::size_t>(other_length), bn_arena(BigLifetime::WORKING).value_size());
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_EQ(0U, bn_arena(BigLifetime::TEMPORARY).high_water_mark());
    expect_fresh_scratch();
    g_bn_tmp2 = -17;
    g_bn_tmp3 = 23;

    init_bf_length(GetParam());

    EXPECT_EQ(static_cast<std::size_t>(GetParam()), bn_arena(BigLifetime::WORKING).value_size());
    expect_fresh_scratch();
}

TEST_P(TestBigNumKernelStorage, repeatedCleanupReleasesAllScratchSlotsForReuse)
{
    expect_fresh_scratch();
    BigArena &working = bn_arena(BigLifetime::WORKING);
    const std::size_t high_water = working.high_water_mark();

    free_bf_vars();
    EXPECT_FALSE(g_bn_tmp2);
    EXPECT_FALSE(g_bn_tmp3);
    EXPECT_FALSE(g_bn_tmp_copy1);
    EXPECT_FALSE(g_bn_tmp_copy2);

    free_bf_vars();
    EXPECT_FALSE(g_bn_tmp2);
    EXPECT_FALSE(g_bn_tmp3);
    EXPECT_FALSE(g_bn_tmp_copy1);
    EXPECT_FALSE(g_bn_tmp_copy2);
    {
        std::vector<BigArena::Allocation> pairs;
        pairs.reserve(high_water / 2);
        for (std::size_t i = 0; i < high_water; i += 2)
        {
            pairs.emplace_back(working.allocate_double());
        }
        EXPECT_EQ(high_water, working.high_water_mark());
    }
    init_bf_length(GetParam());

    EXPECT_EQ(high_water, working.high_water_mark());
    expect_fresh_scratch();
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumKernelStorage, testing::Values(4, 8, 12, 56, 60, 64, 672));

class TestBigNumProductStorage : public TestBigNumValue, public testing::WithParamInterface<std::tuple<int, bool>>
{
protected:
    void SetUp() override
    {
        init_bf_length(std::get<0>(GetParam()));
    }
};

TEST_P(TestBigNumProductStorage, transfersTheFirstSlotAndReleasesTheCompanion)
{
    const bool squared = std::get<1>(GetParam());
    const BigNum left{"-7.5"};
    const BigNum right{"0.5"};
    const BigNum original_left{left};
    const BigNum original_right{right};
    const BigNum expected{squared ? 56.25L : -3.75L};
    BigArena &working = bn_arena(BigLifetime::WORKING);
    BigArena::Allocation reservation = working.allocate_double();
    const BigArena::Index product_index = reservation.index();
    reservation.reset();
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();

    {
        const BigNum result = squared ? left.sqr() : left * right;

        EXPECT_EQ(&working, &result.arena());
        EXPECT_EQ(expected, result);
        EXPECT_EQ(original_left, left);
        EXPECT_EQ(original_right, right);
        EXPECT_EQ(high_water, working.high_water_mark());
        EXPECT_EQ(mark, save_stack());
        BigArena::Allocation companion = working.allocate_single();
        ASSERT_EQ(product_index + 1, companion.index());
        EXPECT_EQ(1U, companion.blocks());
        companion.reset();
        const BigNum neighbor{working, 17};
        EXPECT_EQ(17.0L, neighbor.to_float());
        EXPECT_EQ(expected, result);
        EXPECT_EQ(high_water, working.high_water_mark());
    }

    const BigArena::Allocation reused = working.allocate_double();
    EXPECT_EQ(product_index, reused.index());
    EXPECT_EQ(high_water, working.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndOperations, TestBigNumProductStorage,
    testing::Combine(testing::Values(4, 12, 56, 60, 64, 672), testing::Bool()));

class TestBigNum : public testing::Test
{
protected:
    TestBigNum() :
        m_saved_bf_digits(g_bf_digits, 0)
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

    static std::string round_trip(const std::string_view input, const int decimals)
    {
        return BigNum{bn_arena(BigLifetime::TEMPORARY), input}.to_string(decimals);
    }

    ValueSaver<int> m_saved_bf_digits;
};

struct BigNumFormatInput
{
    const char *name;
    const char *input;
    int decimals;
    const char *expected;
};

class TestBigNumFormatting : public TestBigNum, public testing::WithParamInterface<BigNumFormatInput>
{
};

TEST_P(TestBigNumFormatting, formatsParsedValue)
{
    EXPECT_EQ(GetParam().expected, round_trip(GetParam().input, GetParam().decimals));
}

TEST_P(TestBigNumFormatting, releasesTemporaryStorageAfterFormatting)
{
    auto &temporary = bn_arena(BigLifetime::TEMPORARY);
    auto probe = temporary.allocate_single();
    const auto index = probe.index();
    probe.reset();
    const auto marker = save_stack();

    EXPECT_EQ(GetParam().expected, round_trip(GetParam().input, GetParam().decimals));

    auto reused = temporary.allocate_single();
    EXPECT_EQ(index, reused.index());
    EXPECT_EQ(marker, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigNumFormatting,
    testing::Values(BigNumFormatInput{"Integer", "42", 5, "42."},
        BigNumFormatInput{"PositiveFraction", "12.5", 5, "12.5"},
        BigNumFormatInput{"TrailingFractionalZeros", "12.500", 5, "12.5"}),
    [](const testing::TestParamInfo<BigNumFormatInput> &info) { return info.param.name; });

TEST_F(TestBigNum, formatsZeroValue)
{
    const auto marker = save_stack();
    {
        const BigNum value{bn_arena(BigLifetime::TEMPORARY), 0};

        EXPECT_EQ("0.", value.to_string(5));
    }
    EXPECT_EQ(marker, save_stack());
}

} // namespace id::test
