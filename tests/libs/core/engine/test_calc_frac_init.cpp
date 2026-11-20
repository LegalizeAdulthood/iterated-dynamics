// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calc_frac_init.h>
#include <math/BigFloat.h>

#include <MockDriver.h>

#include <engine/bailout_formula.h>
#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <math/big_init.h>
#include <math/BigNum.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <tuple>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestCalcFracInit : public Test
{
protected:
    void SetUp() override
    {
        g_user.std_calc_mode = CalcMode::SOLID_GUESS;
        g_user.biomorph_value = -1;
        EXPECT_CALL(m_driver, check_memory()).Times(AnyNumber());
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        free_pixel_grid();
        free_bf_vars();
    }

    void set_region(const ImageRegion &region, const bool arbitrary_precision)
    {
        g_image_region = region;
        if (arbitrary_precision)
        {
            g_debug_flag = DebugFlags::FORCE_ARBITRARY_PRECISION_MATH;
            fractal_float_to_bf();
        }
        g_calc_status = CalcStatus::RESUMABLE;
    }

    ImageRegion current_region() const
    {
        if (g_bf_math == BFMathType::NONE)
        {
            return g_image_region;
        }
        return {{static_cast<double>(g_bf_x_min.to_float()), static_cast<double>(g_bf_y_min.to_float())},
            {static_cast<double>(g_bf_x_max.to_float()), static_cast<double>(g_bf_y_max.to_float())},
            {static_cast<double>(g_bf_x_3rd.to_float()), static_cast<double>(g_bf_y_3rd.to_float())}};
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::array<double, 2>> m_saved_tolerance{g_math_tol, std::array<double, 2>{0.05, 0.05}};
};

struct ImageTranslationInput
{
    const char *name;
    ImageRegion input;
    ImageRegion expected;
    bool arbitrary_precision;
    bool translated;
    double tolerance;
};

void PrintTo(const ImageTranslationInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestCalcFracInitBounds : public TestCalcFracInit, public WithParamInterface<ImageTranslationInput>
{
};

TEST_P(TestCalcFracInitBounds, reportsRegionTranslationWithoutInvalidatingResume)
{
    const ImageTranslationInput &input{GetParam()};
    set_region(input.input, input.arbitrary_precision);

    EXPECT_EQ(input.translated, calc_frac_init());

    const ImageRegion region{current_region()};
    EXPECT_NEAR(input.expected.m_min.x, region.m_min.x, input.tolerance);
    EXPECT_NEAR(input.expected.m_max.x, region.m_max.x, input.tolerance);
    EXPECT_NEAR(input.expected.m_min.y, region.m_min.y, input.tolerance);
    EXPECT_NEAR(input.expected.m_max.y, region.m_max.y, input.tolerance);
    EXPECT_NEAR(input.expected.m_3rd.x, region.m_3rd.x, input.tolerance);
    EXPECT_NEAR(input.expected.m_3rd.y, region.m_3rd.y, input.tolerance);
    EXPECT_EQ(region.m_min, region.m_3rd);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(input.arbitrary_precision, g_bf_math != BFMathType::NONE);
}

const ImageRegion UNTRANSLATED_REGION{{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}};
const ImageRegion TRANSLATION_INPUT{{32768.0, -32770.0}, {32770.0, -32768.0}, {32768.0, -32770.0}};
const ImageRegion TRANSLATION_EXPECTED{{32765.99, -32767.99}, {32767.99, -32765.99}, {32765.99, -32767.99}};

INSTANTIATE_TEST_SUITE_P(TranslationInputs, TestCalcFracInitBounds,
    Values(ImageTranslationInput{"UntranslatedFloat", UNTRANSLATED_REGION, UNTRANSLATED_REGION, false, false, 0.0},
        ImageTranslationInput{
            "UntranslatedArbitraryPrecision", UNTRANSLATED_REGION, UNTRANSLATED_REGION, true, false, 0.0},
        ImageTranslationInput{"TranslatedFloat", TRANSLATION_INPUT, TRANSLATION_EXPECTED, false, true, 1e-8},
        ImageTranslationInput{
            "TranslatedArbitraryPrecision", TRANSLATION_INPUT, TRANSLATION_EXPECTED, true, true, 1e-8}),
    [](const TestParamInfo<ImageTranslationInput> &info) { return info.param.name; });

TEST_F(TestCalcFracInit, arbitraryPrecisionBoundsShrinkOversizedRegionsAtTheSameCenter)
{
    set_region({{-65535.98, -49151.985}, {65535.98, 49151.985}, {-65535.98, -49151.985}}, true);

    EXPECT_TRUE(calc_frac_init());

    const ImageRegion region{current_region()};
    EXPECT_NEAR(-32767.99, region.m_min.x, 1e-7);
    EXPECT_NEAR(32767.99, region.m_max.x, 1e-7);
    EXPECT_NEAR(-24575.9925, region.m_min.y, 1e-7);
    EXPECT_NEAR(24575.9925, region.m_max.y, 1e-7);
    EXPECT_EQ(region.m_min, region.m_3rd);
}

TEST_F(TestCalcFracInit, arbitraryPrecisionCornerAdjustmentSnapsNearlyAxisAlignedEdges)
{
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-1.99999999, -1.49999999}}, true);

    adjust_corner_bf();

    EXPECT_EQ(g_bf_x_min, g_bf_x_3rd);
    EXPECT_EQ(g_bf_y_min, g_bf_y_3rd);
}

