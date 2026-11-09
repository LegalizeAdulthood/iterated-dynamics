// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/burning_ship.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
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

struct ShipPointInput
{
    std::string name;
    int degree;
    std::complex<double> point;
};

void PrintTo(const ShipPointInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ShipPointInput> point_inputs()
{
    const std::array<std::complex<double>, 11> points{{{0.5, 0.25}, {-0.5, 0.25}, {0.5, -0.25}, {-0.5, -0.25},
        {0, 0.25}, {0, -0.25}, {0.5, 0}, {-0.5, 0}, {0, 0}, {1e-20, -0.25}, {-1e-20, -0.25}}};
    std::vector<ShipPointInput> inputs;
    for (const int degree : {2, 3, 4, 5})
    {
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            inputs.push_back({"Degree" + std::to_string(degree) + "_Point" + std::to_string(i), degree, points[i]});
        }
    }
    return inputs;
}

std::vector<Byte> encoded(BigFloatConstPtr value)
{
    return {value, value + g_bf_length + 2};
}

} // namespace

class TestBurningShipValues : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::BURNING_SHIP};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test};
    ValueSaver<decltype(g_bailout_float)> m_fp{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_bn{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bf{g_bailout_bigfloat};
    ValueSaver<double> m_degree{g_params[2]};
    ValueSaver<double> m_limit{g_magnitude_limit, 4};
    ValueSaver<double> m_magnitude{g_magnitude};
    ValueSaver<DComplex> m_init{g_init};
    ValueSaver<DComplex> m_old{g_old_z};
    ValueSaver<DComplex> m_new{g_new_z};
    ValueSaver<double> m_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_sqr_y{g_temp_sqr_y};
};

void TestBurningShipValues::SetUp()
{
    init_bf_dec(60);
    set_bailout_formula(Bailout::MOD);
}

void TestBurningShipValues::TearDown()
{
    free_bf_vars();
}

class TestBurningShipPoint : public TestBurningShipValues, public testing::WithParamInterface<ShipPointInput>
{
};

TEST_P(TestBurningShipPoint, nativeOrbitRetainsAbsoluteValuesAndParameterSigns)
{
    const auto &input = GetParam();
    g_params[2] = input.degree;
    const std::complex<double> parameter{0.125, -0.0625};
    g_old_z_bf.x = input.point.real();
    g_old_z_bf.y = input.point.imag();
    g_param_z_bf.x = parameter.real();
    g_param_z_bf.y = parameter.imag();
    const auto original_x = encoded(g_old_z_bf.x.data());
    const auto original_y = encoded(g_old_z_bf.y.data());
    g_bailout_bigfloat = +[] { return 17; };
    std::complex<double> expected;
    if (input.degree == 2)
    {
        expected = {
            input.point.real() * input.point.real() - input.point.imag() * input.point.imag() + parameter.real(),
            std::abs(2 * input.point.real() * input.point.imag()) - parameter.imag()};
    }
    else
    {
        expected =
            std::pow(std::complex<double>{std::abs(input.point.real()), -std::abs(input.point.imag())}, input.degree) +
            parameter;
    }
    const double tolerance = 1e-12 * std::max(1.0, std::abs(expected));
    const auto stack = save_stack();

    EXPECT_EQ(17, burning_ship_bf_fractal());

    EXPECT_NEAR(expected.real(), static_cast<double>(bf_to_float(g_new_z_bf.x.data())), tolerance);
    EXPECT_NEAR(expected.imag(), static_cast<double>(bf_to_float(g_new_z_bf.y.data())), tolerance);
    EXPECT_EQ(original_x, encoded(g_old_z_bf.x.data()));
    EXPECT_EQ(original_y, encoded(g_old_z_bf.y.data()));
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestBurningShipPoint, referenceOrbitKeepsItsOrientationAndScratchRoles)
{
    const auto &input = GetParam();
    g_params[2] = input.degree;
    BigStackSaver saved;
    BFComplex point{BigFloat{bf_arena(BigLifetime::TEMPORARY)}, BigFloat{bf_arena(BigLifetime::TEMPORARY)}};
    BFComplex center{BigFloat{bf_arena(BigLifetime::TEMPORARY)}, BigFloat{bf_arena(BigLifetime::TEMPORARY)}};
    point.x = input.point.real();
    point.y = input.point.imag();
    center.x = 0.125;
    center.y = -0.0625;
    const auto center_x = encoded(center.x.data());
    const auto center_y = encoded(center.y.data());
    g_new_z_bf.x = 7;
    g_new_z_bf.y = 9;
    std::complex<double> expected;
    if (input.degree == 2)
    {
        expected = {input.point.real() * input.point.real() - input.point.imag() * input.point.imag() + 0.125,
            std::abs(2 * input.point.real() * input.point.imag()) - 0.0625};
    }
    else
    {
        expected =
            std::pow(std::complex<double>{std::abs(input.point.real()), std::abs(input.point.imag())}, input.degree) +
            std::complex<double>{0.125, -0.0625};
    }
    const double tolerance = 1e-12 * std::max(1.0, std::abs(expected));
    const auto stack = save_stack();

    burning_ship_ref_pt_bf(center, point);

    EXPECT_NEAR(expected.real(), static_cast<double>(bf_to_float(point.x.data())), tolerance);
    EXPECT_NEAR(expected.imag(), static_cast<double>(bf_to_float(point.y.data())), tolerance);
    EXPECT_EQ(center_x, encoded(center.x.data()));
    EXPECT_EQ(center_y, encoded(center.y.data()));
    EXPECT_EQ(stack, save_stack());
    if (input.degree == 2)
    {
        EXPECT_EQ(BigFloat{7}, g_new_z_bf.x);
        EXPECT_EQ(BigFloat{9}, g_new_z_bf.y);
    }
    else
    {
        EXPECT_NEAR(expected.real() - 0.125, static_cast<double>(bf_to_float(g_new_z_bf.x.data())), tolerance);
        EXPECT_NEAR(expected.imag() + 0.0625, static_cast<double>(bf_to_float(g_new_z_bf.y.data())), tolerance);
    }
}

INSTANTIATE_TEST_SUITE_P(Points, TestBurningShipPoint, testing::ValuesIn(point_inputs()),
    [](const testing::TestParamInfo<ShipPointInput> &info) { return info.param.name; });

class TestBurningShipFrame : public TestBurningShipValues, public testing::WithParamInterface<int>
{
};

TEST_P(TestBurningShipFrame, smallFramesRetainTheOrdinaryEscapePattern)
{
    g_params[2] = GetParam();
    std::array<int, 48> ordinary{}, big_float{};
    for (int row = 0; row < 6; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            SCOPED_TRACE(row);
            SCOPED_TRACE(col);
            const DComplex point{-1.875 + col * 0.5 + 1.0 / 32, -1.125 + row * 0.5 + 1.0 / 64};
            g_init = g_old_z = point;
            g_param_z_bf.x = g_old_z_bf.x = point.x;
            g_param_z_bf.y = g_old_z_bf.y = point.y;
            const int pixel = row * 8 + col;
            for (int iteration = 1; iteration <= 20; ++iteration)
            {
                if (burning_ship_orbit() != 0)
                {
                    ordinary[pixel] = iteration;
                    break;
                }
            }
            for (int iteration = 1; iteration <= 20; ++iteration)
            {
                if (burning_ship_bf_fractal() != 0)
                {
                    big_float[pixel] = iteration;
                    break;
                }
            }
            EXPECT_EQ(ordinary[pixel], big_float[pixel]);
        }
    }
    EXPECT_EQ(ordinary, big_float);
}

INSTANTIATE_TEST_SUITE_P(Degrees, TestBurningShipFrame, testing::Values(2, 3, 4, 5));

} // namespace id::test
