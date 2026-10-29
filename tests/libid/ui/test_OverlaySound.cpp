// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"
#include "TestRender.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/calmanfp.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/log_map.h>
#include <engine/NonStandardFractal.h>
#include <engine/OrbitPlot.h>
#include <engine/Potential.h>
#include <engine/PertEngine.h>
#include <engine/pixel.h>
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
#include <io/disk_storage.h>
#include <io/special_dirs.h>
#include <io/check_write_file.h>
#include <geometry/line3d.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>
#include <misc/memory.h>
#include <misc/debug_flags.h>
#include <ui/diskvid.h>
#include <ui/FrothyBasinController.h>
#include <ui/calculate_image.h>
#include <ui/KeyboardHandler.h>
#include <ui/id_keys.h>
#include <ui/SoundSession.h>
#include <ui/StandardFractalController.h>
#include <ui/standard_orbit_plot.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#define private public
#include <engine/StandardFractal.h>
#undef private

#include <ui/ImageRenderContext.h>

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

int unexpected_froth_renderer()
{
    throw std::logic_error("FrothyBasin selection entered a renderer callback");
}

bool unexpected_froth_setup()
{
    throw std::logic_error("FrothyBasin selection entered legacy per-image setup");
}

} // namespace

class TestOverlaySound : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void select_froth();

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

void TestOverlaySound::select_froth()
{
    g_fractal_type = FractalType::FROTHY_BASIN;
    m_specific = *get_fractal_specific(g_fractal_type);
    m_specific.calc_type = unexpected_froth_renderer;
    m_specific.per_image = unexpected_froth_setup;
    g_dispatch = FractalDispatch{m_specific};
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

TEST_F(TestOverlaySound, completedBifurcationDoesNotInheritThePreviousRenderInterruption)
{
    ValueSaver saved_start_orbit{g_start_show_orbit, true};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 1};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 1};
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
        SoundSession owner{m_output};
        SoundSession borrowed{g_sound, SoundSession::Ownership::BORROWED};
        ImageRenderContext rendering{borrowed};
        rendering.start();
        for (int step = 0; step < 100 && !rendering.done(); ++step)
        {
            rendering.iterate();
        }
        ASSERT_TRUE(rendering.done());
        EXPECT_EQ(-1, rendering.result());
        EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
        EXPECT_EQ(-1, finish_render_sound(owner, rendering.result(), rendering.failure_reported()));
    }
    ASSERT_TRUE(calc_interrupt_requested());
    ASSERT_EQ('x', m_key);

    // The main loop consumes the menu key before selecting the next image.
    m_key = 0;
    g_start_show_orbit = false;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_fractal_type = FractalType::BIFURCATION;
    m_specific = *get_fractal_specific(g_fractal_type);
    m_specific.orbit_calc = [] { return 0; };
    g_dispatch = FractalDispatch{m_specific};
    g_params[0] = 1;
    g_params[1] = 0.66;
    g_image_region = ImageRegion{{2, 0}, {3, 1}, {2, 0}};
    const int plots{m_plots};
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession owner{m_output};
    SoundSession borrowed{g_sound, SoundSession::Ownership::BORROWED};
    ImageRenderContext rendering{borrowed};

    rendering.start();
    EXPECT_FALSE(calc_interrupt_requested());
    for (int step = 0; step < 100 && !rendering.done(); ++step)
    {
        rendering.iterate();
    }

    ASSERT_TRUE(rendering.done());
    EXPECT_GT(m_plots, plots);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(0, rendering.result());
    EXPECT_EQ(0, finish_render_sound(owner, rendering.result(), rendering.failure_reported()));
    EXPECT_FALSE(calc_interrupt_requested());
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

struct ClippedOverlayPoint
{
    const char *name;
    DComplex point;
};

void PrintTo(const ClippedOverlayPoint &input, std::ostream *out)
{
    *out << input.name;
}

class TestClippedOverlaySound : public TestOverlaySound, public WithParamInterface<ClippedOverlayPoint>
{
};

TEST_P(TestClippedOverlaySound, clipsPointBeforeSubmittingSound)
{
    OrbitPlot plot;
    plot.reset(GetParam().point.x, GetParam().point.y);

    EXPECT_FALSE(plot.iterate());
    EXPECT_TRUE(plot.done());

    EXPECT_TRUE(m_events.empty());
}

INSTANTIATE_TEST_SUITE_P(Points, TestClippedOverlaySound,
    Values(ClippedOverlayPoint{"NegativeX", {-1.0, 0.0}}, ClippedOverlayPoint{"LargeY", {0.0, 3.0}}),
    [](const TestParamInfo<ClippedOverlayPoint> &info) { return info.param.name; });

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

    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(0, m_plots);
    EXPECT_GE(calculation.calculate_standard_pixel(false), 0);

    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(1, std::count(m_events.begin(), m_events.end(), "boundary"));
}

struct StandardOrbitCheckpointInput
{
    const char *name;
    int sound_flag;
    int show_dot;
    long max_iterations;
    long yield_iteration;
    int orbit_calls_before;
    int orbit_calls_after;
};

void PrintTo(const StandardOrbitCheckpointInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStandardOrbitCheckpoint : public TestOverlaySound, public WithParamInterface<StandardOrbitCheckpointInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_sound_flag{g_sound_flag};
};

void TestStandardOrbitCheckpoint::SetUp()
{
    TestOverlaySound::SetUp();
    g_show_orbit = false;
    g_show_dot = GetParam().show_dot;
    g_orbit_delay = 20;
    g_max_iterations = GetParam().max_iterations;
    g_sound_flag = GetParam().sound_flag;
}

