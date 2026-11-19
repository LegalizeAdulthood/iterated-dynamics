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
static_assert(!std::is_convertible_v<BigNum, BigNumPtr>);
static_assert(!std::is_convertible_v<BigNumPtr, BigNum>);
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
static_assert(std::is_void_v<decltype(copy_bn(std::declval<BigNum &>(), std::declval<const BigNum &>()))>);
static_assert(std::is_void_v<decltype(add_a_bn(std::declval<BigNum &>(), std::declval<const BigNum &>()))>);
static_assert(std::is_void_v<decltype(sub_a_bn(std::declval<BigNum &>(), std::declval<const BigNum &>()))>);
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
static_assert(std::is_same_v<decltype(float_to_bn(std::declval<BigNum &>(), LDouble{})), BigNum &>);
static_assert(std::is_same_v<decltype(bn_to_float(std::declval<BigNum &>())), LDouble>);
static_assert(std::is_same_v<decltype(bn_to_string(std::declval<const BigNum &>(), int{})), std::string>);

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

std::vector<Byte> encoded_integer(const long value)
{
    std::vector<Byte> result(g_bn_length);
    const std::uint32_t bits = static_cast<std::uint32_t>(value);
    for (int byte = 0; byte < g_int_length; ++byte)
    {
        result[g_bn_length - g_int_length + byte] = static_cast<Byte>(bits >> (8 * byte));
    }
    return result;
}

