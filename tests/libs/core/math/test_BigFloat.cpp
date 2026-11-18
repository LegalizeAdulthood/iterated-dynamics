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
static_assert(std::is_same_v<decltype(str_to_bf(std::declval<BigFloat &>(), std::string_view{})), BigFloat &>);
static_assert(std::is_same_v<decltype(int_to_bf(std::declval<BigFloat &>(), long{})), BigFloat &>);
static_assert(std::is_same_v<decltype(float_to_bf(std::declval<BigFloat &>(), LDouble{})), BigFloat &>);
static_assert(std::is_same_v<decltype(bf_to_float(std::declval<BigFloat &>())), LDouble>);
static_assert(
    std::is_same_v<decltype(copy_bf(std::declval<BigFloat &>(), std::declval<const BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(clear_bf(std::declval<BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(max_bf(std::declval<BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(&bf_to_string), std::string (*)(const BigFloat &, int)>);
static_assert(std::is_same_v<decltype(&bf_to_string_e), std::string (*)(const BigFloat &, int)>);
static_assert(std::is_same_v<decltype(&bf_to_string_f), std::string (*)(const BigFloat &, int)>);

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

enum class BigFloatStorageProcedure
{
    COPY,
    CLEAR,
    MAXIMUM
};

struct BigFloatStorageInput
{
    BigFloatStorageProcedure operation;
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> expected_tail;
};

class TestBigFloatStorageProcedures
    : public TestBigFloatValue,
      public testing::WithParamInterface<std::tuple<int, BigLifetime, BigFloatStorageInput>>
{
};

TEST_P(TestBigFloatStorageProcedures, preservesBytesDestinationSourcePaddingAndAllocation)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatStorageInput &input = std::get<2>(GetParam());
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    std::vector<Byte> source_bytes(size);
    std::copy(input.source_tail.begin(), input.source_tail.end(), source_bytes.end() - 5);
    BigFloat source = BigFloat::from_legacy(source_bytes.data(), source_bytes.size());
    std::fill_n(source.data() + size, source.arena().block_size() - size, Byte{0x3c});
    const std::vector<Byte> original_source(source.data(), source.data() + source.arena().block_size());
    BigFloat destination{arena};
    const Byte *const address = destination.data();
    const std::size_t stride = arena.block_size();
    std::fill_n(destination.data(), stride, Byte{0xa5});
    std::vector<Byte> expected(stride, 0xa5);
    std::fill_n(expected.begin(), size, Byte{0});
    std::copy(input.expected_tail.begin(), input.expected_tail.end(), expected.begin() + size - 5);
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};

    switch (input.operation)
    {
    case BigFloatStorageProcedure::COPY:
        EXPECT_EQ(&destination, &copy_bf(destination, std::as_const(source)));
        break;
    case BigFloatStorageProcedure::CLEAR:
        EXPECT_EQ(&destination, &clear_bf(destination));
        break;
    case BigFloatStorageProcedure::MAXIMUM:
        EXPECT_EQ(&destination, &max_bf(destination));
        break;
    }

    EXPECT_EQ(expected, std::vector<Byte>(destination.data(), destination.data() + stride));
    EXPECT_EQ(original_source, std::vector<Byte>(source.data(), source.data() + source.arena().block_size()));
    EXPECT_EQ(address, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsLifetimesAndOperations, TestBigFloatStorageProcedures,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY),
        testing::Values(BigFloatStorageInput{BigFloatStorageProcedure::COPY, {0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}},
            BigFloatStorageInput{BigFloatStorageProcedure::COPY, {0xc0, 0xfe, 0xff, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}},
            BigFloatStorageInput{BigFloatStorageProcedure::COPY, {}, {}},
            BigFloatStorageInput{BigFloatStorageProcedure::CLEAR, {0xc0, 0xfe, 0xff, 0, 0}, {}},
            BigFloatStorageInput{BigFloatStorageProcedure::MAXIMUM, {0xc0, 0xfe, 0xff, 0, 0},
                {0, 1, 0, static_cast<Byte>((std::numeric_limits<long double>::max_exponent / 8) % 256),
                    static_cast<Byte>((std::numeric_limits<long double>::max_exponent / 8) / 256)}})));

class TestBigFloatSign : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigFloatSign, observesConstValueWithoutChangingItsBytesOrStorage)
{
    init_bf_length(std::get<0>(GetParam()));
    const int sign = std::get<1>(GetParam());
    const BigFloat value{static_cast<LDouble>(sign) / 2};
    const std::vector<Byte> original = encoded_value(value);
    const Byte *const address = value.data();
    const BigArena &arena = value.arena();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(sign, sign_bf(value));
    EXPECT_EQ(sign == 0, is_bf_zero(value));
    EXPECT_EQ(sign < 0, is_bf_neg(value));
    EXPECT_EQ(sign != 0, is_bf_not_zero(value));

    EXPECT_EQ(original, encoded_value(value));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(
    PrecisionsAndSigns, TestBigFloatSign, testing::Combine(testing::Values(4, 64, 708), testing::Values(-1, 0, 1)));

struct BigFloatComparisonInput
{
    LDouble left;
    LDouble right;
    int offset;
};

class TestBigFloatComparison : public TestBigFloatValue,
                               public testing::WithParamInterface<std::tuple<int, BigFloatComparisonInput>>
{
};

TEST_P(TestBigFloatComparison, preservesTheMismatchPositionAndBothOperands)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatComparisonInput &input = std::get<1>(GetParam());
    const BigFloat left{input.left};
    const BigFloat right{input.right};
    const std::vector<Byte> left_bytes = encoded_value(left);
    const std::vector<Byte> right_bytes = encoded_value(right);
    const Byte *const left_address = left.data();
    const Byte *const right_address = right.data();
    const BigArena &arena = left.arena();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int position = g_bf_length - input.offset;

    EXPECT_EQ(-position, cmp_bf(left, right));
    EXPECT_EQ(position, cmp_bf(right, left));
    EXPECT_EQ(0, cmp_bf(left, left));

    EXPECT_EQ(left_bytes, encoded_value(left));
    EXPECT_EQ(right_bytes, encoded_value(right));
    EXPECT_EQ(left_address, left.data());
    EXPECT_EQ(right_address, right.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigFloatComparison,
    testing::Combine(testing::Values(4, 64, 708),
        testing::Values(BigFloatComparisonInput{-1.25L, 0.0L, 0}, BigFloatComparisonInput{0.5L, 1.25L, 0},
            BigFloatComparisonInput{-512.0L, -1.5L, 0}, BigFloatComparisonInput{1.25L, 2.25L, 1},
            BigFloatComparisonInput{-1.5L, -1.25L, 2})));

class TestBigFloatNegation : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatNegation, preservesDestinationIdentitySourceAndPadding)
{
    init_bf_length(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat source{arena, 1.0L};
    BigFloat result{arena};
    const Byte *const source_address = source.data();
    const Byte *const result_address = result.data();
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill_n(source.data() + size, stride - size, Byte{0x5a});
    std::fill_n(result.data(), stride, Byte{0xa5});
    const std::vector<Byte> original(source.data(), source.data() + stride);
    std::vector<Byte> negative(size);
    std::fill_n(negative.end() - 3, 3, Byte{0xff});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &neg_bf(result, std::as_const(source)));
    EXPECT_EQ(negative, encoded_value(result));
    EXPECT_EQ(original, std::vector<Byte>(source.data(), source.data() + stride));

    EXPECT_EQ(&source, &neg_bf(source, std::as_const(source)));
    EXPECT_EQ(negative, encoded_value(source));
    EXPECT_EQ(&source, &neg_a_bf(source));
    EXPECT_EQ(original, std::vector<Byte>(source.data(), source.data() + stride));

    EXPECT_EQ(source_address, source.data());
    EXPECT_EQ(result_address, result.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(result.data() + size, result.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatNegation, testing::Values(4, 64, 708));

struct BigFloatParseCase
{
    std::string_view text;
    std::array<Byte, 5> tail;
};

class TestBigFloatParseInterface : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, BigFloatParseCase>>
{
};

TEST_P(TestBigFloatParseInterface, preservesBytesDestinationAndExistingWorkspace)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatParseCase &input = std::get<1>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat value{arena};
    const Byte *const address = value.data();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::size_t stride = arena.block_size();
    std::vector<Byte> expected(size);
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 5);
    std::fill_n(value.data(), stride, Byte{0xa5});
    std::fill_n(g_bf_tmp1.data(), stride, Byte{0x3c});
    std::fill_n(g_bf_tmp2.data(), stride, Byte{0x5a});
    const Byte *const scratch = g_bf_tmp1.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    EXPECT_EQ(&value, &str_to_bf(value, input.text));

    EXPECT_EQ(expected, encoded_value(value));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(scratch, g_bf_tmp1.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(value.data() + size, value.data() + stride));
    EXPECT_EQ(
        std::vector<Byte>(stride - size, 0x3c), std::vector<Byte>(g_bf_tmp1.data() + size, g_bf_tmp1.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride, 0x5a), std::vector<Byte>(g_bf_tmp2.data(), g_bf_tmp2.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigFloatParseInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatParseCase{"1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"-1.25", {0xc0, 0xfe, 0xff, 0x00, 0x00}},
            BigFloatParseCase{"+1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{std::string_view{"1.25x", 4}, {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"1.25e2", {0x00, 0x7d, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"1.25E+2tail", {0x00, 0x7d, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"125e-2", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"0.5", {0x00, 0x80, 0x00, 0xff, 0xff}},
            BigFloatParseCase{"1.00390625", {0x01, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"junk1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatParseCase{"1.25e+", {0x40, 0x01, 0x00, 0x00, 0x00}}, BigFloatParseCase{"0", {}},
            BigFloatParseCase{"-0", {}}, BigFloatParseCase{"", {}}, BigFloatParseCase{"+", {}},
            BigFloatParseCase{"-", {}}, BigFloatParseCase{".", {}}, BigFloatParseCase{"garbage", {}},
            BigFloatParseCase{"1.25x", {}})));

struct BigFloatIntegerInput
{
    long value;
    std::array<Byte, 7> tail;
};

class TestBigFloatIntegerInterface : public TestBigFloatValue,
                                     public testing::WithParamInterface<std::tuple<int, BigFloatIntegerInput>>
{
};

TEST_P(TestBigFloatIntegerInterface, preservesBytesDestinationAndAllocation)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatIntegerInput &input = std::get<1>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat value{arena};
    const Byte *const address = value.data();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::size_t stride = arena.block_size();
    std::vector<Byte> expected(size);
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 7);
    std::fill_n(value.data(), stride, Byte{0xa5});
    std::fill_n(g_bf_tmp1.data(), stride, Byte{0x3c});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    EXPECT_EQ(&value, &int_to_bf(value, input.value));

    EXPECT_EQ(expected, encoded_value(value));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(value.data() + size, value.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride, 0x3c), std::vector<Byte>(g_bf_tmp1.data(), g_bf_tmp1.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndIntegers, TestBigFloatIntegerInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatIntegerInput{0, {}},
            BigFloatIntegerInput{1, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
            BigFloatIntegerInput{-1, {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff}},
            BigFloatIntegerInput{127, {0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00}},
            BigFloatIntegerInput{-127, {0x00, 0x00, 0x00, 0x81, 0xff, 0x00, 0x00}},
            BigFloatIntegerInput{128, {0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00}},
            BigFloatIntegerInput{-128, {0x00, 0x00, 0x00, 0x80, 0xff, 0x00, 0x00}},
            BigFloatIntegerInput{255, {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}},
            BigFloatIntegerInput{-255, {0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00}},
            BigFloatIntegerInput{256, {0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00}},
            BigFloatIntegerInput{-256, {0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00}},
            BigFloatIntegerInput{65535, {0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0x00}},
            BigFloatIntegerInput{-65535, {0x00, 0x00, 0x01, 0x00, 0xff, 0x01, 0x00}},
            BigFloatIntegerInput{65536, {0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00}},
            BigFloatIntegerInput{-65536, {0x00, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00}},
            BigFloatIntegerInput{std::numeric_limits<S32>::max(), {0xff, 0xff, 0xff, 0x7f, 0x00, 0x03, 0x00}},
            BigFloatIntegerInput{std::numeric_limits<S32>::min(), {0x00, 0x00, 0x00, 0x80, 0xff, 0x03, 0x00}})));

struct BigFloatFloatingInput
{
    LDouble value;
    std::array<Byte, 7> tail;
    int fractional_bit{};
};

class TestBigFloatFloatingInput : public TestBigFloatValue,
                                  public testing::WithParamInterface<std::tuple<int, int, BigFloatFloatingInput>>
{
};

TEST_P(TestBigFloatFloatingInput, preservesBytesDestinationAndSharedLengthsWithoutAllocating)
{
    init_bf_length(std::get<0>(GetParam()));
    const ValueSaver<int> integer_length{g_int_length, std::get<1>(GetParam())};
    const BigFloatFloatingInput &input = std::get<2>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat value{arena};
    const Byte *const address = value.data();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::size_t stride = arena.block_size();
    std::vector<Byte> expected(size);
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 7);
    if (input.fractional_bit != 0)
    {
        const int position = g_bf_length - 2 - (input.fractional_bit + 7) / 8;
        if (position >= 0)
        {
            expected[position] |= static_cast<Byte>(1U << ((8 - input.fractional_bit % 8) % 8));
        }
    }
    std::fill_n(value.data(), stride, Byte{0xa5});
    std::fill_n(g_bf_tmp1.data(), stride, Byte{0x3c});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;

    EXPECT_EQ(&value, &float_to_bf(value, input.value));

    EXPECT_EQ(expected, encoded_value(value));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(value.data() + size, value.data() + stride));
    EXPECT_EQ(std::vector<Byte>(stride, 0x3c), std::vector<Byte>(g_bf_tmp1.data(), g_bf_tmp1.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsIntegerLengthsAndInputs, TestBigFloatFloatingInput,
    testing::Combine(testing::Values(4, 12, 60, 64, 708), testing::Values(1, 2, 4),
        testing::Values(BigFloatFloatingInput{0, {}}, BigFloatFloatingInput{-0.0L, {}},
            BigFloatFloatingInput{1, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
            BigFloatFloatingInput{-1, {0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}},
            BigFloatFloatingInput{1.25L, {0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatFloatingInput{-1.25L, {0x00, 0x00, 0xc0, 0xfe, 0xff, 0x00, 0x00}},
            BigFloatFloatingInput{0.5L, {0x00, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff}},
            BigFloatFloatingInput{-0.5L, {0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff}},
            BigFloatFloatingInput{255, {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}},
            BigFloatFloatingInput{256, {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}},
            BigFloatFloatingInput{257, {0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00}},
            BigFloatFloatingInput{65536, {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00}},
            BigFloatFloatingInput{1.0L / 256, {0x00, 0x00, 0x00, 0x01, 0x00, 0xff, 0xff}},
            BigFloatFloatingInput{1.0L / 65536, {0x00, 0x00, 0x00, 0x01, 0x00, 0xfe, 0xff}},
            BigFloatFloatingInput{std::ldexp(1.0L, 1023), {0x00, 0x00, 0x00, 0x80, 0x00, 0x7f, 0x00}},
            BigFloatFloatingInput{-std::ldexp(1.0L, 1023), {0x00, 0x00, 0x00, 0x80, 0xff, 0x7f, 0x00}},
            BigFloatFloatingInput{std::ldexp(1.0L, -1022), {0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0xff}},
            BigFloatFloatingInput{1.0L + std::ldexp(1.0L, -52), {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, 52})));

struct BigFloatOutputInput
{
    std::array<Byte, 7> tail;
    LDouble expected;
};

class TestBigFloatOutputInterface : public TestBigFloatValue,
                                    public testing::WithParamInterface<std::tuple<int, int, BigFloatOutputInput>>
{
};

TEST_P(TestBigFloatOutputInterface, restoresInputAndLengthsAndPreservesTheExistingCopyPolicy)
{
    init_bf_length(std::get<0>(GetParam()));
    const ValueSaver<int> integer_length{g_int_length, std::get<1>(GetParam())};
    const BigFloatOutputInput &input = std::get<2>(GetParam());
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    std::vector<Byte> bytes(size);
    std::copy(input.tail.begin(), input.tail.end(), bytes.end() - 7);
    BigFloat value = BigFloat::from_legacy(bytes.data(), bytes.size());
    BigArena &arena = value.arena();
    const std::size_t stride = arena.block_size();
    const std::size_t copy_size = 2 * stride;
    std::fill(value.data() + size, value.data() + stride, Byte{0xa5});
    const std::vector<Byte> original(value.data(), value.data() + stride);
    std::fill_n(g_bf_tmp_copy1.data(), copy_size, Byte{0x5a});
    std::fill_n(g_bf_tmp_copy2.data(), copy_size, Byte{0x3c});
    const Byte *const address = value.data();
    const Byte *const copy = g_bf_tmp_copy1.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;

    const LDouble actual = bf_to_float(value);

    EXPECT_EQ(input.expected, actual);
    EXPECT_EQ(std::signbit(input.expected), std::signbit(actual));
    EXPECT_EQ(original, std::vector<Byte>(value.data(), value.data() + stride));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x5a),
        std::vector<Byte>(g_bf_tmp_copy1.data(), g_bf_tmp_copy1.data() + copy_size));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x3c),
        std::vector<Byte>(g_bf_tmp_copy2.data(), g_bf_tmp_copy2.data() + copy_size));

    const BigFloat &source = value;
    const LDouble member_result = source.to_float();

    EXPECT_EQ(input.expected, member_result);
    EXPECT_EQ(std::signbit(input.expected), std::signbit(member_result));
    EXPECT_EQ(bytes, std::vector<Byte>(g_bf_tmp_copy1.data(), g_bf_tmp_copy1.data() + size));
    EXPECT_EQ(std::vector<Byte>(copy_size - size, 0x5a),
        std::vector<Byte>(g_bf_tmp_copy1.data() + size, g_bf_tmp_copy1.data() + copy_size));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x3c),
        std::vector<Byte>(g_bf_tmp_copy2.data(), g_bf_tmp_copy2.data() + copy_size));
    EXPECT_EQ(original, std::vector<Byte>(value.data(), value.data() + stride));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(copy, g_bf_tmp_copy1.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsIntegerLengthsAndInputs, TestBigFloatOutputInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708), testing::Values(1, 2, 4),
        testing::Values(BigFloatOutputInput{{}, 0}, BigFloatOutputInput{{0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, 1},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}, -1},
            BigFloatOutputInput{{0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00}, 1.25L},
            BigFloatOutputInput{{0x00, 0x00, 0xc0, 0xfe, 0xff, 0x00, 0x00}, -1.25L},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff}, 0.5L},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff}, -0.5L},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}, 255},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}, 256},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00}, 32767},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00}, -32768},
            BigFloatOutputInput{{0x00, 0x00, 0x40, 0x01, 0x00, 0x0c, 0x00}, std::ldexp(1.25L, 96)},
            BigFloatOutputInput{{0x00, 0x00, 0xc0, 0xfe, 0xff, 0xf4, 0xff}, -std::ldexp(1.25L, -96)},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x01, 0x00, 0xb8, 0x0b}, std::numeric_limits<LDouble>::infinity()},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0xff, 0xff, 0xb8, 0x0b}, -std::numeric_limits<LDouble>::infinity()},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0x01, 0x00, 0x48, 0xf4}, 0.0L},
            BigFloatOutputInput{{0x00, 0x00, 0x00, 0xff, 0xff, 0x48, 0xf4}, -0.0L})));

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

