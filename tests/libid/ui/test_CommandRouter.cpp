// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/CommandRouter.h>

#include <CommandTestFile.h>
#include <CommandParameterTest.h>
#include <EnvVarSaver.h>
#include <MockSpecialDirectories.h>
#include <TemporaryDirectory.h>
#include <test_data.h>

#include <config/port_config.h>
#include <engine/calcfrac.h>
#include <engine/command_comments.h>
#include <engine/CommandProcessor.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/formula.h>
#include <fractals/fractalp.h>
#include <geometry/stereo_params.h>
#include <io/CurrentPathSaver.h>
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
#include <ui/slideshw.h>
#include <ui/stereo.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

using id::io::test::MockSpecialDirectories;

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

using namespace id::test::data;

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

struct RouterOwnerInput
{
    const char *name;
    bool ui;
    CmdArgFlags flags;
};

void PrintTo(const RouterOwnerInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAcceptedRouterOwner : public TestCommandRouter, public WithParamInterface<RouterOwnerInput>
{
};

TEST_P(TestAcceptedRouterOwner, preservesTheCommandAndReturnsOwnerFlags)
{
    CommandProcessor processor{GetParam().ui ? "ui=yes" : "engine=yes", CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL((GetParam().ui ? m_ui_handler : m_engine_handler), Call(_))
        .WillOnce(
            [this, &processor](const Command &cmd)
            {
                EXPECT_EQ(&processor.command(), &cmd);
                if (!GetParam().ui)
                {
                    EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, cmd.mode);
                }
                return GetParam().flags;
            });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

INSTANTIATE_TEST_SUITE_P(Owners, TestAcceptedRouterOwner,
    Values(
        RouterOwnerInput{"Ui", true, CmdArgFlags::NONE}, RouterOwnerInput{"Engine", false, CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<RouterOwnerInput> &info) { return info.param.name; });

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

class TestRejectedRouterOwner : public TestCommandRouter, public WithParamInterface<RouterOwnerInput>
{
};

TEST_P(TestRejectedRouterOwner, rejectionProducesOnlyTheOwnerDiagnostic)
{
    const std::string argument{GetParam().ui ? "ui=bad" : "engine=bad"};
    CommandProcessor processor{argument, CmdFile::AT_AFTER_STARTUP};
    EXPECT_CALL((GetParam().ui ? m_ui_handler : m_engine_handler), Call(_))
        .WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  " + argument));
}

INSTANTIATE_TEST_SUITE_P(Owners, TestRejectedRouterOwner,
    Values(RouterOwnerInput{"Ui", true, CmdArgFlags::BAD_ARG}, RouterOwnerInput{"Engine", false, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<RouterOwnerInput> &info) { return info.param.name; });

class TestRouterDiagnostic : public TestCommandRouter, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    const CommandRouter &router() const;

    std::unique_ptr<CommandRouter> m_default_router;
};

void TestRouterDiagnostic::SetUp()
{
    TestCommandRouter::SetUp();
    if (GetParam())
    {
        m_default_router = std::make_unique<CommandRouter>();
    }
}

const CommandRouter &TestRouterDiagnostic::router() const
{
    return m_default_router ? *m_default_router : m_router;
}

TEST_P(TestRouterDiagnostic, unknownNameIsDiagnosedOnce)
{
    CommandProcessor processor{"unknown=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router().process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  unknown=yes"));
}

TEST_P(TestRouterDiagnostic, syntaxFailureIsDiagnosedOnceWithoutInvokingAnOwner)
{
    CommandProcessor processor{std::string(33, 'a') + "=yes", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router().process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(1U, result.diagnostics.errors.size());
}

INSTANTIATE_TEST_SUITE_P(Routers, TestRouterDiagnostic, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "Default" : "Injected"; });

struct RestrictedRouterInput
{
    const char *name;
    CmdFile mode;
    DebugFlags debug;
    CmdArgFlags flags;
};

void PrintTo(const RestrictedRouterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRestrictedUiRouter : public TestCommandRouter, public WithParamInterface<RestrictedRouterInput>
{
protected:
    void SetUp() override;
};

void TestRestrictedUiRouter::SetUp()
{
    TestCommandRouter::SetUp();
    g_debug_flag = GetParam().debug;
    if (GetParam().flags == CmdArgFlags::NONE)
    {
        EXPECT_CALL(m_ui_handler, Call(_)).WillOnce(Return(CmdArgFlags::NONE));
    }
    else
    {
        EXPECT_CALL(m_ui_handler, Call(_)).Times(0);
    }
}

TEST_P(TestRestrictedUiRouter, enforcesTheCommandContext)
{
    const CommandRouter router{CommandHandlerTables{m_ui, {}}, CommandHandlerTables{{}, m_engine}};
    CommandProcessor processor{"ui=yes", GetParam().mode};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    if (GetParam().flags == CmdArgFlags::BAD_ARG)
    {
        EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  ui=yes"));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Contexts, TestRestrictedUiRouter,
    Values(RestrictedRouterInput{"AfterStartup", CmdFile::AT_AFTER_STARTUP, DebugFlags::NONE, CmdArgFlags::BAD_ARG},
        RestrictedRouterInput{
            "DebugOverride", CmdFile::AT_AFTER_STARTUP, DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::NONE},
        RestrictedRouterInput{"Startup", CmdFile::AT_CMD_LINE, DebugFlags::NONE, CmdArgFlags::NONE}),
    [](const TestParamInfo<RestrictedRouterInput> &info) { return info.param.name; });

struct RouterTablePairInput
{
    const char *name;
    std::size_t first;
    std::size_t second;
};

void PrintTo(const RouterTablePairInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDuplicateRouterTablePair : public TestCommandRouter, public WithParamInterface<RouterTablePairInput>
{
};

TEST_P(TestDuplicateRouterTablePair, rejectsDuplicateNamesBeforeAdvancement)
{
    CommandProcessor processor{"maxiter=222", CmdFile::AT_AFTER_STARTUP};
    std::array<std::vector<CommandHandler>, 4> tables{};
    tables[GetParam().first] = m_ui;
    tables[GetParam().second] = m_ui;

    EXPECT_THROW(
        (CommandRouter{CommandHandlerTables{tables[0], tables[1]}, CommandHandlerTables{tables[2], tables[3]}}),
        std::logic_error);

    EXPECT_EQ(150, g_max_iterations);
    EXPECT_THROW(processor.command(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(TablePairs, TestDuplicateRouterTablePair,
    Values(RouterTablePairInput{"UiRestrictedAndUiGeneral", 0, 1},
        RouterTablePairInput{"UiRestrictedAndEngineRestricted", 0, 2},
        RouterTablePairInput{"UiRestrictedAndEngineGeneral", 0, 3},
        RouterTablePairInput{"UiGeneralAndEngineRestricted", 1, 2},
        RouterTablePairInput{"UiGeneralAndEngineGeneral", 1, 3},
        RouterTablePairInput{"EngineRestrictedAndEngineGeneral", 2, 3}),
    [](const TestParamInfo<RouterTablePairInput> &info) { return info.param.name; });

struct RouterTableInput
{
    const char *name;
    std::size_t group;
};

void PrintTo(const RouterTableInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDuplicateRouterTable : public TestCommandRouter, public WithParamInterface<RouterTableInput>
{
};

TEST_P(TestDuplicateRouterTable, rejectsDuplicateNamesWithinTheTable)
{
    const std::vector<CommandHandler> duplicates{m_ui[0], m_ui[0]};
    std::array<std::vector<CommandHandler>, 4> tables{};
    tables[GetParam().group] = duplicates;

    EXPECT_THROW(
        (CommandRouter{CommandHandlerTables{tables[0], tables[1]}, CommandHandlerTables{tables[2], tables[3]}}),
        std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Tables, TestDuplicateRouterTable,
    Values(RouterTableInput{"UiRestricted", 0}, RouterTableInput{"UiGeneral", 1},
        RouterTableInput{"EngineRestricted", 2}, RouterTableInput{"EngineGeneral", 3}),
    [](const TestParamInfo<RouterTableInput> &info) { return info.param.name; });

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
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  batch=yes"));
}

struct AskVideoInput
{
    const char *name;
    const char *value;
    bool initial;
    bool expected;
    CmdArgFlags flags;
};

void PrintTo(const AskVideoInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterAskVideo : public TestCommandRouter, public WithParamInterface<AskVideoInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_ask_video{g_ask_video};
};

void TestRouterAskVideo::SetUp()
{
    TestCommandRouter::SetUp();
    g_ask_video = GetParam().initial;
}

TEST_P(TestRouterAskVideo, processesVideoPromptPreference)
{
    CommandProcessor processor{"askvideo=" + std::string{GetParam().value}, CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    EXPECT_EQ(GetParam().expected, g_ask_video);
    if (GetParam().flags == CmdArgFlags::BAD_ARG)
    {
        EXPECT_THAT(
            result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  askvideo=maybe"));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Values, TestRouterAskVideo,
    Values(AskVideoInput{"Yes", "y", false, true, CmdArgFlags::NONE},
        AskVideoInput{"No", "n", true, false, CmdArgFlags::NONE},
        AskVideoInput{"InvalidWhenEnabled", "maybe", true, true, CmdArgFlags::BAD_ARG},
        AskVideoInput{"InvalidWhenDisabled", "maybe", false, false, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<AskVideoInput> &info) { return info.param.name; });

TEST_F(TestCommandRouter, videoBadNamePreservesPendingMode)
{
    ValueSaver saved_init_mode{g_init_mode, 7};
    CommandProcessor processor{"video=fmeh", CmdFile::AT_CMD_LINE};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(7, g_init_mode);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  video=fmeh"));
}

class TestUnavailableRouterVideo : public TestCommandRouter, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_table_length{g_video_table_len};
    ValueSaver<int> m_saved_key{g_video_mode_keys[0]};
    ValueSaver<int> m_saved_init_mode{g_init_mode, 0};
};

void TestUnavailableRouterVideo::SetUp()
{
    TestCommandRouter::SetUp();
    g_video_table_len = GetParam() ? 1 : 0;
    if (GetParam())
    {
        g_video_mode_keys[0] = ID_KEY_F2;
    }
}

TEST_P(TestUnavailableRouterVideo, unavailableBindingClearsPendingMode)
{
    CommandProcessor processor{"video=F1", CmdFile::AT_CMD_LINE};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(-1, g_init_mode);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  video=f1"));
}

INSTANTIATE_TEST_SUITE_P(Bindings, TestUnavailableRouterVideo, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "NonmatchingTable" : "EmptyTable"; });

struct RouterVideoContextInput
{
    const char *name;
    CmdFile mode;
    int initial_mode;
};

void PrintTo(const RouterVideoContextInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterVideoSelection : public TestCommandRouter, public WithParamInterface<RouterVideoContextInput>
{
protected:
    void SetUp() override;

    const VideoInfo m_active{800, 600, 256, nullptr, "active"};
    ValueSaver<int> m_saved_adapter{g_adapter, 0};
    ValueSaver<VideoInfo> m_saved_entry{g_video_entry, m_active};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 800};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 600};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_table_length{g_video_table_len, 3};
    ValueSaver<int> m_saved_first_key{g_video_mode_keys[0], ID_KEY_F2};
    ValueSaver<int> m_saved_second_key{g_video_mode_keys[1], ID_KEY_F1};
    ValueSaver<int> m_saved_third_key{g_video_mode_keys[2], ID_KEY_F1};
    ValueSaver<int> m_saved_init_mode{g_init_mode};
};

void TestRouterVideoSelection::SetUp()
{
    TestCommandRouter::SetUp();
    g_init_mode = GetParam().initial_mode;
}

TEST_P(TestRouterVideoSelection, selectsTheFirstBindingWithoutActivatingIt)
{
    const CommandRouter router;
    CommandProcessor processor{"video=F1", GetParam().mode};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(1, g_init_mode);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(0, g_adapter);
    EXPECT_EQ(m_active, g_video_entry);
    EXPECT_EQ(800, g_screen_x_dots);
    EXPECT_EQ(600, g_screen_y_dots);
    EXPECT_EQ(256, g_colors);
}

INSTANTIATE_TEST_SUITE_P(Contexts, TestRouterVideoSelection,
    Values(RouterVideoContextInput{"CommandLine", CmdFile::AT_CMD_LINE, 0},
        RouterVideoContextInput{"AfterStartup", CmdFile::AT_AFTER_STARTUP, 1}),
    [](const TestParamInfo<RouterVideoContextInput> &info) { return info.param.name; });

struct VirtualScreenInput
{
    const char *name;
    const char *value;
    bool initial;
    bool expected;
    CmdArgFlags flags;
};

void PrintTo(const VirtualScreenInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterVirtualScreen : public TestCommandRouter, public WithParamInterface<VirtualScreenInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_virtual_screens{g_virtual_screens};
};

void TestRouterVirtualScreen::SetUp()
{
    TestCommandRouter::SetUp();
    g_virtual_screens = GetParam().initial;
}

TEST_P(TestRouterVirtualScreen, processesTheVirtualScreenPreference)
{
    CommandProcessor processor{"virtual=" + std::string{GetParam().value}, CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    EXPECT_EQ(GetParam().expected, g_virtual_screens);
    if (GetParam().flags == CmdArgFlags::BAD_ARG)
    {
        EXPECT_THAT(
            result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  virtual=maybe"));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Preferences, TestRouterVirtualScreen,
    Values(VirtualScreenInput{"No", "n", true, false, CmdArgFlags::FRACTAL_PARAM},
        VirtualScreenInput{"Yes", "y", false, true, CmdArgFlags::FRACTAL_PARAM},
        VirtualScreenInput{"InvalidWhenEnabled", "maybe", true, true, CmdArgFlags::BAD_ARG},
        VirtualScreenInput{"InvalidWhenDisabled", "maybe", false, false, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<VirtualScreenInput> &info) { return info.param.name; });

struct ExitNoAskInput
{
    const char *name;
    const char *value;
    bool initial;
    bool expected;
    CmdFile mode;
    DebugFlags debug;
    CmdArgFlags flags;
};

void PrintTo(const ExitNoAskInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterExitNoAsk : public TestCommandRouter, public WithParamInterface<ExitNoAskInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_escape_exit{g_escape_exit};
};

void TestRouterExitNoAsk::SetUp()
{
    TestCommandRouter::SetUp();
    g_escape_exit = GetParam().initial;
    g_debug_flag = GetParam().debug;
}

TEST_P(TestRouterExitNoAsk, processesTheExitPreferenceInItsCommandContext)
{
    CommandProcessor processor{"exitnoask=" + std::string{GetParam().value}, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(GetParam().expected, g_escape_exit);
    EXPECT_EQ(GetParam().flags, result.flags);
    if (GetParam().flags == CmdArgFlags::BAD_ARG)
    {
        EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  exitnoask=!"));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Preferences, TestRouterExitNoAsk,
    Values(ExitNoAskInput{"No", "n", true, false, CmdFile::SSTOOLS_INI, DebugFlags::NONE,
               CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        ExitNoAskInput{"Yes", "y", false, true, CmdFile::SSTOOLS_INI, DebugFlags::NONE,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        ExitNoAskInput{
            "InvalidWhenDisabled", "!", false, false, CmdFile::SSTOOLS_INI, DebugFlags::NONE, CmdArgFlags::BAD_ARG},
        ExitNoAskInput{
            "InvalidWhenEnabled", "!", true, true, CmdFile::SSTOOLS_INI, DebugFlags::NONE, CmdArgFlags::BAD_ARG},
        ExitNoAskInput{"OverrideYes", "y", false, true, CmdFile::AT_AFTER_STARTUP,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        ExitNoAskInput{"OverrideInvalidWhenEnabled", "!", true, true, CmdFile::AT_AFTER_STARTUP,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<ExitNoAskInput> &info) { return info.param.name; });

TEST_F(TestCommandRouter, exitNoAskAfterStartupPreservesPreference)
{
    ValueSaver saved_escape_exit{g_escape_exit, true};
    CommandProcessor processor{"exitnoask=n", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_TRUE(g_escape_exit);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  exitnoask=n"));
}

struct RouterHistoryInput
{
    const char *name;
    const char *value;
    int initial;
    int expected;
    CmdFile mode;
    DebugFlags debug;
    CmdArgFlags flags;
};

void PrintTo(const RouterHistoryInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterHistoryCapacity : public TestCommandRouter, public WithParamInterface<RouterHistoryInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_capacity{g_max_image_history};
};

void TestRouterHistoryCapacity::SetUp()
{
    TestCommandRouter::SetUp();
    g_max_image_history = GetParam().initial;
    g_debug_flag = GetParam().debug;
}

TEST_P(TestRouterHistoryCapacity, processesTheHistoryCapacityInItsCommandContext)
{
    const std::string argument{"maxhistory=" + std::string{GetParam().value}};
    CommandProcessor processor{argument, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(GetParam().flags, result.flags);
    EXPECT_EQ(GetParam().expected, g_max_image_history);
    if (GetParam().flags == CmdArgFlags::BAD_ARG)
    {
        EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  " + argument));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Capacities, TestRouterHistoryCapacity,
    Values(RouterHistoryInput{"NonNumeric", "yes", 0, 0, CmdFile::SSTOOLS_INI, DebugFlags::NONE, CmdArgFlags::BAD_ARG},
        RouterHistoryInput{"Negative", "-10", 0, 0, CmdFile::SSTOOLS_INI, DebugFlags::NONE, CmdArgFlags::BAD_ARG},
        RouterHistoryInput{"Zero", "0", -1, 0, CmdFile::SSTOOLS_INI, DebugFlags::NONE,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        RouterHistoryInput{"Ten", "10", 0, 10, CmdFile::SSTOOLS_INI, DebugFlags::NONE,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        RouterHistoryInput{"OverrideTen", "10", 0, 10, CmdFile::AT_AFTER_STARTUP,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        RouterHistoryInput{"OverrideNonNumeric", "yes", 10, 10, CmdFile::AT_AFTER_STARTUP,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::BAD_ARG},
        RouterHistoryInput{"OverrideNegative", "-10", 10, 10, CmdFile::AT_AFTER_STARTUP,
            DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<RouterHistoryInput> &info) { return info.param.name; });

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

struct RouterAutoKeyInput
{
    const char *name;
    const char *value;
    CmdFile mode;
    SlidesMode initial;
    SlidesMode expected;
};

void PrintTo(const RouterAutoKeyInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAcceptedRouterAutoKey : public TestCommandRouter, public WithParamInterface<RouterAutoKeyInput>
{
protected:
    void SetUp() override;

    ValueSaver<SlidesMode> m_saved_slides{g_slides};
};

void TestAcceptedRouterAutoKey::SetUp()
{
    TestCommandRouter::SetUp();
    g_slides = GetParam().initial;
}

class TestRejectedRouterAutoKey : public TestAcceptedRouterAutoKey
{
};

TEST_P(TestRejectedRouterAutoKey, rejectsInvalidModesWithoutChangingSlides)
{
    const std::string argument{"autokey=" + std::string{GetParam().value}};
    CommandProcessor processor{argument, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(GetParam().expected, g_slides);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  " + argument));
}

INSTANTIATE_TEST_SUITE_P(Modes, TestRejectedRouterAutoKey,
    Values(RouterAutoKeyInput{"Unknown", "fmeh", CmdFile::AT_CMD_LINE, SlidesMode::RECORD, SlidesMode::RECORD},
        RouterAutoKeyInput{"Off", "off", CmdFile::AT_CMD_LINE, SlidesMode::RECORD, SlidesMode::RECORD}),
    [](const TestParamInfo<RouterAutoKeyInput> &info) { return info.param.name; });

TEST_P(TestAcceptedRouterAutoKey, selectsTheRequestedSlideMode)
{
    CommandProcessor processor{"autokey=" + std::string{GetParam().value}, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(GetParam().expected, g_slides);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

INSTANTIATE_TEST_SUITE_P(Modes, TestAcceptedRouterAutoKey,
    Values(RouterAutoKeyInput{"RecordAtStartup", "record", CmdFile::AT_CMD_LINE, SlidesMode::OFF, SlidesMode::RECORD},
        RouterAutoKeyInput{
            "RecordAfterStartup", "record", CmdFile::AT_AFTER_STARTUP, SlidesMode::OFF, SlidesMode::RECORD},
        RouterAutoKeyInput{"PlayAtStartup", "play", CmdFile::AT_CMD_LINE, SlidesMode::OFF, SlidesMode::PLAY},
        RouterAutoKeyInput{"PlayAfterStartup", "play", CmdFile::AT_AFTER_STARTUP, SlidesMode::OFF, SlidesMode::PLAY}),
    [](const TestParamInfo<RouterAutoKeyInput> &info) { return info.param.name; });

struct RouterAutoKeyNameInput
{
    const char *name;
    const char *filename;
    CmdFile mode;
    const char *initial;
};

void PrintTo(const RouterAutoKeyNameInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterAutoKeyName : public TestCommandRouter, public WithParamInterface<RouterAutoKeyNameInput>
{
protected:
    void SetUp() override;

    ValueSaver<fs::path> m_saved_auto_name{g_auto_name};
};

void TestRouterAutoKeyName::SetUp()
{
    TestCommandRouter::SetUp();
    g_auto_name = GetParam().initial;
}

TEST_P(TestRouterAutoKeyName, preservesTheFallbackFilename)
{
    CommandProcessor processor{"autokeyname=" + std::string{GetParam().filename}, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(fs::path{GetParam().filename}, g_auto_name);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
}

INSTANTIATE_TEST_SUITE_P(Filenames, TestRouterAutoKeyName,
    Values(RouterAutoKeyNameInput{"StartupSimple", "baz.key", CmdFile::AT_CMD_LINE, "foo.key"},
        RouterAutoKeyNameInput{"StartupMixedPath", "Missing/MixedCaseAutoKeyName.KEY", CmdFile::AT_CMD_LINE, "baz.key"},
        RouterAutoKeyNameInput{
            "InteractiveSimple", "baz.key", CmdFile::AT_AFTER_STARTUP, "Missing/MixedCaseAutoKeyName.KEY"},
        RouterAutoKeyNameInput{
            "InteractiveMixedPath", "Missing/MixedCaseAutoKeyName.KEY", CmdFile::AT_AFTER_STARTUP, "baz.key"}),
    [](const TestParamInfo<RouterAutoKeyNameInput> &info) { return info.param.name; });

struct RouterRdsState
{
    bool batch;
    bool texture;
    int depth;
    CalibrationBars bars;
    std::string filename;
    bool reuse;
};

struct RouterRdsInput
{
    const char *name;
    const char *command;
    RouterRdsState initial;
    RouterRdsState expected;
};

void PrintTo(const RouterRdsInput &input, std::ostream *out)
{
    *out << input.name;
}

class RouterRdsStateGuard
{
public:
    void apply(const RouterRdsState &initial);

private:
    ValueSaver<bool> m_saved_rds_batch{g_auto_stereo_batch};
    ValueSaver<bool> m_saved_texture{g_use_stereo_texture};
    ValueSaver<int> m_saved_depth{g_auto_stereo_depth};
    ValueSaver<CalibrationBars> m_saved_bars{g_calibrate};
    ValueSaver<std::string> m_saved_filename{g_stereo_texture_filename};
    ValueSaver<bool> m_saved_reuse{g_stereo_texture_reuse};
};

void RouterRdsStateGuard::apply(const RouterRdsState &initial)
{
    g_auto_stereo_batch = initial.batch;
    g_use_stereo_texture = initial.texture;
    g_auto_stereo_depth = initial.depth;
    g_calibrate = initial.bars;
    g_stereo_texture_filename = initial.filename;
    g_stereo_texture_reuse = initial.reuse;
}

class TestRouterRdsState : public TestCommandRouter
{
protected:
    RouterRdsStateGuard m_rds_state;
};

class TestAcceptedRouterRds : public TestRouterRdsState, public WithParamInterface<RouterRdsInput>
{
protected:
    void SetUp() override;
};

void TestAcceptedRouterRds::SetUp()
{
    TestRouterRdsState::SetUp();
    m_rds_state.apply(GetParam().initial);
}

TEST_P(TestAcceptedRouterRds, returnsTheExpectedStereoSettings)
{
    CommandProcessor processor{GetParam().command, CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    const RouterRdsState &expected{GetParam().expected};
    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(expected.batch, g_auto_stereo_batch);
    EXPECT_EQ(expected.texture, g_use_stereo_texture);
    EXPECT_EQ(expected.depth, g_auto_stereo_depth);
    EXPECT_EQ(expected.bars, g_calibrate);
    EXPECT_EQ(expected.filename, g_stereo_texture_filename);
    EXPECT_EQ(expected.reuse, g_stereo_texture_reuse);
}

INSTANTIATE_TEST_SUITE_P(Settings, TestAcceptedRouterRds,
    Values(RouterRdsInput{"No", "rds=no", {true, true, 77, CalibrationBars::TOP, "old.gif", true},
               {false, true, 77, CalibrationBars::TOP, "old.gif", true}},
        RouterRdsInput{"Yes", "rds=yes", {false, true, 123, CalibrationBars::TOP, "", true},
            {true, false, 123, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"RandomDepthAndBars", "rds=random/-120/top",
            {false, true, 100, CalibrationBars::MIDDLE, "", true},
            {true, false, -120, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"TextureNoBars", "rds=texture/80/0",
            {false, false, 100, CalibrationBars::MIDDLE, "old.gif", false},
            {true, true, 80, CalibrationBars::NONE, "old.gif", true}},
        RouterRdsInput{"TextureMiddleBars", "rds=texture/80/1",
            {true, true, 80, CalibrationBars::NONE, "old.gif", true},
            {true, true, 80, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"TextureTopBars", "rds=texture/80/2", {true, true, 80, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 80, CalibrationBars::TOP, "old.gif", true}},
        RouterRdsInput{"TextureWithoutFilename", "rds=texture", {false, false, 100, CalibrationBars::MIDDLE, "", true},
            {true, true, 100, CalibrationBars::MIDDLE, "", false}},
        RouterRdsInput{"EmptyDepth", "rds=random//top", {false, true, 77, CalibrationBars::MIDDLE, "", true},
            {true, false, 77, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"OmittedDepthAndBars", "rds=random", {false, true, 77, CalibrationBars::TOP, "", true},
            {true, false, 77, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"EmptyDepthOnly", "rds=random/", {true, false, 77, CalibrationBars::TOP, "", false},
            {true, false, 77, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"EmptyDepthAndBars", "rds=random//", {true, false, 77, CalibrationBars::TOP, "", false},
            {true, false, 77, CalibrationBars::TOP, "", false}},
        RouterRdsInput{"TrailingEmptyField", "rds=random///", {true, false, 77, CalibrationBars::TOP, "", false},
            {true, false, 77, CalibrationBars::TOP, "", false}}),
    [](const TestParamInfo<RouterRdsInput> &info) { return info.param.name; });

class TestRejectedRouterRds : public TestRouterRdsState, public WithParamInterface<RouterRdsInput>
{
protected:
    void SetUp() override;
};

void TestRejectedRouterRds::SetUp()
{
    TestRouterRdsState::SetUp();
    m_rds_state.apply(GetParam().initial);
}

TEST_P(TestRejectedRouterRds, invalidParametersPreserveAllStereoSettings)
{
    CommandProcessor processor{GetParam().command, CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    const RouterRdsState &expected{GetParam().expected};
    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_THAT(result.diagnostics.errors,
        ElementsAre("Oops. I couldn't understand the argument:\n  " + std::string{GetParam().command}));
    EXPECT_EQ(expected.batch, g_auto_stereo_batch);
    EXPECT_EQ(expected.depth, g_auto_stereo_depth);
    EXPECT_EQ(expected.bars, g_calibrate);
    EXPECT_EQ(expected.texture, g_use_stereo_texture);
    EXPECT_EQ(expected.filename, g_stereo_texture_filename);
    EXPECT_EQ(expected.reuse, g_stereo_texture_reuse);
}

INSTANTIATE_TEST_SUITE_P(RejectedSettings, TestRejectedRouterRds,
    Values(RouterRdsInput{"EmptyManual", "rds=", {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
               {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"UnknownModeManual", "rds=fmeh", {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NoWithDepthManual", "rds=no/1", {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NoWithEmptyFieldsManual", "rds=no//",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NonNumericDepthManual", "rds=random/deep",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"FractionalDepthManual", "rds=random/1.5",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"UnknownBarsManual", "rds=random/100/bottom",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NegativeBarsManual", "rds=random/100/-1",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"TooManyBarsManual", "rds=random/100/3",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"FractionalBarsManual", "rds=random/100/1.5",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"ExtraFieldManual", "rds=random/100/top/extra",
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {false, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"EmptyAutomatic", "rds=", {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"UnknownModeAutomatic", "rds=fmeh", {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NoWithDepthAutomatic", "rds=no/1", {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NoWithEmptyFieldsAutomatic", "rds=no//",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NonNumericDepthAutomatic", "rds=random/deep",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"FractionalDepthAutomatic", "rds=random/1.5",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"UnknownBarsAutomatic", "rds=random/100/bottom",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"NegativeBarsAutomatic", "rds=random/100/-1",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"TooManyBarsAutomatic", "rds=random/100/3",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"FractionalBarsAutomatic", "rds=random/100/1.5",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}},
        RouterRdsInput{"ExtraFieldAutomatic", "rds=random/100/top/extra",
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true},
            {true, true, 77, CalibrationBars::MIDDLE, "old.gif", true}}),
    [](const TestParamInfo<RouterRdsInput> &info) { return info.param.name; });

struct RouterTextureInput
{
    const char *name;
    const char *filename;
    CmdFile mode;
    bool automatic;
    bool initial_texture;
    bool initial_reuse;
    bool expected_texture;
    bool expected_reuse;
};

void PrintTo(const RouterTextureInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterStereoTexture : public TestRouterRdsState, public WithParamInterface<RouterTextureInput>
{
protected:
    void SetUp() override;
};

void TestRouterStereoTexture::SetUp()
{
    TestRouterRdsState::SetUp();
    m_rds_state.apply({GetParam().automatic, GetParam().initial_texture, g_auto_stereo_depth, g_calibrate, "old.gif",
        GetParam().initial_reuse});
}

TEST_P(TestRouterStereoTexture, preservesFilenameAndAutomaticConversionSemantics)
{
    CommandProcessor processor{"rds-texture=" + std::string{GetParam().filename}, GetParam().mode};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
    EXPECT_EQ(GetParam().filename, g_stereo_texture_filename);
    EXPECT_EQ(GetParam().expected_texture, g_use_stereo_texture);
    EXPECT_EQ(GetParam().expected_reuse, g_stereo_texture_reuse);
    EXPECT_EQ(GetParam().automatic, g_auto_stereo_batch);
}

INSTANTIATE_TEST_SUITE_P(Textures, TestRouterStereoTexture,
    Values(RouterTextureInput{"StartupManual", "Images/MixedCaseMap.GIF", CmdFile::AT_CMD_LINE, false, false, false,
               true, true},
        RouterTextureInput{
            "StartupAutomatic", "Images/MixedCaseMap.GIF", CmdFile::AT_CMD_LINE, true, false, false, true, true},
        RouterTextureInput{
            "InteractiveManual", "Images/MixedCaseMap.GIF", CmdFile::AT_AFTER_STARTUP, false, false, false, true, true},
        RouterTextureInput{"InteractiveAutomatic", "Images/MixedCaseMap.GIF", CmdFile::AT_AFTER_STARTUP, true, false,
            false, true, true},
        RouterTextureInput{"EmptyUnselectedManual", "", CmdFile::AT_AFTER_STARTUP, false, false, true, false, false},
        RouterTextureInput{"EmptyUnselectedAutomatic", "", CmdFile::AT_AFTER_STARTUP, true, false, true, false, false},
        RouterTextureInput{"EmptySelectedManual", "", CmdFile::AT_AFTER_STARTUP, false, true, true, true, false},
        RouterTextureInput{"EmptySelectedAutomatic", "", CmdFile::AT_AFTER_STARTUP, true, true, true, true, false}),
    [](const TestParamInfo<RouterTextureInput> &info) { return info.param.name; });

class TestCommandRouterFile : public TestCommandRouter
{
protected:
    void SetUp() override;
    void TearDown() override;

    TemporaryDirectory m_input_directory;
    std::optional<CommandTestFile> m_file;
    ValueSaver<double> m_saved_param_0{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_2{g_params[2], 0.0};
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
    ValueSaver<Version> m_saved_file_version{g_file_version, current_id_version()};
    ValueSaver<bool> m_saved_bif_functions{g_new_bifurcation_functions_loaded, true};
};

void TestCommandRouterFile::SetUp()
{
    TestCommandRouter::SetUp();
    m_input_directory.create("id-command-input-");
    m_file.emplace(m_input_directory.path() / "input.par");
}

void TestCommandRouterFile::TearDown()
{
    m_file.reset();
    std::error_code ignored;
    m_input_directory.remove(ignored);
    clear_command_comments();
}

TEST_F(TestCommandRouterFile, selectedSetUsesPositionAndContextForBothOwners)
{
    const std::string prefix{"ignored { ui=no }\n"};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{
        m_file->open_input(prefix + "entry (description) { ui=yes engine=yes } ui=no"), std::fclose};
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
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, uiRejectionRetainsEngineEffectsAndFlagsAndStopsTheSet)
{
    CommandProcessor processor{m_file->open_input("entry { engine=yes ui=bad engine=no }"), CmdFile::AT_AFTER_STARTUP};
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
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, engineRejectionRetainsUiEffectsAndFlagsAndStopsTheSet)
{
    CommandProcessor processor{m_file->open_input("entry { ui=yes engine=bad ui=no }"), CmdFile::AT_AFTER_STARTUP};
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
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, uiFlagsTriggerFileCompatibilityOnceEvenAfterRejection)
{
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
    CommandProcessor processor{m_file->open_input("entry { ui=yes engine=bad }"), CmdFile::AT_AFTER_STARTUP};
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
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, actionStopsTheSetAndRetainsBothOwnersResults)
{
    CommandProcessor processor{m_file->open_input("entry { ui=yes engine=yes ui=no }"), CmdFile::AT_AFTER_STARTUP};
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
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, defaultRouterRetainsSelectedSetInitializationAndLaterEffects)
{
    const CommandRouter router;
    CommandProcessor initial{"params=77", CmdFile::AT_CMD_LINE};
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, router.process(initial).flags);
    CommandProcessor processor{
        m_file->open_input("entry { type=mandel maxiter=222 hertz=550 }"), CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_DOUBLE_EQ(0.0, g_params[0]);
    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(550, g_base_hertz);
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, selectedSetRoutesVideoPromptParameter)
{
    ValueSaver saved_ask_video{g_ask_video, true};
    CommandProcessor processor{m_file->open_input("entry { askvideo=no }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_ask_video);
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, selectedSetRoutesVideoSelectionParameter)
{
    ValueSaver saved_video_table_len{g_video_table_len, 1};
    ValueSaver saved_video_key{g_video_mode_keys[0], ID_KEY_F6};
    ValueSaver saved_init_mode{g_init_mode, -1};
    CommandProcessor processor{m_file->open_input("entry { video=F6 }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(0, g_init_mode);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, selectedSetRoutesVirtualScreenParameterAndCompletesCompatibility)
{
    ValueSaver saved_virtual_screens{g_virtual_screens, true};
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
    CommandProcessor processor{m_file->open_input("entry { virtual=no }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_FALSE(g_virtual_screens);
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(m_file->remove());
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
    CommandProcessor processor{m_file->open_input(text), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(10, g_max_image_history);
    EXPECT_EQ(DebugFlags::NONE, g_debug_flag);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  maxhistory=20"));
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, selectedSetRoutesAutokeyModeParameter)
{
    ValueSaver saved_slides{g_slides, SlidesMode::OFF};
    CommandProcessor processor{m_file->open_input("entry { autokey=record autokey=play }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(SlidesMode::PLAY, g_slides);
    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandRouterFile, selectedSetRoutesAutokeyFilenameParameter)
{
    ValueSaver saved_auto_key_name{g_auto_name, "foo.key"};
    CommandProcessor processor{
        m_file->open_input("entry { autokeyname=SelectedAutokeyFallback.KEY }"), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(fs::path{"SelectedAutokeyFallback.KEY"}, g_auto_name);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
    EXPECT_TRUE(m_file->remove());
}

class TestSelectedRdsOrder : public TestCommandRouterFile, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

    RouterRdsStateGuard m_rds_state;
};

void TestSelectedRdsOrder::SetUp()
{
    TestCommandRouterFile::SetUp();
    m_rds_state.apply({false, false, 77, CalibrationBars::TOP, "", false});
}

TEST_P(TestSelectedRdsOrder, preservesRdsAndTextureParameterOrder)
{
    const CommandRouter router;
    const std::string_view commands{GetParam() ? "entry { rds-texture=MixedCaseMap.GIF rds=random }"
                                               : "entry { rds=random rds-texture=MixedCaseMap.GIF }"};
    CommandProcessor processor{m_file->open_input(commands), CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(g_auto_stereo_batch);
    EXPECT_EQ(!GetParam(), g_use_stereo_texture);
    EXPECT_EQ(!GetParam(), g_stereo_texture_reuse);
    EXPECT_EQ("MixedCaseMap.GIF", g_stereo_texture_filename);
    EXPECT_EQ(77, g_auto_stereo_depth);
    EXPECT_EQ(CalibrationBars::TOP, g_calibrate);
    EXPECT_TRUE(m_file->remove());
}

INSTANTIATE_TEST_SUITE_P(Orders, TestSelectedRdsOrder, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "TextureFirst" : "RandomFirst"; });

class TestCommandRouterStartup : public TestCommandRouterFile
{
protected:
    void SetUp() override;
    void TearDown() override;
    CommandTestFile &source(const fs::path &name);

    TemporaryDirectory m_directory;
    fs::path m_library;
    std::map<fs::path, CommandTestFile> m_sources;
    const std::shared_ptr<NiceMock<MockSpecialDirectories>> m_directories{
        std::make_shared<NiceMock<MockSpecialDirectories>>()};
    ValueSaver<std::shared_ptr<SpecialDirectories>> m_saved_directories{g_special_dirs, m_directories};
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
    ValueSaver<fs::path> m_saved_temp_dir{g_temp_dir};
    ValueSaver<int> m_saved_init_mode{g_init_mode};
    ValueSaver<ShowFile> m_saved_show_file{g_show_file};
    ValueSaver<bool> m_saved_colors_preloaded{g_colors_preloaded};
    ValueSaver<bool> m_saved_read_color{g_read_color};
    ValueSaver<bool> m_saved_ask_video{g_ask_video};
    ValueSaver<bool> m_saved_virtual_screens{g_virtual_screens};
    ValueSaver<bool> m_saved_escape_exit{g_escape_exit};
};

void TestCommandRouterStartup::SetUp()
{
    m_directory.create("id-router-startup-");
    m_library = m_directory.path() / ID_PROGRAM_NAME;
    fs::create_directories(m_library);
    ON_CALL(*m_directories, program_dir()).WillByDefault(Return(m_library));
    ON_CALL(*m_directories, documents_dir()).WillByDefault(Return(m_directory.path()));
    g_first_init = true;
    source("sstools.ini").write("");
}

void TestCommandRouterStartup::TearDown()
{
    clear_read_library_path();
    clear_save_library();
    m_sources.clear();
    std::error_code ignored;
    m_directory.remove(ignored);
    TestCommandRouterFile::TearDown();
}

CommandTestFile &TestCommandRouterStartup::source(const fs::path &name)
{
    return m_sources.try_emplace(name, m_library / name).first->second;
}

TEST_F(TestCommandRouterStartup, routesConfigurationIndirectSetsAndArgumentsInOrder)
{
    source("sstools.ini").write("ui=1 engine=2\n");
    source("whole.par").write("entry { engine=3 ui=4 }");
    source("selected.par").write("ignored { ui=999 }\nchosen { ui=5 engine=6 }");
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
    EXPECT_TRUE(source("sstools.ini").remove());
    EXPECT_TRUE(source("whole.par").remove());
    EXPECT_TRUE(source("selected.par").remove());
}

TEST_F(TestCommandRouterStartup, configurationRejectionStopsAllStartupSources)
{
    source("sstools.ini").write("engine=111 ui=bad engine=999\n");
    const InSequence sequence;
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                g_max_iterations = 111;
                return CmdArgFlags::FRACTAL_PARAM;
            });
    EXPECT_CALL(m_ui_handler, Call(_)).WillOnce([](const Command &cmd) { return cmd.bad_arg(); });

    const CommandParseResult result{m_router.process_startup({"id", "ui=yes", "engine=222"})};

    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre(HasSubstr("ui=bad")));
    EXPECT_TRUE(source("sstools.ini").remove());
}

struct RouterStartupOwnerInput
{
    const char *name;
    const char *owner;
    int initial_hertz;
};

void PrintTo(const RouterStartupOwnerInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterStartupRejection : public TestCommandRouterStartup, public WithParamInterface<RouterStartupOwnerInput>
{
protected:
    void SetUp() override;

    std::vector<std::string> m_accepted;
};

void TestRouterStartupRejection::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_base_hertz = GetParam().initial_hertz;
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillRepeatedly(
            [this](const Command &cmd)
            {
                if (cmd.value == "bad")
                {
                    return cmd.bad_arg();
                }
                m_accepted.emplace_back("ui=" + std::string{cmd.value});
                g_base_hertz = cmd.num_val;
                return CmdArgFlags::PARAM_3D;
            });
    EXPECT_CALL(m_engine_handler, Call(_))
        .WillRepeatedly(
            [this](const Command &cmd)
            {
                if (cmd.value == "bad")
                {
                    return cmd.bad_arg();
                }
                m_accepted.emplace_back("engine=" + std::string{cmd.value});
                g_max_iterations = cmd.num_val;
                return CmdArgFlags::FRACTAL_PARAM;
            });
}

TEST_P(TestRouterStartupRejection, rejectionStopsAllLaterStartupSources)
{
    const std::string owner{GetParam().owner};
    source("broken.par").write("entry { engine=210 " + owner + "=bad ui=999 }");

    const CommandParseResult result{
        m_router.process_startup({"id", "ui=100", owner + "=bad", "engine=200", "@broken", "ui=300"})};

    EXPECT_THAT(m_accepted, ElementsAre("ui=100"));
    EXPECT_EQ(100, g_base_hertz);
    EXPECT_EQ(150, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre(HasSubstr(owner + "=bad")));
    EXPECT_TRUE(source("broken.par").remove());
}

INSTANTIATE_TEST_SUITE_P(Owners, TestRouterStartupRejection,
    Values(RouterStartupOwnerInput{"Ui", "ui", 440}, RouterStartupOwnerInput{"Engine", "engine", 100}),
    [](const TestParamInfo<RouterStartupOwnerInput> &info) { return info.param.name; });

TEST_F(TestCommandRouterStartup, initializesBeforeConfigurationAndPreservesInitialRequests)
{
    source("sstools.ini").write("ui=yes\n");
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
    source("sstools.ini").write("ui=yes\n");
    const fs::path process_state{m_directory.path() / "process-state"};
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

class TestRouterStartupOverride : public TestCommandRouterStartup, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
};

void TestRouterStartupOverride::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_first_init = GetParam();
    g_init_mode = 9;
    if (!GetParam())
    {
        g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
        g_show_file = ShowFile::LOAD_IMAGE;
    }
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [](const Command &)
            {
                EXPECT_EQ(-1, g_init_mode);
                g_debug_flag = DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME;
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                return CmdArgFlags::NONE;
            });
}

TEST_P(TestRouterStartupOverride, preservesFirstInitAndRestartSuppression)
{
    const CommandParseResult result{m_router.process_startup({"id", "ui=yes"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(GetParam(), g_first_init);
    EXPECT_EQ(GetParam() ? 7 : -1, g_init_mode);
    EXPECT_EQ(GetParam() ? ShowFile::LOAD_IMAGE : ShowFile::IMAGE_LOADED, g_show_file);
}

INSTANTIATE_TEST_SUITE_P(Phases, TestRouterStartupOverride, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "Initial" : "Restart"; });

TEST_F(TestCommandRouterStartup, fallbackReappliesOnlyDefaultsAndConfiguration)
{
    source("sstools.ini").write("type=formula maxiter=321 ui=yes\n");
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
    const fs::path process_state{m_directory.path() / "process-state"};
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
    EXPECT_TRUE(source("sstools.ini").remove());
}

TEST_F(TestCommandRouterStartup, fallbackFinalizesFirstInitWithoutDebugOverride)
{
    const CommandRouter router{CommandHandlerTables{{}, m_ui}, engine_command_handlers()};
    const CommandParseResult startup{router.process_startup({"id", "debugflag=110", "type=formula"})};
    ASSERT_TRUE(startup.diagnostics.errors.empty());
    ASSERT_TRUE(startup.needs_file_entry);
    ASSERT_TRUE(g_first_init);

    const CommandParseResult fallback{router.reset_startup()};

    EXPECT_TRUE(fallback.diagnostics.errors.empty());
    EXPECT_FALSE(fallback.needs_file_entry);
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_EQ(DebugFlags::NONE, g_debug_flag);
    EXPECT_FALSE(g_first_init);

    const fs::path process_state{m_directory.path() / "process-state"};
    g_temp_dir = process_state;
    source("sstools.ini").write("ui=yes\n");
    EXPECT_CALL(m_ui_handler, Call(_))
        .WillOnce(
            [&](const Command &)
            {
                EXPECT_FALSE(g_first_init);
                EXPECT_EQ(process_state, g_temp_dir);
                g_init_mode = 7;
                g_show_file = ShowFile::LOAD_IMAGE;
                g_colors_preloaded = true;
                return CmdArgFlags::NONE;
            });

    const CommandParseResult restarted{router.process_startup({"id"})};

    EXPECT_TRUE(restarted.diagnostics.errors.empty());
    EXPECT_EQ(process_state, g_temp_dir);
    EXPECT_EQ(-1, g_init_mode);
    EXPECT_EQ(ShowFile::IMAGE_LOADED, g_show_file);
    EXPECT_TRUE(g_read_color);
    EXPECT_FALSE(g_first_init);
}

TEST_F(TestCommandRouterStartup, fallbackRetainsFirstInitWithConfigurationDebugOverride)
{
    source("sstools.ini").write("debugflag=110\n");
    const CommandRouter router;
    const CommandParseResult startup{router.process_startup({"id", "type=formula"})};
    ASSERT_TRUE(startup.diagnostics.errors.empty());
    ASSERT_TRUE(startup.needs_file_entry);
    ASSERT_TRUE(g_first_init);
    const fs::path process_state{m_directory.path() / "process-state"};
    g_temp_dir = process_state;

    const CommandParseResult fallback{router.reset_startup()};

    EXPECT_TRUE(fallback.diagnostics.errors.empty());
    EXPECT_FALSE(fallback.needs_file_entry);
    EXPECT_EQ(process_state, g_temp_dir);
    EXPECT_EQ(DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME, g_debug_flag);
    EXPECT_TRUE(g_first_init);
}

class TestRouterFinalType : public TestCommandRouterStartup, public WithParamInterface<bool>
{
};

TEST_P(TestRouterFinalType, selectsTheFinalTypePolicy)
{
    const CommandRouter router;
    const std::vector<std::string> arguments{GetParam() ? std::vector<std::string>{"id", "type=formula", "batch=yes"}
                                                        : std::vector<std::string>{"id", "type=formula"}};

    const CommandParseResult result{router.process_startup(arguments)};

    EXPECT_EQ(!GetParam(), result.needs_file_entry);
    if (GetParam())
    {
        EXPECT_THAT(result.diagnostics.errors, ElementsAre(HasSubstr("type=formula requires formulaname=")));
    }
    else
    {
        EXPECT_TRUE(result.diagnostics.errors.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Policies, TestRouterFinalType, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "Batch" : "Interactive"; });

struct RouterHistorySourceInput
{
    const char *name;
    std::vector<std::string> arguments;
    int initial;
    int expected;
};

void PrintTo(const RouterHistorySourceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterHistorySources : public TestCommandRouterStartup, public WithParamInterface<RouterHistorySourceInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_capacity{g_max_image_history};
};

void TestRouterHistorySources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_max_image_history = GetParam().initial;
    source("sstools.ini").write("maxhistory=2\n");
    source("whole.par").write("entry { maxhistory=4 }");
    source("selected.par").write("ignored { maxhistory=99 }\nchosen { maxhistory=6 }");
}

TEST_P(TestRouterHistorySources, laterSourcesDetermineTheHistoryCapacity)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(GetParam().expected, g_max_image_history);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterHistorySources,
    Values(RouterHistorySourceInput{"Configuration", {"id"}, 0, 2},
        RouterHistorySourceInput{"WholeFile", {"id", "@whole"}, 2, 4},
        RouterHistorySourceInput{"SelectedSet", {"id", "@whole", "@selected/chosen"}, 4, 6},
        RouterHistorySourceInput{"Argument", {"id", "@whole", "@selected/chosen", "maxhistory=8"}, 6, 8}),
    [](const TestParamInfo<RouterHistorySourceInput> &info) { return info.param.name; });

struct RouterSlideSourceInput
{
    const char *name;
    std::vector<std::string> arguments;
    SlidesMode initial;
    SlidesMode expected;
};

void PrintTo(const RouterSlideSourceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterSlideSources : public TestCommandRouterStartup, public WithParamInterface<RouterSlideSourceInput>
{
protected:
    void SetUp() override;

    ValueSaver<SlidesMode> m_saved_slides{g_slides};
};

void TestRouterSlideSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_slides = GetParam().initial;
    source("sstools.ini").write("autokey=record\n");
    source("whole.par").write("entry { autokey=play }");
    source("selected.par").write("ignored { autokey=play }\nchosen { autokey=record }");
}

TEST_P(TestRouterSlideSources, laterSourcesDetermineTheSlideMode)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(GetParam().expected, g_slides);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterSlideSources,
    Values(RouterSlideSourceInput{"Configuration", {"id"}, SlidesMode::OFF, SlidesMode::RECORD},
        RouterSlideSourceInput{"WholeFile", {"id", "@whole"}, SlidesMode::RECORD, SlidesMode::PLAY},
        RouterSlideSourceInput{
            "SelectedSet", {"id", "@whole", "@selected/chosen"}, SlidesMode::PLAY, SlidesMode::RECORD},
        RouterSlideSourceInput{
            "Argument", {"id", "@whole", "@selected/chosen", "autokey=play"}, SlidesMode::RECORD, SlidesMode::PLAY}),
    [](const TestParamInfo<RouterSlideSourceInput> &info) { return info.param.name; });

TEST_F(TestCommandRouterStartup, autoKeyNameFindsLibraryKeyFileWithOriginalCase)
{
    ValueSaver saved_auto_key_name{g_auto_name, "foo.key"};
    ValueSaver saved_check_cur_dir{g_check_cur_dir, false};
    clear_read_library_path();
    add_read_library(m_library);
    fs::create_directory(m_library / "key");
    source("key/MixedCase.KEY").write("ESC\n");
    CommandProcessor processor{"autokeyname=MixedCase.KEY", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(m_library / "key/MixedCase.KEY", g_auto_name);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
    EXPECT_TRUE(source("key/MixedCase.KEY").remove());
    EXPECT_TRUE(fs::remove(m_library / "key"));
}

struct RouterAutoNameSourceInput
{
    const char *name;
    std::vector<std::string> arguments;
    const char *initial;
    const char *expected;
};

void PrintTo(const RouterAutoNameSourceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterAutoNameSources : public TestCommandRouterStartup, public WithParamInterface<RouterAutoNameSourceInput>
{
protected:
    void SetUp() override;

    ValueSaver<fs::path> m_saved_auto_name{g_auto_name};
};

void TestRouterAutoNameSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_auto_name = GetParam().initial;
    source("sstools.ini").write("autokeyname=ConfigAutoKeyRoute.KEY\n");
    source("whole.par").write("entry { autokeyname=WholeAutoKeyRoute.KEY }");
    source("selected.par")
        .write("ignored { autokeyname=IgnoredAutoKeyRoute.KEY }\n"
               "chosen { autokeyname=SelectedAutoKeyRoute.KEY }");
}

TEST_P(TestRouterAutoNameSources, laterSourcesDetermineTheAutokeyFilename)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(fs::path{GetParam().expected}, g_auto_name);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterAutoNameSources,
    Values(RouterAutoNameSourceInput{"Configuration", {"id"}, "foo.key", "ConfigAutoKeyRoute.KEY"},
        RouterAutoNameSourceInput{"WholeFile", {"id", "@whole"}, "ConfigAutoKeyRoute.KEY", "WholeAutoKeyRoute.KEY"},
        RouterAutoNameSourceInput{
            "SelectedSet", {"id", "@whole", "@selected/chosen"}, "WholeAutoKeyRoute.KEY", "SelectedAutoKeyRoute.KEY"},
        RouterAutoNameSourceInput{"Argument",
            {"id", "@whole", "@selected/chosen", "autokeyname=ArgumentAutoKeyRoute.KEY"}, "SelectedAutoKeyRoute.KEY",
            "ArgumentAutoKeyRoute.KEY"}),
    [](const TestParamInfo<RouterAutoNameSourceInput> &info) { return info.param.name; });

struct RouterRdsSourceInput
{
    const char *name;
    std::vector<std::string> arguments;
    RouterRdsState initial;
    RouterRdsState expected;
};

void PrintTo(const RouterRdsSourceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterRdsSources : public TestCommandRouterStartup, public WithParamInterface<RouterRdsSourceInput>
{
protected:
    void SetUp() override;

    RouterRdsStateGuard m_rds_state;
};

void TestRouterRdsSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    m_rds_state.apply(GetParam().initial);
    source("sstools.ini").write("rds=yes/10/none\n");
    source("whole.par").write("entry { rds=texture/20/top }");
    source("selected.par").write("ignored { rds=no }\nchosen { rds=random/30/middle }");
}

TEST_P(TestRouterRdsSources, laterSourcesDetermineTheStereoSettings)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    const RouterRdsState &expected{GetParam().expected};
    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(expected.batch, g_auto_stereo_batch);
    EXPECT_EQ(expected.texture, g_use_stereo_texture);
    EXPECT_EQ(expected.reuse, g_stereo_texture_reuse);
    EXPECT_EQ(expected.depth, g_auto_stereo_depth);
    EXPECT_EQ(expected.bars, g_calibrate);
    EXPECT_EQ(expected.filename, g_stereo_texture_filename);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterRdsSources,
    Values(RouterRdsSourceInput{"Configuration", {"id"}, {false, false, 77, CalibrationBars::MIDDLE, "map.gif", false},
               {true, false, 10, CalibrationBars::NONE, "map.gif", false}},
        RouterRdsSourceInput{"WholeFile", {"id", "@whole"}, {true, false, 10, CalibrationBars::NONE, "map.gif", false},
            {true, true, 20, CalibrationBars::TOP, "map.gif", true}},
        RouterRdsSourceInput{"SelectedSet", {"id", "@whole", "@selected/chosen"},
            {true, true, 20, CalibrationBars::TOP, "map.gif", true},
            {true, false, 30, CalibrationBars::MIDDLE, "map.gif", false}},
        RouterRdsSourceInput{"Argument", {"id", "@whole", "@selected/chosen", "rds=no"},
            {true, false, 30, CalibrationBars::MIDDLE, "map.gif", false},
            {false, false, 30, CalibrationBars::MIDDLE, "map.gif", false}}),
    [](const TestParamInfo<RouterRdsSourceInput> &info) { return info.param.name; });

class TestRouterTextureSources : public TestCommandRouterStartup, public WithParamInterface<RouterRdsSourceInput>
{
protected:
    void SetUp() override;

    RouterRdsStateGuard m_rds_state;
};

void TestRouterTextureSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    m_rds_state.apply(GetParam().initial);
    source("sstools.ini").write("rds-texture=ConfigTexture.GIF\n");
    source("whole.par").write("entry { rds-texture=WholeTexture.GIF }");
    source("selected.par")
        .write("ignored { rds-texture=IgnoredTexture.GIF }\nchosen { rds-texture=SelectedTexture.GIF }");
}

TEST_P(TestRouterTextureSources, laterSourcesDetermineTheTextureSettings)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    const RouterRdsState &expected{GetParam().expected};
    EXPECT_EQ(CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_TRUE(result.diagnostics.warnings.empty());
    EXPECT_EQ(expected.filename, g_stereo_texture_filename);
    EXPECT_EQ(expected.texture, g_use_stereo_texture);
    EXPECT_EQ(expected.reuse, g_stereo_texture_reuse);
    EXPECT_EQ(expected.batch, g_auto_stereo_batch);
    EXPECT_EQ(expected.depth, g_auto_stereo_depth);
    EXPECT_EQ(expected.bars, g_calibrate);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterTextureSources,
    Values(RouterRdsSourceInput{"Configuration", {"id"}, {true, false, 100, CalibrationBars::MIDDLE, "old.gif", false},
               {true, true, 100, CalibrationBars::MIDDLE, "ConfigTexture.GIF", true}},
        RouterRdsSourceInput{"WholeFile", {"id", "@whole"},
            {true, true, 100, CalibrationBars::MIDDLE, "ConfigTexture.GIF", true},
            {true, true, 100, CalibrationBars::MIDDLE, "WholeTexture.GIF", true}},
        RouterRdsSourceInput{"SelectedSet", {"id", "@whole", "@selected/chosen"},
            {true, true, 100, CalibrationBars::MIDDLE, "WholeTexture.GIF", true},
            {true, true, 100, CalibrationBars::MIDDLE, "SelectedTexture.GIF", true}},
        RouterRdsSourceInput{"Argument", {"id", "@whole", "@selected/chosen", "rds-texture="},
            {true, true, 100, CalibrationBars::MIDDLE, "SelectedTexture.GIF", true},
            {true, true, 100, CalibrationBars::MIDDLE, "", false}}),
    [](const TestParamInfo<RouterRdsSourceInput> &info) { return info.param.name; });

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeaveVideoPromptPreferenceAlone)
{
    g_ask_video = false;

    engine::reset_restart_defaults();

    EXPECT_FALSE(g_ask_video);
}

struct RouterPreferenceSourceInput
{
    const char *name;
    std::vector<std::string> arguments;
    bool indirect;
    bool expected;
};

void PrintTo(const RouterPreferenceSourceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterAskVideoSources : public TestCommandRouterStartup,
                                  public WithParamInterface<RouterPreferenceSourceInput>
{
protected:
    void SetUp() override;
};

void TestRouterAskVideoSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_ask_video = false;
    if (GetParam().indirect)
    {
        source("whole.par").write("entry { askvideo=no }");
        source("selected.par").write("ignored { askvideo=no }\nchosen { askvideo=yes }");
    }
    else
    {
        source("sstools.ini").write("askvideo=no\n");
    }
}

TEST_P(TestRouterAskVideoSources, laterSourcesDetermineTheVideoPromptPreference)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(GetParam().expected, g_ask_video);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterAskVideoSources,
    Values(RouterPreferenceSourceInput{"Configuration", {"id"}, false, false},
        RouterPreferenceSourceInput{"Argument", {"id", "askvideo=yes"}, false, true},
        RouterPreferenceSourceInput{"WholeFile", {"id", "@whole"}, true, false},
        RouterPreferenceSourceInput{"SelectedSet", {"id", "@whole", "@selected/chosen"}, true, true}),
    [](const TestParamInfo<RouterPreferenceSourceInput> &info) { return info.param.name; });

class TestRouterStartupPhase : public TestCommandRouterStartup, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
};

void TestRouterStartupPhase::SetUp()
{
    TestCommandRouterStartup::SetUp();
    if (!GetParam())
    {
        const CommandRouter router;
        ASSERT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    }
}

TEST_P(TestRouterStartupPhase, restoresVideoPromptDefault)
{
    const CommandRouter router;
    g_ask_video = false;

    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());

    EXPECT_TRUE(g_ask_video);
    EXPECT_FALSE(g_first_init);
}

TEST_P(TestRouterStartupPhase, restoresVirtualScreenDefault)
{
    const CommandRouter router;
    g_virtual_screens = false;

    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());

    EXPECT_TRUE(g_virtual_screens);
    EXPECT_FALSE(g_first_init);
}

TEST_P(TestRouterStartupPhase, restoresExitConfirmationDefault)
{
    const CommandRouter router;
    g_escape_exit = true;

    EXPECT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());

    EXPECT_FALSE(g_escape_exit);
    EXPECT_FALSE(g_first_init);
}

INSTANTIATE_TEST_SUITE_P(Phase, TestRouterStartupPhase, Values(true, false),
    [](const TestParamInfo<bool> &info) { return info.param ? "Initial" : "Restart"; });

class TestRouterStartupFallback : public TestCommandRouterStartup
{
protected:
    void SetUp() override;
};

void TestRouterStartupFallback::SetUp()
{
    TestCommandRouterStartup::SetUp();
    const CommandRouter router;
    ASSERT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
    ASSERT_TRUE(router.process_startup({"id"}).diagnostics.errors.empty());
}

TEST_F(TestRouterStartupFallback, restoresVideoPromptDefault)
{
    const CommandRouter router;
    g_ask_video = false;

    EXPECT_TRUE(router.reset_startup().diagnostics.errors.empty());

    EXPECT_TRUE(g_ask_video);
}

TEST_F(TestRouterStartupFallback, restoresVirtualScreenDefault)
{
    const CommandRouter router;
    g_virtual_screens = false;

    EXPECT_TRUE(router.reset_startup().diagnostics.errors.empty());

    EXPECT_TRUE(g_virtual_screens);
}

TEST_F(TestRouterStartupFallback, restoresExitConfirmationDefault)
{
    const CommandRouter router;
    g_escape_exit = true;

    EXPECT_TRUE(router.reset_startup().diagnostics.errors.empty());

    EXPECT_FALSE(g_escape_exit);
}

TEST_F(TestCommandRouterStartup, fallbackRetainsVideoPromptConfigurationOverride)
{
    const CommandRouter router;
    source("sstools.ini").write("askvideo=no\n");
    g_ask_video = true;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_FALSE(g_ask_video);
}

class TestRouterResetPreference : public TestCommandRouterStartup, public WithParamInterface<bool>
{
};

TEST_P(TestRouterResetPreference, preservesVideoPromptPreference)
{
    const CommandRouter router;
    g_ask_video = GetParam();
    CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_EQ(GetParam(), g_ask_video);
}

TEST_P(TestRouterResetPreference, preservesVirtualScreenPreference)
{
    const CommandRouter router;
    g_virtual_screens = GetParam();
    CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_EQ(GetParam(), g_virtual_screens);
}

INSTANTIATE_TEST_SUITE_P(Preference, TestRouterResetPreference, Values(true, false),
    [](const TestParamInfo<bool> &info) { return info.param ? "Enabled" : "Disabled"; });

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeaveVirtualScreenPreferenceAlone)
{
    g_virtual_screens = false;

    engine::reset_restart_defaults();

    EXPECT_FALSE(g_virtual_screens);
}

class TestRouterVirtualSources : public TestCommandRouterStartup, public WithParamInterface<RouterPreferenceSourceInput>
{
protected:
    void SetUp() override;
};

void TestRouterVirtualSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    g_virtual_screens = false;
    if (GetParam().indirect)
    {
        source("whole.par").write("entry { virtual=no }");
        source("selected.par").write("ignored { virtual=no }\nchosen { virtual=yes }");
    }
    else
    {
        source("sstools.ini").write("virtual=no\n");
    }
}

TEST_P(TestRouterVirtualSources, laterSourcesDetermineTheVirtualScreenPreference)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_EQ(GetParam().expected, g_virtual_screens);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterVirtualSources,
    Values(RouterPreferenceSourceInput{"Configuration", {"id"}, false, false},
        RouterPreferenceSourceInput{"Argument", {"id", "virtual=yes"}, false, true},
        RouterPreferenceSourceInput{"WholeFile", {"id", "@whole"}, true, false},
        RouterPreferenceSourceInput{"SelectedSet", {"id", "@whole", "@selected/chosen"}, true, true}),
    [](const TestParamInfo<RouterPreferenceSourceInput> &info) { return info.param.name; });

TEST_F(TestCommandRouterStartup, fallbackRetainsVirtualScreenConfigurationOverride)
{
    const CommandRouter router;
    source("sstools.ini").write("virtual=no\n");
    g_virtual_screens = true;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_FALSE(g_virtual_screens);
}

TEST_F(TestCommandRouterStartup, engineRestartDefaultsLeavePendingModeAlone)
{
    g_init_mode = 7;

    engine::reset_restart_defaults();

    EXPECT_EQ(7, g_init_mode);
}

class TestRouterPendingMode : public TestCommandRouterStartup
{
protected:
    void expect_active_mode_unchanged();

    const VideoInfo m_active{800, 600, 256, nullptr, "active"};
    ValueSaver<int> m_saved_adapter{g_adapter, 0};
    ValueSaver<VideoInfo> m_saved_entry{g_video_entry, m_active};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 800};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 600};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_table_length{g_video_table_len, 3};
    ValueSaver<int> m_saved_first_key{g_video_mode_keys[0], ID_KEY_F6};
    ValueSaver<int> m_saved_second_key{g_video_mode_keys[1], ID_KEY_F5};
    ValueSaver<int> m_saved_third_key{g_video_mode_keys[2], ID_KEY_F4};
};

void TestRouterPendingMode::expect_active_mode_unchanged()
{
    EXPECT_EQ(0, g_adapter);
    EXPECT_EQ(m_active, g_video_entry);
    EXPECT_EQ(800, g_screen_x_dots);
    EXPECT_EQ(600, g_screen_y_dots);
    EXPECT_EQ(256, g_colors);
}

TEST_F(TestRouterPendingMode, defaultsDoNotActivateVideoMode)
{
    g_init_mode = 9;
    const CommandRouter router;

    const CommandParseResult result{router.process_startup({"id"})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::NONE, result.flags);
    EXPECT_EQ(-1, g_init_mode);
    expect_active_mode_unchanged();
}

struct RouterPendingModeInput
{
    const char *name;
    std::vector<std::string> arguments;
    int initial;
    int expected;
};

void PrintTo(const RouterPendingModeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterPendingModeSources : public TestRouterPendingMode, public WithParamInterface<RouterPendingModeInput>
{
protected:
    void SetUp() override;
};

void TestRouterPendingModeSources::SetUp()
{
    TestRouterPendingMode::SetUp();
    source("sstools.ini").write("video=F6\n");
    source("whole.par").write("entry { video=F5 }");
    source("selected.par").write("ignored { video=F6 }\nchosen { video=F4 }");
    g_init_mode = GetParam().initial;
}

TEST_P(TestRouterPendingModeSources, sourcesSelectAPendingModeWithoutActivatingIt)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_EQ(GetParam().expected, g_init_mode);
    expect_active_mode_unchanged();
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterPendingModeSources,
    Values(RouterPendingModeInput{"Configuration", {"id"}, -1, 0},
        RouterPendingModeInput{"WholeFile", {"id", "@whole"}, 0, 1},
        RouterPendingModeInput{"SelectedSet", {"id", "@whole", "@selected/chosen"}, 1, 2},
        RouterPendingModeInput{"Argument", {"id", "@whole", "@selected/chosen", "video=F5"}, 2, 1}),
    [](const TestParamInfo<RouterPendingModeInput> &info) { return info.param.name; });

class TestRouterResetPendingMode : public TestCommandRouterStartup, public WithParamInterface<int>
{
};

TEST_P(TestRouterResetPendingMode, preservesPendingMode)
{
    const CommandRouter router;
    g_init_mode = GetParam();
    CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};

    const CommandParseResult result{router.process(processor)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_EQ(GetParam(), g_init_mode);
}

INSTANTIATE_TEST_SUITE_P(Mode, TestRouterResetPendingMode, Values(-1, 7),
    [](const TestParamInfo<int> &info) { return info.param < 0 ? "Unselected" : "Selected"; });

TEST_F(TestCommandRouterStartup, engineFractalDefaultsLeaveExitConfirmationAlone)
{
    g_escape_exit = true;

    engine::reset_fractal_defaults();

    EXPECT_TRUE(g_escape_exit);
}

TEST_F(TestCommandRouterStartup, fallbackRetainsExitConfirmationConfigurationOverride)
{
    const CommandRouter router;
    source("sstools.ini").write("exitnoask=yes\n");
    g_escape_exit = false;

    const CommandParseResult result{router.reset_startup()};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, result.flags);
    EXPECT_TRUE(g_escape_exit);
}

TEST_F(TestCommandRouterStartup, explicitResetPreservesUnrelatedUiPreferences)
{
    ValueSaver saved_history{g_max_image_history, 23};
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    ValueSaver saved_auto_name{g_auto_name, fs::path{"Keep.KEY"}};
    ValueSaver saved_auto_stereo{g_auto_stereo_batch, true};
    ValueSaver saved_texture{g_stereo_texture_filename, std::string{"Keep.GIF"}};
    ValueSaver saved_reuse{g_stereo_texture_reuse, true};
    g_ask_video = false;
    g_virtual_screens = false;
    g_init_mode = 7;
    g_escape_exit = true;
    g_max_iterations = 999;
    CommandProcessor processor{"reset=1960", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_FALSE(g_escape_exit);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, g_max_iterations);
    EXPECT_FALSE(g_ask_video);
    EXPECT_FALSE(g_virtual_screens);
    EXPECT_EQ(7, g_init_mode);
    EXPECT_EQ(23, g_max_image_history);
    EXPECT_EQ(SlidesMode::PLAY, g_slides);
    EXPECT_EQ(fs::path{"Keep.KEY"}, g_auto_name);
    EXPECT_TRUE(g_auto_stereo_batch);
    EXPECT_EQ("Keep.GIF", g_stereo_texture_filename);
    EXPECT_TRUE(g_stereo_texture_reuse);
}

TEST_F(TestCommandRouterFile, repeatedResetsRetainLaterSettingsWithoutADeferredReset)
{
    CommandProcessor processor{m_file->open_input("entry { exitnoask=yes type=julia maxiter=999 reset=1960 maxiter=321 "
                                                  "exitnoask=yes reset=1/5 maxiter=777 exitnoask=yes }"),
        CmdFile::AT_CMD_LINE_SET_NAME};
    ValueSaver saved_escape_exit{g_escape_exit};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D | CmdArgFlags::RESET, result.flags);
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_EQ(777L, g_max_iterations);
    EXPECT_EQ((Version{1, 5, 0, 0, false}), g_version);
    EXPECT_EQ(g_version, g_file_version);
    EXPECT_TRUE(g_escape_exit);
}

TEST_F(TestCommandRouterFile, acceptedResetRemainsEffectiveBeforeALaterError)
{
    CommandProcessor processor{
        m_file->open_input("entry { maxiter=999 reset=1960 maxiter=321 unknown=yes maxiter=777 }"),
        CmdFile::AT_AFTER_STARTUP};
    ValueSaver saved_escape_exit{g_escape_exit, true};
    const CommandRouter router;

    const CommandParseResult result{router.process(processor)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  unknown=yes"));
    EXPECT_EQ(321L, g_max_iterations);
    EXPECT_EQ(parse_legacy_version(1960), g_version);
    EXPECT_EQ(g_version, g_file_version);
    EXPECT_FALSE(g_escape_exit);
}

struct RouterInlineResetInput
{
    const char *name;
    std::vector<std::string> arguments;
    bool first_init;
    bool escape_exit;
    long iterations;
    Version version;
};

void PrintTo(const RouterInlineResetInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterInlineResetSources : public TestCommandRouterStartup, public WithParamInterface<RouterInlineResetInput>
{
protected:
    void SetUp() override;
};

void TestRouterInlineResetSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    source("sstools.ini").write("exitnoask=yes maxiter=901 reset=1960 maxiter=111\n");
    source("whole.par").write("entry { exitnoask=yes maxiter=902 reset=100 maxiter=222 }");
    source("selected.par").write("chosen { reset=1/5 exitnoask=yes maxiter=333 }");
    g_first_init = GetParam().first_init;
}

TEST_P(TestRouterInlineResetSources, resetsTakeEffectInline)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup(GetParam().arguments)};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(GetParam().escape_exit, g_escape_exit);
    EXPECT_EQ(GetParam().iterations, g_max_iterations);
    EXPECT_EQ(GetParam().version, g_version);
    EXPECT_EQ(g_version, g_file_version);
}

INSTANTIATE_TEST_SUITE_P(Sources, TestRouterInlineResetSources,
    Values(RouterInlineResetInput{"Configuration", {"id"}, true, false, 111L, parse_legacy_version(1960)},
        RouterInlineResetInput{"WholeFile", {"id", "@whole"}, false, false, 222L, Version{1, 0, 0, 0, false}},
        RouterInlineResetInput{
            "SelectedSet", {"id", "@whole", "@selected/chosen"}, false, true, 333L, Version{1, 5, 0, 0, false}},
        RouterInlineResetInput{"Argument",
            {"id", "@whole", "@selected/chosen", "reset=1/4", "maxiter=444", "exitnoask=yes"}, false, true, 444L,
            Version{1, 4, 0, 0, false}}),
    [](const TestParamInfo<RouterInlineResetInput> &info) { return info.param.name; });

struct RouterLegacyFloatInput
{
    const char *name;
    bool legacy_float;
    const char *argument;
    bool first_init;
};

void PrintTo(const RouterLegacyFloatInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRouterLegacyFloatSources : public TestCommandRouterStartup, public WithParamInterface<RouterLegacyFloatInput>
{
protected:
    void SetUp() override;

    ValueSaver<UserData> m_saved_user{g_user};
    ValueSaver<double> m_saved_param_1{g_params[1]};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
};

void TestRouterLegacyFloatSources::SetUp()
{
    TestCommandRouterStartup::SetUp();
    source("legacy.par").write("entry { reset=1824 type=marksjulia params=0.1/0.9/3 }");
    source("float-only.par").write(GetParam().legacy_float ? "chosen { float=y }" : "chosen { float=n }");
    g_first_init = GetParam().first_init;
}

TEST_P(TestRouterLegacyFloatSources, indirectFloatSettingDoesNotRepeatCompatibilityMigration)
{
    const CommandRouter router;

    const CommandParseResult result{router.process_startup({"id", "@legacy", GetParam().argument})};

    EXPECT_TRUE(result.diagnostics.errors.empty());
    EXPECT_EQ(FractalType::MARKS_JULIA, g_fractal_type);
    EXPECT_DOUBLE_EQ(4.0, g_params[2]);
    EXPECT_EQ(GetParam().legacy_float, g_user.legacy_float);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestRouterLegacyFloatSources,
    Values(RouterLegacyFloatInput{"EnabledWholeFile", true, "@float-only", true},
        RouterLegacyFloatInput{"EnabledSelectedSet", true, "@float-only/chosen", false},
        RouterLegacyFloatInput{"DisabledWholeFile", false, "@float-only", false},
        RouterLegacyFloatInput{"DisabledSelectedSet", false, "@float-only/chosen", false}),
    [](const TestParamInfo<RouterLegacyFloatInput> &info) { return info.param.name; });

class TestStartupCommand : public TestParameterCommand
{
public:
    TestStartupCommand();

private:
    ValueSaver<bool> m_saved_first_init{g_first_init, true};
    const std::shared_ptr<NiceMock<MockSpecialDirectories>> m_directories{
        std::make_shared<NiceMock<MockSpecialDirectories>>()};
    ValueSaver<std::shared_ptr<SpecialDirectories>> m_saved_special_dirs{g_special_dirs, m_directories};
    CurrentPathSaver m_current_directory{ID_TEST_HOME_DIR};
};

TestStartupCommand::TestStartupCommand()
{
    ON_CALL(*m_directories, program_dir()).WillByDefault(Return(fs::path{ID_TEST_HOME_DIR}));
    ON_CALL(*m_directories, documents_dir()).WillByDefault(Return(fs::path{ID_TEST_DATA_DIR}));
}

class TestStartupHeadlessCommand : public TestStartupCommand
{
private:
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
};

class TestStartupTempDirectory : public TestStartupCommand
{
protected:
    fs::path m_temp_directory{ID_TEST_DATA_DIR};

private:
    EnvVarSaver m_saved_tmp{"TMP", nullptr};
    EnvVarSaver m_saved_temp{"TEMP", nullptr};
    EnvVarSaver m_saved_tmp_dir{"TMPDIR", m_temp_directory.make_preferred().string().c_str()};
    ValueSaver<fs::path> m_saved_temp_dir{g_temp_dir, fs::path{}};
};

TEST_F(TestStartupTempDirectory, startupUsesTmpDirWhenTmpAndTempUnset)
{
    const std::vector<std::string> args{"id"};

    const CommandRouter router;
    router.process_startup(args);

    EXPECT_EQ(m_temp_directory.string(), adjust_dir(g_temp_dir.string()));
}

TEST_F(TestStartupCommand, startupPreservesOrbitDelayAfterShowDot)
{
    ValueSaver saved_video_table_len{g_video_table_len, 1};
    ValueSaver saved_video_key{g_video_mode_keys[0], ID_KEY_F6};
    ValueSaver saved_orbit_delay{g_orbit_delay, -999};
    ValueSaver saved_orbit_skip_points{g_orbit_skip_points, -999};
    const std::vector<std::string> args{"id", "video=F6", "type=mandel", "showdot=b/20", "orbitdelay=100"};

    const CommandRouter router;
    router.process_startup(args);

    EXPECT_EQ(100, g_orbit_delay);
    EXPECT_EQ(100, g_orbit_skip_points);
}

TEST_F(TestStartupHeadlessCommand, startupErrorStopsBeforeLaterBatchArgument)
{
    const CommandRouter router;
    const CommandParseResult parsed{router.process_startup({"id", "maxiter=bad", "batch=yes"})};
    const CommandDiagnostics &diagnostics{parsed.diagnostics};

    ASSERT_EQ(1U, diagnostics.errors.size());
    EXPECT_THAT(diagnostics.errors.front(), HasSubstr("maxiter=bad"));
    EXPECT_EQ(BatchMode::NONE, g_init_batch);
}

TEST_F(TestStartupHeadlessCommand, startupCollectsMissingFileWarningsWithoutUi)
{
    const CommandRouter router;
    const CommandParseResult parsed{router.process_startup({"id", "lfile=missing-startup-lsystem.l"})};
    const CommandDiagnostics &diagnostics{parsed.diagnostics};

    EXPECT_TRUE(diagnostics.errors.empty());
    ASSERT_EQ(1U, diagnostics.warnings.size());
    EXPECT_THAT(diagnostics.warnings.front(), HasSubstr("please check command line"));
}

TEST_F(TestStartupHeadlessCommand, startupReturnsFileEntrySelectionWithoutUi)
{
    const CommandRouter router;
    const CommandParseResult parsed{router.process_startup({"id", "type=formula"})};

    EXPECT_TRUE(parsed.needs_file_entry);
    EXPECT_TRUE(parsed.diagnostics.errors.empty());
    EXPECT_TRUE(g_formula_name.empty());
}

TEST_F(TestStartupHeadlessCommand, startupErrorPreventsLaterAction)
{
    const CommandRouter router;
    const CommandParseResult parsed{router.process_startup({"id", "maxiter=bad", "makedoc=unused.txt", "maxiter=888"})};

    EXPECT_EQ(1U, parsed.diagnostics.errors.size());
    EXPECT_EQ(0U, parsed.action.index());
    EXPECT_NE(888, g_max_iterations);
    EXPECT_FALSE(parsed.needs_file_entry);
}

} // namespace id::test
