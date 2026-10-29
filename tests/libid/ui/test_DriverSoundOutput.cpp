// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <fractals/fractalp.h>
#include <io/check_write_file.h>
#include <io/library.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

#define private public
#include <ui/DriverSoundOutput.h>
#undef private

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

namespace id::test
{

namespace
{

using Source = DriverSoundOutput::Source;
using Clock = DriverSoundOutput::Clock;

std::string read_text(const fs::path &path)
{
    std::ifstream file{path};
    if (!file)
    {
        throw std::runtime_error("Can't read sound test output");
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

void write_text(const fs::path &path, const std::string &text)
{
    std::ofstream file{path};
    file << text;
    if (!file)
    {
        throw std::runtime_error("Can't write sound test input");
    }
}

struct MappingCase
{
    const char *name;
    Source source;
    int mode;
    bool scaled;
    SoundSample sample;
    int expected;
};

void PrintTo(const MappingCase &mapping, std::ostream *out)
{
    *out << mapping.name;
}

} // namespace

class TestDriverSoundOutput : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_base_hertz{g_base_hertz, 220};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver<int> m_saved_orbit_save_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_orbit_delay{g_orbit_delay, 10};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<int> m_saved_x_dots{g_logical_screen.x_dots, 800};
    ValueSaver<int> m_saved_y_dots{g_logical_screen.y_dots, 600};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
    fs::path m_directory;
    bool m_directory_created{};
    Clock::time_point m_time{};
    long long m_time_ms{1234};
    int m_now_calls{};
    int m_timestamp_calls{};
    DriverSoundOutput::NowFn m_now{[this]
        {
            ++m_now_calls;
            return m_time;
        }};
    DriverSoundOutput::TimestampFn m_timestamp{[this]
        {
            ++m_timestamp_calls;
            return m_time_ms;
        }};
};

void TestDriverSoundOutput::SetUp()
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{info->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_directory = fs::temp_directory_path() /
        ("id-driver-sound-" + name + "-" + std::to_string(Clock::now().time_since_epoch().count()));
    m_directory_created = fs::create_directory(m_directory);
    ASSERT_TRUE(m_directory_created);
    set_save_library(m_directory);
}

void TestDriverSoundOutput::TearDown()
{
    clear_save_library();
    if (m_directory_created)
    {
        std::error_code ignored;
        fs::remove_all(m_directory, ignored);
    }
}

class TestDriverSoundMapping : public TestDriverSoundOutput, public WithParamInterface<MappingCase>
{
};

TEST_P(TestDriverSoundMapping, preservesLegacyFrequency)
{
    const MappingCase &mapping{GetParam()};
    g_sound_flag = mapping.mode;
    g_debug_flag = mapping.scaled ? DebugFlags::FORCE_SCALED_SOUND_FORMULA : DebugFlags::NONE;
    DriverSoundOutput output{mapping.source, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(mapping.expected)).WillOnce(Return(false));

    EXPECT_TRUE(output.play(mapping.sample));
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
}

INSTANTIATE_TEST_SUITE_P(Sources, TestDriverSoundMapping,
    Values(MappingCase{"OverlayX", Source::OVERLAY, SOUNDFLAG_X, false, {10, 20, 30}, 230},
        MappingCase{"OverlayY", Source::OVERLAY, SOUNDFLAG_Y, false, {10, 20, 30}, 240},
        MappingCase{"OverlayZ", Source::OVERLAY, SOUNDFLAG_Z, false, {10, 20, 30}, 250},
        MappingCase{"ScaledOverlayX", Source::OVERLAY, SOUNDFLAG_X, true, {400, 150, 0}, 720},
        MappingCase{"ScaledOverlayY", Source::OVERLAY, SOUNDFLAG_Y, true, {400, 150, 0}, 470},
        MappingCase{"ScaledOverlayZ", Source::OVERLAY, SOUNDFLAG_Z, true, {400, 150, 0}, 470},
        MappingCase{"OrbitX", Source::ORBIT, SOUNDFLAG_X, false, {-1.125, 2.25, 3.5}, 107},
        MappingCase{"OrbitY", Source::ORBIT, SOUNDFLAG_Y, false, {-1.125, 2.25, 3.5}, 445},
        MappingCase{"OrbitZ", Source::ORBIT, SOUNDFLAG_Z, false, {-1.125, 2.25, 3.5}, 570},
        MappingCase{"StickyOrbit", Source::STICKY_ORBIT, SOUNDFLAG_Z, false, {10, 20, 30}, 220},
        MappingCase{"RestoredImage", Source::RESTORED_IMAGE, SOUNDFLAG_X, false, {128, 0, 0}, 1720},
        MappingCase{"RestoredImageTruncation", Source::RESTORED_IMAGE, SOUNDFLAG_Z, false, {1, 0, 0}, 231}),
    [](const TestParamInfo<MappingCase> &info) { return info.param.name; });

class TestDriverSoundRecording : public TestDriverSoundOutput, public WithParamInterface<std::tuple<bool, bool>>
{
};

TEST_P(TestDriverSoundRecording, recordsBeforeDeviceAttemptAndHandlesEitherDeviceOutcome)
{
    const bool recording{std::get<0>(GetParam())};
    const bool device_accepts{std::get<1>(GetParam())};
    g_orbit_save_flags = recording ? OSF_MIDI : 0;
    const fs::path path{m_directory / "sound/sound001.txt"};
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_FALSE(fs::exists(m_directory / "sound"));
    EXPECT_CALL(m_driver, sound_on(230))
        .WillOnce(Invoke(
            [&](int)
            {
                if (recording)
                {
                    EXPECT_EQ("230\n", read_text(path));
                }
                else
                {
                    EXPECT_FALSE(fs::exists(m_directory / "sound"));
                }
                return device_accepts;
            }));

    EXPECT_TRUE(output.play({10, 20, 0}));
    EXPECT_EQ(!device_accepts, output.ready());
    if (device_accepts)
    {
        EXPECT_FALSE(output.finish());
        EXPECT_CALL(m_driver, sound_off());
        m_time += 1ms;
        output.update();
    }
    else
    {
        EXPECT_EQ(0, m_now_calls);
    }
    EXPECT_TRUE(output.finish());
    EXPECT_TRUE(output.finish());
    if (recording)
    {
        EXPECT_EQ("230\n", read_text(path));
    }
}

INSTANTIATE_TEST_SUITE_P(Modes, TestDriverSoundRecording, Combine(Bool(), Bool()));

TEST_F(TestDriverSoundOutput, readinessAndCompletionDoNotServiceExpiredPlayback)
{
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    ASSERT_TRUE(output.play({10, 20, 0}));
    const int now_calls{m_now_calls};
    m_time += 1ms;

    EXPECT_FALSE(output.ready());
    EXPECT_FALSE(output.ready());
    EXPECT_FALSE(output.finish());
    EXPECT_EQ(now_calls, m_now_calls);

    EXPECT_CALL(m_driver, sound_off()).Times(1);
    output.update();
    output.update();
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
    EXPECT_TRUE(output.finish());
    output.update();
}

TEST_F(TestDriverSoundOutput, rejectsPendingSamplesWithoutDuplicateRecordsOrBoundaries)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    output.begin_orbit();
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    ASSERT_TRUE(output.play({10, 20, 0}));