TEST_F(TestBigFloatValue, importsEncodedBytesWithoutMachineFloatConversion)
{
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloat source{bf_arena(BigLifetime::TEMPORARY)};
    str_to_bf(source, "-1.23456789012345678901234567890123456789e-400");
    const std::vector<Byte> expected(source.data(), source.data() + size);

    const BigFloat value = BigFloat::from_legacy(source.data(), size);
    std::fill_n(source.data(), size, Byte{0});

    EXPECT_EQ(expected, encoded_value(value));
}

struct BigFloatUnpackInput
{
    int bn_length;
    int stored_length;
    std::vector<Byte> source;
    std::vector<Byte> expected;
};

class TestBigFloatUnpack : public TestBigFloatValue,
                           public testing::WithParamInterface<std::tuple<BigLifetime, BigFloatUnpackInput>>
{
};

TEST_P(TestBigFloatUnpack, preservesDestinationStorageSourceCursorAndPrecision)
{
    const BigFloatUnpackInput &input = std::get<1>(GetParam());
    init_bf_length(input.bn_length);
    BigArena &arena = bf_arena(std::get<0>(GetParam()));
    BigFloat destination{arena};
    Byte *const address = destination.data();
    const std::size_t stride = arena.block_size();
    std::fill_n(address, stride, Byte{0xcc});
    std::vector<Byte> source{0xaa};
    source.insert(source.end(), input.source.begin(), input.source.end());
    source.push_back(0xbb);
    const std::vector<Byte> original = source;
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    const Byte *const cursor = unpack_bf(destination, source.data() + 1, input.stored_length);

    EXPECT_EQ(source.data() + source.size() - 1, cursor);
    EXPECT_EQ(input.expected, encoded_value(destination));
    EXPECT_EQ(original, source);
    EXPECT_EQ(address, destination.data());
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(std::vector<Byte>(stride - input.expected.size(), 0xcc),
        std::vector<Byte>(address + input.expected.size(), address + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLifetimes, TestBigFloatUnpack,
    testing::Combine(testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY),
        testing::Values(BigFloatUnpackInput{8, 8, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
                            {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
            BigFloatUnpackInput{4, 12,
                {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff},
                {0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff}},
            BigFloatUnpackInput{4, 8, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
                {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}})));

TEST_F(TestBigFloatValue, valueSurvivesArenaRewindAndReuse)
{
    // Borrow BigStack buffers here to test importing bytes across a real rewind.
    const auto marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    BigFloatPtr source = bf_stack().allocate_single();
    BigFloat{"1.23456789012345678901234567890123456789"}.copy_to_legacy(source, size);
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

    float_to_bf(value, 2.5L);
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

class TestBigFloatAddition : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatAddition, preservesOperandsAliasesDestinationIdentityAndPadding)
{
    init_bf_length(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat left{arena, 1.25L};
    const BigFloat right{arena, -0.5L};
    const std::vector<Byte> original_left = encoded_value(left);
    const std::vector<Byte> original_right = encoded_value(right);
    BigFloat left_alias{arena, left};
    BigFloat right_alias{arena, right};
    BigFloat result{arena};
    const Byte *const address = result.data();
    const Byte *const left_address = left_alias.data();
    const Byte *const right_address = right_alias.data();
    const std::size_t size = arena.value_size();
    const std::size_t padding = arena.block_size() - size;
    std::fill_n(result.data(), arena.block_size(), Byte{0xa5});
    std::fill_n(left_alias.data() + size, padding, Byte{0x5a});
    std::fill_n(right_alias.data() + size, padding, Byte{0x3c});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const std::array<Byte, 5> sum_tail{0, 0xc0, 0, 0xff, 0xff};
    std::vector<Byte> expected(size);
    std::copy(sum_tail.begin(), sum_tail.end(), expected.end() - 5);

    EXPECT_EQ(&result, &add_bf(result, left, right));
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(&left_alias, &add_bf(left_alias, left_alias, right));
    EXPECT_EQ(expected, encoded_value(left_alias));
    EXPECT_EQ(&right_alias, &add_bf(right_alias, left, right_alias));
    EXPECT_EQ(expected, encoded_value(right_alias));
    copy_bf(result, left);
    EXPECT_EQ(&result, &add_a_bf(result, right));
    EXPECT_EQ(expected, encoded_value(result));

    copy_bf(result, left);
    const std::array<Byte, 5> doubled_tail{0x80, 2, 0, 0, 0};
    std::copy(doubled_tail.begin(), doubled_tail.end(), expected.end() - 5);
    EXPECT_EQ(&result, &add_a_bf(result, result));
    EXPECT_EQ(expected, encoded_value(result));

    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(left_address, left_alias.data());
    EXPECT_EQ(right_address, right_alias.data());
    EXPECT_EQ(
        std::vector<Byte>(padding, 0xa5), std::vector<Byte>(result.data() + size, result.data() + size + padding));
    EXPECT_EQ(std::vector<Byte>(padding, 0x5a),
        std::vector<Byte>(left_alias.data() + size, left_alias.data() + size + padding));
    EXPECT_EQ(std::vector<Byte>(padding, 0x3c),
        std::vector<Byte>(right_alias.data() + size, right_alias.data() + size + padding));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatAddition, testing::Values(4, 64, 708));

class TestBigFloatSubtraction : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatSubtraction, preservesOperandsAliasesDestinationIdentityAndPadding)
{
    init_bf_length(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat left{arena, 1.25L};
    const BigFloat right{arena, -0.5L};
    const std::vector<Byte> original_left = encoded_value(left);
    const std::vector<Byte> original_right = encoded_value(right);
    BigFloat left_alias{arena, left};
    BigFloat right_alias{arena, right};
    BigFloat result{arena};
    const Byte *const address = result.data();
    const Byte *const left_address = left_alias.data();
    const Byte *const right_address = right_alias.data();
    const std::size_t size = arena.value_size();
    const std::size_t padding = arena.block_size() - size;
    std::fill_n(result.data(), arena.block_size(), Byte{0xa5});
    std::fill_n(left_alias.data() + size, padding, Byte{0x5a});
    std::fill_n(right_alias.data() + size, padding, Byte{0x3c});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const std::array<Byte, 5> difference_tail{0xc0, 1, 0, 0, 0};
    std::vector<Byte> expected(size);
    std::copy(difference_tail.begin(), difference_tail.end(), expected.end() - 5);

    EXPECT_EQ(&result, &sub_bf(result, left, right));
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(&left_alias, &sub_bf(left_alias, left_alias, right));
    EXPECT_EQ(expected, encoded_value(left_alias));
    EXPECT_EQ(&right_alias, &sub_bf(right_alias, left, right_alias));
    EXPECT_EQ(expected, encoded_value(right_alias));
    copy_bf(result, left);
    EXPECT_EQ(&result, &sub_a_bf(result, right));
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(&result, &abs_a_bf(sub_bf(result, right, left)));
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(&result, &sub_a_bf(result, result));
    EXPECT_EQ(std::vector<Byte>(size), encoded_value(result));

    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(left_address, left_alias.data());
    EXPECT_EQ(right_address, right_alias.data());
    EXPECT_EQ(
        std::vector<Byte>(padding, 0xa5), std::vector<Byte>(result.data() + size, result.data() + size + padding));
    EXPECT_EQ(std::vector<Byte>(padding, 0x5a),
        std::vector<Byte>(left_alias.data() + size, left_alias.data() + size + padding));
    EXPECT_EQ(std::vector<Byte>(padding, 0x3c),
        std::vector<Byte>(right_alias.data() + size, right_alias.data() + size + padding));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatSubtraction, testing::Values(4, 64, 708));

TEST_P(TestBigFloatArithmetic, legacyArithmeticSamplesPreserveOperands)
{
    BigFloat left{"-7.5"};
    BigFloat right{"2.5"};
    const std::vector<Byte> original_left = encoded_value(left);
    const std::vector<Byte> original_right = encoded_value(right);
    BigFloat result{bf_arena(BigLifetime::WORKING)};

    add_bf(result, left, right);
    EXPECT_EQ("-5.0", bf_to_string(result, 8));
    sub_bf(result, left, right);
    EXPECT_EQ("-10.0", bf_to_string(result, 8));
    mult_bf(result, left, right);
    EXPECT_EQ("-18.75", bf_to_string(result, 8));
    div_bf(result.data(), left.data(), right.data());
    EXPECT_EQ("-3.0", bf_to_string(result, 8));

    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigFloatArithmetic, legacyInPlaceSamplesPreserveTheRightOperand)
{
    BigFloat left{"-7.5"};
    BigFloat right{"2.5"};
    const std::vector<Byte> original_right = encoded_value(right);

    add_a_bf(left, right);
    EXPECT_EQ("-5.0", bf_to_string(left, 8));
    sub_a_bf(left, right);
    EXPECT_EQ("-7.5", bf_to_string(left, 8));
    add_a_bf(left, left);
    EXPECT_EQ("-15.0", bf_to_string(left, 8));
    sub_a_bf(left, left);
    EXPECT_EQ("0.0", bf_to_string(left, 8));

    EXPECT_EQ(original_right, encoded_value(right));
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
    auto raw_divisor = original_divisor;
    BigFloat expected{bf_arena(BigLifetime::WORKING)};
    add_bf(expected, left, right);
    div_bf(expected.data(), expected.data(), raw_divisor.data());
    mult_bf(expected, expected, left);
    sub_bf(expected, expected, right);
    const auto marker = save_stack();

    const BigFloat result = ((left + right) / divisor) * left - right;
    const BigFloat unrelated = BigFloat{"-8"} * BigFloat{"-9"};
    bf_stack().allocate_single();
    restore_stack(marker);

    EXPECT_EQ(encoded_value(expected), encoded_value(result));
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

class TestBigFloatScalarMultiply : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, const char *, U16>>
{
};

TEST_P(TestBigFloatScalarMultiply, operatorMatchesDestinationFormAndPreservesSource)
{
    init_bf_length(std::get<0>(GetParam()));
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    const BigFloat source{persistent, std::get<1>(GetParam())};
    const U16 factor = std::get<2>(GetParam());
    const std::vector<Byte> original = encoded_value(source);
    BigFloat expected{persistent};
    mult_bf_int(expected, source, factor);

    const BigFloat result = source * factor;

    EXPECT_EQ(encoded_value(expected), encoded_value(result));
    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &result.arena());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsValuesAndFactors, TestBigFloatScalarMultiply,
    testing::Combine(testing::Values(4, 32, 708),
        testing::Values("0", "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400"),
        testing::Values(U16{0}, U16{1}, U16{2}, U16{255}, U16{256}, U16{32768}, U16{65535})));

TEST_P(TestBigFloatArithmetic, unsignedScalingPreservesDestinationAliasesPaddingAndStorage)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, 2.0L};
    BigFloat result{arena};
    const Byte *const address = result.data();
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill_n(result.data(), stride, Byte{0xa5});
    const std::vector<Byte> original = encoded_value(source);
    const std::array<Byte, 6> tail{0xfe, 0xff, 1, 0, 2, 0};
    std::vector<Byte> expected(stride, 0xa5);
    std::fill_n(expected.begin(), size, Byte{0});
    std::copy(tail.begin(), tail.end(), expected.begin() + size - tail.size());
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &mult_bf_int(result, source, 65535));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &mult_bf_int(result, std::as_const(result), 65535));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &mult_a_bf_int(result, 65535));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
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

