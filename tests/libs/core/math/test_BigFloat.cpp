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
#include <memory>
#include <new>
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

static_assert(std::is_same_v<decltype(&bf_hex_dump), void (*)(const BigFloat &)>);
static_assert(std::is_same_v<decltype(std::declval<const BigFloat &>().to_float()), LDouble>);
static_assert(
    std::is_same_v<decltype(copy_bf(std::declval<BigFloat &>(), std::declval<const BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(clear_bf(std::declval<BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(max_bf(std::declval<BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(&div_bf), BigFloat &(*)(BigFloat &, const BigFloat &, const BigFloat &)>);
static_assert(std::is_same_v<decltype(unsafe_div_bf_int(std::declval<BigFloat &>(), std::declval<BigFloat &>(), U16{})),
    BigFloat &>);
static_assert(std::is_same_v<decltype(div_a_bf_int(std::declval<BigFloat &>(), U16{})), BigFloat &>);
static_assert(std::is_same_v<decltype(exp_bf(std::declval<BigFloat &>(), std::declval<const BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(ln_bf(std::declval<BigFloat &>(), std::declval<const BigFloat &>())), BigFloat &>);
static_assert(std::is_same_v<decltype(sin_cos_bf(std::declval<BigFloat &>(), std::declval<BigFloat &>(),
                                 std::declval<const BigFloat &>())),
    BigFloat &>);
static_assert(std::is_same_v<decltype(atan2_bf(std::declval<BigFloat &>(), std::declval<const BigFloat &>(),
                                 std::declval<const BigFloat &>())),
    BigFloat &>);
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

TEST_P(TestBigFloatSign, comparesWithZeroWithoutChangingValueOrStorage)
{
    init_bf_length(std::get<0>(GetParam()));
    const int sign = std::get<1>(GetParam());
    const BigFloat value{static_cast<LDouble>(sign) / 2};
    const BigFloat original{value};
    const BigFloat zero{0};
    const BigArena &arena = value.arena();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(sign < 0, value < zero);
    EXPECT_EQ(sign == 0, value == zero);
    EXPECT_EQ(sign > 0, value > zero);
    EXPECT_EQ(sign != 0, value != zero);
    EXPECT_EQ(original, value);
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
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
    const BigArena &arena = left.arena();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_LT(left, right);
    EXPECT_GT(right, left);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigFloatComparison,
    testing::Combine(testing::Values(4, 64, 708),
        testing::Values(BigFloatComparisonInput{-1.25L, 0.0L}, BigFloatComparisonInput{0.5L, 1.25L},
            BigFloatComparisonInput{-512.0L, -1.5L}, BigFloatComparisonInput{1.25L, 2.25L},
            BigFloatComparisonInput{-1.5L, -1.25L})));

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
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat expected{input.expected};
    g_bf_tmp2 = -1.25L;
    const BigFloat untouched{g_bf_tmp2};
    arena.allocate_single().reset();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    const BigFloat value{arena, input.text};

    EXPECT_EQ(expected, value);
    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(untouched, g_bf_tmp2);
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
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

TEST_P(TestBigFloatScalarConstruction, preservesTheNativeValueAndArena)
{
    const auto [length, input] = GetParam();
    init_bf_length(length);
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    arena.allocate_single().reset();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    const BigFloat value{arena, input};

    EXPECT_EQ(input, value.to_float());
    EXPECT_EQ(&arena, &value.arena());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
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

TEST_P(TestBigFloatOutputInterface, observesConstValuesUsingExistingCopyStorage)
{
    const auto [length, input] = GetParam();
    init_bf_length(length);
    const BigFloat value{bf_arena(BigLifetime::PERSISTENT), input.text};
    const BigFloat original{value};
    const BigArena &working = bf_arena(BigLifetime::WORKING);
    const BigArena::Index copy_index = g_bf_tmp_copy1.index();
    const std::size_t size = working.value_size();
    const std::size_t copy_size = 2 * working.block_size();
    std::fill_n(g_bf_tmp_copy1.data(), copy_size, Byte{0x5a});
    std::fill_n(g_bf_tmp_copy2.data(), copy_size, Byte{0x3c});
    const std::size_t working_high_water = working.high_water_mark();
    const std::size_t persistent_high_water = value.arena().high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;

    const LDouble actual = value.to_float();

    EXPECT_EQ(input.expected, actual);
    EXPECT_EQ(std::signbit(input.expected), std::signbit(actual));
    EXPECT_EQ(original, value);
    EXPECT_EQ(copy_index, g_bf_tmp_copy1.index());
    EXPECT_EQ(std::vector<Byte>(copy_size - size, 0x5a),
        std::vector<Byte>(g_bf_tmp_copy1.data() + size, g_bf_tmp_copy1.data() + copy_size));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x3c),
        std::vector<Byte>(g_bf_tmp_copy2.data(), g_bf_tmp_copy2.data() + copy_size));
    EXPECT_EQ(working_high_water, working.high_water_mark());
    EXPECT_EQ(persistent_high_water, value.arena().high_water_mark());
    EXPECT_EQ(mark, save_stack());
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

TEST_F(TestBigFloatValue, copiesValuesBelowMachineRangeBetweenArenas)
{
    constexpr std::string_view TEXT{"-1.23456789012345678901234567890123456789e-10000"};
    BigFloat source{bf_arena(BigLifetime::TEMPORARY), TEXT};
    BigArena &working = bf_arena(BigLifetime::WORKING);
    const BigFloat value{working, source};

    source = 0;

    EXPECT_LT(value, BigFloat{0});
    EXPECT_EQ(0.0L, value.to_float());
    EXPECT_EQ(BigFloat{TEXT}, value);
    EXPECT_EQ(&working, &value.arena());
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
    constexpr std::string_view TEXT{"1.23456789012345678901234567890123456789"};
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    const BigStackMark marker = save_stack();
    Byte *const borrowed = bf_stack().allocate_single();
    const BigFloat value{temporary, TEXT};

    restore_stack(marker);
    Byte *const reused = bf_stack().allocate_single();
    ASSERT_EQ(borrowed, reused);
    std::fill_n(reused, temporary.block_size(), Byte{0xff});

    EXPECT_EQ(BigFloat{TEXT}, value);
    EXPECT_EQ(&temporary, &value.arena());
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

TEST_F(TestBigFloatValue, copyAssignmentUsesTheCurrentArenaLayout)
{
    init_bf_dec(80);
    const BigFloat original{"1.23456789012345678901234567890123456789"};
    const BigFloat expected{original};
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    BigFloat assigned{persistent, 2.5L};
    const std::size_t high_water = persistent.high_water_mark();

    assigned = original;

    EXPECT_EQ(expected, assigned);
    EXPECT_EQ(expected, original);
    EXPECT_EQ(&persistent, &assigned.arena());
    EXPECT_EQ(high_water, persistent.high_water_mark());
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

class TestBigFloatDoubling : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatDoubling, preservesSourceDestinationIdentityAndPadding)
{
    init_bf_length(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, 128.0L};
    BigFloat result{arena};
    const Byte *const address = result.data();
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill_n(result.data(), stride, Byte{0xa5});
    const std::vector<Byte> original = encoded_value(source);
    const std::array<Byte, 5> tail{0, 1, 0, 1, 0};
    std::vector<Byte> expected(stride, 0xa5);
    std::fill_n(expected.begin(), size, Byte{0});
    std::copy(tail.begin(), tail.end(), expected.begin() + size - tail.size());
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &double_bf(result, source));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &double_bf(result, std::as_const(result)));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &double_a_bf(result));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatDoubling, testing::Values(4, 64, 708));

class TestBigFloatHalving : public TestBigFloatValue, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatHalving, preservesSourceDestinationIdentityAndPadding)
{
    init_bf_length(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, 1.0L};
    BigFloat result{arena};
    const Byte *const address = result.data();
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill_n(result.data(), stride, Byte{0xa5});
    const std::vector<Byte> original = encoded_value(source);
    const std::array<Byte, 5> tail{0, 0x80, 0, 0xff, 0xff};
    std::vector<Byte> expected(stride, 0xa5);
    std::fill_n(expected.begin(), size, Byte{0});
    std::copy(tail.begin(), tail.end(), expected.begin() + size - tail.size());
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &half_bf(result, source));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &half_bf(result, std::as_const(result)));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    copy_bf(result, source);
    EXPECT_EQ(&result, &half_a_bf(result));
    EXPECT_EQ(expected, std::vector<Byte>(result.data(), result.data() + stride));

    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatHalving, testing::Values(4, 64, 708));

class TestBigFloatReciprocal : public TestBigFloatValue, public testing::WithParamInterface<LDouble>
{
};

TEST_P(TestBigFloatReciprocal, returnsExistingDestinationAndMakesSourcePositive)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat source{arena, GetParam()};
    BigFloat result{arena};
    const BigFloat expected_source{arena, 2.0L};
    const BigFloat expected_result{arena, GetParam() < 0 ? -0.5L : 0.5L};
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill(result.data() + size, result.data() + stride, Byte{0xa5});
    const Byte *const address = result.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &unsafe_inv_bf(result, source));

    EXPECT_EQ(encoded_value(expected_result), encoded_value(result));
    EXPECT_EQ(encoded_value(expected_source), encoded_value(source));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(result.data() + size, result.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Signs, TestBigFloatReciprocal, testing::Values(-2.0L, 2.0L));

TEST_F(TestBigFloatValue, reciprocalOfZeroSaturatesInPlace)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat source{arena};
    BigFloat result{arena, 1.0L};
    constexpr int exponent = std::numeric_limits<LDouble>::max_exponent / 8;
    const int size = g_bf_length + 2;
    std::vector<Byte> expected(size);
    expected[size - 4] = 1;
    expected[size - 2] = static_cast<Byte>(exponent);
    expected[size - 1] = static_cast<Byte>(exponent >> 8);

    EXPECT_EQ(&result, &unsafe_inv_bf(result, source));

    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ(std::vector<Byte>(size), encoded_value(source));
}

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

TEST_P(TestBigFloatDivision, writesAndReturnsTheDestinationWithAliasedInputs)
{
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat numerator{arena, -8.0L};
    BigFloat denominator{arena, -4.0L};
    BigFloat result{arena};
    const DivisionDestination target = GetParam();
    const bool shared_input = target == DivisionDestination::SHARED_INPUT;
    const BigFloat &divisor = shared_input ? numerator : denominator;
    BigFloat &numerator_or_result = target == DivisionDestination::SEPARATE ? result : numerator;
    BigFloat &output = target == DivisionDestination::DENOMINATOR ? denominator : numerator_or_result;
    const BigFloat expected{arena, shared_input ? 1.0L : 2.0L};
    const std::vector<Byte> original_numerator = encoded_value(numerator);
    const std::vector<Byte> original_denominator = encoded_value(denominator);
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill(output.data() + size, output.data() + stride, Byte{0xa5});
    const Byte *const address = output.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&output, &div_bf(output, std::as_const(numerator), divisor));

    EXPECT_EQ(encoded_value(expected), encoded_value(output));
    EXPECT_EQ(address, output.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(output.data() + size, output.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    if (&output != &numerator)
    {
        EXPECT_EQ(original_numerator, encoded_value(numerator));
    }
    if (&output != &denominator)
    {
        EXPECT_EQ(original_denominator, encoded_value(denominator));
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

TEST_P(TestBigFloatExponential, preservesSourceAndReturnsTheExistingDestination)
{
    const BigFloatExponentialInput &test = GetParam();
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, test.input};
    BigFloat result{arena};
    const std::vector<Byte> original = encoded_value(source);
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill(result.data() + size, result.data() + stride, Byte{0xa5});
    const Byte *const address = result.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &exp_bf(result, source));

    EXPECT_NEAR(test.expected, result.to_float(), test.tolerance);
    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(result.data() + size, result.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatExponential,
    testing::Values(BigFloatExponentialInput{0.0L, 1.0L, 0.0L},
        BigFloatExponentialInput{0.375L, 1.4549914146182013L, 1e-12L},
        BigFloatExponentialInput{-0.625L, 0.5352614285189903L, 1e-12L}));

class TestBigFloatLogarithm : public TestBigFloatValue, public testing::WithParamInterface<std::tuple<int, bool>>
{
};

TEST_P(TestBigFloatLogarithm, preservesDestinationStorageAndHandlesAliasedSources)
{
    init_bf_length(64);
    const int input = std::get<0>(GetParam());
    const bool alias = std::get<1>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    BigFloat source{arena, static_cast<LDouble>(input)};
    BigFloat result{arena};
    BigFloat &output = alias ? source : result;
    const std::vector<Byte> original = encoded_value(source);
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::vector<Byte> expected(size);
    if (input <= 0)
    {
        constexpr int exponent = std::numeric_limits<LDouble>::max_exponent / 8 - 1;
        expected[size - 3] = 0xff;
        expected[size - 2] = static_cast<Byte>(exponent);
        expected[size - 1] = static_cast<Byte>(exponent >> 8);
    }
    std::fill(output.data() + size, output.data() + stride, Byte{0xa5});
    const Byte *const address = output.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&output, &ln_bf(output, std::as_const(source)));

    EXPECT_EQ(expected, encoded_value(output));
    EXPECT_EQ(alias ? expected : original, encoded_value(source));
    EXPECT_EQ(address, output.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(output.data() + size, output.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(
    InputsAndAliases, TestBigFloatLogarithm, testing::Combine(testing::Values(1, 0, -1), testing::Bool()));

class TestBigFloatSineCosine : public TestBigFloatValue, public testing::WithParamInterface<LDouble>
{
};

TEST_P(TestBigFloatSineCosine, writesBothDestinationsAndPreservesSourceAndStorage)
{
    init_big_pi();
    const LDouble angle = GetParam();
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat source{arena, angle};
    BigFloat sine{arena};
    BigFloat cosine{arena};
    const std::vector<Byte> original = encoded_value(source);
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    const std::vector<Byte> guard(stride - size, 0xa5);
    std::fill(sine.data() + size, sine.data() + stride, Byte{0xa5});
    std::fill(cosine.data() + size, cosine.data() + stride, Byte{0xa5});
    const Byte *const sine_address = sine.data();
    const Byte *const cosine_address = cosine.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&sine, &sin_cos_bf(sine, cosine, source));

    EXPECT_NEAR(std::sin(angle), sine.to_float(), 1e-12L);
    EXPECT_NEAR(std::cos(angle), cosine.to_float(), 1e-12L);
    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(sine_address, sine.data());
    EXPECT_EQ(cosine_address, cosine.data());
    EXPECT_EQ(guard, std::vector<Byte>(sine.data() + size, sine.data() + stride));
    EXPECT_EQ(guard, std::vector<Byte>(cosine.data() + size, cosine.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Angles, TestBigFloatSineCosine, testing::Values(0.0L, 0.375L, -0.625L));

class TestBigFloatSineCosineAlias : public TestBigFloatValue, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigFloatSineCosineAlias, copiesTheSourceBeforeWritingEitherDestination)
{
    init_big_pi();
    constexpr LDouble angle = -0.625L;
    BigFloat source{angle};
    BigFloat other{0};
    BigFloat &sine = GetParam() ? source : other;
    BigFloat &cosine = GetParam() ? other : source;

    EXPECT_EQ(&sine, &sin_cos_bf(sine, cosine, source));

    EXPECT_NEAR(std::sin(angle), sine.to_float(), 1e-12L);
    EXPECT_NEAR(std::cos(angle), cosine.to_float(), 1e-12L);
}

INSTANTIATE_TEST_SUITE_P(Destinations, TestBigFloatSineCosineAlias, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "Sine" : "Cosine"; });

class TestBigFloatArctangent2 : public TestBigFloatValue,
                                public testing::WithParamInterface<std::tuple<LDouble, LDouble>>
{
};

TEST_P(TestBigFloatArctangent2, writesTheDestinationAndPreservesOperandsAndStorage)
{
    init_big_pi();
    const LDouble y = std::get<0>(GetParam());
    const LDouble x = std::get<1>(GetParam());
    BigArena &arena = bf_arena(BigLifetime::WORKING);
    const BigFloat ny{arena, y};
    const BigFloat nx{arena, x};
    BigFloat result{arena};
    const std::vector<Byte> original_y = encoded_value(ny);
    const std::vector<Byte> original_x = encoded_value(nx);
    const std::size_t size = arena.value_size();
    const std::size_t stride = arena.block_size();
    std::fill(result.data() + size, result.data() + stride, Byte{0xa5});
    const Byte *const address = result.data();
    const std::size_t high_water = arena.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&result, &atan2_bf(result, ny, nx));

    EXPECT_NEAR(std::atan2(y, x), result.to_float(), 1e-11L);
    EXPECT_EQ(original_y, encoded_value(ny));
    EXPECT_EQ(original_x, encoded_value(nx));
    EXPECT_EQ(address, result.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(result.data() + size, result.data() + stride));
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(QuadrantsAndAxes, TestBigFloatArctangent2,
    testing::Values(std::make_tuple(1.0L, -2.0L), std::make_tuple(-1.0L, 0.0L), std::make_tuple(2.0L, 1.0L)));

class TestBigFloatArctangent2Alias : public TestBigFloatValue, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigFloatArctangent2Alias, copiesBothOperandsBeforeWritingAnAliasedDestination)
{
    init_big_pi();
    BigFloat ny{-1.0L};
    BigFloat nx{-2.0L};
    BigFloat &result = GetParam() ? ny : nx;
    const BigFloat &other = GetParam() ? nx : ny;
    const std::vector<Byte> original = encoded_value(other);

    EXPECT_EQ(&result, &atan2_bf(result, std::as_const(ny), std::as_const(nx)));

    EXPECT_NEAR(std::atan2(-1.0L, -2.0L), result.to_float(), 1e-11L);
    EXPECT_EQ(original, encoded_value(other));
}

INSTANTIATE_TEST_SUITE_P(Destinations, TestBigFloatArctangent2Alias, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "Y" : "X"; });

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
    div_bf(result, left, right);
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
    BigFloat expected{bf_arena(BigLifetime::WORKING)};
    add_bf(expected, left, right);
    div_bf(expected, expected, divisor);
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
    BigFloat maximum{bf_arena(BigLifetime::WORKING)};
    max_bf(maximum);
    BigFloat result{bf_arena(BigLifetime::WORKING)};
    div_bf(result, value, zero);

    EXPECT_EQ(encoded_value(value), encoded_value(value + zero));
    EXPECT_EQ(encoded_value(value), encoded_value(value - zero));
    EXPECT_EQ(encoded_value(-value), encoded_value(zero - value));
    EXPECT_EQ(encoded_value(zero), encoded_value(value * zero));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / value));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / zero));
    EXPECT_EQ(encoded_value(maximum), encoded_value(result));
    EXPECT_EQ(encoded_value(maximum), encoded_value(value / zero));
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

