// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/loadfile.h>

#include "MockDriver.h"
#include "test_data.h"
#include "test_migrate_gif.h"

#include <engine/calc_frac_init.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/Inversion.h>
#include <engine/Potential.h>
#include <engine/random_seed.h>
#include <engine/spindac.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/lorenz.h>
#include <io/gif_extensions.h>
#include <io/gif_file.h>
#include <io/GifRestore.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/get_video_mode.h>
#include <ui/video_mode.h>
#include <ui/get_3d_params.h>
#include <ui/history.h>
#include <ui/id_keys.h>
#include <ui/make_batch_file.h>
#include <ui/restore_image.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::migrate_gif;
using namespace id::test::data;
using namespace id::ui;
using namespace testing;

namespace id::fractals
{

inline std::ostream &operator<<(std::ostream &str, const FractalType value)
{
    return str << +value;
}

void PrintTo(const FractalType value, std::ostream *str)
{
    *str << value;
}

} // namespace id::fractals

namespace id::test
{

struct FileFractalType
{
    const char *filename;
    FractalType type;
};

std::ostream &operator<<(std::ostream &str, const FileFractalType &value)
{
    return str << std::filesystem::path(value.filename).filename().string() << ", " << +value.type;
}

void PrintTo(const FileFractalType &value, std::ostream *str)
{
    *str << value;
}

static void check_gif_write(const int result)
{
    if (result == GIF_ERROR)
    {
        throw std::runtime_error("Can't write metadata test input");
    }
}

static void write_gif_extensions(GifFileType *output, const GifFileType *metadata)
{
    for (int i = 0; i < metadata->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{metadata->ExtensionBlocks[i]};
        if (block.Function != CONTINUE_EXT_FUNC_CODE)
        {
            if (i > 0)
            {
                check_gif_write(EGifPutExtensionTrailer(output));
            }
            check_gif_write(EGifPutExtensionLeader(output, block.Function));
        }
        check_gif_write(EGifPutExtensionBlock(output, block.ByteCount, block.Bytes));
    }
    if (metadata->ExtensionBlockCount > 0)
    {
        check_gif_write(EGifPutExtensionTrailer(output));
    }
}

TEST(TestBackwardsLegacyV20, oldMandelOutsideRealSetsBadOutside)
{
    ValueSaver saved_bad_outside{g_bad_outside, false};
    ValueSaver saved_file_version{g_file_version, Version{19, 60, 0, 0, true}};
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver saved_outside_method{g_outside_method, ColorMethod::REAL};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::ITER};
    ValueSaver saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded, true};

    backwards_legacy_v20();

    EXPECT_TRUE(g_bad_outside);
}

TEST(TestBackwardsLegacyV20, oldJuliaOutsideSummSetsBadOutside)
{
    ValueSaver saved_bad_outside{g_bad_outside, false};
    ValueSaver saved_file_version{g_file_version, Version{19, 60, 0, 0, true}};
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::JULIA};
    ValueSaver saved_outside_method{g_outside_method, ColorMethod::SUM};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::ITER};
    ValueSaver saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded, true};

    backwards_legacy_v20();

    EXPECT_TRUE(g_bad_outside);
}

TEST(TestBackwardsLegacyV20, modernMandelOutsideRealClearsBadOutside)
{
    ValueSaver saved_bad_outside{g_bad_outside, true};
    ValueSaver saved_file_version{g_file_version, Version{19, 61, 0, 0, true}};
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver saved_outside_method{g_outside_method, ColorMethod::REAL};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::ITER};
    ValueSaver saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded, true};

    backwards_legacy_v20();

    EXPECT_FALSE(g_bad_outside);
}

TEST(TestBackwardsLegacyV20, oldNonMandelOutsideRealClearsBadOutside)
{
    ValueSaver saved_bad_outside{g_bad_outside, true};
    ValueSaver saved_file_version{g_file_version, Version{19, 60, 0, 0, true}};
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::FN_PLUS_FN};
    ValueSaver saved_outside_method{g_outside_method, ColorMethod::REAL};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::ITER};
    ValueSaver saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded, true};

    backwards_legacy_v20();

    EXPECT_FALSE(g_bad_outside);
}

TEST(TestFractalInfoVersion, legacyFractintRelease)
{
    FractalInfo info{};
    info.info_version = FRACTAL_INFO_VERSION_LEGACY_20_4;
    info.release = 1960;

    EXPECT_EQ(parse_legacy_version(1960), fractal_info_version(info));
}

TEST(TestFractalInfoVersion, idOneOneUsesLegacyReleaseField)
{
    FractalInfo info{};
    info.info_version = FRACTAL_INFO_VERSION_LEGACY_20_4;
    info.release = 101;

    EXPECT_EQ((Version{1, 1, 0, 0, false}), fractal_info_version(info));
}

TEST(TestFractalInfoVersion, modernIdUsesFourVersionFields)
{
    FractalInfo info{};
    info.info_version = FRACTAL_INFO_VERSION;
    info.release = 2004;
    info.version_major = 1;
    info.version_minor = 2;
    info.version_patch = 3;
    info.version_tweak = 4;

    EXPECT_EQ((Version{1, 2, 3, 4, false}), fractal_info_version(info));
}

TEST(TestFractalInfoVersion, modernIdUsesLegacyVersionField)
{
    FractalInfo info{};
    info.info_version = FRACTAL_INFO_VERSION;
    info.release = 1730;

    EXPECT_EQ(parse_legacy_version(1730), fractal_info_version(info));
}

TEST(TestFractalInfoVersion, legacyInvalidReleaseFallsBackToFractintFourteenTen)
{
    FractalInfo info{};
    info.info_version = 5;
    info.release = 4000;

    EXPECT_EQ(parse_legacy_version(1410), fractal_info_version(info));
}

class TestGetVideoMode : public Test
{
protected:
    void SetUp() override;
    ImageMetadata metadata() const;

