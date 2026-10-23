// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/CommandHandler.h>

#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <string>

using namespace id::engine;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestCommandHandler : public Test
{
protected:
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    CommandParseResult m_result;
    StrictMock<MockFunction<CmdArgFlags(const Command &)>> m_handler;
    const std::array<CommandHandler, 1> m_handlers{CommandHandler{"known", m_handler.AsStdFunction()}};
};

TEST_F(TestCommandHandler, unknownNameDoesNotInvokeHandlerOrDiagnose)
{
    const Command cmd{"unknown=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_command(m_handlers, cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
    EXPECT_TRUE(m_result.diagnostics.warnings.empty());
}

TEST_F(TestCommandHandler, emptyTableDoesNotRecognizeName)
{
    const std::array<CommandHandler, 0> handlers{};
    const Command cmd{"known=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_command(handlers, cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommandHandler, acceptanceWithoutFlagsIsRecognized)
{
    const Command cmd{"KNOWN=yes", CmdFile::AT_AFTER_STARTUP, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce(Return(CmdArgFlags::NONE));

    EXPECT_EQ(CmdArgFlags::NONE, handle_command(m_handlers, cmd));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommandHandler, acceptancePreservesChangeFlags)
{
    const Command cmd{"known=yes", CmdFile::AT_AFTER_STARTUP, m_result};
    const CmdArgFlags flags{CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce(Return(flags));

    EXPECT_EQ(flags, handle_command(m_handlers, cmd));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommandHandler, rejectedValueIsRecognizedAndDiagnosedOnce)
{
    const Command cmd{"known=garbage", CmdFile::AT_AFTER_STARTUP, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce([](const Command &command) { return command.bad_arg(); });

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_command(m_handlers, cmd));

    EXPECT_THAT(m_result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  known=garbage"));
}

TEST_F(TestCommandHandler, syntaxErrorDoesNotInvokeHandlerOrRepeatDiagnostic)
{
    const Command cmd{std::string(33, 'a') + "=yes", CmdFile::AT_AFTER_STARTUP, m_result};
    ASSERT_EQ(CmdArgFlags::BAD_ARG, cmd.status);
    ASSERT_EQ(1U, m_result.diagnostics.errors.size());

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_command(m_handlers, cmd));

    EXPECT_EQ(1U, m_result.diagnostics.errors.size());
}

TEST_F(TestCommandHandler, restrictedNameIsRejectedWithoutInvokingHandler)
{
    const Command cmd{"known=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_command(m_handlers, cmd, true));

    EXPECT_THAT(m_result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  known=yes"));
}

TEST_F(TestCommandHandler, unknownNameIsNotRejectedByStartupRestriction)
{
    const Command cmd{"unknown=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_command(m_handlers, cmd, true).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommandHandler, overrideAllowsRestrictedHandler)
{
    g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
    const Command cmd{"known=yes", CmdFile::AT_AFTER_STARTUP, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce(Return(CmdArgFlags::NONE));

    EXPECT_EQ(CmdArgFlags::NONE, handle_command(m_handlers, cmd, true));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestCommandHandler, overrideDoesNotBypassValueValidation)
{
    g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
    const Command cmd{"known=garbage", CmdFile::AT_AFTER_STARTUP, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce([](const Command &command) { return command.bad_arg(); });

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_command(m_handlers, cmd, true));

    EXPECT_EQ(1U, m_result.diagnostics.errors.size());
}

class TestStartupCommandHandler : public TestCommandHandler, public WithParamInterface<CmdFile>
{
};

TEST_P(TestStartupCommandHandler, invokesHandlerDuringStartup)
{
    const Command cmd{"known=yes", GetParam(), m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce(Return(CmdArgFlags::NONE));

    EXPECT_EQ(CmdArgFlags::NONE, handle_command(m_handlers, cmd, true));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

INSTANTIATE_TEST_SUITE_P(StartupSources, TestStartupCommandHandler,
    Values(CmdFile::AT_CMD_LINE, CmdFile::SSTOOLS_INI, CmdFile::AT_CMD_LINE_SET_NAME));

class TestEngineCommand : public Test
{
protected:
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    CommandParseResult m_result;
};

TEST_F(TestEngineCommand, unknownNameIsUndiagnosed)
{
    const Command cmd{"unknown=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnVideoPromptParameter)
{
    const Command cmd{"askvideo=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnVirtualScreenParameter)
{
    const Command cmd{"virtual=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnHistoryCapacityParameter)
{
    const Command cmd{"maxhistory=10", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, recognizesAcceptanceWithoutFlags)
{
    const Command cmd{"fpu=387", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::NONE, handle_engine_command(cmd));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, recognizesAcceptanceWithFlags)
{
    const Command cmd{"batch=yes", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, handle_engine_command(cmd));

    EXPECT_EQ(BatchMode::NORMAL, g_init_batch);
    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, malformedRecognizedValueIsRejectedOnce)
{
    const Command cmd{"batch=garbage", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_engine_command(cmd));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_THAT(m_result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  batch=garbage"));
}

TEST_F(TestEngineCommand, restrictedNameIsRecognizedWithoutChangingState)
{
    const Command cmd{"batch=yes", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_engine_command(cmd));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_THAT(m_result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  batch=yes"));
}

TEST_F(TestEngineCommand, evaluatesOverrideAfterEachParameter)
{
    const Command enable{"debugflag=" + std::to_string(static_cast<int>(DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)),
        CmdFile::AT_AFTER_STARTUP, m_result};
    const Command batch{"batch=yes", CmdFile::AT_AFTER_STARTUP, m_result};
    const Command disable{"debugflag=0", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_EQ(CmdArgFlags::NONE, handle_engine_command(enable));
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, handle_engine_command(batch));
    EXPECT_EQ(BatchMode::NORMAL, g_init_batch);
    EXPECT_EQ(CmdArgFlags::NONE, handle_engine_command(disable));
    g_init_batch = BatchMode::NONE;
    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_engine_command(batch));

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_EQ(1U, m_result.diagnostics.errors.size());
}

} // namespace id::test