class TestBigFloatScalarDivision : public TestBigFloatValue,
                                   public testing::WithParamInterface<std::tuple<int, const char *, U16>>
{
};

TEST_P(TestBigFloatScalarDivision, assignmentMatchesInPlaceDivision)
{
    init_bf_length(std::get<0>(GetParam()));
    const U16 divisor = std::get<2>(GetParam());
    BigArena &persistent = bf_arena(BigLifetime::PERSISTENT);
    const BigFloat source{persistent, std::get<1>(GetParam())};
    const std::vector<Byte> original = encoded_value(source);
    BigFloat expected{source};
    BigFloat value{source};
    const Byte *const address = value.data();
    const std::size_t high_water = persistent.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&expected, &div_a_bf_int(expected, divisor));
    EXPECT_EQ(&value, &(value /= divisor));

    EXPECT_EQ(encoded_value(expected), encoded_value(value));
    EXPECT_EQ(original, encoded_value(source));
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(&persistent, &value.arena());
    EXPECT_EQ(high_water, persistent.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatScalarDivision,
    testing::Combine(testing::Values(4, 32, 708),
        testing::Values("0", "-1.2345678901234567890123456789e-400", "1.2345678901234567890123456789e400"),
        testing::Values(U16{0}, U16{1}, U16{2}, U16{255}, U16{256}, U16{32768}, U16{65535})));