    ValueSaver<int> saved_init_mode{g_init_mode};
    ValueSaver<BatchMode> saved_init_batch{g_init_batch, BatchMode::NORMAL};
    ValueSaver<bool> saved_ask_video{g_ask_video, false};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file, false};
    ValueSaver<bool> saved_fast_restore{g_fast_restore, false};
    ValueSaver<CalcStatus> saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<Viewport> saved_viewport{g_viewport, Viewport{}};
    ValueSaver<VideoInfo> saved_video_entry{g_video_entry};
    ValueSaver<int> saved_video_table_len{g_video_table_len, 1};
    ValueSaver<VideoInfo> saved_video_mode{g_video_table[0]};
    ValueSaver<int> saved_video_key{g_video_mode_keys[0]};
    ValueSaver<int> saved_file_x_dots{g_file_x_dots, 65535};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots, 65535};
    ValueSaver<int> saved_file_colors{g_file_colors, 256};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio, 0.0f};
    ValueSaver<float> saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<int> saved_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver<int> saved_skip_y_dots{g_skip_y_dots, 0};
    FractalInfo m_info{};
    ExtBlock3 m_block_3_info{};
};

void TestGetVideoMode::SetUp()
{
    g_video_mode_keys[0] = ID_KEY_F2;
    g_video_table[0].x_dots = 800;
    g_video_table[0].y_dots = 600;
    g_video_table[0].colors = 256;
    g_video_table[0].driver = nullptr;
    m_info.x_dots = 800;
    m_info.y_dots = 600;
    m_info.colors = 256;
}

ImageMetadata TestGetVideoMode::metadata() const
{
    ImageMetadata data;
    data.fractal = m_info;
    data.fractal.info_version = 4;
    data.fractal.calc_status = static_cast<short>(g_calc_status);
    data.width = g_file_x_dots;
    data.height = g_file_y_dots;
    data.colors = g_file_colors;
    data.aspect_ratio = g_file_aspect_ratio;
    return data;
}

TEST_F(TestGetVideoMode, reducesLargeGifView)
{
    const std::optional<ImageVideoMode> setup{get_video_mode(metadata(), g_viewport)};

    ASSERT_TRUE(setup);
    EXPECT_EQ(596, setup->width);
    EXPECT_EQ(596, setup->height);
    EXPECT_EQ(109, setup->skip_x);
    EXPECT_EQ(109, setup->skip_y);
    EXPECT_EQ(65535, g_file_x_dots);
    EXPECT_EQ(65535, g_file_y_dots);
    EXPECT_EQ(0, g_skip_x_dots);
}

TEST_F(TestGetVideoMode, fitsOversizedGifWithUnequalSkipFactors)
{
    g_file_x_dots = 1600;
    g_file_y_dots = 1800;
    g_file_aspect_ratio = 0.75f;
    g_calc_status = CalcStatus::RESUMABLE;

    const std::optional<ImageVideoMode> setup{get_video_mode(metadata(), g_viewport)};

    ASSERT_TRUE(setup);
    EXPECT_EQ(800, setup->width);
    EXPECT_EQ(600, setup->height);
    EXPECT_EQ(1, setup->skip_x);
    EXPECT_EQ(2, setup->skip_y);
    EXPECT_EQ(CalcStatus::PARAMS_CHANGED, setup->calc_status);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
}

TEST_F(TestGetVideoMode, reducesMatchingGifIntoExplicitViewWindow)
{
    g_file_x_dots = 800;
    g_file_y_dots = 600;
    g_viewport.enabled = true;
    g_viewport.x_dots = 200;
    g_calc_status = CalcStatus::RESUMABLE;

    const std::optional<ImageVideoMode> setup{get_video_mode(metadata(), g_viewport)};

    ASSERT_TRUE(setup);
    EXPECT_TRUE(setup->viewport.enabled);
    EXPECT_FLOAT_EQ(4.0f, setup->viewport.reduction);
    EXPECT_EQ(0, setup->viewport.x_dots);
    EXPECT_EQ(0, setup->viewport.y_dots);
    EXPECT_EQ(3, setup->skip_x);
    EXPECT_EQ(3, setup->skip_y);
    EXPECT_EQ(CalcStatus::PARAMS_CHANGED, setup->calc_status);
    EXPECT_EQ(200, g_viewport.x_dots);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
}

class TestFindFractalInfo : public Test
{
protected:
    std::filesystem::path write_file(bool legacy_formula);
    std::filesystem::path write_small_file(bool extensions_first = false);
    void add_main_record();
    bool read_file(const std::filesystem::path &path);
    void expect_no_optional_blocks() const;
    void TearDown() override;

    ValueSaver<int> saved_file_x_dots{g_file_x_dots};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots};
    ValueSaver<int> saved_file_colors{g_file_colors};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file, false};
    FractalInfo m_info{};
    ExtBlock2 m_block_2{};
    ExtBlock3 m_block_3{};
    ExtBlock4 m_block_4{};
    ExtBlock5 m_block_5{};
    ExtBlock6 m_block_6{};
    ExtBlock7 m_block_7{};
    GifFileType m_extensions{};
    std::filesystem::path m_path;
};

