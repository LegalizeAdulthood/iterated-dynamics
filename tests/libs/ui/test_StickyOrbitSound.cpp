// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <MockDriver.h>
#include <MockSoundOutput.h>
#include <PendingDriverKey.h>
#include <TemporaryDirectory.h>
#include <ui/TestRender.h>

#include <engine/calcfrac.h>
#include <engine/bailout_formula.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/PertEngine.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/StandardPass.h>
#include <engine/StickyOrbits.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/burning_ship.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <math/biginit.h>
#include <ui/KeyboardHandler.h>
#include <ui/SoundSession.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
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

long s_math_calls{};
long s_iteration{};
std::vector<Point2i> s_seeds;
long s_perturbation_calls{};
std::vector<std::complex<double>> s_parameter_displacements;

int initialize_mandel_sticky_pixel()
{
    s_seeds.push_back({g_col, g_row});
    const double step{g_bf_math == BFMathType::NONE ? 0.01 : 0.0};
    g_init = {0.25 + g_col * step, 0.25 + g_row * step};
    g_float_param = &g_init;
    g_old_z = g_init;
    g_temp_sqr_x = g_old_z.x * g_old_z.x;
    g_temp_sqr_y = g_old_z.y * g_old_z.y;
    if (g_bf_math == BFMathType::BIG_NUM)
    {
        BigStackSaver saved;
        const BigNum offset{alloc_stack(g_bn_length)};
        float_to_bn(offset, 1e-40);
        mult_bn_int(offset, offset, static_cast<U16>(g_col));
        float_to_bn(g_param_z_bn.x, g_init.x);
        add_a_bn(g_param_z_bn.x, offset);
        float_to_bn(g_param_z_bn.y, g_init.y);
        copy_bn(g_old_z_bn.x, g_param_z_bn.x);
        copy_bn(g_old_z_bn.y, g_param_z_bn.y);
    }
    else if (g_bf_math == BFMathType::BIG_FLT)
    {
        BigStackSaver saved;
        const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
        float_to_bf(offset, 1e-40);
        mult_a_bf_int(offset, static_cast<U16>(g_col));
        float_to_bf(g_param_z_bf.x, g_init.x);
        add_a_bf(g_param_z_bf.x, offset);
        float_to_bf(g_param_z_bf.y, g_init.y);
        copy_bf(g_old_z_bf.x, g_param_z_bf.x);
        copy_bf(g_old_z_bf.y, g_param_z_bf.y);
    }
    return 1;
}

int calculate_mandel_sticky_orbit()
{
    ++s_math_calls;
    return julia_orbit();
}

void calculate_sticky_perturbation(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
{
    ++s_perturbation_calls;
    s_parameter_displacements.push_back(parameter);
    mandel_perturb(reference, delta, parameter);
}

int initialize_sticky_pixel()
{
    s_seeds.push_back({g_col, g_row});
    s_iteration = 0;
    return 0;
}

int calculate_sticky_orbit()
{
    ++s_math_calls;
    ++s_iteration;
    g_new_z = {1.0 + static_cast<double>(s_iteration % 4) * 0.1 + g_col * 0.01, 1.0 + g_row * 0.01};
    return 0;
}

int calculate_clipped_sticky_orbit()
{
    calculate_sticky_orbit();
    g_new_z = {3.0, 3.0};
    return 0;
}

bool initialize_sticky_image()
{
    return true;
}

} // namespace

class TestStickyOrbitSoundBase : public Test
{
protected:
    void prepare(OrbitDrawMode mode);
    void TearDown() override;
    void run(StickyOrbits &calculation);
    void run(StandardPass &pass);

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockSoundOutput>> m_output{std::make_shared<StrictMock<MockSoundOutput>>()};
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_output};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 8, 0, 0, 7.0, 7.0}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<int> m_saved_x_dots{g_screen_x_dots, 8};
    ValueSaver<int> m_saved_y_dots{g_screen_y_dots, 8};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_and_color{g_and_color, 15};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<bool> m_saved_orbit_save_flag{g_orbit_save_flag, false};
    ValueSaver<int> m_saved_show_dot{g_show_dot, -1};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 3};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<long> m_saved_interval{g_orbit_interval, 1};
    ValueSaver<int> m_saved_skip{g_orbit_skip_points, 0};
    ValueSaver<bool> m_saved_keep_coords{g_keep_screen_coords, false};
    ValueSaver<CalcMode> m_saved_calc_mode{g_std_calc_mode, CalcMode::ORBIT};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<OrbitDrawMode> m_saved_draw_mode{g_draw_mode};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{1, 1}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{1, 1}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_work_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_num_work_list{g_num_work_list, 0};
    std::vector<DComplex> m_samples;
    std::vector<std::array<int, 3>> m_plots;
    std::vector<std::string> m_events;
    TemporaryDirectory m_directory;
    int m_key{};
    bool m_ready{true};
};

