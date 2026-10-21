// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/calmanfp.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/log_map.h>
#include <engine/OrbitPlot.h>
#include <engine/PertEngine.h>
#include <engine/pixel_grid.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/SoundOutput.h>
#include <engine/StandardPass.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/FrothyBasin.h>
#include <fractals/lorenz.h>
#include <fractals/lyapunov.h>
#include <io/library.h>
#include <io/check_write_file.h>
#include <geometry/line3d.h>
#include <misc/ValueSaver.h>
#include <misc/debug_flags.h>
#include <ui/calculate_image.h>
#include <ui/frothy_basin.h>
#include <ui/KeyboardHandler.h>
#include <ui/id_keys.h>
#include <ui/SoundSession.h>
#include <ui/standard_fractal.h>
#include <ui/standard_orbit_plot.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#define private public
#include <engine/StandardFractal.h>
#undef private

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

class MockOverlaySound : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

int s_orbit_calls{};
int s_pixel_calls{};
std::vector<Point2i> s_pixel_positions;

int calculate_overlay_orbit()
{
    ++s_orbit_calls;
    g_new_z = {1.0, 1.0};
    g_old_z = g_new_z;
    return 0;
}

int initialize_overlay_pixel()
{
    ++s_pixel_calls;
    s_pixel_positions.push_back({g_col, g_row});
    g_old_z = {};
    return 0;
}

bool initialize_overlay_image()
{
    return true;
}

} // namespace

class TestOverlaySound : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockOverlaySound>> m_output{std::make_shared<StrictMock<MockOverlaySound>>()};
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_output};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<int> m_saved_x_dots{g_screen_x_dots, 2};
    ValueSaver<int> m_saved_y_dots{g_screen_y_dots, 2};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_and_color{g_and_color, 15};
    ValueSaver<int> m_saved_orbit_color{g_orbit_color, 15};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, true};
    ValueSaver<bool> m_saved_orbit_save_flag{g_orbit_save_flag, false};
    ValueSaver<int> m_saved_show_dot{g_show_dot, -1};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 2};
    ValueSaver<long> m_saved_distance_estimator{g_distance_estimator, 0};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<int> m_saved_check_interval{g_keyboard_check_interval, 10000};
    ValueSaver<int> m_saved_max_check_interval{g_max_keyboard_check_interval, 10000};
    ValueSaver<CalcMode> m_saved_calc_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<SymmetryType> m_saved_force_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<ColorState> m_saved_color_state{g_color_state, ColorState::UNKNOWN_MAP};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<double> m_saved_mx1{g_plot_mx1, 1.0};
    ValueSaver<double> m_saved_mx2{g_plot_mx2, 0.0};
    ValueSaver<double> m_saved_my1{g_plot_my1, 1.0};
    ValueSaver<double> m_saved_my2{g_plot_my2, 0.0};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{1, 1}};
    ValueSaver<int> m_saved_col{g_col, 0};
    ValueSaver<int> m_saved_row{g_row, 0};
    ValueSaver<double> m_saved_param0{g_params[0], 1.0};
    ValueSaver<double> m_saved_param1{g_params[1], 0.0};
    ValueSaver<double> m_saved_param2{g_params[2], 1.02871376822};
    std::vector<std::string> m_events;
    std::vector<SoundSample> m_samples;
    std::filesystem::path m_directory;
    int m_plots{};
    int m_key{};
    bool m_ready{true};
};

