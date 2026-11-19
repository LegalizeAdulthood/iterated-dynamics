// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <math/BigFloat.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/fractalb.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigNum.h>
#include <math/fixed_pt.h>
#include <math/sqr.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
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

struct BigNumBailoutInput
{
    std::string name;
    Bailout test;
    DComplex point;
    double metric;
    double limit;
};

void PrintTo(const BigNumBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BigNumBailoutInput> inputs()
{
    std::vector<BigNumBailoutInput> result;
    const std::array<const char *, 7> names{"Mod", "Real", "Imag", "Or", "And", "Manh", "Manr"};
    const std::array<DComplex, 4> points{{{1.5, -1}, {-0.5, 1.25}, {0, 0}, {1, 0.5}}};
    const std::array<std::array<double, 7>, 4> metrics{
        {{3.25, 2.25, 1, 2.25, 1, 6.25, 0.25}, {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 0.5625},
            {0, 0, 0, 0, 0, 0, 0}, {1.25, 1, 0.25, 1, 0.25, 2.25, 2.25}}};
    for (int point = 0; point < 4; ++point)
    {
        for (int test = 0; test < 7; ++test)
        {
            for (int offset = -1; offset <= 1; ++offset)
            {
                const double metric = metrics[point][test];
                result.push_back(
                    {std::string{names[test]} + "_" + std::to_string(point) + "_" + std::to_string(offset + 1),
                        static_cast<Bailout>(test), points[point], metric, metric + offset * 0.25});
            }
        }
    }
    return result;
}

} // namespace

class TestBigNumBailout : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test};
    ValueSaver<double> m_limit{g_magnitude_limit};
    ValueSaver<double> m_magnitude{g_magnitude, -17};
    ValueSaver<bool> m_overflow{g_overflow};
    ValueSaver<decltype(g_bailout_float)> m_fp{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_bn{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bf{g_bailout_bigfloat};
};

void TestBigNumBailout::SetUp()
{
    init_bf_dec(40);
}

void TestBigNumBailout::TearDown()
{
    free_bf_vars();
}

class TestBigNumBailoutCases : public TestBigNumBailout, public testing::WithParamInterface<BigNumBailoutInput>
{
};

TEST_F(TestBigNumBailout, canonicalGlobalsOwnWorkingStorageThroughBindingAndCleanup)
{
    static_assert(std::is_same_v<decltype(g_new_z_bn), Complex<BigNum>>);
    static_assert(std::is_same_v<decltype(g_saved_z_bn), Complex<BigNum>>);
    static_assert(std::is_same_v<decltype(g_close_enough_bn), BigNum>);
    static_assert(std::is_same_v<decltype(g_param_z_bn), Complex<BigNum>>);
    static_assert(std::is_same_v<decltype(g_old_z_bn), BNComplex>);
    static_assert(std::is_same_v<decltype(g_bn_tmp), BigNum>);
    const std::array<BigNum *, 20> values{&g_new_z_bn.x, &g_new_z_bn.y, &g_saved_z_bn.x, &g_saved_z_bn.y,
        &g_close_enough_bn, &g_x_min_bn, &g_x_max_bn, &g_y_min_bn, &g_y_max_bn, &g_x_3rd_bn, &g_y_3rd_bn, &g_delta_x_bn,
        &g_delta_y_bn, &g_delta2_x_bn, &g_delta2_y_bn, &g_param_z_bn.x, &g_param_z_bn.y, &g_old_z_bn.x, &g_old_z_bn.y,
        &g_bn_tmp};
    BigArena &working = bn_arena(BigLifetime::WORKING);
    for (BigNum *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigNum{0}, *value);
        *value = -1.25L;
    }

    const std::size_t high_water = working.high_water_mark();
    init_bf_dec(40);
    EXPECT_EQ(high_water, working.high_water_mark());
    for (const BigNum *value : values)
    {
        EXPECT_EQ(BigNum{0}, *value);
    }
    init_bf_dec(80);
    for (BigNum *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &value->arena());
        EXPECT_EQ(BigNum{0}, *value);
    }
    free_bf_vars();
    for (const BigNum *value : values)
    {
        EXPECT_FALSE(*value);
    }
    init_bf_dec(40);
    for (const BigNum *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &value->arena());
        EXPECT_EQ(BigNum{0}, *value);
    }
}

TEST_P(TestBigNumBailoutCases, comparesFractionalThresholdsAndPreservesCachesAndOldOrbit)
{
    const auto &input = GetParam();
    const bool escaped = input.metric >= input.limit;
    set_bailout_formula(input.test);
    g_magnitude_limit = input.limit;
    g_old_z_bn.x = -7;
    g_old_z_bn.y = 9;
    g_new_z_bn.x = input.point.x;
    g_new_z_bn.y = input.point.y;
    const BigNum expected_x{sqr(input.point.x)};
    const BigNum expected_y{sqr(input.point.y)};
    const BigNum expected_old{escaped ? -7 : input.point.x};

    EXPECT_EQ(escaped, g_bailout_bignum() != 0);

    EXPECT_EQ(expected_x, g_tmp_sqr_x_bn);
    EXPECT_EQ(expected_y, g_tmp_sqr_y_bn);
    EXPECT_EQ(expected_old, g_old_z_bn.x);
    EXPECT_EQ(escaped ? 9 : input.point.y, bn_to_float(g_old_z_bn.y));
    EXPECT_EQ(input.point.x, bn_to_float(g_new_z_bn.x));
    EXPECT_EQ(input.point.y, bn_to_float(g_new_z_bn.y));
    EXPECT_EQ(-17, g_magnitude);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigNumBailoutCases, testing::ValuesIn(inputs()),
    [](const testing::TestParamInfo<BigNumBailoutInput> &info) { return info.param.name; });

class TestBigNumWideBailout : public TestBigNumBailout, public testing::WithParamInterface<Bailout>
{
};

