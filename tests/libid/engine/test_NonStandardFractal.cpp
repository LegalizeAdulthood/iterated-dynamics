// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/NonStandardFractal.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/engine_timer.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/bif_may.h>
#include <fractals/fractalp.h>
#include <fractals/population.h>
#include <misc/ValueSaver.h>
#include <ui/calculate_image.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

int s_orbit_calls{};

int stable_population()
{
    ++s_orbit_calls;
    g_population = 0.5;
    return 0;
}

int unexpected_renderer()
{
    throw std::logic_error("Calculation entered a renderer callback");
}

bool unexpected_setup()
{
    throw std::logic_error("Calculation entered legacy per-image setup");
}

struct BifurcationCase
{
    const char *name;
    FractalType type;
};

void PrintTo(const BifurcationCase &value, std::ostream *out)
{
    *out << value.name;
}

constexpr BifurcationCase BIFURCATION_CASES[]{
    {"Verhulst", FractalType::BIFURCATION},
    {"Lambda", FractalType::BIF_LAMBDA},
    {"PlusSinPi", FractalType::BIF_PLUS_SIN_PI},
    {"EqualsSinPi", FractalType::BIF_EQ_SIN_PI},
    {"Stewart", FractalType::BIF_STEWART},
    {"May", FractalType::BIF_MAY},
};

} // namespace

class TestNonStandardFractal : public Test
{
protected:
    void SetUp() override
    {
        s_orbit_calls = 0;
        select_type(FractalType::BIFURCATION);
        set_normal_dot();
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int x, int y, int color)
                {
                    m_pixels.at(y * WIDTH + x) = color;
                    m_columns.push_back(x);
                }));
    }

    void TearDown() override
    {
        end_resume();
    }

    void select_type(FractalType type)
    {
        g_fractal_type = type;
        m_specific = *get_fractal_specific(type);
        m_specific.calc_type = unexpected_renderer;
        m_specific.per_image = unexpected_setup;
        m_specific.orbit_calc = stable_population;
        g_dispatch = FractalDispatch{m_specific};
    }

    void complete(NonStandardFractal &calculation)
    {
        for (int chunks = 0; chunks < WIDTH + 1 && !calculation.done(); ++chunks)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    static constexpr int WIDTH{4};
    static constexpr int HEIGHT{6};
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::BIFURCATION)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<LogicalScreen> m_saved_screen{
        g_logical_screen, LogicalScreen{WIDTH, HEIGHT, 0, 0, WIDTH - 1.0, HEIGHT - 1.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, WIDTH};
    ValueSaver<int> m_saved_height{g_screen_y_dots, HEIGHT};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{2.0, 0.0}, {4.0, 1.0}, {2.0, 0.0}}};
    ValueSaver<LDouble> m_saved_dx{g_delta_x, 2.0 / (WIDTH - 1)};
    ValueSaver<LDouble> m_saved_dy{g_delta_y, 1.0 / (HEIGHT - 1)};
    ValueSaver<double> m_saved_filter{g_params[0], 4.0};
    ValueSaver<double> m_saved_seed{g_params[1], 0.66};
    ValueSaver<double> m_saved_beta{g_params[2], 3.0};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 8};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<long> m_saved_time{g_calc_time, 0};
    ValueSaver<long> m_saved_timer_start{g_engine_timer_start};
    ValueSaver<long> m_saved_interval{g_timer_interval};
    std::array<int, WIDTH * HEIGHT> m_pixels{};
    std::vector<int> m_columns;
};

TEST_F(TestNonStandardFractal, noOperationRequiresExplicitSetupAndSelection)
{
    NonStandardFractal calculation;

    EXPECT_TRUE(calculation.done());
    EXPECT_THROW(calculation.select(), std::logic_error);
    EXPECT_THROW(calculation.iterate(), std::logic_error);
    EXPECT_THROW(calculation.suspend(), std::logic_error);
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
}

TEST_F(TestNonStandardFractal, unsupportedTypeDoesNotInvokeItsRenderer)
{
    select_type(FractalType::MANDEL);
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_FALSE(calculation.select());

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
}

TEST_F(TestNonStandardFractal, activeOperationCannotBeReplaced)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_TRUE(calculation.select());

    EXPECT_THROW(calculation.select(), std::logic_error);

    calculation.iterate();
    EXPECT_EQ(HEIGHT, m_columns.size());
    EXPECT_FALSE(calculation.done());
}