TEST_P(TestStandardOrbitCheckpoint, yieldsWithoutPollingOrRepeatingIterationWork)
{
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::ONE_PASS);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(true));
    EXPECT_TRUE(calculation.consume_traversal_yield());
    EXPECT_EQ(GetParam().yield_iteration, g_color_iter);
    EXPECT_EQ(GetParam().orbit_calls_before, s_orbit_calls);
    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_EQ(GetParam().orbit_calls_after, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

INSTANTIATE_TEST_SUITE_P(Checkpoints, TestStandardOrbitCheckpoint,
    Values(StandardOrbitCheckpointInput{"Marker", SOUNDFLAG_BEEP, 15, 2050, 2048, 2047, 2049},
        StandardOrbitCheckpointInput{"Sound", SOUNDFLAG_X, -1, 18, 16, 15, 17}),
    [](const TestParamInfo<StandardOrbitCheckpointInput> &info) { return info.param.name; });

class TestStandardPixelCheckpoints : public TestOverlaySound, public WithParamInterface<CalcMode>
{
};

TEST_P(TestStandardPixelCheckpoints, unfinishedPixelRetainsSelectionAndResumesAtEachCheckpoint)
{
    g_show_orbit = false;
    g_max_iterations = 4098;
    g_std_calc_mode = GetParam();
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    StandardFractal calculation;
    calculation.resume();
    bool selected{};
    for (int chunks = 0; chunks < 16 && !selected; ++chunks)
    {
        selected = calculation.select_pixel();
    }
    ASSERT_TRUE(selected);
    const Point2i point{calculation.selected_pixel()};

    for (const int checkpoint : {2048, 4096})
    {
        EXPECT_FALSE(calculation.calculate_pixel());
        EXPECT_TRUE(calculation.consume_traversal_yield());
        EXPECT_EQ(checkpoint, g_color_iter);
        EXPECT_EQ(checkpoint - 1, s_orbit_calls);
        EXPECT_EQ(1, s_pixel_calls);
        EXPECT_EQ(0, m_plots);
        ASSERT_TRUE(calculation.select_pixel());
        EXPECT_EQ(point, calculation.selected_pixel());
    }

    EXPECT_TRUE(calculation.calculate_pixel());

    EXPECT_FALSE(calculation.consume_traversal_yield());
    EXPECT_EQ(4097, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_GT(m_plots, 0);
}

TEST_P(TestStandardPixelCheckpoints, completedPixelRetainsPassBatching)
{
    g_show_orbit = false;
    g_keyboard_check_interval = 0;
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    StandardFractal calculation;
    calculation.m_standard_pass.select(GetParam());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_FALSE(calculation.consume_traversal_yield());
    EXPECT_EQ(1, s_orbit_calls);
}

TEST_P(TestStandardPixelCheckpoints, atomicPixelDoesNotYieldAtOrbitCheckpoints)
{
    g_show_orbit = false;
    g_max_iterations = 4098;
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    StandardFractal calculation;
    calculation.m_standard_pass.select(GetParam());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    EXPECT_GE(calculation.calculate_standard_pixel(false), 0);

    EXPECT_FALSE(calculation.consume_traversal_yield());
    EXPECT_EQ(4097, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

TEST_P(TestStandardPixelCheckpoints, controllerHandlesMenuKeyBeforeCompletingThePixel)
{
    g_show_orbit = false;
    g_max_iterations = 4098;
    g_std_calc_mode = GetParam();
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    bool requested{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(
            [this, &requested]
            {
                if (!requested && s_orbit_calls >= 2047)
                {
                    requested = true;
                    m_key = 'x';
                }
                return m_key;
            });
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_TRUE(requested);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(2047, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(0, m_plots);
    EXPECT_FALSE(g_resume_data.empty());

    g_resuming = true;
    EXPECT_EQ(0, render_standard_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GT(s_pixel_calls, 1);
    EXPECT_GT(m_plots, 0);
}

INSTANTIATE_TEST_SUITE_P(
    PixelPasses, TestStandardPixelCheckpoints, Values(CalcMode::SOLID_GUESS, CalcMode::DIFFUSION, CalcMode::TESSERAL));

TEST_F(TestOverlaySound, controllerInterruptsIncompletePerturbationReferenceAndResumesImage)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver saved_param{g_param_z1, DComplex{}};
    g_user.perturbation = PerturbationMode::YES;
    g_user.std_calc_mode = CalcMode::ONE_PASS;
    g_params[0] = g_params[1] = 0.0;
    g_show_orbit = false;
    g_max_iterations = 5000;
    m_specific.flags = FractalFlags::PERTURB;
    m_specific.pert_ref = [](const std::complex<double> &center, std::complex<double> &z)
    {
        ++s_orbit_calls;
        mandel_ref_pt(center, z);
    };
    g_dispatch = FractalDispatch{m_specific};
    bool requested{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(
            [this, &requested]
            {
                if (!requested && s_orbit_calls >= 2048)
                {
                    requested = true;
                    m_key = 'x';
                }
                return m_key;
            });
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_TRUE(requested);
    EXPECT_LT(s_orbit_calls, 5000);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4, m_plots);
}

class TestPerturbationController : public TestOverlaySound
{
protected:
    void SetUp() override;

    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<DComplex> m_saved_param{g_param_z1, DComplex{}};
    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
};

void TestPerturbationController::SetUp()
{
    TestOverlaySound::SetUp();
    g_user.perturbation = PerturbationMode::YES;
    g_user.std_calc_mode = CalcMode::ONE_PASS;
    g_params[0] = g_params[1] = 0.0;
    g_show_orbit = false;
    g_max_iterations = 5000;
    g_image_region = ImageRegion{{0.12, 0.13}, {0.13, 0.12}, {0.12, 0.12}};
    m_specific.flags = FractalFlags::PERTURB;
    m_specific.pert_ref = mandel_ref_pt;
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        ++s_orbit_calls;
        mandel_perturb(reference, delta, parameter);
    };
    g_dispatch = FractalDispatch{m_specific};
}

TEST_F(TestPerturbationController, menuInterruptsLongPixelAndResumeMatchesUninterruptedPlots)
{
    std::vector<int> colors;
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(
            [this, &colors](int, int, int color)
            {
                ++m_plots;
                colors.push_back(color);
            });
    bool requested{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(
            [this, &requested]
            {
                if (!requested && s_orbit_calls >= 2048)
                {
                    requested = true;
                    m_key = 'x';
                }
                return m_key;
            });
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_TRUE(requested);
    EXPECT_EQ(2048, s_orbit_calls);
    EXPECT_TRUE(colors.empty());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    g_resuming = true;
    EXPECT_EQ(0, render_standard_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    ASSERT_EQ(4, colors.size());
    const std::vector<int> resumed{colors};
    colors.clear();
    g_resuming = false;
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(resumed, colors);
}

TEST_F(TestPerturbationController, outputBackpressureBetweenPixelChunksDoesNotAdvanceOrRepeatWork)
{
    SoundSession sound{m_output, SoundSession::Ownership::BORROWED};
    StandardFractal calculation;
    StandardFractalController controller{calculation, sound};
    controller.start();
    while (s_orbit_calls == 0 && !controller.done())
    {
        controller.iterate();
    }
    ASSERT_EQ(2048, s_orbit_calls);
    ASSERT_EQ(0, m_plots);
    m_ready = false;
    int updates{};
    EXPECT_CALL(m_driver, delay(1)).Times(2);
    EXPECT_CALL(*m_output, update())
        .WillRepeatedly(
            [this, &updates]
            {
                if (!m_ready)
                {
                    EXPECT_EQ(2048, s_orbit_calls);
                    EXPECT_EQ(0, m_plots);
                    if (++updates == 3)
                    {
                        m_ready = true;
                    }
                }
            });

    controller.iterate();

    EXPECT_EQ(3, updates);
    EXPECT_EQ(4096, s_orbit_calls);
    EXPECT_EQ(0, m_plots);
    while (!controller.done())
    {
        controller.iterate();
    }
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(4 * 4999, s_orbit_calls);
    EXPECT_EQ(4, m_plots);
}

class TestPerturbationOverlayController : public TestPerturbationController, public WithParamInterface<CalcMode>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, true};
};

void TestPerturbationOverlayController::SetUp()
{
    TestPerturbationController::SetUp();
    g_user.std_calc_mode = GetParam();
    g_max_iterations = 4;
    g_show_orbit = true;
    g_image_region = {};
}

TEST_P(TestPerturbationOverlayController, outputBackpressureDoesNotRepeatOrAdvanceOrbitMath)
{
    bool refused{};
    int updates{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this, &refused](const SoundSample &sample)
            {
                if (!refused)
                {
                    refused = true;
                    m_ready = false;
                    return false;
                }
                m_samples.push_back(sample);
                return true;
            });
    EXPECT_CALL(*m_output, update())
        .WillRepeatedly(
            [this, &updates]
            {
                if (!m_ready)
                {
                    EXPECT_EQ(1, s_orbit_calls);
                    EXPECT_EQ(0, m_plots);
                    ++updates;
                    m_ready = true;
                }
            });

    EXPECT_EQ(0, render_standard_image());

    EXPECT_TRUE(refused);
    EXPECT_EQ(1, updates);
    EXPECT_FALSE(m_samples.empty());
    EXPECT_EQ(s_orbit_calls, m_samples.size());
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_P(TestPerturbationOverlayController, menuInterruptsAfterSampleAndResumeCompletesImage)
{
    bool requested{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this, &requested](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                if (!requested)
                {
                    requested = true;
                    m_key = 'x';
                }
                return true;
            });
    EXPECT_CALL(*m_output, mute()).Times(AtLeast(1));
    EXPECT_CALL(m_driver, unget_key('x'));

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_EQ(1, m_samples.size());
    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_P(TestPerturbationOverlayController, togglingOffStopsOutputWithoutInterruptingCalculation)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(
            [this](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                m_key = 'o';
                return true;
            });
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(1, m_samples.size());
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_P(TestPerturbationOverlayController, togglingOffAndOnContinuesTheSameOrbitWithoutRepeatingSamples)
{
    SoundSession sound{m_output, SoundSession::Ownership::BORROWED};
    StandardFractal calculation;
    StandardFractalController controller{calculation, sound};
    controller.start();
    controller.iterate();
    ASSERT_EQ(1, m_samples.size());
    ASSERT_EQ(1, s_orbit_calls);
    EXPECT_CALL(*m_output, mute());

    EXPECT_TRUE(dispatch_keyboard_key('o'));
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_TRUE(dispatch_keyboard_key('o'));
    EXPECT_TRUE(g_show_orbit);
    controller.iterate();

    EXPECT_EQ(2, m_samples.size());
    EXPECT_EQ(2, s_orbit_calls);
    EXPECT_EQ(1, std::count(m_events.begin(), m_events.end(), "boundary"));
    while (!controller.done())
    {
        controller.iterate();
    }
    EXPECT_EQ(0, controller.result());
    EXPECT_FALSE(g_orbit_save_flag);
}

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestPerturbationOverlayController,
    Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS, CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS, CalcMode::DIFFUSION,
        CalcMode::TESSERAL));

TEST_F(TestOverlaySound, controllerSuspendsWithinAnOrbitAndResumesUnfinishedPixel)
{
    g_show_orbit = false;
    g_max_iterations = 2050;
    m_key = 'x';
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(2047, s_orbit_calls);
    EXPECT_EQ(0, m_plots);

    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4, m_plots);
    EXPECT_EQ(5, s_pixel_calls);
}

class TestLogMapController : public TestOverlaySound, public WithParamInterface<bool>
{
};

TEST_P(TestLogMapController, menuKeyInterruptsSamplingWithinAnOrbitAndResumeFinishesTheImage)
{
    g_show_orbit = false;
    g_show_dot = GetParam() ? 15 : -1;
    g_max_iterations = 4098;
    g_inside_method = ColorMethod::ZMAG;
    ValueSaver saved_log_map{g_log_map_flag, 2L};
    ValueSaver saved_auto_log_map{g_log_map_auto_calculate, false};
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    bool requested{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(
            [this, &requested]
            {
                if (!requested && s_orbit_calls >= 2047)
                {
                    requested = true;
                    m_key = 'x';
                }
                return m_key;
            });
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_TRUE(requested);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(2047, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(2L, g_log_map_flag);
    EXPECT_EQ(4098L, g_max_iterations);
    EXPECT_FALSE(g_resume_data.empty());
    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4098L, g_log_map_flag);
    EXPECT_EQ(4098L, g_max_iterations);
    EXPECT_EQ(9, s_pixel_calls);
    EXPECT_GT(m_plots, 0);
}

INSTANTIATE_TEST_SUITE_P(ShowDot, TestLogMapController, Bool());

struct OptimizedOverlayPixelInput
{
    const char *name;
    FractalType type;
    CalcMode mode;
};

void PrintTo(const OptimizedOverlayPixelInput &input, std::ostream *out)
{
    *out << input.name;
}

const OptimizedOverlayPixelInput OPTIMIZED_OVERLAY_PIXEL_INPUTS[]{
    {"MandelOnePass", FractalType::MANDEL, CalcMode::ONE_PASS},
    {"MandelTwoPass", FractalType::MANDEL, CalcMode::TWO_PASS},
    {"MandelBoundary", FractalType::MANDEL, CalcMode::BOUNDARY_TRACE},
    {"MandelSolidGuess", FractalType::MANDEL, CalcMode::SOLID_GUESS},
    {"MandelDiffusion", FractalType::MANDEL, CalcMode::DIFFUSION},
    {"MandelTesseral", FractalType::MANDEL, CalcMode::TESSERAL},
    {"JuliaOnePass", FractalType::JULIA, CalcMode::ONE_PASS}, {"JuliaTwoPass", FractalType::JULIA, CalcMode::TWO_PASS},
    {"JuliaBoundary", FractalType::JULIA, CalcMode::BOUNDARY_TRACE},
    {"JuliaSolidGuess", FractalType::JULIA, CalcMode::SOLID_GUESS},
    {"JuliaDiffusion", FractalType::JULIA, CalcMode::DIFFUSION},
    {"JuliaTesseral", FractalType::JULIA, CalcMode::TESSERAL}};

class TestOptimizedOverlayPixel : public TestOverlaySound, public WithParamInterface<OptimizedOverlayPixelInput>
{
protected:
    void SetUp() override;

    ValueSaver<DComplex> m_saved_param{g_param_z1, DComplex{}};
};

void TestOptimizedOverlayPixel::SetUp()
{
    TestOverlaySound::SetUp();
    g_show_orbit = false;
    g_fractal_type = GetParam().type;
}

TEST_P(TestOptimizedOverlayPixel, yieldsCompletedWorkWithoutPolling)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    g_keyboard_check_interval = 0;
    calc_mandelbrot_init();
    StandardFractal calculation;
    calculation.m_standard_pass.select(GetParam().mode);

    EXPECT_GE(calculation.calculate_mandelbrot_pixel(), 0);

    EXPECT_TRUE(calculation.consume_traversal_yield());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOptimizedOverlayPixel, ValuesIn(OPTIMIZED_OVERLAY_PIXEL_INPUTS),
    [](const TestParamInfo<OptimizedOverlayPixelInput> &info) { return info.param.name; });

class TestOptimizedOverlayPixelBatch : public TestOptimizedOverlayPixel
{
protected:
    void SetUp() override;

    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
};

void TestOptimizedOverlayPixelBatch::SetUp()
{
    TestOptimizedOverlayPixel::SetUp();
    g_max_iterations = 4098;
}

TEST_P(TestOptimizedOverlayPixelBatch, retainsBatchingUntilBudgetExpires)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    g_keyboard_check_interval = 2 * g_max_iterations + 1;
    calc_mandelbrot_init();
    StandardFractal calculation;
    calculation.m_standard_pass.select(GetParam().mode);

    for (int pixel = 0; pixel < 2; ++pixel)
    {
        EXPECT_GE(calculation.calculate_mandelbrot_pixel(), 0);
        EXPECT_FALSE(calculation.consume_traversal_yield());
    }
    EXPECT_LT(g_keyboard_check_interval, 0);

    EXPECT_GE(calculation.calculate_mandelbrot_pixel(), 0);

    EXPECT_TRUE(calculation.consume_traversal_yield());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOptimizedOverlayPixelBatch, ValuesIn(OPTIMIZED_OVERLAY_PIXEL_INPUTS),
    [](const TestParamInfo<OptimizedOverlayPixelInput> &info) { return info.param.name; });

TEST_P(TestStandardPixelCheckpoints, controllerHandlesMenuKeyBeforeOptimizedTraversalChunkCompletes)
{
    g_show_orbit = false;
    g_max_iterations = 4098;
    g_max_keyboard_check_interval = 1000;
    g_std_calc_mode = GetParam();
    g_logical_screen = LogicalScreen{16, 16, 0, 0, 1.0, 1.0};
    g_screen_x_dots = 16;
    g_screen_y_dots = 16;
    free_pixel_grid();
    alloc_pixel_grid();
    fill_pixel_grid();
    ValueSaver saved_param{g_param_z1, DComplex{}};
    ValueSaver saved_bailout{g_magnitude_limit, 4.0};
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_BEEP};
    m_specific.calc_type = []
    {
        ++s_pixel_calls;
        return calc_mandelbrot_type();
    };
    m_specific.per_image = []
    {
        calc_mandelbrot_init();
        return true;
    };
    g_dispatch = FractalDispatch{m_specific};
    bool requested{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(
            [this, &requested]
            {
                if (!requested && s_pixel_calls > 0)
                {
                    requested = true;
                    m_key = 'x';
                }
                return m_key;
            });
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());

    EXPECT_EQ(-1, render_standard_image());

    EXPECT_TRUE(requested);
    EXPECT_EQ(2, s_pixel_calls);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());

    g_resuming = true;
    EXPECT_EQ(0, render_standard_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GT(s_pixel_calls, 2);
}

