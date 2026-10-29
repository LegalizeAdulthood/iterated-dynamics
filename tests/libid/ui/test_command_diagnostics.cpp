// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <engine/Command.h>
#include <misc/ValueSaver.h>
#include <ui/command_diagnostics.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>
#include <stdexcept>

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

struct StartupWarningKeyInput
{
    const char *name;
    int key;
    bool proceed;
};

void PrintTo(const StartupWarningKeyInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCommandDiagnosticsWarning : public TestCommandDiagnostics, public WithParamInterface<StartupWarningKeyInput>
{
};

TEST_P(TestCommandDiagnosticsWarning, returnsAcknowledgementDecision)
{
    expect_startup_screen(GetParam().key);
    EXPECT_CALL(m_driver, put_string(1, 0, 7, StrEq("missing startup file")));

    EXPECT_EQ(GetParam().proceed, show_startup_command_diagnostics({{}, {"missing startup file"}}));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
}

INSTANTIATE_TEST_SUITE_P(Key, TestCommandDiagnosticsWarning,
    Values(StartupWarningKeyInput{"Enter", ID_KEY_ENTER, true}, StartupWarningKeyInput{"Escape", ID_KEY_ESC, false}),
    [](const TestParamInfo<StartupWarningKeyInput> &info) { return info.param.name; });

TEST_F(TestCommandDiagnostics, startupErrorUsesOriginalFatalDisplayWithoutPresentingOtherDiagnostics)
{
    CommandDiagnostics diagnostics;
    arg_error("bad1=1", &diagnostics);
    arg_error("bad2=1", &diagnostics);
    diagnostics.warnings.push_back("missing startup file");
    const InSequence sequence;
    EXPECT_CALL(m_driver, set_for_text());
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(0, 0, 15, StrEq("*** Error during startup:")));
    EXPECT_CALL(m_driver, put_string(2, 0, 15, StrEq(diagnostics.errors.front())));
    EXPECT_CALL(m_driver, move_cursor(8, 0))
        .WillOnce(Throw(std::runtime_error("Stop before the startup delay and process exit")));

    EXPECT_THROW(show_startup_command_diagnostics(diagnostics), std::runtime_error);
    EXPECT_EQ(BatchMode::NONE, g_init_batch);
}

} // namespace id::test
