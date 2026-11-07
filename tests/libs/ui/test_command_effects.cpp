// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/command_effects.h>

#include <ColorMapSaver.h>
#include <CommandParameterTest.h>
#include <MockDriver.h>
#include <test_data.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/command_comments.h>
#include <engine/convert_center_mag.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/random_seed.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <io/check_write_file.h>
#include <io/encoder.h>
#include <io/gif_file.h>
#include <io/GifWriter.h>
#include <io/loadfile.h>
#include <io/loadmap.h>
#include <io/special_dirs.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/comments.h>
#include <ui/id_keys.h>
#include <ui/make_batch_file.h>
#include <ui/video_mode.h>

#include <gif_lib.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::misc;
using namespace id::math;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace fs = std::filesystem;
using namespace id::fractals;
using namespace id::io;
using namespace id::test::data;

namespace id::test
{

class TestCommandEffectsNoDriver : public Test
{
protected:
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
};

TEST_F(TestCommandEffectsNoDriver, noActionDoesNotRequestExit)
{
    CommandParseResult result;

    EXPECT_FALSE(apply_command_effects(result));
}

TEST_F(TestCommandEffectsNoDriver, pendingErrorsPreventGenerationAndDeviceInitialization)
{
    ValueSaver saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM | SOUNDFLAG_SPEAKER};
    CommandParseResult result;
    result.action = MakeHelpDocument{"must-not-be-written.txt"};
    result.initialize_fm = true;
    result.diagnostics.errors.push_back("invalid earlier argument");

    EXPECT_FALSE(apply_command_effects(result));
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("invalid earlier argument"));
    EXPECT_EQ(SOUNDFLAG_X | SOUNDFLAG_SPEAKER, g_sound_flag);
}

TEST_F(TestCommandEffectsNoDriver, pendingErrorsPreserveFmWithoutInitializationRequest)
{
    ValueSaver saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM};
    CommandParseResult result;
    result.diagnostics.errors.push_back("invalid earlier argument");

    EXPECT_FALSE(apply_command_effects(result));

    EXPECT_EQ(SOUNDFLAG_X | SOUNDFLAG_OPL3_FM, g_sound_flag);
}

class TestCommandEffectsFm : public TestWithParam<bool>
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_sound{g_sound_flag, SOUNDFLAG_X | SOUNDFLAG_OPL3_FM};
};

TEST_P(TestCommandEffectsFm, updatesRequestedDeviceBit)
{
    EXPECT_CALL(m_driver, init_fm()).WillOnce(Return(GetParam()));
    CommandParseResult result;
    result.initialize_fm = true;

    EXPECT_FALSE(apply_command_effects(result));

    EXPECT_EQ(GetParam() ? SOUNDFLAG_X | SOUNDFLAG_OPL3_FM : SOUNDFLAG_X, g_sound_flag);
}

INSTANTIATE_TEST_SUITE_P(Initialization, TestCommandEffectsFm, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "Succeeds" : "Fails"; });

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
    ValueSaver<int> m_saved_active_colors{g_colors};
    ColorMapSaver m_saved_palette;
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

    g_colors = 256;
    g_dac_box[0][0] = 42;
    exec_cmd_arg("makepar=" + m_parameter_path.filename().string() + "/view", CmdFile::SSTOOLS_INI);

    EXPECT_EQ(CmdArgFlags::NONE, m_result);
    EXPECT_EQ(16, g_colors);
    EXPECT_EQ(0, g_dac_box[0][0]);
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

TEST_F(TestParameterCommandFile, parameterErrorClearsUninitializedFmRequest)
{
    ValueSaver saved_sound{g_sound_flag, SOUNDFLAG_OFF};
    ValueSaver saved_first_init{g_first_init, false};
    CommandParseResult parsed{process_commands("entry { sound=x/f unknown=yes }")};
    ASSERT_TRUE(parsed.initialize_fm);
    ASSERT_EQ(1U, parsed.diagnostics.errors.size());
    const std::string diagnostic{parsed.diagnostics.errors.front()};
    ASSERT_THAT(diagnostic, HasSubstr("unknown=yes"));

    EXPECT_FALSE(apply_command_effects(parsed));

    EXPECT_EQ(SOUNDFLAG_X, g_sound_flag);
    EXPECT_THAT(parsed.diagnostics.errors, ElementsAre(diagnostic));
}

} // namespace id::test