struct ScanlineOverlayCheckpointInput
{
    const char *name;
    int primary_pixels;
    CalcMode mode;
};

void PrintTo(const ScanlineOverlayCheckpointInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestScanlineOverlayCheckpoint : public TestOverlaySound, public WithParamInterface<ScanlineOverlayCheckpointInput>
{
protected:
    void SetUp() override;
};

void TestScanlineOverlayCheckpoint::SetUp()
{
    TestOverlaySound::SetUp();
    g_show_orbit = false;
    g_max_keyboard_check_interval = 1;
    g_std_calc_mode = GetParam().mode;
    g_resuming = false;
    s_pixel_positions.clear();
}

TEST_P(TestScanlineOverlayCheckpoint, suspensionRetainsTheNextPixel)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    StandardFractal calculation;
    calculation.resume();
    for (int increments = 0; increments < GetParam().primary_pixels; ++increments)
    {
        calculation.iterate();
    }
    ASSERT_EQ(GetParam().primary_pixels, s_pixel_positions.size());
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

INSTANTIATE_TEST_SUITE_P(Checkpoints, TestScanlineOverlayCheckpoint,
    Values(ScanlineOverlayCheckpointInput{"OnePassAfterFirstPixel", 1, CalcMode::ONE_PASS},
        ScanlineOverlayCheckpointInput{"TwoPassAfterFirstPixel", 1, CalcMode::TWO_PASS},
        ScanlineOverlayCheckpointInput{"OnePassAfterSecondPixel", 2, CalcMode::ONE_PASS},
        ScanlineOverlayCheckpointInput{"TwoPassAfterSecondPixel", 2, CalcMode::TWO_PASS}),
    [](const TestParamInfo<ScanlineOverlayCheckpointInput> &info) { return info.param.name; });

struct PerturbationOverlayYieldInput
{
    const char *name;
    CalcMode mode;
};

void PrintTo(const PerturbationOverlayYieldInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPerturbationOverlayYield : public TestOverlaySound, public WithParamInterface<PerturbationOverlayYieldInput>
{
protected:
    void SetUp() override;
};

void TestPerturbationOverlayYield::SetUp()
{
    TestOverlaySound::SetUp();
    m_specific.flags = FractalFlags::PERTURB;
}

TEST_P(TestPerturbationOverlayYield, retainsTraversalYieldGateWithoutPolling)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    StandardFractal calculation;
    calculation.m_standard_pass.select(GetParam().mode);
    calculation.m_perturbation_active = true;
    calculation.m_pert_engine.initialize_frame({0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0});

    EXPECT_GE(calculation.calculate_standard_pixel(true), 0);

    EXPECT_EQ(GetParam().mode == CalcMode::ONE_PASS, calculation.consume_traversal_yield());
    calculation.m_pert_engine.finish();
}

INSTANTIATE_TEST_SUITE_P(Passes, TestPerturbationOverlayYield,
    Values(PerturbationOverlayYieldInput{"OnePass", CalcMode::ONE_PASS},
        PerturbationOverlayYieldInput{"SolidGuess", CalcMode::SOLID_GUESS}),
    [](const TestParamInfo<PerturbationOverlayYieldInput> &info) { return info.param.name; });

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

class TestTrueColorStorage : public TestOverlaySound
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<bool> m_saved_true_color{g_true_color, true};
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, false};
    ValueSaver<std::string> m_saved_name{geometry::g_light_name, std::string{"truecolor.tga"}};
};

