// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/fractalb.h>
#include <engine/LogicalScreen.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigNum.h>
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
    const std::array<DComplex, 3> points{{{1.5, -1}, {-0.5, 1.25}, {0, 0}}};
    const std::array<std::array<double, 7>, 3> metrics{{{3.25, 2.25, 1, 2.25, 1, 6.25, 0.25},
        {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 0.5625}, {0, 0, 0, 0, 0, 0, 0}}};
    for (int point = 0; point < 3; ++point)
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
    const std::array<BigNum *, 17> values{&g_new_z_bn.x, &g_new_z_bn.y, &g_saved_z_bn.x, &g_saved_z_bn.y,
        &g_close_enough_bn, &g_x_min_bn, &g_x_max_bn, &g_y_min_bn, &g_y_max_bn, &g_x_3rd_bn, &g_y_3rd_bn, &g_delta_x_bn,
        &g_delta_y_bn, &g_delta2_x_bn, &g_delta2_y_bn, &g_param_z_bn.x, &g_param_z_bn.y};
    auto &working = bn_arena(BigLifetime::WORKING);
    for (auto *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigNum{0}, *value);
        *value = -1.25L;
    }

    std::array<BigNumConstPtr, 17> buffers;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        buffers[i] = values[i]->data();
    }
    const auto high_water = working.high_water_mark();
    init_bf_dec(40);
    EXPECT_EQ(high_water, working.high_water_mark());
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(buffers[i], values[i]->data());
        EXPECT_EQ(BigNum{0}, *values[i]);
    }
    init_bf_dec(80);
    for (auto *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigNum{0}, *value);
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
        EXPECT_EQ(BigNum{0}, *value);
    }
}

TEST_P(TestBigNumBailoutCases, preservesThresholdExtractionCachesAndOldOrbit)
{
    const auto &input = GetParam();
    const bool escaped = static_cast<long>(input.metric) >= static_cast<long>(input.limit);
    const bool manh = input.test == Bailout::MANH;
    const bool manr = input.test == Bailout::MANR;
    set_bailout_formula(input.test);
    g_magnitude_limit = input.limit;
    float_to_bn(g_old_z_bn.x, -7);
    float_to_bn(g_old_z_bn.y, 9);
    float_to_bn(g_new_z_bn.x.data(), input.point.x);
    float_to_bn(g_new_z_bn.y.data(), input.point.y);
    std::vector<Byte> expected_x(g_r_length), expected_y(g_r_length), expected_old(g_r_length);
    square_bn(expected_x.data(), g_new_z_bn.x.data());
    square_bn(expected_y.data(), g_new_z_bn.y.data());
    if (escaped && (manh || manr))
    {
        float_to_bn(g_bn_tmp, manh ? std::abs(input.point.x) + std::abs(input.point.y) : input.point.x + input.point.y);
        square_bn(expected_old.data(), g_bn_tmp);
    }
    else
    {
        float_to_bn(expected_old.data(), escaped ? -7 : input.point.x);
    }

    EXPECT_EQ(escaped, g_bailout_bignum() != 0);

    EXPECT_EQ(expected_x, std::vector<Byte>(g_tmp_sqr_x_bn, g_tmp_sqr_x_bn + g_r_length));
    EXPECT_EQ(expected_y, std::vector<Byte>(g_tmp_sqr_y_bn, g_tmp_sqr_y_bn + g_r_length));
    const int old_size = escaped && (manh || manr) ? g_r_length : g_bn_length;
    EXPECT_EQ(std::vector<Byte>(expected_old.begin(), expected_old.begin() + old_size),
        std::vector<Byte>(g_old_z_bn.x, g_old_z_bn.x + old_size));
    EXPECT_EQ(escaped ? (manh ? std::abs(input.point.y) : 9) : input.point.y, bn_to_float(g_old_z_bn.y));
    EXPECT_EQ(input.point.x, bn_to_float(g_new_z_bn.x.data()));
    EXPECT_EQ(input.point.y, bn_to_float(g_new_z_bn.y.data()));
    EXPECT_EQ(-17, g_magnitude);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigNumBailoutCases, testing::ValuesIn(inputs()),
    [](const testing::TestParamInfo<BigNumBailoutInput> &info) { return info.param.name; });