TEST_F(TestNonStandardFractal, maySelectionInitializesBetaWithoutLegacySetup)
{
    select_type(FractalType::BIF_MAY);
    for (const double beta : {3.0, 1.0})
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        g_params[2] = beta;
        NonStandardFractal calculation;
        calculation.start();

        ASSERT_TRUE(calculation.select());

        const double exponent{std::max(beta, 2.0)};
        EXPECT_EQ(exponent, g_params[2]);
        g_rate = 1.25;
        g_population = 0.5;
        EXPECT_EQ(0, bifurc_may_orbit());
        EXPECT_NEAR(1.25 * 0.5 / std::pow(1.5, exponent), g_population, 1e-15);
    }
}

TEST_F(TestNonStandardFractal, abandonedOperationDiscardsWorkWithoutChargingAnOldTimer)
{
    g_timer_interval = 12345;
    {
        NonStandardFractal calculation;
        calculation.start();
        ASSERT_TRUE(calculation.select());
    }

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0L, g_calc_time);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestNonStandardFractal, failedIncrementCleansUpInsteadOfEnteringUi)
{
    EXPECT_THROW(
        {
            NonStandardFractal calculation;
            calculation.start();
            ASSERT_TRUE(calculation.select());
            g_dispatch.set_orbit_calc([]() -> int { throw std::runtime_error("Orbit failed"); });
            calculation.iterate();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(m_columns.empty());
}

class TestStandaloneBifurcation : public TestNonStandardFractal, public WithParamInterface<BifurcationCase>
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(GetParam().type);
    }
};

TEST_P(TestStandaloneBifurcation, selectionDoesNoIterationAndOneIncrementComputesOneColumn)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    ASSERT_TRUE(calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(12, s_orbit_calls);
    EXPECT_THAT(m_columns, ElementsAre(0, 0, 0, 0, 0, 0));
}

TEST_P(TestStandaloneBifurcation, selectedOperationCompletesWithoutRendererCallbacks)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_TRUE(calculation.select());

    complete(calculation);

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    const long completed_time{g_calc_time};
    calculation.finish(false);
    EXPECT_EQ(completed_time, g_calc_time);
}

TEST_P(TestStandaloneBifurcation, suspensionResumesAtTheNextUncomputedColumn)
{
    NonStandardFractal control;
    control.start();
    ASSERT_TRUE(control.select());
    complete(control);
    const std::array<int, WIDTH * HEIGHT> expected{m_pixels};
    m_pixels.fill(0);
    m_columns.clear();
    s_orbit_calls = 0;
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_TRUE(calculation.select());
    calculation.iterate();
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    const long suspended_time{g_calc_time};

    NonStandardFractal resumed;
    resumed.start();
    EXPECT_EQ(suspended_time, g_calc_time);
    ASSERT_TRUE(resumed.select());
    EXPECT_TRUE(g_resume_data.empty());
    complete(resumed);

    EXPECT_EQ(expected, m_pixels);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    for (int x = 0; x < WIDTH; ++x)
    {
        EXPECT_EQ(HEIGHT, std::count(m_columns.begin(), m_columns.end(), x));
    }
}

TEST_P(TestStandaloneBifurcation, tableHasNoRendererOrPerImageCallback)
{
    const FractalSpecific &specific{*get_fractal_specific(GetParam().type)};

    EXPECT_EQ(nullptr, specific.calc_type);
    EXPECT_EQ(nullptr, specific.per_image);
    EXPECT_NE(nullptr, specific.orbit_calc);
}

TEST_P(TestStandaloneBifurcation, uiEntryUsesSelectedOperationWithoutLegacyCallbacks)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));

    EXPECT_EQ(0, calculate_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
}

TEST_P(TestStandaloneBifurcation, uiInterruptionLeavesMenuKeyPendingAndResumesAtNextColumn)
{
    int pending_key{};
    bool interrupt_sent{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [&]
            {
                if (!interrupt_sent && m_columns.size() >= HEIGHT)
                {
                    pending_key = 'x';
                    interrupt_sent = true;
                }
                return pending_key;
            }));
    EXPECT_CALL(m_driver, get_key())
        .WillRepeatedly(Invoke(
            [&pending_key]
            {
                const int key{pending_key};
                pending_key = 0;
                return key;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([&pending_key](int key) { pending_key = key; }));

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_EQ('x', pending_key);
    ASSERT_EQ(HEIGHT, m_columns.size());
    pending_key = 0;

    EXPECT_EQ(0, calculate_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    for (int x = 0; x < WIDTH; ++x)
    {
        EXPECT_EQ(HEIGHT, std::count(m_columns.begin(), m_columns.end(), x));
    }
}

INSTANTIATE_TEST_SUITE_P(Bifurcations, TestStandaloneBifurcation, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) -> std::string { return info.param.name; });

} // namespace id::test