void TestTrueColorStorage::SetUp()
{
    TestOverlaySound::SetUp();
    end_disk();
    init_memory();
    m_directory = std::filesystem::temp_directory_path() /
        ("id-truecolor-storage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    set_save_library(m_directory);
}

void TestTrueColorStorage::TearDown()
{
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    TestOverlaySound::TearDown();
}

TEST_F(TestTrueColorStorage, imageSetupReturnsStorageRequirementsWithoutEnteringUi)
{
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    ImageCalculation image;

    EXPECT_TRUE(image.start().empty());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(22U, image.targa_storage().header.size());
    EXPECT_EQ(2, image.targa_storage().width);
    EXPECT_EQ(2, image.targa_storage().height);
    EXPECT_EQ(put_color_a, g_put_color);
    EXPECT_TRUE(std::filesystem::exists(image.targa_storage().path));
    image.finish();
}

class TestPreparedTrueColorStorage : public TestTrueColorStorage
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<ImageCalculation> m_image;
    std::filesystem::path m_prepared_path;
};

void TestPreparedTrueColorStorage::SetUp()
{
    TestTrueColorStorage::SetUp();
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    m_image = std::make_unique<ImageCalculation>();
    ASSERT_TRUE(m_image->start().empty());
    m_prepared_path = m_image->targa_storage().path;
}

void TestPreparedTrueColorStorage::TearDown()
{
    if (m_image)
    {
        EXPECT_NO_THROW(m_image->finish());
        m_image.reset();
    }
    TestTrueColorStorage::TearDown();
}

TEST_F(TestPreparedTrueColorStorage, refusedStorageFallsBackAndDiscardsThePreparedFile)
{
    const std::string warning{m_image->complete_targa_storage(false)};

    EXPECT_THAT(warning, HasSubstr(m_prepared_path.string()));
    EXPECT_FALSE(g_true_color);
    EXPECT_FALSE(std::filesystem::exists(m_prepared_path));
    EXPECT_TRUE(m_image->targa_storage().header.empty());
    EXPECT_EQ(put_color_a, g_put_color);
}

class TestRefusedTrueColorStorage : public TestPreparedTrueColorStorage
{
protected:
    void SetUp() override;
};

void TestRefusedTrueColorStorage::SetUp()
{
    TestPreparedTrueColorStorage::SetUp();
    ASSERT_NE(nullptr, m_image);
    const std::string warning{m_image->complete_targa_storage(false)};
    ASSERT_THAT(warning, HasSubstr(m_prepared_path.string()));
}

TEST_F(TestRefusedTrueColorStorage, repeatedStorageCompletionIsRejected)
{
    EXPECT_THROW(m_image->complete_targa_storage(false), std::logic_error);
}

TEST_F(TestTrueColorStorage, finishingAbandonedSetupDiscardsThePreparedFile)
{
    ImageCalculation image;
    ASSERT_TRUE(image.start().empty());
    const std::filesystem::path path{image.targa_storage().path};

    image.finish();

    EXPECT_FALSE(std::filesystem::exists(path));
    EXPECT_FALSE(g_disk_flag);
    EXPECT_FALSE(g_true_color);
}

TEST_F(TestTrueColorStorage, controllerPreparesStorageAndFinishesTheTargaImage)
{
    EXPECT_EQ(0, render_image());

    EXPECT_TRUE(g_true_color);
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_EQ(22U + 2U * 2U * 3U, std::filesystem::file_size(m_directory / "image" / "truecolor.tga"));
}

class TestTargaFlushFailure : public TestTrueColorStorage
{
protected:
    void SetUp() override;
    void damage_backing() const;
};

void TestTargaFlushFailure::SetUp()
{
    TestTrueColorStorage::SetUp();
    g_logical_screen = LogicalScreen{256, 256, 0, 0, 1.0, 1.0};
    g_screen_x_dots = 256;
    g_screen_y_dots = 256;
    free_pixel_grid();
    alloc_pixel_grid();
    fill_pixel_grid();
}

void TestTargaFlushFailure::damage_backing() const
{
    // Leave a distant dirty cache block beyond stdio's buffered reads.
    write_targa_pixel(255, 255, 10, 20, 30);
    Byte red{};
    Byte green{};
    Byte blue{};
    read_targa_pixel(0, 0, &red, &green, &blue);
    std::filesystem::resize_file(targa_disk_path(), 0);
}

TEST_F(TestTargaFlushFailure, damagedBackingFailsDuringCacheFlush)
{
    ImageCalculation calculation;
    complete_image_setup(calculation, calculation.start());
    damage_backing();

    EXPECT_THROW(calculation.finish(), MemoryError);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_NO_THROW(calculation.finish());
    EXPECT_NO_THROW(end_disk());
}

