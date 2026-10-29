// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/SoundSession.h>

#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <engine/NullSoundOutput.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <io/check_write_file.h>
#include <io/library.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/KeyboardHandler.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace fs = std::filesystem;

namespace id::test
{

namespace
{

class MockSoundOutput : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

class MockSoundKeyboardHandler : public KeyboardHandler
{
public:
    MOCK_METHOD(bool, handle_key, (int), (override));
};

using MockOutput = StrictMock<MockSoundOutput>;
using Source = DriverSoundOutput::Source;

std::string read_text(const fs::path &path)
{
    std::ifstream file{path};
    if (!file)
    {
        throw std::runtime_error("Can't read sound session test output");
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

} // namespace

class TestSoundSession : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound};
    ValueSaver<int> m_saved_sound_flag{g_sound_flag, SOUNDFLAG_OFF};
    ValueSaver<int> m_saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 10};
    ValueSaver<int> m_saved_base_hertz{g_base_hertz, 220};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    std::shared_ptr<MockOutput> m_output{std::make_shared<MockOutput>()};
    fs::path m_directory;
    bool m_directory_created{};
};

void TestSoundSession::SetUp()
{
    reset_calc_interrupted();
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string test_name{info->name()};
    std::replace(test_name.begin(), test_name.end(), '/', '-');
    m_directory = fs::temp_directory_path() /
        ("id-sound-session-" + test_name + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    m_directory_created = fs::create_directory(m_directory);
    ASSERT_TRUE(m_directory_created);
    set_save_library(m_directory);
}

void TestSoundSession::TearDown()
{
    reset_calc_interrupted();
    clear_save_library();
    if (m_directory_created)
    {
        std::error_code ignored;
        fs::remove_all(m_directory, ignored);
    }
}

struct SoundSessionModeInput
{
    const char *name;
    int mode;
};

void PrintTo(const SoundSessionModeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestNullSoundSession : public TestSoundSession, public WithParamInterface<SoundSessionModeInput>
{
protected:
    void SetUp() override;
};

void TestNullSoundSession::SetUp()
{
    TestSoundSession::SetUp();
    g_sound_flag = GetParam().mode;
}

TEST_P(TestNullSoundSession, selectsNullOutputWithoutPlaybackOrRecording)
{
    SoundSession session{Source::OVERLAY};

    EXPECT_NE(nullptr, dynamic_cast<NullSoundOutput *>(g_sound.get()));
    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.finish());
    EXPECT_FALSE(fs::exists(m_directory / "sound"));
}

INSTANTIATE_TEST_SUITE_P(Modes, TestNullSoundSession,
    Values(SoundSessionModeInput{"Off", SOUNDFLAG_OFF}, SoundSessionModeInput{"Beep", SOUNDFLAG_BEEP}),
    [](const TestParamInfo<SoundSessionModeInput> &info) { return info.param.name; });

TEST_F(TestSoundSession, recordingRequestRetainsDriverAdapterWithoutPlayback)
{
    g_orbit_save_flags = OSF_MIDI;
    SoundSession session{Source::OVERLAY};
    EXPECT_NE(nullptr, dynamic_cast<DriverSoundOutput *>(g_sound.get()));

    g_sound->begin_orbit();

    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.finish());
    EXPECT_THAT(read_text(m_directory / "sound/sound001.txt"), StartsWith("time="));
}

TEST_F(TestSoundSession, unsupportedDeviceRecordsWithoutPlaybackWaiting)
{
    g_sound_flag = SOUNDFLAG_X;
    g_orbit_save_flags = OSF_MIDI;
    g_orbit_delay = 100000;
    SoundSession session{Source::OVERLAY};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));

    EXPECT_TRUE(g_sound->play({10, 20, 0}));
    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.finish());

    EXPECT_EQ("230\n", read_text(m_directory / "sound/sound001.txt"));
}

TEST_F(TestSoundSession, batchModeDoesNotEnableRecording)
{
    g_init_batch = BatchMode::NORMAL;
    g_sound_flag = SOUNDFLAG_X;
    SoundSession session{Source::OVERLAY};
    EXPECT_CALL(m_driver, sound_on(230)).WillOnce(Return(false));

    EXPECT_TRUE(g_sound->play({10, 20, 0}));
    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.finish());

    EXPECT_FALSE(fs::exists(m_directory / "sound"));
}

struct SoundProducerMappingInput
{
    const char *name;
    FractalType type;
    CalcMode mode;
    int frequency;
};

void PrintTo(const SoundProducerMappingInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSoundProducerMapping : public TestSoundSession, public WithParamInterface<SoundProducerMappingInput>
{
protected:
    void SetUp() override;

    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode};
};

void TestSoundProducerMapping::SetUp()
{
    TestSoundSession::SetUp();
    g_sound_flag = SOUNDFLAG_X;
    g_cur_fractal_specific = get_fractal_specific(GetParam().type);
    g_std_calc_mode = GetParam().mode;
    EXPECT_CALL(m_driver, sound_on(GetParam().frequency)).WillOnce(Return(false));
}

