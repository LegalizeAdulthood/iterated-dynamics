// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/cmdfiles.h>
#include <engine/command_comments.h>
#include <engine/CommandProcessor.h>
#include <ui/CommandRouter.h>
#include <ui/command_effects.h>

#include "ColorMapSaver.h"
#include "fractals/formula.h"
#include "MockDriver.h"
#include "test_data.h"
#include "test_library.h"
#include "ValueUnchanged.h"

#include <config/path_limits.h>

#include <engine/bailout_formula.h>
#include <engine/Browse.h>
#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/color_utils.h>
#include <engine/convert_center_mag.h>
#include <engine/engine_timer.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/log_map.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/Potential.h>
#include <engine/random_seed.h>
#include <engine/show_dot.h>
#include <engine/SynchronousOrbit.h>
#include <engine/SolidGuess.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/StickyOrbits.h>
#include <engine/text_color.h>
#include <engine/trig_fns.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/ifs.h>
#include <fractals/julibrot.h>
#include <fractals/lambda_fn.h>
#include <fractals/lorenz.h>
#include <fractals/lsystem.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <geometry/stereo_params.h>
#include <io/check_write_file.h>
#include <io/CurrentPathSaver.h>
#include <io/encoder.h>
#include <io/gif_file.h>
#include <io/gifview.h>
#include <io/GifWriter.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/loadmap.h>
#include <io/save_timer.h>
#include <io/special_dirs.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/big_while_loop.h>
#include <ui/comments.h>
#include <ui/id_keys.h>
#include <ui/make_batch_file.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gif_lib.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

using namespace id;
using namespace id::config;
using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::data;
using namespace id::ui;
using namespace testing;

namespace id::engine
{

TEST(TestUserData, perturbationDefaultAuto)
{
    EXPECT_EQ(PerturbationMode::AUTO, UserData{}.perturbation);
}

TEST(TestUserData, perturbationToleranceDefault)
{
    EXPECT_DOUBLE_EQ(DEFAULT_PERTURBATION_TOLERANCE, UserData{}.perturbation_tolerance);
}

static std::ostream &operator<<(std::ostream &str, const CmdArgFlags value)
{
    if (value == CmdArgFlags::BAD_ARG)
    {
        return str << "BAD_ARG";
    }
    if (value == CmdArgFlags::NONE)
    {
        return str << "NONE";
    }

    bool emitted{false};
    if (bit_set(value, CmdArgFlags::FRACTAL_PARAM))
    {
        str << "FRACTAL_PARAM";
        emitted = true;
    }
    if (bit_set(value, CmdArgFlags::PARAM_3D))
    {
        if (emitted)
        {
            str << " | ";
        }
        str << "PARAM_3D";
        emitted = true;
    }
    if (bit_set(value, CmdArgFlags::YES_3D))
    {
        if (emitted)
        {
            str << " | ";
        }
        str << "YES_3D";
        emitted = true;
    }
    if (bit_set(value, CmdArgFlags::RESET))
    {
        if (emitted)
        {
            str << " | ";
        }
        str << "RESET";
        emitted = true;
    }
    return str;
}

} // namespace id::engine

namespace id::test
{

class TestParameterCommand : public Test
{
public:
    ~TestParameterCommand() override = default;

protected:
    void SetUp() override;
    void TearDown() override;
    void exec_cmd_arg(const std::string &cur_arg, CmdFile mode = CmdFile::AT_AFTER_STARTUP);

    CmdArgFlags m_result{};
    CommandParseResult m_parsed;
};

void TestParameterCommand::SetUp()
{
    Test::SetUp();
    clear_read_library_path();
    add_read_library(ID_TEST_HOME_DIR);
}

void TestParameterCommand::TearDown()
{
    clear_read_library_path();
    Test::TearDown();
}

void TestParameterCommand::exec_cmd_arg(const std::string &cur_arg, const CmdFile mode)
{
    CommandProcessor processor{cur_arg, mode};
    const CommandRouter router;
    m_parsed = router.process(processor);
    m_result = m_parsed.flags;
}

class TestNextCommand : public Test
{
protected:
    void SetUp() override
    {
        clear_command_comments();
    }

    void TearDown() override
    {
        if (m_file != nullptr)
        {
            std::fclose(m_file);
            m_file = nullptr;
        }
        std::error_code ignored;
        fs::remove(m_input_path, ignored);
        clear_command_comments();
    }

    void open_input(const std::string &text)
    {
        m_input_path = test_input_path();
        ASSERT_FALSE(m_input_path.empty());
        m_file = std::fopen(m_input_path.string().c_str(), "w+b");
        ASSERT_NE(nullptr, m_file);
        ASSERT_EQ(text.size(), std::fwrite(text.data(), 1, text.size(), m_file));
        std::rewind(m_file);
    }

    int read_next(const CmdFile mode = CmdFile::AT_CMD_LINE)
    {
        return next_command(m_command, m_file, m_line_buf, m_line, mode, &m_diagnostics);
    }

    std::string command() const
    {
        return m_command;
    }

    fs::path test_input_path() const
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        std::string name{info->test_suite_name()};
        name += '_';
        name += info->name();
        std::replace_if(name.begin(), name.end(), [](const unsigned char ch) { return std::isalnum(ch) == 0; }, '_');
        return fs::temp_directory_path() / (name + ".tmp");
    }

    CommandDiagnostics m_diagnostics;
    std::string m_command;
    std::string m_line_buf;
    std::string_view m_line;
    fs::path m_input_path;
    std::FILE *m_file{};
};

TEST_F(TestNextCommand, readsCommandsAcrossLines)
{
    open_input("  alpha beta\r\n\tgamma\n");

    EXPECT_EQ(5, read_next());
    EXPECT_EQ("alpha", command());
    EXPECT_EQ(4, read_next());
    EXPECT_EQ("beta", command());
    EXPECT_EQ(5, read_next());
    EXPECT_EQ("gamma", command());
    EXPECT_EQ(-1, read_next());
}

TEST_F(TestNextCommand, emptyInputReturnsEof)
{
    open_input("");

    EXPECT_EQ(-1, read_next());
}

TEST_F(TestNextCommand, semicolonStartsComment)
{
    open_input("alpha ; ignored\nbeta\n");

    EXPECT_EQ(5, read_next());
    EXPECT_EQ("alpha", command());
    EXPECT_EQ(4, read_next());
    EXPECT_EQ("beta", command());
    EXPECT_EQ(-1, read_next());
}

TEST_F(TestNextCommand, savesTrimmedCommentsForCommandSet)
{
    open_input("; first\n; \tsecond\n}\n");

    EXPECT_EQ(1, read_next(CmdFile::AT_CMD_LINE_SET_NAME));
    EXPECT_EQ("}", command());
    EXPECT_EQ("first", g_command_comment[0]);
    EXPECT_EQ("second", g_command_comment[1]);
}

TEST_F(TestNextCommand, savesOnlyFourComments)
{
    open_input("; one\n; two\n; three\n; four\n; five\n}\n");

    EXPECT_EQ(1, read_next(CmdFile::AT_CMD_LINE_SET_NAME));
    EXPECT_EQ("}", command());
    EXPECT_THAT(g_command_comment, ElementsAre("one", "two", "three", "four"));
}

TEST_F(TestNextCommand, ignoresCommentsForCommandLine)
{
    open_input("; first\nalpha\n");

    EXPECT_EQ(5, read_next());
    EXPECT_EQ("alpha", command());
    EXPECT_EQ("", g_command_comment[0]);
}

TEST_F(TestNextCommand, truncatesSavedCommentToParCommentLength)
{
    const std::string comment(MAX_COMMENT_LEN + 10, 'x');
    open_input("; " + comment + "\n}\n");

    EXPECT_EQ(1, read_next(CmdFile::AT_AFTER_STARTUP));
    EXPECT_EQ("}", command());
    EXPECT_EQ(std::string(MAX_COMMENT_LEN - 1, 'x'), g_command_comment[0]);
}

TEST_F(TestNextCommand, joinsContinuationLine)
{
    open_input("alpha\\\n    beta gamma\n");

    EXPECT_EQ(9, read_next());
    EXPECT_EQ("alphabeta", command());
    EXPECT_EQ(5, read_next());
    EXPECT_EQ("gamma", command());
}

TEST_F(TestNextCommand, sstoolsSkipsNonIdSections)
{
    open_input("[other]\nfoo\n[ID]\nbar\n");

    EXPECT_EQ(3, read_next(CmdFile::SSTOOLS_INI));
    EXPECT_EQ("bar", command());
}

TEST_F(TestNextCommand, returnsErrorForMissingContinuationLine)
{
    open_input("alpha\\");

    EXPECT_EQ(-1, read_next());
    ASSERT_EQ(1U, m_diagnostics.errors.size());
    EXPECT_THAT(m_diagnostics.errors.front(), HasSubstr("alpha"));
}

static fs::path home_file(const char *subdir, const char *filename)
{
    return fs::path{ID_TEST_HOME_DIR} / subdir / filename;
}

class EnvVarSaver
{
public:
    EnvVarSaver(const char *name, const char *value) :
        m_name{name}
    {
        if (const char *old_value = std::getenv(name); old_value != nullptr)
        {
            m_had_value = true;
            m_value = old_value;
        }
        set(value);
    }

    ~EnvVarSaver()
    {
        set(m_had_value ? m_value.c_str() : nullptr);
    }

private:
    void set(const char *value) const
    {
#ifdef _WIN32
        _putenv_s(m_name.c_str(), value == nullptr ? "" : value);
#else
        if (value == nullptr)
        {
            unsetenv(m_name.c_str());
        }
        else
        {
            setenv(m_name.c_str(), value, 1);
        }
#endif
    }

    std::string m_name;
    bool m_had_value{};
    std::string m_value;
};

class TestSpecialDirectories : public SpecialDirectories
{
public:
    TestSpecialDirectories(fs::path program_dir, fs::path documents_dir) :
        m_program_dir{std::move(program_dir)},
        m_documents_dir{std::move(documents_dir)}
    {
    }

    fs::path program_dir() const override
    {
        return m_program_dir;
    }

    fs::path documents_dir() const override
    {
        return m_documents_dir;
    }

private:
    fs::path m_program_dir;
    fs::path m_documents_dir;
};

class TestParameterCommandError : public TestParameterCommand
{
protected:
    void TearDown() override;
};

void TestParameterCommandError::TearDown()
{
    EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
    EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    TestParameterCommand::TearDown();
}

TEST_F(TestParameterCommand, invalidArgumentReturnsDiagnosticWithoutUiOrBatchMutation)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    ValueSaver saved_batch{g_init_batch, BatchMode::NORMAL};

    exec_cmd_arg("maxiter=bad");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    ASSERT_EQ(1U, m_parsed.diagnostics.errors.size());
    EXPECT_THAT(m_parsed.diagnostics.errors.front(), HasSubstr("maxiter=bad"));
    EXPECT_EQ(BatchMode::NORMAL, g_init_batch);
}

TEST_F(TestParameterCommand, missingFileReturnsWarningWithoutUi)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    const fs::path original{g_l_system_filename};
    ValueSaver saved_filename{g_l_system_filename};

    exec_cmd_arg("lfile=missing-command-lsystem.l");

    EXPECT_TRUE(m_parsed.diagnostics.errors.empty());
    ASSERT_EQ(1U, m_parsed.diagnostics.warnings.size());
    EXPECT_EQ(
        "Can't find lfile=missing-command-lsystem.l, please check PAR file", m_parsed.diagnostics.warnings.front());
    EXPECT_EQ(original, g_l_system_filename);
}

TEST_F(TestParameterCommandError, parameterTooLong)
{
    exec_cmd_arg("maximumoftwentycharactersinparametername", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
}

struct BatchParameterInput
{
    const char *name;
    const char *argument;
    CmdFile source;
    BatchMode initial;
    BatchMode expected;
    CmdArgFlags result;
    std::size_t errors;
};

void PrintTo(const BatchParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestBatchParameter : public TestParameterCommand, public WithParamInterface<BatchParameterInput>
{
protected:
    void SetUp() override;

    ValueSaver<BatchMode> m_saved_batch{g_init_batch};
};

void TestBatchParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_init_batch = GetParam().initial;
}

TEST_P(TestBatchParameter, commandPreservesExpectedBatchOutcome)
{
    const BatchParameterInput &input{GetParam()};

    exec_cmd_arg(input.argument, input.source);

    EXPECT_EQ(input.result, m_result);
    EXPECT_EQ(input.expected, g_init_batch);
    EXPECT_EQ(input.errors, m_parsed.diagnostics.errors.size());
    EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBatchParameter,
    Values(BatchParameterInput{"Malformed", "batch=g", CmdFile::SSTOOLS_INI, BatchMode::NONE, BatchMode::NONE,
               CmdArgFlags::BAD_ARG, 1},
        BatchParameterInput{"Yes", "batch=yes", CmdFile::SSTOOLS_INI, BatchMode::NONE, BatchMode::NORMAL,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, 0},
        BatchParameterInput{"No", "batch=no", CmdFile::SSTOOLS_INI, BatchMode::NORMAL, BatchMode::NONE,
            CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, 0},
        BatchParameterInput{"AfterStartup", "batch=yes", CmdFile::AT_AFTER_STARTUP, BatchMode::NONE, BatchMode::NONE,
            CmdArgFlags::BAD_ARG, 1}),
    [](const TestParamInfo<BatchParameterInput> &info) { return info.param.name; });

struct HelpDocumentParameterInput
{
    const char *name;
    const char *argument;
    const char *filename;
};

void PrintTo(const HelpDocumentParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestHelpDocumentParameter : public TestParameterCommand, public WithParamInterface<HelpDocumentParameterInput>
{
protected:
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
};

TEST_P(TestHelpDocumentParameter, commandSelectsOutputFilename)
{
    exec_cmd_arg(GetParam().argument, CmdFile::SSTOOLS_INI);

    const MakeHelpDocument *request{std::get_if<MakeHelpDocument>(&m_parsed.action)};
    ASSERT_NE(nullptr, request);
    EXPECT_EQ(GetParam().filename, request->filename);
}

INSTANTIATE_TEST_SUITE_P(Filenames, TestHelpDocumentParameter,
    Values(HelpDocumentParameterInput{"Default", "makedoc", "id.txt"},
        HelpDocumentParameterInput{"Custom", "makedoc=foo.txt", "foo.txt"}),
    [](const TestParamInfo<HelpDocumentParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, makeMigReturnsDimensionsWithoutRunningIo)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};

    exec_cmd_arg("makemig=2/3");

    const MakeMultiImageGif *request{std::get_if<MakeMultiImageGif>(&m_parsed.action)};
    ASSERT_NE(nullptr, request);
    EXPECT_EQ(2U, request->x_mult);
    EXPECT_EQ(3U, request->y_mult);
}

TEST_F(TestParameterCommand, soundFmRequestsDeviceInitializationWithoutIo)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    ValueSaver saved_sound{g_sound_flag};

    exec_cmd_arg("sound=x/f");

    EXPECT_TRUE(m_parsed.initialize_fm);
    EXPECT_NE(0, g_sound_flag & SOUNDFLAG_OPL3_FM);
}

TEST_F(TestParameterCommand, makeParOnlyReportsOutputSettings)
{
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    ValueSaver saved_path{g_parameter_file, fs::path{"original.par"}};
    ValueSaver saved_name{g_parameter_set_name, std::string{"original"}};
    ValueSaver saved_make_par{g_make_parameter_file, false};

    exec_cmd_arg("makepar=result/entry", CmdFile::SSTOOLS_INI);

    const MakeParameterFile *request{std::get_if<MakeParameterFile>(&m_parsed.action)};
    ASSERT_NE(nullptr, request);
    EXPECT_EQ("result.par", request->filename);
    EXPECT_EQ("entry", request->entry_name);
    EXPECT_EQ("original.par", g_parameter_file);
    EXPECT_EQ("original", g_parameter_set_name);
    EXPECT_FALSE(g_make_parameter_file);
}

struct ParameterArgumentInput
{
    const char *name;
    const char *argument;
};

