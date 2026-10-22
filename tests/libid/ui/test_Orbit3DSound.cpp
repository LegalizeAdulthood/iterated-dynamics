// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/sound.h>
#include <engine/SoundOutput.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <fractals/ifs.h>
#include <fractals/lorenz.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <io/check_write_file.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <ui/KeyboardHandler.h>
#include <ui/SoundSession.h>
#include <ui/StereoController.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

class MockOrbit3DSound : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

class MockStereoOuterHandler : public KeyboardHandler
{
public:
    MOCK_METHOD(bool, handle_key, (int), (override));
};

long s_math_calls{};
long s_limit{102};
std::function<void()> s_on_orbit;

double orbit_value(const long iteration)
{
    // The first 100 points establish bounds; later points exercise output.
    return iteration <= 100 ? (iteration % 2 == 0 ? 2.0 : 0.0) : 2.0 + std::min(iteration - 101, 2L);
}

int calculate_sound_orbit(double *x, double *y, double *z)
{
    ++s_math_calls;
    g_max_count = s_limit;
    *x = orbit_value(g_color_iter);
    *y = 2.0 * *x;
    *z = 3.0 * *x;
    if (s_on_orbit)
    {
        s_on_orbit();
    }
    return 0;
}

} // namespace

class TestOrbit3DSound : public TestWithParam<FractalType>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void run(Orbit3D &calculation);
    int render(SoundSession &sound);
    void create_save_directory();

    MockDriver m_driver;
    std::shared_ptr<StrictMock<MockOrbit3DSound>> m_output{std::make_shared<StrictMock<MockOrbit3DSound>>()};
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
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<std::string> m_saved_orbit_name{g_orbit_save_name, "orbits.raw"};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_OFF};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
    ValueSaver<int> m_saved_x_rot{g_x_rot, 0};
    ValueSaver<int> m_saved_y_rot{g_y_rot, 0};
    ValueSaver<int> m_saved_z_rot{g_z_rot, 0};
    ValueSaver<int> m_saved_viewer_z{g_viewer_z, 0};
    ValueSaver<int> m_saved_x_shift{g_x_shift, 0};
    ValueSaver<int> m_saved_y_shift{g_y_shift, 0};
    ValueSaver<int> m_saved_x_shift1{g_x_shift1, 0};
    ValueSaver<int> m_saved_y_shift1{g_y_shift1, 0};
    ValueSaver<int> m_saved_xx_adjust{g_xx_adjust, 0};
    ValueSaver<int> m_saved_yy_adjust{g_yy_adjust, 0};
    ValueSaver<int> m_saved_xx_adjust1{g_xx_adjust1, 0};
    ValueSaver<int> m_saved_yy_adjust1{g_yy_adjust1, 0};
    ValueSaver<int> m_saved_shift_x{g_shift_x, 0};
    ValueSaver<int> m_saved_shift_y{g_shift_y, 0};
    ValueSaver<int> m_saved_eye_separation{g_eye_separation, 10};
    ValueSaver<int> m_saved_convergence{g_converge_x_adjust, 0};
    ValueSaver<math::Point2i> m_saved_adjust{g_adjust_3d, math::Point2i{0, 0}};
    ValueSaver<GlassesType> m_saved_glasses{g_glasses_type, GlassesType::NONE};
    ValueSaver<StereoImage> m_saved_eye{g_which_image, StereoImage::NONE};
    ValueSaver<Display3DMode> m_saved_display{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_saved_map_set{g_map_set, false};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<decltype(g_standard_plot)> m_saved_standard_plot{g_standard_plot};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    std::vector<std::array<double, 3>> m_samples;
    std::vector<std::array<int, 5>> m_plots;
    std::vector<StereoImage> m_eyes;
    std::filesystem::path m_directory;
    int m_key{};
    bool m_ready{true};
};

