// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/encoder.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/pixel_limits.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/StickyOrbits.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractype.h>
#include <fractals/formula.h>
#include <fractals/ifs.h>
#include <fractals/lorenz.h>
#include <fractals/lsystem.h>
#include <fractals/parser.h>
#include <io/check_write_file.h>
#include <io/gif_extensions.h>
#include <io/gif_file.h>
#include <io/GifWriter.h>
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
#include <io/disk_storage.h>
#include <ui/diskvid.h>
#include <ui/big_while_loop.h>
#include <ui/evolve.h>
#include <ui/make_batch_file.h>
#include <ui/slideshw.h>
#include <ui/temp_msg.h>
#include <ui/video.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <limits>
#include <sstream>
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

namespace
{

using GifFilePtr = std::unique_ptr<GifFileType, void (*)(GifFileType *)>;

class FailingSaveBuffer : public std::stringbuf
{
public:
    std::streamsize remaining{std::numeric_limits<std::streamsize>::max()};
    bool fail_sync{};

protected:
    std::streamsize xsputn(const char *data, std::streamsize count) override;
    int sync() override;
};

std::streamsize FailingSaveBuffer::xsputn(const char *data, const std::streamsize count)
{
    const std::streamsize written{std::stringbuf::xsputn(data, std::min(count, remaining))};
    remaining -= written;
    return written;
}

int FailingSaveBuffer::sync()
{
    return fail_sync ? -1 : 0;
}

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
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, flush()).Times(AnyNumber());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(_)).Times(0);
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
    ASSERT_TRUE(prepare_disk_storage(g_screen_x_dots, g_screen_y_dots * 2, 256));
    g_disk_16_bit = true;
    std::vector<Byte> stored;
    for (int row = 0; row < 3; ++row)
    {
        stored.insert(stored.end(), pixels.begin() + row * 5, pixels.begin() + (row + 1) * 5);
        for (int col = 0; col < 5; ++col)
        {
            const Byte low{static_cast<Byte>(201 - row * 13 - col * 7)};
            write_disk_pixel(col + 2, row + 3 + 1, low);
            stored.push_back(low);
        }
    }
    // Progress must not run until both halves of the stored row are captured.
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(
            [this](const int x, const int y, const int color)
            {
                m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color);
                for (int col = 0; col < 5; ++col)
                {
                    write_disk_pixel(col + 2, y - g_logical_screen.y_offset + 3 + 1, 0);
                }
            });
    EXPECT_CALL(m_driver, flush()).Times(4);

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, stored, 10, 3);
    EXPECT_EQ(pixels, image_pixels());
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentablePotentialWidth)
{
    configure_image(GIF_MAX_PIXELS / 2 + 1, 1, 256);
    g_disk_16_bit = true;

    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentableWidth)
{
    configure_image(GIF_MAX_PIXELS + 1, 1, 256);

    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, rejectsUnrepresentableHeight)
{
    configure_image(1, GIF_MAX_PIXELS + 1, 256);

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

TEST_F(TestGifWriterRaster, savesWithoutConsumingPendingInput)
{
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 8, 6);
    EXPECT_EQ(FRACTAL_INFO_VERSION, get_fractal_info(gif.get()).info_version);
}