std::filesystem::path TestFindFractalInfo::write_file(const bool legacy_formula)
{
    const std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    m_path = std::filesystem::temp_directory_path() / ("id-find-fractal-info-" + name + ".gif");
    int error{};
    std::unique_ptr<GifFileType, void (*)(GifFileType *)> input{DGifOpenFileName(ID_TEST_GIF_EXT3_FILE, &error),
        [](GifFileType *gif)
        {
            int close_error{};
            DGifCloseFile(gif, &close_error);
        }};
    if (!input || DGifSlurp(input.get()) == GIF_ERROR)
    {
        throw std::runtime_error("Can't read metadata test fixture");
    }
    std::unique_ptr<GifFileType, void (*)(GifFileType *)> output{
        EGifOpenFileName(m_path.string().c_str(), false, &error), [](GifFileType *gif)
        {
            int close_error{};
            EGifCloseFile(gif, &close_error);
        }};
    if (!output)
    {
        throw std::runtime_error("Can't create metadata test input");
    }
    EGifSetGifVersion(output.get(), true);
    check_gif_write(EGifPutScreenDesc(output.get(), input->SWidth, input->SHeight, input->SColorResolution,
        input->SBackGroundColor, input->SColorMap));
    for (int i = 0; i < input->ImageCount; ++i)
    {
        SavedImage &image{input->SavedImages[i]};
        const GifImageDesc &desc{image.ImageDesc};
        check_gif_write(
            EGifPutImageDesc(output.get(), desc.Left, desc.Top, desc.Width, desc.Height, false, desc.ColorMap));
        for (int row = 0; row < desc.Height; ++row)
        {
            check_gif_write(EGifPutLine(output.get(), image.RasterBits + row * desc.Width, desc.Width));
        }
    }
    if (legacy_formula)
    {
        // Name-only records predate the formula usage flags.
        FormulaInfo formula{get_formula_info(input.get())};
        char identifier[]{"fractint003"};
        check_gif_write(GifAddExtensionBlock(&output->ExtensionBlockCount, &output->ExtensionBlocks,
            APPLICATION_EXT_FUNC_CODE, 11, reinterpret_cast<Byte *>(identifier)));
        check_gif_write(GifAddExtensionBlock(&output->ExtensionBlockCount, &output->ExtensionBlocks,
            CONTINUE_EXT_FUNC_CODE, sizeof(formula.form_name), reinterpret_cast<Byte *>(formula.form_name)));
        FractalInfo info{get_fractal_info(input.get())};
        constexpr int MAIN_SIZE{GIF_EXTENSION1_FRACTAL_INFO_LENGTH};
        info.tot_extend_len = MAIN_SIZE + (MAIN_SIZE + 254) / 255 + 15 + sizeof(formula.form_name) + 16;
        put_fractal_info(output.get(), info);
    }
    write_gif_extensions(output.get(), output.get());
    check_gif_write(EGifCloseFile(output.release(), &error));
    return m_path;
}

std::filesystem::path TestFindFractalInfo::write_small_file(const bool extensions_first)
{
    const std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    m_path = std::filesystem::temp_directory_path() / ("id-find-fractal-info-" + name + ".gif");
    int error{};
    std::unique_ptr<GifFileType, void (*)(GifFileType *)> output{
        EGifOpenFileName(m_path.string().c_str(), false, &error), [](GifFileType *gif)
        {
            int close_error{};
            EGifCloseFile(gif, &close_error);
        }};
    const GifColorType colors[2]{{255, 0, 0}, {0, 255, 0}};
    std::unique_ptr<ColorMapObject, decltype(&GifFreeMapObject)> palette{GifMakeMapObject(2, colors), GifFreeMapObject};
    if (!output || !palette)
    {
        throw std::runtime_error("Can't create small metadata test input");
    }
    EGifSetGifVersion(output.get(), true);
    check_gif_write(EGifPutScreenDesc(output.get(), 2, 2, 1, 0, palette.get()));
    if (extensions_first)
    {
        write_gif_extensions(output.get(), &m_extensions);
    }
    check_gif_write(EGifPutImageDesc(output.get(), 0, 0, 2, 2, false, nullptr));
    Byte row[2]{0, 1};
    check_gif_write(EGifPutLine(output.get(), row, 2));
    check_gif_write(EGifPutLine(output.get(), row, 2));
    if (!extensions_first)
    {
        write_gif_extensions(output.get(), &m_extensions);
    }
    check_gif_write(EGifCloseFile(output.release(), &error));
    return m_path;
}

void TestFindFractalInfo::add_main_record()
{
    FractalInfo info{};
    std::copy_n(INFO_ID, sizeof(info.info_id), info.info_id);
    info.info_version = FRACTAL_INFO_VERSION;
    info.fractal_type = +FractalType::MANDEL;
    info.iterations_old = INITIAL_MAX_ITERATIONS;
    info.iterations = INITIAL_MAX_ITERATIONS;
    put_fractal_info(&m_extensions, info);
}

bool TestFindFractalInfo::read_file(const std::filesystem::path &path)
{
    return find_fractal_info(
        path.string(), &m_info, &m_block_2, &m_block_3, &m_block_4, &m_block_5, &m_block_6, &m_block_7);
}

void TestFindFractalInfo::expect_no_optional_blocks() const
{
    EXPECT_FALSE(m_block_2.got_data);
    EXPECT_FALSE(m_block_3.got_data);
    EXPECT_FALSE(m_block_4.got_data);
    EXPECT_FALSE(m_block_5.got_data);
    EXPECT_FALSE(m_block_6.got_data);
    EXPECT_FALSE(m_block_7.got_data);
}

void TestFindFractalInfo::TearDown()
{
    GifFreeExtensions(&m_extensions.ExtensionBlockCount, &m_extensions.ExtensionBlocks);
    if (!m_path.empty())
    {
        std::filesystem::remove(m_path);
    }
}

TEST_F(TestFindFractalInfo, metadataReadDoesNotChangeActiveImage)
{
    add_main_record();
    ValueSaver saved_type{g_fractal_type, FractalType::JULIA};
    ValueSaver saved_iterations{g_max_iterations, 1234L};
    ValueSaver saved_width{g_file_x_dots, 987};
    ValueSaver saved_height{g_file_y_dots, 654};
    ValueSaver saved_colors{g_file_colors, 256};
    ValueSaver saved_aspect{g_file_aspect_ratio, 0.75f};
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};

    const ImageMetadata metadata{read_image_metadata(write_small_file())};

    EXPECT_EQ(2, metadata.width);
    EXPECT_EQ(2, metadata.height);
    EXPECT_EQ(2, metadata.colors);
    EXPECT_EQ((std::array<Byte, 3>{255, 0, 0}), metadata.palette[0]);
    EXPECT_EQ(+FractalType::MANDEL, metadata.fractal.fractal_type);
    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(1234, g_max_iterations);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_EQ(654, g_file_y_dots);
    EXPECT_EQ(256, g_file_colors);
    EXPECT_FLOAT_EQ(0.75f, g_file_aspect_ratio);
}