TEST_F(TestCalcFracInit, enteringArbitraryPrecisionStillInvalidatesResume)
{
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}, false);
    g_debug_flag = DebugFlags::FORCE_ARBITRARY_PRECISION_MATH;

    EXPECT_FALSE(calc_frac_init());

    EXPECT_NE(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(CalcStatus::PARAMS_CHANGED, g_calc_status);
}

struct MinimumIncrementPrecision
{
    int digits;
    const char *positive;
    const char *negative;
};

void PrintTo(const MinimumIncrementPrecision &value, std::ostream *str)
{
    *str << value.digits;
}

struct MinimumIncrementCoordinate
{
    const char *name;
    ImageRegion region;
    const BigFloat *target;
    bool negative;
};

void PrintTo(const MinimumIncrementCoordinate &value, std::ostream *str)
{
    *str << value.name;
}

class TestBigFloatMinimumIncrement
    : public TestCalcFracInit,
      public WithParamInterface<std::tuple<MinimumIncrementPrecision, MinimumIncrementCoordinate>>
{
};

TEST_P(TestBigFloatMinimumIncrement, preservesExactCoordinateBytesAtActivePrecision)
{
    const auto &precision = std::get<0>(GetParam());
    const auto &coordinate = std::get<1>(GetParam());
    g_bf_digits = precision.digits;
    g_logical_screen = {2, 3, 0, 0, 1.0, 2.0};
    ValueSaver saved_drift{g_aspect_drift, 0.0F};
    set_region(coordinate.region, true);
    const auto saved = save_stack();

    EXPECT_FALSE(calc_frac_init());

    EXPECT_EQ(precision.digits, g_decimals);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(saved, save_stack());
    EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &coordinate.target->arena());
    std::vector<Byte> packed(static_cast<std::size_t>(g_bf_length + 2));
    coordinate.target->pack_bf(packed.data(), packed.size());
    std::string encoded;
    constexpr char hex_digits[]{"0123456789abcdef"};
    for (const Byte value : packed)
    {
        encoded.push_back(hex_digits[value >> 4]);
        encoded.push_back(hex_digits[value & 15]);
    }
    EXPECT_EQ(coordinate.negative ? precision.negative : precision.positive, encoded);
}

// Canonical byte snapshots from the pointer-based increment, including
// precision-dependent truncation of the low mantissa bytes.
INSTANTIATE_TEST_SUITE_P(Coordinates, TestBigFloatMinimumIncrement,
    Combine(Values(MinimumIncrementPrecision{10, "0d37092d00000000004001000000", "e591eda5ffffffffff7ffdff0000"},
                MinimumIncrementPrecision{
                    20, "003757420d37092d00000000004001000000", "0092517be591eda5ffffffffff7ffdff0000"},
                MinimumIncrementPrecision{50, "000000000000000000000000003757420d37092d00000000004001000000",
                    "0000000000000000000000000092517be591eda5ffffffffff7ffdff0000"}),
        Values(MinimumIncrementCoordinate{"XMax", {{1.25, -3}, {1.25, -2}, {2.25, -3}}, &g_bf_x_max, false},
            MinimumIncrementCoordinate{"YMax", {{0.25, -2.5}, {2.25, -2.5}, {0.25, -3.5}}, &g_bf_y_max, true},
            MinimumIncrementCoordinate{"XThird", {{0.25, -3}, {2.25, -2}, {1.25, -3}}, &g_bf_x_3rd, false},
            MinimumIncrementCoordinate{"YThird", {{0.25, -3}, {2.25, -2}, {0.25, -2.5}}, &g_bf_y_3rd, true})),
    [](const TestParamInfo<std::tuple<MinimumIncrementPrecision, MinimumIncrementCoordinate>> &info)
    { return std::string{std::get<1>(info.param).name} + std::to_string(std::get<0>(info.param).digits); });

TEST_F(TestCalcFracInit, potentialDisablesDistanceEstimation)
{
    ValueSaver saved_potential{g_potential, Potential{}};
    ValueSaver saved_distance{g_distance_estimator};
    ValueSaver saved_bailout{g_magnitude_limit};
    ValueSaver saved_colors{g_colors, 256};
    g_potential.params = {32.0, 10.0, 4.0};
    g_user.distance_estimator_value = -9;
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}, false);

    EXPECT_FALSE(calc_frac_init());

    EXPECT_TRUE(g_potential.flag);
    EXPECT_EQ(0, g_distance_estimator);
    EXPECT_EQ(0, g_user.distance_estimator_value);
}

TEST_F(TestCalcFracInit, bigNumBailoutPreservesFractionalPotentialLimit)
{
    ValueSaver saved_potential{g_potential, Potential{}};
    ValueSaver saved_distance{g_distance_estimator};
    ValueSaver saved_limit{g_magnitude_limit};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_test{g_bailout_test};
    ValueSaver saved_float{g_bailout_float};
    ValueSaver saved_bn{g_bailout_bignum};
    ValueSaver saved_bf{g_bailout_bigfloat};
    g_potential.params = {32.0, 10.0, 1.5};
    set_bailout_formula(Bailout::MOD);
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}, true);

    EXPECT_FALSE(calc_frac_init());

    ASSERT_EQ(BFMathType::BIG_NUM, g_bf_math);
    ASSERT_TRUE(g_potential.flag);
    ASSERT_EQ(1.5, g_magnitude_limit);
    g_new_z_bn.x = 1;
    g_new_z_bn.y = 0.5;
    EXPECT_EQ(0, g_bailout_bignum());
    EXPECT_EQ(BigNum{1}, g_old_z_bn.x);
    EXPECT_EQ(BigNum{0.5}, g_old_z_bn.y);
}

} // namespace id::test
