// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/CommandProcessor.h>

#include <engine/calcfrac.h>
#include <engine/Browse.h>
#include <engine/Command.h>
#include <engine/command_comments.h>
#include <engine/sound.h>
#include <fractals/fractalp.h>
#include <fractals/formula.h>
#include <fractals/ifs.h>
#include <fractals/lsystem.h>
#include <io/library.h>
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
#include <vector>

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

TEST_F(TestCommandProcessor, singleCommandRetainsItsOriginalRejectedResult)
{
    CommandProcessor processor{"unknown=yes", CmdFile::AT_AFTER_STARTUP};
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    processor.command().result.action = MakeHelpDocument{"output.txt"};
    processor.command().result.initialize_fm = true;
    processor.command().result.needs_file_entry = true;

    EXPECT_FALSE(processor.advance());
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(CmdArgFlags::BAD_ARG, processor.result().flags);
    EXPECT_THAT(
        processor.result().diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  unknown=yes"));
    EXPECT_TRUE(std::holds_alternative<MakeHelpDocument>(processor.result().action));
    EXPECT_TRUE(processor.result().initialize_fm);
    EXPECT_TRUE(processor.result().needs_file_entry);
    EXPECT_THROW(processor.command(), std::logic_error);
}

TEST_F(TestCommandProcessor, singleCommandDoesNotResetInitializationFlagsOrComments)
{
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM, cmd_arg("params=77", CmdFile::AT_CMD_LINE).flags);
    g_command_comment[0] = "keep comment";
    std::string text{"type=mandel"};
    CommandProcessor processor{text, CmdFile::AT_AFTER_STARTUP};
    text.clear();

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("mandel", processor.command().value);
    EXPECT_EQ(CmdFile::AT_AFTER_STARTUP, processor.command().mode);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_DOUBLE_EQ(77.0, g_params[0]);
    EXPECT_EQ("keep comment", g_command_comment[0]);
}

TEST_F(TestCommandProcessor, singleCommandDoesNotRunFileCompatibility)
{
    prepare_legacy_compatibility();
    CommandProcessor processor{"maxiter=222", CmdFile::AT_AFTER_STARTUP};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
}

TEST_F(TestCommandProcessor, singleCommandDoesNotResolveMissingEntries)
{
    CommandProcessor processor{"type=formula", CmdFile::AT_AFTER_STARTUP};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_FALSE(processor.result().needs_file_entry);
}

class TestStartupCommandProcessor : public TestCommandProcessor
{
protected:
    void SetUp() override;
    void TearDown() override;
    void write_source(const std::string &name, std::string_view text);
    void run(CommandProcessor &processor);