    EXPECT_FALSE(output.play({20, 30, 0}));
    EXPECT_FALSE(output.play({20, 30, 0}));
    EXPECT_EQ("time=1234\n230\n", read_text(m_directory / "sound/sound001.txt"));
    EXPECT_EQ(1, m_timestamp_calls);

    EXPECT_CALL(m_driver, sound_off());
    m_time += 1ms;
    output.update();
    EXPECT_CALL(m_driver, sound_on(240)).WillOnce(Return(false));
    EXPECT_TRUE(output.play({20, 30, 0}));
    EXPECT_TRUE(output.finish());
    EXPECT_EQ("time=1234\n230\n240\n", read_text(m_directory / "sound/sound001.txt"));
}

TEST_F(TestDriverSoundOutput, writesOrbitBoundariesWithoutChangingReadiness)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};

    output.begin_orbit();
    m_time_ms = 5678;
    output.begin_orbit();

    EXPECT_TRUE(output.ready());
    EXPECT_EQ(0, m_now_calls);
    EXPECT_EQ("time=1234\ntime=5678\n", read_text(m_directory / "sound/sound001.txt"));
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, zeroDurationStillReleasesThroughUpdate)
{
    g_orbit_delay = 0;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    ASSERT_TRUE(output.play({10, 20, 0}));
    EXPECT_FALSE(output.ready());
    EXPECT_CALL(m_driver, sound_off());

    output.update();

    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, muteClearsOldDeadlineAndRetainsRecordingForLaterPlayback)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    InSequence sequence;
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    EXPECT_CALL(m_driver, mute());
    EXPECT_CALL(m_driver, sound_on(240)).WillOnce(Return(true));
    EXPECT_CALL(m_driver, sound_off());
    ASSERT_TRUE(output.play({10, 20, 0}));
    m_time += 500us;

    output.mute();
    EXPECT_TRUE(output.ready());
    ASSERT_TRUE(output.play({20, 30, 0}));
    m_time += 500us;
    output.update();
    EXPECT_FALSE(output.ready());
    m_time += 500us;
    output.update();

    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
    EXPECT_EQ("230\n240\n", read_text(m_directory / "sound/sound001.txt"));
}