void TestStickyOrbitSoundBase::prepare(const OrbitDrawMode mode)
{
    reset_calc_interrupted();
    s_math_calls = 0;
    s_iteration = 0;
    s_seeds.clear();
    g_draw_mode = mode;
    m_specific.flags = FractalFlags::NONE;
    m_specific.orbit_calc = calculate_sticky_orbit;
    m_specific.per_pixel = initialize_sticky_pixel;
    m_specific.per_image = initialize_sticky_image;
    m_specific.calc_type = standard_fractal_type;
    g_dispatch = FractalDispatch{m_specific};
    g_image_region.m_min = {0.0, 0.0};
    g_image_region.m_max = {2.0, 2.0};
    g_image_region.m_3rd = {0.0, 0.0};
    set_normal_dot();
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                m_plots.push_back({x, y, color});
                m_events.emplace_back("plot");
            }));
    expect_pending_driver_key(m_driver, m_key);
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                EXPECT_EQ(0.0, sample.z);
                m_samples.push_back({sample.x, sample.y});
                m_events.emplace_back("sample");
                return true;
            }));
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Invoke([this] { return m_ready; }));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestStickyOrbitSoundBase::TearDown()
{
    reset_calc_interrupted();
    end_resume();
    s_seeds.clear();
    if (!m_directory.path().empty())
    {
        clear_save_library();
        std::error_code ignored;
        m_directory.remove(ignored);
    }
}

void TestStickyOrbitSoundBase::run(StickyOrbits &calculation)
{
    for (int increments = 0; increments < 10000; ++increments)
    {
        if (calculation.iterate())
        {
            return;
        }
    }
    FAIL() << "Sticky orbit calculation did not complete";
}

void TestStickyOrbitSoundBase::run(StandardPass &pass)
{
    for (int increments = 0; increments < 10000; ++increments)
    {
        if (pass.iterate())
        {
            return;
        }
    }
    FAIL() << "Sticky orbit pass did not complete";
}

class TestStickyOrbitSound : public TestStickyOrbitSoundBase, public WithParamInterface<OrbitDrawMode>
{
protected:
    void SetUp() override;
};

void TestStickyOrbitSound::SetUp()
{
    prepare(GetParam());
}

TEST_P(TestStickyOrbitSound, submitsNumericalSampleBeforeEachPlotWithoutPolling)
{
    StickyOrbits calculation;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);

    run(calculation);

    EXPECT_EQ(GetParam() == OrbitDrawMode::RECTANGLE ? 4U : 2U, s_seeds.size());
    EXPECT_EQ(s_seeds.size() * 3, m_samples.size());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    for (std::size_t i = 0; i < m_samples.size(); ++i)
    {
        EXPECT_EQ("sample", m_events[i * 2]);
        EXPECT_EQ("plot", m_events[i * 2 + 1]);
    }
    EXPECT_DOUBLE_EQ(1.1, m_samples.front().x);
    EXPECT_DOUBLE_EQ(1.0, m_samples.front().y);
}

TEST_P(TestStickyOrbitSound, readyOutputRetains2048IterationChunks)
{
    StickyOrbits calculation;
    g_max_iterations = 4097;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);

    EXPECT_FALSE(calculation.iterate());
    EXPECT_EQ(2048, s_math_calls);
    EXPECT_EQ(2048U, m_samples.size());
    EXPECT_EQ(1U, s_seeds.size());
    EXPECT_FALSE(calculation.iterate());
    EXPECT_EQ(4096, s_math_calls);
    EXPECT_EQ(4096U, m_samples.size());
}

struct StickyChunkInput
{
    const char *name;
    long iteration;
    long limit;
    long expected;
};

void PrintTo(const StickyChunkInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStickyChunkLimit : public TestWithParam<StickyChunkInput>
{
};

TEST_P(TestStickyChunkLimit, clampsTheNextChunkToTheIterationLimit)
{
    EXPECT_EQ(GetParam().expected, sticky_orbit_chunk_limit(GetParam().iteration, GetParam().limit));
}

constexpr long MAX_STICKY_ITERATIONS = std::numeric_limits<long>::max();

INSTANTIATE_TEST_SUITE_P(Boundaries, TestStickyChunkLimit,
    Values(StickyChunkInput{"FirstChunk", 0, 4097, 2048}, StickyChunkInput{"SecondChunk", 2048, 4097, 4096},
        StickyChunkInput{"FinalPartialChunk", 4096, 4097, 4097}, StickyChunkInput{"StoppedAtZero", 0, 0, 0},
        StickyChunkInput{"StoppedAtOne", 1, 1, 1}, StickyChunkInput{"StoppedBeforeChunk", 2047, 2047, 2047},
        StickyChunkInput{"StoppedAtChunk", 2048, 2048, 2048}, StickyChunkInput{"StoppedAfterChunk", 2049, 2049, 2049},
        StickyChunkInput{"StoppedAtLongMax", MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS},
        StickyChunkInput{"LongMaxFinalOne", MAX_STICKY_ITERATIONS - 1, MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS},
        StickyChunkInput{
            "LongMaxPartialChunk", MAX_STICKY_ITERATIONS - 2047, MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS},
        StickyChunkInput{
            "LongMaxWholeChunk", MAX_STICKY_ITERATIONS - 2048, MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS},
        StickyChunkInput{
            "LongMaxPenultimateChunk", MAX_STICKY_ITERATIONS - 2049, MAX_STICKY_ITERATIONS, MAX_STICKY_ITERATIONS - 1}),
    [](const TestParamInfo<StickyChunkInput> &info) { return info.param.name; });

TEST_P(TestStickyOrbitSound, rejectedPointRetainsMathAndPlotUntilAcceptance)
{
    StickyOrbits calculation;
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));

    EXPECT_FALSE(calculation.iterate());
    EXPECT_FALSE(calculation.iterate());
    EXPECT_EQ(1, s_math_calls);
    EXPECT_EQ(1U, s_seeds.size());
    EXPECT_TRUE(m_plots.empty());
    g_new_z = {100.0, 200.0};
    run(calculation);

    EXPECT_DOUBLE_EQ(1.1, m_samples.front().x);
    EXPECT_DOUBLE_EQ(1.0, m_samples.front().y);
    EXPECT_EQ(s_seeds.size() * 3, m_samples.size());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    EXPECT_EQ(m_samples.size(), s_math_calls);
}