TEST_F(TestGifWriterRaster, flushesProgressAfterEachStoredRowAndRestoresIt)
{
    configure_image(600, 3, 256);
    const std::vector<Byte> pixels{image_pixels()};
    int flushes{};
    int reads{};
    int writes{};
    const std::vector<int> edges{0, 599, 1, 598, 2, 597};
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            [this, &reads](const int x, const int y)
            {
                ++reads;
                return m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x);
            });
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(
            [this, &flushes, &reads, &writes, &edges](const int x, const int y, const int color)
            {
                EXPECT_TRUE(g_busy);
                if (flushes < 3)
                {
                    EXPECT_EQ(flushes, y);
                    EXPECT_EQ(edges[writes % edges.size()], x);
                    if (writes % edges.size() == 0)
                    {
                        EXPECT_EQ(601, reads);
                    }
                }
                ++writes;
                m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color);
            });
    EXPECT_CALL(m_driver, flush())
        .Times(4)
        .WillRepeatedly(
            [this, &flushes, &reads, &writes, &pixels]
            {
                EXPECT_TRUE(g_busy);
                if (flushes < 3)
                {
                    EXPECT_EQ(6 * (flushes + 1), writes);
                    EXPECT_NE(pixels, image_pixels());
                }
                else
                {
                    EXPECT_EQ(pixels, image_pixels());
                }
                reads = 0;
                ++flushes;
            });

    ASSERT_EQ(0, save());

    EXPECT_EQ(36, writes);
    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 600, 3);
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, restoresProgressAfterScanlineWriteFailure)
{
    configure_image(600, 12, 256);
    const std::vector<Byte> pixels{image_pixels()};
    FailingSaveBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {600, 12}, GifWriter::Palette(256)};
    int flushes{};
    EXPECT_CALL(m_driver, flush())
        .WillRepeatedly(
            [&buffer, &flushes]
            {
                ++flushes;
                buffer.remaining = 0;
            });

    EXPECT_FALSE(encoder(writer, m_path));

    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    EXPECT_GE(flushes, 2);
    EXPECT_LT(flushes, 13);
    EXPECT_EQ(pixels, image_pixels());
    EXPECT_FALSE(writer.finish());
}

TEST_F(TestGifWriterRaster, restoresProgressAndReportsCloseFailure)
{
    const std::vector<Byte> pixels{image_pixels()};
    FailingSaveBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {8, 6}, GifWriter::Palette(256)};
    buffer.fail_sync = true;
    EXPECT_CALL(m_driver, flush()).Times(7);

    EXPECT_FALSE(encoder(writer, m_path));

    EXPECT_EQ(E_GIF_ERR_CLOSE_FAILED, writer.error());
    EXPECT_EQ(pixels, image_pixels());
    EXPECT_FALSE(writer.finish());
}

TEST_F(TestGifWriterRaster, failedReplacementPreservesOriginalAndRemovesTemporaryFile)
{
    const std::vector<Byte> pixels{image_pixels()};
    ASSERT_EQ(0, save());
    g_logical_screen.x_dots = GIF_MAX_PIXELS + 1;
    std::filesystem::path filename{m_path};

    EXPECT_EQ(-1, save_image(filename));

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 8, 6);
    EXPECT_FALSE(std::filesystem::exists(m_directory / "id.tmp"));
    EXPECT_EQ(BatchMode::BAILOUT_ERROR_NO_SAVE, g_init_batch);
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterRaster, overwriteReplacesOriginalOnlyAfterSuccessfulSave)
{
    ASSERT_EQ(0, save());
    m_frame[0] = 123;
    const std::vector<Byte> pixels{image_pixels()};
    std::filesystem::path filename{m_path};

    ASSERT_EQ(0, save_image(filename));

    const GifFilePtr gif{read_saved_gif()};
    expect_raster(*gif, pixels, 8, 6);
    EXPECT_FALSE(std::filesystem::exists(m_directory / "id.tmp"));
}

TEST_F(TestGifWriterRaster, selectsNextFilenameWithoutOverwriting)
{
    ASSERT_EQ(0, save());
    g_overwrite_file = false;
    std::filesystem::path filename{m_path};

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(m_directory / "image3.gif", filename);
    EXPECT_TRUE(std::filesystem::exists(m_directory / "image2.gif"));
    EXPECT_TRUE(std::filesystem::exists(m_path));
    EXPECT_FALSE(std::filesystem::exists(m_directory / "id.tmp"));
}

TEST_F(TestGifWriterRaster, autosaveReusesFilenameAndFinalSaveEndsResaving)
{
    g_resave_flag = TimedSave::STARTED;
    std::filesystem::path filename{m_path};
    ASSERT_EQ(0, save_image(filename));
    EXPECT_EQ(m_path, filename);
    EXPECT_TRUE(g_started_resaves);
    m_frame[0] = 123;
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(m_path, filename);
    EXPECT_TRUE(g_started_resaves);
    EXPECT_FALSE(std::filesystem::exists(m_directory / "id.tmp"));
    {
        const GifFilePtr gif{read_saved_gif()};
        expect_raster(*gif, pixels, 8, 6);
    }
    g_resave_flag = TimedSave::FINAL;

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(TimedSave::NONE, g_resave_flag);
    EXPECT_FALSE(g_started_resaves);
    EXPECT_EQ(m_directory / "image2.gif", filename);
}