    fs::path m_directory;
    std::vector<fs::path> m_sources;
    ValueSaver<bool> m_saved_check_cur_dir{g_check_cur_dir, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<ShowFile> m_saved_show_file{g_show_file, ShowFile::IMAGE_LOADED};
    ValueSaver<fs::path> m_saved_read_filename{g_read_filename};
    ValueSaver<Browse> m_saved_browse{g_browse};
    ValueSaver<std::string> m_saved_formula_name{g_formula_name, std::string{}};
    ValueSaver<std::string> m_saved_l_system_name{g_l_system_name, std::string{}};
    ValueSaver<std::string> m_saved_ifs_name{g_ifs_name, std::string{}};
    ValueSaver<fs::path> m_saved_parameter_file{g_parameter_file, fs::path{}};
    ValueSaver<std::string> m_saved_parameter_set_name{g_parameter_set_name, std::string{}};
};

void TestStartupCommandProcessor::SetUp()
{
    TestCommandProcessor::SetUp();
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    m_directory = fs::temp_directory_path() / (std::string{info->test_suite_name()} + '_' + info->name());
    fs::create_directories(m_directory);
    clear_read_library_path();
    add_read_library(m_directory);
    set_fractal_type(FractalType::MANDEL);
    g_first_init = true;
}

void TestStartupCommandProcessor::TearDown()
{
    clear_read_library_path();
    for (const fs::path &path : m_sources)
    {
        std::error_code ignored;
        fs::remove(path, ignored);
    }
    std::error_code ignored;
    fs::remove(m_directory, ignored);
    TestCommandProcessor::TearDown();
}

void TestStartupCommandProcessor::write_source(const std::string &name, const std::string_view text)
{
    const fs::path path{m_directory / name};
    m_sources.push_back(path);
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{std::fopen(path.string().c_str(), "wb"), std::fclose};
    if (!file || std::fwrite(text.data(), 1, text.size(), file.get()) != text.size())
    {
        throw std::runtime_error("Cannot write startup test file: " + path.string());
    }
}

void TestStartupCommandProcessor::run(CommandProcessor &processor)
{
    while (processor.advance())
    {
        handle_current(processor);
    }
}

TEST_F(TestStartupCommandProcessor, ownsArgumentsAndAdvancesFromConfigurationInOrder)
{
    write_source("sstools.ini", "[id]\nmaxiter=111\nhertz=330\n");
    std::vector<std::string> args{"id", "maxiter=222", "hertz=660"};
    g_max_iterations = 432;
    CommandProcessor processor{args};
    args.clear();

    EXPECT_EQ(432, g_max_iterations);
    ASSERT_TRUE(processor.advance());
    const Command &first{processor.command()};
    EXPECT_EQ(CmdFile::SSTOOLS_INI, first.mode);
    EXPECT_EQ("111", first.value);
    handle_current(processor);
    EXPECT_EQ(&first, &processor.command());
    EXPECT_EQ("111", first.value);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("hertz", processor.command().variable);
    EXPECT_EQ(CmdFile::SSTOOLS_INI, processor.command().mode);
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(330, g_base_hertz);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, processor.result().flags);
    EXPECT_EQ(CmdFile::AT_CMD_LINE, processor.command().mode);
    EXPECT_EQ("222", processor.command().value);
    EXPECT_TRUE(fs::remove(m_directory / "sstools.ini"));
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ("660", processor.command().value);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());
    EXPECT_EQ(660, g_base_hertz);
    EXPECT_THROW(processor.command(), std::logic_error);
    EXPECT_FALSE(processor.advance());
}

TEST_F(TestStartupCommandProcessor, rejectedConfigurationStopsItsFileButArgumentsContinueAfterErrors)
{
    write_source("sstools.ini", "maxiter=111\nunknown=yes\nhertz=999\n");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=bad", "batch=yes", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_EQ(CmdFile::AT_CMD_LINE, processor.command().mode);
    EXPECT_EQ("bad", processor.command().value);
    EXPECT_TRUE(fs::remove(m_directory / "sstools.ini"));
    handle_current(processor);
    run(processor);

    EXPECT_EQ(BatchMode::NORMAL, g_init_batch);
    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, processor.result().flags);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("unknown=yes"), HasSubstr("maxiter=bad")));
}

TEST_F(TestStartupCommandProcessor, configurationWarningsPermitLaterCommandsAndArguments)
{
    write_source("sstools.ini", "lfile=missing-startup.l\nmaxiter=111\n");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222"}};

    run(processor);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_THAT(processor.result().diagnostics.warnings, ElementsAre(HasSubstr("please check sstools.ini")));
}

TEST_F(TestStartupCommandProcessor, configurationActionStopsRemainingConfigurationAndArguments)
{
    write_source("sstools.ini", "hertz=330\nmakedoc=unused.txt\nmaxiter=999\n");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222"}};

    run(processor);

    EXPECT_EQ(330, g_base_hertz);
    EXPECT_EQ(150, g_max_iterations);
    EXPECT_TRUE(std::holds_alternative<MakeHelpDocument>(processor.result().action));
    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_TRUE(fs::remove(m_directory / "sstools.ini"));
}

