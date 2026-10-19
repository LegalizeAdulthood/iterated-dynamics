// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/resume.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <io/check_write_file.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <ui/dynamic2d.h>
#include <ui/KeyboardHandler.h>
#include <ui/SoundSession.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
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

class MockDynamicSound : public SoundOutput
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
double s_x_step{0.125};
double s_y_step{-0.25};
std::vector<std::string> s_events;

int calculate_sound_orbit(double *x, double *y, double *)
{
    ++s_math_calls;
    s_events.emplace_back("math");
    *x += s_x_step;
    *y += s_y_step;
    return 0;
}

} // namespace

class TestDynamicSound : public TestWithParam<FractalType>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void run(Dynamic2D &calculation);
    void create_save_directory();
    void save_legacy_state(long count, int x_step = 0, int y_step = 0, double x = 1.0, double y = 9.0);
    void interrupt_and_resume(std::size_t accepted);
    std::vector<std::array<double, 3>> expected_samples() const;

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockDynamicSound>> m_output{std::make_shared<StrictMock<MockDynamicSound>>()};
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
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 3};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<std::string> m_saved_orbit_name{g_orbit_save_name, "orbits.raw"};
    ValueSaver<int> m_saved_skip{g_orbit_skip_points, 0};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_OFF};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
    ValueSaver<double> m_saved_intervals{g_params[0], -2.0};
    ValueSaver<double> m_saved_step_size{g_params[1], 0.01};
    ValueSaver<double> m_saved_a{g_params[2], 0.0};
    ValueSaver<double> m_saved_b{g_params[3], 0.0};
    ValueSaver<decltype(g_delta_x)> m_saved_delta_x{g_delta_x, 0.125};
    ValueSaver<decltype(g_delta_y)> m_saved_delta_y{g_delta_y, 0.125};
    ValueSaver<decltype(g_delta_x2)> m_saved_delta_x2{g_delta_x2, 0.0};
    ValueSaver<decltype(g_delta_y2)> m_saved_delta_y2{g_delta_y2, 0.0};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    std::vector<std::array<double, 3>> m_samples;
    std::vector<std::array<int, 5>> m_plots;
    std::filesystem::path m_directory;
    int m_key{};
    bool m_ready{true};
};