class TestBigFloatScalarDivisionDestination : public TestBigFloatValue, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigFloatScalarDivisionDestination, reusesTheDestinationAndRetainsSourceMutation)
{
    BigArena &working = bf_arena(BigLifetime::WORKING);
    BigFloat source{working, -6.0L};
    BigFloat result{working};
    BigFloat &output = GetParam() ? source : result;
    const BigFloat quotient{working, -3.0L};
    const BigFloat magnitude{working, 6.0L};
    const std::size_t size = working.value_size();
    const std::size_t stride = working.block_size();
    std::fill(output.data() + size, output.data() + stride, Byte{0xa5});
    const Byte *const address = output.data();
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();

    EXPECT_EQ(&output, &unsafe_div_bf_int(output, source, 2));

    EXPECT_EQ(encoded_value(quotient), encoded_value(output));
    EXPECT_EQ(encoded_value(GetParam() ? quotient : magnitude), encoded_value(source));
    EXPECT_EQ(address, output.data());
    EXPECT_EQ(std::vector<Byte>(stride - size, 0xa5), std::vector<Byte>(output.data() + size, output.data() + stride));
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Aliases, TestBigFloatScalarDivisionDestination, testing::Bool());

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

class TestBigFloatPairStorage : public TestBigFloatValue,
                                public testing::WithParamInterface<std::tuple<int, BigLifetime>>
{
protected:
    void SetUp() override
    {
        init_bf_length(std::get<0>(GetParam()));
    }
};