TEST_F(TestBigNumBailout, retainsWideProductBytesBeyondMachinePrecision)
{
    for (int test = 0; test < 7; ++test)
    {
        SCOPED_TRACE(test);
        set_bailout_formula(static_cast<Bailout>(test));
        g_magnitude_limit = 0;
        str_to_bn(g_new_z_bn.x.data(), "0.123456789012345678901234567890123456789");
        str_to_bn(g_new_z_bn.y.data(), "-0.345678901234567890123456789012345678901");
        std::vector<Byte> expected_x(g_r_length), expected_y(g_r_length);
        square_bn(expected_x.data(), g_new_z_bn.x.data());
        square_bn(expected_y.data(), g_new_z_bn.y.data());
        const std::vector<Byte> original_x(g_new_z_bn.x.data(), g_new_z_bn.x.data() + g_bn_length);
        const std::vector<Byte> original_y(g_new_z_bn.y.data(), g_new_z_bn.y.data() + g_bn_length);

        EXPECT_EQ(1, g_bailout_bignum());

        EXPECT_EQ(expected_x, std::vector<Byte>(g_tmp_sqr_x_bn, g_tmp_sqr_x_bn + g_r_length));
        EXPECT_EQ(expected_y, std::vector<Byte>(g_tmp_sqr_y_bn, g_tmp_sqr_y_bn + g_r_length));
        EXPECT_EQ(original_x, std::vector<Byte>(g_new_z_bn.x.data(), g_new_z_bn.x.data() + g_bn_length));
        EXPECT_EQ(original_y, std::vector<Byte>(g_new_z_bn.y.data(), g_new_z_bn.y.data() + g_bn_length));
    }
}

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
    std::vector<Byte> parameter_x(g_bn_length), parameter_y(g_bn_length);
    bf_to_bn(parameter_x.data(), g_bf_params[0]);
    bf_to_bn(parameter_y.data(), g_bf_params[1]);
    g_magnitude_limit = 1024;

    ASSERT_TRUE(mandel_per_image_bn());
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
            EXPECT_DOUBLE_EQ(real, static_cast<double>(bn_to_float(g_new_z_bn.x.data())));
            EXPECT_DOUBLE_EQ(imag, static_cast<double>(bn_to_float(g_new_z_bn.y.data())));
            EXPECT_DOUBLE_EQ(real * real, static_cast<double>(bn_to_float(g_tmp_sqr_x_bn + g_shift_factor)));
            EXPECT_DOUBLE_EQ(imag * imag, static_cast<double>(bn_to_float(g_tmp_sqr_y_bn + g_shift_factor)));
            EXPECT_EQ(parameter_x, std::vector<Byte>(g_param_z_bn.x.data(), g_param_z_bn.x.data() + g_bn_length));
            EXPECT_EQ(parameter_y, std::vector<Byte>(g_param_z_bn.y.data(), g_param_z_bn.y.data() + g_bn_length));

            EXPECT_EQ(0, julia_orbit_bn());

            EXPECT_EQ(BigNum{real * real - imag * imag} + g_param_z_bn.x, g_new_z_bn.x);
            EXPECT_EQ(BigNum{2 * real * imag} + g_param_z_bn.y, g_new_z_bn.y);
            EXPECT_EQ(parameter_x, std::vector<Byte>(g_param_z_bn.x.data(), g_param_z_bn.x.data() + g_bn_length));
            EXPECT_EQ(parameter_y, std::vector<Byte>(g_param_z_bn.y.data(), g_param_z_bn.y.data() + g_bn_length));
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

} // namespace id::test
