// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <io/check_write_file.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <ui/KeyboardHandler.h>
#include <ui/OrbitController.h>
#include <ui/orbit2d.h>
#include <ui/SoundSession.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

class MockOrbit2DSound : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

long s_math_calls{};
std::vector<std::string> s_events;

int calculate_sound_orbit(double *x, double *y, double *z)
{
    ++s_math_calls;
    s_events.emplace_back("math");
    *x += 1.0;
    *y += 2.0;
    *z += 3.0;
    return g_color_iter >= 3;
}

std::string read_text(const std::filesystem::path &path)
{
    std::ifstream file{path};
    if (!file)
    {
        throw std::runtime_error("Can't read orbit sound test output");
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

} // namespace

class TestOrbit2DSound : public TestWithParam<FractalType>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void run(Orbit2D &calculation);
    int render();
    int render(SoundSession &sound);
    void create_save_directory();
    std::vector<std::array<double, 3>> expected_samples() const;
    void load_legacy_state(
        Orbit2D &calculation, int count, long iteration, double x = 1.0, double y = 1.0, double z = 1.0);

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockOrbit2DSound>> m_output{std::make_shared<StrictMock<MockOrbit2DSound>>()};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_output};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{20, 20, 0, 0, 19.0, 19.0}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<int> m_saved_x_dots{g_screen_x_dots, 20};
    ValueSaver<int> m_saved_y_dots{g_screen_y_dots, 20};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_and_color{g_and_color, 15};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 1};
    ValueSaver<long> m_saved_max_count{g_max_count};
    ValueSaver<long> m_saved_iteration{g_color_iter};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<std::string> m_saved_orbit_name{g_orbit_save_name, "orbits.raw"};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_OFF};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    std::vector<std::array<double, 3>> m_samples;
    std::vector<std::array<int, 3>> m_points;
    std::vector<std::array<int, 5>> m_lines;
    std::filesystem::path m_directory;
    int m_key{};
    bool m_ready{true};
};

void TestOrbit2DSound::SetUp()
{
    reset_calc_interrupted();
    s_math_calls = 0;
    s_events.clear();
    g_fractal_type = GetParam();
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    g_dispatch.set_orbit_calc(reinterpret_cast<OrbitCalc>(calculate_sound_orbit));
    g_image_region.m_min = {0.0, 0.0};
    g_image_region.m_max = {10.0, 10.0};
    g_image_region.m_3rd = {0.0, 0.0};
    ASSERT_TRUE(orbit3d_per_image());
    set_normal_dot();
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                m_points.push_back({x, y, color});
                s_events.emplace_back("plot");
            }));
    EXPECT_CALL(m_driver, draw_line(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int old_x, int old_y, int color)
            {
                m_lines.push_back({x, y, old_x, old_y, color});
                s_events.emplace_back("plot");
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
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                s_events.emplace_back("sample");
                return true;
            }));
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Invoke([this] { return m_ready; }));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestOrbit2DSound::TearDown()
{
    reset_calc_interrupted();
    end_resume();
    s_events.clear();
    if (!m_directory.empty())
    {
        clear_save_library();
        std::error_code ignored;
        std::filesystem::remove_all(m_directory, ignored);
    }
}

void TestOrbit2DSound::run(Orbit2D &calculation)
{
    for (int increments = 0; increments < 10; ++increments)
    {
        if (calculation.done())
        {
            return;
        }
        calculation.iterate();
    }
    FAIL() << "Orbit2D did not complete";
}

int TestOrbit2DSound::render()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return render(sound);
}

int TestOrbit2DSound::render(SoundSession &sound)
{
    NonStandardFractal calculation;
    calculation.start();
    if (calculation.select() != NonStandardFractal::Operation::ORBIT_2D)
    {
        throw std::logic_error("Unexpected selected orbit operation");
    }
    const int result{orbit2d_type(calculation, sound)};
    calculation.finish(result != 0);
    return result;
}

void TestOrbit2DSound::create_save_directory()
{
    m_directory = std::filesystem::temp_directory_path() /
        ("id-orbit2d-sound-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    set_save_library(m_directory);
}

std::vector<std::array<double, 3>> TestOrbit2DSound::expected_samples() const
{
    if (GetParam() == FractalType::LORENZ)
    {
        return {{1.0, 1.0, 1.0}, {2.0, 4.0, 3.0}, {3.0, 7.0, 5.0}};
    }
    return {{1.0, 1.0, 1.0}, {2.0, 3.0, 4.0}, {3.0, 5.0, 7.0}};
}

void TestOrbit2DSound::load_legacy_state(
    Orbit2D &calculation, const int count, const long iteration, const double x, const double y, const double z)
{
    alloc_resume(100, 1);
    put_resume(count, 2, -1, -1, x, y, z, 0, 0.0, iteration);
    calculation.resume();
}

TEST_P(TestOrbit2DSound, numericalSamplePrecedesPlotAndMathWithoutPollingOrMuting)
{
    Orbit2D calculation;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    run(calculation);

    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_THAT(s_events, ElementsAre("sample", "plot", "math", "sample", "plot", "math", "sample", "plot", "math"));
    EXPECT_EQ(GetParam() == FractalType::LORENZ ? 1U : 3U, m_points.size());
    EXPECT_EQ(GetParam() == FractalType::LORENZ ? 2U : 0U, m_lines.size());
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.done());
    calculation.iterate();
    EXPECT_EQ(3, s_math_calls);
}