void TestOrbit3DSound::SetUp()
{
    reset_calc_interrupted();
    s_math_calls = 0;
    s_limit = 102;
    s_on_orbit = {};
    g_fractal_type = GetParam();
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    g_dispatch.set_orbit_calc(reinterpret_cast<OrbitCalc>(calculate_sound_orbit));
    g_image_region.m_min = {-20.0, -20.0};
    g_image_region.m_max = {20.0, 20.0};
    g_image_region.m_3rd = {-20.0, -20.0};
    ASSERT_TRUE(orbit3d_per_image());
    set_normal_dot();
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                m_plots.push_back({x, y, x, y, color});
                m_eyes.push_back(g_which_image);
            }));
    EXPECT_CALL(m_driver, draw_line(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int old_x, int old_y, int color)
            {
                m_plots.push_back({x, y, old_x, old_y, color});
                m_eyes.push_back(g_which_image);
            }));
    EXPECT_CALL(m_driver, read_pixel(_, _)).WillRepeatedly(Return(0));
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
                return true;
            }));
    EXPECT_CALL(*m_output, update()).Times(AnyNumber());
    EXPECT_CALL(*m_output, ready()).WillRepeatedly(Invoke([this] { return m_ready; }));
    EXPECT_CALL(*m_output, finish()).WillRepeatedly(Return(true));
}

void TestOrbit3DSound::TearDown()
{
    reset_calc_interrupted();
    s_on_orbit = {};
    if (!m_directory.empty())
    {
        clear_save_library();
        std::error_code ignored;
        std::filesystem::remove_all(m_directory, ignored);
    }
}

void TestOrbit3DSound::run(Orbit3D &calculation)
{
    for (int increments = 0; increments < 10000; ++increments)
    {
        if (calculation.done())
        {
            return;
        }
        calculation.iterate();
    }
    FAIL() << "Orbit3D did not complete";
}

void TestOrbit3DSound::create_save_directory()
{
    m_directory = std::filesystem::temp_directory_path() /
        ("id-orbit3d-sound-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(m_directory));
    set_save_library(m_directory);
}

int TestOrbit3DSound::render(SoundSession &sound)
{
    NonStandardFractal calculation;
    calculation.start();
    if (calculation.select() != NonStandardFractal::Operation::ORBIT_3D)
    {
        throw std::logic_error("Expected a selected 3D orbit operation");
    }
    StereoController controller{calculation, sound};
    const int result{controller.run()};
    calculation.finish(result != 0);
    return result;
}

TEST_P(TestOrbit3DSound, selectsEvery3dOrbitWithoutRenderingOrPolling)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    for (const FractalType type :
        {FractalType::KAM_3D, FractalType::LORENZ_3D, FractalType::ROSSLER, FractalType::PICKOVER,
            FractalType::LORENZ_3D1, FractalType::LORENZ_3D3, FractalType::LORENZ_3D4, FractalType::ICON_3D})
    {
        g_fractal_type = type;
        g_cur_fractal_specific = get_fractal_specific(type);
        g_dispatch = FractalDispatch{*g_cur_fractal_specific};
        EXPECT_EQ(nullptr, g_cur_fractal_specific->calc_type);
        NonStandardFractal calculation;
        calculation.start();

        EXPECT_EQ(NonStandardFractal::Operation::ORBIT_3D, calculation.select());

        EXPECT_FALSE(calculation.done());
        EXPECT_FALSE(calculation.view_done());
        EXPECT_EQ(0, g_color_iter);
        EXPECT_EQ(0, s_math_calls);
        EXPECT_TRUE(m_plots.empty());
        EXPECT_TRUE(m_samples.empty());
        calculation.suspend();
        EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    }
}

TEST_P(TestOrbit3DSound, selectedViewsRequireExplicitAdvanceAndRestartOrbitState)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    for (const GlassesType glasses : {GlassesType::PHOTO, GlassesType::STEREO_PAIR})
    {
        g_glasses_type = glasses;
        NonStandardFractal calculation;
        calculation.start();
        ASSERT_EQ(NonStandardFractal::Operation::ORBIT_3D, calculation.select());
        EXPECT_EQ(StereoImage::RED, g_which_image);
        EXPECT_THROW(calculation.next_view(), std::logic_error);
        for (int point = 0; point < 103; ++point)
        {
            calculation.iterate();
        }

        ASSERT_TRUE(calculation.view_done());
        EXPECT_FALSE(calculation.done());
        EXPECT_EQ(CalcStatus::IN_PROGRESS, g_calc_status);
        EXPECT_THROW(calculation.iterate(), std::logic_error);
        calculation.next_view();

        EXPECT_EQ(StereoImage::BLUE, g_which_image);
        EXPECT_EQ(0, g_color_iter);
        EXPECT_FALSE(calculation.view_done());
        for (int point = 0; point < 103; ++point)
        {
            calculation.iterate();
        }
        EXPECT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        EXPECT_THROW(calculation.next_view(), std::logic_error);
        calculation.finish(false);
    }
}