TEST_F(TestStartupCommandProcessor, earlierArgumentErrorsDoNotPreventActionTermination)
{
    CommandProcessor processor{
        std::vector<std::string>{"id", "type=formula", "maxiter=bad", "makedoc=unused.txt", "maxiter=999"}};

    run(processor);

    EXPECT_EQ(150, g_max_iterations);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("maxiter=bad")));
    EXPECT_TRUE(std::holds_alternative<MakeHelpDocument>(processor.result().action));
    EXPECT_FALSE(processor.result().needs_file_entry);
}

TEST_F(TestStartupCommandProcessor, bareGifArgumentIsProbedBetweenParameterEffects)
{
    write_source("preview.gif", "GIF89a");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=111", "preview.gif", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(ShowFile::IMAGE_LOADED, g_show_file);
    handle_current(processor);
    ASSERT_TRUE(processor.advance());

    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ("maxiter", processor.command().variable);
    EXPECT_EQ("222", processor.command().value);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
    EXPECT_EQ(m_directory / "preview.gif", g_read_filename);
    EXPECT_EQ(g_read_filename, g_browse.selected_path);
    EXPECT_EQ("preview.gif", g_browse.name);
    EXPECT_TRUE(fs::remove(m_directory / "preview.gif"));
    handle_current(processor);
    EXPECT_FALSE(processor.advance());
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, nonGifArgumentIsExposedAndRejectionDoesNotStopLaterArguments)
{
    write_source("preview.dat", "NOTGIF");
    CommandProcessor processor{std::vector<std::string>{"id", "preview.dat", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("preview.dat", processor.command().variable);
    handle_current(processor);
    run(processor);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("preview.dat")));
    EXPECT_EQ(ShowFile::IMAGE_LOADED, g_show_file);
}

TEST_F(TestStartupCommandProcessor, commentArgumentEndsTheSourcePermanently)
{
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222", "; comment", "maxiter=999"}};

    run(processor);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_FALSE(processor.advance());
    EXPECT_THROW(processor.command(), std::logic_error);
}

TEST_F(TestStartupCommandProcessor, finalTypeDeterminesTheEntrySelectionRequest)
{
    CommandProcessor processor{std::vector<std::string>{"id", "type=formula", "type=mandel"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_FALSE(processor.result().needs_file_entry);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, laterNameSatisfiesTheFinalEntrySelection)
{
    CommandProcessor processor{std::vector<std::string>{"id", "type=formula", "formulaname=example"}};

    run(processor);

    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, missingEntryRequestsSelectionOnlyAtCompletion)
{
    CommandProcessor processor{std::vector<std::string>{"id", "type=formula"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_FALSE(processor.advance());

    EXPECT_TRUE(processor.result().needs_file_entry);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, rejectedArgumentSuppressesEntrySelection)
{
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=bad", "type=formula"}};

    run(processor);

    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("maxiter=bad")));
}

TEST_F(TestStartupCommandProcessor, finalBatchSettingReportsMissingEntryOnlyOnce)
{
    CommandProcessor processor{std::vector<std::string>{"id", "type=formula", "batch=yes"}};

    run(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("type=formula requires formulaname=")));
}

TEST_F(TestStartupCommandProcessor, perCommandEntryRequestIsNotAggregated)
{
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    processor.command().result.needs_file_entry = true;
    EXPECT_FALSE(processor.advance());

    EXPECT_FALSE(processor.result().needs_file_entry);
}

TEST_F(TestStartupCommandProcessor, configurationCompatibilityFinishesBeforeArgumentsAndIsNotRepeated)
{
    prepare_legacy_compatibility();
    write_source("sstools.ini", "maxiter=111\n");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    ASSERT_TRUE(processor.advance());
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
}

TEST_F(TestStartupCommandProcessor, argumentFlagsDoNotTriggerFileCompatibility)
{
    prepare_legacy_compatibility();
    write_source("sstools.ini", "hertz=330\n");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=222"}};

    run(processor);

    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, processor.result().flags);
}