void PrintTo(const ParameterArgumentInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMakeParArity : public TestParameterCommandError, public WithParamInterface<ParameterArgumentInput>
{
};

TEST_P(TestMakeParArity, rejectsInvalidArgumentCount)
{
    exec_cmd_arg(GetParam().argument, CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
}

INSTANTIATE_TEST_SUITE_P(Arity, TestMakeParArity,
    Values(ParameterArgumentInput{"TooFew", "makepar"}, ParameterArgumentInput{"TooMany", "makepar=foo/bar/fmeh"}),
    [](const TestParamInfo<ParameterArgumentInput> &info) { return info.param.name; });

static std::string read_file_contents(const fs::path &path)
{
    const std::ifstream file{path};
    std::ostringstream str;
    str << file.rdbuf();
    return str.str();
}

struct TestCommandMakePar : TestParameterCommand
{
    ~TestCommandMakePar() override = default;

protected:
    void SetUp() override;
    void exec_cmd_arg(const std::string &arg, CmdFile mode);

    ValueSaver<Version> m_saved_version{g_version, Version{1, 2, 3, 4, false}};
    ValueSaver<fs::path> m_saved_save_dir{g_save_dir, ID_TEST_DATA_DIR};
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<const FractalSpecific *> m_saved_cur_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<int> m_saved_x_dots{g_file_x_dots, 800};
    ValueSaver<int> m_saved_y_dots{g_file_y_dots, 600};
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

void TestCommandMakePar::SetUp()
{
    TestParameterCommand::SetUp();
    EXPECT_CALL(m_driver, stack_screen());
    EXPECT_CALL(m_driver, unstack_screen());
    EXPECT_CALL(m_driver, check_memory());
}

void TestCommandMakePar::exec_cmd_arg(const std::string &arg, const CmdFile mode)
{
    TestParameterCommand::exec_cmd_arg(arg, mode);
    EXPECT_TRUE(std::holds_alternative<MakeParameterFile>(m_parsed.action));
    EXPECT_TRUE(apply_command_effects(m_parsed));
    EXPECT_TRUE(m_parsed.diagnostics.errors.empty());
}

static fs::path par_path(const char *filename)
{
    return fs::path{ID_TEST_DATA_DIR} / ID_TEST_PAR_SUBDIR / filename;
}

static std::string parameter_test_name()
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{std::string{info->test_suite_name()} + '_' + info->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    std::transform(name.begin(), name.end(), name.begin(),
        [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return name;
}

class TestGeneratedParameterFile : public TestCommandMakePar
{
protected:
    void SetUp() override;
    void TearDown() override;
    void write_initial_contents(std::string_view contents);

    fs::path m_output;
};

void TestGeneratedParameterFile::SetUp()
{
    TestCommandMakePar::SetUp();
    const std::string filename{"generated-" + parameter_test_name() + ".par"};
    m_output = par_path(filename.c_str());
    fs::create_directories(m_output.parent_path());
    fs::remove(m_output);
}

void TestGeneratedParameterFile::TearDown()
{
    if (!m_output.empty())
    {
        std::error_code error;
        fs::remove(m_output, error);
        EXPECT_FALSE(error) << m_output;
    }
    TestCommandMakePar::TearDown();
}

void TestGeneratedParameterFile::write_initial_contents(const std::string_view contents)
{
    std::ofstream file{m_output};
    ASSERT_TRUE(file);
    file << contents;
}

TEST_F(TestGeneratedParameterFile, makeParNewFile)
{
    const fs::path &path{m_output};

    exec_cmd_arg("makepar=" + path.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(R"par(bar                {
                     ; Id Version 1.2.3.4
  reset=1/2/3/4 type=mandel passes= corners=0/0/0/0 params=0/0
  maxiter=0 fillcolor=0 outside=0 biomorph=0 symmetry=none
  periodicity=0 cyclerange=0/0 hertz=0 sound=off volume=0 attack=0
  decay=0 sustain=0 srelease=0 orbitinterval=0
}

)par",
        read_file_contents(path))
        << path;
}

struct SavedParameterSeedInput
{
    const char *name;
    bool fixed;
};

void PrintTo(const SavedParameterSeedInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSavedParameterSeed : public TestGeneratedParameterFile, public WithParamInterface<SavedParameterSeedInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_seed{g_random_seed, 4567};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag};
};

void TestSavedParameterSeed::SetUp()
{
    TestGeneratedParameterFile::SetUp();
    g_random_seed_flag = GetParam().fixed;
}

TEST_P(TestSavedParameterSeed, savesOnlyFixedRandomSeeds)
{
    exec_cmd_arg("makepar=" + m_output.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    const std::string text{read_file_contents(m_output)};
    if (GetParam().fixed)
    {
        EXPECT_THAT(text, HasSubstr(" rseed=4567"));
    }
    else
    {
        EXPECT_THAT(text, Not(HasSubstr("rseed=")));
    }
}

INSTANTIATE_TEST_SUITE_P(Seeds, TestSavedParameterSeed,
    Values(SavedParameterSeedInput{"Fixed", true}, SavedParameterSeedInput{"Generated", false}),
    [](const TestParamInfo<SavedParameterSeedInput> &info) { return info.param.name; });

TEST_F(TestGeneratedParameterFile, makeParNewEntryExistingFile)
{
    const fs::path &path{m_output};
    write_initial_contents(R"par(bar                {
  reset=1/2/3/4 type=mandel passes= corners=0/0/0/0 params=0/0
  maxiter=0 fillcolor=0 inside=0 outside=0 biomorph=0 symmetry=none
  periodicity=0 cyclerange=0/0 hertz=0 sound=off volume=0 attack=0
  decay=0 sustain=0 srelease=0 orbitinterval=0
}

)par");

    exec_cmd_arg("makepar=" + path.filename().string() + "/foo", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(R"par(bar                {
  reset=1/2/3/4 type=mandel passes= corners=0/0/0/0 params=0/0
  maxiter=0 fillcolor=0 inside=0 outside=0 biomorph=0 symmetry=none
  periodicity=0 cyclerange=0/0 hertz=0 sound=off volume=0 attack=0
  decay=0 sustain=0 srelease=0 orbitinterval=0
}

foo                {
                     ; Id Version 1.2.3.4
  reset=1/2/3/4 type=mandel passes= corners=0/0/0/0 params=0/0
  maxiter=0 fillcolor=0 outside=0 biomorph=0 symmetry=none
  periodicity=0 cyclerange=0/0 hertz=0 sound=off volume=0 attack=0
  decay=0 sustain=0 srelease=0 orbitinterval=0
}

)par",
        read_file_contents(path))
        << path;
}

struct SavedParameterVersionInput
{
    const char *name;
    Version version;
    std::optional<bool> legacy_float;
    const char *reset_text;
    const char *version_text;
};

void PrintTo(const SavedParameterVersionInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSavedParameterVersion : public TestGeneratedParameterFile,
                                  public WithParamInterface<SavedParameterVersionInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_legacy_float{g_user.legacy_float};
};

void TestSavedParameterVersion::SetUp()
{
    TestGeneratedParameterFile::SetUp();
    g_version = GetParam().version;
    if (GetParam().legacy_float)
    {
        g_user.legacy_float = *GetParam().legacy_float;
    }
}

TEST_P(TestSavedParameterVersion, savesCompatibilityVersionText)
{
    exec_cmd_arg("makepar=" + m_output.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    const std::string text{read_file_contents(m_output)};
    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_THAT(text, HasSubstr(GetParam().reset_text));
    if (GetParam().version_text)
    {
        EXPECT_THAT(text, HasSubstr(GetParam().version_text));
    }
}

INSTANTIATE_TEST_SUITE_P(Versions, TestSavedParameterVersion,
    Values(SavedParameterVersionInput{"LegacyReset", parse_legacy_version(1730), std::nullopt,
               "  reset=1730 type=mandel", nullptr},
        SavedParameterVersionInput{
            "LegacyFloat", parse_legacy_version(1730), true, " reset=1730 type=mandel float=y", nullptr},
        SavedParameterVersionInput{
            "IdReset", Version{1, 3, 2, 0, false}, std::nullopt, "  reset=1/3/2 type=mandel", "; Id Version 1.3.2"}),
    [](const TestParamInfo<SavedParameterVersionInput> &info) { return info.param.name; });

TEST_F(TestGeneratedParameterFile, makeParWritesTrimmedCornerValues)
{
    const fs::path &path{m_output};
    ValueSaver saved_use_center_mag{g_use_center_mag, false};
    ValueSaver saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver saved_x_min{g_image_region.m_min.x, -1.25};
    ValueSaver saved_x_3rd{g_image_region.m_3rd.x, -1.25};
    ValueSaver saved_x_max{g_image_region.m_max.x, 2.5};
    ValueSaver saved_y_min{g_image_region.m_min.y, -3.75};
    ValueSaver saved_y_3rd{g_image_region.m_3rd.y, -3.75};
    ValueSaver saved_y_max{g_image_region.m_max.y, 4.0};

    exec_cmd_arg("makepar=" + path.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_THAT(read_file_contents(path), HasSubstr(" corners=-1.25/2.5/-3.75/4"));
}

TEST_F(TestGeneratedParameterFile, makeParWrapsAtConfiguredLineLength)
{
    const fs::path &path{m_output};
    ValueSaver saved_max_line_length{g_max_line_length, MIN_MAX_LINE_LENGTH};

    exec_cmd_arg("makepar=" + path.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    const std::string text{read_file_contents(path)};
    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_THAT(text, HasSubstr(" passes=\n  corners="));

    std::istringstream lines{text};
    std::string line;
    while (std::getline(lines, line))
    {
        if (line.rfind("  ", 0) == 0 && line.find("; Id Version") == std::string::npos)
        {
            EXPECT_LE(line.length(), static_cast<std::size_t>(g_max_line_length)) << line;
        }
    }
}

TEST_F(TestGeneratedParameterFile, makeParMapOnlyWritesColors)
{
    const fs::path &path{m_output};
    ValueSaver saved_read_filename{g_read_filename, fs::path{}};
    ValueSaver saved_map_name{g_map_name, std::string{"rainbow.map"}};
    ValueSaver saved_file_colors{g_file_colors, 16};

    exec_cmd_arg("makepar=" + path.filename().string() + "/bar", CmdFile::SSTOOLS_INI);

    const std::string text{read_file_contents(path)};
    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_THAT(text, HasSubstr(" colors="));
    EXPECT_THAT(text, Not(HasSubstr(" type=mandel")));
    EXPECT_THAT(text, Not(HasSubstr(" reset=")));
}

class TestCommandMakeParView : public TestCommandMakePar
{
protected:
    void TearDown() override;
    std::string make_par_for_image(int width, int height, float aspect_ratio);

    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{false, false, false, false, 0.75f, 4.2f}};
    ValueSaver<float> m_saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<float> m_saved_file_aspect{g_file_aspect_ratio};
    ValueSaver<int> m_saved_video_count{g_video_table_len, 1};
    ValueSaver<VideoInfo> m_saved_mode{g_video_table[0], VideoInfo{800, 600, 16, nullptr, {}}};
    ValueSaver<int> m_saved_mode_key{g_video_mode_keys[0], ID_KEY_F6};
    ValueSaver<int> m_saved_active_key{g_video_entry_key, 0};
    ValueSaver<std::string> m_saved_comment{g_par_comment[0]};
    ValueSaver<bool> m_saved_fast_restore{g_fast_restore, false};
    ValueSaver<fs::path> m_saved_read_filename{g_read_filename};
    GifFileType m_extensions{};
    fs::path m_image_path;
    fs::path m_parameter_path;
};

void TestCommandMakeParView::TearDown()
{
    GifFreeExtensions(&m_extensions.ExtensionBlockCount, &m_extensions.ExtensionBlocks);
    std::error_code ignored;
    fs::remove(m_image_path, ignored);
    fs::remove(m_parameter_path, ignored);
    TestCommandMakePar::TearDown();
}

std::string TestCommandMakeParView::make_par_for_image(const int width, const int height, const float aspect_ratio)
{
    const std::string name{parameter_test_name()};
    m_image_path = par_path((name + ".gif").c_str());
    m_parameter_path = par_path((name + ".par").c_str());
    fs::remove(m_image_path);
    fs::remove(m_parameter_path);
    FractalInfo info{};
    setup_save_info(&info);
    info.x_dots = 800;
    info.y_dots = 600;
    info.colors = 16;
    info.final_aspect_ratio = aspect_ratio;
    put_fractal_info(&m_extensions, info);
    GifWriter writer{m_image_path, {width, height}, GifWriter::Palette(16)};
    const std::vector<Byte> pixels(width, 0);
    for (int row = 0; row < height; ++row)
    {
        if (!writer.write_pixels(pixels))
        {
            throw std::runtime_error("Can't write makepar test scanline");
        }
    }
    if (!writer.write_extensions(m_extensions) || !writer.finish())
    {
        throw std::runtime_error("Can't finish makepar test image");
    }
    g_read_filename = m_image_path;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, set_video_mode(_)).Times(0);
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(0);

    exec_cmd_arg("makepar=" + m_parameter_path.filename().string() + "/view", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    return read_file_contents(m_parameter_path);
}

struct SavedViewportInput
{
    const char *name;
    int width;
    int height;
    float aspect;
    const char *text;
    std::optional<std::pair<int, int>> logical_screen;
};

void PrintTo(const SavedViewportInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSavedViewport : public TestCommandMakeParView, public WithParamInterface<SavedViewportInput>
{
};

TEST_P(TestSavedViewport, retainsReducedViewParameters)
{
    const SavedViewportInput &input{GetParam()};

    const std::string text{make_par_for_image(input.width, input.height, input.aspect)};

    EXPECT_THAT(text, HasSubstr(input.text));
    if (input.logical_screen)
    {
        EXPECT_EQ(input.logical_screen->first, g_logical_screen.x_dots);
        EXPECT_EQ(input.logical_screen->second, g_logical_screen.y_dots);
    }
}

INSTANTIATE_TEST_SUITE_P(Viewports, TestSavedViewport,
    Values(
        SavedViewportInput{"Automatic", 200, 150, 0.75f, " viewwindows=4/0.75/no/0/0", std::pair<int, int>{200, 150}},
        SavedViewportInput{"Explicit", 201, 150, 0.75f, " viewwindows=4.2/0.75/no/201/150", std::nullopt},
        SavedViewportInput{"Aspect", 200, 100, 0.5f, " viewwindows=4/0.5/no/0/0", std::nullopt}),
    [](const TestParamInfo<SavedViewportInput> &info) { return info.param.name; });

struct SavedVideoKeyInput
{
    const char *name;
    int width;
    int height;
    int binding;
    int active_key;
    bool clear_reserved_key;
    const char *comment;
};

void PrintTo(const SavedVideoKeyInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSavedVideoKey : public TestCommandMakeParView, public WithParamInterface<SavedVideoKeyInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_reserved_key{g_video_mode_keys[MAX_VIDEO_MODES - 1]};
};

void TestSavedVideoKey::SetUp()
{
    TestCommandMakeParView::SetUp();
    const SavedVideoKeyInput &input{GetParam()};
    g_video_table[0].x_dots = input.width;
    g_video_table[0].y_dots = input.height;
    g_video_mode_keys[0] = input.binding;
    g_video_entry_key = input.active_key;
    g_par_comment[0] = "video:$vidkey$!";
    if (input.clear_reserved_key)
    {
        g_video_mode_keys[MAX_VIDEO_MODES - 1] = 0;
    }
}

TEST_P(TestSavedVideoKey, expandsSelectedModeWithoutChangingActiveVideo)
{
    const VideoInfo active_video{g_video_entry};

    const std::string text{make_par_for_image(200, 150, 0.75f)};

    EXPECT_THAT(text, HasSubstr(GetParam().comment));
    EXPECT_EQ(GetParam().active_key, g_video_entry_key);
    EXPECT_EQ(active_video, g_video_entry);
}

INSTANTIATE_TEST_SUITE_P(Bindings, TestSavedVideoKey,
    Values(SavedVideoKeyInput{"Startup", 800, 600, ID_KEY_F6, 0, false, "; video:F6!"},
        SavedVideoKeyInput{"OtherActiveKey", 800, 600, ID_KEY_F6, ID_KEY_F2, false, "; video:F6!"},
        SavedVideoKeyInput{"Unbound", 800, 600, 0, ID_KEY_F2, true, "; video:!"},
        SavedVideoKeyInput{"NoMatch", 1024, 768, ID_KEY_F6, ID_KEY_F2, false, "; video:!"}),
    [](const TestParamInfo<SavedVideoKeyInput> &info) { return info.param.name; });

TEST_F(TestCommandMakeParView, makeParWithoutMatchingVideoModeDoesNotInventAViewport)
{
    g_video_table[0].x_dots = 1024;
    g_video_table[0].y_dots = 768;

    const std::string text{make_par_for_image(200, 150, 0.75f)};

    EXPECT_THAT(text, Not(HasSubstr("viewwindows=")));
    EXPECT_EQ(200, g_logical_screen.x_dots);
    EXPECT_EQ(150, g_logical_screen.y_dots);
}

struct RejectedReleaseInput
{
    const char *name;
    const char *argument;
    std::optional<Version> initial_version;
    std::optional<Version> initial_file_version;
};

void PrintTo(const RejectedReleaseInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRejectedRelease : public TestParameterCommandError, public WithParamInterface<RejectedReleaseInput>
{
protected:
    void SetUp() override;

    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
};

void TestRejectedRelease::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().initial_version)
    {
        g_version = *GetParam().initial_version;
    }
    if (GetParam().initial_file_version)
    {
        g_file_version = *GetParam().initial_file_version;
    }
}

TEST_P(TestRejectedRelease, rejectsArgumentWithoutChangingProvidedVersions)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    if (GetParam().initial_version)
    {
        EXPECT_EQ(*GetParam().initial_version, g_version);
    }
    if (GetParam().initial_file_version)
    {
        EXPECT_EQ(*GetParam().initial_file_version, g_file_version);
    }
}

INSTANTIATE_TEST_SUITE_P(Arguments, TestRejectedRelease,
    Values(
        RejectedReleaseInput{"NotResetAlias", "release=1906", Version{1, 2, 3, 4, false}, Version{5, 6, 7, 8, false}},
        RejectedReleaseInput{"Unsupported", "release=100", std::nullopt, std::nullopt}),
    [](const TestParamInfo<RejectedReleaseInput> &info) { return info.param.name; });

struct LambdaResetCompatibilityInput
{
    const char *name;
    const char *argument;
    int escape_result;
};

void PrintTo(const LambdaResetCompatibilityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLambdaResetCompatibility : public TestParameterCommand,
                                     public WithParamInterface<LambdaResetCompatibilityInput>
{
protected:
    void SetUp() override;

    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<TrigFn> m_saved_trig{g_trig_index[0]};
    ValueSaver<DComplex *> m_saved_float_param{g_float_param};
    ValueSaver<DComplex> m_saved_param{g_param_z1};
    ValueSaver<DComplex> m_saved_old_z{g_old_z};
    ValueSaver<double> m_saved_limit{g_magnitude_limit};
};

void TestLambdaResetCompatibility::SetUp()
{
    TestParameterCommand::SetUp();
    exec_cmd_arg(GetParam().argument);
    set_trig_array(0, "exp");
    set_fractal_type(FractalType::LAMBDA_FN);
    g_param_z1 = DComplex{1.9, 0.4};
    g_float_param = &g_param_z1;
    g_magnitude_limit = 64.0;
    lambda_trig_per_image();
    g_old_z = DComplex{9.0, 0.0};
}

TEST_P(TestLambdaResetCompatibility, resetVersionControlsEscapeResult)
{
    EXPECT_EQ(GetParam().escape_result, orbit_calc());
}

INSTANTIATE_TEST_SUITE_P(Versions, TestLambdaResetCompatibility,
    Values(LambdaResetCompatibilityInput{"Through2002", "reset=2002", 0},
        LambdaResetCompatibilityInput{"After2002", "reset=2003", 1}),
    [](const TestParamInfo<LambdaResetCompatibilityInput> &info) { return info.param.name; });

struct AtanColorCompatibilityInput
{
    const char *name;
    int version;
    int scale;
};

void PrintTo(const AtanColorCompatibilityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAtanColorCompatibility : public TestParameterCommand, public WithParamInterface<AtanColorCompatibilityInput>
{
protected:
    void SetUp() override;

    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_scale{g_atan_colors};
};

void TestAtanColorCompatibility::SetUp()
{
    TestParameterCommand::SetUp();
    g_version = parse_legacy_version(GetParam().version);
}

TEST_P(TestAtanColorCompatibility, versionControlsColorScale)
{
    init_atan_colors();

    EXPECT_EQ(GetParam().scale, g_atan_colors);
}

INSTANTIATE_TEST_SUITE_P(Versions, TestAtanColorCompatibility,
    Values(AtanColorCompatibilityInput{"Through2002", 2002, 180}, AtanColorCompatibilityInput{"After2002", 2003, 256}),
    [](const TestParamInfo<AtanColorCompatibilityInput> &info) { return info.param.name; });

struct FilenameMaskParameterInput
{
    const char *name;
    const char *initial;
    const char *argument;
    CmdFile source;
    const char *expected;
};

void PrintTo(const FilenameMaskParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFilenameMaskParameter : public TestParameterCommand, public WithParamInterface<FilenameMaskParameterInput>
{
protected:
    void SetUp() override;

    ValueSaver<std::string> m_saved_mask{g_image_filename_mask};
};

void TestFilenameMaskParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_image_filename_mask = GetParam().initial;
}

TEST_P(TestFilenameMaskParameter, extensionSelectsExpectedMask)
{
    exec_cmd_arg(GetParam().argument, GetParam().source);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().expected, g_image_filename_mask);
}

INSTANTIATE_TEST_SUITE_P(Extensions, TestFilenameMaskParameter,
    Values(FilenameMaskParameterInput{"Long", "*.pot", "filename=.foobar", CmdFile::AT_AFTER_STARTUP, "*.foobar"},
        FilenameMaskParameterInput{"Gif", "*.pot", "filename=.gif", CmdFile::AT_AFTER_STARTUP, "*.gif"},
        FilenameMaskParameterInput{"CommandLinePot", "*.foo", "filename=.pot", CmdFile::AT_CMD_LINE, "*.pot"}),
    [](const TestParamInfo<FilenameMaskParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommandError, filenameValueTooLong)
{
    ValueSaver saved_gif_filename_mask{g_image_filename_mask, "*.pot"};
    const std::string too_long{"filename=" + std::string(ID_FILE_MAX_PATH, 'f') + ".gif"};

    exec_cmd_arg(too_long);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ("*.pot", g_image_filename_mask);
}

TEST_F(TestParameterCommandError, mapTooLong)
{
    ValueSaver saved_map_name{g_map_name, "foo.map"};
    const std::string too_long{"map=" + std::string(ID_FILE_MAX_PATH, 'f') + ".map"};

    exec_cmd_arg(too_long, CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ("foo.map", g_map_name);
}

struct MapLookupParameterInput
{
    const char *name;
    const char *initial;
    bool current_directory;
};

void PrintTo(const MapLookupParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMapLookupParameter : public TestParameterCommand, public WithParamInterface<MapLookupParameterInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<std::string> m_saved_map{g_map_name};
    std::optional<CurrentPathSaver> m_saved_path;
};

void TestMapLookupParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_map_name = GetParam().initial;
    if (GetParam().current_directory)
    {
        m_saved_path.emplace(fs::path{ID_TEST_HOME_DIR});
    }
    else
    {
        add_read_library(ID_TEST_HOME_DIR);
    }
}

void TestMapLookupParameter::TearDown()
{
    m_saved_path.reset();
    TestParameterCommand::TearDown();
}

TEST_P(TestMapLookupParameter, lookupSelectsRequestedMap)
{
    const std::string name{std::string{ID_TEST_MAP_SUBDIR} + SLASH + ID_TEST_MAP_FILE};

    exec_cmd_arg("map=" + name, CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(name, g_map_name);
}

INSTANTIATE_TEST_SUITE_P(Lookups, TestMapLookupParameter,
    Values(MapLookupParameterInput{"LibrarySubdirectory", "", false},
        MapLookupParameterInput{"ExistingRelativeFile", "foo.map", true}),
    [](const TestParamInfo<MapLookupParameterInput> &info) { return info.param.name; });

struct DeprecatedAdapterInput
{
    const char *name;
    const char *argument;
    CmdArgFlags result;
};

void PrintTo(const DeprecatedAdapterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDeprecatedAdapterParameter : public TestParameterCommand, public WithParamInterface<DeprecatedAdapterInput>
{
};

TEST_P(TestDeprecatedAdapterParameter, recognizesExpectedCompatibilityResult)
{
    exec_cmd_arg(GetParam().argument, CmdFile::SSTOOLS_INI);

    EXPECT_EQ(GetParam().result, m_result);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Adapters, TestDeprecatedAdapterParameter,
    Values(DeprecatedAdapterInput{"Egamono", "adapter=egamono", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Hgc", "adapter=hgc", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Ega", "adapter=ega", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Cga", "adapter=cga", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Mcga", "adapter=mcga", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Vga", "adapter=vga", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        DeprecatedAdapterInput{"Invalid", "adapter=bad", CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<DeprecatedAdapterInput> &info) { return info.param.name; });

struct LegacyCommandInput
{
    const char *name;
    const char *argument;
    CmdFile mode;
    CmdArgFlags result;
};

void PrintTo(const LegacyCommandInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLegacyCommandInput : public TestParameterCommand, public WithParamInterface<LegacyCommandInput>
{
};

TEST_P(TestLegacyCommandInput, retainsCompatibilityResult)
{
    exec_cmd_arg(GetParam().argument, GetParam().mode);

    EXPECT_EQ(GetParam().result, m_result);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Fpu, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Accepted", "fpu=387", CmdFile::SSTOOLS_INI, CmdArgFlags::NONE},
        LegacyCommandInput{"Invalid", "fpu=487", CmdFile::SSTOOLS_INI, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(Ignored, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Afi", "afi=anything", CmdFile::SSTOOLS_INI,
               CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D},
        LegacyCommandInput{"IterIncr", "iterincr=fmeh", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE},
        LegacyCommandInput{"TweakLzw", "tweaklzw=fmeh", CmdFile::AT_AFTER_STARTUP, CmdArgFlags::NONE},
        LegacyCommandInput{"ExitMode", "exitmode=fmeh", CmdFile::AT_AFTER_STARTUP, CmdArgFlags::NONE},
        LegacyCommandInput{"RamVideo", "ramvideo=64", CmdFile::AT_AFTER_STARTUP, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

struct DeprecatedDisplayParameterInput
{
    const char *name;
    const char *argument;
    CmdArgFlags result;
    std::optional<bool> first_init;
};

void PrintTo(const DeprecatedDisplayParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDeprecatedDisplayParameter : public TestParameterCommand,
                                       public WithParamInterface<DeprecatedDisplayParameterInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_first_init{g_first_init};
};

void TestDeprecatedDisplayParameter::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().first_init)
    {
        g_first_init = *GetParam().first_init;
    }
}

TEST_P(TestDeprecatedDisplayParameter, retainsCompatibilityResult)
{
    exec_cmd_arg(GetParam().argument, CmdFile::SSTOOLS_INI);

    EXPECT_EQ(GetParam().result, m_result);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(TextSafe, TestDeprecatedDisplayParameter,
    Values(DeprecatedDisplayParameterInput{"N", "textsafe=n", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, true},
        DeprecatedDisplayParameterInput{"Y", "textsafe=y", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, true},
        DeprecatedDisplayParameterInput{"B", "textsafe=b", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, true},
        DeprecatedDisplayParameterInput{"S", "textsafe=s", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, true},
        DeprecatedDisplayParameterInput{"Invalid", "textsafe=!", CmdArgFlags::BAD_ARG, true}),
    [](const TestParamInfo<DeprecatedDisplayParameterInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(VesaDetect, TestDeprecatedDisplayParameter,
    Values(DeprecatedDisplayParameterInput{"N", "vesadetect=n", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D,
               std::nullopt},
        DeprecatedDisplayParameterInput{
            "Y", "vesadetect=y", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, std::nullopt},
        DeprecatedDisplayParameterInput{"Invalid", "vesadetect=!", CmdArgFlags::BAD_ARG, std::nullopt}),
    [](const TestParamInfo<DeprecatedDisplayParameterInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(BiosPalette, TestDeprecatedDisplayParameter,
    Values(DeprecatedDisplayParameterInput{"N", "biospalette=n", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D,
               std::nullopt},
        DeprecatedDisplayParameterInput{
            "Y", "biospalette=y", CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, std::nullopt},
        DeprecatedDisplayParameterInput{"Invalid", "biospalette=!", CmdArgFlags::BAD_ARG, std::nullopt}),
    [](const TestParamInfo<DeprecatedDisplayParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommandError, filenameTooLong)
{
    const std::string saved_gif_filename_mask{g_image_filename_mask};
    const ShowFile saved_show_file{g_show_file};
    const std::string saved_browse_name{g_browse.name};
    const std::string too_long{"filename=" + std::string(1024, 'f')};

    exec_cmd_arg(too_long, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ(saved_gif_filename_mask, g_image_filename_mask);
    EXPECT_EQ(saved_show_file, g_show_file);
    EXPECT_EQ(saved_browse_name, g_browse.name);
}

class DACSaver
{
public:
    DACSaver()
    {
        for (int i = 0; i < 256; ++i)
        {
            m_dac_box[i][0] = g_dac_box[i][0];
            m_dac_box[i][1] = g_dac_box[i][1];
            m_dac_box[i][2] = g_dac_box[i][2];
        }
    }

    ~DACSaver()
    {
        for (int i = 0; i < 256; ++i)
        {
            g_dac_box[i][0] = m_dac_box[i][0];
            g_dac_box[i][1] = m_dac_box[i][1];
            g_dac_box[i][2] = m_dac_box[i][2];
        }
    }

private:
    Byte m_dac_box[256][3];
};

TEST_F(TestParameterCommand, colorsEmptySetsDefaultDAC)
{
    DACSaver saved_dac_box;
    for (int i = 0; i < 256; ++i) // NOLINT(modernize-loop-convert)
    {
        g_dac_box[i][0] = 0x10;
        g_dac_box[i][1] = 0x20;
        g_dac_box[i][2] = 0x30;
    }

    exec_cmd_arg("colors", CmdFile::AT_CMD_LINE);

    const Byte default_color = expand_6bit_color(40);
    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(default_color, g_dac_box[0][0]);
    EXPECT_EQ(default_color, g_dac_box[0][1]);
    EXPECT_EQ(default_color, g_dac_box[0][2]);
    EXPECT_EQ(default_color, g_dac_box[255][0]);
    EXPECT_EQ(default_color, g_dac_box[255][1]);
    EXPECT_EQ(default_color, g_dac_box[255][2]);
}

TEST_F(TestParameterCommand, colorsMapAddsDefaultExtension)
{
    ColorMapSaver saved_dac_box;
    ValueSaver saved_map_name{g_map_name, ""};
    ValueSaver saved_last_map_name{g_last_map_name, ""};

    exec_cmd_arg("colors=@test", CmdFile::AT_CMD_LINE_SET_NAME);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ("test.map", g_map_name);
    EXPECT_EQ("test", g_last_map_name);
}

TEST_F(TestParameterCommand, colorsMapNameIsCaseSensitive)
{
    if constexpr (!ID_TEST_FILESYSTEM_CASE_SENSITIVE)
    {
        return;
    }

    ColorMapSaver saved_dac_box;
    ValueSaver saved_map_name{g_map_name, ""};
    ValueSaver saved_last_map_name{g_last_map_name, ""};
    ValueSaver saved_first_init{g_first_init, false};
    ValueSaver saved_init_batch{g_init_batch, BatchMode::NORMAL};
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};

    exec_cmd_arg("colors=@TEST", CmdFile::AT_CMD_LINE_SET_NAME);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_TRUE(g_map_name.empty());
    EXPECT_TRUE(g_last_map_name.empty());
}

TEST_F(TestParameterCommand, missingMapReturnsDiagnosticAndPreservesMapName)
{
    ColorMapSaver saved_palette;
    ValueSaver saved_map_name{g_map_name, std::string{"previous.map"}};
    ValueSaver saved_specified{g_map_specified, false};
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(nullptr)};
    CommandProcessor processor{"map=missing-palette.map", CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;
    const CommandParseResult parsed{router.process(processor)};
    const CmdArgFlags result{parsed.flags};
    const CommandDiagnostics &diagnostics{parsed.diagnostics};

    EXPECT_EQ(CmdArgFlags::NONE, result);
    EXPECT_EQ("previous.map", g_map_name);
    EXPECT_FALSE(g_map_specified);
    ASSERT_EQ(1U, diagnostics.errors.size());
    EXPECT_EQ("Could not load color map missing-palette.map", diagnostics.errors.front());
}

class TestParameterCommandFile : public TestParameterCommand
{
protected:
    void SetUp() override;
    CommandParseResult process_commands(const char *commands);

private:
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 150L};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> m_file{nullptr, std::fclose};
};

void TestParameterCommandFile::SetUp()
{
    TestParameterCommand::SetUp();
    m_file.reset(std::tmpfile());
    ASSERT_NE(nullptr, m_file);
}

CommandParseResult TestParameterCommandFile::process_commands(const char *commands)
{
    std::fputs(commands, m_file.get());
    std::rewind(m_file.get());
    CommandProcessor processor{m_file.release(), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;
    return router.process(processor);
}

TEST_F(TestParameterCommandFile, missingPaletteStopsCommandFileWithoutLosingEarlierChangeFlags)
{
    const CommandParseResult parsed{
        process_commands("test { maxiter=321 colors=@missing-command-palette.map maxiter=654 }")};
    const CmdArgFlags result{parsed.flags};
    const CommandDiagnostics &diagnostics{parsed.diagnostics};

    EXPECT_TRUE(bit_set(result, CmdArgFlags::FRACTAL_PARAM));
    EXPECT_EQ(321, g_max_iterations);
    ASSERT_EQ(1U, diagnostics.errors.size());
    EXPECT_EQ("Could not load color map missing-command-palette.map", diagnostics.errors.front());
}

struct RecordColorsParameterInput
{
    const char *name;
    const char *argument;
    RecordColorsMode expected;
    CmdArgFlags result;
};

void PrintTo(const RecordColorsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRecordColorsParameter : public TestParameterCommand, public WithParamInterface<RecordColorsParameterInput>
{
private:
    ValueSaver<RecordColorsMode> m_saved_colors{g_record_colors, RecordColorsMode::NONE};
};

TEST_P(TestRecordColorsParameter, setsRecordingMode)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_record_colors);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestRecordColorsParameter,
    Values(RecordColorsParameterInput{"Invalid", "recordcolors=p", RecordColorsMode::NONE, CmdArgFlags::BAD_ARG},
        RecordColorsParameterInput{"Automatic", "recordcolors=a", RecordColorsMode::AUTOMATIC, CmdArgFlags::NONE},
        RecordColorsParameterInput{"Comment", "recordcolors=c", RecordColorsMode::COMMENT, CmdArgFlags::NONE},
        RecordColorsParameterInput{"Yes", "recordcolors=y", RecordColorsMode::YES, CmdArgFlags::NONE}),
    [](const TestParamInfo<RecordColorsParameterInput> &info) { return info.param.name; });

struct MaxLineLengthParameterInput
{
    const char *name;
    int value;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const MaxLineLengthParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMaxLineLengthParameter : public TestParameterCommand, public WithParamInterface<MaxLineLengthParameterInput>
{
private:
    ValueSaver<int> m_saved_length{g_max_line_length, 0};
};

TEST_P(TestMaxLineLengthParameter, setsLineLength)
{
    exec_cmd_arg("maxlinelength=" + std::to_string(GetParam().value), CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_max_line_length);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMaxLineLengthParameter,
    Values(MaxLineLengthParameterInput{"TooSmall", MIN_MAX_LINE_LENGTH - 1, 0, CmdArgFlags::BAD_ARG},
        MaxLineLengthParameterInput{"TooLarge", MAX_MAX_LINE_LENGTH + 1, 0, CmdArgFlags::BAD_ARG},
        MaxLineLengthParameterInput{"Midpoint", (MIN_MAX_LINE_LENGTH + MAX_MAX_LINE_LENGTH) / 2,
            (MIN_MAX_LINE_LENGTH + MAX_MAX_LINE_LENGTH) / 2, CmdArgFlags::NONE}),
    [](const TestParamInfo<MaxLineLengthParameterInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(TPlus, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Invalid", "tplus=!", CmdFile::AT_CMD_LINE, CmdArgFlags::BAD_ARG},
        LegacyCommandInput{"Yes", "tplus=y", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(NonInterlaced, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Invalid", "noninterlaced=!", CmdFile::AT_CMD_LINE, CmdArgFlags::BAD_ARG},
        LegacyCommandInput{"Yes", "noninterlaced=y", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(MaxColorRes, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Invalid", "maxcolorres=3", CmdFile::AT_CMD_LINE, CmdArgFlags::BAD_ARG},
        LegacyCommandInput{"One", "maxcolorres=1", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE},
        LegacyCommandInput{"Four", "maxcolorres=4", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE},
        LegacyCommandInput{"Eight", "maxcolorres=8", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE},
        LegacyCommandInput{"Sixteen", "maxcolorres=16", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(PixelZoom, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Invalid", "pixelzoom=5", CmdFile::AT_CMD_LINE, CmdArgFlags::BAD_ARG},
        LegacyCommandInput{"Four", "pixelzoom=4", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

struct OverwriteParameterInput
{
    const char *name;
    const char *argument;
    bool initial;
    bool expected;
    CmdArgFlags result;
};

void PrintTo(const OverwriteParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOverwriteParameter : public TestParameterCommand, public WithParamInterface<OverwriteParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file};
};

void TestOverwriteParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_overwrite_file = GetParam().initial;
}

TEST_P(TestOverwriteParameter, setsOverwritePolicy)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_overwrite_file);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Warn, TestOverwriteParameter,
    Values(OverwriteParameterInput{"Invalid", "warn=!", false, false, CmdArgFlags::BAD_ARG},
        OverwriteParameterInput{"No", "warn=n", false, true, CmdArgFlags::NONE},
        OverwriteParameterInput{"Yes", "warn=yes", true, false, CmdArgFlags::NONE}),
    [](const TestParamInfo<OverwriteParameterInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(Overwrite, TestOverwriteParameter,
    Values(OverwriteParameterInput{"Invalid", "overwrite=!", true, true, CmdArgFlags::BAD_ARG},
        OverwriteParameterInput{"No", "overwrite=n", false, false, CmdArgFlags::NONE},
        OverwriteParameterInput{"Yes", "overwrite=yes", true, true, CmdArgFlags::NONE}),
    [](const TestParamInfo<OverwriteParameterInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(Gif87a, TestLegacyCommandInput,
    Values(LegacyCommandInput{"Invalid", "gif87a=!", CmdFile::AT_CMD_LINE, CmdArgFlags::BAD_ARG},
        LegacyCommandInput{"No", "gif87a=n", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE},
        LegacyCommandInput{"Yes", "gif87a=yes", CmdFile::AT_CMD_LINE, CmdArgFlags::NONE}),
    [](const TestParamInfo<LegacyCommandInput> &info) { return info.param.name; });

struct DitherParameterInput
{
    const char *name;
    const char *argument;
    std::optional<bool> initial;
    std::optional<bool> expected;
    CmdArgFlags result;
};

void PrintTo(const DitherParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDitherParameter : public TestParameterCommand, public WithParamInterface<DitherParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_dither{g_dither_flag};
};

void TestDitherParameter::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().initial)
    {
        g_dither_flag = *GetParam().initial;
    }
}

TEST_P(TestDitherParameter, setsDitherMode)
{
    const bool expected{GetParam().expected.value_or(g_dither_flag)};

    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(expected, g_dither_flag);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDitherParameter,
    Values(DitherParameterInput{"Invalid", "dither=!", std::nullopt, std::nullopt, CmdArgFlags::BAD_ARG},
        DitherParameterInput{"No", "dither=n", true, false, CmdArgFlags::NONE},
        DitherParameterInput{"Yes", "dither=yes", false, true, CmdArgFlags::NONE}),
    [](const TestParamInfo<DitherParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, saveTime)
{
    ValueSaver saved_time_interval{g_save_time_interval, -1};

    exec_cmd_arg("savetime=20", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(20, g_save_time_interval);
}

class ParamSaver
{
public:
    explicit ParamSaver(double params[MAX_PARAMS]);
    ~ParamSaver();

private:
    double m_params[MAX_PARAMS];
};

ParamSaver::ParamSaver(double params[MAX_PARAMS])
{
    std::copy(&g_params[0], &g_params[MAX_PARAMS], &m_params[0]);
    std::copy(&params[0], &params[MAX_PARAMS], &g_params[0]);
}

ParamSaver::~ParamSaver()
{
    std::copy(&m_params[0], &m_params[MAX_PARAMS], &g_params[0]);
}

TEST_F(TestParameterCommand, typeSierpinski)
{
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::LYAPUNOV};
    ValueSaver saved_fractal_specific{g_cur_fractal_specific, nullptr};
    ValueSaver saved_x_min{g_image_region.m_min.x, 111.0};
    ValueSaver saved_x_max{g_image_region.m_max.x, 222.0};
    ValueSaver saved_x_3rd{g_image_region.m_3rd.x, 333.0};
    ValueSaver saved_y_min{g_image_region.m_min.y, 444.0};
    ValueSaver saved_y_max{g_image_region.m_max.y, 555.0};
    ValueSaver saved_y_3rd{g_image_region.m_3rd.y, 666.0};
    double params[MAX_PARAMS]{111.0, 222.0, 333.0, 444.0, 555.0, 666.0, 777.0, 888.0, 999.0, 101010.0};
    ParamSaver saved_params(params);

    exec_cmd_arg("type=sierpinski", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(FractalType::SIERPINSKI, g_fractal_type);
    EXPECT_EQ(g_cur_fractal_specific, get_fractal_specific(FractalType::SIERPINSKI));
    const FractalSpecific &fractal{*g_cur_fractal_specific};
    EXPECT_EQ(g_image_region.m_min.x, fractal.x_min);
    EXPECT_EQ(g_image_region.m_max.x, fractal.x_max);
    EXPECT_EQ(g_image_region.m_3rd.x, fractal.x_min);
    EXPECT_EQ(g_image_region.m_min.y, fractal.y_min);
    EXPECT_EQ(g_image_region.m_max.y, fractal.y_max);
    EXPECT_EQ(g_image_region.m_3rd.y, fractal.y_min);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(fractal.params[i], g_params[i]);
    }
}

struct ColoringParameterInput
{
    const char *name;
    const char *argument;
    ColorMethod method;
    int color;
    CmdArgFlags result;
};

void PrintTo(const ColoringParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInsideParameter : public TestParameterCommand, public WithParamInterface<ColoringParameterInput>
{
private:
    ValueSaver<ColorMethod> m_saved_method{g_inside_method, static_cast<ColorMethod>(-9999)};
    ValueSaver<int> m_saved_color{g_inside_color, -9999};
};

TEST_P(TestInsideParameter, setsInsideColoring)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().method, g_inside_method);
    EXPECT_EQ(GetParam().color, g_inside_color);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideParameter,
    Values(
        ColoringParameterInput{"Invalid", "inside=foo", static_cast<ColorMethod>(-9999), -9999, CmdArgFlags::BAD_ARG},
        ColoringParameterInput{"ZMag", "inside=zmag", ColorMethod::ZMAG, -1, CmdArgFlags::FRACTAL_PARAM},
        ColoringParameterInput{"Number", "inside=100", ColorMethod::COLOR, 100, CmdArgFlags::FRACTAL_PARAM},
        ColoringParameterInput{
            "EpsilonCross", "inside=epsiloncross", ColorMethod::EPS_CROSS, -1, CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<ColoringParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, proximity)
{
    ValueSaver saved_close_proximity{g_close_proximity, -123.0};

    exec_cmd_arg("proximity=423", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(423.0, g_close_proximity);
}

struct FillColorParameterInput
{
    const char *name;
    const char *argument;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const FillColorParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFillColorParameter : public TestParameterCommand, public WithParamInterface<FillColorParameterInput>
{
private:
    ValueSaver<int> m_saved_fill_color{g_fill_color, 999};
};

TEST_P(TestFillColorParameter, setsFillColor)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_fill_color);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFillColorParameter,
    Values(FillColorParameterInput{"Normal", "fillcolor=normal", -1, CmdArgFlags::FRACTAL_PARAM},
        FillColorParameterInput{"Number", "fillcolor=100", 100, CmdArgFlags::FRACTAL_PARAM},
        FillColorParameterInput{"Invalid", "fillcolor=fmeh", 999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<FillColorParameterInput> &info) { return info.param.name; });

struct FinAttractParameterInput
{
    const char *name;
    const char *argument;
    bool initial;
    bool expected;
    CmdArgFlags result;
};

void PrintTo(const FinAttractParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFinAttractParameter : public TestParameterCommand, public WithParamInterface<FinAttractParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_enabled{g_attractor.enabled};
};

void TestFinAttractParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_attractor.enabled = GetParam().initial;
}

TEST_P(TestFinAttractParameter, setsFiniteAttractorMode)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_attractor.enabled);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFinAttractParameter,
    Values(FinAttractParameterInput{"Invalid", "finattract=!", true, true, CmdArgFlags::BAD_ARG},
        FinAttractParameterInput{"Yes", "finattract=y", false, true, CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<FinAttractParameterInput> &info) { return info.param.name; });

struct BofParameterInput
{
    const char *name;
    const char *argument;
    bool expected;
    CmdArgFlags result;
};

void PrintTo(const BofParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestBofParameter : public TestParameterCommand, public WithParamInterface<BofParameterInput>
{
private:
    ValueSaver<bool> m_saved_match_book{g_bof_match_book_images, true};
};

TEST_P(TestBofParameter, setsBookImageMatching)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_bof_match_book_images);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBofParameter,
    Values(BofParameterInput{"Invalid", "nobof=!", true, CmdArgFlags::BAD_ARG},
        BofParameterInput{"Yes", "nobof=y", false, CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<BofParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, setTrigArrayHonorsStringViewLength)
{
    ValueSaver saved_trig_index0{g_trig_index[0], TrigFn::IDENT};
    const std::string storage{"sin-suffix"};
    const std::string_view name{storage.data(), 3};

    set_trig_array(0, name);

    EXPECT_EQ(TrigFn::SIN, g_trig_index[0]);
}

struct FunctionParameterInput
{
    const char *name;
    const char *argument;
    std::vector<TrigFn> expected;
};

void PrintTo(const FunctionParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFunctionParameter : public TestParameterCommand, public WithParamInterface<FunctionParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<TrigFn> m_saved_first{g_trig_index[0]};
    ValueSaver<TrigFn> m_saved_second{g_trig_index[1]};
    ValueSaver<TrigFn> m_saved_third{g_trig_index[2]};
    ValueSaver<TrigFn> m_saved_fourth{g_trig_index[3]};
    ValueSaver<bool> m_saved_loaded{g_new_bifurcation_functions_loaded, false};
};

void TestFunctionParameter::SetUp()
{
    TestParameterCommand::SetUp();
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        g_trig_index[i] = TrigFn::IDENT;
    }
}

TEST_P(TestFunctionParameter, setsFunctions)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], g_trig_index[i]) << i;
    }
    EXPECT_TRUE(g_new_bifurcation_functions_loaded);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFunctionParameter,
    Values(FunctionParameterInput{"Sin", "function=sin", {TrigFn::SIN}},
        FunctionParameterInput{"SinCos", "function=sin/cos", {TrigFn::SIN, TrigFn::COS}},
        FunctionParameterInput{"SinCosTan", "function=sin/cos/tan", {TrigFn::SIN, TrigFn::COS, TrigFn::TAN}},
        FunctionParameterInput{
            "SinCosTanCot", "function=sin/cos/tan/cotan", {TrigFn::SIN, TrigFn::COS, TrigFn::TAN, TrigFn::COTAN}}),
    [](const TestParamInfo<FunctionParameterInput> &info) { return info.param.name; });

class TestOutsideParameter : public TestParameterCommand, public WithParamInterface<ColoringParameterInput>
{
private:
    ValueSaver<ColorMethod> m_saved_method{g_outside_method, static_cast<ColorMethod>(-9999)};
    ValueSaver<int> m_saved_color{g_outside_color, -9999};
};

TEST_P(TestOutsideParameter, setsOutsideColoring)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().method, g_outside_method);
    EXPECT_EQ(GetParam().color, g_outside_color);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideParameter,
    Values(ColoringParameterInput{"Real", "outside=real", ColorMethod::REAL, -1, CmdArgFlags::FRACTAL_PARAM},
        ColoringParameterInput{"Number", "outside=100", ColorMethod::COLOR, 100, CmdArgFlags::FRACTAL_PARAM},
        ColoringParameterInput{
            "Invalid", "outside=zmag", static_cast<ColorMethod>(-9999), -9999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<ColoringParameterInput> &info) { return info.param.name; });

struct BfDigitsParameterInput
{
    const char *name;
    const char *argument;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const BfDigitsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestBfDigitsParameter : public TestParameterCommand, public WithParamInterface<BfDigitsParameterInput>
{
private:
    ValueSaver<int> m_saved_digits{g_bf_digits, 9999};
};

TEST_P(TestBfDigitsParameter, setsPrecisionDigits)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_bf_digits);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBfDigitsParameter,
    Values(BfDigitsParameterInput{"Number", "bfdigits=200", 200, CmdArgFlags::FRACTAL_PARAM},
        BfDigitsParameterInput{"Invalid", "bfdigits=fmeh", 9999, CmdArgFlags::BAD_ARG},
        BfDigitsParameterInput{"TooSmall", "bfdigits=-1", 9999, CmdArgFlags::BAD_ARG},
        BfDigitsParameterInput{"TooLarge", "bfdigits=2001", 9999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<BfDigitsParameterInput> &info) { return info.param.name; });

struct MaxIterationsParameterInput
{
    const char *name;
    const char *argument;
    long expected;
    CmdArgFlags result;
};

void PrintTo(const MaxIterationsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMaxIterationsParameter : public TestParameterCommand, public WithParamInterface<MaxIterationsParameterInput>
{
private:
    ValueSaver<long> m_saved_iterations{g_max_iterations, 9999};
};

TEST_P(TestMaxIterationsParameter, setsIterationLimit)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_max_iterations);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMaxIterationsParameter,
    Values(MaxIterationsParameterInput{"Number", "maxiter=20", 20, CmdArgFlags::FRACTAL_PARAM},
        MaxIterationsParameterInput{"TooSmall", "maxiter=1", 9999, CmdArgFlags::BAD_ARG},
        MaxIterationsParameterInput{"NotNumber", "maxiter=fmeh", 9999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<MaxIterationsParameterInput> &info) { return info.param.name; });

struct PassesParameterInput
{
    const char *name;
    const char *argument;
    CalcMode mode;
    std::optional<int> stop_pass;
    std::optional<PerturbationMode> perturbation;
    CmdArgFlags result;
};

void PrintTo(const PassesParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPassesParameter : public TestParameterCommand, public WithParamInterface<PassesParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<CalcMode> m_saved_mode{g_user.std_calc_mode, static_cast<CalcMode>('Z')};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass};
    ValueSaver<PerturbationMode> m_saved_perturbation{g_user.perturbation};
};

void TestPassesParameter::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().stop_pass)
    {
        g_stop_pass = -1;
    }
    if (GetParam().perturbation)
    {
        g_user.perturbation = PerturbationMode::AUTO;
    }
}

TEST_P(TestPassesParameter, setsTraversalOptions)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().mode, g_user.std_calc_mode);
    if (GetParam().stop_pass)
    {
        EXPECT_EQ(*GetParam().stop_pass, g_stop_pass);
    }
    if (GetParam().perturbation)
    {
        EXPECT_EQ(*GetParam().perturbation, g_user.perturbation);
    }
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPassesParameter,
    Values(PassesParameterInput{"Invalid", "passes=!", static_cast<CalcMode>('Z'), std::nullopt, std::nullopt,
               CmdArgFlags::BAD_ARG},
        PassesParameterInput{
            "Missing", "passes=", static_cast<CalcMode>('Z'), std::nullopt, std::nullopt, CmdArgFlags::BAD_ARG},
        PassesParameterInput{
            "NonNumeric", "passes=gg", static_cast<CalcMode>('Z'), std::nullopt, std::nullopt, CmdArgFlags::BAD_ARG},
        PassesParameterInput{"BoundaryTrace", "passes=b", CalcMode::BOUNDARY_TRACE, std::nullopt, std::nullopt,
            CmdArgFlags::FRACTAL_PARAM},
        PassesParameterInput{
            "SolidGuess", "passes=g", CalcMode::SOLID_GUESS, 0, std::nullopt, CmdArgFlags::FRACTAL_PARAM},
        PassesParameterInput{
            "SolidGuess3", "passes=g3", CalcMode::SOLID_GUESS, 3, std::nullopt, CmdArgFlags::FRACTAL_PARAM},
        PassesParameterInput{"Perturbation", "passes=p", CalcMode::PERTURBATION, std::nullopt, PerturbationMode::YES,
            CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<PassesParameterInput> &info) { return info.param.name; });

struct PerturbationParameterInput
{
    const char *name;
    const char *argument;
    PerturbationMode initial;
    PerturbationMode expected;
    CmdArgFlags result;
};

void PrintTo(const PerturbationParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPerturbationParameter : public TestParameterCommand, public WithParamInterface<PerturbationParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<PerturbationMode> m_saved_perturbation{g_user.perturbation};
};

void TestPerturbationParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_user.perturbation = GetParam().initial;
}

TEST_P(TestPerturbationParameter, setsPerturbationMode)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_user.perturbation);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationParameter,
    Values(PerturbationParameterInput{"Auto", "perturbation=auto", PerturbationMode::YES, PerturbationMode::AUTO,
               CmdArgFlags::FRACTAL_PARAM},
        PerturbationParameterInput{
            "Yes", "perturbation=yes", PerturbationMode::AUTO, PerturbationMode::YES, CmdArgFlags::FRACTAL_PARAM},
        PerturbationParameterInput{
            "No", "perturbation=no", PerturbationMode::AUTO, PerturbationMode::NO, CmdArgFlags::FRACTAL_PARAM},
        PerturbationParameterInput{
            "Invalid", "perturbation=maybe", PerturbationMode::YES, PerturbationMode::YES, CmdArgFlags::BAD_ARG},
        PerturbationParameterInput{
            "Missing", "perturbation=", PerturbationMode::YES, PerturbationMode::YES, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<PerturbationParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, perturbationNoOverridesPassesPerturbation)
{
    ValueSaver saved_user_std_calc_mode{g_user.std_calc_mode, static_cast<CalcMode>('Z')};
    ValueSaver saved_user_perturbation{g_user.perturbation, PerturbationMode::AUTO};

    exec_cmd_arg("passes=p", CmdFile::AT_CMD_LINE);
    exec_cmd_arg("perturbation=no", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(CalcMode::PERTURBATION, g_user.std_calc_mode);
    EXPECT_EQ(PerturbationMode::NO, g_user.perturbation);
}

struct PerturbationToleranceParameterInput
{
    const char *name;
    const char *argument;
    double initial;
    double expected;
    CmdArgFlags result;
};

void PrintTo(const PerturbationToleranceParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPerturbationToleranceParameter : public TestParameterCommand,
                                           public WithParamInterface<PerturbationToleranceParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_tolerance{g_user.perturbation_tolerance};
};

void TestPerturbationToleranceParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_user.perturbation_tolerance = GetParam().initial;
}

TEST_P(TestPerturbationToleranceParameter, setsErrorTolerance)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_DOUBLE_EQ(GetParam().expected, g_user.perturbation_tolerance);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationToleranceParameter,
    Values(PerturbationToleranceParameterInput{"Value", "perturbation-tolerance=0.001", DEFAULT_PERTURBATION_TOLERANCE,
               0.001, CmdArgFlags::FRACTAL_PARAM},
        PerturbationToleranceParameterInput{"Invalid", "perturbation-tolerance=fmeh", 0.25, 0.25, CmdArgFlags::BAD_ARG},
        PerturbationToleranceParameterInput{"Missing", "perturbation-tolerance=", 0.25, 0.25, CmdArgFlags::BAD_ARG},
        PerturbationToleranceParameterInput{"Zero", "perturbation-tolerance=0", 0.25, 0.25, CmdArgFlags::BAD_ARG},
        PerturbationToleranceParameterInput{
            "Negative", "perturbation-tolerance=-0.001", 0.25, 0.25, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<PerturbationToleranceParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, isMandYes)
{
    ValueSaver saved_is_mandelbrot{g_frm_is_mandelbrot, false};

    exec_cmd_arg("ismand=y", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_TRUE(g_frm_is_mandelbrot);
}

struct CycleLimitParameterInput
{
    const char *name;
    const char *argument;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const CycleLimitParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCycleLimitParameter : public TestParameterCommand, public WithParamInterface<CycleLimitParameterInput>
{
private:
    ValueSaver<int> m_saved_limit{g_init_cycle_limit, 9999};
};

TEST_P(TestCycleLimitParameter, setsCycleLimit)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_init_cycle_limit);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCycleLimitParameter,
    Values(CycleLimitParameterInput{"TooLow", "cyclelimit=1", 9999, CmdArgFlags::BAD_ARG},
        CycleLimitParameterInput{"TooHigh", "cyclelimit=257", 9999, CmdArgFlags::BAD_ARG},
        CycleLimitParameterInput{"Number", "cyclelimit=100", 100, CmdArgFlags::NONE}),
    [](const TestParamInfo<CycleLimitParameterInput> &info) { return info.param.name; });

struct CycleRangeParameterInput
{
    const char *name;
    const char *argument;
    int low;
    int high;
    CmdArgFlags result;
};

void PrintTo(const CycleRangeParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCycleRangeParameter : public TestParameterCommand, public WithParamInterface<CycleRangeParameterInput>
{
private:
    ValueSaver<int> m_saved_low{g_color_cycle_range_lo, 9999};
    ValueSaver<int> m_saved_high{g_color_cycle_range_hi, 9999};
};

TEST_P(TestCycleRangeParameter, setsCycleRange)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().low, g_color_cycle_range_lo);
    EXPECT_EQ(GetParam().high, g_color_cycle_range_hi);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCycleRangeParameter,
    Values(CycleRangeParameterInput{"NoParams", "cyclerange", 1, 255, CmdArgFlags::NONE},
        CycleRangeParameterInput{"OneParam", "cyclerange=10", 10, 255, CmdArgFlags::NONE},
        CycleRangeParameterInput{"TwoParams", "cyclerange=10/20", 10, 20, CmdArgFlags::NONE},
        CycleRangeParameterInput{"LowTooLow", "cyclerange=-1/20", 9999, 9999, CmdArgFlags::BAD_ARG},
        CycleRangeParameterInput{"HighTooHigh", "cyclerange=10/256", 9999, 9999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<CycleRangeParameterInput> &info) { return info.param.name; });

struct IterationRangesParameterInput
{
    const char *name;
    const char *argument;
    long log_map_flag;
    std::optional<std::vector<int>> ranges;
    CmdArgFlags result;
};

void PrintTo(const IterationRangesParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestIterationRangesParameter : public TestParameterCommand,
                                     public WithParamInterface<IterationRangesParameterInput>
{
private:
    ValueSaver<long> m_saved_log_map{g_log_map_flag, 9999};
    ValueSaver<std::vector<int>> m_saved_ranges{g_iteration_ranges};
};

TEST_P(TestIterationRangesParameter, setsIterationRanges)
{
    exec_cmd_arg(GetParam().argument, CmdFile::AT_CMD_LINE);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().log_map_flag, g_log_map_flag);
    if (GetParam().ranges)
    {
        EXPECT_EQ(*GetParam().ranges, g_iteration_ranges);
    }
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestIterationRangesParameter,
    Values(
        IterationRangesParameterInput{"InvalidCount", "ranges=100/102/foo", 9999, std::nullopt, CmdArgFlags::BAD_ARG},
        IterationRangesParameterInput{
            "NoStriping", "ranges=10/20/30", 0, std::vector<int>{10, 20, 30}, CmdArgFlags::FRACTAL_PARAM},
        IterationRangesParameterInput{
            "OneStripe", "ranges=10/-20/30", 0, std::vector<int>{10, -1, 20, 30}, CmdArgFlags::FRACTAL_PARAM}),
    [](const TestParamInfo<IterationRangesParameterInput> &info) { return info.param.name; });

struct SaveNameParameterInput
{
    const char *name;
    CmdFile mode;
    bool first_init;
    std::optional<std::string> initial_name;
    const char *expected;
};

void PrintTo(const SaveNameParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSaveNameParameter : public TestParameterCommand, public WithParamInterface<SaveNameParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_first_init{g_first_init};
    ValueSaver<fs::path> m_saved_filename{g_save_filename};
};

void TestSaveNameParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_first_init = GetParam().first_init;
    if (GetParam().initial_name)
    {
        g_save_filename = *GetParam().initial_name;
    }
}

TEST_P(TestSaveNameParameter, setsSaveNameForSource)
{
    exec_cmd_arg("savename=test.gif", GetParam().mode);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().expected, g_save_filename);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSaveNameParameter,
    Values(SaveNameParameterInput{"FirstInit", CmdFile::AT_CMD_LINE, true, std::nullopt, "test.gif"},
        SaveNameParameterInput{"AfterStartup", CmdFile::AT_AFTER_STARTUP, false, "bar.gif", "test.gif"},
        SaveNameParameterInput{"OtherSource", CmdFile::AT_CMD_LINE_SET_NAME, false, "bar.gif", "bar.gif"}),
    [](const TestParamInfo<SaveNameParameterInput> &info) { return info.param.name; });

struct MinStackParameterInput
{
    const char *name;
    const char *argument;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const MinStackParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMinStackParameter : public TestParameterCommand, public WithParamInterface<MinStackParameterInput>
{
private:
    ValueSaver<int> m_saved_min_stack{g_soi_min_stack, 9999};
};

TEST_P(TestMinStackParameter, setsMinimumStack)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_soi_min_stack);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMinStackParameter,
    Values(MinStackParameterInput{"TooFew", "minstack=", 9999, CmdArgFlags::BAD_ARG},
        MinStackParameterInput{"TooMany", "minstack=10/20", 9999, CmdArgFlags::BAD_ARG},
        MinStackParameterInput{"Number", "minstack=200", 200, CmdArgFlags::NONE}),
    [](const TestParamInfo<MinStackParameterInput> &info) { return info.param.name; });

struct MathToleranceParameterInput
{
    const char *name;
    const char *argument;
    std::optional<double> second;
};

void PrintTo(const MathToleranceParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMathToleranceParameter : public TestParameterCommand, public WithParamInterface<MathToleranceParameterInput>
{
private:
    ValueSaver<double> m_saved_first{g_math_tol[0], 9999.0};
    ValueSaver<double> m_saved_second{g_math_tol[1], 8888.0};
};

TEST_P(TestMathToleranceParameter, setsExplicitToleranceComponents)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(9999.0, g_math_tol[0]);
    if (GetParam().second)
    {
        EXPECT_EQ(*GetParam().second, g_math_tol[1]);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMathToleranceParameter,
    Values(MathToleranceParameterInput{"OneValue", "mathtolerance=20", std::nullopt},
        MathToleranceParameterInput{"TwoValues", "mathtolerance=20/30", 30.0},
        MathToleranceParameterInput{"SecondOnly", "mathtolerance=/30", 30.0}),
    [](const TestParamInfo<MathToleranceParameterInput> &info) { return info.param.name; });

static std::string adjust_dir(std::string actual)
{
    if (actual[1] == ':')
    {
        actual[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(actual[0])));
    }
    fs::path sep;
    sep += fs::path::preferred_separator;
    if (actual.back() == sep.string().back())
    {
        actual.pop_back();
    }
    return actual;
}

class TestDirectoryParameter : public TestParameterCommand
{
protected:
    void SetUp() override;

    fs::path m_start_directory{ID_TEST_DATA_DIR};
    fs::path m_new_directory{ID_TEST_DATA_SUBDIR};

private:
    ValueSaver<fs::path> m_saved_temp_directory{g_temp_dir};
    ValueSaver<fs::path> m_saved_working_directory{g_working_dir};
};

void TestDirectoryParameter::SetUp()
{
    TestParameterCommand::SetUp();
    m_start_directory.make_preferred();
    m_new_directory.make_preferred();
}

TEST_F(TestDirectoryParameter, tempDirExisting)
{
    g_temp_dir = m_start_directory;

    exec_cmd_arg("tempdir=" + m_new_directory.string());

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(m_new_directory.string(), adjust_dir(g_temp_dir.string()));
}

class TestStartupCommand : public TestParameterCommand
{
private:
    ValueSaver<bool> m_saved_first_init{g_first_init, true};
    ValueSaver<std::shared_ptr<SpecialDirectories>> m_saved_special_dirs{g_special_dirs,
        std::make_shared<TestSpecialDirectories>(fs::path{ID_TEST_HOME_DIR}, fs::path{ID_TEST_DATA_DIR})};
    CurrentPathSaver m_current_directory{ID_TEST_HOME_DIR};
};

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

TEST_F(TestParameterCommandFile, parameterSetStopsAtAction)
{
    const CommandParseResult parsed{process_commands("test { maxiter=321 makemig=2/3 maxiter=654 }")};

    EXPECT_EQ(321, g_max_iterations);
    EXPECT_TRUE(bit_set(parsed.flags, CmdArgFlags::FRACTAL_PARAM));
    EXPECT_TRUE(std::holds_alternative<MakeMultiImageGif>(parsed.action));
}

TEST_F(TestDirectoryParameter, workDirExisting)
{
    g_working_dir = m_start_directory;

    exec_cmd_arg("workdir=" + m_new_directory.string());

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(m_new_directory.string(), adjust_dir(g_working_dir.string()));
}

class TextColorSaver
{
public:
    TextColorSaver();
    ~TextColorSaver();

private:
    Byte m_text_color[31]{};
};

TextColorSaver::TextColorSaver()
{
    std::copy(&g_text_color[0], &g_text_color[31], &m_text_color[0]);
}

TextColorSaver::~TextColorSaver()
{
    std::copy(&m_text_color[0], &m_text_color[31], &g_text_color[0]);
}

TEST_F(TestParameterCommand, textColorsMono)
{
    TextColorSaver saved_text_colors;

    exec_cmd_arg("textcolors=mono");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    for (int i : {1, 3, 4, 7, 8, 9, 10, 15, 18, 19, 21, 23, 26, 29, 30})
    {
        EXPECT_EQ(BLACK * 16 + WHITE, g_text_color[i]) << i;
    }
    for (int i : {6, 12, 13, 14, 20, 27, 28})
    {
        EXPECT_EQ(WHITE * 16 + BLACK, g_text_color[i]) << i;
    }
    for (int i : {0, 2, 5, 11, 16, 17, 22, 24, 25})
    {
        EXPECT_EQ(BLACK * 16 + LT_WHITE, g_text_color[i]) << i;
    }
}

struct TextColorsParameterInput
{
    const char *name;
    const char *argument;
    std::optional<Byte> first_color;
};

void PrintTo(const TextColorsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTextColorsParameter : public TestParameterCommand, public WithParamInterface<TextColorsParameterInput>
{
protected:
    void SetUp() override;

private:
    TextColorSaver m_saved_colors;
};

void TestTextColorsParameter::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().first_color)
    {
        g_text_color[0] = *GetParam().first_color;
    }
}

TEST_P(TestTextColorsParameter, setsExplicitTextColors)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(0x24, g_text_color[0]);
    EXPECT_EQ(0x28, g_text_color[1]);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTextColorsParameter,
    Values(TextColorsParameterInput{"Values", "textcolors=24/28", std::nullopt},
        TextColorsParameterInput{"SkippedValue", "textcolors=/28", Byte{0x24}}),
    [](const TestParamInfo<TextColorsParameterInput> &info) { return info.param.name; });

struct PotentialParameterInput
{
    const char *name;
    const char *argument;
    std::array<double, 3> expected;
    bool initial_16bit;
    bool expected_16bit;
};

void PrintTo(const PotentialParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPotentialParameter : public TestParameterCommand, public WithParamInterface<PotentialParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_first{g_potential.params[0], 9999.0};
    ValueSaver<double> m_saved_second{g_potential.params[1], 8888.0};
    ValueSaver<double> m_saved_third{g_potential.params[2], 7777.0};
    ValueSaver<bool> m_saved_16bit{g_potential.store_16bit};
};

void TestPotentialParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_potential.store_16bit = GetParam().initial_16bit;
}

TEST_P(TestPotentialParameter, setsPotentialComponents)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], g_potential.params[i]) << i;
    }
    EXPECT_EQ(GetParam().expected_16bit, g_potential.store_16bit);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialParameter,
    Values(PotentialParameterInput{"OneValue", "potential=111", {111.0, 8888.0, 7777.0}, true, false},
        PotentialParameterInput{"TwoValues", "potential=111/222", {111.0, 222.0, 7777.0}, true, false},
        PotentialParameterInput{"ThreeValues", "potential=111/222/333", {111.0, 222.0, 333.0}, true, false},
        PotentialParameterInput{"SixteenBit", "potential=111/222/333/16bit", {111.0, 222.0, 333.0}, false, true}),
    [](const TestParamInfo<PotentialParameterInput> &info) { return info.param.name; });

struct NumericParamsParameterInput
{
    const char *name;
    const char *argument;
    std::vector<double> initial;
    std::vector<double> expected;
};

void PrintTo(const NumericParamsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestNumericParamsParameter : public TestParameterCommand, public WithParamInterface<NumericParamsParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_first{g_params[0]};
    ValueSaver<double> m_saved_second{g_params[1]};
    ValueSaver<double> m_saved_third{g_params[2]};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
};

void TestNumericParamsParameter::SetUp()
{
    TestParameterCommand::SetUp();
    for (std::size_t i = 0; i < GetParam().initial.size(); ++i)
    {
        g_params[i] = GetParam().initial[i];
    }
}

TEST_P(TestNumericParamsParameter, setsAndZeroFillsParameters)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], g_params[i]) << i;
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestNumericParamsParameter,
    Values(NumericParamsParameterInput{"OneValue", "params=1.0", {1111.0, 2222.0}, {1.0, 0.0}},
        NumericParamsParameterInput{"TwoValues", "params=1.0/2.0", {1111.0, 2222.0, 3333.0}, {1.0, 2.0, 0.0}}),
    [](const TestParamInfo<NumericParamsParameterInput> &info) { return info.param.name; });

struct RetainedParamsInput
{
    const char *name;
    const char *argument;
    std::vector<std::string> expected;
};

void PrintTo(const RetainedParamsInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestRetainedParamsText : public TestParameterCommand, public WithParamInterface<RetainedParamsInput>
{
private:
    ValueSaver<std::array<std::string, MAX_PARAMS>> m_saved_text{g_param_text};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
};

TEST_P(TestRetainedParamsText, retainsOriginalRepresentation)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], g_param_text[i]) << i;
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestRetainedParamsText,
    Values(RetainedParamsInput{"Integers", "params=112112112/1200000/3/1/1/0",
               {"112112112", "1200000", "3", "1", "1", "0", ""}},
        RetainedParamsInput{"ExplicitDecimals", "params=112112112.000/1200000.0", {"112112112.000", "1200000.0", ""}}),
    [](const TestParamInfo<RetainedParamsInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, miimBreadthFirstLeftToRight)
{
    ValueSaver saved_major_method{g_major_method, Major::RANDOM_RUN};
    ValueSaver saved_minor_method{g_inverse_julia_minor_method, Minor::RIGHT_FIRST};

    exec_cmd_arg("miim=b/l");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(Major::BREADTH_FIRST, g_major_method);
    EXPECT_EQ(Minor::LEFT_FIRST, g_inverse_julia_minor_method);
}

struct InitOrbitParameterInput
{
    const char *name;
    const char *argument;
    InitOrbitMode initial;
    InitOrbitMode mode;
    DComplex expected;
    CmdArgFlags result;
};

void PrintTo(const InitOrbitParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInitOrbitParameter : public TestParameterCommand, public WithParamInterface<InitOrbitParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<InitOrbitMode> m_saved_mode{g_use_init_orbit};
    ValueSaver<DComplex> m_saved_orbit{g_init_orbit, DComplex{111.0, 222.0}};
};

void TestInitOrbitParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_use_init_orbit = GetParam().initial;
}

TEST_P(TestInitOrbitParameter, setsInitialOrbit)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().mode, g_use_init_orbit);
    EXPECT_EQ(GetParam().expected.x, g_init_orbit.x);
    EXPECT_EQ(GetParam().expected.y, g_init_orbit.y);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInitOrbitParameter,
    Values(InitOrbitParameterInput{"Pixel", "initorbit=pixel", InitOrbitMode::VALUE, InitOrbitMode::PIXEL,
               {111.0, 222.0}, CmdArgFlags::FRACTAL_PARAM},
        InitOrbitParameterInput{"Value", "initorbit=10/20", InitOrbitMode::PIXEL, InitOrbitMode::VALUE, {10.0, 20.0},
            CmdArgFlags::FRACTAL_PARAM},
        InitOrbitParameterInput{"TooFewParameters", "initorbit=10", InitOrbitMode::PIXEL, InitOrbitMode::PIXEL,
            {111.0, 222.0}, CmdArgFlags::BAD_ARG},
        InitOrbitParameterInput{"TooFewFloatParameters", "initorbit=10/fmeh", InitOrbitMode::PIXEL,
            InitOrbitMode::PIXEL, {111.0, 222.0}, CmdArgFlags::BAD_ARG},
        InitOrbitParameterInput{"TooManyParameters", "initorbit=10/20/30", InitOrbitMode::PIXEL, InitOrbitMode::PIXEL,
            {111.0, 222.0}, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<InitOrbitParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, threeDModeMonocular)
{
    ValueSaver saved_julibrot_3d_mode{g_julibrot_3d_mode, Julibrot3DMode::LEFT_EYE};

    exec_cmd_arg("3dmode=monocular");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(Julibrot3DMode::MONOCULAR, g_julibrot_3d_mode);
}

struct JulibrotGeometryParameterInput
{
    const char *name;
    const char *argument;
    std::vector<float> expected;
};

void PrintTo(const JulibrotGeometryParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestJulibrotGeometryParameter : public TestParameterCommand,
                                      public WithParamInterface<JulibrotGeometryParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<int> m_saved_z_dots{g_julibrot_z_dots, 9999};
    ValueSaver<float> m_saved_origin{g_julibrot_origin, 111.0F};
    ValueSaver<float> m_saved_depth{g_julibrot_depth};
    ValueSaver<float> m_saved_height{g_julibrot_height};
    ValueSaver<float> m_saved_width{g_julibrot_width};
    ValueSaver<float> m_saved_distance{g_julibrot_dist};
};

void TestJulibrotGeometryParameter::SetUp()
{
    TestParameterCommand::SetUp();
    const std::size_t count{GetParam().expected.size()};
    if (count >= 2)
    {
        g_julibrot_depth = 222.0F;
    }
    if (count >= 3)
    {
        g_julibrot_height = 333.0F;
    }
    if (count >= 4)
    {
        g_julibrot_width = 444.0F;
    }
    if (count >= 5)
    {
        g_julibrot_dist = 555.0F;
    }
}

TEST_P(TestJulibrotGeometryParameter, setsExplicitGeometry)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(100, g_julibrot_z_dots);
    const std::array<float, 5> actual{
        g_julibrot_origin, g_julibrot_depth, g_julibrot_height, g_julibrot_width, g_julibrot_dist};
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], actual[i]) << i;
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestJulibrotGeometryParameter,
    Values(JulibrotGeometryParameterInput{"ZDots", "julibrot3d=100", {111.0F}},
        JulibrotGeometryParameterInput{"Origin", "julibrot3d=100/10", {10.0F, 222.0F}},
        JulibrotGeometryParameterInput{"Depth", "julibrot3d=100/10/12", {10.0F, 12.0F, 333.0F}},
        JulibrotGeometryParameterInput{"Height", "julibrot3d=100/10/12/14", {10.0F, 12.0F, 14.0F, 444.0F}},
        JulibrotGeometryParameterInput{"Width", "julibrot3d=100/10/12/14/16", {10.0F, 12.0F, 14.0F, 16.0F, 555.0F}},
        JulibrotGeometryParameterInput{
            "Distance", "julibrot3d=100/10/12/14/16/18", {10.0F, 12.0F, 14.0F, 16.0F, 18.0F}}),
    [](const TestParamInfo<JulibrotGeometryParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, julibrotEyes)
{
    ValueSaver saved_eyes{g_eyes, 111.0F};

    exec_cmd_arg("julibroteyes=10");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(10.0F, g_eyes);
}

TEST_F(TestParameterCommand, julibrotFromToBadNumberOfValues)
{
    ValueSaver saved_julibrot_x_max{g_julibrot_x_max, 222.0F};
    ValueSaver saved_julibrot_x_min{g_julibrot_x_min, 111.0F};
    ValueSaver saved_julibrot_y_max{g_julibrot_y_max, 444.0F};
    ValueSaver saved_julibrot_y_min{g_julibrot_y_min, 333.0F};

    exec_cmd_arg("julibrotfromto=40/30/20/10");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(40.0, g_julibrot_x_max);
    EXPECT_EQ(30.0, g_julibrot_x_min);
    EXPECT_EQ(20.0, g_julibrot_y_max);
    EXPECT_EQ(10.0, g_julibrot_y_min);
}

TEST_F(TestParameterCommand, cornersNoValues)
{
    ValueSaver saved_use_center_mag{g_use_center_mag, true};

    exec_cmd_arg("corners=");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_FALSE(g_use_center_mag);
}

struct RegionParameterInput
{
    const char *name;
    const char *argument;
    ImageRegion expected;
};

void PrintTo(const RegionParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestImageCornersParameter : public TestParameterCommand, public WithParamInterface<RegionParameterInput>
{
private:
    ValueSaver<bool> m_saved_center_mag{g_use_center_mag, true};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{111.0, 444.0}, {333.0, 666.0}, {222.0, 555.0}}};
};