TEST_P(TestBigFloatPairStorage, reductionReleasesTheCompanionInEachLifetime)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    const std::size_t stride = (g_bf_length + 2 + 63) / 64 * 64;
    constexpr std::size_t ARENA_BYTES{1024 * 1024};
    EXPECT_EQ(stride, arena.block_size());
    EXPECT_EQ((ARENA_BYTES / stride) & ~std::size_t{1}, arena.capacity());
    BigArena::Allocation pair = arena.allocate_double();
    std::fill_n(pair.data(), 2 * stride, Byte{0});
    const BigFloat canonical{arena, -7.5L};
    const Byte *const address = pair.data();
    EXPECT_EQ(2U, pair.blocks());

    pair.reduce();

    EXPECT_EQ(address, pair.data());
    EXPECT_EQ(1U, pair.blocks());
    const BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(address + arena.block_size(), companion.data());
    EXPECT_EQ(BigFloat{-7.5L}, canonical);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLifetimes, TestBigFloatPairStorage,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

namespace
{

std::vector<Byte> paired_bytes(const BigArena::Allocation &allocation)
{
    return {allocation.data(), allocation.data() + allocation.blocks() * allocation.arena().block_size()};
}

} // namespace

class TestBigFloatProductLifetime : public TestBigFloatValue,
                                    public testing::WithParamInterface<std::tuple<int, BigLifetime>>
{
protected:
    void SetUp() override;
};