TEST_F(TestFindFractalInfo, metadataRejectsInvalidTypeWithoutChangingActiveImage)
{
    FractalInfo info{};
    std::copy_n(INFO_ID, sizeof(info.info_id), info.info_id);
    info.fractal_type = +FractalType::MAX;
    put_fractal_info(&m_extensions, info);
    ValueSaver saved_type{g_fractal_type, FractalType::JULIA};
    ValueSaver saved_iterations{g_max_iterations, 1234L};
    ValueSaver saved_width{g_file_x_dots, 987};
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    const std::filesystem::path path{write_small_file()};

    EXPECT_THROW(read_image_metadata(path), std::runtime_error);

    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(1234, g_max_iterations);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestFindFractalInfo, malformedMetadataDoesNotChangeActiveImage)
{
    add_main_record();
    m_extensions.ExtensionBlocks[m_extensions.ExtensionBlockCount - 1].ByteCount = 1;
    ValueSaver saved_type{g_fractal_type, FractalType::JULIA};
    ValueSaver saved_width{g_file_x_dots, 987};
    MockDriver driver;
    ValueSaver saved_driver{g_driver, &driver};
    const std::filesystem::path path{write_small_file()};

    EXPECT_THROW(read_image_metadata(path), std::runtime_error);

    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestFindFractalInfo, metadataRetainsExtensionsUntilExplicitCommit)
{
    add_main_record();
    const std::vector<Byte> resume{1, 2, 3, 4};
    put_resume_info(&m_extensions, resume);
    const ImageMetadata metadata{read_image_metadata(write_small_file())};

    EXPECT_TRUE(metadata.resume.got_data);
    EXPECT_EQ(resume, metadata.resume.resume_data);
    EXPECT_FALSE(metadata.formula.got_data);
    EXPECT_FALSE(metadata.evolution.got_data);
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

TEST_F(TestFindFractalInfo, readsMainRecordWithoutOptionalBlocks)
{
    m_block_2.got_data = true;
    m_block_3.got_data = true;
    m_block_4.got_data = true;
    m_block_5.got_data = true;
    m_block_6.got_data = true;
    m_block_7.got_data = true;

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT1_FILE));

    EXPECT_STREQ("Fractal", m_info.info_id);
    EXPECT_EQ(17, m_info.info_version);
    EXPECT_EQ(0, m_info.fractal_type);
    EXPECT_EQ(640, g_file_x_dots);
    EXPECT_EQ(480, g_file_y_dots);
    EXPECT_EQ(256, g_file_colors);
    expect_no_optional_blocks();
}

TEST_F(TestFindFractalInfo, plainGifReceivesNonFractalDefaults)
{
    ASSERT_FALSE(read_file(write_file(false)));

    EXPECT_STREQ("GIFFILE", m_info.info_id);
    EXPECT_EQ(0, m_info.info_version);
    EXPECT_EQ(+FractalType::PLASMA, m_info.fractal_type);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, m_info.iterations);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, m_info.iterations_old);
    EXPECT_DOUBLE_EQ(-1.0, m_info.x_min);
    EXPECT_DOUBLE_EQ(1.0, m_info.x_max);
    EXPECT_DOUBLE_EQ(-1.0, m_info.y_min);
    EXPECT_DOUBLE_EQ(1.0, m_info.y_max);
    EXPECT_EQ(g_file_x_dots, m_info.x_dots);
    EXPECT_EQ(g_file_y_dots, m_info.y_dots);
    EXPECT_EQ(g_file_colors, m_info.colors);
    expect_no_optional_blocks();
}

TEST_F(TestFindFractalInfo, legacyNameOnlyRecordUsesDefaultFormulaFlags)
{
    ASSERT_FALSE(read_file(write_file(true)));

    EXPECT_STREQ("Fractal", m_info.info_id);
    ASSERT_TRUE(m_block_3.got_data);
    EXPECT_EQ(40, m_block_3.length);
    EXPECT_STREQ("Mandel3", m_block_3.form_name);
    EXPECT_EQ(1, m_block_3.uses_p1);
    EXPECT_EQ(1, m_block_3.uses_p2);
    EXPECT_EQ(1, m_block_3.uses_p3);
    EXPECT_EQ(0, m_block_3.uses_ismand);
    EXPECT_EQ(1, m_block_3.ismand);
    EXPECT_EQ(0, m_block_3.uses_p4);
    EXPECT_EQ(0, m_block_3.uses_p5);
    EXPECT_FALSE(m_block_2.got_data);
    EXPECT_FALSE(m_block_4.got_data);
    EXPECT_FALSE(m_block_5.got_data);
    EXPECT_FALSE(m_block_6.got_data);
    EXPECT_FALSE(m_block_7.got_data);
}