template <typename Calculation>
class TestAbandonedCalculation : public TestTargaFlushFailure
{
protected:
    void prepare(ImageCalculation &calculation);
    void prepare(StandardFractal &calculation);
};

template <typename Calculation>
void TestAbandonedCalculation<Calculation>::prepare(ImageCalculation &calculation)
{
    complete_image_setup(calculation, calculation.start());
}

template <typename Calculation>
void TestAbandonedCalculation<Calculation>::prepare(StandardFractal &calculation)
{
    complete_image_setup(calculation, calculation.resume());
}

struct AbandonedCalculationNames
{
    template <typename Calculation>
    static std::string GetName(int);
};

template <typename Calculation>
std::string AbandonedCalculationNames::GetName(int)
{
    if constexpr (std::is_same_v<Calculation, ImageCalculation>)
    {
        return "Image";
    }
    else if constexpr (std::is_same_v<Calculation, StandardFractal>)
    {
        return "Standard";
    }
    else
    {
        return "NonStandard";
    }
}

using AbandonedCalculations = Types<ImageCalculation, StandardFractal, NonStandardFractal>;
TYPED_TEST_SUITE(TestAbandonedCalculation, AbandonedCalculations, AbandonedCalculationNames);

TYPED_TEST(TestAbandonedCalculation, cleanupPreservesTheActiveException)
{
    EXPECT_THAT(
        [this]
        {
            TypeParam calculation;
            this->prepare(calculation);
            this->damage_backing();
            throw std::runtime_error("Original rendering failure");
        },
        ThrowsMessage<std::runtime_error>(StrEq("Original rendering failure")));

    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_NO_THROW(end_disk());
}

class TestRenderCleanupBase : public TestTargaFlushFailure
{
protected:
    void damage_backing(ImageRenderContext &rendering);
    void expect_diagnostic(const char *message);
    void expect_released(ImageRenderContext &rendering, SoundSession &sound);

    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
};

void TestRenderCleanupBase::damage_backing(ImageRenderContext &rendering)
{
    rendering.start();
    ASSERT_FALSE(rendering.done());
    ASSERT_TRUE(g_disk_flag);
    TestTargaFlushFailure::damage_backing();
}

void TestRenderCleanupBase::expect_diagnostic(const char *message)
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
}

void TestRenderCleanupBase::expect_released(ImageRenderContext &rendering, SoundSession &sound)
{
    EXPECT_TRUE(rendering.done());
    EXPECT_EQ(-1, rendering.result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(g_log_map_table.empty());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_TRUE(exit_check().empty());
    EXPECT_FALSE(dispatch_keyboard_key('x'));
    EXPECT_NO_THROW(end_disk());
    EXPECT_NO_THROW(sound.cancel());
    EXPECT_FALSE(sound.finish());
    EXPECT_NO_THROW(rendering.iterate());
}

class TestRenderCleanup : public TestRenderCleanupBase, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
};

void TestRenderCleanup::SetUp()
{
    TestRenderCleanupBase::SetUp();
    if (GetParam())
    {
        select_froth();
    }
}

class TestRenderFailurePropagation : public TestRenderCleanup
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<SoundSession> m_sound;
    std::unique_ptr<ImageRenderContext> m_rendering;
};

void TestRenderFailurePropagation::SetUp()
{
    TestRenderCleanup::SetUp();
    m_sound = std::make_unique<SoundSession>(m_output);
    m_rendering = std::make_unique<ImageRenderContext>(*m_sound);
    damage_backing(*m_rendering);
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
}

void TestRenderFailurePropagation::TearDown()
{
    m_rendering.reset();
    m_sound.reset();
    TestRenderCleanup::TearDown();
}

TEST_P(TestRenderCleanup, originalDiagnosticSurvivesStorageAndSoundCleanupFailures)
{
    SoundSession sound{m_output};
    ImageRenderContext rendering{sound};
    damage_backing(rendering);
    expect_diagnostic("Original rendering failure");
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("Original rendering failure")));
    EXPECT_CALL(*m_output, mute()).WillOnce(Throw(std::runtime_error("Mute failure")));
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Sound close failure")));

    EXPECT_NO_THROW(while (!rendering.done()) { rendering.iterate(); });

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    expect_released(rendering, sound);
}

TEST_P(TestRenderCleanup, allocationDiagnosticSurvivesStorageCleanupFailure)
{
    SoundSession sound{m_output};
    ImageRenderContext rendering{sound};
    damage_backing(rendering);
    expect_diagnostic("Insufficient free memory for calculation");
    EXPECT_CALL(*m_output, update()).WillOnce(Throw(std::bad_alloc{}));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));

    EXPECT_NO_THROW(rendering.iterate());

    expect_released(rendering, sound);
}

TEST_P(TestRenderFailurePropagation, precisionFailureStillPropagatesToTheOuterUi)
{
    EXPECT_CALL(*m_output, update()).WillOnce(Throw(PrecisionError{"Original precision failure"}));

    EXPECT_THAT([this] { m_rendering->iterate(); }, ThrowsMessage<PrecisionError>(StrEq("Original precision failure")));

    expect_released(*m_rendering, *m_sound);
}

TEST_P(TestRenderFailurePropagation, unexpectedFailureStillPropagatesWithoutChangingItsType)
{
    EXPECT_CALL(*m_output, update()).WillOnce(Throw(std::logic_error("Original logic failure")));

    EXPECT_THAT([this] { m_rendering->iterate(); }, ThrowsMessage<std::logic_error>(StrEq("Original logic failure")));

    expect_released(*m_rendering, *m_sound);
}

INSTANTIATE_TEST_SUITE_P(StandardAndFroth, TestRenderFailurePropagation, Bool());

struct RenderFailurePolicyInput
{
    const char *name;
    bool frothy;
    BatchMode batch;
};

void PrintTo(const RenderFailurePolicyInput &input, std::ostream *out)
{
    *out << input.name;
}

const RenderFailurePolicyInput RENDER_FAILURE_POLICIES[]{{"StandardInteractive", false, BatchMode::NONE},
    {"FrothyInteractive", true, BatchMode::NONE}, {"StandardBatch", false, BatchMode::NORMAL},
    {"FrothyBatch", true, BatchMode::NORMAL}};

class TestRenderFailurePolicy : public TestRenderCleanupBase, public WithParamInterface<RenderFailurePolicyInput>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void expect_failure_diagnostic(const char *message);
    void expect_failure_policy(const char *message);

    std::unique_ptr<SoundSession> m_owner;
    std::unique_ptr<SoundSession> m_borrowed;
    std::unique_ptr<ImageRenderContext> m_rendering;
};

void TestRenderFailurePolicy::SetUp()
{
    TestRenderCleanupBase::SetUp();
    if (GetParam().frothy)
    {
        select_froth();
    }
    m_owner = std::make_unique<SoundSession>(m_output);
    m_borrowed = std::make_unique<SoundSession>(g_sound, SoundSession::Ownership::BORROWED);
}

void TestRenderFailurePolicy::TearDown()
{
    m_rendering.reset();
    m_borrowed.reset();
    m_owner.reset();
    TestRenderCleanupBase::TearDown();
}

void TestRenderFailurePolicy::expect_failure_diagnostic(const char *message)
{
    if (GetParam().batch == BatchMode::NORMAL)
    {
        EXPECT_CALL(m_driver, stack_screen()).Times(0);
    }
    else
    {
        expect_diagnostic(message);
    }
}

void TestRenderFailurePolicy::expect_failure_policy(const char *message)
{
    EXPECT_EQ(GetParam().batch == BatchMode::NORMAL ? BatchMode::BAILOUT_ERROR_NO_SAVE : BatchMode::NONE, g_init_batch);
    if (GetParam().batch == BatchMode::NORMAL)
    {
        std::ifstream diagnostic{get_append_save_path(WriteFile::DEBUG, "stopmsg")};
        EXPECT_EQ(std::string{message} + "\n",
            (std::string{std::istreambuf_iterator<char>{diagnostic}, std::istreambuf_iterator<char>{}}));
    }
}

