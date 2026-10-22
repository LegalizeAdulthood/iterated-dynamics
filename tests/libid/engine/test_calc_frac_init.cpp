// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calc_frac_init.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

class TestCalcFracInitBounds : public TestCalcFracInit, public WithParamInterface<bool>
{
};

TEST_P(TestCalcFracInitBounds, reportsUntranslatedRegion)
{
    const ImageRegion region{{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}};
    set_region(region, GetParam());

    EXPECT_FALSE(calc_frac_init());

    EXPECT_EQ(region, current_region());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(GetParam(), g_bf_math != BFMathType::NONE);
}

TEST_P(TestCalcFracInitBounds, reportsTranslationWithoutInvalidatingResume)
{
    set_region({{32768.0, -32770.0}, {32770.0, -32768.0}, {32768.0, -32770.0}}, GetParam());

    EXPECT_TRUE(calc_frac_init());

    const ImageRegion region{current_region()};
    EXPECT_NEAR(32765.99, region.m_min.x, 1e-8);
    EXPECT_NEAR(32767.99, region.m_max.x, 1e-8);
    EXPECT_NEAR(-32767.99, region.m_min.y, 1e-8);
    EXPECT_NEAR(-32765.99, region.m_max.y, 1e-8);
    EXPECT_EQ(region.m_min, region.m_3rd);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(GetParam(), g_bf_math != BFMathType::NONE);
}

INSTANTIATE_TEST_SUITE_P(Precision, TestCalcFracInitBounds, Bool());

TEST_F(TestCalcFracInit, enteringArbitraryPrecisionStillInvalidatesResume)
{
    set_region({{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}}, false);
    g_debug_flag = DebugFlags::FORCE_ARBITRARY_PRECISION_MATH;

    EXPECT_FALSE(calc_frac_init());

    EXPECT_NE(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(CalcStatus::PARAMS_CHANGED, g_calc_status);
}

} // namespace id::test