class TestGifWriterExtensions : public TestGifWriterRaster
{
protected:
    bool read_metadata();
    void expect_layout(GifFileType *gif, const std::vector<std::string> &identifiers) const;
    std::vector<Byte> payload(const GifFileType *gif, const std::string &identifier) const;

    FractalInfo m_info{};
    ExtBlock2 m_resume{};
    ExtBlock3 m_formula{};
    ExtBlock4 m_ranges{};
    ExtBlock5 m_parameters{};
    ExtBlock6 m_evolution{};
    ExtBlock7 m_orbits{};
    ValueSaver<std::vector<Byte>> m_saved_resume{g_resume_data, std::vector<Byte>{}};
    ValueSaver<int> m_saved_resume_len{g_resume_len, 0};
    ValueSaver<std::vector<int>> m_saved_ranges{g_iteration_ranges, std::vector<int>{}};
    ValueSaver<Display3DMode> m_saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<CompiledFormula> m_saved_formula{g_formula};
    ValueSaver<int> m_saved_file_width{g_file_x_dots};
    ValueSaver<int> m_saved_file_height{g_file_y_dots};
    ValueSaver<int> m_saved_file_colors{g_file_colors};
    ValueSaver<float> m_saved_file_aspect{g_file_aspect_ratio};
    ValueSaver<bool> m_saved_make_parameter_file{g_make_parameter_file, false};
};

bool TestGifWriterExtensions::read_metadata()
{
    return find_fractal_info(
        m_path.string(), &m_info, &m_resume, &m_formula, &m_ranges, &m_parameters, &m_evolution, &m_orbits);
}

void TestGifWriterExtensions::expect_layout(GifFileType *gif, const std::vector<std::string> &identifiers) const
{
    ASSERT_EQ(1, gif->ImageCount);
    EXPECT_EQ(0, gif->SavedImages[0].ExtensionBlockCount);
    std::vector<std::string> actual;
    int encoded_length{};
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (block.Function == APPLICATION_EXT_FUNC_CODE)
        {
            ASSERT_EQ(11, block.ByteCount);
            actual.emplace_back(reinterpret_cast<const char *>(block.Bytes), block.ByteCount);
            encoded_length += 3; // Introducer, function code, and terminator.
        }
        else
        {
            EXPECT_EQ(CONTINUE_EXT_FUNC_CODE, block.Function);
            EXPECT_GT(block.ByteCount, 0);
            EXPECT_LE(block.ByteCount, 255);
        }
        encoded_length += 1 + block.ByteCount;
    }
    EXPECT_EQ(identifiers, actual);
    EXPECT_EQ(encoded_length, get_fractal_info(gif).tot_extend_len);

    // Older readers find the final main record at this fixed trailer offset.
    std::ifstream input{m_path, std::ios::binary};
    std::array<char, 14> header{};
    input.seekg(-encoded_length - 1, std::ios::end);
    input.read(header.data(), header.size());
    EXPECT_EQ((std::array<char, 3>{'!', static_cast<char>(0xFF), 11}),
        (std::array<char, 3>{header[0], header[1], header[2]}));
    ASSERT_FALSE(identifiers.empty());
    EXPECT_EQ(identifiers.front(), (std::string{header.data() + 3, 11}));
    input.seekg(-522, std::ios::end);
    input.read(header.data(), header.size());
    EXPECT_EQ("fractint001", (std::string{header.data() + 3, 11}));
    input.seekg(-1, std::ios::end);
    EXPECT_EQ(';', input.get());
}