class TestBigFloatAbs : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, const char *>>
{
};

TEST_P(TestBigFloatAbs, absoluteValuePreservesItsOperandAndDestinationArena)
{
    init_bf_length(std::get<0>(GetParam()));
    const char *text = std::get<1>(GetParam());
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    const BigFloat source{persistent, text};
    const std::vector<Byte> original = encoded_value(source);
    const BigFloat magnitude{persistent, *text == '-' ? text + 1 : text};
    const std::vector<Byte> expected = encoded_value(magnitude);

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

TEST_P(TestBigFloatAbs, destinationFormsPreserveIdentityPaddingAndStorage)
{
    init_bf_length(std::get<0>(GetParam()));
    const char *text = std::get<1>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, text};
    const BigFloat magnitude{arena, *text == '-' ? text + 1 : text};
    const std::vector<Byte> original = encoded_value(source);
    const std::vector<Byte> expected = encoded_value(magnitude);
    BigFloat result{arena};
    BigFloat in_place{arena, source};
    const Byte *const result_address = result.data();
    const Byte *const in_place_address = in_place.data();
    const std::size_t size = arena.value_size();
    const std::size_t padding = arena.block_size() - size;
    std::fill_n(result.data(), arena.block_size(), Byte{0xa5});
    std::fill_n(in_place.data() + size, padding, Byte{0x5a});
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &abs_bf(result, source));
    EXPECT_EQ(&in_place, &abs_a_bf(in_place));

    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(expected, encoded_value(in_place));
    EXPECT_EQ(result_address, result.data());
    EXPECT_EQ(in_place_address, in_place.data());
    EXPECT_EQ(
        std::vector<Byte>(padding, 0xa5), std::vector<Byte>(result.data() + size, result.data() + size + padding));
    EXPECT_EQ(
        std::vector<Byte>(padding, 0x5a), std::vector<Byte>(in_place.data() + size, in_place.data() + size + padding));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndValues, TestBigFloatAbs,
    testing::Combine(testing::Values(4, 32, 708),
        testing::Values("0", "2.5", "-2.5", "1.2345678901234567890123456789e-400",
            "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400",
            "-1.2345678901234567890123456789e400")));