TEST_F(TestStartupCommandProcessor, indirectFilesExposeCommandsInSourceOrder)
{
    write_source("whole.par", "entry { maxiter=111 }");
    write_source("selected.par", "ignored { hertz=999 }\nchosen { hertz=330 }\ntrailing { hertz=888 }");
    CommandProcessor processor{std::vector<std::string>{"id", "@whole", "@selected/chosen", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(150, g_max_iterations);
    EXPECT_EQ("111", processor.command().value);
    EXPECT_EQ(CmdFile::AT_CMD_LINE_SET_NAME, processor.command().mode);
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_EQ("330", processor.command().value);
    EXPECT_EQ(CmdFile::AT_CMD_LINE_SET_NAME, processor.command().mode);
    EXPECT_EQ(m_directory / "selected.par", g_parameter_file);
    EXPECT_EQ("chosen", g_parameter_set_name);
    EXPECT_TRUE(fs::remove(m_directory / "whole.par"));
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(330, g_base_hertz);
    EXPECT_EQ("222", processor.command().value);
    EXPECT_EQ(CmdFile::AT_CMD_LINE, processor.command().mode);
    EXPECT_TRUE(fs::remove(m_directory / "selected.par"));
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, indirectLookupUsesEarlierLibrarySettings)
{
    const fs::path library{m_directory / "late-library"};
    fs::create_directory(library);
    write_source("late-library/late-choice.par", "chosen { maxiter=222 }");
    m_sources.push_back(library);
    CommandProcessor processor{
        std::vector<std::string>{"id", "librarydirs=" + library.string(), "@late-choice/chosen"}};

    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("librarydirs", processor.command().variable);
    EXPECT_TRUE(find_file(ReadFile::PARAMETER, "late-choice.par").empty());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("222", processor.command().value);
    EXPECT_EQ(150, g_max_iterations);
    EXPECT_EQ(library / "late-choice.par", g_parameter_file);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, failedIndirectLookupsPreserveSelectionAndContinueArguments)
{
    write_source("available.par", "present { hertz=999 }");
    g_parameter_file = m_directory / "previous.par";
    g_parameter_set_name = "previous";
    CommandProcessor processor{std::vector<std::string>{
        "id", "maxiter=111", "@missing-indirect-parameter-file", "@available/missing", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());

    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ("222", processor.command().value);
    EXPECT_EQ(m_directory / "previous.par", g_parameter_file);
    EXPECT_EQ("previous", g_parameter_set_name);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_THAT(processor.result().diagnostics.errors,
        ElementsAre(HasSubstr("@missing-indirect-parameter-file"), HasSubstr("'missing' file entry item not found"),
            HasSubstr("@available/missing")));
    EXPECT_TRUE(fs::remove(m_directory / "available.par"));
    handle_current(processor);
    EXPECT_FALSE(processor.advance());
    EXPECT_EQ(222, g_max_iterations);
}

TEST_F(TestStartupCommandProcessor, rejectedIndirectCommandStopsOnlyItsFile)
{
    write_source("rejected.par", "entry { maxiter=111 unknown=yes hertz=999 }");
    write_source("following.par", "entry { hertz=330 }");
    CommandProcessor processor{std::vector<std::string>{"id", "@rejected", "@following", "maxiter=222"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("unknown", processor.command().variable);
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("330", processor.command().value);
    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("unknown=yes")));
    EXPECT_TRUE(fs::remove(m_directory / "rejected.par"));
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ(330, g_base_hertz);
    EXPECT_EQ("222", processor.command().value);
    EXPECT_TRUE(fs::remove(m_directory / "following.par"));
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, processor.result().flags);
}

TEST_F(TestStartupCommandProcessor, indirectActionStopsItsFileAndRemainingStartupInputAfterEarlierErrors)
{
    write_source("action.par", "entry { maxiter=111 makemig=2/3 maxiter=999 }");
    write_source("following.par", "entry { hertz=999 }");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=bad", "@action", "@following", "maxiter=222"}};

    run(processor);

    EXPECT_EQ(111, g_max_iterations);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("maxiter=bad")));
    const MakeMultiImageGif *action{std::get_if<MakeMultiImageGif>(&processor.result().action)};
    ASSERT_NE(nullptr, action);
    EXPECT_EQ(2U, action->x_mult);
    EXPECT_EQ(3U, action->y_mult);
    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_TRUE(fs::remove(m_directory / "action.par"));
}