TEST_F(TestFindFractalInfo, readsLegacyOptionalRecords)
{
    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT2_FILE));
    ASSERT_TRUE(m_block_2.got_data);
    EXPECT_EQ(196, m_block_2.length);
    ASSERT_EQ(196, m_block_2.resume_data.size());
    const std::vector<Byte> resume_prefix{2, 0, 1, 0, 0, 0, 63, 6, 0, 0, 175, 4, 128, 2, 0, 0};
    EXPECT_EQ(resume_prefix,
        (std::vector<Byte>{m_block_2.resume_data.begin(), m_block_2.resume_data.begin() + resume_prefix.size()}));

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT3_FILE));
    ASSERT_TRUE(m_block_3.got_data);
    EXPECT_EQ(66, m_block_3.length);
    EXPECT_STREQ("Mandel3", m_block_3.form_name);
    EXPECT_EQ(0, m_block_3.uses_p1);
    EXPECT_EQ(1, m_block_3.ismand);

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT4_FILE));
    ASSERT_TRUE(m_block_4.got_data);
    EXPECT_EQ(5, m_block_4.length);
    EXPECT_EQ((std::vector<int>{10, 20, 30, 40, 50}), m_block_4.range_data);

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT5_FILE));
    ASSERT_TRUE(m_block_5.got_data);
    EXPECT_EQ(396, m_block_5.apm_data.size());

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT6_FILE));
    ASSERT_TRUE(m_block_6.got_data);
    EXPECT_EQ(200, m_block_6.length);
    EXPECT_EQ(1, m_block_6.evolving);
    EXPECT_EQ(9, m_block_6.image_grid_size);
    EXPECT_EQ(20596, m_block_6.this_generation_random_seed);
    EXPECT_DOUBLE_EQ(4.0, m_block_6.x_parameter_range);
    EXPECT_DOUBLE_EQ(3.0, m_block_6.y_parameter_range);
    EXPECT_DOUBLE_EQ(-2.0, m_block_6.x_parameter_offset);
    EXPECT_DOUBLE_EQ(-1.5, m_block_6.y_parameter_offset);
    EXPECT_EQ(4, m_block_6.px);
    EXPECT_EQ(4, m_block_6.py);
    EXPECT_EQ(640, m_block_6.x_dots);
    EXPECT_EQ(480, m_block_6.y_dots);
    EXPECT_EQ(81, m_block_6.e_count);
    EXPECT_EQ(5, m_block_6.mutate[0]);

    ASSERT_FALSE(read_file(ID_TEST_GIF_EXT7_FILE));
    ASSERT_TRUE(m_block_7.got_data);
    EXPECT_EQ(200, m_block_7.length);
    EXPECT_DOUBLE_EQ(-1.0, m_block_7.ox_min);
    EXPECT_DOUBLE_EQ(1.0, m_block_7.ox_max);
    EXPECT_DOUBLE_EQ(-1.0, m_block_7.oy_min);
    EXPECT_DOUBLE_EQ(1.0, m_block_7.oy_max);
    EXPECT_DOUBLE_EQ(-1.0, m_block_7.ox_3rd);
    EXPECT_DOUBLE_EQ(-1.0, m_block_7.oy_3rd);
    EXPECT_EQ(1, m_block_7.keep_screen_coords);
    EXPECT_EQ('r', m_block_7.draw_mode);
}

TEST_F(TestFindFractalInfo, extensionsBeforeImageRetainEmptyOptionalRecords)
{
    add_main_record();
    put_resume_info(&m_extensions, {});
    put_ranges_info(&m_extensions, {});
    put_extended_param_info(&m_extensions, {});

    ASSERT_FALSE(read_file(write_small_file(true)));

    EXPECT_STREQ(INFO_ID, m_info.info_id);
    EXPECT_TRUE(m_block_2.got_data);
    EXPECT_EQ(0, m_block_2.length);
    EXPECT_TRUE(m_block_2.resume_data.empty());
    EXPECT_TRUE(m_block_4.got_data);
    EXPECT_EQ(0, m_block_4.length);
    EXPECT_TRUE(m_block_4.range_data.empty());
    EXPECT_TRUE(m_block_5.got_data);
    EXPECT_TRUE(m_block_5.apm_data.empty());
    EXPECT_FALSE(m_block_3.got_data);
    EXPECT_FALSE(m_block_6.got_data);
    EXPECT_FALSE(m_block_7.got_data);
}

TEST_F(TestFindFractalInfo, readsMultiBlockPayloadsWithoutPollingPendingInput)
{
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ON_CALL(driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);
    const std::vector<Byte> resume(511, 0xA5);
    const std::vector<char> parameters(256, 'a');
    const std::vector<int> ranges{-1, 128, 255, 32767, -32768};
    put_resume_info(&m_extensions, resume);
    put_ranges_info(&m_extensions, ranges);
    put_extended_param_info(&m_extensions, parameters);
    add_main_record();
    const std::filesystem::path path{write_small_file()};

    ASSERT_FALSE(read_file(path));

    EXPECT_EQ(511, m_block_2.length);
    EXPECT_EQ(resume, m_block_2.resume_data);
    EXPECT_EQ(5, m_block_4.length);
    EXPECT_EQ(ranges, m_block_4.range_data);
    EXPECT_EQ(parameters, m_block_5.apm_data);
    EXPECT_TRUE(std::filesystem::remove(path));
    EXPECT_EQ(resume, m_block_2.resume_data);
}

TEST_F(TestFindFractalInfo, skipsInvalidLzwPixelsWhenReadingMetadata)
{
    add_main_record();
    const std::filesystem::path path{write_small_file()};
    {
        std::fstream file{path, std::ios::in | std::ios::out | std::ios::binary};
        // First compressed byte, after the descriptor and block length.
        file.seekp(31);
        file.put('\xFF');
    }
    GifRestore restore{path};
    while (!restore.done())
    {
        restore.iterate();
    }
    ASSERT_LT(restore.result(), 0);

    ASSERT_FALSE(read_file(path));

    EXPECT_STREQ(INFO_ID, m_info.info_id);
    EXPECT_EQ(FRACTAL_INFO_VERSION, m_info.info_version);
}

