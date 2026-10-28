// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calc_frac_init.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>

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
        return {{static_cast<double>(bf_to_float(g_bf_x_min)), static_cast<double>(bf_to_float(g_bf_y_min))},
            {static_cast<double>(bf_to_float(g_bf_x_max)), static_cast<double>(bf_to_float(g_bf_y_max))},
            {static_cast<double>(bf_to_float(g_bf_x_3rd)), static_cast<double>(bf_to_float(g_bf_y_3rd))}};
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

TEST_F(TestCalcFracInit, enteringArbitraryPrecisionStillInvalidatesResume)
{
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}, false);
    g_debug_flag = DebugFlags::FORCE_ARBITRARY_PRECISION_MATH;

    EXPECT_FALSE(calc_frac_init());

    EXPECT_NE(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(CalcStatus::PARAMS_CHANGED, g_calc_status);
}

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

} // namespace id::test