TEST_P(TestStickyOrbitSound, refusalAtChunkBoundaryResumesWithoutRepeatingAcceptedOutput)
{
    StandardPass pass;
    pass.select(CalcMode::ORBIT);
    g_max_iterations = 2050;
    EXPECT_FALSE(pass.iterate());
    ASSERT_EQ(2048U, m_samples.size());
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false));
    EXPECT_FALSE(pass.iterate());
    EXPECT_EQ(2049, s_math_calls);
    EXPECT_EQ(2048U, m_plots.size());
    pass.suspend();
    ASSERT_EQ(1, g_num_work_list);
    const WorkList work{g_work_list[0]};
    alloc_resume(32, 3);
    pass.save_resume();

    ASSERT_EQ(3, start_resume());
    StandardPass resumed;
    resumed.restore_resume(CalcMode::ORBIT);
    end_resume();
    g_begin_pt = work.begin;
    g_work_pass = work.pass;
    g_num_work_list = 0;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));
    EXPECT_FALSE(resumed.iterate());
    EXPECT_EQ(2048U, m_samples.size());
    run(resumed);

    EXPECT_EQ((GetParam() == OrbitDrawMode::RECTANGLE ? 4U : 2U) * 2050, m_samples.size());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    EXPECT_EQ(m_plots[2047][2], m_plots[2048][2]);
}

TEST_P(TestStickyOrbitSound, skippedIterationsDoNotSubmitSamples)
{
    StickyOrbits calculation;
    g_max_iterations = 5;
    g_orbit_skip_points = 1;
    g_orbit_interval = 2;

    run(calculation);

    EXPECT_EQ(s_seeds.size() * 2, m_samples.size());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    EXPECT_EQ(s_seeds.size() * 5, s_math_calls);
}

TEST_P(TestStickyOrbitSound, suspensionBeforeFirstIncrementDoesNotInitializeTraversal)
{
    StickyOrbits calculation;
    const WorkList expected{g_start_pt, g_stop_pt, g_begin_pt, g_work_symmetry, g_work_pass};

    calculation.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(expected.start, g_work_list[0].start);
    EXPECT_EQ(expected.stop, g_work_list[0].stop);
    EXPECT_EQ(expected.begin, g_work_list[0].begin);
    EXPECT_EQ(expected.pass, g_work_list[0].pass);
    EXPECT_EQ(expected.symmetry, g_work_list[0].symmetry);
    EXPECT_TRUE(calculation.iterate());
    EXPECT_TRUE(s_seeds.empty());
    EXPECT_EQ(0, s_math_calls);
    EXPECT_TRUE(m_samples.empty());
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestStickyOrbitSound, suspendedSeedPrecedesOtherQueuedWork)
{
    StickyOrbits calculation;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false));
    EXPECT_FALSE(calculation.iterate());
    add_work_list({2, 0}, {2, 0}, {2, 0}, 0, 0);

    calculation.suspend();

    ASSERT_EQ(2, g_num_work_list);
    EXPECT_EQ((Point2i{0, 0}), g_work_list[0].begin);
    EXPECT_EQ((Point2i{2, 0}), g_work_list[1].begin);
}

struct StickyClippingInput
{
    const char *name;
    OrbitDrawMode mode;
    int periodicity;
    int iterations_per_seed;
};

