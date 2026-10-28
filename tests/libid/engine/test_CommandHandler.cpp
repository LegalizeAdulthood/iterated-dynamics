// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/CommandHandler.h>

#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <ostream>
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

struct CommandLookupInput
{
    const char *name;
    const char *argument;
    bool empty_table;
    bool restricted;
};

void PrintTo(const CommandLookupInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCommandLookup : public TestCommandHandler, public WithParamInterface<CommandLookupInput>
{
};

TEST_P(TestCommandLookup, unrecognizedNameDoesNotInvokeHandlerOrDiagnose)
{
    const CommandLookupInput &input{GetParam()};
    const Command cmd{input.argument, CmdFile::AT_AFTER_STARTUP, m_result};
    const std::array<CommandHandler, 0> empty_handlers{};

    EXPECT_FALSE((input.empty_table ? handle_command(empty_handlers, cmd, input.restricted)
                                    : handle_command(m_handlers, cmd, input.restricted))
            .has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
    EXPECT_TRUE(m_result.diagnostics.warnings.empty());
}

INSTANTIATE_TEST_SUITE_P(Lookups, TestCommandLookup,
    Values(CommandLookupInput{"Unknown", "unknown=yes", false, false},
        CommandLookupInput{"EmptyTable", "known=yes", true, false},
        CommandLookupInput{"RestrictedUnknown", "unknown=yes", false, true}),
    [](const TestParamInfo<CommandLookupInput> &info) { return info.param.name; });

struct AcceptedCommandInput
{
    const char *name;
    const char *argument;
    CmdFile source;
    bool restricted;
    DebugFlags debug;
    CmdArgFlags flags;
};

void PrintTo(const AcceptedCommandInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAcceptedCommandHandler : public TestCommandHandler, public WithParamInterface<AcceptedCommandInput>
{
protected:
    void SetUp() override;
};

void TestAcceptedCommandHandler::SetUp()
{
    g_debug_flag = GetParam().debug;
}

TEST_P(TestAcceptedCommandHandler, acceptancePreservesResultFlags)
{
    const AcceptedCommandInput &input{GetParam()};
    const Command cmd{input.argument, input.source, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce(Return(input.flags));

    EXPECT_EQ(input.flags, handle_command(m_handlers, cmd, input.restricted));

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

INSTANTIATE_TEST_SUITE_P(Acceptance, TestAcceptedCommandHandler,
    Values(AcceptedCommandInput{"NoFlags", "KNOWN=yes", CmdFile::AT_AFTER_STARTUP, false, DebugFlags::NONE,
               CmdArgFlags::NONE},
        AcceptedCommandInput{"ChangeFlags", "known=yes", CmdFile::AT_AFTER_STARTUP, false, DebugFlags::NONE,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        AcceptedCommandInput{"Override", "known=yes", CmdFile::AT_AFTER_STARTUP, true,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::NONE},
        AcceptedCommandInput{
            "CommandLine", "known=yes", CmdFile::AT_CMD_LINE, true, DebugFlags::NONE, CmdArgFlags::NONE},
        AcceptedCommandInput{
            "Configuration", "known=yes", CmdFile::SSTOOLS_INI, true, DebugFlags::NONE, CmdArgFlags::NONE},
        AcceptedCommandInput{
            "CommandLineSet", "known=yes", CmdFile::AT_CMD_LINE_SET_NAME, true, DebugFlags::NONE, CmdArgFlags::NONE}),
    [](const TestParamInfo<AcceptedCommandInput> &info) { return info.param.name; });

struct RejectedCommandInput
{
    const char *name;
    bool restricted;
    DebugFlags debug;
};

void PrintTo(const RejectedCommandInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRejectedCommandHandler : public TestCommandHandler, public WithParamInterface<RejectedCommandInput>
{
protected:
    void SetUp() override;
};

void TestRejectedCommandHandler::SetUp()
{
    g_debug_flag = GetParam().debug;
}

TEST_P(TestRejectedCommandHandler, rejectionDiagnosesTheValueOnce)
{
    const Command cmd{"known=garbage", CmdFile::AT_AFTER_STARTUP, m_result};
    EXPECT_CALL(m_handler, Call(Ref(cmd))).WillOnce([](const Command &command) { return command.bad_arg(); });

    EXPECT_EQ(CmdArgFlags::BAD_ARG, handle_command(m_handlers, cmd, GetParam().restricted));

    EXPECT_THAT(m_result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  known=garbage"));
}

INSTANTIATE_TEST_SUITE_P(Rejection, TestRejectedCommandHandler,
    Values(RejectedCommandInput{"Unrestricted", false, DebugFlags::NONE},
        RejectedCommandInput{"Override", true, DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME}),
    [](const TestParamInfo<RejectedCommandInput> &info) { return info.param.name; });

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

TEST_F(TestEngineCommand, doesNotOwnAutokeyModeParameter)
{
    const Command cmd{"autokey=play", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnAutokeyFilenameParameter)
{
    const Command cmd{"autokeyname=demo.key", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnVideoSelectionParameter)
{
    const Command cmd{"video=F6", CmdFile::AT_CMD_LINE, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnAutomaticStereoParameter)
{
    const Command cmd{"rds=random/100/top", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnStereoTextureParameter)
{
    const Command cmd{"rds-texture=map.gif", CmdFile::AT_AFTER_STARTUP, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnExitConfirmationParameter)
{
    const Command cmd{"exitnoask=yes", CmdFile::SSTOOLS_INI, m_result};

    EXPECT_FALSE(handle_engine_command(cmd).has_value());

    EXPECT_TRUE(m_result.diagnostics.errors.empty());
}

TEST_F(TestEngineCommand, doesNotOwnResetParameter)
{
    const Command cmd{"reset=1960", CmdFile::AT_AFTER_STARTUP, m_result};

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
