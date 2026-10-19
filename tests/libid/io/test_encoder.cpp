// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/encoder.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_limits.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractype.h>
#include <io/check_write_file.h>
#include <io/gif_extensions.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/save_timer.h>
#include <io/special_dirs.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/diskvid.h>
#include <ui/evolve.h>
#include <ui/slideshw.h>
#include <ui/temp_msg.h>
#include <ui/video.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

TEST(TestEncoder, saveInfoPreservesActiveIdCompatibilityVersion)
{
    ValueSaver saved_version{g_version, Version{1, 3, 2, 7, false}};
    FractalInfo info{};

    setup_save_info(&info);

    EXPECT_EQ(2004, info.release);
    EXPECT_EQ(1, info.version_major);
    EXPECT_EQ(3, info.version_minor);
    EXPECT_EQ(2, info.version_patch);
    EXPECT_EQ(7, info.version_tweak);
    EXPECT_EQ((Version{1, 3, 2, 7, false}), fractal_info_version(info));
}

TEST(TestEncoder, saveInfoPreservesActiveLegacyCompatibilityVersion)
{
    ValueSaver saved_version{g_version, parse_legacy_version(1730)};
    FractalInfo info{};

    setup_save_info(&info);

    EXPECT_EQ(1730, info.release);
    EXPECT_EQ(0, info.version_major);
    EXPECT_EQ(0, info.version_minor);
    EXPECT_EQ(0, info.version_patch);
    EXPECT_EQ(0, info.version_tweak);
    EXPECT_EQ(parse_legacy_version(1730), fractal_info_version(info));
}

class TestGifUint16 : public testing::TestWithParam<int>
{
};

TEST(TestEncoder, saveInfoPreservesLegacyFloat)
{
    ValueSaver saved_version{g_version, parse_legacy_version(1730)};
    ValueSaver saved_legacy_float{g_user.legacy_float};
    for (const bool legacy_float : {false, true})
    {
        g_user.legacy_float = legacy_float;
        FractalInfo info{};

        setup_save_info(&info);

        EXPECT_EQ(legacy_float ? 1 : 0, info.float_flag);
    }
}

TEST(TestEncoder, saveInfoUsesFloatForModernId)
{
    ValueSaver saved_version{g_version, Version{1, 4, 0, 0, false}};
    ValueSaver saved_legacy_float{g_user.legacy_float, false};
    FractalInfo info{};

    setup_save_info(&info);

    EXPECT_EQ(1, info.float_flag);
}

TEST_P(TestGifUint16, writesLittleEndianBytes)
{
    const int value{GetParam()};
    std::FILE *file{std::tmpfile()};
    ASSERT_NE(nullptr, file);

    ASSERT_TRUE(write_gif_uint16(file, value));
    std::rewind(file);

    std::array<unsigned char, 2> bytes{};
    EXPECT_EQ(1, std::fread(bytes.data(), bytes.size(), 1, file));
    std::fclose(file);

    EXPECT_EQ(value & 0xFF, bytes[0]);
    EXPECT_EQ(value >> 8, bytes[1]);
}

INSTANTIATE_TEST_SUITE_P(GifDimension, TestGifUint16, testing::Values(0, 32767, 32768, GIF_MAX_PIXELS));

TEST(TestEncoder, writeGifUint16RejectsOutOfRangeDimensions)
{
    std::FILE *file{std::tmpfile()};
    ASSERT_NE(nullptr, file);

    EXPECT_FALSE(write_gif_uint16(file, -1));
    EXPECT_FALSE(write_gif_uint16(file, GIF_MAX_PIXELS + 1));
    EXPECT_EQ(0, std::ftell(file));

    std::fclose(file);
}

namespace
{

using GifFilePtr = std::unique_ptr<GifFileType, void (*)(GifFileType *)>;

constexpr std::array<Byte, 3> EGA_PALETTE[]{
    {0, 0, 0},
    {0, 0, 42},
    {0, 42, 0},
    {0, 42, 42},
    {42, 0, 0},
    {42, 0, 42},
    {42, 21, 0},
    {42, 42, 42},
    {21, 21, 21},
    {21, 21, 63},
    {21, 63, 21},
    {21, 63, 63},
    {63, 21, 21},
    {63, 21, 63},
    {63, 63, 21},
    {63, 63, 63},
};

} // namespace