void PrintTo(const StickyClippingInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStickyClipping : public TestStickyOrbitSoundBase, public WithParamInterface<StickyClippingInput>
{
protected:
    void SetUp() override;
};

void TestStickyClipping::SetUp()
{
    prepare(GetParam().mode);
    g_dispatch.set_orbit_calc(calculate_clipped_sticky_orbit);
    g_periodicity_check = GetParam().periodicity;
    EXPECT_CALL(*m_output, play(_)).Times(0);
}

TEST_P(TestStickyClipping, clippedSeedDoesNotSubmitSound)
{
    StickyOrbits calculation;

    run(calculation);

    EXPECT_TRUE(m_plots.empty());
    EXPECT_EQ(s_seeds.size() * GetParam().iterations_per_seed, s_math_calls);
}

INSTANTIATE_TEST_SUITE_P(Policies, TestStickyClipping,
    Values(StickyClippingInput{"RectangleWithoutPeriodicity", OrbitDrawMode::RECTANGLE, 0, 3},
        StickyClippingInput{"LineWithoutPeriodicity", OrbitDrawMode::LINE, 0, 3},
        StickyClippingInput{"RectangleWithPeriodicity", OrbitDrawMode::RECTANGLE, 1, 1},
        StickyClippingInput{"LineWithPeriodicity", OrbitDrawMode::LINE, 1, 1}),
    [](const TestParamInfo<StickyClippingInput> &info) { return info.param.name; });

struct StickyPixelCoordinateInput
{
    const char *name;
    double coordinate;
    int expected;
};

void PrintTo(const StickyPixelCoordinateInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStickyPixelCoordinates : public TestStickyOrbitSoundBase,
                                   public WithParamInterface<StickyPixelCoordinateInput>
{
};

TEST_P(TestStickyPixelCoordinates, clipsAndConvertsProjectedCoordinates)
{
    prepare(OrbitDrawMode::LINE);
    g_image_region = {{0.0, 0.0}, {7.0, 7.0}, {0.0, 0.0}};
    g_dispatch.set_orbit_calc(
        []
        {
            g_new_z = {GetParam().coordinate - 0.5, 7.5 - GetParam().coordinate};
            return 0;
        });
    StickyOrbits calculation;

    run(calculation);

    if (GetParam().expected == 0)
    {
        EXPECT_TRUE(m_plots.empty());
        EXPECT_TRUE(m_samples.empty());
    }
    else
    {
        ASSERT_FALSE(m_plots.empty());
        EXPECT_THAT(m_plots, Each(ElementsAre(GetParam().expected, GetParam().expected, _)));
    }
}

INSTANTIATE_TEST_SUITE_P(Coordinates, TestStickyPixelCoordinates,
    Values(StickyPixelCoordinateInput{"BelowHalfPixelTie", std::nextafter(4.0, 0.0), 4},
        StickyPixelCoordinateInput{"AboveHalfPixelTie", std::nextafter(4.0, 8.0), 4},
        StickyPixelCoordinateInput{"BeforeHalfPixelTie", 4.0 - 1e-8, 3},
        StickyPixelCoordinateInput{"AfterHalfPixelTie", 4.0 + 1e-8, 4},
        StickyPixelCoordinateInput{"FirstVisibleBoundary", std::nextafter(1.0, 0.0), 1},
        StickyPixelCoordinateInput{"LastBoundary", std::nextafter(8.0, 0.0), 0},
        StickyPixelCoordinateInput{"Zero", 0.0, 0}, StickyPixelCoordinateInput{"Outside", 8.0, 0},
        StickyPixelCoordinateInput{"PositiveInfinity", std::numeric_limits<double>::infinity(), 0},
        StickyPixelCoordinateInput{"NegativeInfinity", -std::numeric_limits<double>::infinity(), 0},
        StickyPixelCoordinateInput{"NaN", std::numeric_limits<double>::quiet_NaN(), 0},
        StickyPixelCoordinateInput{"BeyondIntegerRange", std::numeric_limits<double>::max(), 0}),
    [](const TestParamInfo<StickyPixelCoordinateInput> &info) { return info.param.name; });

TEST_P(TestStickyOrbitSound, rejectsNegativeResumeCount)
{
    alloc_resume(32, 3);
    put_resume(-1L);
    ASSERT_EQ(3, start_resume());
    StickyOrbits calculation;

    EXPECT_THROW(calculation.restore_resume(), std::logic_error);
}

TEST_P(TestStickyOrbitSound, refusalAtSeedBoundaryDoesNotRestartOrAdvanceSeed)
{
    StickyOrbits calculation;
    g_max_iterations = 1;
    EXPECT_FALSE(calculation.iterate());
    ASSERT_EQ(1U, m_samples.size());
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false));
    EXPECT_FALSE(calculation.iterate());
    EXPECT_EQ(2U, s_seeds.size());
    EXPECT_EQ(2, s_math_calls);
    EXPECT_EQ(1U, m_plots.size());
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));

    run(calculation);

    const std::size_t seed_count{GetParam() == OrbitDrawMode::RECTANGLE ? 4U : 2U};
    EXPECT_EQ(seed_count, s_seeds.size());
    EXPECT_EQ(seed_count, s_math_calls);
    EXPECT_EQ(seed_count, m_samples.size());
    EXPECT_EQ(seed_count, m_plots.size());
}