std::vector<Byte> TestGifWriterExtensions::payload(const GifFileType *gif, const std::string &identifier) const
{
    bool found{};
    std::vector<Byte> bytes;
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (block.Function == APPLICATION_EXT_FUNC_CODE)
        {
            if (found)
            {
                break;
            }
            found = std::string{reinterpret_cast<const char *>(block.Bytes),
                        static_cast<std::size_t>(block.ByteCount)} == identifier;
        }
        else if (found)
        {
            bytes.insert(bytes.end(), block.Bytes, block.Bytes + block.ByteCount);
        }
    }
    return bytes;
}

TEST_F(TestGifWriterExtensions, savesMainRecordWithoutInapplicableOptionalRecords)
{
    ValueSaver saved_iterations{g_max_iterations, 0x123456L};
    ValueSaver saved_time{g_calc_time, 0x12345678L};
    ValueSaver saved_version{g_version, Version{1, 5, 2, 3, false}};
    ValueSaver saved_region{g_image_region, ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}}};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint001"});
    const std::vector<Byte> bytes{payload(gif.get(), "fractint001")};
    ASSERT_EQ(504, bytes.size());
    EXPECT_EQ((std::vector<Byte>{'F', 'r', 'a', 'c', 't', 'a', 'l', 0, 0xFF, 0x7F, 4, 0}),
        (std::vector<Byte>{bytes.begin(), bytes.begin() + 12}));
    EXPECT_EQ(
        (std::vector<Byte>{0, 0, 0, 0, 0, 0, 4, 0xC0}), (std::vector<Byte>{bytes.begin() + 12, bytes.begin() + 20}));
    ASSERT_FALSE(read_metadata());
    EXPECT_STREQ("Fractal", m_info.info_id);
    EXPECT_EQ(FRACTAL_INFO_VERSION, m_info.info_version);
    EXPECT_EQ(0x123456, m_info.iterations);
    EXPECT_EQ(32767, m_info.iterations_old);
    EXPECT_EQ(0x12345678, m_info.calc_time);
    EXPECT_EQ((Version{1, 5, 2, 3, false}), fractal_info_version(m_info));
    EXPECT_EQ(8, m_info.x_dots);
    EXPECT_EQ(6, m_info.y_dots);
    EXPECT_EQ(256, m_info.colors);
    EXPECT_EQ(8, g_file_x_dots);
    EXPECT_EQ(6, g_file_y_dots);
    EXPECT_EQ(256, g_file_colors);
    EXPECT_FALSE(m_resume.got_data);
    EXPECT_FALSE(m_formula.got_data);
    EXPECT_FALSE(m_ranges.got_data);
    EXPECT_FALSE(m_parameters.got_data);
    EXPECT_FALSE(m_evolution.got_data);
    EXPECT_FALSE(m_orbits.got_data);
}

class TestGifWriterResume : public TestGifWriterExtensions, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterResume, savesOnlyUsedResumeBytesWithLegacySubBlocks)
{
    const int size{GetParam()};
    std::vector<Byte> expected(size);
    for (int i = 0; i < size; ++i)
    {
        expected[i] = static_cast<Byte>(i * 37 + 129);
    }
    g_resume_data = expected;
    g_resume_data.push_back(0x5A); // Allocated capacity beyond the used prefix is not saved.
    g_resume_len = size;
    g_calc_status = CalcStatus::RESUMABLE;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint002", "fractint001"});
    EXPECT_TRUE(has_gif_extension(gif.get(), "fractint002"));
    EXPECT_EQ(expected, get_resume_info(gif.get()));
    const int chunks{(size + 254) / 255};
    ASSERT_EQ(chunks + 4, gif->ExtensionBlockCount);
    for (int i = 0; i < chunks; ++i)
    {
        EXPECT_EQ(CONTINUE_EXT_FUNC_CODE, gif->ExtensionBlocks[i + 1].Function);
        EXPECT_EQ(std::min(255, size - i * 255), gif->ExtensionBlocks[i + 1].ByteCount);
    }
    EXPECT_EQ(521 + 15 + size + chunks, get_fractal_info(gif.get()).tot_extend_len);
    ASSERT_FALSE(read_metadata());
    EXPECT_EQ(static_cast<int>(CalcStatus::RESUMABLE), m_info.calc_status);
    EXPECT_TRUE(m_resume.got_data);
    EXPECT_EQ(size, m_resume.length);
    EXPECT_EQ(expected, m_resume.resume_data);
}