TEST_P(TestImageCornersParameter, setsImageCorners)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_FALSE(g_use_center_mag);
    EXPECT_EQ(GetParam().expected.m_min.x, g_image_region.m_min.x);
    EXPECT_EQ(GetParam().expected.m_max.x, g_image_region.m_max.x);
    EXPECT_EQ(GetParam().expected.m_min.y, g_image_region.m_min.y);
    EXPECT_EQ(GetParam().expected.m_max.y, g_image_region.m_max.y);
    EXPECT_EQ(GetParam().expected.m_3rd.x, g_image_region.m_3rd.x);
    EXPECT_EQ(GetParam().expected.m_3rd.y, g_image_region.m_3rd.y);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestImageCornersParameter,
    Values(RegionParameterInput{"FourValues", "corners=1/2/3/4", {{1.0, 3.0}, {2.0, 4.0}, {1.0, 3.0}}},
        RegionParameterInput{"SixValues", "corners=1/2/3/4/5/6", {{1.0, 3.0}, {2.0, 4.0}, {5.0, 6.0}}}),
    [](const TestParamInfo<RegionParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, cornersLongValueSelectsBigFloatPrecision)
{
    constexpr std::string_view x_min{"1.23456789012345678901"};
    constexpr int expected_decimals = static_cast<int>(x_min.size()) + 1;
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver saved_bf_digits{g_bf_digits, 0};
    ValueSaver saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver saved_decimals{g_decimals, 0};
    ValueSaver saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver saved_use_center_mag{g_use_center_mag, true};
    ValueSaver saved_x_min{g_image_region.m_min.x, 111.0};
    ValueSaver saved_x_3rd{g_image_region.m_3rd.x, 222.0};
    ValueSaver saved_x_max{g_image_region.m_max.x, 333.0};
    ValueSaver saved_y_min{g_image_region.m_min.y, 444.0};
    ValueSaver saved_y_3rd{g_image_region.m_3rd.y, 555.0};
    ValueSaver saved_y_max{g_image_region.m_max.y, 666.0};

    exec_cmd_arg("corners=" + std::string{x_min} + "/2/3/4");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(BFMathType::BIG_NUM, g_bf_math);
    EXPECT_EQ(expected_decimals, g_decimals);
    EXPECT_FALSE(g_use_center_mag);
}

