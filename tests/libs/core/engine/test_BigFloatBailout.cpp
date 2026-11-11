// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/fractals.h>
#include <engine/LogicalScreen.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>
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

std::vector<Byte> encoded(const BigFloat &value)
{
    return {value.data(), value.data() + g_bf_length + 2};
}

std::vector<Byte> encoded(const LDouble value)
{
    std::vector<Byte> result(g_bf_length + 2);
    float_to_bf(result.data(), value);
    return result;
}

struct JuliaZPowerInput
{
    const char *name;
    std::complex<double> exponent;
    SymmetryType symmetry;
};

void PrintTo(const JuliaZPowerInput &input, std::ostream *out)
{
    *out << input.name;
}

const std::array<JuliaZPowerInput, 8> JULIA_Z_POWERS{
    {{"Even", {2, 0}, SymmetryType::ORIGIN}, {"Odd", {3, 0}, SymmetryType::NONE},
        {"NegativeEven", {-2, 0}, SymmetryType::ORIGIN}, {"NegativeOdd", {-3, 0}, SymmetryType::NONE},
        {"Fractional", {0.5, 0}, SymmetryType::NONE}, {"Complex", {0.5, 0.25}, SymmetryType::NONE},
        {"Imaginary", {0, -0.25}, SymmetryType::NONE}, {"Zero", {0, 0}, SymmetryType::ORIGIN}}};

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
    static_assert(std::is_same_v<decltype(g_param_z_bf), Complex<BigFloat>>);
    const std::array<BigFloat *, 16> values{&g_old_z_bf.x, &g_old_z_bf.y, &g_new_z_bf.x, &g_new_z_bf.y, &g_saved_z_bf.x,
        &g_saved_z_bf.y, &g_close_enough_bf, &g_tmp_sqr_x_bf, &g_tmp_sqr_y_bf, &g_bf_tmp, &g_delta_x_bf, &g_delta_y_bf,
        &g_delta2_x_bf, &g_delta2_y_bf, &g_param_z_bf.x, &g_param_z_bf.y};
    std::array<BigFloatConstPtr, 16> buffers;
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
    std::vector<Byte> expected_x(g_r_bf_length + 2);
    std::vector<Byte> expected_y(g_r_bf_length + 2);
    square_bf(expected_x.data(), g_new_z_bf.x.data());
    square_bf(expected_y.data(), g_new_z_bf.y.data());
    const auto marker = save_stack();

    EXPECT_EQ(escaped, g_bailout_bigfloat() != 0);

    EXPECT_EQ(marker, save_stack());
    const auto slice = [](const std::vector<Byte> &v)
    { return std::vector<Byte>{v.data(), v.data() + g_bf_length + 2}; };
    EXPECT_EQ(slice(expected_x), encoded(g_tmp_sqr_x_bf));
    EXPECT_EQ(slice(expected_y), encoded(g_tmp_sqr_y_bf));
    EXPECT_EQ(encoded(escaped ? ((manh || manr) ? input.metric : -7) : input.point.x), encoded(g_old_z_bf.x));
    EXPECT_EQ(encoded(escaped ? (manh ? std::abs(input.point.y) : 9) : input.point.y), encoded(g_old_z_bf.y));
    EXPECT_EQ(encoded(input.point.x), encoded(g_new_z_bf.x));
    EXPECT_EQ(encoded(input.point.y), encoded(g_new_z_bf.y));
    if (manh || manr || input.test == Bailout::MOD)
    {
        const double sum = manh ? std::abs(input.point.x) + std::abs(input.point.y) : input.point.x + input.point.y;
        EXPECT_EQ(encoded(input.test == Bailout::MOD ? input.metric : sum), encoded(g_bf_tmp));
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
            const auto original_x = encoded(g_new_z_bf.x);
            const auto original_y = encoded(g_new_z_bf.y);

            EXPECT_EQ(point == 2, g_bailout_bigfloat() != 0);

            EXPECT_EQ(original_x, encoded(g_new_z_bf.x));
            EXPECT_EQ(original_y, encoded(g_new_z_bf.y));
        }
    }
}

class TestBigFloatJulia : public TestBigFloatBailout
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

void TestBigFloatJulia::SetUp()
{
    TestBigFloatBailout::SetUp();
    g_fractal_type = FractalType::JULIA;
    set_bailout_formula(Bailout::MOD);
}