TEST_P(TestBigNumWideBailout, retainsSqrValuesBeyondMachinePrecision)
{
    set_bailout_formula(GetParam());
    g_magnitude_limit = 0;
    g_new_z_bn.x = BigNum{"0.123456789012345678901234567890123456789"};
    g_new_z_bn.y = BigNum{"-0.345678901234567890123456789012345678901"};
    const BigNum expected_x = g_new_z_bn.x.sqr();
    const BigNum expected_y = g_new_z_bn.y.sqr();
    const BigNum original_x{g_new_z_bn.x};
    const BigNum original_y{g_new_z_bn.y};

    EXPECT_EQ(1, g_bailout_bignum());

    EXPECT_EQ(expected_x, g_tmp_sqr_x_bn);
    EXPECT_EQ(expected_y, g_tmp_sqr_y_bn);
    EXPECT_EQ(original_x, g_new_z_bn.x);
    EXPECT_EQ(original_y, g_new_z_bn.y);
}

TEST_P(TestBigNumWideBailout, comparesMetricBytesBelowMachinePrecision)
{
    const Bailout test = GetParam();
    set_bailout_formula(test);
    g_magnitude_limit = test == Bailout::MOD ? 2 : test == Bailout::MANH || test == Bailout::MANR ? 4 : 1;
    g_new_z_bn.x = BigNum{"0.999999999999999999999999999999999999999"};
    g_new_z_bn.y = g_new_z_bn.x;

    EXPECT_EQ(0, g_bailout_bignum());
}

INSTANTIATE_TEST_SUITE_P(Modes, TestBigNumWideBailout,
    testing::Values(
        Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND, Bailout::MANH, Bailout::MANR));