void TestOverlaySound::SetUp()
{
    reset_calc_interrupted();
    s_orbit_calls = 0;
    s_pixel_calls = 0;
    s_pixel_positions.clear();
    m_specific.flags = FractalFlags::NONE;
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.orbit_calc = calculate_overlay_orbit;
    m_specific.per_pixel = initialize_overlay_pixel;
    m_specific.per_image = initialize_overlay_image;
    m_specific.calc_type = standard_fractal_type;
    g_dispatch = FractalDispatch{m_specific};
    g_image_region.m_min = {};
    g_image_region.m_max = {};
    g_image_region.m_3rd = {};
    alloc_pixel_grid();
    fill_pixel_grid();
    set_normal_dot();
    set_normal_span();
    EXPECT_CALL(m_driver, read_pixel(_, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int, int, int)
            {
                ++m_plots;
                m_events.emplace_back("plot");
            }));
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([this] { return m_key; }));
    EXPECT_CALL(m_driver, get_key())
        .WillRepeatedly(Invoke(
            [this]
            {
                const int key{m_key};
                m_key = 0;
                return key;
            }));
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_events.emplace_back("sample");
                m_samples.push_back(sample);
                return true;
            }));
    EXPECT_CALL(*m_output, begin_orbit()).WillRepeatedly(Invoke([this] { m_events.emplace_back("boundary"); }));
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Invoke([this] { return m_ready; }));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestOverlaySound::TearDown()
{
    orbit_plot().scrub();
    free_pixel_grid();
    end_resume();
    reset_calc_interrupted();
    if (!m_directory.empty())
    {
        clear_save_library();
        std::error_code ignored;
        std::filesystem::remove_all(m_directory, ignored);
    }
}

TEST_F(TestOverlaySound, acceptsVisibleSampleBeforePlotAndDoesNotResubmitCompletedPoint)
{
    OrbitPlot plot;
    plot.reset(1.0, 1.0);

    EXPECT_TRUE(plot.iterate());
    EXPECT_TRUE(plot.done());
    EXPECT_FALSE(plot.iterate());

    EXPECT_THAT(m_events, ElementsAre("sample", "plot"));
    ASSERT_EQ(1U, m_samples.size());
    EXPECT_EQ(1.0, m_samples[0].x);
    EXPECT_EQ(1.0, m_samples[0].y);
    EXPECT_EQ(0.0, m_samples[0].z);
    plot.scrub();
}

TEST_F(TestOverlaySound, scrubbingDisplayMutesTheBoundOutputBeforeRestoringPixels)
{
    OrbitPlot &plot{orbit_plot()};
    plot.reset(1.0, 1.0);
    ASSERT_TRUE(plot.iterate());
    ASSERT_TRUE(g_orbit_save_flag);
    EXPECT_CALL(m_driver, mute()).Times(0);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(*m_output, finish()).Times(0);
    EXPECT_CALL(*m_output, mute()).WillOnce(Invoke([this] { m_events.emplace_back("mute"); }));

    scrub_orbit_display();

    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_THAT(m_events, ElementsAre("sample", "plot", "mute", "plot"));
}

TEST_F(TestOverlaySound, clipsPointsBeforeSubmittingSound)
{
    OrbitPlot plot;
    for (const DComplex point : {DComplex{-1.0, 0.0}, DComplex{0.0, 3.0}})
    {
        plot.reset(point.x, point.y);
        EXPECT_FALSE(plot.iterate());
        EXPECT_TRUE(plot.done());
    }
    EXPECT_TRUE(m_events.empty());
}

TEST_F(TestOverlaySound, refusalLeavesPointUnplottedUntilAccepted)
{
    OrbitPlot plot;
    plot.reset(1.0, 1.0);
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(true));

    EXPECT_FALSE(plot.iterate());
    EXPECT_FALSE(plot.done());
    EXPECT_EQ(0, m_plots);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_TRUE(plot.iterate());
    EXPECT_TRUE(plot.done());
    EXPECT_EQ(1, m_plots);
    plot.scrub();
}