std::vector<Byte> saturation_bytes(const bool negative)
{
    std::vector<Byte> result(g_bn_length, negative ? Byte{0} : Byte{0xff});
    result.front() = negative ? Byte{1} : Byte{0xff};
    result.back() = negative ? Byte{0x80} : Byte{0x7f};
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

TEST_F(TestBigNumValue, importedBytesSurviveArenaRewindAndReuse)
{
    // Borrow BigStack buffers here to test importing bytes across a real rewind.
    const auto marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bn_length);
    BigNumPtr source = bn_stack().allocate_single();
    BigNum{"-1.23456789012345678901234567890123456789"}.copy_to_legacy(source, size);
    const std::vector<Byte> expected(source, source + size);
    const BigNumConstPtr input = source;

    const BigNum value = BigNum::from_legacy(input, size);
    restore_stack(marker);
    BigNumPtr reused = bn_stack().allocate_single();
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

TEST_F(TestBigNumValue, copyAssignmentUsesTheCurrentArenaLayout)
{
    const int old_length = g_bn_length;
    g_bailout_test = Bailout::REAL;
    init_bf_dec(80);
    const BigNum original{"-1.25"};
    BigNum assigned{2.5L};

    assigned = original;

    std::vector<Byte> too_small(old_length);
    EXPECT_THROW(assigned.copy_to_legacy(too_small.data(), too_small.size()), std::invalid_argument);
    EXPECT_EQ(BigNum{-1.25L}, assigned);
}

TEST_F(TestBigNumValue, legacyBorrowsAccessOwnedStorage)
{
    static_assert(std::is_same_v<decltype(std::declval<BigNum &>().data()), BigNumPtr>);
    static_assert(std::is_same_v<decltype(std::declval<const BigNum &>().data()), BigNumConstPtr>);
    BigNum value{-1.25L};
    const auto *before = std::as_const(value).data();
    EXPECT_EQ(before, value.data());
    EXPECT_EQ(encoded_value(value), std::vector<Byte>(before, before + g_bn_length));

    float_to_bn(value, 2.5L);

    EXPECT_EQ(before, std::as_const(value).data());
    EXPECT_EQ(BigNum{2.5L}, value);
    EXPECT_EQ(encoded_value(value), std::vector<Byte>(value.data(), value.data() + g_bn_length));
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

TEST_F(TestBigNumValue, conversionAdaptersValidateDestinationBuffers)
{
    const BigFloat floating{"-1.25"};
    std::vector<Byte> bn_buffer(static_cast<std::size_t>(g_bn_length), Byte{0x5a});
    const auto original_bn = bn_buffer;

    EXPECT_THROW(floating.copy_to_legacy_num(nullptr, bn_buffer.size()), std::invalid_argument);
    EXPECT_THROW(floating.copy_to_legacy_num(bn_buffer.data(), bn_buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(original_bn, bn_buffer);
}

TEST_F(TestBigNumValue, bigNumConversionRejectsAnUndersizedBigFloatArena)
{
    const BigNum source{"-1.25"};
    BigArena arena{static_cast<std::size_t>(g_bf_length + 1), 2, [](Byte *) {}};

    EXPECT_THROW(source.to_big_float(arena), std::invalid_argument);
    EXPECT_THROW((BigFloat{arena, source}), std::invalid_argument);
}

namespace
{

enum class LayoutScalarInput
{
    NEGATIVE,
    ZERO,
    POSITIVE,
    MINIMUM,
    QUANTUM,
    NEGATIVE_QUANTUM
};

std::vector<Byte> scalar_input_bytes(const LayoutScalarInput input)
{
    switch (input)
    {
    case LayoutScalarInput::NEGATIVE:
        return encoded_value(BigNum{"-1.2345678901234567890123456789"});
    case LayoutScalarInput::ZERO:
        return std::vector<Byte>(g_bn_length);
    case LayoutScalarInput::POSITIVE:
        return encoded_value(BigNum{"1.2345678901234567890123456789"});
    case LayoutScalarInput::MINIMUM:
    {
        std::vector<Byte> result(g_bn_length);
        result.back() = 0x80;
        return result;
    }
    case LayoutScalarInput::QUANTUM:
    {
        std::vector<Byte> result(g_bn_length);
        result.front() = 1;
        return result;
    }
    case LayoutScalarInput::NEGATIVE_QUANTUM:
        return std::vector<Byte>(g_bn_length, 0xff);
    }
    throw std::logic_error("Unexpected scalar input");
}

void negate_encoded_bytes(std::vector<Byte> &value)
{
    unsigned carry = 1;
    for (Byte &byte : value)
    {
        const unsigned digit = (byte ^ 0xff) + carry;
        byte = static_cast<Byte>(digit);
        carry = digit >> 8;
    }
}

std::vector<Byte> scalar_quotient_bytes(std::vector<Byte> value, const U16 divisor)
{
    const bool negative = (value.back() & 0x80) != 0;
    if (divisor == 0)
    {
        return saturation_bytes(negative);
    }
    if (negative)
    {
        negate_encoded_bytes(value);
    }
    unsigned remainder = 0;
    for (std::size_t i = value.size(); i > 0; --i)
    {
        Byte &byte = value[i - 1];
        const unsigned digit = (remainder << 8) | byte;
        byte = static_cast<Byte>(digit / divisor);
        remainder = digit % divisor;
    }
    if (negative)
    {
        negate_encoded_bytes(value);
    }
    return value;
}

} // namespace

class TestBigNumLayoutScalar : public TestBigNumValue,
                               public testing::WithParamInterface<std::tuple<int, LayoutScalarInput, U16>>
{
protected:
    void SetUp() override
    {
        const int integer_length = std::get<0>(GetParam());
        init_layout_precision(integer_length);
        ASSERT_EQ(integer_length, g_int_length);
    }
};

class TestBigNumLayoutScaling : public TestBigNumLayoutScalar
{
};

TEST_P(TestBigNumLayoutScaling, unsignedScalingPreservesAllOperandBitsAndFixedWidthOverflow)
{
    const U16 factor = std::get<2>(GetParam());
    BigArena &persistent = bn_arena(BigLifetime::PERSISTENT);
    const std::vector<Byte> original = scalar_input_bytes(std::get<1>(GetParam()));
    const BigNum source = BigNum::from_legacy(persistent, original.data(), original.size());
    std::vector<Byte> expected = original;
    unsigned carry = 0;
    for (Byte &byte : expected)
    {
        const unsigned digit = static_cast<unsigned>(byte) * factor + carry;
        byte = static_cast<Byte>(digit);
        carry = digit >> 8;
    }

    const BigNum result = source * factor;

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &result.arena());
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndFactors, TestBigNumLayoutScaling,
    testing::Combine(testing::Values(1, 2, 4),
        testing::Values(LayoutScalarInput::NEGATIVE, LayoutScalarInput::ZERO, LayoutScalarInput::POSITIVE),
        testing::Values<U16>(0, 1, 2, 255, 256, 32768, 65535)));

class TestBigNumLayoutScalarDivision : public TestBigNumLayoutScalar
{
};

TEST_P(TestBigNumLayoutScalarDivision, unsignedDivisionAssignmentPreservesRoundingAndZeroDivisorSaturation)
{
    const U16 divisor = std::get<2>(GetParam());
    BigArena &persistent = bn_arena(BigLifetime::PERSISTENT);
    const std::vector<Byte> input = scalar_input_bytes(std::get<1>(GetParam()));
    BigNum value = BigNum::from_legacy(persistent, input.data(), input.size());

    EXPECT_EQ(&value, &(value /= divisor));

    EXPECT_EQ(scalar_quotient_bytes(input, divisor), encoded_value(value));
    EXPECT_EQ(&persistent, &value.arena());
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndDivisors, TestBigNumLayoutScalarDivision,
    testing::Combine(testing::Values(1, 2, 4),
        testing::Values(LayoutScalarInput::NEGATIVE, LayoutScalarInput::ZERO, LayoutScalarInput::POSITIVE,
            LayoutScalarInput::MINIMUM, LayoutScalarInput::QUANTUM, LayoutScalarInput::NEGATIVE_QUANTUM),
        testing::Values<U16>(0, 1, 2, 255, 256, 32768, 65535)));

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumLayout, testing::Values(1, 2, 4));

TEST_P(TestBigNumLayout, divisionWritesTheDestinationAndPreservesDistinctOperands)
{
    const BigNum numerator{"-7.5"};
    const BigNum denominator{"0.5"};
    const std::vector<Byte> original_numerator = encoded_value(numerator);
    const std::vector<Byte> original_denominator = encoded_value(denominator);
    BigNum quotient{bn_arena(BigLifetime::WORKING)};

    EXPECT_EQ(&quotient, &div_bn(quotient, numerator, denominator));

    EXPECT_EQ(encoded_integer(-15), encoded_value(quotient));
    EXPECT_EQ(original_numerator, encoded_value(numerator));
    EXPECT_EQ(original_denominator, encoded_value(denominator));
}

TEST_P(TestBigNumLayout, legacyDivisionKeepsSaturationAndZeroRules)
{
    const std::vector<Byte> maximum = saturation_bytes(false);
    const std::vector<Byte> negative_maximum = saturation_bytes(true);
    BigNum numerator = BigNum::from_legacy(maximum.data(), maximum.size());
    BigNum denominator{"0.25"};
    BigNum result{bn_arena(BigLifetime::WORKING)};

    div_bn(result, numerator, denominator);
    EXPECT_EQ(maximum, encoded_value(result));
    neg_a_bn(numerator);
    div_bn(result, numerator, denominator);
    EXPECT_EQ(negative_maximum, encoded_value(result));
    clear_bn(denominator);
    div_bn(result, numerator, denominator);
    EXPECT_EQ(maximum, encoded_value(result));
    clear_bn(numerator);
    div_bn(result, numerator, denominator);
    EXPECT_EQ(encoded_integer(0), encoded_value(result));
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
    const auto original_left = encoded_value(left);
    const auto original_right = encoded_value(right);

    EXPECT_EQ("-5.", (left + right).to_string(8));
    EXPECT_EQ("-10.", (left - right).to_string(8));
    EXPECT_EQ("-18.75", (left * right).to_string(8));
    EXPECT_EQ("-15.", (left / BigNum{"0.5"}).to_string(8));
    EXPECT_EQ("7.5", (-left).to_string(8));
    EXPECT_EQ("56.25", left.sqr().to_string(8));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigNumLayout, compoundAssignmentChangesOnlyItsDestination)
{
    BigNum value{"1.5"};
    const BigNum right{2};
    const BigNum copy = value;
    const auto original_right = encoded_value(right);

    EXPECT_EQ(&value, &(value += right));
    EXPECT_EQ("3.5", value.to_string(8));
    EXPECT_EQ(&value, &(value -= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ(&value, &(value *= right));
    EXPECT_EQ("3.", value.to_string(8));
    EXPECT_EQ(&value, &(value /= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ("1.5", copy.to_string(8));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigNumLayout, repeatedNegativeOperandsAndSelfAssignmentAreSafe)
{
    BigNum value{-2};
    const auto original = encoded_value(value);

    EXPECT_EQ("4.", (value * value).to_string(8));
    EXPECT_EQ("4.", value.sqr().to_string(8));
    EXPECT_EQ(original, encoded_value(value));
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
    const std::vector<Byte> original_left = encoded_value(left);
    const std::vector<Byte> original_right = encoded_value(right);
    const std::vector<Byte> original_divisor = encoded_value(divisor);
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
    EXPECT_EQ(encoded_value(expected), encoded_value(result));
    EXPECT_EQ("72.", unrelated.to_string(8));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
    EXPECT_EQ(original_divisor, encoded_value(divisor));
}

TEST_P(TestBigNumLayout, divisionAssignmentMatchesAFreshResult)
{
    BigNum value = BigNum{"-1.234567890123456789"} + BigNum{"2.345678901234567890"};
    const BigNum divisor{"-0.5"};
    const auto expected = encoded_value(value / divisor);

    value /= divisor;

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_P(TestBigNumLayout, divisionKeepsLegacySaturationAndZeroResults)
{
    const std::vector<Byte> maximum = saturation_bytes(false);
    const BigNum positive = BigNum::from_legacy(maximum.data(), maximum.size());
    const BigNum quarter{"0.25"};
    const BigNum zero{0};

    EXPECT_EQ(maximum, encoded_value(positive / quarter));
    EXPECT_EQ(maximum, encoded_value((-positive) / zero));
    EXPECT_EQ(saturation_bytes(true), encoded_value((-positive) / quarter));
    EXPECT_EQ(encoded_integer(0), encoded_value(zero / zero));
    EXPECT_EQ(encoded_integer(0), encoded_value(zero / quarter));
}

TEST_P(TestBigNumProducts, productsAndSquaresReturnOnlyCanonicalBytes)
{
    const BigNum value{"-7.5"};
    const std::vector<Byte> original = encoded_value(value);
    std::vector<Byte> expected = encoded_integer(56);
    expected[g_bn_length - g_int_length - 1] = 0x40;

    EXPECT_EQ(expected, encoded_value(value * value));
    EXPECT_EQ(expected, encoded_value(value.sqr()));
    EXPECT_EQ(original, encoded_value(value));
}

TEST_P(TestBigNumProducts, normalizationPreservesSignedLowBitTruncation)
{
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length));
    quantum.front() = 1;
    const BigNum positive = BigNum::from_legacy(quantum.data(), quantum.size());
    const BigNum half{"0.5"};
    const auto zero = encoded_value(BigNum{0});

    EXPECT_EQ(zero, encoded_value(positive * half));
    EXPECT_EQ(zero, encoded_value(positive.sqr()));
    std::fill(quantum.begin(), quantum.end(), Byte{0xff});
    const BigNum negative = BigNum::from_legacy(quantum.data(), quantum.size());
    EXPECT_EQ(quantum, encoded_value(negative * half));
    EXPECT_EQ(quantum, encoded_value(negative));
}

TEST_F(TestBigNumValue, arithmeticKeepsFixedWidthOverflow)
{
    const std::vector<Byte> maximum = saturation_bytes(false);
    std::vector<Byte> minimum(maximum.size());
    minimum.back() = 0x80;
    std::vector<Byte> quantum(maximum.size());
    quantum.front() = 1;
    const BigNum high = BigNum::from_legacy(maximum.data(), maximum.size());
    const BigNum low = BigNum::from_legacy(minimum.data(), minimum.size());
    const BigNum unit = BigNum::from_legacy(quantum.data(), quantum.size());
    std::vector<Byte> negative_twice_quantum(maximum.size(), Byte{0xff});
    negative_twice_quantum.front() = 0xfe;

    EXPECT_EQ(minimum, encoded_value(high + unit));
    EXPECT_EQ(maximum, encoded_value(low - unit));
    EXPECT_EQ(minimum, encoded_value(-low));
    EXPECT_EQ(negative_twice_quantum, encoded_value(high * BigNum{2}));
}

TEST_F(TestBigNumValue, comparisonsReadNumericalValuesWithoutChangingOperands)
{
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length));
    quantum.front() = 1;
    const BigNum tiny = BigNum::from_legacy(quantum.data(), quantum.size());
    auto above_one = encoded_value(BigNum{1});
    above_one.front() = 1;
    const BigNum values[]{BigNum{-7.5L}, BigNum{-1}, -tiny, BigNum{0}, tiny, BigNum{1},
        BigNum::from_legacy(above_one.data(), above_one.size()), BigNum{7.5L}};

    for (std::size_t i = 0; i < std::size(values); ++i)
    {
        for (std::size_t j = 0; j < std::size(values); ++j)
        {
            const auto left = encoded_value(values[i]);
            const auto right = encoded_value(values[j]);
            const BigNum copy = values[j];

            EXPECT_EQ(i == j, values[i] == copy);
            EXPECT_EQ(i != j, values[i] != copy);
            EXPECT_EQ(i < j, values[i] < copy);
            EXPECT_EQ(i <= j, values[i] <= copy);
            EXPECT_EQ(i > j, values[i] > copy);
            EXPECT_EQ(i >= j, values[i] >= copy);
            EXPECT_EQ(left, encoded_value(values[i]));
            EXPECT_EQ(right, encoded_value(copy));
        }
    }
}


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

TEST_P(TestBigNumRetainedSqr, canonicalSquarePreservesDestinationSourceAndScratchLifetime)
{
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum source{std::get<2>(GetParam()) ? -2.0L : 2.0L};
    BigNum destination{arena};
    const std::vector<Byte> expected = encoded_value(BigNum{4});
    std::fill(source.data() + g_bn_length, source.data() + arena.block_size(), Byte{0x5a});
    const std::vector<Byte> original_source(source.data(), source.data() + arena.block_size());
    std::fill_n(destination.data(), arena.block_size(), Byte{0xa5});
    const Byte *const original_destination = destination.data();
    BigArena::Allocation scratch = temporary.allocate_double();
    const Byte *const scratch_address = scratch.data();
    scratch.reset();
    const std::size_t working_high_water = arena.high_water_mark();
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const BigStackMark stack_mark = save_stack();

    square_bn(destination, source);

    EXPECT_EQ(expected, encoded_value(destination));
    EXPECT_EQ(original_destination, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(working_high_water, arena.high_water_mark());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(stack_mark, save_stack());
    EXPECT_EQ(original_source, std::vector<Byte>(source.data(), source.data() + arena.block_size()));
    EXPECT_EQ(std::vector<Byte>(arena.block_size() - g_bn_length, 0xa5),
        std::vector<Byte>(destination.data() + g_bn_length, destination.data() + arena.block_size()));
    BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(scratch_address, reused.data());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
}

TEST_P(TestBigNumRetainedSqr, canonicalSquarePreservesFixedWidthOverflow)
{
    // Squaring the largest signed integer wraps to 1 at the canonical width.
    const LDouble magnitude = std::ldexp(LDouble{1}, 8 * g_int_length - 1) - 1;
    BigNum source{std::get<2>(GetParam()) ? -magnitude : magnitude};
    const std::vector<Byte> original_source = encoded_value(source);
    BigNum destination{bn_arena(BigLifetime::WORKING)};
    const std::vector<Byte> expected = encoded_value(BigNum{1});

    square_bn(destination, source);

    EXPECT_EQ(expected, encoded_value(destination));
    EXPECT_EQ(original_source, encoded_value(source));
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigNumRetainedSqr,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4), testing::Bool()));

class TestBigNumCanonicalSqrFraction : public TestBigNumRetainedSqr
{
};

TEST_P(TestBigNumCanonicalSqrFraction, discardsLowProductBitsBeforeReturningTheCanonicalResult)
{
    const bool negative = std::get<2>(GetParam());
    const std::size_t fractional_bytes = static_cast<std::size_t>(g_bn_length - g_int_length);
    std::vector<Byte> source_bytes(static_cast<std::size_t>(g_bn_length), negative ? Byte{0xff} : Byte{0});
    source_bytes.front() = negative ? Byte{0xff} : Byte{1};
    source_bytes[fractional_bytes] = negative ? Byte{0xfe} : Byte{1};
    BigNum source = BigNum::from_legacy(source_bytes.data(), source_bytes.size());
    BigNum destination{bn_arena(BigLifetime::WORKING)};
    // For |source| = 1 + q, q is one canonical LSB; q*q is discarded.
    std::vector<Byte> expected(source_bytes.size());
    expected.front() = 2;
    expected[fractional_bytes] = 1;

    square_bn(destination, source);

    EXPECT_EQ(expected, encoded_value(destination));
    EXPECT_EQ(source_bytes, encoded_value(source));
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigNumCanonicalSqrFraction,
    testing::Combine(testing::Values(8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4), testing::Bool()));

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

TEST_P(TestBigNumRetainedMultiply, owningMultiplyWritesCanonicalBytesAndPreservesStorageAndOperands)
{
    const RetainedMultiplyInput input = std::get<2>(GetParam());
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum left{static_cast<LDouble>(input.left)};
    BigNum distinct_right{static_cast<LDouble>(input.right)};
    BigNum &right = input.shared ? left : distinct_right;
    BigNum destination{arena};
    const int product = input.left * (input.shared ? input.left : input.right);
    const std::vector<Byte> expected = encoded_value(BigNum{static_cast<LDouble>(product)});
    const int restored_left = input.shared && input.left < 0 ? -input.left : input.left;
    const std::vector<Byte> expected_left_bytes = encoded_value(BigNum{static_cast<LDouble>(restored_left)});
    std::fill(left.data() + g_bn_length, left.data() + arena.block_size(), Byte{0x5a});
    std::fill(distinct_right.data() + g_bn_length, distinct_right.data() + arena.block_size(), Byte{0x6b});
    std::vector<Byte> expected_left(left.data(), left.data() + arena.block_size());
    std::copy(expected_left_bytes.begin(), expected_left_bytes.end(), expected_left.begin());
    const std::vector<Byte> original_right(distinct_right.data(), distinct_right.data() + arena.block_size());
    std::fill_n(destination.data(), arena.block_size(), Byte{0xa5});
    const Byte *const original_destination = destination.data();
    BigArena::Allocation scratch = temporary.allocate_double();
    const Byte *const scratch_address = scratch.data();
    scratch.reset();
    const std::size_t working_high_water = arena.high_water_mark();
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const BigStackMark stack_mark = save_stack();
    const std::array<int, 9> lengths = kernel_lengths();

    mult_bn(destination, left, right);

    EXPECT_EQ(expected, encoded_value(destination));
    EXPECT_EQ(original_destination, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(working_high_water, arena.high_water_mark());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(stack_mark, save_stack());
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(expected_left, std::vector<Byte>(left.data(), left.data() + arena.block_size()));
    EXPECT_EQ(original_right, std::vector<Byte>(distinct_right.data(), distinct_right.data() + arena.block_size()));
    EXPECT_EQ(std::vector<Byte>(arena.block_size() - g_bn_length, 0xa5),
        std::vector<Byte>(destination.data() + g_bn_length, destination.data() + arena.block_size()));
    BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(scratch_address, reused.data());
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

class TestBigNumCanonicalMultiplyFraction : public TestBigNumValue,
                                            public testing::WithParamInterface<std::tuple<int, int, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumCanonicalMultiplyFraction::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumCanonicalMultiplyFraction, discardsFractionalProductBytesAtTheExistingWindow)
{
    const bool negative = std::get<2>(GetParam());
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length), negative ? Byte{0xff} : Byte{0});
    if (!negative)
    {
        quantum.front() = 1;
    }
    BigNum left = BigNum::from_legacy(quantum.data(), quantum.size());
    BigNum right{"0.5"};
    const std::vector<Byte> original_right = encoded_value(right);
    BigNum destination{bn_arena(BigLifetime::WORKING)};
    const std::vector<Byte> expected(quantum.size(), negative ? Byte{0xff} : Byte{0});

    mult_bn(destination, left, right);

    EXPECT_EQ(expected, encoded_value(destination));
    EXPECT_EQ(quantum, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigNumCanonicalMultiplyFraction,
    testing::Combine(testing::Values(8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4), testing::Bool()));

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
    std::fill(left.data() + g_bn_length, left.data() + arena.block_size(), Byte{0x5a});
    std::fill(right.data() + g_bn_length, right.data() + arena.block_size(), Byte{0x6b});
    const std::vector<Byte> original_left(left.data(), left.data() + arena.block_size());
    const std::vector<Byte> original_right(right.data(), right.data() + arena.block_size());
    long expected_integer{};
    BigNum result;

    switch (operation)
    {
    case CanonicalValue::ADD:
        expected_integer = -4;
        result = add_bn(left, right);
        break;
    case CanonicalValue::SUB:
        expected_integer = -10;
        result = sub_bn(left, right);
        break;
    case CanonicalValue::MULT:
        expected_integer = -7L * 65535;
        result = mult_bn_int(left, U16{65535});
        break;
    case CanonicalValue::ABS_NEGATIVE:
    case CanonicalValue::ABS_POSITIVE:
        expected_integer = 7;
        result = abs_bn(left);
        break;
    }

    const std::vector<Byte> expected = encoded_integer(expected_integer);
    ASSERT_TRUE(result);
    EXPECT_EQ(&arena, &result.arena());
    EXPECT_NE(left.data(), result.data());
    EXPECT_NE(right.data(), result.data());
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(original_left, std::vector<Byte>(left.data(), left.data() + arena.block_size()));
    EXPECT_EQ(original_right, std::vector<Byte>(right.data(), right.data() + arena.block_size()));
    left = 0;
    right = 0;
    EXPECT_EQ(expected, encoded_value(result));
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

TEST_P(TestBigNumCanonicalProcedures, preservesDestinationIdentityTailsAliasesAndUnchangedSources)
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
    std::fill_n(destination.data() + g_bn_length, arena.block_size() - g_bn_length, Byte{0xa5});
    const std::vector<Byte> original_source(source.data(), source.data() + arena.block_size());
    const long initial = self ? source_integer : separate_integer;
    long expected_integer{};
    const Byte *const original_destination = destination.data();
    const std::size_t high_water = arena.high_water_mark();

    switch (operation)
    {
    case CanonicalProcedure::COPY:
    case CanonicalProcedure::COPY_SELF:
        expected_integer = source_integer;
        copy_bn(destination, source);
        break;
    case CanonicalProcedure::ADD:
    case CanonicalProcedure::ADD_SELF:
        expected_integer = initial + source_integer;
        add_a_bn(destination, source);
        break;
    case CanonicalProcedure::SUB:
    case CanonicalProcedure::SUB_SELF:
        expected_integer = initial - source_integer;
        sub_a_bn(destination, source);
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

    std::vector<Byte> expected = encoded_integer(expected_integer);
    expected.resize(arena.block_size(), 0xa5);
    EXPECT_EQ(original_destination, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(expected, std::vector<Byte>(destination.data(), destination.data() + arena.block_size()));
    if (!self)
    {
        EXPECT_EQ(original_source, std::vector<Byte>(source.data(), source.data() + arena.block_size()));
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
    MAX,
    NEGATE,
    NEGATE_ASSIGN,
    DOUBLE,
    DOUBLE_ASSIGN,
    HALF,
    HALF_ASSIGN,
    MULT_ASSIGN
};

struct EncodedPattern
{
    U16 low;
    Byte fill;
    Byte high;
};

std::vector<Byte> encoded_pattern(const EncodedPattern &pattern)
{
    std::vector<Byte> bytes(static_cast<std::size_t>(g_bn_length), pattern.fill);
    bytes[0] = static_cast<Byte>(pattern.low);
    bytes[1] = static_cast<Byte>(pattern.low >> 8);
    bytes.back() = pattern.high;
    return bytes;
}

struct UnaryInput
{
    const char *name;
    UnaryOperation operation;
    EncodedPattern source;
    EncodedPattern expected;
    U16 factor{};
};

void PrintTo(const UnaryInput &input, std::ostream *out)
{
    *out << input.name;
}

struct ObservationInput
{
    const char *name;
    EncodedPattern source;
    int mismatch;
    bool high_byte;
};

void PrintTo(const ObservationInput &input, std::ostream *out)
{
    *out << input.name;
}

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

TEST_P(TestBigNumUnary, preservesDestinationIdentityTailsAliasesAndAllocation)
{
    const UnaryInput &input = std::get<2>(GetParam());
    const bool self = std::get<3>(GetParam());
    const std::vector<Byte> source_bytes = encoded_pattern(input.source);
    BigNum source = BigNum::from_legacy(source_bytes.data(), source_bytes.size());
    BigNum separate = BigNum::from_legacy(source_bytes.data(), source_bytes.size());
    BigNum &destination = self ? source : separate;
    BigArena &arena = destination.arena();
    std::fill_n(destination.data() + g_bn_length, arena.block_size() - g_bn_length, Byte{0xa5});
    const std::vector<Byte> original_source(source.data(), source.data() + arena.block_size());
    const Byte *const original_destination = destination.data();
    const std::size_t high_water = arena.high_water_mark();
    const std::vector<Byte> expected_bytes = encoded_pattern(input.expected);
    std::vector<Byte> expected(destination.data(), destination.data() + arena.block_size());
    std::copy(expected_bytes.begin(), expected_bytes.end(), expected.begin());

    switch (input.operation)
    {
    case UnaryOperation::CLEAR:
        EXPECT_EQ(&destination, &clear_bn(destination));
        break;
    case UnaryOperation::MAX:
        EXPECT_EQ(&destination, &max_bn(destination));
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

    EXPECT_EQ(original_destination, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(expected, std::vector<Byte>(destination.data(), destination.data() + arena.block_size()));
    if (!self)
    {
        EXPECT_EQ(original_source, std::vector<Byte>(source.data(), source.data() + arena.block_size()));
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndAliases, TestBigNumUnary,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(UnaryInput{"Clear", UnaryOperation::CLEAR, {0xfffd, 0xff, 0xff}, {0, 0, 0}},
            UnaryInput{"Maximum", UnaryOperation::MAX, {3, 0, 0}, {0xffff, 0xff, 0x7f}},
            UnaryInput{"NegatePositive", UnaryOperation::NEGATE, {3, 0, 0}, {0xfffd, 0xff, 0xff}},
            UnaryInput{"NegateNegative", UnaryOperation::NEGATE, {0xfffd, 0xff, 0xff}, {3, 0, 0}},
            UnaryInput{"NegateMinimum", UnaryOperation::NEGATE, {0, 0, 0x80}, {0, 0, 0x80}},
            UnaryInput{"NegateInPlace", UnaryOperation::NEGATE_ASSIGN, {0xfffd, 0xff, 0xff}, {3, 0, 0}},
            UnaryInput{"DoublePositive", UnaryOperation::DOUBLE, {3, 0, 0}, {6, 0, 0}},
            UnaryInput{"DoubleNegative", UnaryOperation::DOUBLE, {0xfffd, 0xff, 0xff}, {0xfffa, 0xff, 0xff}},
            UnaryInput{"DoubleOverflow", UnaryOperation::DOUBLE, {0xffff, 0xff, 0x7f}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"DoubleInPlace", UnaryOperation::DOUBLE_ASSIGN, {0xffff, 0xff, 0x7f}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"HalfPositiveOdd", UnaryOperation::HALF, {3, 0, 0}, {1, 0, 0}},
            UnaryInput{"HalfNegativeOdd", UnaryOperation::HALF, {0xfffd, 0xff, 0xff}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"HalfMinimum", UnaryOperation::HALF, {0, 0, 0x80}, {0, 0, 0xc0}},
            UnaryInput{"HalfInPlace", UnaryOperation::HALF_ASSIGN, {0xfffd, 0xff, 0xff}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"MultiplyZero", UnaryOperation::MULT_ASSIGN, {0xfffd, 0xff, 0xff}, {0, 0, 0}, 0},
            UnaryInput{"MultiplyOne", UnaryOperation::MULT_ASSIGN, {0xffff, 0xff, 0x7f}, {0xffff, 0xff, 0x7f}, 1},
            UnaryInput{"MultiplyUnsigned", UnaryOperation::MULT_ASSIGN, {0xffff, 0xff, 0xff}, {1, 0xff, 0xff}, 65535},
            UnaryInput{"MultiplyOverflow", UnaryOperation::MULT_ASSIGN, {0, 0, 0x80}, {0, 0, 0}, 2}),
        testing::Bool()));

class TestBigNumObservations : public TestBigNumValue,
                               public testing::WithParamInterface<std::tuple<int, int, ObservationInput>>
{
protected:
    void SetUp() override;
};

void TestBigNumObservations::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 100 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumObservations, readsConstOwnersWithoutMutationOrAllocation)
{
    const ObservationInput &input = std::get<2>(GetParam());
    const std::vector<Byte> bytes = encoded_pattern(input.source);
    const BigNum value = BigNum::from_legacy(bytes.data(), bytes.size());
    const BigNum zero{0};
    BigArena &arena = value.arena();
    const std::vector<Byte> original(value.data(), value.data() + arena.block_size());
    const std::vector<Byte> original_zero(zero.data(), zero.data() + arena.block_size());
    const std::size_t high_water = arena.high_water_mark();
    const int comparison = input.high_byte ? input.mismatch * g_bn_length : input.mismatch;

    EXPECT_EQ(input.mismatch < 0, is_bn_neg(value));
    EXPECT_EQ(input.mismatch == 0, is_bn_zero(value));
    EXPECT_EQ(input.mismatch != 0, is_bn_not_zero(value));
    EXPECT_EQ(comparison, cmp_bn(value, zero));
    EXPECT_EQ(-comparison, cmp_bn(zero, value));
    EXPECT_EQ(0, cmp_bn(value, value));

    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(original, std::vector<Byte>(value.data(), value.data() + arena.block_size()));
    EXPECT_EQ(original_zero, std::vector<Byte>(zero.data(), zero.data() + arena.block_size()));
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndValues, TestBigNumObservations,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(ObservationInput{"Zero", {0, 0, 0}, 0, false}, ObservationInput{"Quantum", {1, 0, 0}, 1, false},
            ObservationInput{"SecondByte", {256, 0, 0}, 2, false},
            ObservationInput{"NegativeQuantum", {0xffff, 0xff, 0xff}, -1, true},
            ObservationInput{"Maximum", {0xffff, 0xff, 0x7f}, 1, true},
            ObservationInput{"Minimum", {0, 0, 0x80}, -1, true}, ObservationInput{"HighByte", {0, 0, 1}, 1, true})));

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

TEST_P(TestBigNumIntegerDivision, preservesResultIdentityMutationAliasesTailsAndStorage)
{
    const U16 divisor = std::get<2>(GetParam());
    const bool negative = std::get<3>(GetParam());
    const IntegerDivisionCall call = std::get<4>(GetParam());
    BigNum source{negative ? -6.0L : 6.0L};
    BigNum destination{-17};
    BigNum &result = call == IntegerDivisionCall::DISTINCT ? destination : source;
    std::vector<Byte> expected(g_bn_length, 0xff);
    expected.back() = 0x7f;
    if (divisor != 0)
    {
        expected = encoded_value(BigNum{(negative ? -6.0L : 6.0L) / divisor});
    }
    else if (negative)
    {
        std::fill(expected.begin(), expected.end(), Byte{0});
        expected.front() = 1;
        expected.back() = 0x80;
    }
    const std::vector<Byte> expected_source =
        call == IntegerDivisionCall::DISTINCT ? encoded_value(BigNum{6}) : expected;
    const std::vector<Byte> untouched = encoded_value(destination);
    const std::size_t stride = source.arena().block_size();
    std::fill(source.data() + g_bn_length, source.data() + stride, Byte{0x5a});
    std::fill(destination.data() + g_bn_length, destination.data() + stride, Byte{0xa5});
    const Byte *const address = result.data();
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const std::size_t high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    if (call == IntegerDivisionCall::IN_PLACE)
    {
        EXPECT_EQ(&result, &div_a_bn_int(result, divisor));
    }
    else
    {
        EXPECT_EQ(&result, &unsafe_div_bn_int(result, source, divisor));
    }

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(expected_source, encoded_value(source));
    if (call != IntegerDivisionCall::DISTINCT)
    {
        EXPECT_EQ(untouched, encoded_value(destination));
    }
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(std::vector<Byte>(stride - g_bn_length, 0x5a),
        std::vector<Byte>(source.data() + g_bn_length, source.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride - g_bn_length, 0xa5),
        std::vector<Byte>(destination.data() + g_bn_length, destination.data() + stride));
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
    const auto expected = encoded_value(BigNum{std::get<2>(GetParam()) ? -0.5L : 0.5L});
    const auto expected_source = encoded_value(BigNum{2});
    BigStackSaver saved;
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    const std::size_t stride = temporary.block_size();
    BigArena::Allocation before = temporary.allocate_single();
    BigNum result{temporary};
    BigArena::Allocation after = temporary.allocate_single();
    ASSERT_EQ(before.data() + stride, result.data());
    ASSERT_EQ(result.data() + stride, after.data());
    std::fill_n(before.data(), stride, Byte{0xa5});
    std::fill_n(after.data(), stride, Byte{0xa5});
    std::fill(result.data() + g_bn_length, result.data() + stride, Byte{0xa5});
    const Byte *const address = result.data();
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();
    const auto high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(expected_source, encoded_value(source));
    EXPECT_EQ(std::vector<Byte>(stride, 0xa5), std::vector<Byte>(before.data(), before.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride, 0xa5), std::vector<Byte>(after.data(), after.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride - g_bn_length, 0xa5),
        std::vector<Byte>(result.data() + g_bn_length, result.data() + stride));
}

TEST_P(TestBigNumKernelWindows, divisionCapturesBothInputsBeforeAnAliasedWrite)
{
    BigNum left{-6};
    BigNum right{2};
    const bool alias_left = std::get<2>(GetParam());
    BigNum &output = alias_left ? left : right;
    const auto expected = encoded_value(BigNum{-3});
    const auto other = encoded_value(alias_left ? right : left);
    auto *const result = output.data();
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();

    EXPECT_EQ(&output, &div_bn(output, left, right));
    EXPECT_EQ(result, output.data());

    EXPECT_EQ(expected, encoded_value(output));
    EXPECT_EQ(other, encoded_value(alias_left ? right : left));
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, reciprocalZeroKeepsSaturationAndPrecision)
{
    BigNum source{0};
    BigNum result{bn_arena(BigLifetime::WORKING)};
    std::vector<Byte> expected(g_bn_length, 0xff);
    expected.back() = 0x7f;
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const Byte *const address = result.data();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(BigNum{0}, source);
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, divisionCopiesASharedInputBeforeWritingItsDestination)
{
    BigNum value{std::get<2>(GetParam()) ? -2.0L : 2.0L};
    const std::vector<Byte> expected = encoded_value(BigNum{1});
    const std::size_t stride = value.arena().block_size();
    std::fill(value.data() + g_bn_length, value.data() + stride, Byte{0xa5});
    const Byte *const address = value.data();
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();
    const std::size_t high_water = value.arena().high_water_mark();

    EXPECT_EQ(&value, &div_bn(value, value, value));

    EXPECT_EQ(expected, encoded_value(value));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(std::vector<Byte>(stride - g_bn_length, 0xa5),
        std::vector<Byte>(value.data() + g_bn_length, value.data() + stride));
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, value.arena().high_water_mark());
}

TEST_P(TestBigNumKernelWindows, reciprocalOverflowKeepsEarlyReturnSignAndSourceMutation)
{
    const LDouble magnitude = std::ldexp(LDouble{1}, -40);
    BigNum source{std::get<2>(GetParam()) ? -magnitude : magnitude};
    BigNum result{bn_arena(BigLifetime::WORKING)};
    std::vector<Byte> expected(g_bn_length, 0xff);
    expected.back() = 0x7f;
    const std::vector<Byte> expected_source = encoded_value(BigNum{magnitude});
    const std::array<int, 9> lengths = kernel_lengths();
    const BigStackMark stack = save_stack();

    EXPECT_EQ(&result, &unsafe_inv_bn(result, source));

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(expected_source, encoded_value(source));
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBigNumKernelWindows, bigFloatMantissaProductsRestoreTheBigNumContext)
{
    const bool negative = std::get<2>(GetParam());
    BigFloat left{negative ? -0.75L : 0.75L};
    BigFloat right{0.5L};
    const auto original_left = encoded_value(left);
    const auto original_right = encoded_value(right);
    const auto expected_product = encoded_value(BigFloat{negative ? -0.375L : 0.375L});
    const auto expected_sqr = encoded_value(BigFloat{0.5625L});
    const auto lengths = kernel_lengths();
    const auto stack = save_stack();

    BigFloat product{bf_arena(BigLifetime::WORKING)};
    const std::size_t canonical_size = product.arena().value_size();
    const std::size_t stride = product.arena().block_size();
    std::fill_n(product.data(), stride, Byte{0xa5});
    EXPECT_EQ(&product, &mult_bf(product, left, right));

    EXPECT_EQ(expected_product, encoded_value(product));
    EXPECT_EQ(std::vector<Byte>(stride - canonical_size, 0xa5),
        std::vector<Byte>(product.data() + canonical_size, product.data() + stride));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
    EXPECT_EQ(lengths, kernel_lengths());

    EXPECT_EQ(&product, &square_bf(product, left));

    EXPECT_EQ(expected_sqr, encoded_value(product));
    EXPECT_EQ(std::vector<Byte>(stride - canonical_size, 0xa5),
        std::vector<Byte>(product.data() + canonical_size, product.data() + stride));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(lengths, kernel_lengths());
    EXPECT_EQ(stack, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndLayouts, TestBigNumKernelWindows,
    testing::Combine(testing::Values(12, 32, 672), testing::Values(1, 2, 4), testing::Bool()));

namespace
{

enum class KernelSlot
{
    TMP1,
    TMP2,
    TMP3,
    COPY1,
    COPY2
};

} // namespace

class TestBigNumKernelStorage : public TestBigNumValue, public testing::WithParamInterface<std::tuple<int, KernelSlot>>
{
protected:
    void SetUp() override;
    Byte *buffer() const;
    bool allocated() const;
    std::size_t reservation_size() const;
    void expect_owner() const;
};

void TestBigNumKernelStorage::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

Byte *TestBigNumKernelStorage::buffer() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
    {
        BigArena &working = bn_arena(BigLifetime::WORKING);
        const Byte *scratch{};
        // Probe the working arena without committing or changing its storage.
        working.prepare_relocation(working.value_size(), working.block_size(),
            [&](Byte *destination, const Byte *source, std::size_t blocks)
            {
                std::fill_n(destination, blocks * working.block_size(), Byte{0});
                if (is_bn_product_scratch(source))
                {
                    EXPECT_EQ(2U, blocks);
                    scratch = source;
                }
            });
        // The live arena storage is mutable outside the relocation callback.
        return const_cast<Byte *>(scratch);
    }
    case KernelSlot::TMP2:
        return g_bn_tmp2.data();
    case KernelSlot::TMP3:
        return g_bn_tmp3.data();
    case KernelSlot::COPY1:
        return g_bn_tmp_copy1.data();
    case KernelSlot::COPY2:
        return g_bn_tmp_copy2.data();
    }
    return nullptr;
}

bool TestBigNumKernelStorage::allocated() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        return buffer() != nullptr;
    case KernelSlot::TMP2:
        return static_cast<bool>(g_bn_tmp2);
    case KernelSlot::TMP3:
        return static_cast<bool>(g_bn_tmp3);
    case KernelSlot::COPY1:
        return static_cast<bool>(g_bn_tmp_copy1);
    case KernelSlot::COPY2:
        return static_cast<bool>(g_bn_tmp_copy2);
    }
    return false;
}

