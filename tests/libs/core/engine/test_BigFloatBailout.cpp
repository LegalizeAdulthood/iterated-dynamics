// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

struct BigFloatBailoutInput
{
    std::string name;
    Bailout test;
    DComplex point;
    double metric;
    double limit;
};

void PrintTo(const BigFloatBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BigFloatBailoutInput> inputs()
{
    std::vector<BigFloatBailoutInput> result;
    const std::array<const char *, 7> names{"Mod", "Real", "Imag", "Or", "And", "Manh", "Manr"};
    const std::array<DComplex, 4> points{{{1.5, -1}, {-0.5, 1.25}, {0, 0}, {-0.5, -1.25}}};
    const std::array<std::array<double, 7>, 4> metrics{
        {{3.25, 2.25, 1, 2.25, 1, 6.25, 0.25}, {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 0.5625},
            {0, 0, 0, 0, 0, 0, 0}, {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 3.0625}}};
    for (int point = 0; point < 4; ++point)
    {
        for (int test = 0; test < 7; ++test)
        {
            for (int offset = -1; offset <= 1; ++offset)
            {
                const double metric = metrics[point][test];
                result.push_back(
                    {std::string{names[test]} + "_" + std::to_string(point) + "_" + std::to_string(offset + 1),
                        static_cast<Bailout>(test), points[point], metric, metric + offset * 0.125});
            }
        }
    }
    return result;
}

std::vector<Byte> encoded(BigFloatConstPtr value)
{
    return {value, value + g_bf_length + 2};
}

std::vector<Byte> encoded(const LDouble value)
{
    std::vector<Byte> result(g_bf_length + 2);
    float_to_bf(result.data(), value);
    return result;
}

} // namespace

class TestBigFloatBailout : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test, Bailout::MOD};
    ValueSaver<double> m_limit{g_magnitude_limit};
    ValueSaver<double> m_magnitude{g_magnitude, -17};
    ValueSaver<decltype(g_bailout_float)> m_fp{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_bn{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bf{g_bailout_bigfloat};
};

void TestBigFloatBailout::SetUp()
{
    init_bf_dec(40);
}

void TestBigFloatBailout::TearDown()
{
    free_bf_vars();
}

class TestBigFloatBailoutCases : public TestBigFloatBailout, public testing::WithParamInterface<BigFloatBailoutInput>
{
};

TEST_F(TestBigFloatBailout, canonicalGlobalsOwnWorkingStorageThroughBindingAndCleanup)
{
    static_assert(std::is_same_v<decltype(g_old_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_new_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_saved_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_close_enough_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_tmp_sqr_x_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_tmp_sqr_y_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp), BigFloat>);
    const std::array<BigFloat *, 10> values{&g_old_z_bf.x, &g_old_z_bf.y, &g_new_z_bf.x, &g_new_z_bf.y, &g_saved_z_bf.x,
        &g_saved_z_bf.y, &g_close_enough_bf, &g_tmp_sqr_x_bf, &g_tmp_sqr_y_bf, &g_bf_tmp};
    std::array<BigFloatConstPtr, 10> buffers;
    auto &working = bf_arena(BigLifetime::WORKING);
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        auto &value = *values[i];
        ASSERT_TRUE(value);
        EXPECT_EQ(&working, &value.arena());
        EXPECT_EQ(BigFloat{0}, value);
        value = -1.25L;
        buffers[i] = value.data();
    }

    const auto high_water = working.high_water_mark();
    init_bf_dec(40);
    EXPECT_EQ(high_water, working.high_water_mark());
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(buffers[i], values[i]->data());
        EXPECT_EQ(BigFloat{0}, *values[i]);
    }
    init_bf_dec(80);
    for (const auto *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
    }
    free_bf_vars();
    for (const auto *value : values)
    {
        EXPECT_FALSE(*value);
    }
    init_bf_dec(40);
    for (const auto *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
    }
}

TEST_P(TestBigFloatBailoutCases, preservesStrictThresholdCachesAndOldOrbit)
{
    const auto &input = GetParam();
    const bool escaped = input.metric > input.limit;
    const bool manh = input.test == Bailout::MANH;
    const bool manr = input.test == Bailout::MANR;
    set_bailout_formula(input.test);
    g_magnitude_limit = input.limit;
    float_to_bf(g_old_z_bf.x.data(), -7);
    float_to_bf(g_old_z_bf.y.data(), 9);
    float_to_bf(g_new_z_bf.x.data(), input.point.x);
    float_to_bf(g_new_z_bf.y.data(), input.point.y);
    std::vector<Byte> expected_x(g_r_bf_length + 2), expected_y(g_r_bf_length + 2);
    square_bf(expected_x.data(), g_new_z_bf.x.data());
    square_bf(expected_y.data(), g_new_z_bf.y.data());
    const auto marker = save_stack();

    EXPECT_EQ(escaped, g_bailout_bigfloat() != 0);

    EXPECT_EQ(marker, save_stack());
    EXPECT_EQ(encoded(expected_x.data()), encoded(g_tmp_sqr_x_bf.data()));
    EXPECT_EQ(encoded(expected_y.data()), encoded(g_tmp_sqr_y_bf.data()));
    EXPECT_EQ(encoded(escaped ? ((manh || manr) ? input.metric : -7) : input.point.x), encoded(g_old_z_bf.x.data()));
    EXPECT_EQ(encoded(escaped ? (manh ? std::abs(input.point.y) : 9) : input.point.y), encoded(g_old_z_bf.y.data()));
    EXPECT_EQ(encoded(input.point.x), encoded(g_new_z_bf.x.data()));
    EXPECT_EQ(encoded(input.point.y), encoded(g_new_z_bf.y.data()));
    if (manh || manr || input.test == Bailout::MOD)
    {
        const double sum = manh ? std::abs(input.point.x) + std::abs(input.point.y) : input.point.x + input.point.y;
        EXPECT_EQ(encoded(input.test == Bailout::MOD ? input.metric : sum), encoded(g_bf_tmp.data()));
    }
    EXPECT_EQ(-17, g_magnitude);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatBailoutCases, testing::ValuesIn(inputs()),
    [](const testing::TestParamInfo<BigFloatBailoutInput> &info) { return info.param.name; });

TEST_F(TestBigFloatBailout, strictComparisonRetainsDisplacementsBelowMachinePrecision)
{
    const std::array<double, 7> limits{0.5, 0.25, 0.25, 0.25, 0.25, 1, 1};
    const std::array<const char *, 3> values{
        "0.4999999999999999999999999999999", "0.5", "0.5000000000000000000000000000001"};
    for (int test = 0; test < 7; ++test)
    {
        SCOPED_TRACE(test);
        set_bailout_formula(static_cast<Bailout>(test));
        g_magnitude_limit = limits[test];
        for (int point = 0; point < 3; ++point)
        {
            SCOPED_TRACE(point);
            str_to_bf(g_new_z_bf.x.data(), values[point]);
            str_to_bf(g_new_z_bf.y.data(), values[point]);
            const auto original_x = encoded(g_new_z_bf.x.data());
            const auto original_y = encoded(g_new_z_bf.y.data());

            EXPECT_EQ(point == 2, g_bailout_bigfloat() != 0);

            EXPECT_EQ(original_x, encoded(g_new_z_bf.x.data()));
            EXPECT_EQ(original_y, encoded(g_new_z_bf.y.data()));
        }
    }
}

} // namespace id::test
