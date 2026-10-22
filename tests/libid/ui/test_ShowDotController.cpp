// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/ShowDotController.h>

#include "MockDriver.h"
#include "TestRender.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/NullSoundOutput.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/SoundOutput.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/lyapunov.h>
#include <misc/ValueSaver.h>
#include <ui/ImageRenderContext.h>
#include <ui/KeyboardHandler.h>
#include <ui/LyapunovController.h>
#include <ui/SoundSession.h>
#include <ui/StandardFractalController.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
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

std::function<int()> s_calculate;

int calculate_pixel()
{
    return s_calculate();
}

bool initialize_image()
{
    return true;
}

} // namespace

class TestShowDotController : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        m_pixels.resize(WIDTH * HEIGHT);
        for (int i = 0; i < WIDTH * HEIGHT; ++i)
        {
            m_pixels[i] = 1 + i % 13;
        }
        set_normal_dot();
        reset_calc_interrupted();
        m_specific.flags = FractalFlags::NONE;
        m_specific.symmetry = SymmetryType::NONE;
        m_specific.per_image = initialize_image;
        m_specific.calc_type = calculate_pixel;
        g_dispatch = FractalDispatch{m_specific};
        s_calculate = [this]
        {
            EXPECT_THAT(m_pixels, Each(Ne(15)));
            ++m_calculations;
            if (m_yield_once)
            {
                m_yield_once = false;
                return -1;
            }
            g_color = 7;
            g_plot(g_col, g_row, g_color);
            if (m_calculations == m_interrupt_after)
            {
                m_key = 'x';
            }
            return g_color;
        };
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * WIDTH + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int col, int row, int color) { m_pixels.at(row * WIDTH + col) = color; }));
        EXPECT_CALL(m_driver, flush()).WillRepeatedly(Invoke([this] { ++m_flushes; }));
        EXPECT_CALL(m_driver, delay(0)).Times(AnyNumber());
        EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([this] { return m_key; }));
        EXPECT_CALL(m_driver, get_key())
            .WillRepeatedly(Invoke(
                [this]
                {
                    const int key{m_key};
                    m_key = 0;
                    return key;
                }));
        EXPECT_CALL(m_driver, unget_key('x')).WillRepeatedly(Invoke([this](int key) { m_key = key; }));
    }

    void TearDown() override
    {
        s_calculate = {};
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
        end_resume();
        reset_calc_interrupted();
    }

    void clear_image()
    {
        std::fill(m_pixels.begin(), m_pixels.end(), 0);
        m_key = 0;
    }

    static constexpr int WIDTH{16};
    static constexpr int HEIGHT{12};
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<LogicalScreen> m_saved_screen{
        g_logical_screen, LogicalScreen{WIDTH, HEIGHT, 0, 0, WIDTH - 1, HEIGHT - 1}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, WIDTH};
    ValueSaver<int> m_saved_height{g_screen_y_dots, HEIGHT};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 15};
    ValueSaver<int> m_saved_size{g_size_dot, 0};
    ValueSaver<AutoShowDot> m_saved_auto{g_auto_show_dot, AutoShowDot::NONE};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<bool> m_saved_overlay{g_show_orbit, false};
    ValueSaver<int> m_saved_fill{g_fill_color, -1};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<SymmetryType> m_saved_force{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<SymmetryType> m_saved_symmetry{g_symmetry};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<int> m_saved_interval{g_keyboard_check_interval};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, std::make_shared<NullSoundOutput>()};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    int m_calculations{};
    int m_flushes{};
    int m_interrupt_after{-1};
    int m_key{};
    bool m_yield_once{};
};

TEST_F(TestShowDotController, disabledMarkerDoesNotTouchImage)
{
    g_show_dot = -1;
    const std::vector<int> original{m_pixels};
    ShowDotController marker;

    EXPECT_FALSE(marker.enabled());
    marker.show({0, 0});
    marker.restore();

    EXPECT_EQ(original, m_pixels);
}

TEST_F(TestShowDotController, restoresThePointAsWellAsTheTriangleAtEveryCorner)
{
    g_size_dot = 128;
    const std::vector<int> original{m_pixels};
    ShowDotController marker;
    for (const Point2i point : {Point2i{0, 0}, Point2i{15, 0}, Point2i{15, 11}, Point2i{0, 11}})
    {
        marker.show(point);
        EXPECT_EQ(15, m_pixels[point.y * WIDTH + point.x]);
        EXPECT_NE(original, m_pixels);
        marker.restore();
        EXPECT_EQ(original, m_pixels);
    }
}