TEST_F(TestBigFloatJulia, affineSeedsKeepPreciseFixedParameterAndSquareCaches)
{
    float_to_bf(g_bf_x_min.data(), -2);
    float_to_bf(g_bf_x_max.data(), 2);
    float_to_bf(g_bf_y_min.data(), -1);
    float_to_bf(g_bf_y_max.data(), 1);
    float_to_bf(g_bf_x_3rd.data(), -1.5);
    float_to_bf(g_bf_y_3rd.data(), -0.75);
    str_to_bf(g_bf_params[0].data(), "0.25000000000000000001");
    str_to_bf(g_bf_params[1].data(), "-0.12500000000000000001");
    const auto parameter_x = encoded(g_bf_params[0]);
    const auto parameter_y = encoded(g_bf_params[1]);
    // These bounded reference values use double, including scalar conversion.
    constexpr double tolerance = 64 * std::numeric_limits<double>::epsilon();
    g_magnitude_limit = 1024;

    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    EXPECT_NE(encoded(0.25), encoded(g_param_z_bf.x));
    EXPECT_NE(encoded(-0.125), encoded(g_param_z_bf.y));
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

            EXPECT_EQ(1, julia_per_pixel_bf());

            EXPECT_EQ(37, g_color_iter);
            EXPECT_NEAR(real, static_cast<double>(bf_to_float(g_old_z_bf.x.data())), tolerance);
            EXPECT_NEAR(imag, static_cast<double>(bf_to_float(g_old_z_bf.y.data())), tolerance);
            EXPECT_NEAR(real, static_cast<double>(bf_to_float(g_new_z_bf.x.data())), tolerance);
            EXPECT_NEAR(imag, static_cast<double>(bf_to_float(g_new_z_bf.y.data())), tolerance);
            EXPECT_NEAR(real * real, static_cast<double>(bf_to_float(g_tmp_sqr_x_bf.data())), tolerance);
            EXPECT_NEAR(imag * imag, static_cast<double>(bf_to_float(g_tmp_sqr_y_bf.data())), tolerance);
            EXPECT_EQ(parameter_x, encoded(g_param_z_bf.x));
            EXPECT_EQ(parameter_y, encoded(g_param_z_bf.y));

            EXPECT_EQ(0, julia_orbit_bf());

            EXPECT_NEAR(
                real * real - imag * imag + 0.25, static_cast<double>(bf_to_float(g_new_z_bf.x.data())), tolerance);
            EXPECT_NEAR(2 * real * imag - 0.125, static_cast<double>(bf_to_float(g_new_z_bf.y.data())), tolerance);
            EXPECT_EQ(parameter_x, encoded(g_param_z_bf.x));
            EXPECT_EQ(parameter_y, encoded(g_param_z_bf.y));
        }
    }
}

