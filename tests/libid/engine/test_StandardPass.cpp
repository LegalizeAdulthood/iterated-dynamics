// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/pixel.h>
#include <engine/StandardPass.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/engine_timer.h>
#include <engine/LogicalScreen.h>
#include <engine/log_map.h>
#include <engine/Potential.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/spindac.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/lyapunov.h>
#include <io/disk_storage.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ctime>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

int unexpected_calculation()
{
    throw std::logic_error{"Pixel routing must not dispatch calculation"};
}

struct PixelPassCase
{
    const char *name;
    CalcMode mode;
    Point2i next;
};

constexpr PixelPassCase PIXEL_PASSES[]{
    {"OnePass", CalcMode::ONE_PASS, {1, 0}},
    {"TwoPass", CalcMode::TWO_PASS, {2, 0}},
    {"BoundaryTrace", CalcMode::BOUNDARY_TRACE, {1, 0}},
    {"SolidGuess", CalcMode::SOLID_GUESS, {4, 0}},
    {"Diffusion", CalcMode::DIFFUSION, {4, 4}},
    {"Tesseral", CalcMode::TESSERAL, {1, 0}},
};

std::vector<Point2i> s_calculated;
bool s_yield_once{};
bool s_checkerboard{};
int s_color{7};

bool initialize_image()
{
    return true;
}

int calculate_selected_pixel()
{
    if (s_yield_once)
    {
        s_yield_once = false;
        return -1;
    }
    s_calculated.push_back({g_col, g_row});
    g_color = s_checkerboard ? 1 + (g_col + g_row) % 2 : s_color;
    g_plot(g_col, g_row, g_color);
    return g_color;
}

void PrintTo(const PixelPassCase &value, std::ostream *out)
{
    *out << value.name;
}

} // namespace

class TestStandardPass : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        set_frame(8, 8);
        g_dispatch.set_calc_type(unexpected_calculation);
        set_normal_dot();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * m_width + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int col, int row, int color)
                {
                    m_pixels.at(row * m_width + col) = color;
                    ++m_plot_count;
                }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    bool select_next(StandardPass &pass)
    {
        for (int chunks = 0; chunks < 16; ++chunks)
        {
            if (pass.select_pixel())
            {
                return true;
            }
        }
        return false;
    }

    void set_frame(int width, int height)
    {
        m_width = width;
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
        g_start_pt = {0, 0};
        g_stop_pt = {width - 1, height - 1};
        g_i_start_pt = g_start_pt;
        g_i_stop_pt = g_stop_pt;
        g_begin_pt = g_start_pt;
        m_pixels.assign(width * height, 0);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 8, 0, 0, 7, 7}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 8};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 8};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill_color{g_fill_color, -1};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 0};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 0};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    int m_width{};
    int m_plot_count{};
};

class TestStandardPassPixel : public TestStandardPass, public WithParamInterface<PixelPassCase>
{
};

TEST_P(TestStandardPassPixel, selectionIsRetainedWithoutCalculationOrPlotting)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    g_col = 99;
    g_row = 99;

    pass.begin_chunk();
    pass.select(GetParam().mode);
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ(GetParam().mode, pass.calc_mode());
    EXPECT_FALSE(pass.done());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassPixel, completionAdvancesTheActivePassOnce)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    g_color = 99;
    g_col = 99;
    g_row = 99;

    pass.complete_pixel(13);

    EXPECT_EQ(13, g_color);
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    if (GetParam().mode == CalcMode::TWO_PASS)
    {
        EXPECT_EQ(13, m_pixels[1]);
        EXPECT_EQ(13, m_pixels[8]);
        EXPECT_EQ(13, m_pixels[9]);
    }
    if (GetParam().mode == CalcMode::DIFFUSION)
    {
        EXPECT_EQ(13, m_pixels[7 * 8 + 7]);
    }
}

TEST_P(TestStandardPassPixel, completionRequiresSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

TEST_P(TestStandardPassPixel, resetDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));

    pass.reset();

    EXPECT_EQ(CalcMode::NONE, pass.calc_mode());
    EXPECT_TRUE(pass.done());
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

TEST_P(TestStandardPassPixel, changingPassDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    pass.complete_pixel(13);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    const CalcMode other{GetParam().mode == CalcMode::ONE_PASS ? CalcMode::TWO_PASS : CalcMode::ONE_PASS};

    pass.select(other);

    EXPECT_EQ(other, pass.calc_mode());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestStandardPassPixel, ValuesIn(PIXEL_PASSES),
    [](const TestParamInfo<PixelPassCase> &info) -> std::string { return info.param.name; });

TEST_F(TestStandardPass, emptyPassHasNoPixelWork)
{
    StandardPass pass;

    EXPECT_EQ(CalcMode::NONE, pass.calc_mode());
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    EXPECT_TRUE(pass.iterate());
}