INSTANTIATE_TEST_SUITE_P(SubBlockSizes, TestGifWriterResume, Values(0, 1, 254, 255, 256, 510, 511, 4097));

TEST_F(TestGifWriterExtensions, omitsResumeRecordForCompletedImageOrEmptyStorage)
{
    g_resume_data = {0x12, 0x34};
    g_resume_len = 2;
    ASSERT_EQ(0, save());
    {
        const GifFilePtr completed{read_saved_gif()};
        expect_layout(completed.get(), {"fractint001"});
    }

    g_resume_data.clear();
    g_resume_len = 0;
    g_calc_status = CalcStatus::RESUMABLE;
    ASSERT_EQ(0, save());
    const GifFilePtr empty{read_saved_gif()};
    expect_layout(empty.get(), {"fractint001"});
    ASSERT_FALSE(read_metadata());
    EXPECT_FALSE(m_resume.got_data);
}

class TestGifWriterName : public TestGifWriterExtensions, public WithParamInterface<FractalType>
{
};

TEST_P(TestGifWriterName, savesSelectedItemNameAndOnlyFormulaUsageFlags)
{
    g_fractal_type = GetParam();
    ValueSaver saved_formula_name{g_formula_name, std::string{"SavedFormula"}};
    ValueSaver saved_l_system_name{g_l_system_name, std::string{"SavedLSystem"}};
    ValueSaver saved_ifs_name{g_ifs_name, std::string{"SavedIFS"}};
    ValueSaver saved_ismand{g_frm_is_mandelbrot, false};
    g_formula.uses_p1 = true;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = true;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = true;
    g_formula.uses_ismand = true;
    const std::string name{g_fractal_type == FractalType::FORMULA ? "SavedFormula"
            : g_fractal_type == FractalType::L_SYSTEM             ? "SavedLSystem"
                                                                  : "SavedIFS"};
    const bool formula{g_fractal_type == FractalType::FORMULA};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint003", "fractint001"});
    FormulaInfo expected{};
    std::copy(name.begin(), name.end(), expected.form_name);
    expected.uses_p1 = formula;
    expected.uses_p3 = formula;
    expected.uses_ismand = formula;
    expected.uses_p5 = formula;
    EXPECT_EQ(expected, get_formula_info(gif.get()));
    const std::vector<Byte> bytes{payload(gif.get(), "fractint003")};
    ASSERT_EQ(66, bytes.size());
    EXPECT_EQ(name, (std::string{bytes.begin(), bytes.begin() + name.size()}));
    const Byte flag{static_cast<Byte>(formula)};
    EXPECT_EQ((std::vector<Byte>{flag, 0, 0, 0, flag, 0, flag, 0, 0, 0, 0, 0, flag, 0}),
        (std::vector<Byte>{bytes.begin() + 40, bytes.begin() + 54}));
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_formula.got_data);
    EXPECT_EQ(66, m_formula.length);
    EXPECT_EQ(name, m_formula.form_name);
    EXPECT_EQ(formula, m_formula.uses_p1);
    EXPECT_EQ(formula, m_formula.uses_p3);
    EXPECT_EQ(formula, m_formula.uses_ismand);
    EXPECT_EQ(0, m_formula.uses_p2);
    EXPECT_EQ(0, m_formula.uses_p4);
    EXPECT_EQ(formula, m_formula.uses_p5);
    EXPECT_EQ(0, m_formula.ismand);
}

INSTANTIATE_TEST_SUITE_P(ItemTypes, TestGifWriterName,
    Values(FractalType::FORMULA, FractalType::L_SYSTEM, FractalType::IFS, FractalType::IFS_3D));