TEST_P(TestStickyOrbitSound, controllerServicesPlaybackBeforeRetryingPendingPoint)
{
    SoundSession sound{m_output};
    int refused{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, &refused](const SoundSample &sample)
            {
                if (!m_ready)
                {
                    ++refused;
                    return false;
                }
                m_samples.push_back({sample.x, sample.y});
                m_ready = false;
                return true;
            }));
    EXPECT_CALL(*m_output, update()).WillRepeatedly(Invoke([this] { m_ready = true; }));

    EXPECT_EQ(0, render_standard_image(sound));

    EXPECT_GT(refused, 0);
    const std::size_t points{(GetParam() == OrbitDrawMode::RECTANGLE ? 64U : 8U) * 3};
    EXPECT_EQ(points, m_samples.size());
    EXPECT_EQ(points, m_plots.size());
    EXPECT_EQ(points, s_math_calls);
}

TEST_P(TestStickyOrbitSound, controllerInterruptAndResumeDoNotReplayAcceptedSamples)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'x';
                return false;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    EXPECT_CALL(*m_output, mute()).Times(1);
    {
        SoundSession sound{m_output};
        EXPECT_EQ(-1, render_standard_image(sound));
    }
    ASSERT_EQ(1U, m_samples.size());
    ASSERT_EQ(1U, m_plots.size());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    m_key = 0;
    g_resuming = true;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(0, render_standard_image(sound));
    }

    const std::size_t seed_count{GetParam() == OrbitDrawMode::RECTANGLE ? 64U : 8U};
    EXPECT_EQ(seed_count * 3, m_samples.size());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    EXPECT_EQ(m_plots[0][2], m_plots[1][2]);
    EXPECT_EQ(m_samples.size() + 2, s_math_calls);
}

TEST_P(TestStickyOrbitSound, controllerAcceptsLegacyVersion2ResumeData)
{
    add_work_list({0, 0}, {1, 1}, {0, 0}, 0, 0);
    alloc_resume(sizeof(g_work_list) + 20, 2);
    put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    g_resuming = true;
    SoundSession sound{m_output};

    EXPECT_EQ(0, render_standard_image(sound));

    EXPECT_EQ((GetParam() == OrbitDrawMode::RECTANGLE ? 4U : 2U) * 3, m_samples.size());
}

TEST_P(TestStickyOrbitSound, driverRefusalStillRecordsConstantFrequencyWithoutWaiting)
{
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_Z};
    ValueSaver saved_orbit_flags{g_orbit_save_flags, OSF_MIDI};
    ValueSaver saved_hertz{g_base_hertz, 440};
    m_directory.create("id-sticky-sound-");
    set_save_library(m_directory.path());
    EXPECT_CALL(m_driver, sound_on(440)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    SoundSession owner;
    EXPECT_EQ(0, render_standard_image());
    EXPECT_TRUE(owner.finish());

    std::ifstream file{m_directory.path() / "sound/sound001.txt"};
    ASSERT_TRUE(file.good());
    std::size_t tones{};
    for (std::string line; std::getline(file, line);)
    {
        EXPECT_EQ("440", line);
        ++tones;
    }
    EXPECT_EQ((GetParam() == OrbitDrawMode::RECTANGLE ? 64U : 8U) * 3, tones);
    EXPECT_EQ(tones, m_plots.size());
}

INSTANTIATE_TEST_SUITE_P(RectangleAndLine, TestStickyOrbitSound, Values(OrbitDrawMode::RECTANGLE, OrbitDrawMode::LINE),
    [](const TestParamInfo<OrbitDrawMode> &info)
    { return info.param == OrbitDrawMode::RECTANGLE ? "Rectangle" : "Line"; });

class TestStickyPerturbationBase : public TestStickyOrbitSoundBase
{
protected:
    void prepare_perturbation(OrbitDrawMode mode);
    void TearDown() override;
    void clear_trace();
    void forbid_ordinary_orbits();
    void run_perturbation(StickyOrbits &calculation);
    void expect_output(const std::vector<DComplex> &samples, const std::vector<std::array<int, 3>> &plots);

    PertEngine m_engine;
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    bool m_big_float{};
};

void TestStickyPerturbationBase::prepare_perturbation(const OrbitDrawMode mode)
{
    prepare(mode);
    set_bailout_formula(Bailout::MOD);
    m_specific.flags = FractalFlags::PERTURB;
    m_specific.per_pixel = initialize_mandel_sticky_pixel;
    m_specific.orbit_calc = calculate_mandel_sticky_orbit;
    m_specific.pert_pt = calculate_sticky_perturbation;
    g_dispatch = FractalDispatch{m_specific};
    m_engine.initialize_frame({0.25, 0.25}, {1.0, 0.0}, {0.0, 1.0});
    clear_trace();
}

void TestStickyPerturbationBase::TearDown()
{
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
    }
    TestStickyOrbitSoundBase::TearDown();
}