TEST_F(TestOverlaySound, synchronousStandardPixelReturnsOnRefusalWithoutRepeatingOrbitMathOrBoundary)
{
    StandardFractal calculation;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));

    EXPECT_EQ(-1, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(-1, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(0, m_plots);
    EXPECT_GE(calculation.calculate_standard_pixel(false), 0);

    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(1, std::count(m_events.begin(), m_events.end(), "boundary"));
}

TEST_F(TestOverlaySound, standardOrbitYieldsWithoutPollingOrRepeatingIterationWork)
{
    g_show_orbit = false;
    g_show_dot = 15;
    g_orbit_delay = 20;
    g_max_iterations = 2050;
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::ONE_PASS);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_EQ(-1, calculation.calculate_standard_pixel(true));
    EXPECT_TRUE(calculation.consume_traversal_yield());
    EXPECT_EQ(2048, g_color_iter);
    EXPECT_EQ(2047, s_orbit_calls);
    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_EQ(2049, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

TEST_F(TestOverlaySound, standardOrbitRetainsSoundWorkFrequency)
{
    g_show_orbit = false;
    g_orbit_delay = 20;
    g_max_iterations = 18;
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_X};
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::ONE_PASS);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_EQ(-1, calculation.calculate_standard_pixel(true));
    EXPECT_TRUE(calculation.consume_traversal_yield());
    EXPECT_EQ(16, g_color_iter);
    EXPECT_EQ(15, s_orbit_calls);
    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_EQ(17, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

TEST_F(TestOverlaySound, standardPixelKeepsNonIncrementalTraversalGate)
{
    g_show_orbit = false;
    g_max_iterations = 2050;
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::SOLID_GUESS);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_FALSE(calculation.consume_traversal_yield());
    EXPECT_EQ(2049, s_orbit_calls);
}

TEST_F(TestOverlaySound, controllerSuspendsWithinAnOrbitAndResumesUnfinishedPixel)
{
    g_show_orbit = false;
    g_max_iterations = 2050;
    m_key = 'x';
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, standard_fractal());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(2047, s_orbit_calls);
    EXPECT_EQ(0, m_plots);

    g_resuming = true;

    EXPECT_EQ(0, standard_fractal());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4, m_plots);
    EXPECT_EQ(5, s_pixel_calls);
}

TEST_F(TestOverlaySound, optimizedPixelYieldsCompletedWorkWithoutPolling)
{
    g_show_orbit = false;
    g_keyboard_check_interval = 0;
    calc_mandelbrot_init();
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::ONE_PASS);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_GE(calculation.calculate_mandelbrot_pixel(), 0);

    EXPECT_TRUE(calculation.consume_traversal_yield());
}

TEST_F(TestOverlaySound, scanlineSuspensionRetainsTheNextPixelInBothPassModes)
{
    g_show_orbit = false;
    g_max_keyboard_check_interval = 1;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    for (const int primary_pixels : {1, 2})
    {
        for (const CalcMode mode : {CalcMode::ONE_PASS, CalcMode::TWO_PASS})
        {
            g_std_calc_mode = mode;
            g_resuming = false;
            s_pixel_positions.clear();
            StandardFractal calculation;
            calculation.resume();
            for (int increments = 0; increments < primary_pixels; ++increments)
            {
                calculation.iterate();
            }
            ASSERT_EQ(primary_pixels, s_pixel_positions.size());
            calculation.suspend();
            ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);

            g_resuming = true;
            StandardFractal resumed;
            resumed.resume();
            for (int increments = 0; increments < 20 && !resumed.done(); ++increments)
            {
                resumed.iterate();
            }
            ASSERT_TRUE(resumed.done());
            ASSERT_EQ(4U, s_pixel_positions.size());
            EXPECT_EQ((Point2i{0, 0}), s_pixel_positions[0]);
            EXPECT_EQ((Point2i{1, 0}), s_pixel_positions[1]);
            EXPECT_EQ((Point2i{0, 1}), s_pixel_positions[2]);
            EXPECT_EQ((Point2i{1, 1}), s_pixel_positions[3]);
        }
    }
}

TEST_F(TestOverlaySound, perturbationPixelRetainsTraversalYieldGateWithoutPolling)
{
    m_specific.flags = FractalFlags::PERTURB;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    for (const CalcMode mode : {CalcMode::ONE_PASS, CalcMode::SOLID_GUESS})
    {
        StandardFractal calculation;
        calculation.m_standard_pass.select(mode);
        calculation.m_perturbation_active = true;
        calculation.m_pert_engine.initialize_frame({0.0, 0.0}, 1.0);

        EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

        EXPECT_EQ(mode == CalcMode::ONE_PASS, calculation.consume_traversal_yield());
        calculation.m_pert_engine.finish();
    }
}

TEST_F(TestOverlaySound, lyapunovScanlineYieldsCompletedPixelWithoutPolling)
{
    g_show_orbit = false;
    g_params[1] = 0.5;
    m_specific.calc_type = lyapunov_type;
    m_specific.per_image = lyapunov_per_image;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(2, m_plots);
    calculation.suspend();
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
}

