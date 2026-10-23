// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/cmdfiles.h>

#include <engine/calcfrac.h>
#include <engine/Command.h>
#include <engine/command_comments.h>
#include <engine/sound.h>
#include <fractals/fractalp.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestCommandProcessor : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    std::FILE *open_input(std::string_view text);
    void handle_current(CommandProcessor &processor);
    void prepare_legacy_compatibility();

    fs::path m_path;
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 150L};
    ValueSaver<int> m_saved_hertz{g_base_hertz, 440};
    ValueSaver<double> m_saved_param_0{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_2{g_params[2], 0.0};
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
    ValueSaver<Version> m_saved_file_version{g_file_version, current_id_version()};
    ValueSaver<bool> m_saved_bif_functions{g_new_bifurcation_functions_loaded, true};
};

void TestCommandProcessor::SetUp()
{
    clear_command_comments();
}

void TestCommandProcessor::TearDown()
{
    if (!m_path.empty())
    {
        std::error_code error;
        fs::remove(m_path, error);
    }
    clear_command_comments();
}

std::FILE *TestCommandProcessor::open_input(const std::string_view text)
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

void TestCommandProcessor::handle_current(CommandProcessor &processor)
{
    const Command &cmd{processor.command()};
    if (const std::optional<CmdArgFlags> flags{handle_engine_command(cmd)}; flags.has_value())
    {
        cmd.result.flags = *flags;
    }
    else
    {
        cmd.result.flags = cmd.bad_arg();
    }
}

void TestCommandProcessor::prepare_legacy_compatibility()
{
    set_fractal_type(FractalType::MARKS_JULIA);
    g_file_version = parse_legacy_version(1824);
    g_params[2] = 1.0;
}

TEST_F(TestCommandProcessor, advancesOneCommandAndRetainsItUntilNextAdvance)
{
    CommandProcessor processor{open_input("entry { params=1/2/3 maxiter=222 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    const Command &first{processor.command()};
    const char *value{first.value.data()};
    EXPECT_EQ("params", first.variable);
    EXPECT_EQ("1/2/3", first.value);
    EXPECT_EQ("2", first.string_val(1));
    EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, first.mode);
    EXPECT_DOUBLE_EQ(0.0, g_params[0]);

    handle_current(processor);

    EXPECT_EQ(&first, &processor.command());
    EXPECT_EQ(value, processor.command().value.data());
    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("maxiter", processor.command().variable);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, processor.result().flags);
    EXPECT_EQ(CmdArgFlags::NONE, processor.command().result.flags);
    EXPECT_TRUE(processor.command().result.diagnostics.errors.empty());
    EXPECT_EQ(150, g_max_iterations);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());
    EXPECT_EQ(222, g_max_iterations);
}

TEST_F(TestCommandProcessor, adoptsPositionedSetAndStopsAtItsClosingBrace)
{
    const std::string prefix{"ignored { maxiter=999 }\n"};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{
        open_input(prefix + "entry (description) { maxiter=222 } maxiter=333"), std::fclose};
    ASSERT_EQ(0, std::fseek(file.get(), static_cast<long>(prefix.size()), SEEK_SET));
    CommandProcessor processor{file.release(), CmdFile::AT_AFTER_STARTUP};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, preservesCommentsAndContinuationLines)
{
    g_command_comment[0] = "old comment";
    CommandProcessor processor{
        open_input("entry {\n; First Comment\nmaxiter=2\\\n  22\n; Second Comment\n}\n"), CmdFile::AT_AFTER_STARTUP};
    EXPECT_TRUE(g_command_comment[0].empty());

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("222", processor.command().value);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ("First Comment", g_command_comment[0]);
    EXPECT_EQ("Second Comment", g_command_comment[1]);
}

TEST_F(TestCommandProcessor, configurationFileDoesNotRequireSetBraces)
{
    CommandProcessor processor{open_input("[other]\nmaxiter=999\n[id]\nmaxiter=222\n"), CmdFile::SSTOOLS_INI};

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(CmdFile::SSTOOLS_INI, processor.command().mode);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, selectedSetClearsParameterInitializationFlags)
{
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, cmd_arg("params=77", CmdFile::AT_CMD_LINE).flags);
    CommandProcessor processor{open_input("entry { type=mandel }"), CmdFile::AT_AFTER_STARTUP};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_DOUBLE_EQ(0.0, g_params[0]);
}

TEST_F(TestCommandProcessor, startupSetPreservesEarlierParameterInitializationFlags)
{
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, cmd_arg("params=77", CmdFile::AT_CMD_LINE).flags);
    CommandProcessor processor{open_input("entry { type=mandel }"), CmdFile::AT_CMD_LINE_SET_NAME};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_DOUBLE_EQ(77.0, g_params[0]);
}