void TestStickyPerturbationBase::clear_trace()
{
    m_samples.clear();
    m_plots.clear();
    m_events.clear();
    s_seeds.clear();
    s_math_calls = 0;
    s_perturbation_calls = 0;
    s_parameter_displacements.clear();
}

void TestStickyPerturbationBase::forbid_ordinary_orbits()
{
    clear_trace();
    g_dispatch.set_orbit_calc([]() -> int { throw std::logic_error("Sticky orbits bypassed perturbation"); });
}

void TestStickyPerturbationBase::run_perturbation(StickyOrbits &calculation)
{
    bool done{};
    for (int chunk = 0; chunk < 10000 && !done; ++chunk)
    {
        done = calculation.iterate(&m_engine);
    }
    ASSERT_TRUE(done);
}

void TestStickyPerturbationBase::expect_output(
    const std::vector<DComplex> &samples, const std::vector<std::array<int, 3>> &plots)
{
    ASSERT_EQ(samples.size(), m_samples.size());
    EXPECT_EQ(plots, m_plots);
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        EXPECT_NEAR(samples[i].x, m_samples[i].x, 1e-14);
        EXPECT_NEAR(samples[i].y, m_samples[i].y, 1e-14);
    }
}

class TestStickyPerturbation : public TestStickyPerturbationBase, public WithParamInterface<OrbitDrawMode>
{
protected:
    void SetUp() override;
};

void TestStickyPerturbation::SetUp()
{
    prepare_perturbation(GetParam());
}

class TestStickyOutputComparison : public TestStickyPerturbationBase
{
protected:
    void capture_ordinary();
    void TearDown() override;

    std::unique_ptr<StickyOrbits> m_ordinary;
    std::vector<DComplex> m_expected_samples;
    std::vector<std::array<int, 3>> m_expected_plots;
    std::vector<Point2i> m_expected_seeds;
};

void TestStickyOutputComparison::capture_ordinary()
{
    m_ordinary = std::make_unique<StickyOrbits>();
    run(*m_ordinary);
    m_expected_samples = m_samples;
    m_expected_plots = m_plots;
    m_expected_seeds = s_seeds;
    forbid_ordinary_orbits();
}

void TestStickyOutputComparison::TearDown()
{
    m_ordinary.reset();
    TestStickyPerturbationBase::TearDown();
}

class TestStickySeedOrder : public TestStickyOutputComparison, public WithParamInterface<OrbitDrawMode>
{
protected:
    void SetUp() override;
};

void TestStickySeedOrder::SetUp()
{
    prepare_perturbation(GetParam());
    capture_ordinary();
}

TEST_P(TestStickySeedOrder, numericalSamplesAndPlotsRetainTheOrdinarySeedOrder)
{
    ASSERT_FALSE(m_expected_samples.empty());
    StickyOrbits perturbation;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);

    run_perturbation(perturbation);

    expect_output(m_expected_samples, m_expected_plots);
    EXPECT_EQ(m_expected_seeds, s_seeds);
    EXPECT_GT(s_perturbation_calls, 0);
    EXPECT_EQ(0, s_math_calls);
}

INSTANTIATE_TEST_SUITE_P(DrawModes, TestStickySeedOrder, Values(OrbitDrawMode::RECTANGLE, OrbitDrawMode::LINE),
    [](const TestParamInfo<OrbitDrawMode> &info)
    { return info.param == OrbitDrawMode::RECTANGLE ? "Rectangle" : "Line"; });

class TestStickyFilteredSequence : public TestStickyOutputComparison, public WithParamInterface<OrbitDrawMode>
{
protected:
    void SetUp() override;
};

void TestStickyFilteredSequence::SetUp()
{
    prepare_perturbation(GetParam());
    g_max_iterations = 5;
    g_orbit_skip_points = 1;
    g_orbit_interval = 2;
    capture_ordinary();
}

TEST_P(TestStickyFilteredSequence, skipAndIntervalRetainTheSamePointSequence)
{
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    expect_output(m_expected_samples, m_expected_plots);
    EXPECT_GT(s_perturbation_calls, m_samples.size());
}

INSTANTIATE_TEST_SUITE_P(DrawModes, TestStickyFilteredSequence, Values(OrbitDrawMode::RECTANGLE, OrbitDrawMode::LINE),
    [](const TestParamInfo<OrbitDrawMode> &info)
    { return info.param == OrbitDrawMode::RECTANGLE ? "Rectangle" : "Line"; });

TEST_P(TestStickyPerturbation, refusedPointDoesNotAdvanceMathOrChangeItsCoordinates)
{
    forbid_ordinary_orbits();
    StickyOrbits perturbation;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(false));

    EXPECT_FALSE(perturbation.iterate(&m_engine));
    EXPECT_FALSE(perturbation.iterate(&m_engine));

    EXPECT_EQ(1, s_perturbation_calls);
    EXPECT_EQ(1, s_seeds.size());
    EXPECT_TRUE(m_plots.empty());
    g_new_z = {100.0, 200.0};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));
    run_perturbation(perturbation);
    ASSERT_FALSE(m_samples.empty());
    EXPECT_DOUBLE_EQ(0.25, m_samples.front().x);
    EXPECT_DOUBLE_EQ(0.375, m_samples.front().y);
}

