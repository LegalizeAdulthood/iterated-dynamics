// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/CommandRouter.h>

#include <config/port_config.h>
#include <engine/calcfrac.h>
#include <engine/command_comments.h>
#include <engine/CommandProcessor.h>
#include <engine/sound.h>
#include <engine/VideoInfo.h>
#include <fractals/formula.h>
#include <fractals/fractalp.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/big_while_loop.h>
#include <ui/history.h>
#include <ui/id_keys.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
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

TEST_F(TestCommandRouter, defaultRouterDiagnosesUnknownNameOnce)
{
    CommandProcessor processor{"unknown=yes", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  unknown=yes"));
}

TEST_F(TestCommandRouter, defaultRouterDoesNotRepeatSyntaxDiagnostic)
{
    CommandProcessor processor{std::string(33, 'a') + "=yes", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(1U, result.diagnostics.errors.size());
}

TEST_F(TestCommandRouter, defaultRouterDoesNotRepeatRejectionDiagnostic)
{
    CommandProcessor processor{"batch=yes", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(BatchMode::NONE, g_init_batch);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  batch=yes"));
}

TEST_F(TestCommandRouter, askVideoYes)
{
    ValueSaver saved_ask_video{g_ask_video, false};
    CommandProcessor processor{"askvideo=y", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(g_ask_video);
}

TEST_F(TestCommandRouter, askVideoNo)
{
    ValueSaver saved_ask_video{g_ask_video, true};
    CommandProcessor processor{"askvideo=n", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_ask_video);
}

TEST_F(TestCommandRouter, invalidVideoPromptValuePreservesPreferenceAndDiagnosesOnce)
{
    ValueSaver saved_ask_video{g_ask_video};
    const CommandRouter router;
    for (const bool ask_video : {true, false})
    {
        SCOPED_TRACE(ask_video);
        g_ask_video = ask_video;
        CommandProcessor processor{"askvideo=maybe", CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult result{router.process(processor)};

        EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
        EXPECT_EQ(ask_video, g_ask_video);
        EXPECT_THAT(
            result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  askvideo=maybe"));
    }
}

TEST_F(TestCommandRouter, virtualNo)
{
    ValueSaver saved_virtual_screens{g_virtual_screens, true};
    CommandProcessor processor{"virtual=n", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_virtual_screens);
}

TEST_F(TestCommandRouter, virtualYes)
{
    ValueSaver saved_virtual_screens{g_virtual_screens, false};
    CommandProcessor processor{"virtual=y", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(g_virtual_screens);
}

TEST_F(TestCommandRouter, invalidVirtualScreenValuePreservesPreferenceAndDiagnosesOnce)
{
    ValueSaver saved_virtual_screens{g_virtual_screens};
    const CommandRouter router;
    for (const bool virtual_screens : {true, false})
    {
        SCOPED_TRACE(virtual_screens);
        g_virtual_screens = virtual_screens;
        CommandProcessor processor{"virtual=maybe", CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult result{router.process(processor)};

        EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
        EXPECT_EQ(virtual_screens, g_virtual_screens);
        EXPECT_THAT(
            result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  virtual=maybe"));
    }
}

TEST_F(TestCommandRouter, maxHistoryNonNumeric)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    CommandProcessor processor{"maxhistory=yes", CmdFile::SSTOOLS_INI};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(0, g_max_image_history);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=yes"));
}

TEST_F(TestCommandRouter, maxHistoryNegative)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    CommandProcessor processor{"maxhistory=-10", CmdFile::SSTOOLS_INI};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(0, g_max_image_history);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=-10"));
}

TEST_F(TestCommandRouter, maxHistory)
{
    ValueSaver saved_max_image_history{g_max_image_history, -1};
    const CommandRouter router;
    for (const int max_history : {0, 10})
    {
        SCOPED_TRACE(max_history);
        CommandProcessor processor{"maxhistory=" + std::to_string(max_history), CmdFile::SSTOOLS_INI};

        const CommandParseResult result{router.process(processor)};

        EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
        EXPECT_EQ(max_history, g_max_image_history);
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

TEST_F(TestCommandRouter, maxHistoryAfterStartup)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    CommandProcessor processor{"maxhistory=10", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(0, g_max_image_history);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=10"));
}

TEST_F(TestCommandRouter, maxHistoryDebugOverrideRetainsValueValidation)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
    CommandProcessor processor{"maxhistory=10", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(10, g_max_image_history);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    for (const std::string_view value : {"yes", "-10"})
    {
        SCOPED_TRACE(value);
        CommandProcessor invalid{"maxhistory=" + std::string{value}, CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult rejected{router.process(invalid)};

        EXPECT_EQ(CmdArgFlags::BAD_ARG, rejected.flags);
        EXPECT_EQ(10, g_max_image_history);
        EXPECT_THAT(rejected.diagnostics.errors,
            ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=" + std::string{value}));
    }
}

class TestCommandRouterFile : public TestCommandRouter
{
protected:
    void TearDown() override;
    std::FILE *open_input(std::string_view text);

    fs::path m_path;
    ValueSaver<double> m_saved_param_0{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_2{g_params[2], 0.0};
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
    ValueSaver<Version> m_saved_file_version{g_file_version, current_id_version()};
    ValueSaver<bool> m_saved_bif_functions{g_new_bifurcation_functions_loaded, true};
};

void TestCommandRouterFile::TearDown()
{
    if (!m_path.empty())
    {
        std::error_code error;
        fs::remove(m_path, error);
    }
    clear_command_comments();
}

std::FILE *TestCommandRouterFile::open_input(const std::string_view text)
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    m_path = fs::temp_directory_path() / (std::string{info->test_suite_name()} + '_' + info->name() + ".par");
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{std::fopen(m_path.string().c_str(), "w+b"), std::fclose};
    if (!file || std::fwrite(text.data(), 1, text.size(), file.get()) != text.size())
    {
        throw std::runtime_error("Cannot write parameter test file: " + m_path.string());
    }
    std::rewind(file.get());
    return file.release();
}

TEST_F(TestCommandRouterFile, selectedSetUsesPositionAndContextForBothOwners)
{
    const std::string prefix{"ignored { ui=no }\n"};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{
        open_input(prefix + "entry (description) { ui=yes engine=yes } ui=no"), std::fclose};
    ASSERT_EQ(0, std::fseek(file.get(), static_cast<long>(prefix.size()), SEEK_SET));
    CommandProcessor processor{file.release(), CmdFile::AT_AFTER_STARTUP};
    const InSequence sequence;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, cmd.mode);
                EXPECT_EQ("yes", cmd.value);
                cmd.result.diagnostics.warnings.push_back("UI warning");
                return CmdArgFlags::RESET;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, cmd.mode);
                EXPECT_EQ("yes", cmd.value);
                cmd.result.diagnostics.warnings.push_back("engine warning");
                return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(
        CmdArgFlags::RESET | CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_THAT(result.diagnostics.warnings, ElementsAre("UI warning", "engine warning"));
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, uiRejectionRetainsEngineEffectsAndFlagsAndStopsTheSet)
{
    CommandProcessor processor{open_input("entry { engine=yes ui=bad engine=no }"), CmdFile::AT_AFTER_STARTUP};
    const InSequence sequence;
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                g_max_iterations = 222;
                return CmdArgFlags::RESET | CmdArgFlags::FRACTAL_PARAM;
            });
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::RESET | CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  ui=bad"));
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, engineRejectionRetainsUiEffectsAndFlagsAndStopsTheSet)
{
    CommandProcessor processor{open_input("entry { ui=yes engine=bad ui=no }"), CmdFile::AT_AFTER_STARTUP};
    const InSequence sequence;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                g_base_hertz = 550;
                return CmdArgFlags::YES_3D | CmdArgFlags::PARAM_3D;
            });
    EXPECT_CALL(m_engine_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(550, g_base_hertz);
    EXPECT_EQ(CmdArgFlags::YES_3D | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  engine=bad"));
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, uiFlagsTriggerFileCompatibilityOnceEvenAfterRejection)
{
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
    CommandProcessor processor{open_input("entry { ui=yes engine=bad }"), CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce(Return(CmdArgFlags::FRACTAL_PARAM));
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                EXPECT_DOUBLE_EQ(1.0, g_params[2]);
                return cmd.bad_arg();
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_EQ(1U, result.diagnostics.errors.size());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, actionStopsTheSetAndRetainsBothOwnersResults)
{
    CommandProcessor processor{open_input("entry { ui=yes engine=yes ui=no }"), CmdFile::AT_AFTER_STARTUP};
    const InSequence sequence;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                cmd.result.initialize_fm = true;
                return CmdArgFlags::RESET;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                cmd.result.action = MakeHelpDocument{"output.txt"};
                return CmdArgFlags::NONE;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(CmdArgFlags::RESET, result.flags);
    EXPECT_TRUE(result.initialize_fm);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    ASSERT_TRUE(std::holds_alternative<MakeHelpDocument>(result.action));
    EXPECT_EQ("output.txt", std::get<MakeHelpDocument>(result.action).filename);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, defaultRouterRetainsSelectedSetInitializationAndLaterEffects)
{
    const CommandRouter router;
    CommandProcessor initial{"params=77", CmdFile::AT_CMD_LINE};
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, router.process(initial).flags);
    CommandProcessor processor{open_input("entry { type=mandel maxiter=222 hertz=550 }"), CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_DOUBLE_EQ(0.0, g_params[0]);
    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(550, g_base_hertz);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, selectedSetRoutesVideoPromptParameter)
{
    ValueSaver saved_ask_video{g_ask_video, true};
    CommandProcessor processor{open_input("entry { askvideo=no }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_ask_video);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, selectedSetRoutesVirtualScreenParameterAndCompletesCompatibility)
{
    ValueSaver saved_virtual_screens{g_virtual_screens, true};
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
    CommandProcessor processor{open_input("entry { virtual=no }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_virtual_screens);
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandRouterFile, maxHistoryUsesLiveOverrideAndRetainsAcceptedFlagsAtCompletion)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
    const std::string text{
        "entry { debugflag=" + std::to_string(static_cast<int>(DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)) +
        " maxhistory=10 debugflag=0 maxhistory=20 }"};
    CommandProcessor processor{open_input(text), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(10, g_max_image_history);
    EXPECT_EQ(DebugFlags::NONE, g_debug_flag);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=20"));
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(fs::remove(m_path));
}

class MockCommandDirectories : public SpecialDirectories
{
public:
    MOCK_METHOD(fs::path, program_dir, (), (const, override));
    MOCK_METHOD(fs::path, documents_dir, (), (const, override));
};

class TestCommandRouterStartup : public TestCommandRouterFile
{
protected:
    void SetUp() override;
    void TearDown() override;
    void write_source(const std::string &name, std::string_view text);

    fs::path m_directory;
    fs::path m_library;
    std::vector<fs::path> m_sources;
    const std::shared_ptr<NiceMock<MockCommandDirectories>> m_directories{
        std::make_shared<NiceMock<MockCommandDirectories>>()};
    ValueSaver<std::shared_ptr<SpecialDirectories>> m_saved_directories{g_special_dirs, m_directories};
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
    ValueSaver<fs::path> m_saved_temp_dir{g_temp_dir};
    ValueSaver<int> m_saved_init_mode{g_init_mode};
    ValueSaver<ShowFile> m_saved_show_file{g_show_file};
    ValueSaver<bool> m_saved_colors_preloaded{g_colors_preloaded};
    ValueSaver<bool> m_saved_read_color{g_read_color};
    ValueSaver<bool> m_saved_ask_video{g_ask_video};
    ValueSaver<bool> m_saved_virtual_screens{g_virtual_screens};
};

void TestCommandRouterStartup::SetUp()
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    m_directory = fs::temp_directory_path() / (std::string{info->test_suite_name()} + '_' + info->name());
    m_library = m_directory / ID_PROGRAM_NAME;
    fs::create_directories(m_library);
    ON_CALL(*m_directories, program_dir()).WillByDefault(Return(m_library));
    ON_CALL(*m_directories, documents_dir()).WillByDefault(Return(m_directory));
    g_first_init = true;
    write_source("sstools.ini", "");
}

void TestCommandRouterStartup::TearDown()
{
    clear_read_library_path();
    clear_save_library();
    for (const fs::path &path : m_sources)
    {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
    std::error_code ignored;
    fs::remove(m_library, ignored);
    fs::remove(m_directory, ignored);
    TestCommandRouterFile::TearDown();
}

void TestCommandRouterStartup::write_source(const std::string &name, const std::string_view text)
{
    const fs::path path{m_library / name};
    m_sources.push_back(path);
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{std::fopen(path.string().c_str(), "wb"), std::fclose};
    if (!file || std::fwrite(text.data(), 1, text.size(), file.get()) != text.size())
    {
        throw std::runtime_error("Cannot write startup test file: " + path.string());
    }
}

TEST_F(TestCommandRouterStartup, routesConfigurationIndirectSetsAndArgumentsInOrder)
{
    write_source("sstools.ini", "ui=1 engine=2\n");
    write_source("whole.par", "entry { engine=3 ui=4 }");
    write_source("selected.par", "ignored { ui=999 }\nchosen { ui=5 engine=6 }");
    std::vector<std::string> commands;
    std::vector<CmdFile> modes;
    EXPECT_CALL(m_ui_handler, Call(_))
        .Times(4)
        .WillRepeatedly(
            [&](const Command &cmd)
            {
                commands.emplace_back("ui=" + std::string{cmd.value});
                modes.push_back(cmd.mode);
                return CmdArgFlags::PARAM_3D;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .Times(4)
        .WillRepeatedly(
            [&](const Command &cmd)
            {
                commands.emplace_back("engine=" + std::string{cmd.value});
                modes.push_back(cmd.mode);
                return CmdArgFlags::FRACTAL_PARAM;
            });

    const CommandParseResult result{m_router.process_startup({"id", "@whole", "@selected/chosen", "engine=7", "ui=8"})};

    EXPECT_THAT(commands, ElementsAre("ui=1", "engine=2", "engine=3", "ui=4", "ui=5", "engine=6", "engine=7", "ui=8"));
    EXPECT_THAT(modes,
        ElementsAre(CmdFile::SSTOOLS_INI, CmdFile::SSTOOLS_INI, CmdFile::AT_CMD_LINE_SET_NAME,
            CmdFile::AT_CMD_LINE_SET_NAME, CmdFile::AT_CMD_LINE_SET_NAME, CmdFile::AT_CMD_LINE_SET_NAME,
            CmdFile::AT_CMD_LINE, CmdFile::AT_CMD_LINE));
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(m_library / "selected.par", g_parameter_file);
    EXPECT_EQ("chosen", g_parameter_set_name);
    EXPECT_TRUE(fs::remove(m_library / "sstools.ini"));
    EXPECT_TRUE(fs::remove(m_library / "whole.par"));
    EXPECT_TRUE(fs::remove(m_library / "selected.par"));
}

TEST_F(TestCommandRouterStartup, configurationRejectionStopsOnlyConfiguration)
{
    write_source("sstools.ini", "engine=111 ui=bad engine=999\n");
    const InSequence sequence;
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                g_max_iterations = 111;
                return CmdArgFlags::FRACTAL_PARAM;
            });
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                EXPECT_EQ(111, g_max_iterations);
                return CmdArgFlags::NONE;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                g_max_iterations = 222;
                return CmdArgFlags::FRACTAL_PARAM;
            });

    const CommandParseResult result{m_router.process_startup({"id", "ui=yes", "engine=222"})};

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre(HasSubstr("ui=bad")));
    EXPECT_TRUE(fs::remove(m_library / "sstools.ini"));
}

TEST_F(TestCommandRouterStartup, eitherOwnerRejectionContinuesArgumentsButStopsIndirectFiles)
{
    std::vector<std::string> accepted;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillRepeatedly(
            [&](const Command &cmd)
            {
                if (cmd.value == "bad")
                {
                    return cmd.bad_arg();
                }
                accepted.emplace_back("ui=" + std::string{cmd.value});
                g_base_hertz = cmd.num_val;
                return CmdArgFlags::PARAM_3D;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillRepeatedly(
            [&](const Command &cmd)
            {
                if (cmd.value == "bad")
                {
                    return cmd.bad_arg();
                }
                accepted.emplace_back("engine=" + std::string{cmd.value});
                g_max_iterations = cmd.num_val;
                return CmdArgFlags::FRACTAL_PARAM;
            });
    for (const std::string owner : {"ui", "engine"})
    {
        SCOPED_TRACE(owner);
        accepted.clear();
        write_source("broken.par", "entry { engine=210 " + owner + "=bad ui=999 }");

        const CommandParseResult result{
            m_router.process_startup({"id", "ui=100", owner + "=bad", "engine=200", "@broken", "ui=300"})};

        EXPECT_THAT(accepted, ElementsAre("ui=100", "engine=200", "engine=210", "ui=300"));
        EXPECT_EQ(300, g_base_hertz);
        EXPECT_EQ(210, g_max_iterations);
        EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
        EXPECT_THAT(result.diagnostics.errors, ElementsAre(HasSubstr(owner + "=bad"), HasSubstr(owner + "=bad")));
        EXPECT_TRUE(fs::remove(m_library / "broken.par"));
    }
}

TEST_F(TestCommandRouterStartup, initializesBeforeConfigurationAndPreservesInitialRequests)
{
    write_source("sstools.ini", "ui=yes\n");
    g_temp_dir.clear();
    g_max_iterations = 999;
    g_init_mode = 9;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [&](const Command &)
            {
                EXPECT_TRUE(g_first_init);
                EXPECT_FALSE(g_temp_dir.empty());
                EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
                EXPECT_EQ(150, g_max_iterations);
                EXPECT_EQ(-1, g_init_mode);
                g_max_iterations = 321;
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                g_colors_preloaded = true;
                g_read_color = true;
                return CmdArgFlags::NONE;
            });

    const CommandParseResult result{m_router.process_startup({"id"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(321, g_max_iterations);
    EXPECT_EQ(7, g_init_mode);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
    EXPECT_FALSE(g_read_color);
    EXPECT_FALSE(g_first_init);
}

TEST_F(TestCommandRouterStartup, restartPreservesProcessStateAndSuppressesStartupRequests)
{
    write_source("sstools.ini", "ui=yes\n");
    const fs::path process_state{m_directory / "process-state"};
    g_first_init = false;
    g_temp_dir = process_state;
    g_max_iterations = 999;
    g_init_mode = 9;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [&](const Command &)
            {
                EXPECT_FALSE(g_first_init);
                EXPECT_EQ(process_state, g_temp_dir);
                EXPECT_EQ(150, g_max_iterations);
                EXPECT_EQ(-1, g_init_mode);
                g_max_iterations = 321;
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                g_colors_preloaded = true;
                return CmdArgFlags::NONE;
            });

    const CommandParseResult result{m_router.process_startup({"id"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(process_state, g_temp_dir);
    EXPECT_EQ(321, g_max_iterations);
    EXPECT_EQ(-1, g_init_mode);
    EXPECT_EQ(ShowFile::IMAGE_LOADED, g_show_file);
    EXPECT_TRUE(g_read_color);
    EXPECT_FALSE(g_first_init);
}

TEST_F(TestCommandRouterStartup, debugOverridePreservesFirstInitAndExistingRestartSuppression)
{
    EXPECT_CALL(m_ui_handler, Call(_))
        .Times(2)
        .WillRepeatedly(
            [](const Command &)
            {
                EXPECT_EQ(-1, g_init_mode);
                g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                return CmdArgFlags::NONE;
            });
    for (const bool first_init : {true, false})
    {
        SCOPED_TRACE(first_init);
        g_first_init = first_init;
        g_init_mode = 9;

        const CommandParseResult result{m_router.process_startup({"id", "ui=yes"})};

        EXPECT_TRUE(result.diagnostics.errors.empty());
        EXPECT_EQ(first_init, g_first_init);
        EXPECT_EQ(first_init ? 7 : -1, g_init_mode);
        EXPECT_EQ(first_init ? ShowFile::LOAD_IMAGE : ShowFile::IMAGE_LOADED, g_show_file);
    }
}

TEST_F(TestCommandRouterStartup, fallbackReappliesOnlyDefaultsAndConfiguration)
{
    write_source("sstools.ini", "type=formula maxiter=321 ui=yes\n");
    const CommandRouter router{CommandHandlerTables{{}, m_ui}, engine_command_handlers()};
    EXPECT_CALL(m_ui_handler, Call(_))
        .Times(2)
        .WillRepeatedly(
            [](const Command &)
            {
                EXPECT_EQ(-1, g_init_mode);
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                g_colors_preloaded = true;
                g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
                return CmdArgFlags::NONE;
            });
    const CommandParseResult startup{router.process_startup({"id", "maxiter=999"})};
    ASSERT_TRUE(startup.needs_file_entry);
    ASSERT_EQ(999, g_max_iterations);
    const fs::path process_state{m_directory / "process-state"};
    g_temp_dir = process_state;
    g_first_init = false;
    g_init_mode = 9;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(result.needs_file_entry);
    EXPECT_TRUE(g_formula_name.empty());
    EXPECT_EQ(321, g_max_iterations);
    EXPECT_EQ(process_state, g_temp_dir);
    EXPECT_FALSE(g_first_init);
    EXPECT_EQ(DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, g_debug_flag);
    EXPECT_EQ(7, g_init_mode);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
    EXPECT_FALSE(g_read_color);
    EXPECT_TRUE(fs::remove(m_library / "sstools.ini"));
}

TEST_F(TestCommandRouterStartup, finalTypeRequestsSelectionOrReportsBatchDiagnostic)
{
    const CommandRouter router;

    const CommandParseResult interactive{router.process_startup({"id", "type=formula"})};
    const CommandParseResult batch{router.process_startup({"id", "type=formula", "batch=yes"})};

    EXPECT_TRUE(interactive.diagnostics.errors.empty());
    EXPECT_TRUE(interactive.needs_file_entry);
    EXPECT_FALSE(batch.needs_file_entry);
    EXPECT_THAT(batch.diagnostics.errors, ElementsAre(HasSubstr("type=formula requires formulaname=")));
}

TEST_F(TestCommandRouterStartup, startupSourcesRouteHistoryCapacityInOrder)
{
    ValueSaver saved_max_image_history{g_max_image_history, 0};
    write_source("sstools.ini", "maxhistory=2\n");
    write_source("whole.par", "entry { maxhistory=4 }");
    write_source("selected.par", "ignored { maxhistory=99 }\nchosen { maxhistory=6 }");
    const CommandRouter router;

    const CommandParseResult configuration{router.process_startup({"id"})};

    EXPECT_TRUE(configuration.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, configuration.flags);
    EXPECT_EQ(2, g_max_image_history);

    const CommandParseResult whole{router.process_startup({"id", "@whole"})};

    EXPECT_TRUE(whole.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, whole.flags);
    EXPECT_EQ(4, g_max_image_history);

    const CommandParseResult selected{router.process_startup({"id", "@whole", "@selected/chosen"})};

    EXPECT_TRUE(selected.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, selected.flags);
    EXPECT_EQ(6, g_max_image_history);

    const CommandParseResult argument{router.process_startup({"id", "@whole", "@selected/chosen", "maxhistory=8"})};

    EXPECT_TRUE(argument.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, argument.flags);
    EXPECT_EQ(8, g_max_image_history);
}

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeaveVideoPromptPreferenceAlone)
{
    g_ask_video = false;

    engine::reset_restart_defaults();

    EXPECT_FALSE(g_ask_video);
}

TEST_F(TestCommandRouterStartup, videoPromptDefaultPrecedesConfigurationAndLaterOverrides)
{
    g_ask_video = false;
    write_source("sstools.ini", "askvideo=no\n");
    const CommandRouter router;

    const CommandParseResult configuration{router.process_startup({"id"})};

    EXPECT_TRUE(configuration.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, configuration.flags);
    EXPECT_FALSE(g_ask_video);

    const CommandParseResult result{router.process_startup({"id", "askvideo=yes"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(g_ask_video);
}

TEST_F(TestCommandRouterStartup, indirectFilesRouteVideoPromptParameter)
{
    write_source("whole.par", "entry { askvideo=no }");
    write_source("selected.par", "ignored { askvideo=no }\nchosen { askvideo=yes }");
    const CommandRouter router;

    const CommandParseResult whole{router.process_startup({"id", "@whole"})};

    EXPECT_TRUE(whole.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, whole.flags);
    EXPECT_FALSE(g_ask_video);

    const CommandParseResult selected{router.process_startup({"id", "@whole", "@selected/chosen"})};

    EXPECT_TRUE(selected.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, selected.flags);
    EXPECT_TRUE(g_ask_video);
}

TEST_F(TestCommandRouterStartup, startupRestartAndFallbackRestoreVideoPromptDefault)
{
    const CommandRouter router;
    g_ask_video = false;

    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    EXPECT_TRUE(g_ask_video);
    EXPECT_FALSE(g_first_init);
    g_ask_video = false;
    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    EXPECT_TRUE(g_ask_video);
    g_ask_video = false;
    EXPECT_TRUE(router.reset_startup().diagnostics.errors.empty());
    EXPECT_TRUE(g_ask_video);
}

TEST_F(TestCommandRouterStartup, fallbackRetainsVideoPromptConfigurationOverride)
{
    const CommandRouter router;
    write_source("sstools.ini", "askvideo=no\n");
    g_ask_video = true;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_FALSE(g_ask_video);
}

TEST_F(TestCommandRouterStartup, explicitResetPreservesVideoPromptPreference)
{
    const CommandRouter router;
    for (const bool ask_video : {true, false})
    {
        SCOPED_TRACE(ask_video);
        g_ask_video = ask_video;
        CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult result{router.process(processor)};

        EXPECT_TRUE(result.diagnostics.errors.empty());
        EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
        EXPECT_EQ(ask_video, g_ask_video);
    }
}

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeaveVirtualScreenPreferenceAlone)
{
    g_virtual_screens = false;

    engine::reset_restart_defaults();

    EXPECT_FALSE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, virtualScreenDefaultPrecedesConfigurationAndLaterOverrides)
{
    g_virtual_screens = false;
    write_source("sstools.ini", "virtual=no\n");
    const CommandRouter router;

    const CommandParseResult configuration{router.process_startup({"id"})};

    EXPECT_TRUE(configuration.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, configuration.flags);
    EXPECT_FALSE(g_virtual_screens);

    const CommandParseResult result{router.process_startup({"id", "virtual=yes"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, indirectFilesRouteVirtualScreenParameter)
{
    write_source("whole.par", "entry { virtual=no }");
    write_source("selected.par", "ignored { virtual=no }\nchosen { virtual=yes }");
    const CommandRouter router;

    const CommandParseResult whole{router.process_startup({"id", "@whole"})};

    EXPECT_TRUE(whole.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, whole.flags);
    EXPECT_FALSE(g_virtual_screens);

    const CommandParseResult selected{router.process_startup({"id", "@whole", "@selected/chosen"})};

    EXPECT_TRUE(selected.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, selected.flags);
    EXPECT_TRUE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, startupRestartAndFallbackRestoreVirtualScreenDefault)
{
    const CommandRouter router;
    g_virtual_screens = false;

    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    EXPECT_TRUE(g_virtual_screens);
    EXPECT_FALSE(g_first_init);
    g_virtual_screens = false;
    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    EXPECT_TRUE(g_virtual_screens);
    g_virtual_screens = false;
    EXPECT_TRUE(router.reset_startup().diagnostics.errors.empty());
    EXPECT_TRUE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, fallbackRetainsVirtualScreenConfigurationOverride)
{
    const CommandRouter router;
    write_source("sstools.ini", "virtual=no\n");
    g_virtual_screens = true;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_FALSE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, explicitResetPreservesVirtualScreenPreference)
{
    const CommandRouter router;
    for (const bool virtual_screens : {true, false})
    {
        SCOPED_TRACE(virtual_screens);
        g_virtual_screens = virtual_screens;
        CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult result{router.process(processor)};

        EXPECT_TRUE(result.diagnostics.errors.empty());
        EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
        EXPECT_EQ(virtual_screens, g_virtual_screens);
    }
}

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeavePendingModeAlone)
{
    g_init_mode = 7;

    engine::reset_restart_defaults();

    EXPECT_EQ(7, g_init_mode);
}

TEST_F(TestCommandRouterStartup, pendingModeDefaultsAndOverridesDoNotActivateVideoMode)
{
    const VideoInfo active{800, 600, 256, nullptr, "active"};
    ValueSaver saved_adapter{g_adapter, 0};
    ValueSaver saved_entry{g_video_entry, active};
    ValueSaver saved_width{g_screen_x_dots, 800};
    ValueSaver saved_height{g_screen_y_dots, 600};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_table_length{g_video_table_len, 2};
    ValueSaver saved_first_key{g_video_mode_keys[0], ID_KEY_F6};
    ValueSaver saved_second_key{g_video_mode_keys[1], ID_KEY_F5};
    g_init_mode = 9;
    write_source("sstools.ini", "ui=before video=F6\n");
    const CommandRouter router{CommandHandlerTables{{}, m_ui}, engine_command_handlers()};
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &cmd)
            {
                EXPECT_EQ(CmdFile::SSTOOLS_INI, cmd.mode);
                EXPECT_EQ(-1, g_init_mode);
                return CmdArgFlags::NONE;
            })
        .WillOnce(
            [](const Command &cmd)
            {
                EXPECT_EQ(CmdFile::AT_CMD_LINE, cmd.mode);
                EXPECT_EQ(0, g_init_mode);
                return CmdArgFlags::NONE;
            });

    const CommandParseResult result{router.process_startup({"id", "ui=after", "video=F5"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(1, g_init_mode);
    EXPECT_EQ(0, g_adapter);
    EXPECT_EQ(active, g_video_entry);
    EXPECT_EQ(800, g_screen_x_dots);
    EXPECT_EQ(600, g_screen_y_dots);
    EXPECT_EQ(256, g_colors);
}

TEST_F(TestCommandRouterStartup, explicitResetPreservesPendingMode)
{
    const CommandRouter router;
    for (const int mode : {-1, 7})
    {
        SCOPED_TRACE(mode);
        g_init_mode = mode;
        CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

        const CommandParseResult result{router.process(processor)};

        EXPECT_TRUE(result.diagnostics.errors.empty());
        EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
        EXPECT_EQ(mode, g_init_mode);
    }
}

} // namespace id::test