TEST_F(TestStandardPass, orbitAlgorithmsDoNotOfferPixelRequests)
{
    StandardPass pass;
    for (const CalcMode mode : {CalcMode::SYNCHRONOUS_ORBIT, CalcMode::ORBIT})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        pass.select(mode);

        EXPECT_THROW(pass.select_pixel(), std::logic_error);
        EXPECT_THROW(pass.begin_chunk(), std::logic_error);
        EXPECT_THROW(pass.done(), std::logic_error);
        EXPECT_THROW(pass.selected_pixel(), std::logic_error);
        EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
        EXPECT_EQ(mode, pass.calc_mode());
    }
    EXPECT_EQ(0, m_plot_count);
}

class TestStandardFractal : public TestStandardPass
{
protected:
    void SetUp() override
    {
        TestStandardPass::SetUp();
        s_calculated.clear();
        s_yield_once = false;
        s_checkerboard = false;
        s_color = 7;
        m_specific.flags = FractalFlags::NONE;
        m_specific.symmetry = SymmetryType::NONE;
        m_specific.per_image = initialize_image;
        m_specific.calc_type = calculate_selected_pixel;
        g_dispatch = FractalDispatch{m_specific};
    }

    void TearDown() override
    {
        end_resume();
        TestStandardPass::TearDown();
    }

    bool select_next(StandardFractal &calculation)
    {
        for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
        {
            if (calculation.select_pixel())
            {
                return true;
            }
        }
        return false;
    }

    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<SymmetryType> m_saved_force_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<SymmetryType> m_saved_image_symmetry{g_symmetry};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<long> m_saved_log_map{g_log_map_flag, 0};
    ValueSaver<long> m_saved_distance_estimator{g_distance_estimator, 0};
    ValueSaver<int> m_saved_marker{g_show_dot, -1};
    ValueSaver<bool> m_saved_overlay{g_show_orbit, false};
    ValueSaver<int> m_saved_check_interval{g_keyboard_check_interval};
};

class TestRenderLifecycle : public TestStandardFractal
{
protected:
    ValueSaver<long> m_saved_time{g_calc_time, 0};
    ValueSaver<long> m_saved_timer_start{g_engine_timer_start, 0};
    ValueSaver<long> m_saved_interval{g_timer_interval, 0};
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, false};
    ValueSaver<bool> m_saved_true_color{g_true_color, false};
};

TEST_F(TestRenderLifecycle, completionAccountsForElapsedImageTimeOnce)
{
    StandardFractal calculation;
    EXPECT_TRUE(calculation.resume().empty());
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
    for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);

    calculation.finish();

    EXPECT_GE(g_calc_time, 100L);
    const long completed_time{g_calc_time};
    calculation.finish();
    EXPECT_EQ(completed_time, g_calc_time);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, suspensionAndResumeAccumulateDistinctIntervals)
{
    StandardFractal calculation;
    EXPECT_TRUE(calculation.resume().empty());
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;

    calculation.suspend();

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_GE(g_calc_time, 100L);
    const long suspended_time{g_calc_time};
    calculation.finish();
    EXPECT_EQ(suspended_time, g_calc_time);

    StandardFractal resumed;
    EXPECT_TRUE(resumed.resume().empty());
    EXPECT_EQ(suspended_time, g_calc_time);
    ASSERT_TRUE(select_next(resumed));
    g_engine_timer_start = static_cast<long>(std::clock()) - 2 * CLOCKS_PER_SEC;
    EXPECT_TRUE(resumed.calculate_pixel());
    for (int chunks = 0; chunks < 100 && !resumed.done(); ++chunks)
    {
        resumed.iterate();
    }
    ASSERT_TRUE(resumed.done());
    resumed.finish();

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GE(g_calc_time, suspended_time + 200);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, parameterChangeDiscardsSuspendedTraversalAndTiming)
{
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    calculation.calculate_pixel();
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    g_calc_time = 123;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    s_calculated.clear();

    StandardFractal restarted;
    restarted.resume();

    EXPECT_FALSE(g_resuming);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0L, g_calc_time);
    ASSERT_TRUE(select_next(restarted));
    EXPECT_EQ((Point2i{0, 0}), restarted.selected_pixel());
    restarted.abort();
}

TEST_F(TestRenderLifecycle, abandoningSetupDoesNotChargeAStaleTimer)
{
    g_timer_interval = 12345;
    {
        StandardFractal calculation;
        EXPECT_TRUE(calculation.resume().empty());
    }

    EXPECT_EQ(0L, g_calc_time);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestRenderLifecycle, finishingUnfinishedWorkRestoresDispatchAndDiscardsResumeData)
{
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());

    calculation.finish();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(calculate_selected_pixel, g_dispatch.calc_type());
}