struct StickyPeriodicityInput
{
    const char *name;
    OrbitDrawMode mode;
    int periodicity;
};

void PrintTo(const StickyPeriodicityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStickyPeriodicityOutput : public TestStickyOutputComparison, public WithParamInterface<StickyPeriodicityInput>
{
protected:
    void SetUp() override;
};

void TestStickyPeriodicityOutput::SetUp()
{
    prepare_perturbation(GetParam().mode);
    g_magnitude_limit = 0.1;
    clear_trace();
    g_periodicity_check = GetParam().periodicity;
    g_dispatch.set_orbit_calc(calculate_mandel_sticky_orbit);
    capture_ordinary();
}

TEST_P(TestStickyPeriodicityOutput, escapedAndClippedPointsRetainTheOrdinaryOutput)
{
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    expect_output(m_expected_samples, m_expected_plots);
}

INSTANTIATE_TEST_SUITE_P(Policies, TestStickyPeriodicityOutput,
    Values(StickyPeriodicityInput{"RectangleWithoutPeriodicity", OrbitDrawMode::RECTANGLE, 0},
        StickyPeriodicityInput{"LineWithoutPeriodicity", OrbitDrawMode::LINE, 0},
        StickyPeriodicityInput{"RectangleWithPeriodicity", OrbitDrawMode::RECTANGLE, 1},
        StickyPeriodicityInput{"LineWithPeriodicity", OrbitDrawMode::LINE, 1}),
    [](const TestParamInfo<StickyPeriodicityInput> &info) { return info.param.name; });

TEST_P(TestStickyPerturbation, referenceReplacementDoesNotReplayAcceptedOutput)
{
    StickyOrbits ordinary;
    run(ordinary);
    const std::vector<DComplex> samples{m_samples};
    const std::vector<std::array<int, 3>> plots{m_plots};
    forbid_ordinary_orbits();
    m_engine.set_glitch_tolerance(8.0);
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    expect_output(samples, plots);
    EXPECT_GT(m_engine.reference_count(), 1);
}

TEST_P(TestStickyPerturbation, clippedPointReleasesTheUnfinishedOrbitBeforeSelectingAnotherSeed)
{
    g_periodicity_check = 1;
    g_image_region = {{0.0, 0.0}, {0.1, 0.1}, {0.0, 0.0}};
    forbid_ordinary_orbits();
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    EXPECT_TRUE(m_samples.empty());
    EXPECT_TRUE(m_plots.empty());
    EXPECT_EQ(GetParam() == OrbitDrawMode::RECTANGLE ? 4 : 2, s_seeds.size());
    EXPECT_EQ(s_seeds.size(), s_perturbation_calls);
}

struct StickyPrecisionInput
{
    const char *name;
    OrbitDrawMode mode;
    FractalType allocator;
};

void PrintTo(const StickyPrecisionInput &input, std::ostream *out)
{
    *out << input.name;
}

const StickyPrecisionInput STICKY_PRECISION_INPUTS[]{{"MandelRectangle", OrbitDrawMode::RECTANGLE, FractalType::MANDEL},
    {"MandelLine", OrbitDrawMode::LINE, FractalType::MANDEL},
    {"PowerRectangle", OrbitDrawMode::RECTANGLE, FractalType::MANDEL_Z_POWER},
    {"PowerLine", OrbitDrawMode::LINE, FractalType::MANDEL_Z_POWER}};

class TestStickyHighPrecision : public TestStickyPerturbationBase, public WithParamInterface<StickyPrecisionInput>
{
protected:
    void SetUp() override;
};

void TestStickyHighPrecision::SetUp()
{
    prepare_perturbation(GetParam().mode);
    m_engine.finish();
    {
        ValueSaver<FractalType> saved_type{g_fractal_type, GetParam().allocator};
        init_bf_dec(60);
    }
    m_big_float = true;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    float_to_bf(center.x, 0.25);
    float_to_bf(center.y, 0.25);
    forbid_ordinary_orbits();
}

TEST_P(TestStickyHighPrecision, seedsRetainSubDoubleCoordinateDifferences)
{
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    ASSERT_FALSE(m_samples.empty());
    EXPECT_EQ(m_samples.size(), m_plots.size());
    EXPECT_EQ(1, m_engine.reference_count());
    EXPECT_THAT(s_parameter_displacements,
        Contains(Truly([](const std::complex<double> &delta) { return delta.real() > 0.0 && delta.real() < 1e-30; })));
    EXPECT_EQ(0, s_math_calls);
}

INSTANTIATE_TEST_SUITE_P(Allocators, TestStickyHighPrecision, ValuesIn(STICKY_PRECISION_INPUTS),
    [](const TestParamInfo<StickyPrecisionInput> &info) { return info.param.name; });

class TestStickyHighPrecisionBailout : public TestStickyHighPrecision
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<StickyOrbits> m_seed_evaluation;
};