TEST_F(TestGifWriterExtensions, terminatesTruncatedItemNamesAndPreservesEmptyNames)
{
    g_fractal_type = FractalType::L_SYSTEM;
    ValueSaver saved_name{g_l_system_name};
    for (const std::string name : {std::string(60, 'A'), std::string{}})
    {
        SCOPED_TRACE(name);
        g_l_system_name = name;
        ASSERT_EQ(0, save());
        const GifFilePtr gif{read_saved_gif()};
        expect_layout(gif.get(), {"fractint003", "fractint001"});
        const FormulaInfo info{get_formula_info(gif.get())};
        EXPECT_EQ(name.substr(0, 39), info.form_name);
        EXPECT_EQ(0, info.form_name[39]);
        ASSERT_FALSE(read_metadata());
        EXPECT_TRUE(m_formula.got_data);
        EXPECT_EQ(name.substr(0, 39), m_formula.form_name);
    }
}

TEST_F(TestGifWriterExtensions, savesSignedIterationRangesInLittleEndianOrder)
{
    g_iteration_ranges = {-1, 0x1234, -32768, 32767};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint004", "fractint001"});
    EXPECT_EQ((std::vector<Byte>{0xFF, 0xFF, 0x34, 0x12, 0, 0x80, 0xFF, 0x7F}), payload(gif.get(), "fractint004"));
    EXPECT_EQ(g_iteration_ranges, get_ranges_info(gif.get()));
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_ranges.got_data);
    EXPECT_EQ(4, m_ranges.length);
    EXPECT_EQ(g_iteration_ranges, m_ranges.range_data);
}

TEST_F(TestGifWriterExtensions, omitsIterationRangesForThreeDimensionalDisplay)
{
    g_iteration_ranges = {16, 32};
    g_display_3d = Display3DMode::YES;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint001"});
    ASSERT_FALSE(read_metadata());
    EXPECT_FALSE(m_ranges.got_data);
}

TEST_F(TestGifWriterExtensions, preservesOpaqueArbitraryPrecisionParameters)
{
    std::vector<Byte> parameters(22 * (16 + 2));
    for (std::size_t i = 0; i < parameters.size(); ++i)
    {
        parameters[i] = static_cast<Byte>(i * 53 + 157);
    }
    ValueSaver saved_bf_length{g_bf_length, 16};
    ValueSaver saved_bn_length{g_bn_length, 12};
    ValueSaver saved_min{g_bf_x_min, parameters.data()};
    g_bf_math = BFMathType::BIG_FLT;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint005", "fractint001"});
    const std::vector<char> expected{parameters.begin(), parameters.end()};
    EXPECT_EQ(parameters, payload(gif.get(), "fractint005"));
    EXPECT_EQ(expected, get_extended_param_info(gif.get()));
    ASSERT_FALSE(read_metadata());
    EXPECT_EQ(static_cast<int>(BFMathType::BIG_FLT), m_info.bf_math);
    EXPECT_EQ(12, m_info.bf_length);
    EXPECT_TRUE(m_parameters.got_data);
    EXPECT_EQ(expected, m_parameters.apm_data);
}

class TestGifWriterEvolution : public TestGifWriterExtensions, public WithParamInterface<bool>
{
};

