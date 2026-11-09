// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
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
    float_to_bn(g_new_z_bn.x, input.point.x);
    float_to_bn(g_new_z_bn.y, input.point.y);
    std::vector<Byte> expected_x(g_r_length), expected_y(g_r_length), expected_old(g_r_length);
    square_bn(expected_x.data(), g_new_z_bn.x);
    square_bn(expected_y.data(), g_new_z_bn.y);
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
    EXPECT_EQ(input.point.x, bn_to_float(g_new_z_bn.x));
    EXPECT_EQ(input.point.y, bn_to_float(g_new_z_bn.y));
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
        str_to_bn(g_new_z_bn.x, "0.123456789012345678901234567890123456789");
        str_to_bn(g_new_z_bn.y, "-0.345678901234567890123456789012345678901");
        std::vector<Byte> expected_x(g_r_length), expected_y(g_r_length);
        square_bn(expected_x.data(), g_new_z_bn.x);
        square_bn(expected_y.data(), g_new_z_bn.y);
        const std::vector<Byte> original_x(g_new_z_bn.x, g_new_z_bn.x + g_bn_length);
        const std::vector<Byte> original_y(g_new_z_bn.y, g_new_z_bn.y + g_bn_length);

        EXPECT_EQ(1, g_bailout_bignum());

        EXPECT_EQ(expected_x, std::vector<Byte>(g_tmp_sqr_x_bn, g_tmp_sqr_x_bn + g_r_length));
        EXPECT_EQ(expected_y, std::vector<Byte>(g_tmp_sqr_y_bn, g_tmp_sqr_y_bn + g_r_length));
        EXPECT_EQ(original_x, std::vector<Byte>(g_new_z_bn.x, g_new_z_bn.x + g_bn_length));
        EXPECT_EQ(original_y, std::vector<Byte>(g_new_z_bn.y, g_new_z_bn.y + g_bn_length));
    }
}

} // namespace id::test