std::size_t TestBigNumKernelStorage::reservation_size() const
{
    const KernelSlot slot = std::get<1>(GetParam());
    const std::size_t width = slot == KernelSlot::TMP2 || slot == KernelSlot::TMP3 ? 1 : 2;
    return width * bn_arena(BigLifetime::WORKING).block_size();
}

void TestBigNumKernelStorage::expect_owner() const
{
    static_assert(std::is_same_v<decltype(g_bn_tmp2), BigNum>);
    static_assert(std::is_same_v<decltype(g_bn_tmp3), BigNum>);
    static_assert(std::is_same_v<decltype(g_bn_tmp_copy1), BigArena::Allocation>);
    static_assert(std::is_same_v<decltype(g_bn_tmp_copy2), BigArena::Allocation>);
    BigArena &working = bn_arena(BigLifetime::WORKING);
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        EXPECT_NE(nullptr, buffer());
        break;
    case KernelSlot::TMP2:
        EXPECT_EQ(&working, &g_bn_tmp2.arena());
        break;
    case KernelSlot::TMP3:
        EXPECT_EQ(&working, &g_bn_tmp3.arena());
        break;
    case KernelSlot::COPY1:
        EXPECT_EQ(&working, &g_bn_tmp_copy1.arena());
        EXPECT_EQ(2U, g_bn_tmp_copy1.blocks());
        break;
    case KernelSlot::COPY2:
        EXPECT_EQ(&working, &g_bn_tmp_copy2.arena());
        EXPECT_EQ(2U, g_bn_tmp_copy2.blocks());
        break;
    }
}