class TestGifWriterRaster : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void configure_image(int width, int height, int colors, int x_offset = 0, int y_offset = 0);
    void expect_failed_save_feedback();
    int save();
    GifFilePtr read_saved_gif() const;
    std::vector<Byte> image_pixels() const;
    void expect_raster(const GifFileType &gif, const std::vector<Byte> &pixels, int width, int height) const;

    MockDriver m_driver;
    std::filesystem::path m_directory;
    std::filesystem::path m_path;
    int m_frame_width{};
    std::vector<Byte> m_frame;
    Byte m_saved_palette[256][3]{};
    bool m_memory_initialized{};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen};
    ValueSaver<int> m_saved_x_dots{g_screen_x_dots};
    ValueSaver<int> m_saved_y_dots{g_screen_y_dots};
    ValueSaver<int> m_saved_colors{g_colors};
    ValueSaver<int> m_saved_and_color{g_and_color};
    ValueSaver<VideoInfo> m_saved_video{g_video_entry};
    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{}};
    ValueSaver<float> m_saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<bool> m_saved_real_dac{g_got_real_dac, false};
    ValueSaver<bool> m_saved_disk_16_bit{g_disk_16_bit, false};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<bool> m_saved_busy{g_busy, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NORMAL};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<TimedSave> m_saved_timed_save{g_timed_save, TimedSave::STARTED};
    ValueSaver<TimedSave> m_saved_resave{g_resave_flag, TimedSave::NONE};
    ValueSaver<bool> m_saved_started_resaves{g_started_resaves, false};
    ValueSaver<int> m_saved_save_time{g_save_time_interval, 0};
    ValueSaver<bool> m_saved_overwrite{g_overwrite_file, true};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<BFMathType> m_saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver<EvolutionModeFlags> m_saved_evolving{g_evolving, EvolutionModeFlags::NONE};
    ValueSaver<std::filesystem::path> m_saved_save_dir{g_save_dir};
    ValueSaver<std::filesystem::path> m_saved_temp_dir{g_temp_dir};
};

void TestGifWriterRaster::SetUp()
{
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_directory = std::filesystem::temp_directory_path() / ("id-encoder-" + name);
    std::filesystem::create_directories(m_directory);
    m_path = m_directory / "image.gif";
    g_save_dir = m_directory;
    g_temp_dir = m_directory;
    set_save_library(m_directory);
    std::memcpy(m_saved_palette, g_dac_box, sizeof(m_saved_palette));
    for (int color = 0; color < 256; ++color)
    {
        for (int channel = 0; channel < 3; ++channel)
        {
            g_dac_box[color][channel] = static_cast<Byte>((color * 29 + channel * 83) % 256);
        }
    }
    set_normal_dot();
    set_normal_span();
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            [this](const int x, const int y) { return m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x); });
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly([this](const int x, const int y, const int color)
            { m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color); });
    configure_image(8, 6, 256);
}

void TestGifWriterRaster::TearDown()
{
    if (m_memory_initialized)
    {
        end_disk();
        exit_check();
    }
    set_null_video();
    free_temp_msg();
    clear_save_library();
    std::memcpy(g_dac_box, m_saved_palette, sizeof(m_saved_palette));
    std::filesystem::remove_all(m_directory);
}

void TestGifWriterRaster::configure_image(
    const int width, const int height, const int colors, const int x_offset, const int y_offset)
{
    g_logical_screen = {
        width, height, x_offset, y_offset, static_cast<double>(width - 1), static_cast<double>(height - 1)};
    g_screen_x_dots = width + x_offset;
    g_screen_y_dots = height + y_offset;
    g_colors = colors;
    g_and_color = colors - 1;
    g_video_entry.x_dots = width;
    g_video_entry.y_dots = height;
    g_video_entry.colors = colors;
    g_viewport.final_aspect_ratio = 0.75f;
    m_frame_width = g_screen_x_dots;
    m_frame.resize(static_cast<std::size_t>(g_screen_x_dots) * g_screen_y_dots);
    for (int row = 0; row < g_screen_y_dots; ++row)
    {
        for (int col = 0; col < g_screen_x_dots; ++col)
        {
            m_frame[static_cast<std::size_t>(row) * m_frame_width + col] =
                static_cast<Byte>((col * 17 + row * 31 + col * row) % colors);
        }
    }
}