TEST_P(TestOrbit3DSound, selectedInterruptionIsNonResumableInEitherView)
{
    g_glasses_type = GlassesType::PHOTO;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    for (const bool second_eye : {false, true})
    {
        NonStandardFractal calculation;
        calculation.start();
        calculation.select();
        if (second_eye)
        {
            for (int point = 0; point < 103; ++point)
            {
                calculation.iterate();
            }
            calculation.next_view();
        }
        calculation.iterate();

        calculation.suspend();

        EXPECT_TRUE(calculation.done());
        EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
        EXPECT_TRUE(g_resume_data.empty());
        EXPECT_THROW(calculation.view_done(), std::logic_error);
    }
}

TEST_P(TestOrbit3DSound, submitsTransformedValuesOnlyAfterBoundsAreEstablished)
{
    Orbit3D calculation;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    for (int i = 0; i < 100; ++i)
    {
        calculation.iterate();
    }
    EXPECT_TRUE(m_samples.empty());
    EXPECT_TRUE(m_plots.empty());

    run(calculation);

    const std::vector<std::array<double, 3>> expected{{1.0, 2.0, 0.0}, {2.0, 4.0, 3.0}, {3.0, 6.0, 6.0}};
    EXPECT_EQ(expected, m_samples);
    EXPECT_EQ(3U, m_plots.size());
    EXPECT_TRUE(calculation.done());
    calculation.iterate();
    EXPECT_EQ(103, s_math_calls);
}

TEST_P(TestOrbit3DSound, refusalRetainsTransformedPointAndDoesNotRepeatMathOrPlot)
{
    Orbit3D calculation;
    std::vector<std::array<double, 3>> attempts;
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, &attempts](const SoundSample &sample)
            {
                attempts.push_back({sample.x, sample.y, sample.z});
                EXPECT_TRUE(m_plots.empty());
                return attempts.size() > 2;
            }));
    for (int i = 0; i < 101; ++i)
    {
        calculation.iterate();
    }

    calculation.iterate();
    EXPECT_EQ(101, s_math_calls);
    EXPECT_EQ(101, g_color_iter);
    EXPECT_TRUE(m_plots.empty());
    calculation.iterate();

    ASSERT_EQ(3U, attempts.size());
    EXPECT_EQ(attempts[0], attempts[1]);
    EXPECT_EQ(attempts[0], attempts[2]);
    EXPECT_EQ(101, s_math_calls);
    EXPECT_EQ(1U, m_plots.size());
}

TEST_P(TestOrbit3DSound, finalPendingPointCompletesBeforeDone)
{
    Orbit3D calculation;
    for (int i = 0; i < 102; ++i)
    {
        calculation.iterate();
    }
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(true));

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(103, g_color_iter);
    EXPECT_EQ(2U, m_plots.size());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(3U, m_plots.size());
    EXPECT_EQ(103, s_math_calls);
}