TEST_F(TestDriverSoundOutput, cancellationCanFinishWithoutDrainingMutedTone)
{
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    ASSERT_TRUE(output.play({10, 20, 0}));
    EXPECT_CALL(m_driver, mute());

    output.mute();

    EXPECT_TRUE(output.finish());
    EXPECT_TRUE(output.finish());
    m_time += 1ms;
    output.update();
}

TEST_F(TestDriverSoundOutput, destructionCancelsUnfinishedToneAndClosesRecording)
{
    g_orbit_save_flags = OSF_MIDI;
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(true));
    EXPECT_CALL(m_driver, mute());
    {
        DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
        EXPECT_TRUE(output.play({10, 20, 0}));
    }

    EXPECT_EQ("230\n", read_text(m_directory / "sound/sound001.txt"));
}

struct SoundOverwriteInput
{
    const char *name;
    bool overwrite;
    const char *existing_text;
    const char *first_text;
    const char *second_text;
};

void PrintTo(const SoundOverwriteInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSoundRecordingOverwrite : public TestDriverSoundOutput, public WithParamInterface<SoundOverwriteInput>
{
protected:
    void SetUp() override;
};

void TestSoundRecordingOverwrite::SetUp()
{
    TestDriverSoundOutput::SetUp();
    g_orbit_save_flags = OSF_MIDI;
    g_overwrite_file = GetParam().overwrite;
    fs::create_directory(m_directory / "sound");
    write_text(m_directory / "sound/sound001.txt", GetParam().existing_text);
}

TEST_P(TestSoundRecordingOverwrite, appliesOverwritePolicyToExistingRecording)
{
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));

    EXPECT_TRUE(output.play({10, 20, 0}));
    EXPECT_TRUE(output.finish());

    EXPECT_EQ(GetParam().first_text, read_text(m_directory / "sound/sound001.txt"));
    if (GetParam().second_text)
    {
        EXPECT_EQ(GetParam().second_text, read_text(m_directory / "sound/sound002.txt"));
    }
    else
    {
        EXPECT_FALSE(fs::exists(m_directory / "sound/sound002.txt"));
    }
}