TEST_P(TestBigFloatArithmetic, constMachineConversionRetainsLegacyResultsAndInputBytes)
{
    for (const char *text :
        {"0", "7.5", "-7.5", "1.2345678901234567890123456789e-400", "-1.2345678901234567890123456789e-400",
            "1.2345678901234567890123456789e400", "-1.2345678901234567890123456789e400"})
    {
        SCOPED_TRACE(text);
        const BigFloat source{bf_arena(BigLifetime::PERSISTENT), text};
        const auto original = encoded_value(source);
        BigFloat legacy = BigFloat::from_legacy(original.data(), original.size());
        const LDouble expected{bf_to_float(legacy)};

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
    const std::size_t stride = (g_bf_length + 2 + 63) / 64 * 64;
    constexpr std::size_t ARENA_BYTES{1024 * 1024};
    EXPECT_EQ(stride, arena.block_size());
    EXPECT_EQ((ARENA_BYTES / stride) & ~std::size_t{1}, arena.capacity());
    BigFloatProduct product{arena};
    const std::vector<Byte> bytes = wide_float_fixture(old_length, negative, guard);
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigFloat canonical{arena, -7.5L};
    const Byte *const address = product.data();
    const std::size_t capacity = arena.capacity();

    init_bf_length(std::get<0>(GetParam()) == 56 ? 64 : 56);

    const std::vector<Byte> expected = wide_float_fixture(g_r_bf_length, negative, guard + g_r_bf_length - old_length);
    EXPECT_EQ(static_cast<std::size_t>((g_bf_length + 2 + 63) / 64 * 64), arena.block_size());
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
    const BigFloat persistent{bf_arena(BigLifetime::PERSISTENT), -2.5L};
    const BigFloat working{bf_arena(BigLifetime::WORKING), 3.75L};
    const std::vector<Byte> bytes = wide_float_pattern();
    BigFloatProduct product{arena};
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigStackMark mark = save_stack();
    {
        BigStackSaver saved;
        Byte *const fixed = bn_stack().allocate_double();
        Byte *const floating = bf_stack().allocate_double();
        BigNum{-5}.copy_to_legacy(fixed, g_bn_length);
        BigFloat{6.25L}.copy_to_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        const BigFloatProduct local{temporary, product};
        const BigStackMark middle = save_stack();
        {
            BigStackSaver inner;
            bn_stack().allocate_double();
            bf_stack().allocate_double();
            const BigFloatProduct nested{temporary, product};
            EXPECT_EQ(bytes, product_bytes(nested));
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_EQ(-5.0L, BigNum::from_legacy(fixed, g_bn_length).to_float());
        BigFloat observed_float = BigFloat::from_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(6.25L, bf_to_float(observed_float));
        EXPECT_EQ(bytes, product_bytes(local));
        EXPECT_EQ(bytes, product_bytes(product));
        EXPECT_EQ(-2.5L, persistent.to_float());
        EXPECT_EQ(3.75L, working.to_float());
    }
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bytes, product_bytes(product));
    EXPECT_EQ(-2.5L, persistent.to_float());
    EXPECT_EQ(3.75L, working.to_float());
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

namespace
{

enum class LegacyMultiplyCall
{
    SAFE,
    SAFE_LEFT,
    SAFE_RIGHT,
    UNSAFE
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

TEST_P(TestBigFloatLegacyMultiply, preservesCanonicalBoundsReturnIdentityOperandEffectsAndTemporaryLifetime)
{
    const LegacyMultiplyCall call = std::get<1>(GetParam());
    const LegacyMultiplyInput &input = std::get<2>(GetParam());
    const bool safe = call != LegacyMultiplyCall::UNSAFE;
    const bool nonzero = input.left != 0 && (input.shared ? input.left : input.right) != 0;
    BigStackSaver saved;
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    const std::vector<Byte> left_bytes = integer_float_bytes(g_bf_length, input.left, input.left_exponent);
    const std::vector<Byte> right_bytes = integer_float_bytes(g_bf_length, input.right, input.right_exponent);
    BigFloat left = BigFloat::from_legacy(persistent, left_bytes.data(), left_bytes.size());
    BigFloat right = BigFloat::from_legacy(persistent, right_bytes.data(), right_bytes.size());
    std::fill(left.data() + left_bytes.size(), left.data() + persistent.block_size(), Byte{0x5a});
    std::fill(right.data() + right_bytes.size(), right.data() + persistent.block_size(), Byte{0x6c});
    std::vector<Byte> expected_left(left.data(), left.data() + persistent.block_size());
    std::vector<Byte> expected_right(right.data(), right.data() + persistent.block_size());
    Byte *const left_source = left.data();
    Byte *const right_source = input.shared ? left.data() : right.data();
    BigFloat separate{persistent};
    std::fill_n(separate.data(), persistent.block_size(), Byte{0xa5});
    std::vector<Byte> expected_separate(persistent.block_size(), 0xa5);
    std::vector<Byte> output(g_r_bf_length + 4, 0xa5);
    std::vector<Byte> expected_output = output;
    BigFloat *destination = &separate;
    if (call == LegacyMultiplyCall::SAFE_LEFT)
    {
        destination = &left;
    }
    else if (call == LegacyMultiplyCall::SAFE_RIGHT)
    {
        destination = input.shared ? &left : &right;
    }
    Byte *const result = safe ? destination->data() : output.data() + 1;
    const std::vector<Byte> expected = integer_float_bytes(g_bf_length, input.product, input.product_exponent);
    if (!safe && nonzero)
    {
        const std::vector<Byte> absolute_left =
            integer_float_bytes(g_bf_length, std::abs(input.left), input.left_exponent);
        std::copy(absolute_left.begin(), absolute_left.end(), expected_left.begin());
        if (!input.shared)
        {
            const std::vector<Byte> absolute_right =
                integer_float_bytes(g_bf_length, std::abs(input.right), input.right_exponent);
            std::copy(absolute_right.begin(), absolute_right.end(), expected_right.begin());
        }
    }
    if (result == left.data())
    {
        std::copy(expected.begin(), expected.end(), expected_left.begin());
    }
    else if (result == right.data())
    {
        std::copy(expected.begin(), expected.end(), expected_right.begin());
    }
    else if (safe)
    {
        std::copy(expected.begin(), expected.end(), expected_separate.begin());
    }
    else
    {
        std::copy(expected.begin(), expected.end(), expected_output.begin() + 1);
    }
    BigFloat survivor{temporary, -7};
    std::fill(survivor.data() + left_bytes.size(), survivor.data() + temporary.block_size(), Byte{0x77});
    const std::vector<Byte> survivor_bytes(survivor.data(), survivor.data() + temporary.block_size());
    Byte *const borrowed = bf_stack().allocate_single();
    std::fill_n(borrowed, temporary.block_size(), Byte{0x69});
    Byte *const num_borrowed = bn_stack().allocate_single();
    const std::size_t num_stride = bn_arena(BigLifetime::TEMPORARY).block_size();
    std::fill_n(num_borrowed, num_stride, Byte{0x3c});
    BigArena::Allocation reservation = temporary.allocate_double();
    Byte *const product_address = reservation.data();
    std::fill_n(product_address, 2 * temporary.block_size(), Byte{0xa5});
    reservation.reset();
    const std::vector<Byte> free_bytes(product_address, product_address + 2 * temporary.block_size());
    const std::size_t high_water = temporary.high_water_mark();
    const BigStackMark mark = save_stack();

    if (safe)
    {
        BigFloat &returned = mult_bf(*destination, left, input.shared ? left : right);
        EXPECT_EQ(destination, &returned);
        EXPECT_EQ(result, returned.data());
    }
    else
    {
        EXPECT_EQ(result, unsafe_mult_bf(result, left_source, right_source));
    }
    EXPECT_EQ(expected, std::vector<Byte>(result, result + expected.size()));
    EXPECT_EQ(expected_separate, std::vector<Byte>(separate.data(), separate.data() + persistent.block_size()));
    EXPECT_EQ(expected_output, output);
    EXPECT_EQ(expected_left, std::vector<Byte>(left.data(), left.data() + persistent.block_size()));
    EXPECT_EQ(expected_right, std::vector<Byte>(right.data(), right.data() + persistent.block_size()));
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    EXPECT_EQ(survivor_bytes, std::vector<Byte>(survivor.data(), survivor.data() + temporary.block_size()));
    EXPECT_EQ(std::vector<Byte>(temporary.block_size(), 0x69),
        std::vector<Byte>(borrowed, borrowed + temporary.block_size()));
    EXPECT_EQ(std::vector<Byte>(num_stride, 0x3c), std::vector<Byte>(num_borrowed, num_borrowed + num_stride));
    BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(product_address, reused.data());
    EXPECT_EQ(2U, reused.blocks());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    if (!nonzero)
    {
        EXPECT_EQ(free_bytes, std::vector<Byte>(reused.data(), reused.data() + 2 * temporary.block_size()));
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsCallsAndInputs, TestBigFloatLegacyMultiply,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(LegacyMultiplyCall::SAFE, LegacyMultiplyCall::SAFE_LEFT, LegacyMultiplyCall::SAFE_RIGHT,
            LegacyMultiplyCall::UNSAFE),
        testing::Values(LegacyMultiplyInput{"Positive", 2, 3, 0, 0, 6, 0, false},
            LegacyMultiplyInput{"NegativeLeft", -2, 3, 0, 0, -6, 0, false},
            LegacyMultiplyInput{"NegativeRight", 2, -3, 0, 0, -6, 0, false},
            LegacyMultiplyInput{"NegativeBoth", -2, -3, 0, 0, 6, 0, false},
            LegacyMultiplyInput{"LargeExponents", 2, 3, 500, 700, 6, 1200, false},
            LegacyMultiplyInput{"SmallExponents", -2, 3, -500, -700, -6, -1200, false},
            LegacyMultiplyInput{"ZeroLeft", 0, 3, 0, 700, 0, 0, false},
            LegacyMultiplyInput{"ZeroRight", -2, 0, -500, 0, 0, 0, false},
            LegacyMultiplyInput{"Shared", -3, -3, 1000, 1000, 9, 2000, true})));

namespace
{

enum class LegacySqrCall
{
    SAFE,
    SAFE_IN_PLACE,
    CACHE_X,
    CACHE_Y,
    UNSAFE
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

TEST_P(TestBigFloatLegacySqr, preservesCanonicalBoundsCachesReturnIdentitySourceEffectsAndTemporaryLifetime)
{
    const LegacySqrCall call = std::get<1>(GetParam());
    const LegacySqrInput &input = std::get<2>(GetParam());
    BigStackSaver saved;
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    const std::size_t working_stride = bf_arena(BigLifetime::WORKING).block_size();
    const std::vector<Byte> source_bytes = integer_float_bytes(g_bf_length, input.source, input.exponent);
    BigFloat source = BigFloat::from_legacy(persistent, source_bytes.data(), source_bytes.size());
    std::fill(source.data() + source_bytes.size(), source.data() + persistent.block_size(), Byte{0x5a});
    std::vector<Byte> expected_source(source.data(), source.data() + persistent.block_size());
    std::fill_n(g_tmp_sqr_x_bf.data(), working_stride, Byte{0x69});
    std::fill_n(g_tmp_sqr_y_bf.data(), working_stride, Byte{0x96});
    std::vector<Byte> expected_x(working_stride, 0x69);
    std::vector<Byte> expected_y(working_stride, 0x96);
    BigFloat separate{persistent};
    std::fill_n(separate.data(), persistent.block_size(), Byte{0xa5});
    std::vector<Byte> expected_separate(persistent.block_size(), 0xa5);
    std::vector<Byte> output(g_r_bf_length + 4, 0xa5);
    std::vector<Byte> expected_output = output;
    BigFloat *destination = &separate;
    const std::vector<Byte> expected = integer_float_bytes(g_bf_length, input.squared, input.squared_exponent);
    if (call == LegacySqrCall::SAFE_IN_PLACE)
    {
        destination = &source;
        std::copy(expected.begin(), expected.end(), expected_source.begin());
    }
    else if (call == LegacySqrCall::CACHE_X)
    {
        destination = &g_tmp_sqr_x_bf;
        std::copy(expected.begin(), expected.end(), expected_x.begin());
    }
    else if (call == LegacySqrCall::CACHE_Y)
    {
        destination = &g_tmp_sqr_y_bf;
        std::copy(expected.begin(), expected.end(), expected_y.begin());
    }
    else if (call == LegacySqrCall::UNSAFE)
    {
        std::copy(expected.begin(), expected.end(), expected_output.begin() + 1);
        if (input.source != 0)
        {
            const std::vector<Byte> absolute_source =
                integer_float_bytes(g_bf_length, std::abs(input.source), input.exponent);
            std::copy(absolute_source.begin(), absolute_source.end(), expected_source.begin());
        }
    }
    else
    {
        std::copy(expected.begin(), expected.end(), expected_separate.begin());
    }
    Byte *const result = call == LegacySqrCall::UNSAFE ? output.data() + 1 : destination->data();
    BigFloat survivor{temporary, -7};
    std::fill(survivor.data() + source_bytes.size(), survivor.data() + temporary.block_size(), Byte{0x77});
    const std::vector<Byte> survivor_bytes(survivor.data(), survivor.data() + temporary.block_size());
    Byte *const borrowed = bf_stack().allocate_single();
    std::fill_n(borrowed, temporary.block_size(), Byte{0x3c});
    Byte *const num_borrowed = bn_stack().allocate_single();
    const std::size_t num_stride = bn_arena(BigLifetime::TEMPORARY).block_size();
    std::fill_n(num_borrowed, num_stride, Byte{0x6c});
    BigArena::Allocation reservation = temporary.allocate_double();
    Byte *const product_address = reservation.data();
    std::fill_n(product_address, 2 * temporary.block_size(), Byte{0xa5});
    reservation.reset();
    const std::vector<Byte> free_bytes(product_address, product_address + 2 * temporary.block_size());
    const std::size_t high_water = temporary.high_water_mark();
    const BigStackMark mark = save_stack();

    if (call == LegacySqrCall::UNSAFE)
    {
        EXPECT_EQ(result, unsafe_square_bf(result, source.data()));
    }
    else
    {
        BigFloat &returned = square_bf(*destination, std::as_const(source));
        EXPECT_EQ(destination, &returned);
        EXPECT_EQ(result, returned.data());
    }
    EXPECT_EQ(expected, std::vector<Byte>(result, result + expected.size()));
    EXPECT_EQ(expected_separate, std::vector<Byte>(separate.data(), separate.data() + persistent.block_size()));
    EXPECT_EQ(expected_output, output);
    EXPECT_EQ(expected_source, std::vector<Byte>(source.data(), source.data() + persistent.block_size()));
    EXPECT_EQ(expected_x, std::vector<Byte>(g_tmp_sqr_x_bf.data(), g_tmp_sqr_x_bf.data() + working_stride));
    EXPECT_EQ(expected_y, std::vector<Byte>(g_tmp_sqr_y_bf.data(), g_tmp_sqr_y_bf.data() + working_stride));
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    EXPECT_EQ(survivor_bytes, std::vector<Byte>(survivor.data(), survivor.data() + temporary.block_size()));
    EXPECT_EQ(std::vector<Byte>(temporary.block_size(), 0x3c),
        std::vector<Byte>(borrowed, borrowed + temporary.block_size()));
    EXPECT_EQ(std::vector<Byte>(num_stride, 0x6c), std::vector<Byte>(num_borrowed, num_borrowed + num_stride));
    BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(product_address, reused.data());
    EXPECT_EQ(2U, reused.blocks());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    if (input.source == 0)
    {
        EXPECT_EQ(free_bytes, std::vector<Byte>(reused.data(), reused.data() + 2 * temporary.block_size()));
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsCallsAndInputs, TestBigFloatLegacySqr,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(LegacySqrCall::SAFE, LegacySqrCall::SAFE_IN_PLACE, LegacySqrCall::CACHE_X,
            LegacySqrCall::CACHE_Y, LegacySqrCall::UNSAFE),
        testing::Values(LegacySqrInput{"Positive", 2, 0, 4, 0}, LegacySqrInput{"Negative", -3, 0, 9, 0},
            LegacySqrInput{"LargeExponent", -2, 500, 4, 1000}, LegacySqrInput{"SmallExponent", 3, -700, 9, -1400},
            LegacySqrInput{"Zero", 0, 0, 0, 0})));

} // namespace id::test