TEST_F(TestStartupCommandProcessor, indirectCompatibilityUsesEachFilesFlagsAndFinishesBeforeFollowingArguments)
{
    prepare_legacy_compatibility();
    write_source("unchanged.par", "entry { hertz=330 }");
    write_source("changed.par", "entry { maxiter=222 }");
    CommandProcessor processor{std::vector<std::string>{"id", "maxiter=111", "@unchanged", "@changed", "maxiter=333"}};

    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_DOUBLE_EQ(1.0, g_params[2]);
    handle_current(processor);
    ASSERT_TRUE(processor.advance());
    EXPECT_EQ("333", processor.command().value);
    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    handle_current(processor);
    EXPECT_FALSE(processor.advance());

    EXPECT_DOUBLE_EQ(2.0, g_params[2]);
    EXPECT_EQ(333, g_max_iterations);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
}

TEST_F(TestStartupCommandProcessor, indirectCommandsDoNotExpandNestedIncludes)
{
    write_source("outer.par", "entry { maxiter=111 @nested hertz=999 }");
    write_source("nested.par", "entry { hertz=330 }");
    CommandProcessor processor{std::vector<std::string>{"id", "@outer", "maxiter=222"}};

    run(processor);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(440, g_base_hertz);
    EXPECT_THAT(processor.result().diagnostics.errors, ElementsAre(HasSubstr("@nested")));
}

TEST_F(TestStartupCommandProcessor, abandonmentBetweenIndirectCommandsClosesTheFile)
{
    write_source("abandoned.par", "entry { maxiter=111 maxiter=999 }");
    {
        CommandProcessor processor{std::vector<std::string>{"id", "@abandoned"}};
        ASSERT_TRUE(processor.advance());
        handle_current(processor);
        ASSERT_TRUE(processor.advance());
    }

    EXPECT_EQ(111, g_max_iterations);
    EXPECT_TRUE(fs::remove(m_directory / "abandoned.par"));
}

TEST_F(TestStartupCommandProcessor, finalIndirectFileCompletesWithoutChangingTheSelectedSet)
{
    write_source("final.par", "entry { maxiter=222 hertz=330 }");
    g_parameter_file = m_directory / "previous.par";
    g_parameter_set_name = "previous";
    CommandProcessor processor{std::vector<std::string>{"id", "@final"}};

    run(processor);

    EXPECT_EQ(222, g_max_iterations);
    EXPECT_EQ(330, g_base_hertz);
    EXPECT_EQ(m_directory / "previous.par", g_parameter_file);
    EXPECT_EQ("previous", g_parameter_set_name);
    EXPECT_FALSE(processor.advance());
    EXPECT_THROW(processor.command(), std::logic_error);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_TRUE(fs::remove(m_directory / "final.par"));
}

TEST_F(TestStartupCommandProcessor, noSourcesCompletesWithoutPresentingACommand)
{
    CommandProcessor processor{std::vector<std::string>{}};

    EXPECT_FALSE(processor.advance());
    EXPECT_FALSE(processor.advance());
    EXPECT_EQ(CmdArgFlags::NONE, processor.result().flags);
    EXPECT_TRUE(processor.result().diagnostics.errors.empty());
    EXPECT_FALSE(processor.result().needs_file_entry);
    EXPECT_THROW(processor.command(), std::logic_error);
}

} // namespace id::test
