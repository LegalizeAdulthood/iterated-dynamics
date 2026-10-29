// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <engine/sound.h>
#include <misc/ValueSaver.h>
#include <ui/command_effects.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestCommandEffectsNoDriver : public Test
{
protected:
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
};

TEST_F(TestCommandEffectsNoDriver, noActionDoesNotRequestExit)
{
    CommandParseResult result;

    EXPECT_FALSE(apply_command_effects(result));
}

TEST_F(TestCommandEffectsNoDriver, pendingErrorsPreventGenerationAndDeviceInitialization)
{
    CommandParseResult result;
    result.action = MakeHelpDocument{"must-not-be-written.txt"};
    result.initialize_fm = true;
    result.diagnostics.errors.push_back("invalid earlier argument");

    EXPECT_FALSE(apply_command_effects(result));
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("invalid earlier argument"));
}

class TestCommandEffectsFm : public TestWithParam<bool>
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM};
};

TEST_P(TestCommandEffectsFm, updatesRequestedDeviceBit)
{
    EXPECT_CALL(m_driver, init_fm()).WillOnce(Return(GetParam()));
    CommandParseResult result;
    result.initialize_fm = true;

    EXPECT_FALSE(apply_command_effects(result));

    EXPECT_EQ(GetParam() ? SOUNDFLAG_X | SOUNDFLAG_OPL3_FM : SOUNDFLAG_X, g_sound_flag);
}

INSTANTIATE_TEST_SUITE_P(Initialization, TestCommandEffectsFm, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "Succeeds" : "Fails"; });

} // namespace id::test