class TestOrbitCornersParameter : public TestParameterCommand, public WithParamInterface<RegionParameterInput>
{
private:
    ValueSaver<bool> m_saved_set_corners{g_set_orbit_corners, false};
    ValueSaver<bool> m_saved_keep_coords{g_keep_screen_coords};
    ValueSaver<ImageRegion> m_saved_region{g_orbit_corner, ImageRegion{{999.0, 999.0}, {999.0, 999.0}, {999.0, 999.0}}};
};

TEST_P(TestOrbitCornersParameter, setsOrbitCorners)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_TRUE(g_set_orbit_corners);
    EXPECT_TRUE(g_keep_screen_coords);
    EXPECT_EQ(GetParam().expected.m_min.x, g_orbit_corner.m_min.x);
    EXPECT_EQ(GetParam().expected.m_max.x, g_orbit_corner.m_max.x);
    EXPECT_EQ(GetParam().expected.m_min.y, g_orbit_corner.m_min.y);
    EXPECT_EQ(GetParam().expected.m_max.y, g_orbit_corner.m_max.y);
    EXPECT_EQ(GetParam().expected.m_3rd.x, g_orbit_corner.m_3rd.x);
    EXPECT_EQ(GetParam().expected.m_3rd.y, g_orbit_corner.m_3rd.y);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitCornersParameter,
    Values(RegionParameterInput{"FourValues", "orbitcorners=1/2/3/4", {{1.0, 3.0}, {2.0, 4.0}, {1.0, 3.0}}},
        RegionParameterInput{"SixValues", "orbitcorners=1/2/3/4/5/6", {{1.0, 3.0}, {2.0, 4.0}, {5.0, 6.0}}}),
    [](const TestParamInfo<RegionParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, screenCoordsYes)
{
    ValueSaver saved_keep_screen_coords{g_keep_screen_coords, false};

    exec_cmd_arg("screencoords=y");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_TRUE(g_keep_screen_coords);
}

