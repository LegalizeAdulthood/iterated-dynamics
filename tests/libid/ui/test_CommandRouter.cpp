// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/CommandRouter.h>

#include <engine/calcfrac.h>
#include <engine/CommandProcessor.h>
#include <engine/sound.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestCommandRouter : public Test
{
protected:
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 150L};
    ValueSaver<int> m_saved_hertz{g_base_hertz, 440};
    StrictMock<MockFunction<CmdArgFlags(const Command &)>> m_ui_handler;
    StrictMock<MockFunction<CmdArgFlags(const Command &)>> m_engine_handler;
    const std::vector<CommandHandler> m_ui{CommandHandler{"ui", m_ui_handler.AsStdFunction()}};
    const std::vector<CommandHandler> m_engine{CommandHandler{"engine", m_engine_handler.AsStdFunction()}};
    const CommandRouter m_router{CommandHandlerTables{{}, m_ui}, CommandHandlerTables{{}, m_engine}};
};

TEST_F(TestCommandRouter, acceptsUiWithoutFlagsOrDelegation)
{
    CommandProcessor processor{"ui=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [&](const Command &cmd)
            {
                EXPECT_EQ(&processor.command(), &cmd);
                return CmdArgFlags::NONE;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, preservesUiFlagsAndAuxiliaryResults)
{
    CommandProcessor processor{"ui=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                cmd.result.initialize_fm = true;
                cmd.result.diagnostics.warnings.push_back("nonfatal warning");
                cmd.result.action = MakeHelpDocument{"output.txt"};
                return CmdArgFlags::RESET | CmdArgFlags::FRACTAL_PARAM;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::RESET | CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.initialize_fm);
    EXPECT_THAT(result.diagnostics.warnings, ElementsAre("nonfatal warning"));
    EXPECT_TRUE(std::holds_alternative<MakeHelpDocument>(result.action));
}

TEST_F(TestCommandRouter, delegatesUnknownUiNameToEngineWithTheSameCommand)
{
    CommandProcessor processor{"engine=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [&](const Command &cmd)
            {
                EXPECT_EQ(&processor.command(), &cmd);
                EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, cmd.mode);
                return CmdArgFlags::FRACTAL_PARAM;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, rejectedUiNameDoesNotDelegateOrRepeatDiagnostic)
{
    CommandProcessor processor{"ui=bad", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  ui=bad"));
}

TEST_F(TestCommandRouter, rejectedEngineNameIsNotDiagnosedAgain)
{
    CommandProcessor processor{"engine=bad", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_engine_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  engine=bad"));
}

TEST_F(TestCommandRouter, unknownNameIsDiagnosedOnce)
{
    CommandProcessor processor{"unknown=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  unknown=yes"));
}

TEST_F(TestCommandRouter, syntaxFailureDoesNotInvokeEitherOwner)
{
    CommandProcessor processor{std::string(33, 'a') + "=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(1U, result.diagnostics.errors.size());
}

TEST_F(TestCommandRouter, restrictedUiNameDoesNotDelegateOrInvokeHandler)
{
    const CommandRouter router{CommandHandlerTables{m_ui, {}}, CommandHandlerTables{{}, m_engine}};
    CommandProcessor processor{"ui=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  ui=yes"));
}

TEST_F(TestCommandRouter, overridePermitsRestrictedUiName)
{
    g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
    const CommandRouter router{CommandHandlerTables{m_ui, {}}, CommandHandlerTables{{}, m_engine}};
    CommandProcessor processor{"ui=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce(Return(CmdArgFlags::NONE));

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, startupContextPermitsRestrictedUiName)
{
    const CommandRouter router{CommandHandlerTables{m_ui, {}}, CommandHandlerTables{{}, m_engine}};
    CommandProcessor processor{"ui=yes", CmdFile::AT_CMD_LINE};
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce(Return(CmdArgFlags::NONE));

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, duplicateNamesAreRejectedAcrossEveryTablePairBeforeAdvancement)
{
    CommandProcessor processor{"maxiter=222", CmdFile::AT_AFTER_STARTUP};
    for (std::size_t first = 0; first < 4; ++first)
    {
        for (std::size_t second = first + 1; second < 4; ++second)
        {
            SCOPED_TRACE(Message() << first << ", " << second);
            std::array<std::vector<CommandHandler>, 4> tables{};
            tables[first] = m_ui;
            tables[second] = m_ui;

            EXPECT_THROW(
                (CommandRouter{CommandHandlerTables{tables[0], tables[1]}, CommandHandlerTables{tables[2], tables[3]}}),
                std::logic_error);
        }
    }
    EXPECT_EQ(150, g_max_iterations);
    EXPECT_THROW(processor.command(), std::logic_error);
}

TEST_F(TestCommandRouter, duplicateNamesAreRejectedWithinEveryTable)
{
    const std::vector<CommandHandler> duplicates{m_ui[0], m_ui[0]};
    for (std::size_t group = 0; group < 4; ++group)
    {
        SCOPED_TRACE(group);
        std::array<std::vector<CommandHandler>, 4> tables{};
        tables[group] = duplicates;

        EXPECT_THROW(
            (CommandRouter{CommandHandlerTables{tables[0], tables[1]}, CommandHandlerTables{tables[2], tables[3]}}),
            std::logic_error);
    }
}

TEST_F(TestCommandRouter, unrelatedHandlerExceptionsPropagate)
{
    CommandProcessor processor{"ui=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce([](const Command &) -> CmdArgFlags { throw std::runtime_error("handler failure"); });

    EXPECT_THROW(m_router.process(processor), std::runtime_error);

    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, defaultRouterUsesActualEngineOwnershipTables)
{
    const CommandRouter router;
    CommandProcessor processor{"maxiter=222", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, defaultRouterRetainsAcceptanceWithoutChangeFlags)
{
    const CommandRouter router;
    CommandProcessor processor{"hertz=660", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(660, g_base_hertz);
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

TEST_F(TestCommandRouter, defaultRouterRetainsEngineStartupRestrictions)
{
    const CommandRouter router;
    CommandProcessor processor{"batch=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(1U, result.diagnostics.errors.size());
}

} // namespace id::test
