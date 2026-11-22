// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/divide_brot.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/fractals.h>
#include <engine/LogicalScreen.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/fixed_pt.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <ostream>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

struct DivideBrotInput
{
    std::string name;
    double a;
    double b;
    Bailout bailout;
};

void PrintTo(const DivideBrotInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DivideBrotInput> orbit_inputs()
{
    const std::array<const char *, 7> metrics{"Mod", "Real", "Imag", "Or", "And", "Manh", "Manr"};
    std::vector<DivideBrotInput> inputs;
    for (const double a : {0.0, 2.0, 3.75, 5.0})
    {
        for (const double b : {0.0, 0.5, 2.0})
        {
            for (int metric = 0; metric < static_cast<int>(metrics.size()); ++metric)
            {
                inputs.push_back({"A" + std::to_string(static_cast<int>(a * 4)) + "_B" +
                        std::to_string(static_cast<int>(b * 2)) + "_" + metrics[metric],
                    a, b, static_cast<Bailout>(metric)});
            }
        }
    }
    return inputs;
}

} // namespace

class TestDivideBrot5 : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::DIVIDE_BROT5};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test};
    ValueSaver<decltype(g_bailout_float)> m_fp{g_bailout_float};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bf{g_bailout_bigfloat};
    ValueSaver<double> m_limit{g_magnitude_limit, 4};
    ValueSaver<double> m_magnitude{g_magnitude};
    ValueSaver<double> m_a{g_params[0], 0};
    ValueSaver<double> m_b{g_params[1], 0};
    ValueSaver<int> m_exponent{g_c_exponent};
    ValueSaver<LDouble> m_b_const{g_b_const};
    ValueSaver<CalcMode> m_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen, LogicalScreen{257, 513, 0, 0, 256, 512}};
    ValueSaver<int> m_col{g_col, 0};
    ValueSaver<int> m_row{g_row, 0};
    ValueSaver<int> m_periodicity{g_periodicity_check, 0};
    ValueSaver<long> m_iteration{g_color_iter, 37};
    ValueSaver<bool> m_overflow{g_overflow, false};
    ValueSaver<DComplex> m_init{g_init};
    ValueSaver<DComplex> m_old{g_old_z};
    ValueSaver<DComplex> m_new{g_new_z};
    ValueSaver<double> m_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_sqr_y{g_temp_sqr_y};
};

void TestDivideBrot5::SetUp()
{
    init_bf_dec(60);
    set_bailout_formula(Bailout::MOD);
    g_bf_x_min = -2;
    g_bf_x_max = 2;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_x_3rd = -1.5;
    g_bf_y_3rd = -0.75;
}

void TestDivideBrot5::TearDown()
{
    free_bf_vars();
}

class TestDivideBrot5Orbit : public TestDivideBrot5, public testing::WithParamInterface<DivideBrotInput>
{
};

TEST_P(TestDivideBrot5Orbit, retainsSetupFormulaAndSelectedBailout)
{
    const auto &input = GetParam();
    g_params[0] = input.a;
    g_params[1] = input.b;
    set_bailout_formula(input.bailout);
    g_magnitude_limit = 1;
    ASSERT_TRUE(mandel_per_image_bf());
    EXPECT_EQ(2 - static_cast<int>(input.a), g_c_exponent);
    EXPECT_EQ(input.b + 1e-20, g_b_const);
    const std::complex<double> seed{0.75, 0.25};
    const std::complex<double> parameter{0.125, -0.0625};
    g_old_z_bf.x = seed.real();
    g_old_z_bf.y = seed.imag();
    g_param_z_bf.x = parameter.real();
    g_param_z_bf.y = parameter.imag();
    g_tmp_sqr_x_bf = g_old_z_bf.x.sqr();
    g_tmp_sqr_y_bf = g_old_z_bf.y.sqr();
    const std::complex<double> expected =
        seed * seed / (std::pow(seed, g_c_exponent) + static_cast<double>(g_b_const)) + parameter;
    const double tolerance = 1e-11 * std::max(1.0, std::abs(expected));
    const bool escaped = bailout_reached(input.bailout, expected.real(), expected.imag(), g_magnitude_limit, true);

    EXPECT_EQ(escaped, divide_brot5_orbit_bf() != 0);

    EXPECT_NEAR(expected.real(), static_cast<double>(g_new_z_bf.x.to_float()), tolerance);
    EXPECT_NEAR(expected.imag(), static_cast<double>(g_new_z_bf.y.to_float()), tolerance);
    EXPECT_FALSE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDivideBrot5Orbit, testing::ValuesIn(orbit_inputs()),
    [](const testing::TestParamInfo<DivideBrotInput> &info) { return info.param.name; });