TEST_P(TestOrbit2DSound, refusalDoesNotAdvancePlotMathColorOrIteration)
{
    Orbit2D calculation;
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Return(false))
        .WillOnce(Return(false))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));

    calculation.iterate();
    calculation.iterate();

    EXPECT_EQ(1, g_color_iter);
    EXPECT_EQ(0, s_math_calls);
    EXPECT_TRUE(m_points.empty());
    EXPECT_TRUE(m_lines.empty());
    run(calculation);
    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(3, s_math_calls);
}

TEST_P(TestOrbit2DSound, pendingFinalPointMustCompleteEvenAfterIterationLimit)
{
    Orbit2D calculation;
    load_legacy_state(calculation, 0, g_max_count);
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(true));

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, s_math_calls);
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    ASSERT_EQ(1U, m_points.size());
    EXPECT_EQ(1, s_math_calls);
}

TEST_P(TestOrbit2DSound, refusalAtColorChangeDoesNotAdvanceColorTwice)
{
    Orbit2D calculation;
    load_legacy_state(calculation, 1000, 1000);
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(true));

    calculation.iterate();
    EXPECT_EQ(1001, g_color_iter);
    calculation.iterate();

    EXPECT_EQ(1001, g_color_iter);
    ASSERT_EQ(1U, m_points.size());
    EXPECT_EQ(3, m_points[0][2]);
    EXPECT_EQ(1, s_math_calls);
}

TEST_P(TestOrbit2DSound, clippedPointsDoNotProduceSound)
{
    Orbit2D calculation;
    load_legacy_state(calculation, 0, 0L, 20.0, 20.0);
    EXPECT_CALL(*m_output, play(_)).Times(0);

    run(calculation);

    EXPECT_TRUE(m_points.empty());
    EXPECT_TRUE(m_lines.empty());
    EXPECT_EQ(3, s_math_calls);
}

TEST_P(TestOrbit2DSound, unboundedPointCompletesWithoutSoundOrOrbitMath)
{
    Orbit2D calculation;
    load_legacy_state(calculation, 0, 0L, 1000000.0, 1000000.0);
    EXPECT_CALL(*m_output, play(_)).Times(0);

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(0, s_math_calls);
}

TEST_P(TestOrbit2DSound, rawOrbitDataIsNotDuplicatedOnRefusal)
{
    create_save_directory();
    g_orbit_save_flags = OSF_RAW;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    {
        Orbit2D calculation;
        run(calculation);
    }

    EXPECT_EQ("pointlist x y z color\n2 3 0 15\n3 5 0 15\n", read_text(m_directory / "orbit/orbits.raw"));
    EXPECT_EQ(3, s_math_calls);
}

TEST_P(TestOrbit2DSound, controllerKeepsReadyOutputPollingCadenceAndFinishesWithoutMute)
{
    SoundSession sound{m_output};
    EXPECT_CALL(m_driver, key_pressed()).Times(3).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(expected_samples(), m_samples);
}

TEST_P(TestOrbit2DSound, controllerServicesPlaybackAndRetriesWithoutRepeatedMath)
{
    SoundSession sound{m_output};
    int refused{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, &refused](const SoundSample &sample)
            {
                if (m_samples.size() == 1 && refused++ == 0)
                {
                    m_ready = false;
                    return false;
                }
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));
    EXPECT_CALL(*m_output, update()).WillRepeatedly(Invoke([this] { m_ready = true; }));

    EXPECT_EQ(0, render(sound));

    EXPECT_GT(refused, 0);
    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(3, s_math_calls);
}

TEST_P(TestOrbit2DSound, controllerDrainsReleaseTailWithoutAdditionalPlottingOrMute)
{
    SoundSession sound{m_output};
    {
        InSequence sequence;
        EXPECT_CALL(*m_output, finish())
            .WillOnce(Invoke(
                []
                {
                    EXPECT_EQ(3, s_math_calls);
                    return false;
                }));
        EXPECT_CALL(m_driver, delay(1));
        EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    }
    EXPECT_CALL(*m_output, mute()).Times(0);

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(3, s_math_calls);
    EXPECT_EQ(3U, m_points.size() + m_lines.size());
}

TEST_P(TestOrbit2DSound, menuInterruptionMutesAndResumesTheUnacceptedPoint)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'x';
                m_ready = false;
                return false;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    EXPECT_CALL(*m_output, mute()).WillOnce(Invoke([this] { m_ready = true; }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(-1, render(sound));
    }
    EXPECT_EQ('x', m_key);
    ASSERT_EQ(1U, m_samples.size());
    EXPECT_EQ(1, s_math_calls);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    m_key = 0;
    ASSERT_TRUE(orbit3d_per_image());
    g_resuming = true;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(0, render(sound));
    }

    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(3, g_color_iter);
    EXPECT_EQ(3, s_math_calls);
    EXPECT_EQ(3U, m_points.size() + m_lines.size());
}