void TestGifWriterRaster::expect_failed_save_feedback()
{
    EXPECT_CALL(m_driver, read_pixel(_, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, display_string(0, 0, _, _, StrEq(" *interrupted* save ")));
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(1));
    EXPECT_CALL(m_driver, flush());
}

int TestGifWriterRaster::save()
{
    std::filesystem::remove(m_path);
    std::filesystem::path filename{m_path};
    return save_image(filename);
}

GifFilePtr TestGifWriterRaster::read_saved_gif() const
{
    int error{};
    GifFilePtr gif{DGifOpenFileName(m_path.string().c_str(), &error), [](GifFileType *input)
        {
            int close_error{};
            DGifCloseFile(input, &close_error);
        }};
    if (!gif || DGifSlurp(gif.get()) == GIF_ERROR)
    {
        throw std::runtime_error("Can't decode saved GIF");
    }
    return gif;
}

std::vector<Byte> TestGifWriterRaster::image_pixels() const
{
    std::vector<Byte> pixels;
    for (int row = 0; row < g_logical_screen.y_dots; ++row)
    {
        const std::size_t offset{
            static_cast<std::size_t>(row + g_logical_screen.y_offset) * m_frame_width + g_logical_screen.x_offset};
        pixels.insert(pixels.end(), m_frame.begin() + offset, m_frame.begin() + offset + g_logical_screen.x_dots);
    }
    return pixels;
}

void TestGifWriterRaster::expect_raster(
    const GifFileType &gif, const std::vector<Byte> &pixels, const int width, const int height) const
{
    EXPECT_EQ(width, gif.SWidth);
    EXPECT_EQ(height, gif.SHeight);
    ASSERT_EQ(1, gif.ImageCount);
    const SavedImage &image{gif.SavedImages[0]};
    EXPECT_EQ(0, image.ImageDesc.Left);
    EXPECT_EQ(0, image.ImageDesc.Top);
    EXPECT_EQ(width, image.ImageDesc.Width);
    EXPECT_EQ(height, image.ImageDesc.Height);
    EXPECT_FALSE(image.ImageDesc.Interlace);
    EXPECT_EQ(nullptr, image.ImageDesc.ColorMap);
    EXPECT_EQ(
        pixels, (std::vector<Byte>{image.RasterBits, image.RasterBits + static_cast<std::size_t>(width) * height}));
}

class TestGifWriterPalette : public TestGifWriterRaster, public WithParamInterface<std::pair<int, bool>>
{
};

TEST_P(TestGifWriterPalette, savesRasterAndPaletteThroughProductionEntry)
{
    const int colors{GetParam().first};
    const bool real_dac{GetParam().second};
    configure_image(8, 6, colors);
    g_got_real_dac = real_dac;
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 8, 6);
    std::ifstream input{m_path, std::ios::binary};
    std::array<char, 6> signature{};
    input.read(signature.data(), signature.size());
    EXPECT_EQ("GIF89a", (std::string{signature.data(), signature.size()}));
    EXPECT_EQ(0, gif->SBackGroundColor);
    EXPECT_EQ(8, gif->SColorResolution);
    EXPECT_EQ(49, gif->AspectByte);
    ASSERT_NE(nullptr, gif->SColorMap);
    ASSERT_EQ(colors, gif->SColorMap->ColorCount);
    for (int color = 0; color < colors; ++color)
    {
        SCOPED_TRACE(color);
        std::array<Byte, 3> expected{};
        if (colors == 2)
        {
            expected = color == 0 ? std::array<Byte, 3>{0, 0, 0} : std::array<Byte, 3>{63, 63, 63};
        }
        else if (colors == 4)
        {
            constexpr std::array<Byte, 3> CGA_PALETTE[]{{0, 0, 0}, {21, 63, 63}, {63, 21, 63}, {63, 63, 63}};
            expected = CGA_PALETTE[color];
        }
        else if (real_dac)
        {
            std::copy_n(g_dac_box[color], 3, expected.begin());
        }
        else
        {
            expected = EGA_PALETTE[color % 16];
        }
        const GifColorType &actual{gif->SColorMap->Colors[color]};
        EXPECT_EQ(expected, (std::array<Byte, 3>{actual.Red, actual.Green, actual.Blue}));
    }
    EXPECT_EQ(pixels, image_pixels());
    EXPECT_FALSE(g_busy);
}