TEST_F(TestFindFractalInfo, plainGifUsesUnsignedHeaderDimensionsAspectAndPalette)
{
    ValueSaver saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver saved_red_0{g_dac_box[0][0]};
    ValueSaver saved_green_0{g_dac_box[0][1]};
    ValueSaver saved_blue_0{g_dac_box[0][2]};
    ValueSaver saved_red_1{g_dac_box[1][0]};
    ValueSaver saved_green_1{g_dac_box[1][1]};
    ValueSaver saved_blue_1{g_dac_box[1][2]};
    g_make_parameter_file = true;
    const std::filesystem::path path{write_small_file()};
    {
        std::fstream file{path, std::ios::in | std::ios::out | std::ios::binary};
        file.seekp(6);
        const char dimensions[]{'\xFF', '\xFF', 0, '\x80'};
        file.write(dimensions, sizeof(dimensions));
        file.seekp(12);
        file.put(49);
    }

    ASSERT_FALSE(read_file(path));

    EXPECT_EQ(65535, g_file_x_dots);
    EXPECT_EQ(32768, g_file_y_dots);
    EXPECT_EQ(65535, m_info.x_dots);
    EXPECT_EQ(32768, m_info.y_dots);
    EXPECT_FLOAT_EQ(32768.0f / 65535.0f, g_file_aspect_ratio);
    EXPECT_EQ(2, g_file_colors);
    EXPECT_EQ(255, g_dac_box[0][0]);
    EXPECT_EQ(0, g_dac_box[0][1]);
    EXPECT_EQ(0, g_dac_box[0][2]);
    EXPECT_EQ(0, g_dac_box[1][0]);
    EXPECT_EQ(255, g_dac_box[1][1]);
    EXPECT_EQ(0, g_dac_box[1][2]);
}

TEST_F(TestFindFractalInfo, readsUnsignedHeaderDimensionBoundaries)
{
    for (const int size : {32767, 32768, 65535})
    {
        SCOPED_TRACE(size);
        const std::filesystem::path path{write_small_file()};
        {
            std::fstream file{path, std::ios::in | std::ios::out | std::ios::binary};
            const Byte dimensions[]{static_cast<Byte>(size), static_cast<Byte>(size >> 8), static_cast<Byte>(size),
                static_cast<Byte>(size >> 8)};
            file.seekp(6);
            file.write(reinterpret_cast<const char *>(dimensions), sizeof(dimensions));
        }

        ASSERT_FALSE(read_file(path));

        EXPECT_EQ(size, g_file_x_dots);
        EXPECT_EQ(size, g_file_y_dots);
        EXPECT_EQ(size, m_info.x_dots);
        EXPECT_EQ(size, m_info.y_dots);
    }
}

TEST_F(TestFindFractalInfo, invalidHeaderOrMissingFileReportsFailure)
{
    for (const int offset : {0, 6, 8})
    {
        SCOPED_TRACE(offset);
        const std::filesystem::path path{write_small_file()};
        {
            std::fstream file{path, std::ios::in | std::ios::out | std::ios::binary};
            file.seekp(offset);
            file.put(0);
        }

        EXPECT_TRUE(read_file(path));
        EXPECT_TRUE(std::filesystem::remove(path));
    }
    EXPECT_TRUE(read_file(m_path));
}

TEST_F(TestFindFractalInfo, truncatedFilesReportFailureAndCloseAtEveryByteBoundary)
{
    add_main_record();
    const std::filesystem::path path{write_small_file()};
    std::ifstream input{path, std::ios::binary};
    const std::vector<char> bytes{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    input.close();
    for (std::size_t size = 0; size < bytes.size(); ++size)
    {
        SCOPED_TRACE(size);
        {
            std::ofstream output{path, std::ios::binary | std::ios::trunc};
            output.write(bytes.data(), static_cast<std::streamsize>(size));
        }
        EXPECT_TRUE(read_file(path));
        EXPECT_TRUE(std::filesystem::remove(path));
    }
    EXPECT_FALSE(read_file(write_small_file()));
}

TEST_F(TestFindFractalInfo, malformedFixedRecordReportsFailureAndClosesFile)
{
    add_main_record();
    FormulaInfo formula{};
    put_formula_info(&m_extensions, formula);
    m_extensions.ExtensionBlocks[m_extensions.ExtensionBlockCount - 1].ByteCount = 65;
    const std::filesystem::path path{write_small_file()};

    EXPECT_TRUE(read_file(path));

    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestFindFractalInfo, nonApplicationExtensionDoesNotSupplyResumeData)
{
    put_resume_info(&m_extensions, {1, 2, 3});
    m_extensions.ExtensionBlocks[0].Function = COMMENT_EXT_FUNC_CODE;
    add_main_record();

    ASSERT_FALSE(read_file(write_small_file()));

    expect_no_optional_blocks();
}

class TestLoadFile : public TestWithParam<FileFractalType>
{
public:
    ~TestLoadFile() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<ShowFile> saved_show_file{g_show_file};
    ValueSaver<int> saved_init_mode{g_init_mode};
    ValueSaver<bool> saved_loaded_3d{g_loaded_3d};
    ValueSaver<bool> saved_fast_restore{g_fast_restore};
    ValueSaver<bool> saved_view_window{g_viewport.enabled};
    ValueSaver<FractalType> saved_fractal_type{g_fractal_type, FractalType::JULIA};
    ValueSaver<const FractalSpecific *> saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA)};
    ValueSaver<int> saved_invert{g_inversion.invert};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file};
    ValueSaver<int> saved_colors{g_colors};
    ValueSaver<bool> saved_potential_flag{g_potential.flag};
    ValueSaver<int> saved_user_biomorph_value{g_user.biomorph_value};
    ValueSaver<long> saved_user_distance_estimator_value{g_user.distance_estimator_value};
    ValueSaver<bool> saved_legacy_float{g_user.legacy_float};
    ValueSaver<long> saved_calc_time{g_calc_time};
    ValueSaver<CalcStatus> saved_calc_status{g_calc_status};
    ValueSaver<bool> saved_potential_16bit{g_potential.store_16bit};
    ValueSaver<int> saved_file_x_dots{g_file_x_dots};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio};
    ValueSaver<bool> saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded};
    ValueSaver<double> saved_math_tol0{g_math_tol[0]};
    ValueSaver<double> saved_math_tol1{g_math_tol[1]};
    ValueSaver<Version> saved_version{g_version};
    ValueSaver<Version> saved_file_version{g_file_version};
    ValueSaver<bool> saved_ask_video{g_ask_video, false};
    ValueSaver<int> saved_video_table_len{g_video_table_len, 1};
    ValueSaver<int> saved_video_colors{g_video_table[0].colors, 256};
    ValueSaver<int> saved_video_width{g_video_table[0].x_dots, 800};
    ValueSaver<int> saved_video_height{g_video_table[0].y_dots, 600};
    ValueSaver<Driver *> saved_driver{g_driver, &m_driver};
};