TEST_F(TestBigFloatJulia, viewportTransitionRestoresJuliaParameterAndKeepsStrictBailout)
{
    float_to_bf(g_bf_x_min.data(), 1);
    float_to_bf(g_bf_x_max.data(), 3);
    float_to_bf(g_bf_x_3rd.data(), 1);
    float_to_bf(g_bf_y_min.data(), -1);
    float_to_bf(g_bf_y_max.data(), 1);
    float_to_bf(g_bf_y_3rd.data(), -1);
    clear_bf(g_bf_params[0].data());
    clear_bf(g_bf_params[1].data());
    g_col = 128;
    g_row = 256;
    g_magnitude_limit = 16;
    g_fractal_type = FractalType::MANDEL;
    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(1, mandel_per_pixel_bf());
    ASSERT_EQ(BigFloat{2}, g_param_z_bf.x);

    g_fractal_type = FractalType::JULIA;
    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(BigFloat{0}, g_param_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_param_z_bf.y);
    ASSERT_EQ(1, julia_per_pixel_bf());
    EXPECT_EQ(37, g_color_iter);
    EXPECT_EQ(BigFloat{2}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(0, julia_orbit_bf());

    EXPECT_EQ(BigFloat{4}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    EXPECT_EQ(BigFloat{4}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(1, julia_orbit_bf());

    EXPECT_EQ(BigFloat{16}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    EXPECT_EQ(BigFloat{4}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);
}

class TestBigFloatJuliaZPower : public TestBigFloatJulia, public testing::WithParamInterface<JuliaZPowerInput>
{
protected:
    void SetUp() override;

    ValueSaver<double> m_power_real{g_params[2]};
    ValueSaver<double> m_power_imaginary{g_params[3]};
    ValueSaver<SymmetryType> m_symmetry{g_symmetry, SymmetryType::ORIGIN};
    ValueSaver<FractalDispatch> m_dispatch{g_dispatch};
    ValueSaver<const FractalSpecific *> m_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA_Z_POWER)};
};

void TestBigFloatJuliaZPower::SetUp()
{
    TestBigFloatJulia::SetUp();
    g_fractal_type = FractalType::JULIA_Z_POWER;
    g_params[2] = GetParam().exponent.real();
    g_params[3] = GetParam().exponent.imag();
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    ASSERT_TRUE(select_alternate_math_dispatch());
    float_to_bf(g_bf_x_min.data(), 0);
    float_to_bf(g_bf_x_max.data(), 1);
    float_to_bf(g_bf_x_3rd.data(), 0);
    float_to_bf(g_bf_y_min.data(), 0);
    float_to_bf(g_bf_y_max.data(), 0.5);
    float_to_bf(g_bf_y_3rd.data(), 0);
    str_to_bf(g_bf_params[0].data(), "0.12500000000000000001");
    str_to_bf(g_bf_params[1].data(), "-0.06250000000000000001");
    g_magnitude_limit = 1024;
}

TEST_P(TestBigFloatJuliaZPower, setupSeedsAndOrbitKeepTheFixedParameterAndExponentRules)
{
    const auto parameter_x = encoded(g_bf_params[0]);
    const auto parameter_y = encoded(g_bf_params[1]);
    EXPECT_EQ(mandel_per_image_bf, g_dispatch.per_image());
    EXPECT_EQ(julia_per_pixel_bf, g_dispatch.per_pixel());
    EXPECT_EQ(julia_z_power_orbit_bf, g_dispatch.orbit_calc());
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_ref);
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_ref_bf);
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_pt);

    ASSERT_TRUE(g_dispatch.per_image()());

    EXPECT_EQ(static_cast<int>(GetParam().exponent.real()), g_c_exponent);
    EXPECT_EQ(GetParam().symmetry, g_symmetry);
    EXPECT_EQ(parameter_x, encoded(g_param_z_bf.x));
    EXPECT_EQ(parameter_y, encoded(g_param_z_bf.y));
    g_col = 128;
    g_row = 256;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    EXPECT_EQ(37, g_color_iter);
    EXPECT_NEAR(0.5, static_cast<double>(bf_to_float(g_old_z_bf.x.data())), 1e-14);
    EXPECT_NEAR(0.25, static_cast<double>(bf_to_float(g_old_z_bf.y.data())), 1e-14);
    const std::complex<double> expected =
        std::pow(std::complex<double>{0.5, 0.25}, GetParam().exponent) + std::complex<double>{0.125, -0.0625};
    const double tolerance = 1e-12 * std::max(1.0, std::abs(expected));

    EXPECT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_NEAR(expected.real(), static_cast<double>(bf_to_float(g_new_z_bf.x.data())), tolerance);
    EXPECT_NEAR(expected.imag(), static_cast<double>(bf_to_float(g_new_z_bf.y.data())), tolerance);
    EXPECT_EQ(parameter_x, encoded(g_param_z_bf.x));
    EXPECT_EQ(parameter_y, encoded(g_param_z_bf.y));
}

TEST_P(TestBigFloatJuliaZPower, zeroSeedRetainsTheLegacyPowerConvention)
{
    ASSERT_TRUE(g_dispatch.per_image()());
    g_col = 0;
    g_row = 512;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_NEAR(0.125, static_cast<double>(bf_to_float(g_new_z_bf.x.data())), 1e-14);
    EXPECT_NEAR(-0.0625, static_cast<double>(bf_to_float(g_new_z_bf.y.data())), 1e-14);
}

TEST_P(TestBigFloatJuliaZPower, unitSeedKeepsTheStrictBailoutBoundary)
{
    clear_bf(g_bf_params[0].data());
    clear_bf(g_bf_params[1].data());
    ASSERT_TRUE(g_dispatch.per_image()());
    g_col = 256;
    g_row = 512;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    ASSERT_EQ(BigFloat{1}, g_old_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.y);
    g_magnitude_limit = 1;

    EXPECT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_EQ(BigFloat{1}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    g_magnitude_limit = 0.5;

    EXPECT_EQ(1, g_dispatch.orbit_calc()());
}

INSTANTIATE_TEST_SUITE_P(Exponents, TestBigFloatJuliaZPower, testing::ValuesIn(JULIA_Z_POWERS),
    [](const testing::TestParamInfo<JuliaZPowerInput> &info) { return info.param.name; });

} // namespace id::test