struct OrbitDrawModeParameterInput
{
    const char *name;
    const char *argument;
    OrbitDrawMode expected;
};

void PrintTo(const OrbitDrawModeParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrbitDrawModeParameter : public TestParameterCommand, public WithParamInterface<OrbitDrawModeParameterInput>
{
private:
    ValueSaver<OrbitDrawMode> m_saved_mode{g_draw_mode, static_cast<OrbitDrawMode>('!')};
};

TEST_P(TestOrbitDrawModeParameter, setsDrawMode)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().expected, g_draw_mode);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitDrawModeParameter,
    Values(OrbitDrawModeParameterInput{"Line", "orbitdrawmode=l", OrbitDrawMode::LINE},
        OrbitDrawModeParameterInput{"Rectangle", "orbitdrawmode=r", OrbitDrawMode::RECTANGLE},
        OrbitDrawModeParameterInput{"Function", "orbitdrawmode=f", OrbitDrawMode::FUNCTION}),
    [](const TestParamInfo<OrbitDrawModeParameterInput> &info) { return info.param.name; });

struct ViewWindowsParameterInput
{
    const char *name;
    const char *argument;
    bool initial_crop;
    float reduction;
    std::optional<bool> enabled;
    std::optional<float> screen_aspect;
    std::optional<float> final_aspect;
    std::optional<bool> crop;
    std::optional<int> x_dots;
    std::optional<int> y_dots;
};