class TestBorrowedRenderFailure : public TestRenderFailurePolicy
{
protected:
    void SetUp() override;
};

void TestBorrowedRenderFailure::SetUp()
{
    TestRenderFailurePolicy::SetUp();
    g_start_show_orbit = true;
    m_rendering = std::make_unique<ImageRenderContext>(*m_borrowed);
    m_rendering->start();
    ASSERT_FALSE(m_rendering->done());
    g_init_batch = GetParam().batch;
    expect_failure_diagnostic("Original sound write failure");
    EXPECT_CALL(*m_output, begin_orbit()).WillOnce(Throw(std::runtime_error("Original sound write failure")));
    EXPECT_CALL(*m_output, mute()).Times(2);
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Sound close failure")));
}

TEST_P(TestRenderCleanup, batchFailureRetainsItsDiagnosticAndNoSaveExitPolicy)
{
    SoundSession sound{m_output};
    ImageRenderContext rendering{sound};
    damage_backing(rendering);
    g_init_batch = BatchMode::NORMAL;
    EXPECT_CALL(*m_output, update()).WillOnce(Throw(std::runtime_error("Original batch failure")));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    EXPECT_CALL(m_driver, stack_screen()).Times(0);

    EXPECT_NO_THROW(rendering.iterate());

    EXPECT_EQ(BatchMode::BAILOUT_ERROR_NO_SAVE, g_init_batch);
    std::ifstream diagnostic{get_append_save_path(WriteFile::DEBUG, "stopmsg")};
    EXPECT_EQ("Original batch failure\n",
        (std::string{std::istreambuf_iterator<char>{diagnostic}, std::istreambuf_iterator<char>{}}));
    expect_released(rendering, sound);
}

TEST_P(TestBorrowedRenderFailure, writeFailureSurvivesOwningSoundCleanup)
{
    EXPECT_NO_THROW(while (!m_rendering->done()) { m_rendering->iterate(); });
    EXPECT_TRUE(m_rendering->failure_reported());
    EXPECT_EQ(-1, finish_render_sound(*m_owner, m_rendering->result(), m_rendering->failure_reported()));

    expect_failure_policy("Original sound write failure");
    expect_released(*m_rendering, *m_borrowed);
    EXPECT_NO_THROW(m_owner->cancel());
    if (GetParam().batch == BatchMode::NONE)
    {
        EXPECT_FALSE(m_owner->finish());
    }
}

INSTANTIATE_TEST_SUITE_P(Policies, TestBorrowedRenderFailure, ValuesIn(RENDER_FAILURE_POLICIES),
    [](const TestParamInfo<RenderFailurePolicyInput> &info) { return info.param.name; });

class TestOwnerCloseFailure : public TestRenderFailurePolicy
{
protected:
    void SetUp() override;
};

void TestOwnerCloseFailure::SetUp()
{
    TestRenderFailurePolicy::SetUp();
    ASSERT_TRUE(m_borrowed->finish());
    g_init_batch = GetParam().batch;
    expect_failure_diagnostic("Sound close failure");
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Sound close failure")));
}

TEST_P(TestOwnerCloseFailure, closeFailureUsesTheRenderErrorPolicy)
{
    EXPECT_EQ(-1, finish_render_sound(*m_owner, 0, false));

    expect_failure_policy("Sound close failure");
    EXPECT_NO_THROW(m_owner->cancel());
    if (GetParam().batch == BatchMode::NONE)
    {
        EXPECT_FALSE(m_owner->finish());
    }
}

INSTANTIATE_TEST_SUITE_P(Policies, TestOwnerCloseFailure, ValuesIn(RENDER_FAILURE_POLICIES),
    [](const TestParamInfo<RenderFailurePolicyInput> &info) { return info.param.name; });

TEST_P(TestRenderCleanup, ownerCloseFailureAfterInterruptionIsNotDiscarded)
{
    SoundSession owner{m_output};
    SoundSession borrowed{g_sound, SoundSession::Ownership::BORROWED};
    EXPECT_CALL(*m_output, mute()).Times(2);
    borrowed.cancel();
    expect_diagnostic("Sound close failure");
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Sound close failure")));

    EXPECT_EQ(-1, finish_render_sound(owner, -1, false));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_NO_THROW(owner.cancel());
}

INSTANTIATE_TEST_SUITE_P(StandardAndFroth, TestRenderCleanup, Bool());

class TestStandardPotentialStorage : public TestOverlaySound
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<Potential> m_saved_potential{g_potential, Potential{{20.0, 5.0, 4.0}, true, true}};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<bool> m_saved_overlay{g_show_orbit, false};
    ValueSaver<bool> m_saved_good_mode{g_good_mode, true};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
};

void TestStandardPotentialStorage::SetUp()
{
    TestOverlaySound::SetUp();
    end_disk();
    init_memory();
    m_directory = std::filesystem::temp_directory_path() /
        ("id-standard-potential-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    g_temp_dir = m_directory / "missing";
    set_save_library(m_directory);
    EXPECT_CALL(m_driver, display_string(_, _, _, _, StrEq("clearing 16bit pot work area")));
    EXPECT_CALL(m_driver, flush());
}

void TestStandardPotentialStorage::TearDown()
{
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    TestOverlaySound::TearDown();
}

TEST_F(TestStandardPotentialStorage, controllerPreparesStorageBeforePixelCalculation)
{
    g_std_calc_mode = CalcMode::SOLID_GUESS;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_TRUE(g_disk_flag);
    EXPECT_TRUE(g_disk_16_bit);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4, s_pixel_calls);
}

TEST_F(TestStandardPotentialStorage, controllerContinuesWithRequestedPassAfterStorageCancellation)
{
    g_std_calc_mode = CalcMode::TWO_PASS;
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    EXPECT_CALL(m_driver, stack_screen());
    EXPECT_CALL(m_driver, unstack_screen());
    EXPECT_CALL(m_driver, move_cursor(_, _));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _));
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, get_key())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(0, s_pixel_calls);
                EXPECT_FALSE(g_disk_flag);
                EXPECT_TRUE(exit_check().empty());
                return ID_KEY_ESC;
            }));

    EXPECT_EQ(0, render_standard_image());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_FALSE(g_disk_16_bit);
    EXPECT_FALSE(g_potential.store_16bit);
    EXPECT_EQ(CalcMode::TWO_PASS, g_std_calc_mode);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(4, s_pixel_calls);
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

    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    g_inside_method = ColorMethod::ITER;
    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_F(TestStandardRenderDiagnostic, allocationFailureDiscardsWorkAndReportsToUi)
{
    g_dispatch.set_per_image([]() -> bool { throw std::bad_alloc{}; });
    const FractalDispatch dispatch{g_dispatch};
    expect_diagnostic("Insufficient free memory for calculation");

    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(0, m_plots);
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    EXPECT_TRUE(g_log_map_table.empty());
}

TEST_F(TestStandardRenderDiagnostic, precisionFailureCleansUpBeforeReachingOuterUi)
{
    ValueSaver saved_digits{g_bf_digits, 0};
    g_dispatch.set_per_image(
        []
        {
            init_bf_dec(2000);
            return true;
        });
    EXPECT_CALL(*m_output, mute());

    EXPECT_THROW(render_image(), PrecisionError);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, m_plots);
    g_dispatch.set_per_image(initialize_overlay_image);
    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_F(TestStandardRenderDiagnostic, backingMemoryFailureReportsToUiAfterCleanup)
{
    g_dispatch.set_per_image([]() -> bool { throw MemoryError{"Read file error"}; });
    expect_diagnostic("Read file error");

    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, m_plots);
    g_dispatch.set_per_image(initialize_overlay_image);
    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
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
    calculation.m_distance_estimator = DistanceEstimator{4.0, 100.0, true, false};
    ASSERT_TRUE(calculation.m_distance_estimator.advance({1.0, 1.0}));
    g_old_z = {1.0, 1.0};
    g_new_z = g_old_z;
    EXPECT_CALL(*m_output, play(_)).WillRepeatedly(Return(false));

    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(false));
    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(false));

    EXPECT_EQ((DComplex{3.0, 2.0}), calculation.distance_estimator().derivative());
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

    EXPECT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(false));
    EXPECT_TRUE(g_orbit_save_flag);
    EXPECT_EQ(1, m_plots);

    m_ready = true;
    calculation.iterate();
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_GE(calculation.calculate_standard_pixel(false), 0);
    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ(1, s_pixel_calls);
}