void TestLoadFile::SetUp()
{
    EXPECT_CALL(m_driver, check_memory);
    EXPECT_CALL(m_driver, write_palette);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    history_init();
    save_history_info();
}

void TestLoadFile::TearDown()
{
    restore_history_info(g_history_ptr);
    Test::TearDown();
}

TEST_P(TestLoadFile, integerTypeMigrated)
{
    ValueSaver saved_read_filename{g_read_filename, GetParam().filename};

    const int result{prepare_image_restore()};

    EXPECT_EQ(0, result);
    EXPECT_EQ(GetParam().type, g_fractal_type);
    EXPECT_EQ(g_file_version, g_version);
}

TEST_F(TestLoadFile, randomSeedRestored)
{
    FractalInfo info{};
    ExtBlock2 block_2_info{};
    ExtBlock3 block_3_info{};
    ExtBlock4 block_4_info{};
    ExtBlock5 block_5_info{};
    ExtBlock6 block_6_info{};
    ExtBlock7 block_7_info{};
    ASSERT_FALSE(find_fractal_info(ID_TEST_GIF_MANDEL_INT, &info, &block_2_info, &block_3_info, //
        &block_4_info, &block_5_info, &block_6_info, &block_7_info));
    ValueSaver saved_read_filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver saved_random_seed{g_random_seed, -999};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    const int result{prepare_image_restore()};

    EXPECT_EQ(0, result);
    EXPECT_EQ(info.random_seed, g_random_seed);
    EXPECT_EQ(info.random_seed_flag != 0, g_random_seed_flag);
}

TEST_F(TestLoadFile, legacyFloatRestored)
{
    ValueSaver saved_read_filename{g_read_filename, ID_TEST_GIF_MANDELFN_INT};
    g_user.legacy_float = true;

    const int result{prepare_image_restore()};

    EXPECT_EQ(0, result);
    EXPECT_FALSE(g_user.legacy_float);
}

TEST_F(TestLoadFile, videoCancellationKeepsActiveImage)
{
    ValueSaver filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver ask{g_ask_video, true};
    ValueSaver batch{g_init_batch, BatchMode::NONE};
    ValueSaver mode_driver{g_video_table[0].driver, &m_driver};
    ValueSaver status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver iterations{g_max_iterations, 1234L};
    ValueSaver width{g_file_x_dots, 987};
    ValueSaver viewport{g_viewport, Viewport{}};
    const std::string description{"mock"};
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, get_description()).WillRepeatedly(ReturnRef(description));
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(1));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    EXPECT_EQ(-1, prepare_image_restore());

    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(1234, g_max_iterations);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(-1, g_init_mode);

    EXPECT_CALL(m_driver, check_memory());
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(1));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_EQ(0, prepare_image_restore());
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
    EXPECT_NE(1234, g_max_iterations);
}

TEST_F(TestLoadFile, threeDCancellationKeepsActiveImage)
{
    ValueSaver filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver display{g_display_3d, Display3DMode::YES};
    ValueSaver batch{g_init_batch, BatchMode::NONE};
    ValueSaver status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver iterations{g_max_iterations, 1234L};
    ValueSaver width{g_file_x_dots, 987};
    const ThreeDSettings before;
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ESC));

    EXPECT_EQ(-1, prepare_image_restore());

    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(1234, g_max_iterations);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(before.x_rot, id::geometry::g_x_rot);
    EXPECT_EQ(before.sphere, id::geometry::g_sphere);
    EXPECT_EQ(-1, g_init_mode);
}

TEST(TestThreeDSettings, editsRemainPendingUntilApplied)
{
    ValueSaver rotation{id::geometry::g_x_rot, 60};
    ValueSaver preview{id::geometry::g_preview, false};
    ValueSaver color_state{g_color_state, ColorState::DEFAULT_MAP};
    ThreeDSettings pending;

    pending.x_rot = 75;
    pending.preview = true;
    pending.color_state = ColorState::MAP_FILE;

    EXPECT_EQ(60, id::geometry::g_x_rot);
    EXPECT_FALSE(id::geometry::g_preview);
    EXPECT_EQ(ColorState::DEFAULT_MAP, g_color_state);
    pending.apply();
    EXPECT_EQ(75, id::geometry::g_x_rot);
    EXPECT_TRUE(id::geometry::g_preview);
    EXPECT_EQ(ColorState::MAP_FILE, g_color_state);
}