void TestStickyHighPrecisionBailout::SetUp()
{
    TestStickyHighPrecision::SetUp();
    m_seed_evaluation = std::make_unique<StickyOrbits>();
    run_perturbation(*m_seed_evaluation);
    ASSERT_FALSE(m_samples.empty());
}

void TestStickyHighPrecisionBailout::TearDown()
{
    m_seed_evaluation.reset();
    TestStickyHighPrecision::TearDown();
}

TEST_P(TestStickyHighPrecisionBailout, bailoutAfterSeedEvaluationMatchesOrdinaryOutput)
{
    g_magnitude_limit = 0.1;
    g_dispatch.set_orbit_calc(calculate_mandel_sticky_orbit);
    clear_trace();
    StickyOrbits ordinary_bailout;
    run(ordinary_bailout);
    const std::vector<DComplex> samples{m_samples};
    const std::vector<std::array<int, 3>> plots{m_plots};
    forbid_ordinary_orbits();
    StickyOrbits precise_bailout;

    run_perturbation(precise_bailout);

    expect_output(samples, plots);
    g_magnitude_limit = 4.0;
}

INSTANTIATE_TEST_SUITE_P(Allocators, TestStickyHighPrecisionBailout, ValuesIn(STICKY_PRECISION_INPUTS),
    [](const TestParamInfo<StickyPrecisionInput> &info) { return info.param.name; });

struct StickyBurningShipInput
{
    const char *name;
    OrbitDrawMode mode;
    int degree;
};

void PrintTo(const StickyBurningShipInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStickyBurningShipOutput : public TestStickyOutputComparison, public WithParamInterface<StickyBurningShipInput>
{
protected:
    void SetUp() override;

    ValueSaver<double> m_saved_power{g_params[2]};
};

void TestStickyBurningShipOutput::SetUp()
{
    prepare_perturbation(GetParam().mode);
    g_image_region = {{-2.0, -2.0}, {2.0, 2.0}, {-2.0, -2.0}};
    g_fractal_type = FractalType::BURNING_SHIP;
    m_specific.pert_pt = burning_ship_perturb;
    m_specific.pert_ref = burning_ship_ref_pt;
    m_specific.pert_ref_bf = burning_ship_ref_pt_bf;
    g_params[2] = GetParam().degree;
    clear_trace();
    g_dispatch.set_orbit_calc(burning_ship_orbit);
    capture_ordinary();
    ASSERT_FALSE(m_expected_samples.empty());
    m_engine.finish();
    m_engine.initialize_frame({0.25, -0.25}, {1.0, 0.0}, {0.0, 1.0});
}

TEST_P(TestStickyBurningShipOutput, plottingAndSoundRetainTheOrdinaryOrientation)
{
    StickyOrbits perturbation;

    run_perturbation(perturbation);

    expect_output(m_expected_samples, m_expected_plots);
}

INSTANTIATE_TEST_SUITE_P(Degrees, TestStickyBurningShipOutput,
    Values(StickyBurningShipInput{"QuadraticRectangle", OrbitDrawMode::RECTANGLE, 2},
        StickyBurningShipInput{"QuadraticLine", OrbitDrawMode::LINE, 2},
        StickyBurningShipInput{"CubicRectangle", OrbitDrawMode::RECTANGLE, 3},
        StickyBurningShipInput{"CubicLine", OrbitDrawMode::LINE, 3}),
    [](const TestParamInfo<StickyBurningShipInput> &info) { return info.param.name; });

TEST_P(TestStickyPerturbation, controllerInterruptAndResumeRetainAcceptedSoundAndPlots)
{
    g_user.perturbation = PerturbationMode::YES;
    {
        SoundSession sound{m_output};
        ASSERT_EQ(0, render_standard_image(sound));
    }
    const std::vector<DComplex> samples{m_samples};
    const std::vector<std::array<int, 3>> plots{m_plots};
    ASSERT_GT(samples.size(), 1);
    clear_trace();
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'x';
                return false;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](const int key) { m_key = key; }));
    EXPECT_CALL(*m_output, mute());
    {
        SoundSession sound{m_output};
        EXPECT_EQ(-1, render_standard_image(sound));
    }
    ASSERT_EQ(1, m_samples.size());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    m_key = 0;
    g_resuming = true;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y});
                return true;
            }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(0, render_standard_image(sound));
    }

    expect_output(samples, plots);
    EXPECT_GT(s_perturbation_calls, 0);
    EXPECT_EQ(0, s_math_calls);
}

INSTANTIATE_TEST_SUITE_P(RectangleAndLine, TestStickyPerturbation,
    Values(OrbitDrawMode::RECTANGLE, OrbitDrawMode::LINE), [](const TestParamInfo<OrbitDrawMode> &info)
    { return info.param == OrbitDrawMode::RECTANGLE ? "Rectangle" : "Line"; });

} // namespace id::test