TEST_F(TestRenderLifecycle, failureRestoresDispatchAndDiscardsPartialWork)
{
    g_dispatch.set_per_image(
        []() -> bool
        {
            g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
            throw std::runtime_error("Calculation setup failed");
        });
    const FractalDispatch dispatch{g_dispatch};

    EXPECT_THROW(
        {
            StandardFractal calculation;
            calculation.resume();
            calculation.iterate();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(g_log_map_table.empty());
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    EXPECT_GE(g_calc_time, 100L);
}

TEST_F(TestRenderLifecycle, lyapunovUsesTheCommonSetupAndCleanup)
{
    g_colors = 2;
    g_log_map_flag = 1;
    g_inside_method = ColorMethod::COLOR;
    Lyapunov calculation;

    EXPECT_THAT(calculation.resume(), ElementsAre("Need at least 16 colors to use logmap"));
    EXPECT_EQ(0L, g_log_map_flag);
    for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
    calculation.finish();

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GE(g_calc_time, 100L);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, activeCalculationCannotBeStartedAgain)
{
    StandardFractal calculation;
    calculation.resume();

    EXPECT_THROW(calculation.resume(), std::logic_error);

    calculation.abort();
}

class TestStandardFractalPixel : public TestStandardFractal, public WithParamInterface<PixelPassCase>
{
};

TEST_P(TestStandardFractalPixel, selectedCoordinateIsObservableBeforeCalculation)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();
    EXPECT_THROW(calculation.calculate_pixel(), std::logic_error);

    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(calculation.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(0, m_plot_count);
    ASSERT_TRUE(calculation.calculate_pixel());
    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}));
    EXPECT_THROW(calculation.calculate_pixel(), std::logic_error);
    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ(GetParam().next, calculation.selected_pixel());
}

TEST_P(TestStandardFractalPixel, unfinishedCalculationRetainsSelectionAndContinuation)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    s_yield_once = true;

    EXPECT_FALSE(calculation.calculate_pixel());

    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(0, m_plot_count);
    ASSERT_TRUE(calculation.select_pixel());
    ASSERT_TRUE(calculation.calculate_pixel());
    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}));
    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ(GetParam().next, calculation.selected_pixel());
}

TEST_P(TestStandardFractalPixel, separateOperationsCompleteTheImage)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();

    for (int chunks = 0; chunks < 1000 && !calculation.done(); ++chunks)
    {
        while (calculation.select_pixel())
        {
            ASSERT_LT(s_calculated.size(), 10000U);
            ASSERT_TRUE(calculation.calculate_pixel());
        }
    }

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_THROW(calculation.selected_pixel(), std::logic_error);
    EXPECT_THROW(calculation.calculate_pixel(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestStandardFractalPixel, ValuesIn(PIXEL_PASSES),
    [](const TestParamInfo<PixelPassCase> &info) -> std::string { return info.param.name; });

TEST_F(TestStandardFractal, scanlineCalculationRemainsBatched)
{
    for (const CalcMode mode : {CalcMode::ONE_PASS, CalcMode::TWO_PASS})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        g_std_calc_mode = mode;
        s_calculated.clear();
        StandardFractal calculation;
        calculation.resume();

        calculation.iterate();

        EXPECT_TRUE(calculation.done());
        EXPECT_EQ(64U, s_calculated.size());
        EXPECT_THAT(m_pixels, Each(7));
    }
}

TEST_F(TestStandardFractal, scanlineCalculationPreservesLegacySignedColors)
{
    g_std_calc_mode = CalcMode::TWO_PASS;
    s_color = -2;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(-2, g_color);
}

TEST_F(TestStandardFractal, boundaryScanKeepsItsCalculationChunk)
{
    set_frame(600, 1);
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    s_checkerboard = true;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(512U, s_calculated.size());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(600U, s_calculated.size());
}

TEST_F(TestStandardFractal, boundaryOutlineKeepsItsCalculationChunk)
{
    set_frame(600, 2);
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(258U, s_calculated.size());
}

TEST_F(TestStandardFractal, solidGuessKeepsSeedCalculationsBatchedByRow)
{
    set_frame(16, 12);
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    EXPECT_TRUE(s_calculated.empty());

    calculation.iterate();

    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}, Point2i{4, 0}, Point2i{8, 0}, Point2i{12, 0}));
    s_calculated.clear();
    calculation.iterate();
    EXPECT_THAT(s_calculated,
        ElementsAre(Point2i{0, 4}, Point2i{4, 4}, Point2i{8, 4}, Point2i{12, 4}, Point2i{14, 2}, Point2i{14, 0}));
    EXPECT_EQ(0, g_current_row);
    s_calculated.clear();
    calculation.iterate();
    EXPECT_THAT(s_calculated,
        ElementsAre(Point2i{0, 8}, Point2i{4, 8}, Point2i{8, 8}, Point2i{12, 8}, Point2i{14, 6}, Point2i{14, 4}));
    EXPECT_EQ(4, g_current_row);
}