TEST_P(TestOrbit3DSound, clippedPointsDoNotProduceSound)
{
    g_xx_adjust = 100;
    Orbit3D calculation;
    EXPECT_CALL(*m_output, play(_)).Times(0);

    run(calculation);

    EXPECT_EQ(103, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestOrbit3DSound, unboundedPointCompletesWithoutSound)
{
    g_xx_adjust = 100000;
    Orbit3D calculation;
    EXPECT_CALL(*m_output, play(_)).Times(0);

    run(calculation);

    EXPECT_EQ(101, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestOrbit3DSound, rawOrbitDataIsNotDuplicatedOnRefusal)
{
    create_save_directory();
    g_orbit_save_flags = OSF_RAW;
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    {
        Orbit3D calculation;
        run(calculation);
    }

    std::ifstream file{m_directory / "orbit/orbits.raw"};
    ASSERT_TRUE(file);
    std::string header;
    std::getline(file, header);
    EXPECT_EQ("pointlist x y z color", header);
    for (long iteration = 1; iteration <= 103; ++iteration)
    {
        double x{}, y{}, z{};
        int color{};
        ASSERT_TRUE(file >> x >> y >> z >> color);
        EXPECT_EQ(orbit_value(iteration), x);
        EXPECT_EQ(2.0 * x, y);
        EXPECT_EQ(3.0 * x, z);
        EXPECT_EQ(15, color);
    }
    std::string extra;
    EXPECT_FALSE(file >> extra);
}

TEST_P(TestOrbit3DSound, controllerPreservesReadyOutputPollingCadenceAndFinishesWithoutMute)
{
    const std::shared_ptr<StrictMock<MockStereoOuterHandler>> outer{
        std::make_shared<StrictMock<MockStereoOuterHandler>>()};
    const ScopedKeyboardHandler scope{outer};
    SoundSession sound{m_output};
    EXPECT_CALL(m_driver, key_pressed()).Times(103).WillRepeatedly(Return(0));
    EXPECT_CALL(*m_output, mute()).Times(0);
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(103, s_math_calls);
    EXPECT_EQ(3U, m_samples.size());
    EXPECT_EQ(3U, m_plots.size());
    EXPECT_CALL(*outer, handle_key('?')).WillOnce(Return(true));
    EXPECT_TRUE(dispatch_keyboard_key('?'));
}

TEST_P(TestOrbit3DSound, controllerServicesRefusalWithoutRepeatedMath)
{
    SoundSession sound{m_output};
    int refused{};
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(Invoke(
            [this, &refused](const SoundSample &sample)
            {
                if (m_samples.empty() && refused++ == 0)
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
    EXPECT_EQ(103, s_math_calls);
    EXPECT_EQ(3U, m_samples.size());
    EXPECT_EQ(3U, m_plots.size());
}

TEST_P(TestOrbit3DSound, completionDrainsReleaseTailWithoutAdditionalPlottingOrMute)
{
    SoundSession sound{m_output};
    {
        InSequence sequence;
        EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
        EXPECT_CALL(m_driver, delay(1));
        EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    }
    EXPECT_CALL(*m_output, mute()).Times(0);

    EXPECT_EQ(0, render(sound));

    EXPECT_EQ(103, s_math_calls);
    EXPECT_EQ(3U, m_plots.size());
}

TEST_P(TestOrbit3DSound, menuInterruptionMutesWithoutCommittingRefusedPoint)
{
    EXPECT_CALL(*m_output, play(_))
        .WillOnce(Invoke(
            [this](const SoundSample &)
            {
                m_key = 'x';
                m_ready = false;
                return false;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([this](int key) { m_key = key; }));
    EXPECT_CALL(*m_output, mute()).WillOnce(Invoke([this] { m_ready = true; }));
    SoundSession sound{m_output};

    EXPECT_EQ(-1, render(sound));

    EXPECT_EQ('x', m_key);
    EXPECT_EQ(101, s_math_calls);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestOrbit3DSound, controllerFailureRestoresScreenAndOuterKeyboardHandler)
{
    const ValueSaver saved_viewport{g_viewport};
    g_viewport.enabled = true;
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_screen_x_dots = 40;
    const std::shared_ptr<StrictMock<MockStereoOuterHandler>> outer{
        std::make_shared<StrictMock<MockStereoOuterHandler>>()};
    const ScopedKeyboardHandler scope{outer};
    EXPECT_CALL(*m_output, play(_)).WillOnce(Throw(std::runtime_error("Sound output failed")));
    EXPECT_CALL(*m_output, mute()).Times(1);
    SoundSession sound{m_output};

    EXPECT_THROW(render(sound), std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(40, g_logical_screen.x_dots);
    EXPECT_EQ(20, g_logical_screen.y_dots);
    EXPECT_EQ(0, g_logical_screen.x_offset);
    EXPECT_FALSE(g_viewport.enabled);
    EXPECT_CALL(*outer, handle_key('?')).WillOnce(Return(true));
    EXPECT_TRUE(dispatch_keyboard_key('?'));
}

TEST_P(TestOrbit3DSound, stereoIfsWithoutPerImageSetupRendersEachEyeOnce)
{
    g_fractal_type = FractalType::IFS_3D;
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    ASSERT_EQ(nullptr, g_dispatch.per_image());
    g_glasses_type = GlassesType::STEREO_PAIR;
    const ValueSaver saved_definition{g_ifs_definition, std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}};
    const ValueSaver saved_dimension{g_ifs_dim, IFSDimension::THREE};
    const ValueSaver saved_transforms{g_num_affine_transforms, 1};
    std::vector<StereoImage> views;
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [&views]
            {
                if (views.empty() || views.back() != g_which_image)
                {
                    views.push_back(g_which_image);
                }
                return 0;
            }));
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::IFS_3D, calculation.select());
    SoundSession sound{m_output};
    StereoController controller{calculation, sound};

    EXPECT_EQ(0, controller.run());

    EXPECT_THAT(views, ElementsAre(StereoImage::RED, StereoImage::BLUE));
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(1025, g_color_iter);
    EXPECT_EQ(0, s_math_calls);
    calculation.finish(false);
}

TEST_P(TestOrbit3DSound, stereoRefusalPreservesSamplesAndEyeOrder)
{
    create_save_directory();
    g_glasses_type = GlassesType::SUPERIMPOSE;
    g_sound_flag = SOUNDFLAG_Z;
    g_orbit_save_flags = OSF_MIDI;
    g_orbit_delay = 1;
    ValueSaver saved_hertz{g_base_hertz, 220};
    std::vector<StereoImage> sound_eyes;
    EXPECT_CALL(m_driver, sound_on(_))
        .WillRepeatedly(Invoke(
            [&sound_eyes](int)
            {
                sound_eyes.push_back(g_which_image);
                return true;
            }));
    EXPECT_CALL(m_driver, sound_off()).Times(4);
    EXPECT_CALL(m_driver, delay(1)).Times(AnyNumber());
    EXPECT_CALL(m_driver, mute()).Times(0);
    // Make the active port busy during calculation, after the UI's ready check.
    s_on_orbit = []
    {
        if (g_color_iter == 101)
        {
            EXPECT_TRUE(g_sound->play({9.0, 8.0, 7.0}));
        }
    };

    SoundSession owner;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    EXPECT_EQ(0, render(sound));
    EXPECT_TRUE(owner.finish());

    EXPECT_EQ(103, s_math_calls);
    EXPECT_THAT(m_eyes,
        ElementsAre(StereoImage::RED, StereoImage::BLUE, StereoImage::RED, StereoImage::BLUE, StereoImage::RED,
            StereoImage::BLUE));
    EXPECT_THAT(sound_eyes, ElementsAre(StereoImage::RED, StereoImage::RED, StereoImage::RED, StereoImage::RED));
    std::ifstream file{m_directory / "sound/sound001.txt"};
    ASSERT_TRUE(file);
    std::vector<int> frequencies;
    for (int frequency; file >> frequency;)
    {
        frequencies.push_back(frequency);
    }
    EXPECT_THAT(frequencies, ElementsAre(920, 220, 520, 820));
}

TEST_P(TestOrbit3DSound, clippedRedEyeDoesNotAddSoundForVisibleBlueEye)
{
    g_glasses_type = GlassesType::SUPERIMPOSE;
    g_sound_flag = SOUNDFLAG_Z;
    EXPECT_CALL(m_driver, sound_on(_)).Times(0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    s_on_orbit = [] { g_xx_adjust = 100; };

    SoundSession owner;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    EXPECT_EQ(0, render(sound));
    EXPECT_TRUE(owner.finish());

    EXPECT_EQ(103, s_math_calls);
    EXPECT_THAT(m_eyes, ElementsAre(StereoImage::BLUE, StereoImage::BLUE, StereoImage::BLUE));
}

INSTANTIATE_TEST_SUITE_P(PointsAndLines, TestOrbit3DSound, Values(FractalType::LORENZ_3D, FractalType::PICKOVER));

} // namespace id::test