void PrintTo(const ViewWindowsParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestViewWindowsParameter : public TestParameterCommand, public WithParamInterface<ViewWindowsParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<Viewport> m_saved_viewport{g_viewport};
    ValueSaver<float> m_saved_screen_aspect{g_screen_aspect, 0.75F};
};

void TestViewWindowsParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_viewport.enabled = false;
    g_viewport.reduction = 999.0F;
    g_viewport.final_aspect_ratio = 999.0F;
    g_viewport.crop = GetParam().initial_crop;
    g_viewport.x_dots = 999;
    g_viewport.y_dots = 999;
}

TEST_P(TestViewWindowsParameter, setsViewportComponents)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().reduction, g_viewport.reduction);
    if (GetParam().enabled)
    {
        EXPECT_EQ(*GetParam().enabled, g_viewport.enabled);
    }
    if (GetParam().screen_aspect)
    {
        EXPECT_EQ(*GetParam().screen_aspect, g_screen_aspect);
    }
    if (GetParam().final_aspect)
    {
        EXPECT_EQ(*GetParam().final_aspect, g_viewport.final_aspect_ratio);
    }
    if (GetParam().crop)
    {
        EXPECT_EQ(*GetParam().crop, g_viewport.crop);
    }
    if (GetParam().x_dots)
    {
        EXPECT_EQ(*GetParam().x_dots, g_viewport.x_dots);
    }
    if (GetParam().y_dots)
    {
        EXPECT_EQ(*GetParam().y_dots, g_viewport.y_dots);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestViewWindowsParameter,
    Values(ViewWindowsParameterInput{"Defaults", "viewwindows", false, 4.2F, true, 0.75F, 0.75F, true, 0, 0},
        ViewWindowsParameterInput{"OneValue", "viewwindows=2", false, 2.0F, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt, std::nullopt, std::nullopt},
        ViewWindowsParameterInput{"TwoValues", "viewwindows=2/3", false, 2.0F, std::nullopt, std::nullopt, 3.0F,
            std::nullopt, std::nullopt, std::nullopt},
        ViewWindowsParameterInput{"ThreeValues", "viewwindows=2/3/n", true, 2.0F, std::nullopt, std::nullopt, 3.0F,
            false, std::nullopt, std::nullopt},
        ViewWindowsParameterInput{"FourValues", "viewwindows=2/3/n/800", true, 2.0F, std::nullopt, std::nullopt, 3.0F,
            false, 800, std::nullopt},
        ViewWindowsParameterInput{
            "FiveValues", "viewwindows=2/3/n/800/600", true, 2.0F, std::nullopt, std::nullopt, 3.0F, false, 800, 600}),
    [](const TestParamInfo<ViewWindowsParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, centerMagOn)
{
    ValueSaver saved_use_center_mag{g_use_center_mag, false};
    ValueUnchanged saved_image_region("g_image_region", g_image_region, {});

    exec_cmd_arg("center-mag");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_TRUE(g_use_center_mag);
}

class TestCenterMagParameter : public TestParameterCommand, public WithParamInterface<RegionParameterInput>
{
private:
    ValueSaver<bool> m_saved_center_mag{g_use_center_mag, false};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{999.0, 999.0}, {999.0, 999.0}, {999.0, 999.0}}};
};

TEST_P(TestCenterMagParameter, setsImageRegionFromCenterAndMagnification)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_TRUE(g_use_center_mag);
    EXPECT_NEAR(GetParam().expected.m_min.x, g_image_region.m_min.x, 1e-6);
    EXPECT_NEAR(GetParam().expected.m_max.x, g_image_region.m_max.x, 1e-6);
    EXPECT_NEAR(GetParam().expected.m_3rd.x, g_image_region.m_3rd.x, 1e-6);
    EXPECT_NEAR(GetParam().expected.m_min.y, g_image_region.m_min.y, 1e-6);
    EXPECT_NEAR(GetParam().expected.m_max.y, g_image_region.m_max.y, 1e-6);
    EXPECT_NEAR(GetParam().expected.m_3rd.y, g_image_region.m_3rd.y, 1e-6);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCenterMagParameter,
    Values(RegionParameterInput{"ThreeValues", "center-mag=2/4/3",
               {{1.555555, 3.666666}, {2.444444, 4.333333}, {1.555555, 3.666666}}},
        RegionParameterInput{
            "FourValues", "center-mag=2/4/3/90", {{1.995061, 3.666666}, {2.004938, 4.333333}, {1.995061, 3.666666}}},
        RegionParameterInput{"FiveValues", "center-mag=2/4/3/90/45",
            {{2.232210, 3.760805}, {1.767789, 4.239194}, {1.760805, 3.767789}}}),
    [](const TestParamInfo<RegionParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, aspectDrift)
{
    ValueSaver saved_aspect_drift{g_aspect_drift, 999.0};

    exec_cmd_arg("aspectdrift=12");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(12.0F, g_aspect_drift);
}

struct InvertParameterInput
{
    const char *name;
    const char *argument;
    double radius;
    std::optional<DComplex> center;
    int invert;
};

void PrintTo(const InvertParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInvertParameter : public TestParameterCommand, public WithParamInterface<InvertParameterInput>
{
private:
    ValueSaver<double> m_saved_radius{g_inversion.params[0], 999.0};
    ValueSaver<double> m_saved_center_x{g_inversion.params[1], 999.0};
    ValueSaver<double> m_saved_center_y{g_inversion.params[2], 999.0};
    ValueSaver<int> m_saved_invert{g_inversion.invert, 999};
};

TEST_P(TestInvertParameter, setsInversionParameters)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().radius, g_inversion.params[0]);
    if (GetParam().center)
    {
        EXPECT_EQ(GetParam().center->x, g_inversion.params[1]);
        EXPECT_EQ(GetParam().center->y, g_inversion.params[2]);
    }
    EXPECT_EQ(GetParam().invert, g_inversion.invert);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInvertParameter,
    Values(InvertParameterInput{"ComputedRadius", "invert=-1", -1.0, std::nullopt, 1},
        InvertParameterInput{"DefaultCenter", "invert=1", 1.0, std::nullopt, 1},
        InvertParameterInput{"RadiusAndCenter", "invert=1/100/200", 1.0, DComplex{100.0, 200.0}, 3}),
    [](const TestParamInfo<InvertParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, oldDemmColorsYes)
{
    ValueSaver saved_old_demm_colors{g_old_demm_colors, false};

    exec_cmd_arg("olddemmcolors=y");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_TRUE(g_old_demm_colors);
}

struct LegacyFloatParameterInput
{
    const char *name;
    const char *argument;
    std::optional<bool> initial;
    std::optional<bool> expected;
    CmdArgFlags result;
};

void PrintTo(const LegacyFloatParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLegacyFloatParameter : public TestParameterCommand, public WithParamInterface<LegacyFloatParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_legacy_float{g_user.legacy_float};
};

void TestLegacyFloatParameter::SetUp()
{
    TestParameterCommand::SetUp();
    if (GetParam().initial)
    {
        g_user.legacy_float = *GetParam().initial;
    }
}

TEST_P(TestLegacyFloatParameter, setsLegacyFloatIntent)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    if (GetParam().expected)
    {
        EXPECT_EQ(*GetParam().expected, g_user.legacy_float);
    }
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLegacyFloatParameter,
    Values(LegacyFloatParameterInput{"Yes", "float=y", false, true, CmdArgFlags::NONE},
        LegacyFloatParameterInput{"No", "float=n", true, false, CmdArgFlags::NONE},
        LegacyFloatParameterInput{"BadArgument", "float=!", true, true, CmdArgFlags::BAD_ARG},
        LegacyFloatParameterInput{"BadValue", "float=foo", std::nullopt, std::nullopt, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<LegacyFloatParameterInput> &info) { return info.param.name; });

struct LegacyFloatBailoutInput
{
    const char *name;
    const char *params;
    long bailout;
    bool legacy_float;
};

void PrintTo(const LegacyFloatBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

using LegacyFloatBailoutParameter = std::tuple<std::string, LegacyFloatBailoutInput>;

class TestLegacyFloatBailout : public TestParameterCommand, public WithParamInterface<LegacyFloatBailoutParameter>
{
protected:
    void SetUp() override;

    std::unique_ptr<std::FILE, decltype(&std::fclose)> m_file{nullptr, std::fclose};

private:
    ValueSaver<UserData> m_saved_user{g_user};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_fractal_specific{g_cur_fractal_specific};
};

void TestLegacyFloatBailout::SetUp()
{
    TestParameterCommand::SetUp();
    m_file.reset(std::tmpfile());
    ASSERT_NE(nullptr, m_file);
}

TEST_P(TestLegacyFloatBailout, appliesLegacyTrigBailout)
{
    const LegacyFloatBailoutInput &input{std::get<1>(GetParam())};
    const std::string params{
        std::string{"{ "} + input.params + " type=" + std::get<0>(GetParam()) + " function=ident }"};
    SCOPED_TRACE(params);
    std::fputs(params.c_str(), m_file.get());
    std::rewind(m_file.get());
    g_user.legacy_float = true;

    CommandProcessor processor{m_file.release(), CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;
    const CmdArgFlags result{router.process(processor).flags};

    EXPECT_NE(CmdArgFlags::BAD_ARG, result);
    EXPECT_EQ(input.bailout, g_user.bailout_value);
    EXPECT_EQ(input.legacy_float, g_user.legacy_float);
}

INSTANTIATE_TEST_SUITE_P(TypesAndCommands, TestLegacyFloatBailout,
    Combine(Values(std::string{"mandelfn"}, std::string{"lambdafn"}),
        Values(LegacyFloatBailoutInput{"Reset", "reset", 0, false},
            LegacyFloatBailoutInput{"FloatNo", "reset float=n", 0, false},
            LegacyFloatBailoutInput{"FloatYes", "reset float=y", 2500, true},
            LegacyFloatBailoutInput{"YesThenNo", "reset float=y float=n", 0, false},
            LegacyFloatBailoutInput{"NoThenYes", "reset float=n float=y", 2500, true},
            LegacyFloatBailoutInput{"YesCustomBailout", "reset float=y bailout=64", 64, true},
            LegacyFloatBailoutInput{"NoCustomBailout", "reset float=n bailout=100", 100, false},
            LegacyFloatBailoutInput{"Reset1799", "reset=1799 float=y", 2500, true},
            LegacyFloatBailoutInput{"Reset1800", "reset=1800 float=y", 0, true},
            LegacyFloatBailoutInput{"Reset2004", "reset=2004 float=y", 0, true},
            LegacyFloatBailoutInput{"ResetId14", "reset=1/4 float=y", 0, true})),
    [](const TestParamInfo<LegacyFloatBailoutParameter> &info)
    { return std::get<0>(info.param) + "_" + std::get<1>(info.param).name; });

TEST_F(TestParameterCommand, fastRestoreYes)
{
    ValueSaver saved_fast_restore{g_fast_restore, false};

    exec_cmd_arg("fastrestore=y");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_TRUE(g_fast_restore);
}

TEST_F(TestParameterCommand, biomorph)
{
    ValueSaver saved_user_biomorph_value{g_user.biomorph_value, 999};

    exec_cmd_arg("biomorph=52");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(52, g_user.biomorph_value);
}

struct OrbitSaveParameterInput
{
    const char *name;
    const char *argument;
    int expected;
};

void PrintTo(const OrbitSaveParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrbitSaveParameter : public TestParameterCommand, public WithParamInterface<OrbitSaveParameterInput>
{
private:
    ValueSaver<int> m_saved_flags{g_orbit_save_flags, 0};
};

TEST_P(TestOrbitSaveParameter, setsSaveFlags)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().expected, g_orbit_save_flags);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitSaveParameter,
    Values(OrbitSaveParameterInput{"Raw", "orbitsave=y", OSF_RAW},
        OrbitSaveParameterInput{"Midi", "orbitsave=s", OSF_MIDI | OSF_RAW}),
    [](const TestParamInfo<OrbitSaveParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, orbitSaveName)
{
    ValueSaver saved_orbit_save_name{g_orbit_save_name, "foo.txt"};

    exec_cmd_arg("orbitsavename=smeagle.txt");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ("smeagle.txt", g_orbit_save_name);
}

TEST_F(TestParameterCommand, bailOut)
{
    ValueSaver saved_bailout{g_user.bailout_value, -1};

    exec_cmd_arg("bailout=50");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(50L, g_user.bailout_value);
}

struct BailoutTestParameterInput
{
    const char *name;
    const char *argument;
    Bailout expected;
    CmdArgFlags result;
};

void PrintTo(const BailoutTestParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestBailoutTestParameter : public TestParameterCommand, public WithParamInterface<BailoutTestParameterInput>
{
private:
    ValueSaver<Bailout> m_saved_bailout{g_bailout_test, Bailout::REAL};
};

TEST_P(TestBailoutTestParameter, setsBailoutSelector)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_bailout_test);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBailoutTestParameter,
    Values(BailoutTestParameterInput{"Mod", "bailoutest=mod", Bailout::MOD, CmdArgFlags::FRACTAL_PARAM},
        BailoutTestParameterInput{"Invalid", "bailoutest=foo", Bailout::REAL, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<BailoutTestParameterInput> &info) { return info.param.name; });

struct SymmetryParameterInput
{
    const char *name;
    const char *argument;
    SymmetryType expected;
    CmdArgFlags result;
};

void PrintTo(const SymmetryParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSymmetryParameter : public TestParameterCommand, public WithParamInterface<SymmetryParameterInput>
{
private:
    ValueSaver<SymmetryType> m_saved_symmetry{g_force_symmetry, SymmetryType::XY_AXIS};
};

TEST_P(TestSymmetryParameter, setsForcedSymmetry)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_force_symmetry);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSymmetryParameter,
    Values(SymmetryParameterInput{"XAxis", "symmetry=xaxis", SymmetryType::X_AXIS, CmdArgFlags::FRACTAL_PARAM},
        SymmetryParameterInput{"Invalid", "symmetry=fmeh", SymmetryType::XY_AXIS, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<SymmetryParameterInput> &info) { return info.param.name; });

struct SoundCommandParameterInput
{
    const char *name;
    const char *argument;
    int flags;
    std::optional<bool> buzzer;
};

void PrintTo(const SoundCommandParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSoundCommandParameter : public TestParameterCommand, public WithParamInterface<SoundCommandParameterInput>
{
private:
    ValueSaver<int> m_saved_sound_flags{g_sound_flag, 0};
};

TEST_P(TestSoundCommandParameter, setsSoundFlags)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().flags, g_sound_flag);
    if (GetParam().buzzer)
    {
        EXPECT_EQ(*GetParam().buzzer, sound_buzzer_enabled());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSoundCommandParameter,
    Values(SoundCommandParameterInput{"Yes", "sound=yes", SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER, true},
        SoundCommandParameterInput{"No", "sound=no", SOUNDFLAG_OFF | SOUNDFLAG_SPEAKER, false},
        SoundCommandParameterInput{"Beep", "sound=b", SOUNDFLAG_BEEP | SOUNDFLAG_SPEAKER, true},
        SoundCommandParameterInput{"OrbitX", "sound=x", SOUNDFLAG_X | SOUNDFLAG_SPEAKER, false},
        SoundCommandParameterInput{"OrbitY", "sound=y", SOUNDFLAG_Y | SOUNDFLAG_SPEAKER, false},
        SoundCommandParameterInput{"OrbitZ", "sound=z", SOUNDFLAG_Z | SOUNDFLAG_SPEAKER, false},
        SoundCommandParameterInput{"OrbitZMidi", "sound=z/m", SOUNDFLAG_Z | SOUNDFLAG_MIDI, std::nullopt},
        SoundCommandParameterInput{"OrbitZSpeaker", "sound=z/p", SOUNDFLAG_Z | SOUNDFLAG_SPEAKER, std::nullopt},
        SoundCommandParameterInput{"OrbitZQuantized", "sound=z/q", SOUNDFLAG_Z | SOUNDFLAG_QUANTIZED, std::nullopt}),
    [](const TestParamInfo<SoundCommandParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, hertzValue)
{
    ValueSaver saved_base_hertz{g_base_hertz, -999};

    exec_cmd_arg("hertz=100");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(100, g_base_hertz);
}

TEST_F(TestParameterCommand, volumeValue)
{
    ValueSaver saved_fm_volume{g_fm_volume, -999};

    exec_cmd_arg("volume=63");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(63, g_fm_volume);
}

struct AttenuateParameterInput
{
    const char *name;
    const char *argument;
    int expected;
};

void PrintTo(const AttenuateParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAttenuateParameter : public TestParameterCommand, public WithParamInterface<AttenuateParameterInput>
{
private:
    ValueSaver<int> m_saved_attenuation{g_hi_attenuation, -999};
};

TEST_P(TestAttenuateParameter, setsAttenuation)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().expected, g_hi_attenuation);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestAttenuateParameter,
    Values(AttenuateParameterInput{"No", "attenuate=n", 0}, AttenuateParameterInput{"Low", "attenuate=l", 1},
        AttenuateParameterInput{"Middle", "attenuate=m", 2}, AttenuateParameterInput{"High", "attenuate=h", 3}),
    [](const TestParamInfo<AttenuateParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, polyphonyValue)
{
    ValueSaver saved_polyphony{g_polyphony, -999};

    exec_cmd_arg("polyphony=1");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(0, g_polyphony);
}

TEST_F(TestParameterCommand, waveTypeValue)
{
    ValueSaver saved_fm_wave_type{g_fm_wave_type, -999};

    exec_cmd_arg("wavetype=4");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_fm_wave_type);
}

TEST_F(TestParameterCommand, attackValue)
{
    ValueSaver saved_fm_attack{g_fm_attack, -999};

    exec_cmd_arg("attack=4");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_fm_attack);
}

TEST_F(TestParameterCommand, decayValue)
{
    ValueSaver saved_fm_decay{g_fm_decay, -999};

    exec_cmd_arg("decay=4");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_fm_decay);
}