TEST_P(TestSoundProducerMapping, renderOwnerSelectsTheProducerMapping)
{
    SoundSession owner;

    EXPECT_TRUE(g_sound->play({10, 20, 30}));
    EXPECT_TRUE(owner.finish());
}

INSTANTIATE_TEST_SUITE_P(Producers, TestSoundProducerMapping,
    Values(SoundProducerMappingInput{"Mandel", FractalType::MANDEL, CalcMode::ONE_PASS, 230},
        SoundProducerMappingInput{"StickyMandel", FractalType::MANDEL, CalcMode::ORBIT, 220},
        SoundProducerMappingInput{"FrothyBasin", FractalType::FROTHY_BASIN, CalcMode::ORBIT, 230},
        SoundProducerMappingInput{"Henon", FractalType::HENON, CalcMode::ONE_PASS, 1220},
        SoundProducerMappingInput{"Lorenz3D", FractalType::LORENZ_3D, CalcMode::ONE_PASS, 1220},
        SoundProducerMappingInput{"Dynamic", FractalType::DYNAMIC, CalcMode::ONE_PASS, 1220}),
    [](const TestParamInfo<SoundProducerMappingInput> &info) { return info.param.name; });

TEST_F(TestSoundSession, renderMappingUsesTheModeAfterImageSetupAndThenRemainsFixed)
{
    ValueSaver saved_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver saved_mode{g_std_calc_mode, CalcMode::ORBIT};
    g_sound_flag = SOUNDFLAG_X;
    SoundSession owner;
    EXPECT_CALL(m_driver, sound_on(230)).Times(2).WillRepeatedly(Return(false));

    // Image setup can force one-pass traversal, for example for true color.
    g_std_calc_mode = CalcMode::ONE_PASS;
    EXPECT_TRUE(g_sound->play({10, 20, 30}));
    g_std_calc_mode = CalcMode::ORBIT;
    EXPECT_TRUE(g_sound->play({10, 20, 30}));
    EXPECT_TRUE(owner.finish());
}

TEST_F(TestSoundSession, retainsAndRestoresTheSharedBinding)
{
    const SoundOutputPtr previous{g_sound};
    const std::weak_ptr<SoundOutput> lifetime{m_output};
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    {
        SoundSession session{m_output};
        EXPECT_EQ(m_output, g_sound);
        m_output.reset();
        EXPECT_FALSE(lifetime.expired());
        EXPECT_TRUE(session.finish());
    }

    EXPECT_EQ(previous, g_sound);
    EXPECT_TRUE(lifetime.expired());
}

TEST_F(TestSoundSession, nestedControllerUsesTheInstalledOutputWithoutTakingItsLifecycle)
{
    EXPECT_CALL(*m_output, play(FieldsAre(1.0, 2.0, 3.0))).WillOnce(Return(true));
    EXPECT_CALL(*m_output, update()).Times(2);
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(true));
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    []
    {
        SoundSession nested{g_sound, SoundSession::Ownership::BORROWED};
        EXPECT_TRUE(g_sound->play({1, 2, 3}));
        EXPECT_TRUE(nested.finish());
        EXPECT_TRUE(nested.finish());
    }();

    EXPECT_EQ(m_output, g_sound);
    EXPECT_TRUE(session.finish());
}

TEST_F(TestSoundSession, borrowedCancellationMutesBeforeTheOwnerCloses)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession owner{m_output};
    {
        SoundSession nested{g_sound, SoundSession::Ownership::BORROWED};
        nested.cancel();
        EXPECT_FALSE(nested.finish());
    }
    EXPECT_EQ(m_output, g_sound);
    owner.cancel();
}

TEST_F(TestSoundSession, producerFailureClosesTheOwnedOutputDuringUnwinding)
{
    const SoundOutputPtr previous{g_sound};
    InSequence sequence;
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("close failed")));

    try
    {
        SoundSession owner{m_output};
        SoundSession nested{g_sound, SoundSession::Ownership::BORROWED};
        throw std::logic_error("calculation failed");
    }
    catch (const std::logic_error &error)
    {
        EXPECT_STREQ("calculation failed", error.what());
    }

    EXPECT_EQ(previous, g_sound);
}

TEST_F(TestSoundSession, servicesReadyOutputWithoutPollingInputOrDelaying)
{
    EXPECT_CALL(*m_output, update()).Times(3);
    EXPECT_CALL(*m_output, ready()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(session.finish());
}

TEST_F(TestSoundSession, backpressureServicesOutputUntilTheNextIncrementCanAdvance)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(true));
    EXPECT_CALL(*m_output, play(FieldsAre(4.0, 5.0, 6.0))).WillOnce(Return(true));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    ASSERT_TRUE(session.wait_until_ready());
    EXPECT_TRUE(g_sound->play({4, 5, 6}));
    EXPECT_TRUE(session.finish());
}