TEST_F(TestOverlaySound, imageSetupReturnsLogMapWarningWithoutPolling)
{
    g_colors = 2;
    ValueSaver saved_log_map{g_log_map_flag, 1L};
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    ImageCalculation image;

    EXPECT_THAT(image.start(), ElementsAre("Need at least 16 colors to use logmap"));

    EXPECT_EQ(0, g_log_map_flag);
    image.finish();
}

TEST_F(TestOverlaySound, imageSetupReturnsTargaFailureAndKeepsPaletteRenderingAvailable)
{
    m_directory = std::filesystem::current_path() / "test-image-targa-fallback";
    std::filesystem::create_directories(m_directory / "image" / "blocked.tga");
    set_save_library(m_directory);
    ValueSaver saved_true_color{g_true_color, true};
    ValueSaver saved_light_name{geometry::g_light_name, std::string{"blocked.tga"}};
    ValueSaver saved_overwrite{g_overwrite_file, true};
    const std::string path{(m_directory / "image" / "blocked.tga").string()};
    ImageCalculation image;

    EXPECT_THAT(image.start(), ElementsAre("OOPS, couldn't open  < " + path + " >"));

    EXPECT_FALSE(g_true_color);
    EXPECT_EQ(put_color_a, g_put_color);
    image.finish();
}

TEST_F(TestOverlaySound, failedImageSetupClearsResumeStateWithoutChargingThePreviousTimer)
{
    ValueSaver saved_time{g_calc_time, 123L};
    g_calc_status = CalcStatus::NON_RESUMABLE;
    g_num_work_list = 1;
    g_resume_data = {1, 2, 3};
    ImageCalculation image;

    image.finish();

    EXPECT_EQ(123L, g_calc_time);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

class TestStandardRenderDiagnostic : public TestOverlaySound
{
protected:
    void expect_diagnostic(const char *message)
    {
        EXPECT_CALL(m_driver, stack_screen());
        EXPECT_CALL(m_driver, unstack_screen());
        EXPECT_CALL(m_driver, move_cursor(_, _));
        EXPECT_CALL(m_driver, hide_text_cursor());
        EXPECT_CALL(m_driver, set_attr(_, _, _, _));
        EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
        EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(m_driver, put_string(_, _, _, StrEq(message)));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
        EXPECT_CALL(*m_output, mute());
    }

    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, false};
};

TEST_F(TestStandardRenderDiagnostic, boundaryValidationReturnsToUiAndAllowsCorrection)
{
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    g_inside_method = ColorMethod::COLOR;
    ValueSaver saved_inside_color{g_inside_color, 0};
    const FractalDispatch dispatch{g_dispatch};
    expect_diagnostic("Boundary tracing cannot be used with inside=0 or outside=0");

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    g_inside_method = ColorMethod::ITER;
    EXPECT_EQ(0, calculate_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_F(TestStandardRenderDiagnostic, allocationFailureDiscardsWorkAndReportsToUi)
{
    g_dispatch.set_per_image([]() -> bool { throw std::bad_alloc{}; });
    const FractalDispatch dispatch{g_dispatch};
    expect_diagnostic("Insufficient free memory for calculation");

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    EXPECT_TRUE(g_log_map_table.empty());
}

TEST_F(TestOverlaySound, pendingStandardOverlayRetainsRejectedCandidate)
{
    StandardFractal calculation;
    calculation.queue_overlay_orbit_plot(1.0, 1.0);
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(true));

    calculation.iterate();
    EXPECT_FALSE(calculation.consume_orbit_overlay_displayed());
    EXPECT_EQ(0, m_plots);
    calculation.iterate();

    EXPECT_TRUE(calculation.consume_orbit_overlay_displayed());
    EXPECT_EQ(1, m_plots);
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_F(TestOverlaySound, soundRetryDoesNotRecalculateDistanceEstimatorDerivative)
{
    ValueSaver saved_distance_estimator{g_distance_estimator, 1L};
    ValueSaver saved_iteration{g_color_iter, 1L};
    StandardFractal calculation;
    calculation.m_standard_pixel_active = true;
    calculation.m_standard_pixel_iteration_started = true;
    calculation.m_standard_pixel_orbit_calc_completed = true;
    calculation.m_check_freq = 2048;
    calculation.m_deriv = {3.0, 2.0};
    g_old_z = {1.0, 1.0};
    g_new_z = g_old_z;
    EXPECT_CALL(*m_output, play(_)).WillRepeatedly(Return(false));

    EXPECT_EQ(-1, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(-1, calculation.calculate_standard_pixel(false));

    EXPECT_EQ((DComplex{3.0, 2.0}), calculation.m_deriv);
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_EQ(0, m_plots);
}

TEST_F(TestOverlaySound, synchronousStandardPixelRetainsOverlayWhileLastSampleIsPlaying)
{
    StandardFractal calculation;
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_ready = false;
                return true;
            }));

    EXPECT_EQ(-1, calculation.calculate_standard_pixel(false));
    EXPECT_TRUE(g_orbit_save_flag);
    EXPECT_EQ(1, m_plots);

    m_ready = true;
    calculation.iterate();
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_GE(calculation.calculate_standard_pixel(false), 0);
    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

TEST_F(TestOverlaySound, frothyBasinRetriesWithoutAdvancingOrbitOrRepeatingBoundary)
{
    FrothyBasin calculation;
    ASSERT_TRUE(calculation.per_image());
    calculation.resume();
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(true));

    calculation.iterate();
    const DComplex candidate{g_old_z};
    const long iteration{g_color_iter};
    EXPECT_EQ(0, m_plots);
    calculation.iterate();
    EXPECT_EQ(candidate, g_old_z);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(0, m_plots);
    calculation.iterate();

    EXPECT_TRUE(calculation.consume_orbit_overlay_displayed());
    EXPECT_EQ(candidate, g_old_z);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(1, m_plots);
    EXPECT_EQ(1, std::count(m_events.begin(), m_events.end(), "boundary"));
}