TEST_F(TestParameterCommand, sustainValue)
{
    ValueSaver saved_fm_sustain{g_fm_sustain, -999};

    exec_cmd_arg("sustain=4");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_fm_sustain);
}

TEST_F(TestParameterCommand, sReleaseValue)
{
    ValueSaver saved_fm_release{g_fm_release, -999};

    exec_cmd_arg("srelease=4");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_fm_release);
}

TEST_F(TestParameterCommand, scaleMapValues)
{
    ValueSaver saved_scale_map0{g_scale_map[0], -999};
    ValueSaver saved_scale_map1{g_scale_map[1], -999};
    ValueSaver saved_scale_map2{g_scale_map[2], -999};

    exec_cmd_arg("scalemap=4/5");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(4, g_scale_map[0]);
    EXPECT_EQ(5, g_scale_map[1]);
    EXPECT_EQ(-999, g_scale_map[2]);
}

struct PeriodicityParameterInput
{
    const char *name;
    const char *argument;
    int expected;
};

void PrintTo(const PeriodicityParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPeriodicityParameter : public TestParameterCommand, public WithParamInterface<PeriodicityParameterInput>
{
private:
    ValueSaver<int> m_saved_periodicity{g_user.periodicity_value, -999};
};

TEST_P(TestPeriodicityParameter, setsPeriodicity)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().expected, g_user.periodicity_value);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPeriodicityParameter,
    Values(PeriodicityParameterInput{"No", "periodicity=no", 0}, PeriodicityParameterInput{"Yes", "periodicity=yes", 1},
        PeriodicityParameterInput{"Show", "periodicity=show", -1},
        PeriodicityParameterInput{"Value", "periodicity=24", 24},
        PeriodicityParameterInput{"ShowValue", "periodicity=-24", -24},
        PeriodicityParameterInput{"ClampedHigh", "periodicity=500", 255},
        PeriodicityParameterInput{"ClampedLow", "periodicity=-500", -255}),
    [](const TestParamInfo<PeriodicityParameterInput> &info) { return info.param.name; });

struct LogMapParameterInput
{
    const char *name;
    const char *argument;
    long expected;
};

void PrintTo(const LogMapParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLogMapParameter : public TestParameterCommand, public WithParamInterface<LogMapParameterInput>
{
private:
    ValueSaver<bool> m_saved_auto_calculate{g_log_map_auto_calculate, true};
    ValueSaver<long> m_saved_map_flag{g_log_map_flag};
};

TEST_P(TestLogMapParameter, setsLogMap)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_FALSE(g_log_map_auto_calculate);
    EXPECT_EQ(GetParam().expected, g_log_map_flag);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLogMapParameter,
    Values(LogMapParameterInput{"No", "logmap=no", 0L}, LogMapParameterInput{"Yes", "logmap=yes", 1L},
        LogMapParameterInput{"Old", "logmap=old", -1L}, LogMapParameterInput{"Value", "logmap=15", 15L}),
    [](const TestParamInfo<LogMapParameterInput> &info) { return info.param.name; });

struct LogModeParameterInput
{
    const char *name;
    const char *argument;
    bool initial_auto;
    LogMapCalculate expected;
    bool expected_auto;
};

void PrintTo(const LogModeParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLogModeParameter : public TestParameterCommand, public WithParamInterface<LogModeParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<LogMapCalculate> m_saved_mode{g_log_map_fly_calculate, static_cast<LogMapCalculate>(-999)};
    ValueSaver<bool> m_saved_auto_calculate{g_log_map_auto_calculate};
};

void TestLogModeParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_log_map_auto_calculate = GetParam().initial_auto;
}

TEST_P(TestLogModeParameter, setsLogMapMode)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().expected, g_log_map_fly_calculate);
    EXPECT_EQ(GetParam().expected_auto, g_log_map_auto_calculate);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLogModeParameter,
    Values(LogModeParameterInput{"Fly", "logmode=f", true, LogMapCalculate::ON_THE_FLY, false},
        LogModeParameterInput{"Table", "logmode=t", true, LogMapCalculate::USE_LOG_TABLE, false},
        LogModeParameterInput{"Auto", "logmode=a", false, LogMapCalculate::NONE, true}),
    [](const TestParamInfo<LogModeParameterInput> &info) { return info.param.name; });

struct DebugFlagParameterInput
{
    const char *name;
    const char *argument;
    DebugFlags expected;
};

void PrintTo(const DebugFlagParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDebugFlagParameter : public TestParameterCommand, public WithParamInterface<DebugFlagParameterInput>
{
private:
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
};

TEST_P(TestDebugFlagParameter, setsDebugFlag)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().expected, g_debug_flag);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDebugFlagParameter,
    Values(DebugFlagParameterInput{"DebugAlias", "debug=300", DebugFlags::PREVENT_MIIM},
        DebugFlagParameterInput{"DebugFlag", "debugflag=300", DebugFlags::PREVENT_MIIM},
        DebugFlagParameterInput{"OddValue", "debugflag=301", static_cast<DebugFlags>(301)}),
    [](const TestParamInfo<DebugFlagParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, randomSeedValue)
{
    ValueSaver save_random_seed{g_random_seed, -999};
    ValueSaver save_random_seed_flag{g_random_seed_flag, false};

    exec_cmd_arg("rseed=301");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(301, g_random_seed);
    EXPECT_TRUE(g_random_seed_flag);
}

struct OrbitDelayParameterInput
{
    const char *name;
    const char *argument;
    int expected;
    CmdArgFlags result;
};

void PrintTo(const OrbitDelayParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrbitDelayParameter : public TestParameterCommand, public WithParamInterface<OrbitDelayParameterInput>
{
private:
    ValueSaver<int> m_saved_delay{g_orbit_delay, -999};
    ValueSaver<int> m_saved_skip_points{g_orbit_skip_points, -999};
};

TEST_P(TestOrbitDelayParameter, setsDelayAndSkipPoints)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().expected, g_orbit_delay);
    EXPECT_EQ(GetParam().expected, g_orbit_skip_points);
    if (GetParam().result == CmdArgFlags::BAD_ARG)
    {
        EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
        EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitDelayParameter,
    Values(OrbitDelayParameterInput{"Value", "orbitdelay=301", 301, CmdArgFlags::NONE},
        OrbitDelayParameterInput{"NotNumeric", "orbitdelay=100\n", -999, CmdArgFlags::BAD_ARG}),
    [](const TestParamInfo<OrbitDelayParameterInput> &info) { return info.param.name; });

struct OrbitIntervalParameterInput
{
    const char *name;
    const char *argument;
    long expected;
};

void PrintTo(const OrbitIntervalParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrbitIntervalParameter : public TestParameterCommand, public WithParamInterface<OrbitIntervalParameterInput>
{
private:
    ValueSaver<long> m_saved_interval{g_orbit_interval, -999L};
};

TEST_P(TestOrbitIntervalParameter, setsClampedInterval)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(GetParam().expected, g_orbit_interval);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitIntervalParameter,
    Values(OrbitIntervalParameterInput{"Value", "orbitinterval=201", 201L},
        OrbitIntervalParameterInput{"TooSmall", "orbitinterval=0", 1L},
        OrbitIntervalParameterInput{"TooBig", "orbitinterval=256", 255L}),
    [](const TestParamInfo<OrbitIntervalParameterInput> &info) { return info.param.name; });

struct ShowDotParameterInput
{
    const char *name;
    const char *argument;
    int initial_size;
    AutoShowDot mode;
    int size;
};

void PrintTo(const ShowDotParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestShowDotParameter : public TestParameterCommand, public WithParamInterface<ShowDotParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<int> m_saved_dot{g_show_dot, -999};
    ValueSaver<AutoShowDot> m_saved_mode{g_auto_show_dot, static_cast<AutoShowDot>('!')};
    ValueSaver<int> m_saved_size{g_size_dot};
};

void TestShowDotParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_size_dot = GetParam().initial_size;
}

TEST_P(TestShowDotParameter, setsDotSettings)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(15, g_show_dot);
    EXPECT_EQ(GetParam().mode, g_auto_show_dot);
    EXPECT_EQ(GetParam().size, g_size_dot);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShowDotParameter,
    Values(ShowDotParameterInput{"Default", "showdot", 10, static_cast<AutoShowDot>('!'), 10},
        ShowDotParameterInput{"Automatic", "showdot=a", -99, AutoShowDot::AUTOMATIC, 0},
        ShowDotParameterInput{"AutomaticSize", "showdot=a/20", -1, AutoShowDot::AUTOMATIC, 20}),
    [](const TestParamInfo<ShowDotParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, showOrbitYes)
{
    ValueSaver save_show_orbit{g_start_show_orbit, false};

    exec_cmd_arg("showorbit=y");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_TRUE(g_start_show_orbit);
}

struct DecompParameterInput
{
    const char *name;
    const char *argument;
    int second;
    long bailout;
};

void PrintTo(const DecompParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDecompParameter : public TestParameterCommand, public WithParamInterface<DecompParameterInput>
{
private:
    ValueSaver<int> m_saved_first{g_decomp[0], -99};
    ValueSaver<int> m_saved_second{g_decomp[1], -99};
    ValueSaver<long> m_saved_bailout{g_user.bailout_value, -99L};
};

TEST_P(TestDecompParameter, setsDecompositionAndOptionalBailout)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(16, g_decomp[0]);
    EXPECT_EQ(GetParam().second, g_decomp[1]);
    EXPECT_EQ(GetParam().bailout, g_user.bailout_value);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDecompParameter,
    Values(DecompParameterInput{"OneValue", "decomp=16", 0, -99L},
        DecompParameterInput{"TwoValues", "decomp=16/4", 4, 4L}),
    [](const TestParamInfo<DecompParameterInput> &info) { return info.param.name; });

struct DistEstParameterInput
{
    const char *name;
    const char *argument;
    int width;
    int x_dots;
    int y_dots;
};

void PrintTo(const DistEstParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDistEstParameter : public TestParameterCommand, public WithParamInterface<DistEstParameterInput>
{
private:
    ValueSaver<long> m_saved_distance{g_user.distance_estimator_value, -99L};
    ValueSaver<int> m_saved_width{g_distance_estimator_width_factor, -99};
    ValueSaver<int> m_saved_x_dots{g_distance_estimator_x_dots, -99};
    ValueSaver<int> m_saved_y_dots{g_distance_estimator_y_dots, -99};
};

TEST_P(TestDistEstParameter, setsDistanceEstimatorSettings)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(4L, g_user.distance_estimator_value);
    EXPECT_EQ(GetParam().width, g_distance_estimator_width_factor);
    EXPECT_EQ(GetParam().x_dots, g_distance_estimator_x_dots);
    EXPECT_EQ(GetParam().y_dots, g_distance_estimator_y_dots);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistEstParameter,
    Values(DistEstParameterInput{"OneValue", "distest=4", 71, 0, 0},
        DistEstParameterInput{"TwoValues", "distest=4/5", 5, 0, 0},
        DistEstParameterInput{"FourValues", "distest=4/5/6/7", 5, 6, 7}),
    [](const TestParamInfo<DistEstParameterInput> &info) { return info.param.name; });

struct FormulaFileParameterInput
{
    const char *name;
    std::string argument;
    fs::path expected;
};

void PrintTo(const FormulaFileParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFormulaFileParameter : public TestParameterCommand, public WithParamInterface<FormulaFileParameterInput>
{
private:
    ValueSaver<fs::path> m_saved_filename{g_formula_filename, fs::path{}};
};

TEST_P(TestFormulaFileParameter, setsFormulaFilename)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(GetParam().expected, g_formula_filename);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFormulaFileParameter,
    Values(FormulaFileParameterInput{"Existing", std::string{"formulafile="} + ID_TEST_FRM_FILE,
               home_file(ID_TEST_FRM_SUBDIR, ID_TEST_FRM_FILE)},
        FormulaFileParameterInput{"Missing", "formulafile=missing.frm", fs::path{"missing.frm"}}),
    [](const TestParamInfo<FormulaFileParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, formulaName)
{
    ValueSaver saved_formula_name{g_formula_name, ""};

    exec_cmd_arg("formulaname=Monongahela");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ("Monongahela", g_formula_name);
}

TEST_F(TestParameterCommand, lFileFilename)
{
    ValueSaver saved_l_system_filename{g_l_system_filename, ""};

    exec_cmd_arg(std::string{"lfile="} + ID_TEST_LSYSTEM_FILE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(home_file(ID_TEST_LSYSTEM_SUBDIR, ID_TEST_LSYSTEM_FILE), g_l_system_filename);
}

TEST_F(TestParameterCommand, lName)
{
    ValueSaver saved_l_system_name{g_l_system_name, ""};

    exec_cmd_arg("lname=Monongahela");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ("Monongahela", g_l_system_name);
}

TEST_F(TestParameterCommand, ifsFileFilename)
{
    ValueSaver saved_ifs_filename{g_ifs_filename, ""};

    exec_cmd_arg(std::string{"ifsfile="} + ID_TEST_IFS_FILE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(home_file(ID_TEST_IFS_SUBDIR, ID_TEST_IFS_FILE), g_ifs_filename);
}

struct IFSNameParameterInput
{
    const char *name;
    const char *argument;
};

void PrintTo(const IFSNameParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestIFSNameParameter : public TestParameterCommand, public WithParamInterface<IFSNameParameterInput>
{
private:
    ValueSaver<std::string> m_saved_name{g_ifs_name, std::string{}};
};

TEST_P(TestIFSNameParameter, setsIFSName)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ("Monongahela", g_ifs_name);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestIFSNameParameter,
    Values(IFSNameParameterInput{"IFS", "ifs=Monongahela"}, IFSNameParameterInput{"IFS3D", "ifs3d=Monongahela"}),
    [](const TestParamInfo<IFSNameParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, parmFile)
{
    ValueSaver saved_command_file{g_parameter_file, ""};

    exec_cmd_arg(std::string{"parmfile="} + ID_TEST_PAR_FILE);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(home_file(ID_TEST_PAR_SUBDIR, ID_TEST_PAR_FILE), g_parameter_file);
}

TEST_F(TestParameterCommand, stereoValue)
{
    ValueSaver saved_glasses_type{g_glasses_type, static_cast<GlassesType>(-1)};

    exec_cmd_arg("stereo=3");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, static_cast<int>(g_glasses_type));
    EXPECT_EQ(GlassesType::PHOTO, g_glasses_type);
}

TEST_F(TestParameterCommand, rotation)
{
    ValueSaver saved_x_rot{g_x_rot, -99};
    ValueSaver saved_y_rot{g_y_rot, -99};
    ValueSaver saved_z_rot{g_z_rot, -99};

    exec_cmd_arg("rotation=30/60/90");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(30, g_x_rot);
    EXPECT_EQ(60, g_y_rot);
    EXPECT_EQ(90, g_z_rot);
}

TEST_F(TestParameterCommand, perspective)
{
    ValueSaver saved_z_viewer{g_viewer_z, -99};

    exec_cmd_arg("perspective=90");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(90, g_viewer_z);
}

TEST_F(TestParameterCommand, xyShift)
{
    ValueSaver saved_x_shift{g_shift_x, -99};
    ValueSaver saved_y_shift{g_shift_y, -99};

    exec_cmd_arg("xyshift=30/90");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(30, g_shift_x);
    EXPECT_EQ(90, g_shift_y);
}

TEST_F(TestParameterCommand, interocular)
{
    ValueSaver saved_eye_separation{g_eye_separation, -99};

    exec_cmd_arg("interocular=90");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(90, g_eye_separation);
}

TEST_F(TestParameterCommand, converge)
{
    ValueSaver saved_converge_x_adjust{g_converge_x_adjust, -99};

    exec_cmd_arg("converge=90");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(90, g_converge_x_adjust);
}

TEST_F(TestParameterCommand, crop)
{
    ValueSaver saved_red_crop_left{g_red_crop_left, -99};
    ValueSaver saved_red_crop_right{g_red_crop_right, -99};
    ValueSaver saved_blue_crop_left{g_blue_crop_left, -99};
    ValueSaver saved_blue_crop_right{g_blue_crop_right, -99};

    exec_cmd_arg("crop=1/2/3/4");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_red_crop_left);
    EXPECT_EQ(2, g_red_crop_right);
    EXPECT_EQ(3, g_blue_crop_left);
    EXPECT_EQ(4, g_blue_crop_right);
}

TEST_F(TestParameterCommand, bright)
{
    ValueSaver saved_red_bright{g_red_bright, -99};
    ValueSaver saved_blue_bright{g_blue_bright, -99};

    exec_cmd_arg("bright=1/2");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_red_bright);
    EXPECT_EQ(2, g_blue_bright);
}

TEST_F(TestParameterCommand, xyAdjust)
{
    ValueSaver saved_adjust_3d_x{g_adjust_3d.x, -99};
    ValueSaver saved_adjust_3d_{g_adjust_3d.y, -99};

    exec_cmd_arg("xyadjust=1/2");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_adjust_3d.x);
    EXPECT_EQ(2, g_adjust_3d.y);
}