class TestOverlayCleanup : public TestOverlaySound
{
protected:
    void SetUp() override;

    std::vector<int> m_pixels{2, 3, 4, 5};
};

void TestOverlayCleanup::SetUp()
{
    TestOverlaySound::SetUp();
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels.at(y * 2 + x); }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke([this](int x, int y, int color) { m_pixels.at(y * 2 + x) = color; }));
}

TEST_F(TestOverlayCleanup, standardPixelRestoresOverlayAfterTogglingOffAndOn)
{
    g_max_iterations = 3;
    StandardFractal calculation;
    calculation.m_standard_pass.select(CalcMode::ONE_PASS);
    ASSERT_EQ(CALC_YIELDED, calculation.calculate_standard_pixel(true));
    calculation.iterate();
    ASSERT_TRUE(calculation.consume_orbit_overlay_displayed());
    ASSERT_NE(5, m_pixels[3]);

    calculation.scrub_overlay();
    g_show_orbit = false;
    ASSERT_THAT(m_pixels, ElementsAre(2, 3, 4, 5));
    g_show_orbit = true;

    int color{CALC_YIELDED};
    for (int step = 0; step < 8 && color == CALC_YIELDED; ++step)
    {
        color = calculation.calculate_standard_pixel(true);
        if (color == CALC_YIELDED)
        {
            calculation.iterate();
            calculation.consume_orbit_overlay_displayed();
        }
    }

    ASSERT_GE(color, 0);
    EXPECT_EQ(2, s_orbit_calls);
    EXPECT_THAT(m_pixels, ElementsAre(color, 3, 4, 5));
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_F(TestOverlayCleanup, frothyPixelRestoresOverlayAfterTogglingOffAndOn)
{
    g_i_stop_pt = {0, 0};
    FrothyBasin calculation;
    ASSERT_TRUE(calculation.per_image());
    calculation.resume();
    // Place the zero-valued orbit over a different pixel from the image plot.
    g_image_region.m_min.x = -1.0;
    g_image_region.m_max.y = -1.0;
    calculation.iterate();
    ASSERT_TRUE(calculation.consume_orbit_overlay_displayed());
    ASSERT_NE(5, m_pixels[3]);

    calculation.scrub_overlay();
    g_show_orbit = false;
    ASSERT_THAT(m_pixels, ElementsAre(2, 3, 4, 5));
    g_show_orbit = true;

    for (int step = 0; step < 8 && !calculation.done(); ++step)
    {
        calculation.iterate();
        calculation.consume_orbit_overlay_displayed();
    }

    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(2U, m_samples.size());
    EXPECT_THAT(m_pixels, ElementsAre(g_color, 3, 4, 5));
    EXPECT_FALSE(g_orbit_save_flag);
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

TEST_F(TestOverlaySound, selectedFrothyBasinPreservesOrbitAndPixelWorkIncrements)
{
    struct WorkState
    {
        int row;
        int col;
        long iteration;
        int plots;
        std::size_t samples;
        bool displayed;
        bool done;
    };

    ValueSaver saved_start_orbit{g_start_show_orbit, true};
    std::vector<WorkState> expected;
    {
        FrothyBasin control;
        ASSERT_TRUE(control.per_image());
        control.resume();
        for (int step = 0; step < 100 && !control.done(); ++step)
        {
            control.iterate();
            const bool displayed{control.consume_orbit_overlay_displayed()};
            expected.push_back({g_row, g_col, g_color_iter, m_plots, m_samples.size(), displayed, control.done()});
        }
        ASSERT_TRUE(control.done());
    }
    const std::vector<std::string> expected_events{m_events};
    const std::vector<SoundSample> expected_samples{m_samples};
    m_events.clear();
    m_samples.clear();
    m_plots = 0;
    select_froth();
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_events.empty());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);

    for (const WorkState &work : expected)
    {
        calculation.iterate();
        EXPECT_EQ(work.displayed, calculation.consume_orbit_overlay_displayed());
        EXPECT_EQ(work.row, g_row);
        EXPECT_EQ(work.col, g_col);
        EXPECT_EQ(work.iteration, g_color_iter);
        EXPECT_EQ(work.plots, m_plots);
        EXPECT_EQ(work.samples, m_samples.size());
        EXPECT_EQ(work.done, calculation.done());
    }

    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(expected_events, m_events);
    ASSERT_EQ(expected_samples.size(), m_samples.size());
    for (std::size_t i = 0; i < m_samples.size(); ++i)
    {
        EXPECT_EQ(expected_samples[i].x, m_samples[i].x);
        EXPECT_EQ(expected_samples[i].y, m_samples[i].y);
    }
    calculation.iterate();
    EXPECT_EQ(expected_events, m_events);
    calculation.finish(false);
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_F(TestOverlaySound, selectedFrothyBasinResumesAtTheNextUnfinishedPixel)
{
    ValueSaver saved_start_orbit{g_start_show_orbit, false};
    g_show_orbit = false;
    std::vector<Point2i> pixels;
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke([&pixels](int x, int y, int) { pixels.push_back({x, y}); }));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    select_froth();
    {
        NonStandardFractal calculation;
        ASSERT_TRUE(calculation.start().empty());
        ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
        calculation.iterate();
        ASSERT_THAT(pixels, ElementsAre(Point2i{0, 0}));

        calculation.suspend();

        EXPECT_TRUE(calculation.done());
        EXPECT_THROW(calculation.select(), std::logic_error);
        ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
        ASSERT_FALSE(g_resume_data.empty());
        int row{-1};
        int col{-1};
        EXPECT_EQ(1, start_resume());
        get_resume(row, col);
        EXPECT_EQ(0, row);
        EXPECT_EQ(1, col);
    }
    g_resuming = true;
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    for (int step = 0; step < 4 && !calculation.done(); ++step)
    {
        calculation.iterate();
    }
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_THAT(pixels, ElementsAre(Point2i{0, 0}, Point2i{1, 0}, Point2i{0, 1}, Point2i{1, 1}));
    calculation.finish(false);
    EXPECT_TRUE(g_resume_data.empty());
}

class TestOverlaySoundController : public TestOverlaySound, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    int render(SoundSession &sound);
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, true};
};

void TestOverlaySoundController::SetUp()
{
    TestOverlaySound::SetUp();
    if (GetParam())
    {
        select_froth();
    }
}

int TestOverlaySoundController::render(SoundSession &sound)
{
    if (!GetParam())
    {
        return render_standard_image(sound);
    }
    NonStandardFractal calculation;
    complete_image_setup(calculation, calculation.start());
    EXPECT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    FrothyBasinController controller{calculation, sound};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    const int result{controller.result()};
    calculation.finish(result < 0);
    return result;
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

        EXPECT_EQ(0, render_image());

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
            EXPECT_EQ(0, render_image());
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

TEST_P(TestOverlaySoundController, zeroDelayMenuKeyInterruptsBeforeThePixelCompletes)
{
    g_max_iterations = 4096;
    SoundSession sound{m_output};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                if (m_samples.size() == 1)
                {
                    m_key = 'x';
                }
                return true;
            }));
    EXPECT_CALL(*m_output, mute()).Times(AtLeast(1));
    EXPECT_CALL(m_driver, unget_key('x'))
        .WillOnce(Invoke(
            [this](int key)
            {
                EXPECT_LT(g_color_iter, g_max_iterations);
                m_key = key;
            }));

    EXPECT_EQ(-1, render(sound));

    EXPECT_EQ(1U, m_samples.size());
    EXPECT_TRUE(m_ready);
    EXPECT_EQ('x', m_key);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_P(TestOverlaySoundController, zeroDelayOrbitToggleDoesNotWaitForPixelCompletion)
{
    g_max_iterations = 4096;
    SoundSession sound{m_output};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                if (m_samples.size() == 1)
                {
                    m_key = 'o';
                }
                return true;
            }));
    EXPECT_CALL(*m_output, mute())
        .WillOnce(Invoke(
            [this]
            {
                EXPECT_EQ(1U, m_samples.size());
                EXPECT_LT(g_color_iter, g_max_iterations);
            }));

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(1U, m_samples.size());
    EXPECT_TRUE(m_ready);
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