INSTANTIATE_TEST_SUITE_P(Policies, TestSoundRecordingOverwrite,
    Values(SoundOverwriteInput{"PreserveExisting", false, "preserved\n", "preserved\n", "230\n"},
        SoundOverwriteInput{"OverwriteExisting", true, "replaced\n", "230\n", nullptr}),
    [](const TestParamInfo<SoundOverwriteInput> &info) { return info.param.name; });

struct DriverOrbitSoundInput
{
    const char *name;
    FractalType type;
};

void PrintTo(const DriverOrbitSoundInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDriverOrbitSound : public TestDriverSoundOutput, public WithParamInterface<DriverOrbitSoundInput>
{
protected:
    void SetUp() override;

    ValueSaver<FractalType> saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> saved_specific{g_cur_fractal_specific};
};

void TestDriverOrbitSound::SetUp()
{
    TestDriverSoundOutput::SetUp();
    g_fractal_type = GetParam().type;
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
}

TEST_P(TestDriverOrbitSound, renderSourceRecognizesSelected3dOrbitWithoutRendererPointers)
{
    EXPECT_CALL(m_driver, sound_on(320)).WillOnce(Return(false));
    EXPECT_EQ(nullptr, g_cur_fractal_specific->calc_type);
    DriverSoundOutput output{Source::RENDER, m_now, m_timestamp};

    EXPECT_TRUE(output.play({1, 2, 3}));
    EXPECT_TRUE(output.finish());
}

INSTANTIATE_TEST_SUITE_P(Orbits, TestDriverOrbitSound,
    Values(DriverOrbitSoundInput{"Kam3D", FractalType::KAM_3D},
        DriverOrbitSoundInput{"Lorenz3D", FractalType::LORENZ_3D},
        DriverOrbitSoundInput{"Rossler", FractalType::ROSSLER},
        DriverOrbitSoundInput{"Pickover", FractalType::PICKOVER},
        DriverOrbitSoundInput{"Lorenz3D1", FractalType::LORENZ_3D1},
        DriverOrbitSoundInput{"Lorenz3D3", FractalType::LORENZ_3D3},
        DriverOrbitSoundInput{"Lorenz3D4", FractalType::LORENZ_3D4},
        DriverOrbitSoundInput{"Icon3D", FractalType::ICON_3D}),
    [](const TestParamInfo<DriverOrbitSoundInput> &info) { return info.param.name; });

TEST_F(TestDriverSoundOutput, instancesKeepSeparateRecordingFiles)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput first{Source::OVERLAY, m_now, m_timestamp};
    DriverSoundOutput second{Source::OVERLAY, m_now, m_timestamp};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));
    EXPECT_CALL(m_driver, sound_on(240)).WillOnce(Return(false));
    EXPECT_CALL(m_driver, sound_on(250)).WillOnce(Return(false));

    EXPECT_TRUE(first.play({10, 20, 0}));
    EXPECT_TRUE(second.play({20, 30, 0}));
    EXPECT_TRUE(first.finish());
    EXPECT_TRUE(second.play({30, 40, 0}));
    EXPECT_TRUE(second.finish());

    EXPECT_EQ("230\n", read_text(m_directory / "sound/sound001.txt"));
    EXPECT_EQ("240\n250\n", read_text(m_directory / "sound/sound002.txt"));
}

TEST_F(TestDriverSoundOutput, reportsOpenFailureWithoutPlaybackAndCanRetry)
{
    g_orbit_save_flags = OSF_MIDI;
    g_overwrite_file = true;
    fs::create_directories(m_directory / "sound/sound001.txt");
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};

    EXPECT_THROW(output.play({10, 20, 0}), std::runtime_error);
    EXPECT_TRUE(output.ready());
    fs::remove(m_directory / "sound/sound001.txt");
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));
    EXPECT_TRUE(output.play({10, 20, 0}));
    EXPECT_TRUE(output.finish());

    EXPECT_EQ("230\n", read_text(m_directory / "sound/sound001.txt"));
}