static FileFractalType s_int_types[]{
    {ID_TEST_GIF_MANDEL_INT, FractalType::MANDEL},                      //
    {ID_TEST_GIF_JULIA_INT, FractalType::JULIA},                        //
    {ID_TEST_GIF_LAMBDA_INT, FractalType::LAMBDA},                      //
    {ID_TEST_GIF_MANOWAR_INT, FractalType::MAN_O_WAR},                  //
    {ID_TEST_GIF_SIERPINSKI_INT, FractalType::SIERPINSKI},              //
    {ID_TEST_GIF_BARNSLEYM1_INT, FractalType::BARNSLEY_M1},             //
    {ID_TEST_GIF_BARNSLEYJ1_INT, FractalType::BARNSLEY_J1},             //
    {ID_TEST_GIF_BARNSLEYM2_INT, FractalType::BARNSLEY_M2},             //
    {ID_TEST_GIF_BARNSLEYJ2_INT, FractalType::BARNSLEY_J2},             //
    {ID_TEST_GIF_SQRTRIG_INT, FractalType::SQR_FN},                     //
    {ID_TEST_GIF_FNPLUSFN_INT, FractalType::FN_PLUS_FN},                //
    {ID_TEST_GIF_MANDELLAMBDA_INT, FractalType::MANDEL_LAMBDA},         //
    {ID_TEST_GIF_MARKSMANDEL_INT, FractalType::MARKS_MANDEL},           //
    {ID_TEST_GIF_MARKSJULIA_INT, FractalType::MARKS_JULIA},             //
    {ID_TEST_GIF_UNITY_INT, FractalType::UNITY},                        //
    {ID_TEST_GIF_MANDEL4_INT, FractalType::MANDEL4},                    //
    {ID_TEST_GIF_JULIA4_INT, FractalType::JULIA4},                      //
    {ID_TEST_GIF_BARNSLEYM3_INT, FractalType::BARNSLEY_M3},             //
    {ID_TEST_GIF_BARNSLEYJ3_INT, FractalType::BARNSLEY_J3},             //
    {ID_TEST_GIF_TRIGSQR_INT, FractalType::FN_Z_SQR},                   //
    {ID_TEST_GIF_FNTIMESFN_INT, FractalType::FN_TIMES_FN},              //
    {ID_TEST_GIF_SQR1FN_INT, FractalType::SQR_1_OVER_FN},               //
    {ID_TEST_GIF_FNMULZZ_INT, FractalType::FN_MUL_Z_PLUS_Z},            //
    {ID_TEST_GIF_KAM_INT, FractalType::KAM},                            //
    {ID_TEST_GIF_KAM3D_INT, FractalType::KAM_3D},                       //
    {ID_TEST_GIF_LAMBDAFN_INT, FractalType::LAMBDA_FN},                 //
    {ID_TEST_GIF_MANFNZSQRD_INT, FractalType::MANDEL_FN_PLUS_Z_SQRD},   //
    {ID_TEST_GIF_JULFNZSQRD_INT, FractalType::JULIA_FN_PLUS_Z_SQRD},    //
    {ID_TEST_GIF_MANDELFN_INT, FractalType::MANDEL_FN},                 //
    {ID_TEST_GIF_MANZPOWER_INT, FractalType::MANDEL_Z_POWER},           //
    {ID_TEST_GIF_JULZPOWER_INT, FractalType::JULIA_Z_POWER},            //
    {ID_TEST_GIF_MANFNPLUSEXP_INT, FractalType::MANDEL_FN_PLUS_EXP},    //
    {ID_TEST_GIF_JULFNPLUSEXP_INT, FractalType::JULIA_FN_PLUS_EXP},     //
    {ID_TEST_GIF_POPCORN_INT, FractalType::POPCORN},                    //
    {ID_TEST_GIF_LORENZ_INT, FractalType::LORENZ},                      //
    {ID_TEST_GIF_NEWTON_INT, FractalType::NEWTON},                      //
    {ID_TEST_GIF_NEWTBASIN_INT, FractalType::NEWT_BASIN},               //
    {ID_TEST_GIF_LORENZ3D_INT, FractalType::LORENZ_3D},                 //
    {ID_TEST_GIF_FORMULA_INT, FractalType::FORMULA},                    //
    {ID_TEST_GIF_JULIBROT_INT, FractalType::JULIBROT},                  //
    {ID_TEST_GIF_ROSSLER_INT, FractalType::ROSSLER},                    //
    {ID_TEST_GIF_HENON_INT, FractalType::HENON},                        //
    {ID_TEST_GIF_SPIDER_INT, FractalType::SPIDER},                      //
    {ID_TEST_GIF_BIFURCATION_INT, FractalType::BIFURCATION},            //
    {ID_TEST_GIF_BIFLAMBDA_INT, FractalType::BIF_LAMBDA},               //
    {ID_TEST_GIF_POPCORNJULIA_INT, FractalType::POPCORN_JUL},           //
    {ID_TEST_GIF_MANOWARJULIA_INT, FractalType::MAN_O_WAR_J},           //
    {ID_TEST_GIF_FNZFNPIX_INT, FractalType::FN_PLUS_FN_PIX},            //
    {ID_TEST_GIF_MARKSMANDELPOWER_INT, FractalType::MARKS_MANDEL_PWR},  //
    {ID_TEST_GIF_BIFEQSINPI_INT, FractalType::BIF_EQ_SIN_PI},           //
    {ID_TEST_GIF_BIFPLUSSINPI_INT, FractalType::BIF_PLUS_SIN_PI},       //
    {ID_TEST_GIF_BIFSTEWART_INT, FractalType::BIF_STEWART},             //
    {ID_TEST_GIF_LAMBDAFNFN_INT, FractalType::LAMBDA_FN_FN},            //
    {ID_TEST_GIF_JULIAFNFN_INT, FractalType::JUL_FN_FN},                //
    {ID_TEST_GIF_MANLAMFNFN_INT, FractalType::MAN_LAM_FN_FN},           //
    {ID_TEST_GIF_MANDELFNFN_INT, FractalType::MAN_FN_FN},               //
    {ID_TEST_GIF_BIFMAY_INT, FractalType::BIF_MAY},                     //
    {ID_TEST_GIF_HALLEYMP_INT, FractalType::HALLEY},                    //
    {ID_TEST_GIF_JULIAINVERSE_INT, FractalType::INVERSE_JULIA},         //
    {ID_TEST_GIF_PHOENIX_INT, FractalType::PHOENIX},                    //
    {ID_TEST_GIF_MANDPHOENIX_INT, FractalType::MAND_PHOENIX},           //
    {ID_TEST_GIF_FROTHYBASIN_INT, FractalType::FROTHY_BASIN},           //
    {ID_TEST_GIF_PHOENIXCMPLX_INT, FractalType::PHOENIX_CPLX},          //
    {ID_TEST_GIF_MANDPHOENIXCMPLX_INT, FractalType::MAND_PHOENIX_CPLX}, //
};

INSTANTIATE_TEST_SUITE_P(MigrateTypes, TestLoadFile, ValuesIn(s_int_types));

} // namespace id::test