class TestOverlaySoundController : public TestOverlaySound, public WithParamInterface<bool>
{
protected:
    int render(SoundSession &sound);
};

int TestOverlaySoundController::render(SoundSession &sound)
{
    return GetParam() ? froth_type(sound) : standard_fractal(sound);
}

TEST_P(TestOverlaySoundController, normalCompletionFinishesWithoutMuting)
{
    SoundSession sound{m_output};
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));

    EXPECT_EQ(0, render(sound));

    EXPECT_FALSE(m_samples.empty());
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_P(TestOverlaySoundController, evolutionCellsShareOneOutputUntilTheOwnerFinishes)
{
    SoundSession owner{m_output};
    int completed_cells{};
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish())
        .WillOnce(Invoke(
            [&completed_cells]
            {
                EXPECT_EQ(2, completed_cells);
                return true;
            }));

    for (; completed_cells < 2; ++completed_cells)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        g_resuming = false;

        EXPECT_EQ(0, GetParam() ? froth_type() : standard_fractal());

        EXPECT_EQ(m_output, g_sound);
        EXPECT_EQ((completed_cells + 1) * 4, std::count(m_events.begin(), m_events.end(), "boundary"));
    }

    EXPECT_EQ(GetParam() ? 16U : 8U, m_samples.size());
    EXPECT_TRUE(owner.finish());
}

TEST_P(TestOverlaySoundController, servicesBackpressureBeforePlottingNextPoint)
{
    SoundSession sound{m_output};
    int pending_updates{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &)
            {
                EXPECT_TRUE(m_ready);
                m_ready = false;
                return true;
            }));
    EXPECT_CALL(*m_output, update())
        .WillRepeatedly(Invoke(
            [this, &pending_updates]
            {
                if (!m_ready)
                {
                    ++pending_updates;
                    m_ready = true;
                }
            }));

    EXPECT_EQ(0, render(sound));
    EXPECT_GT(pending_updates, 0);
}

TEST_P(TestOverlaySoundController, retriesRefusedSampleBeforePlottingWithoutRepeatingThePixel)
{
    SoundSession sound{m_output};
    int attempts{};
    SoundSample rejected{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, &attempts, &rejected](const SoundSample &sample)
            {
                ++attempts;
                if (attempts == 1)
                {
                    EXPECT_EQ(0, m_plots);
                    rejected = sample;
                    return false;
                }
                if (attempts == 2)
                {
                    EXPECT_EQ(0, m_plots);
                    EXPECT_EQ(rejected.x, sample.x);
                    EXPECT_EQ(rejected.y, sample.y);
                }
                m_samples.push_back(sample);
                return true;
            }));

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(GetParam() ? 8U : 4U, m_samples.size());
    EXPECT_EQ(m_samples.size() + 1, attempts);
    if (!GetParam())
    {
        EXPECT_EQ(4, s_orbit_calls);
        EXPECT_EQ(4, s_pixel_calls);
    }
}