TEST_F(TestDriverSoundOutput, reportsUnavailableSavePathWithoutPlaybackOrPrompt)
{
    g_orbit_save_flags = OSF_MIDI;
    write_text(m_directory / "blocked", "not a directory");
    set_save_library(m_directory / "blocked/child");
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};

    EXPECT_THROW(output.play({10, 20, 0}), std::runtime_error);
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, reportedWriteFailureDoesNotBecomeACloseFailure)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    output.begin_orbit();
    ASSERT_TRUE(output.m_file.is_open());
    output.m_file.setstate(std::ios::badbit);
    EXPECT_CALL(m_driver, sound_on(_)).Times(0);

    EXPECT_THAT([&output] { output.play({10, 20, 0}); }, ThrowsMessage<std::runtime_error>(HasSubstr("Can't write ")));

    EXPECT_NO_THROW(EXPECT_TRUE(output.finish()));
    EXPECT_FALSE(output.m_file.is_open());
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, finishedSessionCannotAcceptMoreOutput)
{
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    ASSERT_TRUE(output.finish());

    EXPECT_FALSE(output.ready());
    EXPECT_THROW(output.play({10, 20, 0}), std::logic_error);
    EXPECT_THROW(output.begin_orbit(), std::logic_error);
    EXPECT_THROW(output.mute(), std::logic_error);
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, configurationIsCapturedForTheSession)
{
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};
    g_base_hertz = 880;
    g_sound_flag = SOUNDFLAG_Y;
    g_orbit_save_flags = OSF_MIDI;
    g_debug_flag = DebugFlags::FORCE_SCALED_SOUND_FORMULA;
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));

    EXPECT_TRUE(output.play({10, 20, 0}));
    EXPECT_TRUE(output.finish());
    EXPECT_FALSE(fs::exists(m_directory / "sound"));
}

TEST_F(TestDriverSoundOutput, inactiveSoundModeDoesNotPlayOrRecordSamples)
{
    g_sound_flag = SOUNDFLAG_BEEP;
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::OVERLAY, m_now, m_timestamp};

    EXPECT_TRUE(output.play({10, 20, 0}));
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
    EXPECT_FALSE(fs::exists(m_directory / "sound"));
}

TEST_F(TestDriverSoundOutput, defaultClocksDoNotDelayUnsupportedDevice)
{
    DriverSoundOutput output{Source::ORBIT};
    EXPECT_CALL(m_driver, sound_on(320)).WillOnce(Return(false));

    EXPECT_TRUE(output.play({1, 2, 3}));
    EXPECT_TRUE(output.ready());
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, rejectsInvalidMappingBeforeOutput)
{
    g_orbit_save_flags = OSF_MIDI;
    DriverSoundOutput output{Source::ORBIT, m_now, m_timestamp};

    EXPECT_THROW(output.play({std::numeric_limits<double>::infinity(), 0, 0}), std::logic_error);
    EXPECT_FALSE(fs::exists(m_directory / "sound"));
    EXPECT_TRUE(output.finish());
}

TEST_F(TestDriverSoundOutput, rejectsNegativeTiming)
{
    g_orbit_delay = -1;

    EXPECT_THROW((DriverSoundOutput{Source::OVERLAY, m_now, m_timestamp}), std::logic_error);
}

TEST_F(TestDriverSoundOutput, rejectsScaledOverlayWithZeroWidth)
{
    g_orbit_delay = 0;
    g_debug_flag = DebugFlags::FORCE_SCALED_SOUND_FORMULA;
    g_logical_screen.x_dots = 0;

    EXPECT_THROW((DriverSoundOutput{Source::OVERLAY, m_now, m_timestamp}), std::logic_error);
}

TEST_F(TestDriverSoundOutput, rejectsRestoredImageWithZeroColors)
{
    g_colors = 0;

    EXPECT_THROW((DriverSoundOutput{Source::RESTORED_IMAGE, m_now, m_timestamp}), std::logic_error);
}

} // namespace id::test