void TestBigFloatProductLifetime::SetUp()
{
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigFloatProductLifetime, temporaryRewindsPreservePairedAllocationsAndReleaseLocalOwners)
{
    BigArena &arena = bf_arena(std::get<1>(GetParam()));
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    const BigFloat persistent{bf_arena(BigLifetime::PERSISTENT), -2.5L};
    const BigFloat working{bf_arena(BigLifetime::WORKING), 3.75L};
    const std::vector<Byte> bytes(2 * arena.block_size(), 0x5a);
    BigArena::Allocation product = arena.allocate_double();
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigStackMark mark = save_stack();
    {
        BigStackSaver saved;
        const std::vector<Byte> fixed_sentinel(2 * bn_arena(BigLifetime::TEMPORARY).block_size(), 0xa5);
        Byte *const fixed = bn_stack().allocate_double();
        Byte *const floating = bf_stack().allocate_double();
        std::copy(fixed_sentinel.begin(), fixed_sentinel.end(), fixed);
        BigFloat{6.25L}.copy_to_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        const BigArena::Allocation local = product.clone(temporary);
        const BigStackMark middle = save_stack();
        {
            BigStackSaver inner;
            bn_stack().allocate_double();
            bf_stack().allocate_double();
            const BigArena::Allocation nested = product.clone(temporary);
            EXPECT_EQ(bytes, paired_bytes(nested));
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_TRUE(std::equal(fixed_sentinel.begin(), fixed_sentinel.end(), fixed));
        BigFloat observed_float = BigFloat::from_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(6.25L, bf_to_float(observed_float));
        EXPECT_EQ(bytes, paired_bytes(local));
        EXPECT_EQ(bytes, paired_bytes(product));
        EXPECT_EQ(-2.5L, persistent.to_float());
        EXPECT_EQ(3.75L, working.to_float());
    }
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bytes, paired_bytes(product));
    EXPECT_EQ(-2.5L, persistent.to_float());
    EXPECT_EQ(3.75L, working.to_float());
    const std::size_t high_water = temporary.high_water_mark();
    BigArena::Allocation first = temporary.allocate_double();
    BigArena::Allocation second = temporary.allocate_double();
    EXPECT_EQ(high_water, temporary.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLifetimes, TestBigFloatProductLifetime,
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
    BigArena::Allocation reservation = working.allocate_double();
    Byte *const address = reservation.data();
    std::fill_n(address, 2 * working.block_size(), Byte{0xa5});
    reservation.reset();
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark mark = save_stack();
    const int expected_value = squared ? input.squared : input.product;
    const int expected_exponent = squared ? input.squared_exponent : input.product_exponent;
    const std::vector<Byte> expected = integer_float_bytes(g_bf_length, expected_value, expected_exponent);
    {
        const BigFloat result = squared ? left.sqr() : left * right;

        EXPECT_EQ(address, result.data());
        EXPECT_EQ(&working, &result.arena());
        EXPECT_EQ(high_water, working.high_water_mark());
        EXPECT_EQ(mark, save_stack());
        EXPECT_EQ(expected, encoded_value(result));
        EXPECT_EQ(original_left, std::vector<Byte>(left.data(), left.data() + persistent.block_size()));
        EXPECT_EQ(
            original_right, std::vector<Byte>(distinct_right.data(), distinct_right.data() + persistent.block_size()));
        EXPECT_EQ(std::vector<Byte>(working.block_size(), 0x3c),
            std::vector<Byte>(guard.data(), guard.data() + working.block_size()));
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

TEST_P(TestBigFloatLegacyMultiply, preservesCanonicalBoundsReturnIdentityOperandEffectsAndTemporaryLifetime)
{
    const LegacyMultiplyCall call = std::get<1>(GetParam());
    const LegacyMultiplyInput &input = std::get<2>(GetParam());
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
    BigFloat separate{persistent};
    std::fill_n(separate.data(), persistent.block_size(), Byte{0xa5});
    std::vector<Byte> expected_separate(persistent.block_size(), 0xa5);
    BigFloat *destination = &separate;
    if (call == LegacyMultiplyCall::SAFE_LEFT)
    {
        destination = &left;
    }
    else if (call == LegacyMultiplyCall::SAFE_RIGHT)
    {
        destination = input.shared ? &left : &right;
    }
    const Byte *const result = destination->data();
    const std::vector<Byte> expected = integer_float_bytes(g_bf_length, input.product, input.product_exponent);
    if (result == left.data())
    {
        std::copy(expected.begin(), expected.end(), expected_left.begin());
    }
    else if (result == right.data())
    {
        std::copy(expected.begin(), expected.end(), expected_right.begin());
    }
    else
    {
        std::copy(expected.begin(), expected.end(), expected_separate.begin());
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

    BigFloat &returned = mult_bf(*destination, left, input.shared ? left : right);
    EXPECT_EQ(destination, &returned);
    EXPECT_EQ(result, returned.data());
    EXPECT_EQ(expected, std::vector<Byte>(result, result + expected.size()));
    EXPECT_EQ(expected_separate, std::vector<Byte>(separate.data(), separate.data() + persistent.block_size()));
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
        testing::Values(LegacyMultiplyCall::SAFE, LegacyMultiplyCall::SAFE_LEFT, LegacyMultiplyCall::SAFE_RIGHT),
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
    else
    {
        std::copy(expected.begin(), expected.end(), expected_separate.begin());
    }
    const Byte *const result = destination->data();
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

    BigFloat &returned = square_bf(*destination, std::as_const(source));
    EXPECT_EQ(destination, &returned);
    EXPECT_EQ(result, returned.data());
    EXPECT_EQ(expected, std::vector<Byte>(result, result + expected.size()));
    EXPECT_EQ(expected_separate, std::vector<Byte>(separate.data(), separate.data() + persistent.block_size()));
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
            LegacySqrCall::CACHE_Y),
        testing::Values(LegacySqrInput{"Positive", 2, 0, 4, 0}, LegacySqrInput{"Negative", -3, 0, 9, 0},
            LegacySqrInput{"LargeExponent", -2, 500, 4, 1000}, LegacySqrInput{"SmallExponent", 3, -700, 9, -1400},
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

TEST_P(TestBigFloatWorkspaceRamp, preservesOperandsAndRestoresBorrowedWorkspaceWindows)
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    init_big_pi();
    const auto &test = std::get<1>(GetParam());
    BigFloat input{test.input};
    BigFloat result{0};
    BigFloat one{1};
    const std::vector<Byte> original(input.data(), input.data() + g_bf_length + 2);
    auto *const tmp3 = g_bf_tmp3.data();
    auto *const tmp5 = g_bf_tmp5.data();
    auto *const pi = g_bf_pi.data();
    const std::vector<Byte> original_pi(pi, pi + g_bf_length + 2);
    const int length = g_bf_length;
    const int bn_length = g_bn_length;
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const auto stack = save_stack();

    if (test.logarithm)
    {
        ln_bf(result, input);
    }
    else
    {
        atan2_bf(result, input, one);
    }

    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(tmp3, g_bf_tmp3.data());
    EXPECT_EQ(tmp5, g_bf_tmp5.data());
    EXPECT_EQ(pi, g_bf_pi.data());
    EXPECT_EQ(original_pi, (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + g_bf_length + 2}));
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(original, (std::vector<Byte>{input.data(), input.data() + g_bf_length + 2}));
    EXPECT_EQ(test.expected, bf_to_string(result, 12));
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
    const BigStackMark marker = save_stack();
    {
        const BigFloat value{bf_arena(BigLifetime::TEMPORARY), text};
        EXPECT_EQ(GetParam().expected, value.to_float());
    }
    EXPECT_EQ(marker, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatParse,
    testing::Values(BigFloatParseInput{"PlainDecimal", "1.25", 4, 1.25L},
        BigFloatParseInput{"UnterminatedView", "1.25x", 4, 1.25L}, BigFloatParseInput{"Exponent", "1.25e2", 6, 125.0L}),
    [](const testing::TestParamInfo<BigFloatParseInput> &info) { return info.param.name; });

class TestBigFloatFormat : public TestBigFloat
{
protected:
    void TearDown() override;
    void prepare_value(const char *text);

    BigFloat m_value{};
};

void TestBigFloatFormat::prepare_value(const char *text)
{
    BigFloat value{bf_arena(BigLifetime::TEMPORARY)};
    std::destroy_at(&m_value);
    ::new (&m_value) BigFloat{std::move(value)};
    if (text)
    {
        str_to_bf(m_value, text);
    }
    else
    {
        clear_bf(m_value);
    }
}

void TestBigFloatFormat::TearDown()
{
    m_value = BigFloat{};
    TestBigFloat::TearDown();
}

TEST_F(TestBigFloatFormat, generalFormatHandlesZero)
{
    prepare_value(nullptr);

    EXPECT_EQ("0.0", bf_to_string(m_value, 5));
}

TEST_F(TestBigFloatFormat, resetReleasesItsTemporarySlot)
{
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    BigArena::Allocation probe = temporary.allocate_single();
    const BigArena::Index index = probe.index();
    probe.reset();
    const BigStackMark marker = save_stack();

    prepare_value("1.25");
    EXPECT_EQ(&temporary, &m_value.arena());
    EXPECT_EQ("1.25", bf_to_string(m_value, 5));
    EXPECT_EQ(marker, save_stack());

    m_value = BigFloat{};
    ASSERT_FALSE(m_value);

    BigArena::Allocation reused = temporary.allocate_single();
    EXPECT_EQ(index, reused.index());
    EXPECT_EQ(marker, save_stack());
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
protected:
    void SetUp() override;
};

void TestBigFloatGeneralFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatGeneralFormat, selectsNotationAndFormatsDecimal)
{
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string(input, GetParam().decimals));
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

class TestBigFloatFixedFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
protected:
    void SetUp() override;
};

void TestBigFloatFixedFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatFixedFormat, formatsFixedDecimal)
{
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string_f(input, GetParam().decimals));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatFixedFormat,
    testing::Values(BigFloatFormatInput{"Zero", nullptr, 5, "0.0"},
        BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25"}, BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234"}, BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999"},
        BigFloatFormatInput{"SmallExponent", "0.000125", 8, "0.000125"},
        BigFloatFormatInput{"LargeExponent", "1250000", 8, "1250000.0"},
        BigFloatFormatInput{"FewDigitsLargeExponent", "1250000", 3, "1250000.0"},
        BigFloatFormatInput{"SingleDigit", "1.25", 1, "1.0"},
        BigFloatFormatInput{"DefaultPrecision", "1.234567890123456789", 0, "1.234567890123456789"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

class TestBigFloatScientificFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
protected:
    void SetUp() override;
};

void TestBigFloatScientificFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatScientificFormat, formatsScientificDecimal)
{
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string_e(input, GetParam().decimals));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatScientificFormat,
    testing::Values(BigFloatFormatInput{"Zero", nullptr, 5, "0.0"},
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
    SCIENTIFIC,
    FIXED
};