TEST_F(TestDivideBrot5, affinePixelCoordinatesClearTheSeedWithoutCountingAnIteration)
{
    ASSERT_TRUE(mandel_per_image_bf());
    const int alternate = find_alternate_math(FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT);
    ASSERT_NE(-1, alternate);
    EXPECT_EQ(divide_brot5_orbit_bf, g_alternate_math[alternate].orbit_calc);
    EXPECT_EQ(divide_brot5_bf_per_pixel, g_alternate_math[alternate].per_pixel);
    EXPECT_EQ(mandel_per_image_bf, g_alternate_math[alternate].per_image);
    const FractalSpecific *specific = get_fractal_specific(FractalType::DIVIDE_BROT5);
    EXPECT_EQ(nullptr, specific->pert_ref);
    EXPECT_EQ(nullptr, specific->pert_ref_bf);
    EXPECT_EQ(nullptr, specific->pert_pt);
    for (const int col : {0, 128, 256})
    {
        for (const int row : {0, 256, 512})
        {
            SCOPED_TRACE(col);
            SCOPED_TRACE(row);
            g_col = col;
            g_row = row;
            g_old_z_bf.x = 7;
            g_old_z_bf.y = -9;
            g_tmp_sqr_x_bf = 49;
            g_tmp_sqr_y_bf = 81;
            const double real = -2 + col * (7.0 / 512) + row * (1.0 / 1024);
            const double imag = 1 - (row * (7.0 / 2048) + col * (1.0 / 1024));

            EXPECT_EQ(0, divide_brot5_bf_per_pixel());

            EXPECT_EQ(37, g_color_iter);
            EXPECT_NEAR(real, static_cast<double>(g_param_z_bf.x.to_float()), 1e-14);
            EXPECT_NEAR(imag, static_cast<double>(g_param_z_bf.y.to_float()), 1e-14);
            EXPECT_EQ(BigFloat{0}, g_old_z_bf.x);
            EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);
            EXPECT_EQ(BigFloat{0}, g_tmp_sqr_x_bf);
            EXPECT_EQ(BigFloat{0}, g_tmp_sqr_y_bf);
        }
    }
}

TEST_F(TestDivideBrot5, pixelCoordinatesRetainSubDoubleDisplacements)
{
    g_bf_x_min = BigFloat{"0.75"};
    g_bf_x_3rd = BigFloat{"0.75"};
    g_bf_x_max = BigFloat{"0.7500000000000000000000000000000000000256"};
    g_bf_y_min = 0.25;
    g_bf_y_max = 0.25;
    g_bf_y_3rd = 0.25;
    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(0, divide_brot5_bf_per_pixel());
    const BigFloat first = g_param_z_bf.x;
    g_col = 1;

    ASSERT_EQ(0, divide_brot5_bf_per_pixel());

    BigFloat displacement = g_param_z_bf.x - first;
    EXPECT_NEAR(1e-40, static_cast<double>(displacement.to_float()), 1e-52);
    EXPECT_GT(g_param_z_bf.x, first);
}

TEST_F(TestDivideBrot5, zeroDenominatorsSetOverflowBeforeTheFinalParameterAdditionAndBailout)
{
    g_params[0] = 2;
    g_bailout_bigfloat = +[]
    {
        EXPECT_TRUE(g_overflow);
        EXPECT_NEAR(0.125, static_cast<double>(g_new_z_bf.x.to_float()), 1e-14);
        EXPECT_NEAR(-0.25, static_cast<double>(g_new_z_bf.y.to_float()), 1e-14);
        return 17;
    };
    for (const bool zero_seed : {false, true})
    {
        SCOPED_TRACE(zero_seed);
        g_params[1] = zero_seed ? -1e-20 : -1;
        ASSERT_TRUE(mandel_per_image_bf());
        g_old_z_bf.x = zero_seed ? 0 : 1;
        g_old_z_bf.y = 0;
        g_tmp_sqr_x_bf = zero_seed ? 0 : 1;
        g_tmp_sqr_y_bf = 0;
        g_param_z_bf.x = 0.125;
        g_param_z_bf.y = -0.25;
        g_overflow = false;

        EXPECT_EQ(17, divide_brot5_orbit_bf());
    }
}

TEST_F(TestDivideBrot5, escapeCountsMatchTheOrdinaryCallback)
{
    for (const double a : {0.0, 2.0, 3.75, 5.0})
    {
        for (const double b : {0.0, 0.5, 2.0})
        {
            SCOPED_TRACE(a);
            SCOPED_TRACE(b);
            g_params[0] = a;
            g_params[1] = b;
            ASSERT_TRUE(mandel_per_image_bf());
            g_init = {1.25, 0.5};
            g_old_z = {};
            g_temp_sqr_x = g_temp_sqr_y = 0;
            g_old_z_bf.x = g_old_z_bf.y = 0;
            g_tmp_sqr_x_bf = g_tmp_sqr_y_bf = 0;
            g_param_z_bf.x = g_init.x;
            g_param_z_bf.y = g_init.y;
            int ordinary_steps = 0;
            bool ordinary_escaped = false;
            while (!ordinary_escaped && ordinary_steps < 16)
            {
                ordinary_escaped = divide_brot5_orbit() != 0;
                ++ordinary_steps;
            }
            int big_float_steps = 0;
            bool big_float_escaped = false;
            while (!big_float_escaped && big_float_steps < 16)
            {
                big_float_escaped = divide_brot5_orbit_bf() != 0;
                ++big_float_steps;
            }
            EXPECT_EQ(ordinary_escaped, big_float_escaped);
            EXPECT_EQ(ordinary_steps, big_float_steps);
        }
    }
}

} // namespace id::test