INSTANTIATE_TEST_SUITE_P(Palettes, TestGifWriterPalette,
    Values(std::pair{2, false}, std::pair{2, true}, std::pair{4, false}, std::pair{4, true}, std::pair{16, false},
        std::pair{16, true}, std::pair{256, false}, std::pair{256, true}));

TEST_F(TestGifWriterRaster, savesLogicalPixelsAtPhysicalScreenOffset)
{
    configure_image(9, 5, 256, 3, 2);
    const std::vector<Byte> pixels{image_pixels()};
    const std::vector<Byte> frame{m_frame};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 9, 5);
    EXPECT_EQ(frame, m_frame);
}

TEST_F(TestGifWriterRaster, savesAspectByteForExplicitViewAndClampsIt)
{
    g_viewport.enabled = true;
    g_viewport.x_dots = 8;
    g_viewport.y_dots = 6;
    for (const std::pair<float, int> setting :
        {std::pair{0.75f, 49}, std::pair{0.5f, 81}, std::pair{1000.0f, 1}, std::pair{0.01f, 255}})
    {
        SCOPED_TRACE(setting.first);
        g_viewport.final_aspect_ratio = setting.first;

        ASSERT_EQ(0, save());

        const GifFilePtr gif{read_saved_gif()};
        EXPECT_EQ(setting.second, gif->AspectByte);
    }
}

TEST_F(TestGifWriterRaster, savesAutomaticViewAspectFromPhysicalScreen)
{
    configure_image(8, 3, 256);
    g_viewport.enabled = true;
    g_screen_x_dots = 800;
    g_screen_y_dots = 600;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    EXPECT_EQ(49, gif->AspectByte);
    EXPECT_EQ(8, gif->SWidth);
    EXPECT_EQ(3, gif->SHeight);
}

class TestGifWriterDimensions : public TestGifWriterRaster, public WithParamInterface<std::pair<int, int>>
{
};

TEST_P(TestGifWriterDimensions, savesUnsignedDimensionsThroughProductionEntry)
{
    const int width{GetParam().first};
    const int height{GetParam().second};
    configure_image(width, height, 2);
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, width, height);
}

INSTANTIATE_TEST_SUITE_P(UnsignedBounds, TestGifWriterDimensions,
    Values(std::pair{32768, 1}, std::pair{GIF_MAX_PIXELS, 1}, std::pair{1, 32768}, std::pair{1, GIF_MAX_PIXELS}));

TEST_F(TestGifWriterRaster, savesPotentialRowsAsColorsThenLowBytes)
{
    configure_image(5, 3, 256, 2, 1);
    const std::vector<Byte> pixels{image_pixels()};
    init_memory();
    m_memory_initialized = true;
    ASSERT_EQ(0, common_start_disk(g_screen_x_dots, g_screen_y_dots * 2, 256));
    g_disk_16_bit = true;
    std::vector<Byte> stored;
    for (int row = 0; row < 3; ++row)
    {
        stored.insert(stored.end(), pixels.begin() + row * 5, pixels.begin() + (row + 1) * 5);
        for (int col = 0; col < 5; ++col)
        {
            const Byte low{static_cast<Byte>(201 - row * 13 - col * 7)};
            disk_write_pixel(col + 2, row + 3 + 1, low);
            stored.push_back(low);
        }
    }

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, stored, 10, 3);
    EXPECT_EQ(pixels, image_pixels());
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentablePotentialWidth)
{
    configure_image(GIF_MAX_PIXELS / 2 + 1, 1, 256);
    g_disk_16_bit = true;
    expect_failed_save_feedback();

    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentableWidth)
{
    configure_image(GIF_MAX_PIXELS + 1, 1, 256);
    expect_failed_save_feedback();

    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentableHeight)
{
    configure_image(1, GIF_MAX_PIXELS + 1, 256);
    expect_failed_save_feedback();

    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, reportsOutputOpenFailure)
{
    std::filesystem::path filename{m_directory / "missing" / "image.gif"};

    EXPECT_EQ(-1, save_image(filename));

    EXPECT_FALSE(std::filesystem::exists(filename));
    EXPECT_FALSE(g_busy);
}

} // namespace id::test