TEST_P(TestOrbit2DSound, driverRefusalStillRecordsMappedFrequenciesWithoutWaiting)
{
    create_save_directory();
    g_sound_flag = SOUNDFLAG_Z;
    g_orbit_save_flags = OSF_MIDI;
    ValueSaver saved_hertz{g_base_hertz, 220};
    EXPECT_CALL(m_driver, sound_on(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    SoundSession owner;
    EXPECT_EQ(0, render());
    EXPECT_TRUE(owner.finish());

    EXPECT_EQ(GetParam() == FractalType::LORENZ ? "320\n520\n720\n" : "320\n620\n920\n",
        read_text(m_directory / "sound/sound001.txt"));
}

TEST_P(TestOrbit2DSound, stepwiseControllerAdvancesOnePointPerCall)
{
    SoundSession sound{m_output};
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());
    OrbitController controller{calculation, sound};
    EXPECT_THROW(controller.iterate(), std::logic_error);
    EXPECT_THROW(controller.result(), std::logic_error);
    EXPECT_THROW(controller.suspend(), std::logic_error);
    EXPECT_CALL(*m_output, mute()).Times(0);

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_samples.empty());
    EXPECT_EQ(0, s_math_calls);
    EXPECT_THROW(controller.start(), std::logic_error);
    controller.iterate();
    ASSERT_EQ(1U, m_samples.size());
    EXPECT_EQ(expected_samples()[0], m_samples[0]);
    EXPECT_EQ(1, s_math_calls);
    EXPECT_FALSE(controller.done());
    for (int steps = 0; steps < 10 && !controller.done(); ++steps)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(expected_samples(), m_samples);
    controller.iterate();
    EXPECT_EQ(3, s_math_calls);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
    EXPECT_THROW(controller.suspend(), std::logic_error);
    calculation.finish(false);
}

TEST_P(TestOrbit2DSound, stepwiseControllerRetriesRefusedPointWithoutRepeatingMath)
{
    SoundSession sound{m_output};
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());
    OrbitController controller{calculation, sound};
    controller.start();
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_ready = false;
                return false;
            }))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));

    controller.iterate();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(0, s_math_calls);
    EXPECT_TRUE(m_samples.empty());
    EXPECT_TRUE(m_points.empty());
    EXPECT_TRUE(m_lines.empty());
    EXPECT_CALL(*m_output, update()).WillRepeatedly(Invoke([this] { m_ready = true; }));
    controller.iterate();
    EXPECT_EQ(1, s_math_calls);
    for (int steps = 0; steps < 10 && !controller.done(); ++steps)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(3, s_math_calls);
    calculation.finish(false);
}

TEST_P(TestOrbit2DSound, stepwiseControllerSuspendsAndResumesAfterAcceptedPoint)
{
    EXPECT_CALL(*m_output, mute()).Times(1);
    {
        SoundSession sound{m_output};
        NonStandardFractal calculation;
        calculation.start();
        ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());
        OrbitController controller{calculation, sound};
        controller.start();
        controller.iterate();
        ASSERT_EQ(1, s_math_calls);

        controller.suspend();

        EXPECT_TRUE(controller.done());
        EXPECT_EQ(-1, controller.result());
        EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
        EXPECT_FALSE(dispatch_keyboard_key('x'));
    }
    SoundSession sound{m_output};
    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, resumed.select());
    OrbitController controller{resumed, sound};
    controller.start();
    for (int steps = 0; steps < 10 && !controller.done(); ++steps)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(3, s_math_calls);
    resumed.finish(false);
}

TEST_P(TestOrbit2DSound, stepwiseControllerFailureClosesCalculationAndReleasesHandler)
{
    SoundSession sound{m_output};
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());
    OrbitController controller{calculation, sound};
    controller.start();
    EXPECT_CALL(*m_output, play(_)).WillOnce(Throw(std::runtime_error("Playback failed")));
    EXPECT_CALL(*m_output, mute()).Times(1);

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, s_math_calls);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
}

TEST_P(TestOrbit2DSound, stepwiseControllerFinalizationFailureDoesNotMuteClosedOutput)
{
    SoundSession sound{m_output};
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());
    OrbitController controller{calculation, sound};
    controller.start();
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("Close failed")));
    EXPECT_CALL(*m_output, mute()).Times(0);

    EXPECT_THROW(
        {
            for (int steps = 0; steps < 10 && !controller.done(); ++steps)
            {
                controller.iterate();
            }
        },
        std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(3, s_math_calls);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
}

INSTANTIATE_TEST_SUITE_P(PointsAndLines, TestOrbit2DSound, Values(FractalType::HENON, FractalType::LORENZ));

} // namespace id::test