TEST_F(TestSoundSession, completionDrainsReleaseTailsWithoutNewSamplesOrMuting)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_TRUE(session.finish());
    EXPECT_TRUE(session.finish());
}

TEST_F(TestSoundSession, atomicOutputWaitsAndFinishesWithoutPollingOrCancelling)
{
    set_calc_interrupted();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(*m_output, mute()).Times(0);
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(true));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_TRUE(session.wait_until_ready(SoundSession::InputPolicy::ATOMIC));
    EXPECT_TRUE(session.finish(SoundSession::InputPolicy::ATOMIC));
    EXPECT_TRUE(calc_interrupt_requested());
}

class TestInterruptedSoundSession : public TestSoundSession
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<ScopedKeyboardHandler> m_keyboard_scope;
};

void TestInterruptedSoundSession::SetUp()
{
    TestSoundSession::SetUp();
    m_keyboard_scope = std::make_unique<ScopedKeyboardHandler>(std::make_shared<MainLoopKeyboardHandler>());
}

void TestInterruptedSoundSession::TearDown()
{
    m_keyboard_scope.reset();
    TestSoundSession::TearDown();
}

TEST_F(TestInterruptedSoundSession, interruptionWhileWaitingUsesTheExistingKeyboardHandler)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_FALSE(session.wait_until_ready());
    EXPECT_TRUE(calc_interrupt_requested());
    EXPECT_FALSE(session.finish());
    EXPECT_FALSE(session.wait_until_ready());
    session.cancel();
}

TEST_F(TestSoundSession, handledKeyCanLeavePlaybackWaitingWithoutInterrupting)
{
    const std::shared_ptr<StrictMock<MockSoundKeyboardHandler>> handler{
        std::make_shared<StrictMock<MockSoundKeyboardHandler>>()};
    ScopedKeyboardHandler keyboard_scope{handler};
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('o'));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('o'));
    EXPECT_CALL(*handler, handle_key('o')).WillOnce(Return(true));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(true));
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_TRUE(session.wait_until_ready());
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_TRUE(session.finish());
}

TEST_F(TestInterruptedSoundSession, interruptionDuringCompletionMutesBeforeFinalizing)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, unget_key('x'));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_FALSE(session.finish());
    EXPECT_TRUE(calc_interrupt_requested());
}

TEST_F(TestSoundSession, previouslyRequestedInterruptionDoesNotStartMorePlayback)
{
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};
    set_calc_interrupted();

    EXPECT_FALSE(session.wait_until_ready());
    EXPECT_FALSE(session.finish());
}

TEST_F(TestSoundSession, interruptionRequestedDuringDelayPreventsAnotherUpdate)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, ready()).WillOnce(Return(false));
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(m_driver, delay(1)).WillOnce(Invoke(set_calc_interrupted));
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};

    EXPECT_FALSE(session.wait_until_ready());
}

TEST_F(TestSoundSession, abandonedOwnerMutesClosesAndRestoresTheBinding)
{
    const SoundOutputPtr previous{g_sound};
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    {
        SoundSession session{m_output};
    }

    EXPECT_EQ(previous, g_sound);
}

TEST_F(TestSoundSession, finalizationErrorReachesTheOwnerWithoutMutingAClosedPort)
{
    const SoundOutputPtr previous{g_sound};
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("close failed")));
    {
        SoundSession session{m_output};

        EXPECT_THROW(session.finish(), std::runtime_error);
    }

    EXPECT_EQ(previous, g_sound);
}

TEST_F(TestSoundSession, cancellationErrorReachesTheOwnerWithoutRepeatingMute)
{
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("close failed")));
    SoundSession session{m_output};

    EXPECT_THROW(session.cancel(), std::runtime_error);
    session.cancel();
}

TEST_F(TestSoundSession, cancellationRequiresThePortToDiscardPendingPlayback)
{
    EXPECT_CALL(*m_output, mute());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(false));
    SoundSession session{m_output};

    EXPECT_THROW(session.cancel(), std::logic_error);
}

TEST_F(TestSoundSession, muteFailureStillClosesOutputAndPreservesTheFirstError)
{
    InSequence sequence;
    EXPECT_CALL(*m_output, mute()).WillOnce(Throw(std::runtime_error("mute failed")));
    EXPECT_CALL(*m_output, finish()).WillOnce(Throw(std::runtime_error("close failed")));
    SoundSession session{m_output};

    EXPECT_THAT([&session] { session.cancel(); }, ThrowsMessage<std::runtime_error>(StrEq("mute failed")));

    EXPECT_NO_THROW(session.cancel());
    EXPECT_FALSE(session.finish());
}

TEST_F(TestSoundSession, finishedSessionCannotResumeCalculation)
{
    EXPECT_CALL(*m_output, update());
    EXPECT_CALL(*m_output, finish()).WillOnce(Return(true));
    SoundSession session{m_output};
    ASSERT_TRUE(session.finish());

    EXPECT_THROW(session.wait_until_ready(), std::logic_error);
    session.cancel();
    EXPECT_TRUE(session.finish());
}

} // namespace id::test