void TestDynamicSound::SetUp()
{
    reset_calc_interrupted();
    s_math_calls = 0;
    s_x_step = 0.125;
    s_y_step = -0.25;
    s_events.clear();
    g_fractal_type = GetParam();
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    g_dispatch.set_orbit_calc(reinterpret_cast<OrbitCalc>(calculate_sound_orbit));
    g_image_region.m_min = {0.0, 0.0};
    g_image_region.m_max = {10.0, 10.0};
    g_image_region.m_3rd = {0.0, 0.0};
    ASSERT_TRUE(dynamic2d_per_image());
    set_normal_dot();
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                m_plots.push_back({x, y, x, y, color});
                s_events.emplace_back("plot");
            }));
    EXPECT_CALL(m_driver, draw_line(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int old_x, int old_y, int color)
            {
                m_plots.push_back({x, y, old_x, old_y, color});
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

void TestDynamicSound::TearDown()
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

void TestDynamicSound::run(Dynamic2D &calculation)
{
    for (int increments = 0; increments < 100; ++increments)
    {
        if (calculation.done())
        {
            return;
        }
        calculation.iterate();
    }
    FAIL() << "Dynamic2D did not complete";
}

void TestDynamicSound::create_save_directory()
{
    m_directory = std::filesystem::temp_directory_path() /
        ("id-dynamic-sound-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    set_save_library(m_directory);
}

void TestDynamicSound::save_legacy_state(
    const long count, const int x_step, const int y_step, const double x, const double y)
{
    alloc_resume(100, 1);
    put_resume(count, 3, -1, -1, x, y, x_step, y_step);
}

std::vector<std::array<double, 3>> TestDynamicSound::expected_samples() const
{
    std::vector<std::array<double, 3>> result;
    for (int row = 0; row <= 2; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            double x{19.0 * (col + 0.5) / 2.0 * 0.125};
            double y{10.0 - 19.0 * (row + 0.5) / 2.0 * 0.125};
            for (int iteration = 0; iteration < 3; ++iteration)
            {
                result.push_back({x, y, 0.0});
                x += 0.125;
                y -= 0.25;
            }
        }
    }
    return result;
}

void TestDynamicSound::interrupt_and_resume(const std::size_t accepted)
{
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, accepted](const SoundSample &sample)
            {
                if (m_samples.size() == accepted)
                {
                    m_key = 'x';
                    m_ready = false;
                    return false;
                }
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    EXPECT_CALL(*m_output, mute()).WillOnce(Invoke([this] { m_ready = true; }));
    {
        SoundSession sound{m_output};
        EXPECT_EQ(-1, dynamic2d_type(sound));
    }
    EXPECT_EQ('x', m_key);
    EXPECT_EQ(accepted, m_samples.size());
    EXPECT_EQ(accepted, s_math_calls);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    m_key = 0;
    ASSERT_TRUE(dynamic2d_per_image());
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
        EXPECT_EQ(0, dynamic2d_type(sound));
    }

    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(18, s_math_calls);
    const std::size_t plots_per_seed{GetParam() == FractalType::DYNAMIC ? 3U : 2U};
    ASSERT_EQ(6 * plots_per_seed, m_plots.size());
    for (std::size_t i = 0; i < m_plots.size(); ++i)
    {
        EXPECT_EQ(i / plots_per_seed + 1, m_plots[i][4]);
    }
}

TEST_P(TestDynamicSound, numericalSamplesPrecedePlotsAndMathWithoutPollingOrMuting)
{
    Dynamic2D calculation;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    run(calculation);

    EXPECT_EQ(expected_samples(), m_samples);
    std::vector<std::string> expected_events;
    for (int seed = 0; seed < 6; ++seed)
    {
        for (int iteration = 0; iteration < 3; ++iteration)
        {
            expected_events.emplace_back("sample");
            if (iteration > 0 || GetParam() == FractalType::DYNAMIC)
            {
                expected_events.emplace_back("plot");
            }
            expected_events.emplace_back("math");
        }
    }
    EXPECT_EQ(expected_events, s_events);
    EXPECT_TRUE(calculation.done());
    calculation.iterate();
    EXPECT_EQ(18, s_math_calls);
}

TEST_P(TestDynamicSound, refusalDoesNotAdvanceCoordinatesOrPlot)
{
    Dynamic2D calculation;
    std::vector<std::array<double, 3>> attempts;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [&attempts](const SoundSample &sample)
            {
                attempts.push_back({sample.x, sample.y, sample.z});
                return false;
            }));
    calculation.iterate(); // existing seed-start yield
    calculation.iterate();
    calculation.iterate();

    EXPECT_EQ(0, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
    ASSERT_EQ(2U, attempts.size());
    EXPECT_EQ(expected_samples()[0], attempts[0]);
    EXPECT_EQ(attempts[0], attempts[1]);
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this](const SoundSample &sample)
            {
                m_samples.push_back({sample.x, sample.y, sample.z});
                return true;
            }));

    run(calculation);

    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(18, s_math_calls);
}

TEST_P(TestDynamicSound, soundBeforeSkippedPlotsIsPreserved)
{
    g_orbit_skip_points = 2;
    Dynamic2D calculation;

    run(calculation);

    EXPECT_EQ(expected_samples(), m_samples);
    EXPECT_EQ(6U, m_plots.size());
}