TEST_F(TestShowDotController, restoresSymmetryCopiesWithoutAssumingTheirOriginalColors)
{
    g_size_dot = 128;
    g_i_stop_pt = {7, 5};
    const std::vector<int> original{m_pixels};
    using Plot = void (*)(int, int, int);
    for (const Plot plot : {sym_plot2, sym_plot2y, sym_plot2j, sym_plot4, sym_plot2_basin, sym_plot4_basin})
    {
        g_plot = plot;
        ShowDotController marker;

        marker.show({3, 3});
        EXPECT_NE(original, m_pixels);
        g_plot = put_color_a;
        marker.restore();

        EXPECT_EQ(original, m_pixels);
    }
}

TEST_F(TestShowDotController, restoresPeriodicCopiesAndOverlappingAxisPixels)
{
    ValueSaver saved_period{g_pi_in_pixels, 4};
    g_i_stop_pt = {3, 5};
    const std::vector<int> original{m_pixels};
    using Plot = void (*)(int, int, int);
    for (const Plot plot : {sym_pi_plot, sym_pi_plot2j, sym_pi_plot4j, sym_plot4})
    {
        g_plot = plot;
        ShowDotController marker;
        marker.show({0, 0});
        EXPECT_EQ(15, m_pixels[0]);
        marker.restore();
        EXPECT_EQ(original, m_pixels);
    }
}

TEST_F(TestShowDotController, destructionRestoresAnOutstandingMarker)
{
    const std::vector<int> original{m_pixels};
    {
        ShowDotController marker;
        marker.show({3, 3});
        EXPECT_THROW(marker.show({4, 3}), std::logic_error);
    }
    EXPECT_EQ(original, m_pixels);
}

TEST_F(TestShowDotController, allPixelPassesReadOnlyRestoredImageData)
{
    for (const CalcMode mode : {CalcMode::ONE_PASS, CalcMode::TWO_PASS, CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS,
             CalcMode::DIFFUSION, CalcMode::TESSERAL})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        clear_image();
        g_std_calc_mode = mode;
        g_resuming = false;
        g_size_dot = 128;
        ShowDotController marker;
        StandardFractal calculation;
        calculation.resume();

        for (int chunks = 0; chunks < 1000 && !calculation.done(); ++chunks)
        {
            marker.iterate(calculation);
        }

        EXPECT_TRUE(calculation.done());
        EXPECT_THAT(m_pixels, Each(7));
    }
}

TEST_F(TestShowDotController, zeroDelayRetainsTheOrdinaryScanlineBatch)
{
    clear_image();
    ShowDotController marker;
    StandardFractal calculation;
    calculation.resume();

    marker.iterate(calculation);

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_EQ(0, m_flushes);
}

TEST_F(TestShowDotController, delayHappensWithTheMarkerVisibleButCalculationSeesRestoredPixels)
{
    clear_image();
    g_orbit_delay = 1;
    EXPECT_CALL(m_driver, delay(0)).Times(2).WillRepeatedly(Invoke([this] { EXPECT_EQ(15, m_pixels[0]); }));
    ShowDotController marker;
    StandardFractal calculation;
    calculation.resume();

    marker.iterate(calculation);

    EXPECT_EQ(1, m_calculations);
    EXPECT_EQ(1, m_flushes);
    EXPECT_EQ(7, m_pixels[0]);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
}

TEST_F(TestShowDotController, unfinishedPixelDoesNotRepeatItsMarkerOrDelay)
{
    clear_image();
    g_orbit_delay = 1;
    m_yield_once = true;
    ShowDotController marker;
    StandardFractal calculation;
    calculation.resume();

    marker.iterate(calculation);
    EXPECT_EQ(1, m_flushes);
    EXPECT_THAT(m_pixels, Each(0));
    marker.iterate(calculation);

    EXPECT_EQ(2, m_calculations);
    EXPECT_EQ(1, m_flushes);
    EXPECT_EQ(7, m_pixels[0]);
}

