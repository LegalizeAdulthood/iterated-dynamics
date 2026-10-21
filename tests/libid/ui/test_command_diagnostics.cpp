// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <misc/ValueSaver.h>
#include <ui/command_diagnostics.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestCommandDiagnostics : public Test
{
protected:
    void expect_startup_screen(int key);
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<bool> m_saved_first{g_first_init, true};
};

void TestCommandDiagnostics::expect_startup_screen(const int key)
{
    EXPECT_CALL(m_driver, set_for_text());
    EXPECT_CALL(m_driver, put_string(_, 0, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, move_cursor(_, 0));
    {
        InSequence keys;
        EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(key));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(key));
    }
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
}

TEST_F(TestCommandDiagnostics, cleanStartupNeedsNoPresentation)
{
    EXPECT_TRUE(show_startup_command_diagnostics({}));
}

TEST_F(TestCommandDiagnostics, warningCanBeAcknowledgedAfterParsing)
{
    expect_startup_screen(ID_KEY_ENTER);
    EXPECT_CALL(m_driver, put_string(1, 0, 7, StrEq("missing startup file")));

    EXPECT_TRUE(show_startup_command_diagnostics({{}, {"missing startup file"}}));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
}

TEST_F(TestCommandDiagnostics, escapeFromStartupWarningReturnsAbortDecision)
{
    expect_startup_screen(ID_KEY_ESC);

    EXPECT_FALSE(show_startup_command_diagnostics({{}, {"missing startup file"}}));
}

TEST_F(TestCommandDiagnostics, startupErrorReturnsExitDecisionWithoutExiting)
{
    expect_startup_screen(ID_KEY_ENTER);
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(1, 0, 7, StrEq("invalid argument")));

    EXPECT_FALSE(show_startup_command_diagnostics({{"invalid argument"}, {}}));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
}

} // namespace id::test