TEST_P(TestDynamicSound, clippedPointsDoNotProduceSound)
{
    save_legacy_state(0L, 0, 0, 100.0, 100.0);
    Dynamic2D calculation;
    calculation.resume();
    EXPECT_CALL(*m_output, play(_)).Times(0);
    calculation.iterate();

    calculation.iterate();

    EXPECT_EQ(3, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestDynamicSound, unboundedPointCompletesWithoutSoundOrMath)
{
    save_legacy_state(0L, 0, 0, 1000000.0, 1000000.0);
    Dynamic2D calculation;
    calculation.resume();
    EXPECT_CALL(*m_output, play(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    calculation.iterate();

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(0, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestDynamicSound, controllerPreserves2048IterationPollingCadence)
{
    g_max_iterations = 4100;
    s_x_step = 0.0;
    s_y_step = 0.0;
    SoundSession sound{m_output};
    EXPECT_CALL(m_driver, key_pressed()).Times(25).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));

    EXPECT_EQ(0, dynamic2d_type(sound));

    EXPECT_EQ(6 * 4100, s_math_calls);
    EXPECT_EQ(6U * 4100, m_samples.size());
}

TEST_P(TestDynamicSound, controllerRetriesAcrossIterationBoundaryWithoutAdditionalYield)
{
    g_max_iterations = 2050;
    s_x_step = 0.0;
    s_y_step = 0.0;
    save_legacy_state(2047L, 1, 2);
    g_resuming = true;
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
    EXPECT_CALL(m_driver, key_pressed()).Times(4).WillRepeatedly(Return(0));
    SoundSession sound{m_output};

    EXPECT_EQ(0, dynamic2d_type(sound));

    EXPECT_GT(refused, 0);
    EXPECT_EQ(3, s_math_calls);
    EXPECT_EQ(3U, m_samples.size());
    EXPECT_EQ(3U, m_plots.size());
}

TEST_P(TestDynamicSound, suspensionAtIterationBoundaryPreservesPendingPoint)
{
    g_max_iterations = 2050;
    s_x_step = 0.0;
    s_y_step = 0.0;
    save_legacy_state(2048L);
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    {
        Dynamic2D calculation;
        calculation.resume();
        calculation.iterate(); // existing iteration-boundary yield
        calculation.iterate(); // refused sample
        calculation.suspend();
    }
    EXPECT_EQ(0, s_math_calls);
    Dynamic2D resumed;
    resumed.resume();

    resumed.iterate();

    EXPECT_EQ(2, s_math_calls);
    EXPECT_EQ(2U, m_plots.size());
}

TEST_P(TestDynamicSound, menuInterruptionBeforeFirstPointMutesAndResumes)
{
    interrupt_and_resume(0);
}

TEST_P(TestDynamicSound, menuInterruptionAtNextSeedMutesAndResumesWithoutRepeatingSeed)
{
    interrupt_and_resume(3);
}

TEST_P(TestDynamicSound, completionDrainsReleaseTailWithoutAdditionalPlottingOrMute)
{
    SoundSession sound{m_output};
    {
        InSequence sequence;
        EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
        EXPECT_CALL(m_driver, delay(1));
        EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    }
    EXPECT_CALL(*m_output, mute()).Times(0);

    EXPECT_EQ(0, dynamic2d_type(sound));

    EXPECT_EQ(18, s_math_calls);
    EXPECT_EQ(expected_samples(), m_samples);
}

TEST_P(TestDynamicSound, rawOrbitDataIsNotDuplicatedOnRefusal)
{
    create_save_directory();
    g_orbit_save_flags = OSF_RAW;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    {
        Dynamic2D calculation;
        run(calculation);
    }

    std::ifstream file{m_directory / "orbit/orbits.raw"};
    ASSERT_TRUE(file);
    std::string header;
    std::getline(file, header);
    EXPECT_EQ("pointlist x y z color", header);
    for (const std::array<double, 3> &sample : expected_samples())
    {
        double x{}, y{}, z{};
        int color{};
        ASSERT_TRUE(file >> x >> y >> z >> color);
        EXPECT_EQ(sample[0] + 0.125, x);
        EXPECT_EQ(sample[1] - 0.25, y);
        EXPECT_EQ(0.0, z);
        EXPECT_EQ(15, color);
    }
    std::string extra;
    EXPECT_FALSE(file >> extra);
    EXPECT_EQ(18, s_math_calls);
}

TEST_P(TestDynamicSound, driverRefusalRecordsZeroZChannelWithoutPlaybackWaiting)
{
    create_save_directory();
    g_sound_flag = SOUNDFLAG_Z;
    g_orbit_save_flags = OSF_MIDI;
    ValueSaver saved_hertz{g_base_hertz, 220};
    EXPECT_CALL(m_driver, sound_on(220)).Times(18).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    EXPECT_EQ(0, dynamic2d_type());

    std::ifstream file{m_directory / "sound/sound001.txt"};
    ASSERT_TRUE(file);
    std::vector<int> frequencies;
    for (int frequency; file >> frequency;)
    {
        frequencies.push_back(frequency);
    }
    EXPECT_THAT(frequencies, ElementsAreArray(std::vector<int>(18, 220)));
}

INSTANTIATE_TEST_SUITE_P(DynamicAndCloud, TestDynamicSound, Values(FractalType::DYNAMIC, FractalType::MANDEL_CLOUD));

} // namespace id::test