TEST_F(TestShowDotController, menuInterruptionAndResumeLeaveNoMarkerBehind)
{
    clear_image();
    g_orbit_delay = 1;
    g_size_dot = 128;
    m_interrupt_after = 5;

    EXPECT_EQ(-1, render_standard_image());
    EXPECT_EQ('x', m_key);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    m_key = 0;
    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestShowDotController, parameterRestartUsesFreshMarkerState)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    ASSERT_EQ(-1, render_standard_image());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    clear_image();
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_size_dot = 128;
    ValueSaver saved_iterations{g_max_iterations, 300L};

    EXPECT_EQ(0, render_standard_image());

    EXPECT_THAT(m_pixels, Each(7));
}

class TestImageRenderContext : public TestShowDotController
{
protected:
    void SetUp() override;
};

void TestImageRenderContext::SetUp()
{
    TestShowDotController::SetUp();
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = []
    {
        g_dispatch.set_calc_type(calculate_pixel);
        return true;
    };
    g_dispatch = FractalDispatch{m_specific};
}

TEST_F(TestImageRenderContext, requiresStartAndACompletedResult)
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    ImageRenderContext rendering{sound};

    EXPECT_FALSE(rendering.done());
    EXPECT_THROW(rendering.iterate(), std::logic_error);
    EXPECT_THROW(rendering.result(), std::logic_error);
    EXPECT_THROW(rendering.suspend(), std::logic_error);
    EXPECT_EQ(0, m_calculations);

    rendering.start();

    EXPECT_FALSE(rendering.done());
    EXPECT_EQ(0, m_calculations);
    EXPECT_THROW(rendering.start(), std::logic_error);
    rendering.suspend();
    EXPECT_TRUE(rendering.done());
    EXPECT_EQ(0, rendering.result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestImageRenderContext, outerStepRetainsTheExistingScanlineBatch)
{
    clear_image();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    ImageRenderContext rendering{sound};
    rendering.start();

    rendering.iterate();

    ASSERT_TRUE(rendering.done());
    EXPECT_EQ(0, rendering.result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    rendering.iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestImageRenderContext, interruptedOperationReleasesHandlersAndCanResume)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    EXPECT_THAT(m_pixels, Each(Ne(15)));

    m_key = 0;
    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestImageRenderContext, abandoningAnActiveContextAbortsAndReleasesPresentation)
{
    clear_image();
    g_orbit_delay = 1;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    {
        ImageRenderContext rendering{sound};
        rendering.start();
        rendering.iterate();
        EXPECT_FALSE(rendering.done());
    }

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

class TestStandardFractalController : public TestShowDotController
{
};

TEST_F(TestStandardFractalController, requiresStartAndACompletedResult)
{
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};

    EXPECT_FALSE(controller.done());
    EXPECT_THROW(controller.iterate(), std::logic_error);
    EXPECT_THROW(controller.result(), std::logic_error);
    EXPECT_THROW(controller.suspend(), std::logic_error);
    EXPECT_EQ(0, m_calculations);

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(0, m_calculations);
    EXPECT_THROW(controller.start(), std::logic_error);
    controller.suspend();
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
}

TEST_F(TestStandardFractalController, callerAdvancesExistingChunksToCompletion)
{
    clear_image();
    g_orbit_delay = 1;
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};
    controller.start();

    controller.iterate();

    EXPECT_EQ(1, m_calculations);
    EXPECT_FALSE(controller.done());
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    controller.iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestStandardFractalController, smallImageDoesNotIntroduceExtraYields)
{
    clear_image();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};
    controller.start();

    controller.iterate();

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestStandardFractalController, explicitSuspensionRestoresMarkerAndResumesSavedWork)
{
    clear_image();
    g_orbit_delay = 1;
    g_size_dot = 128;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    {
        StandardFractal calculation;
        StandardFractalController controller{calculation, sound};
        controller.start();
        for (int chunks = 0; chunks < 100 && m_calculations == 0; ++chunks)
        {
            ASSERT_FALSE(controller.done());
            controller.iterate();
        }
        ASSERT_GT(m_calculations, 0);

        controller.suspend();

        EXPECT_TRUE(controller.done());
        EXPECT_EQ(-1, controller.result());
        EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
        EXPECT_FALSE(g_resume_data.empty());
        EXPECT_THAT(m_pixels, Each(Ne(15)));
        EXPECT_FALSE(dispatch_keyboard_key('o'));
    }
    StandardFractal resumed;
    SoundSession resumed_sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{resumed, resumed_sound};
    controller.start();
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestStandardFractalController, interruptedStepReleasesHandlersBeforeReturning)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};
    controller.start();

    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestStandardFractalController, failedStepAbortsCalculationAndReleasesPresentation)
{
    clear_image();
    g_orbit_delay = 1;
    s_calculate = []() -> int { throw std::runtime_error("Calculation failed"); };
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};
    controller.start();
    bool failed{};

    for (int chunks = 0; chunks < 100 && !failed; ++chunks)
    {
        try
        {
            controller.iterate();
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ("Calculation failed", error.what());
            failed = true;
        }
    }

    EXPECT_TRUE(failed);
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_THAT(m_pixels, Each(0));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

