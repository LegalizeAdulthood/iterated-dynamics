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

TEST(TestCommandEffects, noActionDoesNotRequestExit)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    CommandParseResult result;

    EXPECT_FALSE(apply_command_effects(result));
}

TEST(TestCommandEffects, pendingErrorsPreventGenerationAndDeviceInitialization)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    CommandParseResult result;
    result.action = MakeHelpDocument{"must-not-be-written.txt"};
    result.initialize_fm = true;
    result.diagnostics.errors.push_back("invalid earlier argument");

    EXPECT_FALSE(apply_command_effects(result));
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("invalid earlier argument"));
}

TEST(TestCommandEffects, failedFmInitializationClearsRequestedDeviceBit)
{
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    ValueSaver saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM};
    EXPECT_CALL(driver, init_fm()).WillOnce(Return(false));
    CommandParseResult result;
    result.initialize_fm = true;

    EXPECT_FALSE(apply_command_effects(result));

    EXPECT_EQ(SOUNDFLAG_X, g_sound_flag);
}

TEST(TestCommandEffects, successfulFmInitializationPreservesRequestedDeviceBit)
{
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    ValueSaver saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM};
    EXPECT_CALL(driver, init_fm()).WillOnce(Return(true));
    CommandParseResult result;
    result.initialize_fm = true;

    EXPECT_FALSE(apply_command_effects(result));

    EXPECT_EQ(SOUNDFLAG_X | SOUNDFLAG_OPL3_FM, g_sound_flag);
}

} // namespace id::test