TEST_P(TestOverlaySoundController, completionServicesReleaseTailWithoutMoreCalculation)
{
    SoundSession sound{m_output};
    int final_plots{};
    {
        InSequence sequence;
        EXPECT_CALL(*m_output, finish())
            .WillOnce(Invoke(
                [this, &final_plots]
                {
                    final_plots = m_plots;
                    return false;
                }));
        EXPECT_CALL(m_driver, delay(1));
        EXPECT_CALL(*m_output, finish())
            .WillOnce(Invoke(
                [this, &final_plots]
                {
                    EXPECT_EQ(final_plots, m_plots);
                    return true;
                }));
    }
    EXPECT_CALL(*m_output, mute()).Times(0);

    EXPECT_EQ(0, render(sound));
    EXPECT_GT(final_plots, 0);
}

TEST_P(TestOverlaySoundController, recordsBoundariesAndSamplesWhenDriverDoesNotSupportPlayback)
{
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver saved_orbit_flags{g_orbit_save_flags, OSF_MIDI};
    m_directory = std::filesystem::temp_directory_path() /
        ("id-overlay-sound-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    set_save_library(m_directory);
    std::vector<int> tones;
    EXPECT_CALL(m_driver, sound_on(_))
        .WillRepeatedly(Invoke(
            [&tones](const int frequency)
            {
                tones.push_back(frequency);
                return false;
            }));
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    {
        SoundOutputPtr output{std::make_shared<DriverSoundOutput>(
            DriverSoundOutput::Source::OVERLAY, [] { return DriverSoundOutput::Clock::time_point{}; },
            [] { return 1234LL; })};
        SoundSession owner{output};
        for (int cell = 0; cell < 2; ++cell)
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            g_resuming = false;
            EXPECT_EQ(0, GetParam() ? froth_type() : standard_fractal());
        }
        EXPECT_TRUE(owner.finish());
    }

    std::ifstream file{m_directory / "sound/sound001.txt"};
    ASSERT_TRUE(file.good());
    std::vector<int> recorded_tones;
    int boundaries{};
    for (std::string line; std::getline(file, line);)
    {
        if (line == "time=1234")
        {
            ++boundaries;
        }
        else
        {
            recorded_tones.push_back(std::stoi(line));
        }
    }
    EXPECT_EQ(8, boundaries);
    EXPECT_FALSE(tones.empty());
    EXPECT_EQ(tones, recorded_tones);
}

TEST_P(TestOverlaySoundController, togglingOverlayOffMutesWithoutCancellingImage)
{
    SoundSession sound{m_output};
    bool toggled{};
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'o';
                m_ready = false;
                return true;
            }));
    EXPECT_CALL(*m_output, mute())
        .WillOnce(Invoke(
            [this, &toggled]
            {
                toggled = true;
                m_ready = true;
            }));
    EXPECT_CALL(m_driver, delay(1)).Times(AnyNumber());

    EXPECT_EQ(0, render(sound));

    EXPECT_TRUE(toggled);
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_P(TestOverlaySoundController, menuInterruptMutesAndResumingRestartsOutput)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'x';
                m_ready = false;
                return true;
            }));
    EXPECT_CALL(*m_output, mute()).WillRepeatedly(Invoke([this] { m_ready = true; }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(-1, render(sound));
    }
    EXPECT_TRUE(g_show_orbit);
    EXPECT_EQ('x', m_key);
    EXPECT_FALSE(g_orbit_save_flag);

    m_key = 0;
    g_resuming = true;
    EXPECT_CALL(*m_output, play(_)).WillRepeatedly(Return(true));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(0, render(sound));
    }
    EXPECT_TRUE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
}

INSTANTIATE_TEST_SUITE_P(StandardAndFrothyBasin, TestOverlaySoundController, Values(false, true));

} // namespace id::test