TEST_F(TestCommandProcessor, rejectionRetainsDiagnosticsAndAuxiliaryDataButNotBadFlags)
{
    CommandProcessor processor{open_input("entry { first second ignored }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    processor.command().result.flags = CmdArgFlags::FRACTAL_PARAM;
    processor.command().result.diagnostics.errors.push_back("first error");
    processor.command().result.diagnostics.warnings.push_back("first warning");
    ASSERT_TRUE(processor.advance());
    const Command &cmd{processor.command()};
    EXPECT_FALSE(cmd.result.initialize_fm);
    EXPECT_FALSE(cmd.result.needs_file_entry);
    cmd.result.flags = cmd.bad_arg();
    cmd.result.diagnostics.warnings.push_back("second warning");
    cmd.result.initialize_fm = true;
    cmd.result.needs_file_entry = true;
    cmd.result.action = MakeHelpDocument{"output.txt"};

    EXPECT_FALSE(processor.advance());

    const CommandParseResult &result{processor.result()};
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, result.flags);
    EXPECT_THAT(
        result.diagnostics.errors, ElementsAre("first error", "Oops. I couldn't understand the argument:\n  second"));
    EXPECT_THAT(result.diagnostics.warnings, ElementsAre("first warning", "second warning"));
    EXPECT_TRUE(result.initialize_fm);
    EXPECT_FALSE(result.needs_file_entry);
    ASSERT_TRUE(std::holds_alternative<MakeHelpDocument>(result.action));
    EXPECT_EQ("output.txt", std::get<MakeHelpDocument>(result.action).filename);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, mergesAcceptedFlagsAndRetainsEarlierFmInitializationRequest)
{
    CommandProcessor processor{open_input("entry { first second }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    processor.command().result.flags = CmdArgFlags::FRACTAL_PARAM;
    processor.command().result.initialize_fm = true;
    ASSERT_TRUE(processor.advance());
    EXPECT_FALSE(processor.command().result.initialize_fm);
    processor.command().result.flags = CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D;

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D, processor.result().flags);
    EXPECT_TRUE(processor.result().initialize_fm);
}

TEST_F(TestCommandProcessor, rejectionFinalizesValidPrefixAndDoesNotApplyLaterCommands)
{
    prepare_legacy_compatibility();
    CommandProcessor processor{open_input("entry { maxiter=222 unknown=yes maxiter=333 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, processor.result().flags);
    EXPECT_EQ(1U, processor.result().diagnostics.errors.size());
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, rejectedFlagsAloneDoNotTriggerCompatibility)
{
    prepare_legacy_compatibility();
    CommandProcessor processor{open_input("entry { unknown=yes maxiter=333 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    EXPECT_EQ(150, g_max_iterations);
}

TEST_F(TestCommandProcessor, actionStopsAfterEarlierDiagnosticsWithoutStoppingOnThoseDiagnostics)
{
    CommandProcessor processor{open_input("entry { first makemig=2/3 maxiter=333 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    processor.command().result.diagnostics.errors.push_back("earlier error");
    ASSERT_TRUE(processor.advance());
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(150, g_max_iterations);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre("earlier error"));
    const MakeMultiImageGif *action{std::get_if<MakeMultiImageGif>(&processor.result().action)};
    ASSERT_NE(nullptr, action);
    EXPECT_EQ(2U, action->x_mult);
    EXPECT_EQ(3U, action->y_mult);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, successfulCompletionRunsCompatibilityOnlyOnce)
{
    prepare_legacy_compatibility();
    CommandProcessor processor{open_input("entry { maxiter=222 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);

    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_FALSE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, compatibilityUsesOnlyTheCurrentFileFlags)
{
    prepare_legacy_compatibility();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, cmd_arg("maxiter=222", CmdFile::AT_CMD_LINE).flags);
    CommandProcessor processor{open_input("entry { hertz=220 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    EXPECT_EQ(220, g_base_hertz);
}

TEST_F(TestCommandProcessor, missingContinuationReportsFailureAndFinalizesValidPrefix)
{
    prepare_legacy_compatibility();
    CommandProcessor processor{open_input("entry { maxiter=222\nparams=1/\\"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_THAT(
        processor.result().diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  params=1/"));
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, endOfFileWithoutClosingBraceStillCompletesTheFile)
{
    CommandProcessor processor{open_input("entry { maxiter=222\n"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, syntaxFailureIsRetainedWithoutDispatchingLaterCommands)
{
    CommandProcessor processor{
        open_input("entry { " + std::string(33, 'a') + "=1 maxiter=333 }"), CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(CmdArgFlags::BAD_ARG, processor.command().status);
    EXPECT_EQ(CmdArgFlags::BAD_ARG, processor.command().result.flags);

    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    EXPECT_EQ(1U, processor.result().diagnostics.errors.size());
    EXPECT_EQ(150, g_max_iterations);
}

TEST_F(TestCommandProcessor, emptyFileClosesWithoutPresentingACommand)
{
    CommandProcessor processor{open_input(""), CmdFile::AT_AFTER_STARTUP};

    EXPECT_FALSE(processor.advance());
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, abandonmentBetweenCommandsClosesTheFile)
{
    {
        CommandProcessor processor{open_input("entry { maxiter=222 maxiter=333 }"), CmdFile::AT_AFTER_STARTUP};
        ASSERT_TRUE(processor.advance());
    }

    EXPECT_EQ(150, g_max_iterations);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, handlerExceptionClosesTheFileWithoutRollingBackEarlierEffects)
{
    EXPECT_THROW(
        {
            CommandProcessor processor(open_input("entry { maxiter=222 unknown=yes }"), CmdFile::AT_AFTER_STARTUP);
            ASSERT_TRUE(processor.advance());
            handle_current(processor);
            ASSERT_TRUE(processor.advance());
            throw std::runtime_error("handler failure");
        },
        std::runtime_error);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(fs::remove(m_path));
}

TEST_F(TestCommandProcessor, invalidFileOrMissingCurrentCommandThrows)
{
    EXPECT_THROW((CommandProcessor{nullptr, CmdFile::AT_AFTER_STARTUP}), std::logic_error);
    CommandProcessor processor{open_input("entry { maxiter=222 }"), CmdFile::AT_AFTER_STARTUP};
    EXPECT_THROW(processor.command(), std::logic_error);
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_FALSE(processor.advance());
    EXPECT_THROW(processor.command(), std::logic_error);
}

} // namespace id::test