TEST_P(TestBigNumKernelStorage, rebindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    auto *const original = buffer();
    std::fill_n(original, reservation_size(), Byte{0xa5});
    const auto high_water = bn_arena(BigLifetime::WORKING).high_water_mark();

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(original, buffer());
    EXPECT_EQ(high_water, bn_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigNumKernelStorage, kernelWritesPreserveBytesBeyondTheDocumentedExtent)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    const KernelSlot slot = std::get<1>(GetParam());
    const std::size_t extent = slot == KernelSlot::TMP1 ? g_r_length : g_bn_length;
    ASSERT_LE(extent, reservation_size());
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});

    const BigNum quotient = BigNum{6} / BigNum{2};

    EXPECT_EQ(BigNum{3}, quotient);
    EXPECT_EQ(std::vector<Byte>(reservation_size() - extent, 0xa5),
        std::vector<Byte>(buffer() + extent, buffer() + reservation_size()));
}

TEST_P(TestBigNumKernelStorage, precisionChangesDiscardTheWholeScratchReservation)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});
    const int original_length = std::get<0>(GetParam());

    init_bf_length(original_length == 672 ? 4 : 672);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
    std::fill_n(buffer(), reservation_size(), Byte{0x5a});

    init_bf_length(original_length);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigNumKernelStorage, repeatedCleanupUnbindsAndReinitializationClearsScratch)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});

    free_bf_vars();

    EXPECT_FALSE(allocated());

    free_bf_vars();

    EXPECT_FALSE(allocated());

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndSlots, TestBigNumKernelStorage,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672),
        testing::Values(KernelSlot::TMP1, KernelSlot::TMP2, KernelSlot::TMP3, KernelSlot::COPY1, KernelSlot::COPY2)));

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
    const std::vector<Byte> original_left = encoded_value(left);
    const std::vector<Byte> original_right = encoded_value(right);
    std::vector<Byte> expected = encoded_integer(squared ? 56 : -4);
    expected[g_bn_length - g_int_length - 1] = 0x40;
    BigArena &working = bn_arena(BigLifetime::WORKING);
    BigArena::Allocation reservation = working.allocate_double();
    const Byte *const address = reservation.data();
    std::fill_n(reservation.data(), 2 * working.block_size(), Byte{0xa5});
    reservation.reset();
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();

    {
        const BigNum result = squared ? left.sqr() : left * right;

        EXPECT_EQ(&working, &result.arena());
        EXPECT_EQ(address, result.data());
        EXPECT_EQ(expected, encoded_value(result));
        EXPECT_EQ(original_left, encoded_value(left));
        EXPECT_EQ(original_right, encoded_value(right));
        EXPECT_EQ(high_water, working.high_water_mark());
        EXPECT_EQ(mark, save_stack());
        BigArena::Allocation companion = working.allocate_single();
        ASSERT_EQ(address + working.block_size(), companion.data());
        EXPECT_EQ(1U, companion.blocks());
        std::fill_n(companion.data(), working.block_size(), Byte{0x5a});
        EXPECT_EQ(expected, encoded_value(result));
    }

    const BigArena::Allocation reused = working.allocate_double();
    EXPECT_EQ(address, reused.data());
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