struct ThreeDParameterInput
{
    const char *name;
    const char *argument;
    bool initial_overlay;
    Display3DMode initial_display;
    std::optional<CalcStatus> calc_status;
    bool overlay;
    Display3DMode display;
    CmdArgFlags result;
};

void PrintTo(const ThreeDParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestThreeDParameter : public TestParameterCommand, public WithParamInterface<ThreeDParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_overlay{g_overlay_3d};
    ValueSaver<Display3DMode> m_saved_display{g_display_3d};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
};

void TestThreeDParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_overlay_3d = GetParam().initial_overlay;
    g_display_3d = GetParam().initial_display;
    if (GetParam().calc_status)
    {
        g_calc_status = *GetParam().calc_status;
    }
}

TEST_P(TestThreeDParameter, selects3DDisplay)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(GetParam().result, m_result);
    EXPECT_EQ(GetParam().overlay, g_overlay_3d);
    EXPECT_EQ(GetParam().display, g_display_3d);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestThreeDParameter,
    Values(ThreeDParameterInput{"No", "3d=no", true, Display3DMode::YES, std::nullopt, true, Display3DMode::NONE,
               CmdArgFlags::PARAM_3D},
        ThreeDParameterInput{"Yes", "3d=yes", false, Display3DMode::MINUS_ONE, std::nullopt, false, Display3DMode::YES,
            CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D},
        ThreeDParameterInput{"OverlayWithFractal", "3d=overlay", false, Display3DMode::MINUS_ONE,
            CalcStatus::PARAMS_CHANGED, true, Display3DMode::YES, CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D},
        ThreeDParameterInput{"OverlayNoFractal", "3d=overlay", false, Display3DMode::MINUS_ONE, CalcStatus::NO_FRACTAL,
            false, Display3DMode::YES, CmdArgFlags::PARAM_3D | CmdArgFlags::YES_3D}),
    [](const TestParamInfo<ThreeDParameterInput> &info) { return info.param.name; });

struct SphereParameterInput
{
    const char *name;
    const char *argument;
    bool initial;
    bool expected;
};

void PrintTo(const SphereParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSphereParameter : public TestParameterCommand, public WithParamInterface<SphereParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<bool> m_saved_sphere{g_sphere};
};

void TestSphereParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_sphere = GetParam().initial;
}

TEST_P(TestSphereParameter, setsSphereMode)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(GetParam().expected, g_sphere);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSphereParameter,
    Values(
        SphereParameterInput{"No", "sphere=no", true, false}, SphereParameterInput{"Yes", "sphere=yes", false, true}),
    [](const TestParamInfo<SphereParameterInput> &info) { return info.param.name; });

struct ScaleXYZParameterInput
{
    const char *name;
    const char *argument;
    int rough;
};

void PrintTo(const ScaleXYZParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestScaleXYZParameter : public TestParameterCommand, public WithParamInterface<ScaleXYZParameterInput>
{
private:
    ValueSaver<int> m_saved_x_scale{g_x_scale, -99};
    ValueSaver<int> m_saved_y_scale{g_y_scale, -99};
    ValueSaver<int> m_saved_rough{g_rough, -99};
};

TEST_P(TestScaleXYZParameter, setsExplicitScaleComponents)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_x_scale);
    EXPECT_EQ(2, g_y_scale);
    EXPECT_EQ(GetParam().rough, g_rough);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestScaleXYZParameter,
    Values(ScaleXYZParameterInput{"TwoValues", "scalexyz=1/2", -99},
        ScaleXYZParameterInput{"ThreeValues", "scalexyz=1/2/3", 3}),
    [](const TestParamInfo<ScaleXYZParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, roughness)
{
    ValueSaver saved_rough{g_rough, -99};

    exec_cmd_arg("roughness=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_rough);
}

TEST_F(TestParameterCommand, waterline)
{
    ValueSaver saved_water_line{g_water_line, -99};

    exec_cmd_arg("waterline=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_water_line);
}

TEST_F(TestParameterCommand, fillType)
{
    ValueSaver saved_fill_type{g_fill_type, static_cast<FillType>(-99)};

    exec_cmd_arg("filltype=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(FillType::SURFACE_CONSTANT, g_fill_type);
}

TEST_F(TestParameterCommand, lightSource)
{
    ValueSaver saved_light_x{g_light_x, -99};
    ValueSaver saved_light_y{g_light_y, -99};
    ValueSaver saved_light_z{g_light_z, -99};

    exec_cmd_arg("lightsource=1/2/3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_light_x);
    EXPECT_EQ(2, g_light_y);
    EXPECT_EQ(3, g_light_z);
}

TEST_F(TestParameterCommand, smoothing)
{
    ValueSaver saved_light_avg{g_light_avg, -99};

    exec_cmd_arg("smoothing=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_light_avg);
}

TEST_F(TestParameterCommand, latitude)
{
    ValueSaver saved_sphere_theta_min{g_sphere_theta_min, -99};
    ValueSaver saved_sphere_theta_max{g_sphere_theta_max, -99};

    exec_cmd_arg("latitude=1/3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_sphere_theta_min);
    EXPECT_EQ(3, g_sphere_theta_max);
}

TEST_F(TestParameterCommand, longitude)
{
    ValueSaver saved_sphere_phi_min{g_sphere_phi_min, -99};
    ValueSaver saved_sphere_phi_max{g_sphere_phi_max, -99};

    exec_cmd_arg("longitude=1/3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_sphere_phi_min);
    EXPECT_EQ(3, g_sphere_phi_max);
}

TEST_F(TestParameterCommand, radius)
{
    ValueSaver saved_sphere_radius{g_sphere_radius, -99};

    exec_cmd_arg("radius=1");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_sphere_radius);
}

struct TransparentParameterInput
{
    const char *name;
    const char *argument;
    int high;
};

void PrintTo(const TransparentParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTransparentParameter : public TestParameterCommand, public WithParamInterface<TransparentParameterInput>
{
private:
    ValueSaver<int> m_saved_low{g_transparent_color_3d[0], -99};
    ValueSaver<int> m_saved_high{g_transparent_color_3d[1], -99};
};

TEST_P(TestTransparentParameter, setsTransparentColorBounds)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_transparent_color_3d[0]);
    EXPECT_EQ(GetParam().high, g_transparent_color_3d[1]);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTransparentParameter,
    Values(TransparentParameterInput{"OneValue", "transparent=1", 1},
        TransparentParameterInput{"TwoValues", "transparent=1/2", 2}),
    [](const TestParamInfo<TransparentParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, previewNo)
{
    ValueSaver saved_preview{g_preview, true};

    exec_cmd_arg("preview=no");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_preview);
}

TEST_F(TestParameterCommand, showBoxNo)
{
    ValueSaver saved_show_box{g_show_box, true};

    exec_cmd_arg("showbox=no");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_show_box);
}

TEST_F(TestParameterCommand, coarse)
{
    ValueSaver saved_preview_factor{g_preview_factor, -99};

    exec_cmd_arg("coarse=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_preview_factor);
}

TEST_F(TestParameterCommand, randomize)
{
    ValueSaver saved_randomize_3d{g_randomize_3d, -99};

    exec_cmd_arg("randomize=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_randomize_3d);
}

TEST_F(TestParameterCommand, ambient)
{
    ValueSaver saved_ambient{g_ambient, -99};

    exec_cmd_arg("ambient=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_ambient);
}

TEST_F(TestParameterCommand, haze)
{
    ValueSaver saved_haze{g_haze, -99};

    exec_cmd_arg("haze=3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(3, g_haze);
}

TEST_F(TestParameterCommand, fullColorNo)
{
    ValueSaver saved_targa_out{g_targa_out, true};

    exec_cmd_arg("fullcolor=no");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_targa_out);
}

TEST_F(TestParameterCommand, trueColorNo)
{
    ValueSaver saved_true_color{g_true_color, true};

    exec_cmd_arg("truecolor=no");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_true_color);
}

struct TrueModeParameterInput
{
    const char *name;
    const char *argument;
    TrueColorMode initial;
    TrueColorMode expected;
};

void PrintTo(const TrueModeParameterInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTrueModeParameter : public TestParameterCommand, public WithParamInterface<TrueModeParameterInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<TrueColorMode> m_saved_mode{g_true_mode};
};

void TestTrueModeParameter::SetUp()
{
    TestParameterCommand::SetUp();
    g_true_mode = GetParam().initial;
}

TEST_P(TestTrueModeParameter, setsTrueColorMode)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(GetParam().expected, g_true_mode);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTrueModeParameter,
    Values(TrueModeParameterInput{"DefaultColor", "truemode=d", TrueColorMode::ITERATE, TrueColorMode::DEFAULT_COLOR},
        TrueModeParameterInput{"Iterate", "truemode=i", TrueColorMode::DEFAULT_COLOR, TrueColorMode::ITERATE},
        TrueModeParameterInput{"Value", "truemode=1", TrueColorMode::DEFAULT_COLOR, TrueColorMode::ITERATE}),
    [](const TestParamInfo<TrueModeParameterInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, useGrayScaleNo)
{
    ValueSaver saved_gray_flag{g_gray_flag, true};

    exec_cmd_arg("usegrayscale=n");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_gray_flag);
}

class TestStereoWidthParameter : public TestParameterCommand, public WithParamInterface<ParameterArgumentInput>
{
private:
    ValueSaver<double> m_saved_width{g_auto_stereo_width, 1.0};
};

TEST_P(TestStereoWidthParameter, setsStereoWidth)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(4.5, g_auto_stereo_width);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestStereoWidthParameter,
    Values(ParameterArgumentInput{"StereoWidth", "stereowidth=4.5"},
        ParameterArgumentInput{"MonitorWidthAlias", "monitorwidth=4.5"}),
    [](const TestParamInfo<ParameterArgumentInput> &info) { return info.param.name; });

TEST_F(TestParameterCommand, targaOverlayNo)
{
    ValueSaver saved_targa_overlay{g_targa_overlay, true};

    exec_cmd_arg("targa_overlay=n");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_targa_overlay);
}

TEST_F(TestParameterCommand, background)
{
    ValueSaver saved_background_color0{g_background_color[0], 255};
    ValueSaver saved_background_color1{g_background_color[1], 255};
    ValueSaver saved_background_color2{g_background_color[2], 255};

    exec_cmd_arg("background=1/2/3");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(1, g_background_color[0]);
    EXPECT_EQ(2, g_background_color[1]);
    EXPECT_EQ(3, g_background_color[2]);
}

TEST_F(TestParameterCommand, lightNameFirstInit)
{
    ValueSaver saved_first_init{g_first_init, true};
    ValueSaver saved_light_name{g_light_name, "fmeh"};

    exec_cmd_arg("lightname=foo", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ("foo", g_light_name);
}

TEST_F(TestParameterCommand, lightNameAfterStartup)
{
    ValueSaver saved_first_init{g_first_init, false};
    ValueSaver saved_light_name{g_light_name, "fmeh"};

    exec_cmd_arg("lightname=foo");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ("foo", g_light_name);
}

TEST_F(TestParameterCommand, lightNameNotSet)
{
    ValueSaver saved_first_init{g_first_init, false};
    VALUE_UNCHANGED(g_light_name, std::string{"fmeh"});

    exec_cmd_arg("lightname=foo", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
}

TEST_F(TestParameterCommand, ray)
{
    ValueSaver saved_raytrace_format{g_raytrace_format, RayTraceFormat::NONE};

    exec_cmd_arg("ray=3", CmdFile::AT_CMD_LINE);

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_EQ(RayTraceFormat::RAW, g_raytrace_format);
}

TEST_F(TestParameterCommandError, rayNegative)
{
    exec_cmd_arg("ray=-1");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
}

TEST_F(TestParameterCommandError, rayTooLarge)
{
    exec_cmd_arg("ray=8");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
}

TEST_F(TestParameterCommand, rayByName)
{
    ValueSaver saved_raytrace_format{g_raytrace_format, RayTraceFormat::RAYSHADE};

    exec_cmd_arg("ray=none");

    EXPECT_EQ(RayTraceFormat::NONE, g_raytrace_format);
}

TEST_F(TestParameterCommand, rayByNameDKB)
{
    ValueSaver saved_raytrace_format{g_raytrace_format, RayTraceFormat::RAYSHADE};

    exec_cmd_arg("ray=dkb");

    EXPECT_EQ(RayTraceFormat::DKB_POVRAY, g_raytrace_format);
}

TEST_F(TestParameterCommand, rayByNamePOVRay)
{
    ValueSaver saved_raytrace_format{g_raytrace_format, RayTraceFormat::RAYSHADE};

    exec_cmd_arg("ray=pov-ray");

    EXPECT_EQ(RayTraceFormat::DKB_POVRAY, g_raytrace_format);
}

TEST_F(TestParameterCommand, briefNo)
{
    ValueSaver saved_brief{g_brief, true};

    exec_cmd_arg("brief=n");

    EXPECT_EQ(CmdArgFlags::PARAM_3D, m_result);
    EXPECT_FALSE(g_brief);
}

TEST_F(TestParameterCommand, curDirNo)
{
    ValueSaver saved_check_cur_dir{g_check_cur_dir, true};

    exec_cmd_arg("curdir=n");

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_FALSE(g_check_cur_dir);
}

TEST_F(TestParameterCommand, saveDir)
{
    ValueSaver saved_save_dir{g_save_dir, "foo"};

    exec_cmd_arg(std::string{"savedir="} + ID_TEST_DATA_DIR);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(ID_TEST_DATA_DIR, g_save_dir);
}

TEST_F(TestParameterCommand, orbitNameJulia)
{
    ValueSaver saved_new_orbit_type{g_new_orbit_type, FractalType::LAMBDA};

    exec_cmd_arg("orbitname=julia");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM, m_result);
    EXPECT_EQ(FractalType::JULIA, g_new_orbit_type);
}

TEST_F(TestParameterCommand, colorsIncreasingSixBitRamp)
{
    ColorMapSaver saved_dac_box;

    exec_cmd_arg("colors=000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz");

    for (int i = 0; i < 256; ++i)
    {
        const Byte val = expand_6bit_color(i % 64U);
        EXPECT_EQ(val, g_dac_box[i][0]);
        EXPECT_EQ(val, g_dac_box[i][1]);
        EXPECT_EQ(val, g_dac_box[i][2]);
    }
}

TEST_F(TestParameterCommand, colorsHexValues)
{
    ColorMapSaver saved_dac_box;

    exec_cmd_arg("colors=#404040");

    EXPECT_EQ(0x40U, g_dac_box[0][0]);
    EXPECT_EQ(0x40U, g_dac_box[0][1]);
    EXPECT_EQ(0x40U, g_dac_box[0][2]);
}

TEST_F(TestParameterCommand, colorRangeHexValues)
{
    ColorMapSaver saved_dac_box;

    exec_cmd_arg("colors=000<4>#ffffff");

    constexpr Byte values[6]{0x00U, 0x33U, 0x66U, 0x99U, 0xCCU, 0xFFU};
    for (int i = 0; i < 6; ++i)
    {
        EXPECT_EQ(values[i], g_dac_box[i][0]);
        EXPECT_EQ(values[i], g_dac_box[i][1]);
        EXPECT_EQ(values[i], g_dac_box[i][2]);
    }
}

TEST_F(TestParameterCommand, color6BitMappedTo8Bit)
{
    ColorMapSaver saved_dac_box;

    exec_cmd_arg("colors=zzz");

    EXPECT_EQ(expand_6bit_color(63), g_dac_box[0][0]);
    EXPECT_EQ(expand_6bit_color(63), g_dac_box[0][1]);
    EXPECT_EQ(expand_6bit_color(63), g_dac_box[0][2]);
}

TEST_F(TestParameterCommand, singleReadLibraryDir)
{
    using Path = std::filesystem::path;
    clear_read_library_path();

    exec_cmd_arg(std::string{"librarydirs="} + library::ID_TEST_LIBRARY_DIR2);

    const Path path{find_file(ReadFile::FORMULA, ID_TEST_FRM_FILE)};
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{ID_TEST_FRM_FILE}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{test::library::ID_TEST_LIBRARY_DIR2}, path.parent_path().parent_path()) << path;
}

TEST_F(TestParameterCommand, multipleReadLibraryDir)
{
    using Path = std::filesystem::path;
    clear_read_library_path();

    exec_cmd_arg(std::string{"librarydirs="} + //
        library::ID_TEST_LIBRARY_DIR1 + "," + library::ID_TEST_LIBRARY_DIR3);

    const Path path{find_file(ReadFile::FORMULA, "root.frm")};
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Path{"root.frm"}, path.filename()) << path;
    EXPECT_EQ(Path{"formula"}, path.parent_path().filename()) << path;
    EXPECT_EQ(Path{test::library::ID_TEST_LIBRARY_DIR3}, path.parent_path().parent_path()) << path;
}

} // namespace id::test