class TestFrothyBasinController : public TestOverlaySound
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, true};
};

void TestFrothyBasinController::SetUp()
{
    TestOverlaySound::SetUp();
    select_froth();
}

class TestPreparedFrothyController : public TestFrothyBasinController
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<NonStandardFractal> m_calculation;
    std::unique_ptr<SoundSession> m_sound;
    std::unique_ptr<FrothyBasinController> m_controller;
};

void TestPreparedFrothyController::SetUp()
{
    TestFrothyBasinController::SetUp();
    m_calculation = std::make_unique<NonStandardFractal>();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, m_calculation->select());
    m_sound = std::make_unique<SoundSession>(m_output, SoundSession::Ownership::BORROWED);
    m_controller = std::make_unique<FrothyBasinController>(*m_calculation, *m_sound);
}

void TestPreparedFrothyController::TearDown()
{
    m_controller.reset();
    m_sound.reset();
    m_calculation.reset();
    TestFrothyBasinController::TearDown();
}

TEST_F(TestPreparedFrothyController, iterateRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_F(TestPreparedFrothyController, resultRequiresCompletion)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_F(TestPreparedFrothyController, suspendRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedFrothyController : public TestPreparedFrothyController
{
protected:
    void SetUp() override;
};

void TestStartedFrothyController::SetUp()
{
    TestPreparedFrothyController::SetUp();
    m_controller->start();
}

TEST_F(TestStartedFrothyController, duplicateStartIsRejected)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedFrothyController : public TestStartedFrothyController
{
protected:
    void SetUp() override;
};

void TestCompletedFrothyController::SetUp()
{
    TestStartedFrothyController::SetUp();
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).Times(0);
    for (int step = 0; step < 100 && !m_controller->done(); ++step)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_F(TestCompletedFrothyController, iterateDoesNotPlotAfterCompletion)
{
    const int plots{m_plots};

    m_controller->iterate();

    EXPECT_EQ(plots, m_plots);
}

TEST_F(TestCompletedFrothyController, suspendAfterCompletionIsRejected)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

TEST_F(TestFrothyBasinController, stepwiseControllerPreservesCalculationIncrements)
{
    struct WorkState
    {
        int row;
        int col;
        long iteration;
        int plots;
        std::size_t samples;
        bool done;
    };

    std::vector<WorkState> expected;
    {
        NonStandardFractal control;
        ASSERT_TRUE(control.start().empty());
        ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, control.select());
        for (int i = 0; i < 100 && !control.done(); ++i)
        {
            control.iterate();
            control.consume_orbit_overlay_displayed();
            expected.push_back({g_row, g_col, g_color_iter, m_plots, m_samples.size(), control.done()});
        }
        ASSERT_TRUE(control.done());
        control.finish(false);
    }
    const std::vector<std::string> events{m_events};
    const std::vector<SoundSample> samples{m_samples};
    m_events.clear();
    m_samples.clear();
    m_plots = 0;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    SoundSession sound{m_output};
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    FrothyBasinController controller{calculation, sound};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_events.empty());
    for (const WorkState &work : expected)
    {
        controller.iterate();
        EXPECT_EQ(work.row, g_row);
        EXPECT_EQ(work.col, g_col);
        EXPECT_EQ(work.iteration, g_color_iter);
        EXPECT_EQ(work.plots, m_plots);
        EXPECT_EQ(work.samples, m_samples.size());
        EXPECT_EQ(work.done, controller.done());
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(events, m_events);
    ASSERT_EQ(samples.size(), m_samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        EXPECT_EQ(samples[i].x, m_samples[i].x);
        EXPECT_EQ(samples[i].y, m_samples[i].y);
    }
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    calculation.finish(false);
}

TEST_F(TestFrothyBasinController, stepwiseSuspensionScrubsOverlayAndRetainsUnfinishedPixel)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    SoundSession sound{m_output, SoundSession::Ownership::BORROWED};
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).Times(0);
    FrothyBasinController controller{calculation, sound};
    controller.start();
    controller.iterate();
    ASSERT_TRUE(g_orbit_save_flag);

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    ASSERT_EQ(1, start_resume());
    int row{-1};
    int col{-1};
    get_resume(row, col);
    EXPECT_EQ(0, row);
    EXPECT_EQ(0, col);
}

TEST_F(TestFrothyBasinController, stepwisePixelSuspensionRetainsNextUnfinishedPixel)
{
    g_start_show_orbit = false;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::FROTHY_BASIN, calculation.select());
    SoundSession sound{m_output, SoundSession::Ownership::BORROWED};
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).Times(0);
    FrothyBasinController controller{calculation, sound};
    controller.start();
    controller.iterate();
    ASSERT_EQ(1, m_plots);

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_EQ(1, start_resume());
    int row{-1};
    int col{-1};
    get_resume(row, col);
    EXPECT_EQ(0, row);
    EXPECT_EQ(1, col);
}

TEST_F(TestPreparedFrothyController, stepwiseOrbitToggleDoesNotInterruptImage)
{
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).Times(0);
    m_controller->start();
    m_controller->iterate();
    ASSERT_TRUE(g_orbit_save_flag);

    EXPECT_TRUE(dispatch_keyboard_key('o'));

    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_FALSE(m_controller->done());
    for (int i = 0; i < 100 && !m_controller->done(); ++i)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(0, m_controller->result());
    EXPECT_TRUE(m_calculation->completed());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    m_calculation->finish(false);
}

TEST_F(TestPreparedFrothyController, stepwiseSoundWaitInterruptionPreservesCommandAndResume)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_ready = false;
                return true;
            }));
    EXPECT_CALL(*m_output, mute()).WillRepeatedly(Invoke([this] { m_ready = true; }));
    EXPECT_CALL(*m_output, finish()).Times(0);
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    m_controller->start();
    m_controller->iterate();
    ASSERT_FALSE(m_controller->done());

    m_key = 'x';
    m_controller->iterate();

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_TRUE(m_calculation->done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_TRUE(g_show_orbit);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestPreparedFrothyController, stepwiseSoundFailureReleasesHandlersAndCalculation)
{
    EXPECT_CALL(*m_output, update()).WillOnce(Throw(std::runtime_error("sound failed")));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).Times(0);
    m_controller->start();

    EXPECT_THROW(m_controller->iterate(), std::runtime_error);

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_TRUE(m_calculation->done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, m_plots);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestPreparedFrothyController, stepwisePlotFailureScrubsOverlayAndReleasesHandlers)
{
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillOnce(Throw(std::runtime_error("plot failed")))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).Times(0);
    m_controller->start();

    EXPECT_THROW(m_controller->iterate(), std::runtime_error);

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_TRUE(m_calculation->done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestPreparedFrothyController, stepwiseDestructionOnlyReleasesHandlers)
{
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).Times(0);
    m_controller->start();
    m_controller->iterate();
    ASSERT_FALSE(m_controller->done());

    m_controller.reset();

    EXPECT_FALSE(dispatch_keyboard_key('o'));
    EXPECT_FALSE(m_calculation->done());
    m_calculation->scrub_overlay();
    m_calculation->finish(true);
}

INSTANTIATE_TEST_SUITE_P(StandardAndFrothyBasin, TestOverlaySoundController, Values(false, true));

} // namespace id::test