namespace
{

std::int64_t wrapped_integer(const std::int64_t value)
{
    const std::int64_t modulus = std::int64_t{1} << (8 * g_int_length);
    const std::int64_t low = value & (modulus - 1);
    return low >= modulus / 2 ? low - modulus : low;
}

std::int64_t extracted_integer(const std::int64_t value)
{
    const std::int64_t modulus = std::int64_t{1} << (8 * g_int_length);
    return g_int_length == 4 ? static_cast<long>(static_cast<std::uint32_t>(value)) : value & (modulus - 1);
}

struct ProductBailoutInput
{
    const char *name;
    int real;
    int imaginary;
    double limit;
};

void PrintTo(const ProductBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

} // namespace

class TestBigNumProductWindows : public TestBigNumBailout, public testing::WithParamInterface<std::tuple<int, int>>
{
protected:
    void SetUp() override;
    void initialize_product_precision(int length, int integer_length);
};

void TestBigNumProductWindows::initialize_product_precision(const int length, const int integer_length)
{
    g_user.bailout_value = integer_length == 4 ? 11 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(length);
    ASSERT_EQ(integer_length, g_int_length);
}

void TestBigNumProductWindows::SetUp()
{
    TestBigNumBailout::SetUp();
    initialize_product_precision(std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(TestBigNumProductWindows, fractionalThresholdAboveTheMetricDoesNotRoundDownToEquality)
{
    set_bailout_formula(Bailout::REAL);
    g_magnitude_limit = std::nextafter(1.0, 2.0);
    g_new_z_bn.x = 1;
    g_new_z_bn.y = 0;

    EXPECT_EQ(0, g_bailout_bignum());
}

TEST_P(TestBigNumProductWindows, smallPositiveThresholdRemainsGreaterThanZero)
{
    set_bailout_formula(Bailout::REAL);
    g_magnitude_limit = std::ldexp(1.0, -80);
    g_new_z_bn.x = 0;
    g_new_z_bn.y = 0;

    EXPECT_EQ(0, g_bailout_bignum());
}

TEST_P(TestBigNumProductWindows, highBitMetricRemainsBelowItsFractionalUpperThreshold)
{
    set_bailout_formula(Bailout::REAL);
    g_new_z_bn.x = g_int_length == 1 ? 12 : g_int_length == 2 ? 200 : 50000;
    g_new_z_bn.y = 0;
    const double metric = g_int_length == 1 ? 144 : g_int_length == 2 ? 40000 : -1794967296.0;
    g_magnitude_limit = metric + 0.25;

    EXPECT_EQ(0, g_bailout_bignum());
}

TEST_P(TestBigNumProductWindows, highBitMetricReachesItsFractionalLowerThreshold)
{
    set_bailout_formula(Bailout::REAL);
    g_new_z_bn.x = g_int_length == 1 ? 12 : g_int_length == 2 ? 200 : 50000;
    g_new_z_bn.y = 0;
    const double metric = g_int_length == 1 ? 144 : g_int_length == 2 ? 40000 : -1794967296.0;
    g_magnitude_limit = metric - 0.25;

    EXPECT_EQ(1, g_bailout_bignum());
}

TEST_P(TestBigNumProductWindows, precisionChangesClearOrbitAndScratchAndCleanupUnbindsThem)
{
    g_old_z_bn.x = -7;
    g_old_z_bn.y = 9;
    g_tmp_sqr_x_bn = 11;
    g_tmp_sqr_y_bn = 17;
    g_bn_tmp = -23;
    const int length = g_bn_length;
    init_bf_length(length == 4 ? 8 : 4);
    init_bf_length(length);

    EXPECT_EQ(BigNum{0}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{0}, g_old_z_bn.y);
    EXPECT_EQ(BigNum{0}, g_tmp_sqr_x_bn);
    EXPECT_EQ(BigNum{0}, g_tmp_sqr_y_bn);
    EXPECT_EQ(BigNum{0}, g_bn_tmp);

    free_bf_vars();

    EXPECT_FALSE(g_old_z_bn.x);
    EXPECT_FALSE(g_old_z_bn.y);
    EXPECT_FALSE(g_tmp_sqr_x_bn);
    EXPECT_FALSE(g_tmp_sqr_y_bn);
    EXPECT_FALSE(g_bn_tmp);
    init_bf_length(length);
    EXPECT_EQ(BigNum{0}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{0}, g_bn_tmp);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumProductWindows,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 68, 672), testing::Values(1, 2, 4)));

class TestBigNumSlotSizing : public TestBigNumBailout, public testing::WithParamInterface<std::tuple<int, BigLifetime>>
{
protected:
    void SetUp() override;
};

void TestBigNumSlotSizing::SetUp()
{
    g_bailout_test = Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
}

TEST_P(TestBigNumSlotSizing, initialStrideAndCapacityUseOnlyTheCanonicalExtent)
{
    const BigArena &arena = bn_arena(std::get<1>(GetParam()));
    const std::size_t stride = (std::get<0>(GetParam()) + 63) / 64 * 64;
    EXPECT_EQ(std::get<0>(GetParam()), arena.value_size());
    EXPECT_EQ(stride, arena.block_size());
    EXPECT_EQ((1024U * 1024 / stride) & ~std::size_t{1}, arena.capacity());
    EXPECT_EQ(0U, arena.capacity() * arena.block_size() % 64);
}

INSTANTIATE_TEST_SUITE_P(LengthsAndLifetimes, TestBigNumSlotSizing,
    testing::Combine(testing::Values(4, 56, 60, 64, 68, 672),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

class TestBigNumProductBailout
    : public TestBigNumBailout,
      public testing::WithParamInterface<std::tuple<int, int, Bailout, ProductBailoutInput, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumProductBailout::SetUp()
{
    TestBigNumBailout::SetUp();
    const auto [length, integer_length, mode, input, overflow] = GetParam();
    g_user.bailout_value = integer_length == 4 ? 11 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(length);
    ASSERT_EQ(integer_length, g_int_length);
    set_bailout_formula(mode);
    g_magnitude_limit = input.limit;
    g_overflow = overflow;
}

TEST_P(TestBigNumProductBailout, preservesScratchValuesAndFixedWidthOverflow)
{
    const auto [length, integer_length, mode, input, overflow] = GetParam();
    const auto x2 = static_cast<std::int64_t>(input.real) * input.real;
    const auto y2 = static_cast<std::int64_t>(input.imaginary) * input.imaginary;
    const auto sum = wrapped_integer(
        mode == Bailout::MANH ? std::abs(input.real) + std::abs(input.imaginary) : input.real + input.imaginary);
    const BigNum expected_x{static_cast<LDouble>(wrapped_integer(x2))};
    const BigNum expected_y{static_cast<LDouble>(wrapped_integer(y2))};
    const auto rx = extracted_integer(x2);
    const auto ry = extracted_integer(y2);
    const auto magnitude = extracted_integer(rx + ry);
    const auto man_metric = extracted_integer(sum * sum);
    const bool escaped = mode == Bailout::MOD ? magnitude >= input.limit
        : mode == Bailout::REAL               ? rx >= input.limit
        : mode == Bailout::IMAG               ? ry >= input.limit
        : mode == Bailout::OR                 ? rx >= input.limit || ry >= input.limit
        : mode == Bailout::AND                ? rx >= input.limit && ry >= input.limit
                                              : man_metric >= input.limit;
    g_old_z_bn.x = -7;
    g_old_z_bn.y = 9;
    g_bn_tmp = -17;
    g_new_z_bn.x = input.real;
    g_new_z_bn.y = input.imaginary;
    const BigNum expected_tmp{static_cast<LDouble>(mode == Bailout::MOD ? wrapped_integer(magnitude)
            : mode == Bailout::MANH || mode == Bailout::MANR            ? sum
                                                                        : -17)};

    EXPECT_EQ(escaped, g_bailout_bignum() != 0);

    EXPECT_EQ(expected_x, g_tmp_sqr_x_bn);
    EXPECT_EQ(expected_y, g_tmp_sqr_y_bn);
    EXPECT_EQ(BigNum{escaped ? -7.0L : static_cast<LDouble>(input.real)}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{escaped ? 9.0L : static_cast<LDouble>(input.imaginary)}, g_old_z_bn.y);
    EXPECT_EQ(expected_tmp, g_bn_tmp);
    EXPECT_EQ(BigNum{static_cast<LDouble>(input.real)}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{static_cast<LDouble>(input.imaginary)}, g_new_z_bn.y);
    EXPECT_EQ(rx, bn_to_int(g_tmp_sqr_x_bn));
    EXPECT_EQ(ry, bn_to_int(g_tmp_sqr_y_bn));
    EXPECT_EQ(overflow, g_overflow);
    EXPECT_EQ(-17, g_magnitude);
    EXPECT_EQ(length, g_bn_length);
    EXPECT_EQ(integer_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndModes, TestBigNumProductBailout,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(
            Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND, Bailout::MANH, Bailout::MANR),
        testing::Values(ProductBailoutInput{"Continue", 1, -2, 1000}, ProductBailoutInput{"Escape", -3, 2, 1},
            ProductBailoutInput{"Wrap", 100, 100, 1}, ProductBailoutInput{"FractionAbove", 1, 0, 1.25},
            ProductBailoutInput{"UnsignedHighAbove", 12, 0, 144.25},
            ProductBailoutInput{"UnsignedCrossSign", 12, 0, 127.75},
            ProductBailoutInput{"WideUnsignedAbove", 100, 100, 40000.25}),
        testing::Bool()));

class TestBigNumBailoutProductStorage : public TestBigNumBailout,
                                        public testing::WithParamInterface<std::tuple<int, int, Bailout, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumBailoutProductStorage::SetUp()
{
    TestBigNumBailout::SetUp();
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 11 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
    set_bailout_formula(std::get<2>(GetParam()));
    g_magnitude_limit = std::get<3>(GetParam()) ? 0.5 : 32;
}

TEST_P(TestBigNumBailoutProductStorage, reservesCanonicalMetricAndReusesPrivateSqrScratchWithoutTouchingNeighbors)
{
    const Bailout mode = std::get<2>(GetParam());
    const bool escape = std::get<3>(GetParam());
    BigStackSaver saved;
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    ASSERT_EQ(0U, temporary.high_water_mark());
    BigNum survivor{temporary, -7};
    const std::size_t stride = temporary.block_size();

    Byte *const borrowed = bn_stack().allocate_single();
    std::fill_n(borrowed, stride, Byte{0x3c});
    BigArena::Allocation guard = temporary.allocate_double();
    std::fill_n(guard.data(), 2 * stride, Byte{0xa5});
    const BigStackMark mark = save_stack();
    ASSERT_EQ(4U, temporary.high_water_mark());
    BigArena::Allocation slot = temporary.allocate_single();
    const BigArena::Index slot_index = slot.index();
    BigNum neighbor{temporary, -11};

    BigArena::Allocation scratch = temporary.allocate_double();
    const BigArena::Index scratch_index = scratch.index();
    BigArena::Allocation second_pair;
    if (mode == Bailout::MANH)
    {
        second_pair = temporary.allocate_double();
    }
    second_pair.reset();
    scratch.reset();
    slot.reset();
    const std::size_t high_water = temporary.high_water_mark();
    g_old_z_bn.x = -7;
    g_old_z_bn.y = 9;
    g_new_z_bn.x = -3;
    g_new_z_bn.y = 2;

    EXPECT_EQ(escape ? 1 : 0, g_bailout_bignum());

    // The isolated half holds abs_x (MANH) or the metric (MANR).
    // MANH's abs_y and metric share a pair. Only sqr scratch needs
    // another pair, released before the canonical limit is constructed.
    EXPECT_EQ(high_water, temporary.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(BigNum{-11}, neighbor);
    const std::size_t warmed_high_water = temporary.high_water_mark();
    EXPECT_EQ(escape ? 1 : 0, g_bailout_bignum());
    EXPECT_EQ(warmed_high_water, temporary.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(BigNum{-7}, survivor);
    EXPECT_EQ(std::vector<Byte>(stride, 0x3c), std::vector<Byte>(borrowed, borrowed + stride));
    EXPECT_EQ(std::vector<Byte>(2 * stride, 0xa5), std::vector<Byte>(guard.data(), guard.data() + 2 * stride));
    EXPECT_EQ(BigNum{9}, g_tmp_sqr_x_bn);
    EXPECT_EQ(BigNum{4}, g_tmp_sqr_y_bn);
    EXPECT_EQ(BigNum{-3}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{2}, g_new_z_bn.y);
    EXPECT_EQ(BigNum{escape ? -7.0L : -3.0L}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{escape ? 9.0L : 2.0L}, g_old_z_bn.y);
    EXPECT_EQ(BigNum{mode == Bailout::MANH ? 5.0L : -1.0L}, g_bn_tmp);

    BigArena::Allocation reused_slot = temporary.allocate_single();
    EXPECT_EQ(slot_index, reused_slot.index());
    BigArena::Allocation first = temporary.allocate_double();
    EXPECT_EQ(scratch_index, first.index());
    if (mode == Bailout::MANH)
    {
        BigArena::Allocation second = temporary.allocate_double();
        EXPECT_EQ(warmed_high_water, temporary.high_water_mark());
    }
    else
    {
        EXPECT_EQ(warmed_high_water, temporary.high_water_mark());
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsModesAndOutcomes, TestBigNumBailoutProductStorage,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(Bailout::MANH, Bailout::MANR), testing::Bool()));

class TestBigNumBailoutContinuation : public TestBigNumBailout,
                                      public testing::WithParamInterface<std::tuple<Bailout, bool>>
{
};

TEST_P(TestBigNumBailoutContinuation, repeatedOrbitCallsMatchTheOrdinaryBailoutContract)
{
    const Bailout mode = std::get<0>(GetParam());
    const bool escape = std::get<1>(GetParam());
    ValueSaver<DComplex> saved_old{g_old_z, DComplex{1, 1}};
    ValueSaver<DComplex> saved_new{g_new_z};
    ValueSaver<DComplex> saved_parameter{g_param_z1, DComplex{1, 0}};
    ValueSaver<DComplex *> saved_float_parameter{g_float_param, &g_param_z1};
    ValueSaver<double> saved_sqr_x{g_temp_sqr_x, 1};
    ValueSaver<double> saved_sqr_y{g_temp_sqr_y, 1};
    set_bailout_formula(mode);
    g_magnitude_limit = escape ? 1 : 100;
    g_old_z_bn.x = 1;
    g_old_z_bn.y = 1;
    g_param_z_bn.x = 1;
    g_param_z_bn.y = 0;
    square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    const std::array<DComplex, 2> expected{{{1, 2}, {-2, escape ? 2.0 : 4.0}}};

    for (int iteration = 0; iteration < 2; ++iteration)
    {
        const int ordinary_result = julia_orbit();
        const BigStackMark mark = save_stack();
        const std::size_t temporaries = big_memory_stats().temporaries;

        EXPECT_EQ(ordinary_result, julia_orbit_bn());

        EXPECT_EQ(mark, save_stack());
        if (iteration != 0)
        {
            EXPECT_EQ(temporaries, big_memory_stats().temporaries);
        }
        EXPECT_EQ(BigNum{expected[iteration].x}, g_new_z_bn.x);
        EXPECT_EQ(BigNum{expected[iteration].y}, g_new_z_bn.y);
        EXPECT_EQ(BigNum{g_old_z.x}, g_old_z_bn.x);
        EXPECT_EQ(BigNum{g_old_z.y}, g_old_z_bn.y);
        EXPECT_EQ(BigNum{sqr(expected[iteration].x)}, g_tmp_sqr_x_bn);
        EXPECT_EQ(BigNum{sqr(expected[iteration].y)}, g_tmp_sqr_y_bn);
    }
}

INSTANTIATE_TEST_SUITE_P(ModesAndOutcomes, TestBigNumBailoutContinuation,
    testing::Combine(testing::Values(Bailout::MANH, Bailout::MANR), testing::Bool()));

namespace
{

struct JuliaProductInput
{
    const char *name;
    DComplex initial;
    DComplex parameter;
    DComplex next;
    int doubled_product;
};

void PrintTo(const JuliaProductInput &input, std::ostream *out)
{
    *out << input.name;
}

testing::MockFunction<int()> *s_julia_product_bailout{};

int inspect_julia_product_bailout()
{
    return s_julia_product_bailout->Call();
}

} // namespace

class TestBigNumJuliaProduct : public TestBigNumBailout,
                               public testing::WithParamInterface<std::tuple<int, int, JuliaProductInput, bool, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumJuliaProduct::SetUp()
{
    TestBigNumBailout::SetUp();
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 11 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
    set_bailout_formula(Bailout::MOD);
    g_magnitude_limit = std::get<3>(GetParam()) ? 1 : 10000;
}

TEST_P(TestBigNumJuliaProduct, preservesCanonicalProductOperandsAndTemporaryLifetimes)
{
    const JuliaProductInput &input = std::get<2>(GetParam());
    const bool escape = std::get<3>(GetParam());
    const bool borrow_stack = std::get<4>(GetParam());
    BigStackSaver saved_stack;
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    BigNum survivor{temporary, -7};
    const std::size_t temporary_stride = temporary.block_size();
    Byte *const prior_borrow = bn_stack().allocate_single();
    std::fill_n(prior_borrow, temporary_stride, Byte{0x3c});
    const BigStackMark mark = save_stack();
    g_old_z_bn.x = input.initial.x;
    g_old_z_bn.y = input.initial.y;
    g_param_z_bn.x = input.parameter.x;
    g_param_z_bn.y = input.parameter.y;
    square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    BigArena::Allocation reservation = temporary.allocate_single();
    const BigArena::Index product_index = reservation.index();
    BigNum companion{temporary, -19};
    BigArena::Allocation scratch = temporary.allocate_double();
    const BigArena::Index scratch_index = scratch.index();
    scratch.reset();
    reservation.reset();
    const std::size_t high_water = temporary.high_water_mark();
    const BNComplex old_z{g_old_z_bn};
    g_bn_tmp = -17;
    const BigNum canonical_scratch{-17};
    const BigNum product{static_cast<LDouble>(input.doubled_product)};
    Byte *borrowed{};
    int (*const bailout)() = g_bailout_bignum;
    testing::MockFunction<int()> callback;
    const ValueSaver<testing::MockFunction<int()> *> saved_callback{s_julia_product_bailout, &callback};
    const ValueSaver<decltype(g_bailout_bignum)> saved_bailout{g_bailout_bignum, inspect_julia_product_bailout};
    EXPECT_CALL(callback, Call())
        .WillOnce(
            [&]
            {
                EXPECT_EQ(product, g_new_z_bn.y - g_param_z_bn.y);
                EXPECT_EQ(BigNum{-19}, companion);
                EXPECT_EQ(canonical_scratch, g_bn_tmp);
                EXPECT_EQ(old_z, g_old_z_bn);
                {
                    BigArena::Allocation probe = temporary.allocate_double();
                    EXPECT_EQ(scratch_index, probe.index());
                    EXPECT_EQ(high_water, temporary.high_water_mark());
                    BigArena::Allocation single = temporary.allocate_single();
                    EXPECT_NE(product_index, single.index());
                }
                if (borrow_stack)
                {
                    borrowed = bn_stack().allocate_double();
                }
                return bailout();
            });

    EXPECT_EQ(escape ? 1 : 0, julia_orbit_bn());

    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(BigNum{-19}, companion);
    EXPECT_EQ(BigNum{-7}, survivor);
    EXPECT_EQ(
        std::vector<Byte>(temporary_stride, 0x3c), std::vector<Byte>(prior_borrow, prior_borrow + temporary_stride));
    EXPECT_EQ(BigNum{input.next.x}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{input.next.y}, g_new_z_bn.y);
    const DComplex old = escape ? input.initial : input.next;
    EXPECT_EQ(BigNum{old.x}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{old.y}, g_old_z_bn.y);
    EXPECT_EQ(BigNum{sqr(input.next.x)}, g_tmp_sqr_x_bn);
    EXPECT_EQ(BigNum{sqr(input.next.y)}, g_tmp_sqr_y_bn);
    BigArena::Allocation released = temporary.allocate_single();
    EXPECT_EQ(product_index, released.index());
    BigArena::Allocation reused_scratch = temporary.allocate_double();
    EXPECT_EQ(scratch_index, reused_scratch.index());
    if (borrow_stack)
    {
        EXPECT_EQ(reused_scratch.data(), borrowed);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsSignsOutcomesAndStackBorrows, TestBigNumJuliaProduct,
    testing::Combine(testing::Values(4, 12, 64, 672), testing::Values(1, 2, 4),
        testing::Values(JuliaProductInput{"Positive", {1, 1}, {1, 0}, {1, 2}, 2},
            JuliaProductInput{"PositiveNegative", {2, -1}, {0, 1}, {3, -3}, -4},
            JuliaProductInput{"NegativePositive", {-2, 3}, {0, 0}, {-5, -12}, -12},
            JuliaProductInput{"Negative", {-2, -2}, {1, 0}, {1, 8}, 8}),
        testing::Bool(), testing::Bool()));

class TestBigNumJuliaCanonicalProduct : public TestBigNumBailout,
                                        public testing::WithParamInterface<std::tuple<int, int, bool>>
{
protected:
    void SetUp() override;
};

void TestBigNumJuliaCanonicalProduct::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    g_user.bailout_value = integer_length == 4 ? 11 : 0;
    g_bailout_test = integer_length == 2 ? Bailout::REAL : Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
    ASSERT_EQ(integer_length, g_int_length);
    set_bailout_formula(Bailout::REAL);
    g_magnitude_limit = 0;
    g_param_z_bn.x = 0;
    g_param_z_bn.y = 0;
}

TEST_P(TestBigNumJuliaCanonicalProduct, discardsLowProductBitsBeforeDoubling)
{
    const bool negative = std::get<2>(GetParam());
    BigNum quantum{1};
    for (int byte = g_int_length; byte < g_bn_length; ++byte)
    {
        quantum /= U16{256};
    }
    g_old_z_bn.x = negative ? -quantum : quantum;
    g_old_z_bn.y = 0.5L;
    square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    const BigNum expected_y = negative ? -(quantum + quantum) : BigNum{0};
    const BigNum original_x{g_old_z_bn.x};

    EXPECT_EQ(1, julia_orbit_bn());

    EXPECT_EQ(BigNum{-0.25L}, g_new_z_bn.x);
    EXPECT_EQ(expected_y, g_new_z_bn.y);
    EXPECT_EQ(original_x, g_old_z_bn.x);
    EXPECT_EQ(BigNum{0.5L}, g_old_z_bn.y);
}

TEST_P(TestBigNumJuliaCanonicalProduct, preservesFixedWidthOverflowAndDestinationArena)
{
    const bool negative = std::get<2>(GetParam());
    g_old_z_bn.x = negative ? -100 : 100;
    g_old_z_bn.y = 100;
    g_param_z_bn.x = 1;
    g_overflow = true;
    square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
    square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    BigArena &working = bn_arena(BigLifetime::WORKING);
    const BigNum expected_y{static_cast<LDouble>(wrapped_integer(negative ? -20000 : 20000))};
    // The additions in the orbit reuse one working slot for their result.
    working.allocate_single().reset();
    const std::size_t high_water = working.high_water_mark();

    EXPECT_EQ(1, julia_orbit_bn());

    EXPECT_EQ(&working, &g_new_z_bn.x.arena());
    EXPECT_EQ(&working, &g_new_z_bn.y.arena());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(BigNum{1}, g_new_z_bn.x);
    EXPECT_EQ(expected_y, g_new_z_bn.y);
    EXPECT_TRUE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigNumJuliaCanonicalProduct,
    testing::Combine(testing::Values(8, 12, 60, 64, 672), testing::Values(1, 2, 4), testing::Bool()));

class TestBigNumMandelbrotInitialization
    : public TestBigNumBailout,
      public testing::WithParamInterface<std::tuple<InitOrbitMode, ColorMethod, bool, bool>>
{
protected:
    void TearDown() override;

    ValueSaver<const FractalSpecific *> m_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<FractalDispatch> m_dispatch{g_dispatch, FractalDispatch{FractalType::MANDEL}};
    ValueSaver<InitOrbitMode> m_initial_mode{g_use_init_orbit};
    ValueSaver<DComplex> m_initial_value{g_init_orbit};
    ValueSaver<DComplex> m_parameter{g_param_z1, DComplex{0.125, -0.0625}};
    ValueSaver<double> m_real{g_params[0], 0.125};
    ValueSaver<double> m_imaginary{g_params[1], -0.0625};
    ValueSaver<CalcMode> m_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen, LogicalScreen{5, 5, 0, 0, 4, 4}};
    ValueSaver<ImageRegion> m_region{g_image_region, ImageRegion{{-1, -1}, {1, 1}, {-0.5, -0.5}}};
    ValueSaver<Inversion> m_inversion{g_inversion, Inversion{}};
    ValueSaver<LDouble> m_delta_x{g_delta_x, 0.375};
    ValueSaver<LDouble> m_delta_y{g_delta_y, 0.375};
    ValueSaver<LDouble> m_delta2_x{g_delta_x2, 0.125};
    ValueSaver<LDouble> m_delta2_y{g_delta_y2, 0.125};
    ValueSaver<int> m_col{g_col, 1};
    ValueSaver<int> m_row{g_row, 2};
    ValueSaver<int> m_periodicity{g_periodicity_check, 0};
    ValueSaver<ColorMethod> m_inside{g_inside_method};
    ValueSaver<bool> m_book{g_bof_match_book_images};
    ValueSaver<long> m_iteration{g_color_iter, 37};
    ValueSaver<int> m_exponent{g_c_exponent};
    ValueSaver<DComplex> m_power{g_power_z};
    ValueSaver<DComplex *> m_float_parameter{g_float_param};
    ValueSaver<DComplex> m_init{g_init};
    ValueSaver<DComplex> m_old{g_old_z};
    ValueSaver<DComplex> m_new{g_new_z};
    ValueSaver<DComplex> m_tmp{g_tmp_z};
    ValueSaver<double> m_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_sqr_y{g_temp_sqr_y};
    ValueSaver<DebugFlags> m_debug{g_debug_flag, DebugFlags::FORCE_STANDARD_FRACTAL};
};

void TestBigNumMandelbrotInitialization::TearDown()
{
    free_pixel_grid();
    TestBigNumBailout::TearDown();
}

TEST_P(TestBigNumMandelbrotInitialization, dispatchMatchesRegularSeedParameterCachesAndFirstIterate)
{
    const InitOrbitMode mode = std::get<0>(GetParam());
    const ColorMethod inside = std::get<1>(GetParam());
    const bool book = std::get<2>(GetParam());
    const bool zero = std::get<3>(GetParam());
    g_use_init_orbit = mode;
    g_init_orbit = zero ? DComplex{} : DComplex{0.5, -0.25};
    g_inside_method = inside;
    g_bof_match_book_images = book;
    g_magnitude_limit = 4;
    set_bailout_formula(Bailout::MOD);
    const DComplex pixel{-0.375, 0.125};
    const bool bof = book && (inside == ColorMethod::BOF60 || inside == ColorMethod::BOF61);
    DComplex seed = bof ? DComplex{} : mode == InitOrbitMode::VALUE ? g_init_orbit : pixel;
    seed += g_param_z1;
    const long iteration = bof ? -1 : 37;
    alloc_pixel_grid();
    fill_pixel_grid();
    ASSERT_TRUE(g_dispatch.per_image()());
    ASSERT_EQ(mandel_per_pixel, g_dispatch.per_pixel());
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    EXPECT_EQ(seed, g_old_z);
    EXPECT_EQ(pixel, g_init);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(sqr(seed.x), g_temp_sqr_x);
    EXPECT_EQ(sqr(seed.y), g_temp_sqr_y);
    ASSERT_EQ(0, g_dispatch.orbit_calc()());
    const DComplex first = g_new_z;
    EXPECT_EQ(sqr(seed.x) - sqr(seed.y) + pixel.x, first.x);
    EXPECT_EQ(2 * seed.x * seed.y + pixel.y, first.y);

    init_bf_dec(40);
    g_bf_x_min = -1;
    g_bf_x_max = 1;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_x_3rd = -0.5;
    g_bf_y_3rd = -0.5;
    g_color_iter = 37;
    g_dispatch = FractalDispatch{FractalType::MANDEL};
    ASSERT_TRUE(select_alternate_math_dispatch());
    ASSERT_EQ(mandel_per_pixel_bn, g_dispatch.per_pixel());
    ASSERT_TRUE(g_dispatch.per_image()());
    ASSERT_EQ(BFMathType::BIG_NUM, g_bf_math);

    ASSERT_EQ(1, g_dispatch.per_pixel()());

    EXPECT_EQ(BigNum{seed.x}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{seed.y}, g_old_z_bn.y);
    EXPECT_EQ(BigNum{seed.x}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{seed.y}, g_new_z_bn.y);
    EXPECT_EQ(BigNum{pixel.x}, g_param_z_bn.x);
    EXPECT_EQ(BigNum{pixel.y}, g_param_z_bn.y);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(sqr(seed.x), bn_to_float(g_tmp_sqr_x_bn));
    EXPECT_EQ(sqr(seed.y), bn_to_float(g_tmp_sqr_y_bn));

    ASSERT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_EQ(BigNum{first.x}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{first.y}, g_new_z_bn.y);
    EXPECT_EQ(BigNum{pixel.x}, g_param_z_bn.x);
    EXPECT_EQ(BigNum{pixel.y}, g_param_z_bn.y);
    EXPECT_EQ(iteration, g_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ModesAndBof, TestBigNumMandelbrotInitialization,
    testing::Combine(testing::Values(InitOrbitMode::NORMAL, InitOrbitMode::VALUE, InitOrbitMode::PIXEL),
        testing::Values(ColorMethod::COLOR, ColorMethod::BOF60, ColorMethod::BOF61), testing::Bool(), testing::Bool()));

class TestBigNumJulia : public TestBigNumBailout
{
protected:
    void SetUp() override;

    ValueSaver<CalcMode> m_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen, LogicalScreen{257, 513, 0, 0, 256, 512}};
    ValueSaver<int> m_col{g_col, 0};
    ValueSaver<int> m_row{g_row, 0};
    ValueSaver<long> m_iteration{g_color_iter, 37};
    ValueSaver<int> m_periodicity{g_periodicity_check, 0};
    ValueSaver<ColorMethod> m_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<bool> m_book{g_bof_match_book_images, false};
    ValueSaver<int> m_exponent{g_c_exponent};
    ValueSaver<double> m_real{g_params[0], 0.5};
    ValueSaver<double> m_imaginary{g_params[1], 0.375};
};

void TestBigNumJulia::SetUp()
{
    g_fractal_type = FractalType::JULIA;
    TestBigNumBailout::SetUp();
    set_bailout_formula(Bailout::MOD);
}

TEST_F(TestBigNumJulia, affineSeedsKeepPreciseFixedParameterAndWideSquareCaches)
{
    float_to_bf(g_bf_x_min, -2);
    float_to_bf(g_bf_x_max, 2);
    float_to_bf(g_bf_y_min, -1);
    float_to_bf(g_bf_y_max, 1);
    float_to_bf(g_bf_x_3rd, -1.5);
    float_to_bf(g_bf_y_3rd, -0.75);
    str_to_bf(g_bf_params[0], "0.25000000000000000001");
    str_to_bf(g_bf_params[1], "-0.12500000000000000001");
    const BigNum parameter_x{g_bf_params[0]};
    const BigNum parameter_y{g_bf_params[1]};
    g_magnitude_limit = 1024;

    ASSERT_TRUE(mandel_per_image_bn());
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &g_close_enough_bn.arena());
    ASSERT_EQ(BFMathType::BIG_NUM, g_bf_math);
    for (const int col : {0, 128, 256})
    {
        for (const int row : {0, 256, 512})
        {
            SCOPED_TRACE(col);
            SCOPED_TRACE(row);
            g_col = col;
            g_row = row;
            const double real = -2 + col * (7.0 / 512) + row * (1.0 / 1024);
            const double imag = 1 - (row * (7.0 / 2048) + col * (1.0 / 1024));

            EXPECT_EQ(1, julia_per_pixel_bn());

            EXPECT_EQ(37, g_color_iter);
            EXPECT_DOUBLE_EQ(real, static_cast<double>(bn_to_float(g_old_z_bn.x)));
            EXPECT_DOUBLE_EQ(imag, static_cast<double>(bn_to_float(g_old_z_bn.y)));
            EXPECT_DOUBLE_EQ(real, static_cast<double>(bn_to_float(g_new_z_bn.x)));
            EXPECT_DOUBLE_EQ(imag, static_cast<double>(bn_to_float(g_new_z_bn.y)));
            EXPECT_DOUBLE_EQ(real * real, static_cast<double>(bn_to_float(g_tmp_sqr_x_bn)));
            EXPECT_DOUBLE_EQ(imag * imag, static_cast<double>(bn_to_float(g_tmp_sqr_y_bn)));
            EXPECT_EQ(parameter_x, g_param_z_bn.x);
            EXPECT_EQ(parameter_y, g_param_z_bn.y);

            EXPECT_EQ(0, julia_orbit_bn());

            EXPECT_EQ(BigNum{real * real - imag * imag} + g_param_z_bn.x, g_new_z_bn.x);
            EXPECT_EQ(BigNum{2 * real * imag} + g_param_z_bn.y, g_new_z_bn.y);
            EXPECT_EQ(parameter_x, g_param_z_bn.x);
            EXPECT_EQ(parameter_y, g_param_z_bn.y);
        }
    }
}

TEST_F(TestBigNumJulia, viewportTransitionRestoresJuliaParameterAndKeepsBailoutEquality)
{
    float_to_bf(g_bf_x_min, 1);
    float_to_bf(g_bf_x_max, 3);
    float_to_bf(g_bf_x_3rd, 1);
    float_to_bf(g_bf_y_min, -1);
    float_to_bf(g_bf_y_max, 1);
    float_to_bf(g_bf_y_3rd, -1);
    clear_bf(g_bf_params[0]);
    clear_bf(g_bf_params[1]);
    g_col = 128;
    g_row = 256;
    g_magnitude_limit = 16;
    g_fractal_type = FractalType::MANDEL;
    ASSERT_TRUE(mandel_per_image_bn());
    ASSERT_EQ(1, mandel_per_pixel_bn());
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &g_param_z_bn.y.arena());
    ASSERT_EQ(BigNum{2}, g_param_z_bn.x);

    g_fractal_type = FractalType::JULIA;
    ASSERT_TRUE(mandel_per_image_bn());
    ASSERT_EQ(BigNum{0}, g_param_z_bn.x);
    ASSERT_EQ(BigNum{0}, g_param_z_bn.y);
    ASSERT_EQ(1, julia_per_pixel_bn());
    EXPECT_EQ(37, g_color_iter);
    EXPECT_EQ(2, bn_to_float(g_old_z_bn.x));
    EXPECT_EQ(0, bn_to_float(g_old_z_bn.y));

    EXPECT_EQ(1, julia_orbit_bn());

    EXPECT_EQ(BigNum{4}, g_new_z_bn.x);
    EXPECT_EQ(BigNum{0}, g_new_z_bn.y);
    EXPECT_EQ(2, bn_to_float(g_old_z_bn.x));
    EXPECT_EQ(0, bn_to_float(g_old_z_bn.y));
}

class TestBigNumSqrStorage : public TestBigNumBailout, public testing::WithParamInterface<std::tuple<int, bool>>
{
protected:
    void SetUp() override;
    BigNum &cache() const;
};

void TestBigNumSqrStorage::SetUp()
{
    g_bailout_test = Bailout::MOD;
    init_bf_length(std::get<0>(GetParam()));
}

BigNum &TestBigNumSqrStorage::cache() const
{
    return std::get<1>(GetParam()) ? g_tmp_sqr_y_bn : g_tmp_sqr_x_bn;
}

TEST_P(TestBigNumSqrStorage, sqrPreservesTheOtherCacheAndReusesScratch)
{
    static_assert(std::is_same_v<decltype(g_tmp_sqr_x_bn), BigNum>);
    static_assert(std::is_same_v<decltype(g_tmp_sqr_y_bn), BigNum>);
    BigNum &value = cache();
    BigNum &other = std::get<1>(GetParam()) ? g_tmp_sqr_x_bn : g_tmp_sqr_y_bn;
    BigArena &working = bn_arena(BigLifetime::WORKING);
    BigArena &temporary = bn_arena(BigLifetime::TEMPORARY);
    ASSERT_TRUE(value);
    EXPECT_EQ(&working, &value.arena());
    EXPECT_EQ(BigNum{0}, value);
    value = 17;
    other = -11;
    BigNum source{-3};
    BigArena::Allocation scratch = temporary.allocate_double();
    const BigArena::Index scratch_index = scratch.index();
    scratch.reset();
    const std::size_t high_water = working.high_water_mark();
    const std::size_t temporary_high_water = temporary.high_water_mark();
    const BigStackMark mark = save_stack();

    square_bn(value, source);

    EXPECT_EQ(&working, &value.arena());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(BigNum{9}, value);
    EXPECT_EQ(BigNum{-11}, other);
    EXPECT_EQ(BigNum{-3}, source);
    const BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(scratch_index, reused.index());
    EXPECT_EQ(temporary_high_water, temporary.high_water_mark());
}

TEST_P(TestBigNumSqrStorage, subtractionAndDoublingPreserveTheOtherCache)
{
    BigNum &value = cache();
    BigNum &other = std::get<1>(GetParam()) ? g_tmp_sqr_x_bn : g_tmp_sqr_y_bn;
    value = 9;
    other = 4;
    const BigNum original_other{other};

    value -= other;
    EXPECT_EQ(BigNum{5}, value);
    EXPECT_EQ(5, bn_to_int(value));
    double_a_bn(value);
    EXPECT_EQ(10, bn_to_int(value));
    EXPECT_EQ(BigNum{10}, value);
    EXPECT_EQ(original_other, other);
}

TEST_P(TestBigNumSqrStorage, repeatedBindingClearsTheCacheWithoutAnotherAllocation)
{
    BigNum &value = cache();
    const BigArena &working = value.arena();
    value = 17;
    const std::size_t high_water = working.high_water_mark();

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(value);
    EXPECT_EQ(&working, &value.arena());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(BigNum{0}, value);
}

TEST_P(TestBigNumSqrStorage, precisionChangesBindAClearedCacheInTheNewArena)
{
    BigNum &value = cache();
    value = -17;
    const int length = std::get<0>(GetParam()) <= 12 ? 64 : 12;

    init_bf_length(length);

    ASSERT_TRUE(value);
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &value.arena());
    EXPECT_EQ(static_cast<std::size_t>(length), value.arena().value_size());
    EXPECT_EQ(BigNum{0}, value);
}

TEST_P(TestBigNumSqrStorage, cleanupUnbindsTheCacheAndReinitializationClearsIt)
{
    BigNum &value = cache();
    value = 17;

    free_bf_vars();
    EXPECT_FALSE(value);
    free_bf_vars();
    EXPECT_FALSE(value);
    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(value);
    EXPECT_EQ(&bn_arena(BigLifetime::WORKING), &value.arena());
    EXPECT_EQ(BigNum{0}, value);
}

TEST_P(TestBigNumSqrStorage, temporaryRewindsPreserveTheCache)
{
    BigNum &value = cache();
    value = -17;
    const BigStackMark mark = save_stack();
    {
        BigStackSaver saved;
        bn_stack().allocate_double();
    }

    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(BigNum{-17}, value);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndComponents, TestBigNumSqrStorage,
    testing::Combine(testing::Values(4, 12, 56, 60, 64, 672), testing::Bool()));

} // namespace id::test