class TestBigFloatFormatterStorage : public TestBigFloatFormat,
                                     public testing::WithParamInterface<std::tuple<int, BigFloatFormatMode, bool>>
{
};

TEST_P(TestBigFloatFormatterStorage, preservesInputScratchBoundsSharedLengthsAndExistingReservations)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatFormatMode mode = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    prepare_value(negative ? "-1.25" : "1.25");
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::size_t stride = m_value.arena().block_size();
    const std::size_t copy_size = 2 * bf_arena(BigLifetime::WORKING).block_size();
    std::fill(m_value.data() + size, m_value.data() + stride, Byte{0xa5});
    const std::vector<Byte> original(m_value.data(), m_value.data() + stride);
    std::fill_n(g_bf_tmp_copy1.data(), copy_size, Byte{0x5a});
    std::fill_n(g_bf_tmp_copy2.data(), copy_size, Byte{0x3c});
    const Byte *const input = m_value.data();
    const Byte *const scratch = g_bf_tmp_copy1.data();
    const std::size_t working_high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const std::size_t temporary_high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;
    std::string expected{negative ? "-1.25" : "1.25"};
    std::string actual;

    switch (mode)
    {
    case BigFloatFormatMode::GENERAL:
        actual = bf_to_string(m_value, 12);
        break;
    case BigFloatFormatMode::SCIENTIFIC:
        expected += "e0";
        actual = bf_to_string_e(m_value, 12);
        break;
    case BigFloatFormatMode::FIXED:
        actual = bf_to_string_f(m_value, 12);
        break;
    }

    EXPECT_EQ(expected, actual);
    EXPECT_EQ(original, std::vector<Byte>(m_value.data(), m_value.data() + stride));
    EXPECT_EQ(std::vector<Byte>(size, 0), std::vector<Byte>(scratch, scratch + size));
    EXPECT_EQ(std::vector<Byte>(copy_size - size, 0x5a), std::vector<Byte>(scratch + size, scratch + copy_size));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x3c),
        std::vector<Byte>(g_bf_tmp_copy2.data(), g_bf_tmp_copy2.data() + copy_size));
    EXPECT_EQ(input, m_value.data());
    EXPECT_EQ(scratch, g_bf_tmp_copy1.data());
    EXPECT_EQ(working_high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(temporary_high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsModesAndSigns, TestBigFloatFormatterStorage,
    testing::Combine(testing::Values(4, 12, 64, 708),
        testing::Values(BigFloatFormatMode::GENERAL, BigFloatFormatMode::SCIENTIFIC, BigFloatFormatMode::FIXED),
        testing::Bool()));

class TestBigFloatFormatterUnderflow : public TestBigFloatFormat,
                                       public testing::WithParamInterface<decltype(&bf_to_string)>
{
};

TEST_P(TestBigFloatFormatterUnderflow, formatsNonzeroValueBelowNativeRangeAsZero)
{
    prepare_value("1e-5000");

    ASSERT_TRUE(is_bf_not_zero(m_value));
    EXPECT_EQ("0.0", GetParam()(m_value, 5));
}

INSTANTIATE_TEST_SUITE_P(
    Formatters, TestBigFloatFormatterUnderflow, testing::Values(bf_to_string, bf_to_string_e, bf_to_string_f));

TEST_F(TestBigFloatFormat, formatsAtMaximumSupportedPrecision)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(708);
    prepare_value("-1.25");

    EXPECT_EQ("-1.25", bf_to_string(m_value, g_bf_decimals));
    EXPECT_EQ("-1.25", bf_to_string_f(m_value, g_bf_decimals));
    EXPECT_EQ("-1.25e0", bf_to_string_e(m_value, g_bf_decimals));
}

TEST_F(TestBigFloatFormat, repeatedFormattingPreservesTheValue)
{
    prepare_value("-1.234567890123456789");
    const std::string original{
        reinterpret_cast<const char *>(m_value.data()), static_cast<std::size_t>(g_bf_length + 2)};
    const BigFloat &input = m_value;

    EXPECT_EQ("-1.23", bf_to_string(input, 3));
    EXPECT_EQ("-1.234567890123456789e0", bf_to_string_e(input, 0));
    EXPECT_EQ("-1.2346", bf_to_string_f(input, 5));
    EXPECT_EQ(original, (std::string{reinterpret_cast<const char *>(m_value.data()), original.size()}));
}

} // namespace id::test
