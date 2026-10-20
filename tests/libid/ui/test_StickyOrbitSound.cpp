// SPDX-License-Identifier: GPL-3.0-only
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/StandardPass.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <ui/KeyboardHandler.h>
#include <ui/SoundSession.h>
#include <ui/standard_fractal.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
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

class MockStickySound : public SoundOutput
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
long s_iteration{};
std::vector<Point2i> s_seeds;

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

class TestStickyOrbitSound : public TestWithParam<OrbitDrawMode>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void run(StickyOrbits &calculation);
    void run(StandardPass &pass);

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockStickySound>> m_output{std::make_shared<StrictMock<MockStickySound>>()};
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
    std::filesystem::path m_directory;
    int m_key{};
    bool m_ready{true};
};

void TestStickyOrbitSound::SetUp()
{
    reset_calc_interrupted();
    s_math_calls = 0;
    s_iteration = 0;
    s_seeds.clear();
    g_draw_mode = GetParam();
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
                EXPECT_EQ(0.0, sample.z);
                m_samples.push_back({sample.x, sample.y});
                m_events.emplace_back("sample");
                return true;
            }));
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Invoke([this] { return m_ready; }));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestStickyOrbitSound::TearDown()
{
    reset_calc_interrupted();
    end_resume();
    s_seeds.clear();
    if (!m_directory.empty())
    {
        clear_save_library();
        std::error_code ignored;
        std::filesystem::remove_all(m_directory, ignored);
    }
}

void TestStickyOrbitSound::run(StickyOrbits &calculation)
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

void TestStickyOrbitSound::run(StandardPass &pass)
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

TEST_P(TestStickyOrbitSound, clippedPointsDoNotSubmitSound)
{
    StickyOrbits calculation;
    g_dispatch.set_orbit_calc(calculate_clipped_sticky_orbit);
    EXPECT_CALL(*m_output, play(_)).Times(0);

    run(calculation);

    EXPECT_TRUE(m_plots.empty());
    EXPECT_EQ(s_seeds.size() * 3, s_math_calls);
}

TEST_P(TestStickyOrbitSound, clippedPointWithPeriodicityEndsCurrentSeedWithoutSound)
{
    StickyOrbits calculation;
    g_dispatch.set_orbit_calc(calculate_clipped_sticky_orbit);
    g_periodicity_check = 1;
    EXPECT_CALL(*m_output, play(_)).Times(0);

    run(calculation);

    EXPECT_TRUE(m_plots.empty());
    EXPECT_EQ(s_seeds.size(), s_math_calls);
}

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

    EXPECT_EQ(0, standard_fractal(sound));

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
        EXPECT_EQ(-1, standard_fractal(sound));
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
        EXPECT_EQ(0, standard_fractal(sound));
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

    EXPECT_EQ(0, standard_fractal(sound));

    EXPECT_EQ((GetParam() == OrbitDrawMode::RECTANGLE ? 4U : 2U) * 3, m_samples.size());
}

TEST_P(TestStickyOrbitSound, driverRefusalStillRecordsConstantFrequencyWithoutWaiting)
{
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_Z};
    ValueSaver saved_orbit_flags{g_orbit_save_flags, OSF_MIDI};
    ValueSaver saved_hertz{g_base_hertz, 440};
    m_directory = std::filesystem::temp_directory_path() /
        ("id-sticky-sound-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    set_save_library(m_directory);
    EXPECT_CALL(m_driver, sound_on(440)).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);

    SoundSession owner;
    EXPECT_EQ(0, standard_fractal());
    EXPECT_TRUE(owner.finish());

    std::ifstream file{m_directory / "sound/sound001.txt"};
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

} // namespace id::test