TEST_P(TestGifWriterEvolution, savesLiveEvolutionStateWhenNoResumeOrCalculationComplete)
{
    const bool completed{GetParam()};
    g_evolving = EvolutionModeFlags::FIELD_MAP;
    g_calc_status = completed ? CalcStatus::COMPLETED : CalcStatus::RESUMABLE;
    ValueSaver saved_have_info{g_have_evolve_info, completed};
    ValueSaver saved_grid{g_evolve_image_grid_size, 3};
    ValueSaver saved_seed{g_evolve_this_generation_random_seed, 0xABCDU};
    ValueSaver saved_mutation{g_evolve_max_random_mutation, 0.25};
    ValueSaver saved_x_range{g_evolve_x_parameter_range, 4.0};
    ValueSaver saved_y_range{g_evolve_y_parameter_range, 3.0};
    ValueSaver saved_x_offset{g_evolve_x_parameter_offset, -2.0};
    ValueSaver saved_y_offset{g_evolve_y_parameter_offset, -1.5};
    ValueSaver saved_px{g_evolve_param_grid_x, 1};
    ValueSaver saved_py{g_evolve_param_grid_y, 2};
    ValueSaver saved_gene{g_gene_bank[2].mutate, Variations::Y};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint006", "fractint001"});
    const EvolutionInfo info{get_evolution_info(gif.get())};
    EXPECT_EQ(1, info.evolving);
    EXPECT_EQ(3, info.image_grid_size);
    EXPECT_EQ(0xABCD, info.this_generation_random_seed);
    EXPECT_EQ(9, info.count);
    EXPECT_EQ(0.25, info.max_random_mutation);
    EXPECT_EQ(4.0, info.x_parameter_range);
    EXPECT_EQ(3.0, info.y_parameter_range);
    EXPECT_EQ(-2.0, info.x_parameter_offset);
    EXPECT_EQ(-1.5, info.y_parameter_offset);
    EXPECT_EQ(1, info.px);
    EXPECT_EQ(2, info.py);
    EXPECT_EQ(8, info.x_dots);
    EXPECT_EQ(6, info.y_dots);
    EXPECT_EQ(static_cast<int>(Variations::Y), info.mutate[2]);
    const std::vector<Byte> bytes{payload(gif.get(), "fractint006")};
    ASSERT_EQ(200, bytes.size());
    EXPECT_EQ((std::vector<Byte>{1, 0, 3, 0, 0xCD, 0xAB}), (std::vector<Byte>{bytes.begin(), bytes.begin() + 6}));
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_evolution.got_data);
    EXPECT_EQ(200, m_evolution.length);
    EXPECT_EQ(0xABCD, m_evolution.this_generation_random_seed);
    EXPECT_EQ(9, m_evolution.e_count);
    EXPECT_EQ(-2.0, m_evolution.x_parameter_offset);
    EXPECT_EQ(static_cast<int>(Variations::Y), m_evolution.mutate[2]);
}

INSTANTIATE_TEST_SUITE_P(Completion, TestGifWriterEvolution, Bool());

TEST_F(TestGifWriterExtensions, savesCachedEvolutionStateForResumableCalculation)
{
    g_evolving = EvolutionModeFlags::FIELD_MAP;
    g_calc_status = CalcStatus::RESUMABLE;
    ValueSaver saved_have_info{g_have_evolve_info, true};
    ValueSaver saved_info{g_evolve_info, EvolutionInfo{}};
    ValueSaver saved_gene{g_gene_bank[1].mutate, Variations::RANDOM};
    g_evolve_info.image_grid_size = 5;
    g_evolve_info.evolving = 1;
    g_evolve_info.this_generation_random_seed = 0x9876;
    g_evolve_info.max_random_mutation = 0.5;
    g_evolve_info.x_parameter_range = 8;
    g_evolve_info.y_parameter_range = 6;
    g_evolve_info.x_parameter_offset = -4;
    g_evolve_info.y_parameter_offset = -3;
    g_evolve_info.discrete_x_parameter_offset = -7;
    g_evolve_info.discrete_y_parameter_offset = 9;
    g_evolve_info.px = 3;
    g_evolve_info.py = 4;
    g_evolve_info.screen_x_offset = 2;
    g_evolve_info.screen_y_offset = 1;
    g_evolve_info.x_dots = 4;
    g_evolve_info.y_dots = 3;
    g_evolve_info.count = 17;
    EvolutionInfo expected{g_evolve_info};
    for (int i = 0; i < NUM_GENES; ++i)
    {
        expected.mutate[i] = static_cast<std::int16_t>(g_gene_bank[i].mutate);
    }

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint006", "fractint001"});
    EXPECT_EQ(expected, get_evolution_info(gif.get()));
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_evolution.got_data);
    EXPECT_EQ(17, m_evolution.e_count);
    EXPECT_EQ(5, m_evolution.image_grid_size);
    EXPECT_EQ(4, m_evolution.x_dots);
    EXPECT_EQ(3, m_evolution.y_dots);
    EXPECT_EQ(-7, m_evolution.discrete_x_parameter_offset);
    EXPECT_EQ(static_cast<int>(Variations::RANDOM), m_evolution.mutate[1]);
}