TEST_F(TestStandardFractal, diffusionKeepsItsCounterChunk)
{
    set_frame(64, 32);
    g_std_calc_mode = CalcMode::DIFFUSION;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(1024U, s_calculated.size());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(2048U, s_calculated.size());
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestStandardFractal, tesseralKeepsBatchedEdges)
{
    set_frame(600, 4);
    g_std_calc_mode = CalcMode::TESSERAL;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_EQ(512U, s_calculated.size());
    EXPECT_FALSE(calculation.done());
    calculation.iterate();
    EXPECT_EQ(1112U, s_calculated.size());
    calculation.iterate();
    EXPECT_EQ(1204U, s_calculated.size());
    EXPECT_FALSE(calculation.done());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_THAT(m_pixels, Each(7));
}

class TestStandardPotential : public TestStandardFractal
{
protected:
    void SetUp() override;
    void TearDown() override;
    void open_storage();
};

void TestStandardPotential::SetUp()
{
    TestStandardFractal::SetUp();
    end_disk();
    init_memory();
    g_potential.flag = true;
    g_potential.store_16bit = true;
}

void TestStandardPotential::TearDown()
{
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    TestStandardFractal::TearDown();
}

void TestStandardPotential::open_storage()
{
    start_disk_storage(8, 16, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    g_disk_16_bit = true;
}

TEST_F(TestStandardPotential, storageRequirementStopsBeforeWorkOrInput)
{
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.potential_storage_required());
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(CalcMode::SOLID_GUESS, g_std_calc_mode);
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(0, m_plot_count);
    calculation.iterate();
    EXPECT_FALSE(calculation.select_pixel());
    EXPECT_TRUE(s_calculated.empty());
    calculation.abort();
}

TEST_F(TestStandardPotential, availableStorageSelectsOnePassBeforeCalculating)
{
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    ASSERT_TRUE(calculation.potential_storage_required());
    open_storage();

    calculation.complete_potential_storage(true);

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_TRUE(s_calculated.empty());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestStandardPotential, unavailableStorageRetainsRequestedTraversal)
{
    g_std_calc_mode = CalcMode::TWO_PASS;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    ASSERT_TRUE(calculation.potential_storage_required());

    calculation.complete_potential_storage(false);

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_EQ(CalcMode::TWO_PASS, g_std_calc_mode);
    EXPECT_TRUE(g_potential.flag);
    EXPECT_FALSE(g_potential.store_16bit);
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestStandardPotential, resumeKeepsExistingStorageAndContinuesAtSavedPixel)
{
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    ASSERT_TRUE(calculation.potential_storage_required());
    open_storage();
    write_disk_pixel(3, 11, 123);
    calculation.complete_potential_storage(true);
    ASSERT_TRUE(calculation.select_pixel());
    ASSERT_TRUE(calculation.calculate_pixel());
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    g_resuming = true;
    StandardFractal resumed;
    resumed.resume();

    resumed.iterate();

    EXPECT_FALSE(resumed.potential_storage_required());
    EXPECT_TRUE(resumed.done());
    EXPECT_EQ(64U, s_calculated.size());
    ASSERT_GE(s_calculated.size(), 2U);
    EXPECT_EQ((Point2i{0, 0}), s_calculated[0]);
    EXPECT_EQ((Point2i{1, 0}), s_calculated[1]);
    EXPECT_EQ(123, read_disk_pixel(3, 11));
    EXPECT_TRUE(g_disk_16_bit);
}

TEST_F(TestStandardPotential, threePassFollowupRequestsFreshStorage)
{
    g_std_calc_mode = CalcMode::THREE_PASS;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    ASSERT_TRUE(calculation.potential_storage_required());
    open_storage();
    calculation.complete_potential_storage(true);
    calculation.iterate();
    calculation.iterate();

    ASSERT_TRUE(calculation.potential_storage_required());
    EXPECT_EQ(64U, s_calculated.size());
    calculation.complete_potential_storage(true);
    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(128U, s_calculated.size());
}

TEST_F(TestStandardPotential, storageCompletionRequiresAnOutstandingRequest)
{
    StandardFractal calculation;
    EXPECT_THROW(calculation.complete_potential_storage(false), std::logic_error);
    calculation.resume();
    calculation.iterate();
    ASSERT_TRUE(calculation.potential_storage_required());
    calculation.complete_potential_storage(false);

    EXPECT_THROW(calculation.complete_potential_storage(true), std::logic_error);

    calculation.abort();
}

TEST_F(TestStandardFractal, inactivePotentialDoesNotRequestStorage)
{
    g_potential.store_16bit = true;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

} // namespace id::test