class TestLyapunovController : public TestShowDotController
{
protected:
    void SetUp() override
    {
        TestShowDotController::SetUp();
        g_inside_method = ColorMethod::COLOR;
        g_fractal_type = FractalType::LYAPUNOV;
        m_specific.type = FractalType::LYAPUNOV;
    }
};

TEST_F(TestLyapunovController, requiresStartAndACompletedResult)
{
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};

    EXPECT_FALSE(controller.done());
    EXPECT_THROW(controller.iterate(), std::logic_error);
    EXPECT_THROW(controller.result(), std::logic_error);
    EXPECT_THROW(controller.suspend(), std::logic_error);
    EXPECT_EQ(0, m_calculations);

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(0, m_calculations);
    EXPECT_THROW(controller.start(), std::logic_error);
    controller.suspend();
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestLyapunovController, callerAdvancesExistingMarkerChunksToCompletion)
{
    clear_image();
    g_orbit_delay = 1;
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};
    controller.start();

    controller.iterate();

    EXPECT_EQ(1, m_calculations);
    EXPECT_FALSE(controller.done());
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    controller.iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestLyapunovController, zeroDelayRetainsSingleChunkCompletionWithOrWithoutMarker)
{
    for (const int show_dot : {-1, 15})
    {
        SCOPED_TRACE(show_dot);
        clear_image();
        m_calculations = 0;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        g_show_dot = show_dot;
        Lyapunov lyapunov;
        LyapunovController controller{lyapunov};
        controller.start();

        controller.iterate();

        ASSERT_TRUE(controller.done());
        EXPECT_EQ(0, controller.result());
        EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
        EXPECT_THAT(m_pixels, Each(7));
    }
}

TEST_F(TestLyapunovController, explicitSuspensionRestoresMarkerAndResumesSavedWork)
{
    clear_image();
    g_orbit_delay = 1;
    g_size_dot = 128;
    {
        Lyapunov lyapunov;
        LyapunovController controller{lyapunov};
        controller.start();
        controller.iterate();
        ASSERT_EQ(1, m_calculations);

        controller.suspend();

        EXPECT_TRUE(controller.done());
        EXPECT_EQ(-1, controller.result());
        EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
        EXPECT_FALSE(g_resume_data.empty());
        EXPECT_THAT(m_pixels, Each(Ne(15)));
        EXPECT_FALSE(dispatch_keyboard_key('o'));
    }
    Lyapunov resumed;
    LyapunovController controller{resumed};
    controller.start();
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestLyapunovController, interruptedStepReleasesHandlersBeforeReturning)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};
    controller.start();

    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestLyapunovController, orbitToggleDoesNotInterruptCalculation)
{
    clear_image();
    g_orbit_delay = 1;
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};
    controller.start();
    controller.iterate();

    EXPECT_TRUE(dispatch_keyboard_key('o'));
    EXPECT_TRUE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_TRUE(dispatch_keyboard_key('O'));
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_FALSE(controller.done());

    controller.suspend();

    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestLyapunovController, failedStepAbortsCalculationAndReleasesPresentation)
{
    clear_image();
    g_orbit_delay = 1;
    s_calculate = []() -> int { throw std::runtime_error("Calculation failed"); };
    Lyapunov lyapunov;
    LyapunovController controller{lyapunov};
    controller.start();

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_THAT(m_pixels, Each(0));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

} // namespace id::test