TEST_F(TestGifWriterExtensions, savesOrbitSettingsOnlyForOrbitPass)
{
    ValueSaver saved_corner{g_orbit_corner, ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.0, -1.0}}};
    ValueSaver saved_keep{g_keep_screen_coords, true};
    ValueSaver saved_draw{g_draw_mode, OrbitDrawMode::LINE};
    g_std_calc_mode = CalcMode::ORBIT;

    ASSERT_EQ(0, save());

    GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(), {"fractint007", "fractint001"});
    const OrbitsInfo info{get_orbits_info(gif.get())};
    EXPECT_EQ(-2.5, info.orbit_corner_min_x);
    EXPECT_EQ(1.5, info.orbit_corner_max_x);
    EXPECT_EQ(-1.5, info.orbit_corner_min_y);
    EXPECT_EQ(1.5, info.orbit_corner_max_y);
    EXPECT_EQ(-2.0, info.orbit_corner_3rd_x);
    EXPECT_EQ(-1.0, info.orbit_corner_3rd_y);
    EXPECT_EQ(1, info.keep_screen_coords);
    EXPECT_EQ('l', info.draw_mode);
    const std::vector<Byte> bytes{payload(gif.get(), "fractint007")};
    ASSERT_EQ(200, bytes.size());
    EXPECT_EQ((std::vector<Byte>{0, 0, 0, 0, 0, 0, 4, 0xC0}), (std::vector<Byte>{bytes.begin(), bytes.begin() + 8}));
    EXPECT_EQ((std::vector<Byte>{1, 0, 'l', 0}), (std::vector<Byte>{bytes.begin() + 48, bytes.begin() + 52}));
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_orbits.got_data);
    EXPECT_EQ(200, m_orbits.length);
    EXPECT_EQ(-2.5, m_orbits.ox_min);
    EXPECT_EQ(-1.0, m_orbits.oy_3rd);
    EXPECT_EQ(1, m_orbits.keep_screen_coords);
    EXPECT_EQ('l', m_orbits.draw_mode);

    gif.reset();
    g_std_calc_mode = CalcMode::ONE_PASS;
    ASSERT_EQ(0, save());
    const GifFilePtr other_pass{read_saved_gif()};
    expect_layout(other_pass.get(), {"fractint001"});
    ASSERT_FALSE(read_metadata());
    EXPECT_FALSE(m_orbits.got_data);
}

TEST_F(TestGifWriterExtensions, preservesCompleteRecordOrderingAndTotalEncodedLength)
{
    g_fractal_type = FractalType::FORMULA;
    ValueSaver saved_name{g_formula_name, std::string{"AllRecords"}};
    g_resume_data = {0x12, 0x34, 0x56};
    g_resume_len = 2;
    g_calc_status = CalcStatus::RESUMABLE;
    g_iteration_ranges = {16, 32};
    std::vector<Byte> parameters(22 * (16 + 2), 0xA5);
    ValueSaver saved_bf_length{g_bf_length, 16};
    ValueSaver saved_min{g_bf_x_min, parameters.data()};
    g_bf_math = BFMathType::BIG_FLT;
    g_evolving = EvolutionModeFlags::FIELD_MAP;
    ValueSaver saved_have_info{g_have_evolve_info, true};
    ValueSaver saved_evolution{g_evolve_info, EvolutionInfo{}};
    g_std_calc_mode = CalcMode::ORBIT;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_saved_gif()};
    expect_layout(gif.get(),
        {"fractint002", "fractint003", "fractint004", "fractint005", "fractint006", "fractint007", "fractint001"});
    ASSERT_FALSE(read_metadata());
    EXPECT_TRUE(m_resume.got_data);
    EXPECT_TRUE(m_formula.got_data);
    EXPECT_TRUE(m_ranges.got_data);
    EXPECT_TRUE(m_parameters.got_data);
    EXPECT_TRUE(m_evolution.got_data);
    EXPECT_TRUE(m_orbits.got_data);
    EXPECT_EQ((std::vector<Byte>{0x12, 0x34}), m_resume.resume_data);
    EXPECT_EQ("AllRecords", std::string{m_formula.form_name});
    EXPECT_EQ((std::vector<int>{16, 32}), m_ranges.range_data);
    EXPECT_EQ((std::vector<char>{parameters.begin(), parameters.end()}), m_parameters.apm_data);
}

} // namespace id::test
