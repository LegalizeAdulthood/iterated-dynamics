// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/decoder.h>
#include <io/loadfile.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel_limits.h>
#include <engine/Potential.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/gifview.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/slideshw.h>
#include <ui/restore_image.h>
#include <ui/video.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

std::vector<int> s_line_lengths;
std::vector<std::array<int, 3>> s_sound_pixels;

int capture_positive_out_line(Byte *, const int line_len)
{
    s_line_lengths.push_back(line_len);
    return 17;
}

void capture_sound_pixel(const int col, const int row, const int color)
{
    s_sound_pixels.push_back({col, row, color});
}

class TestGifOutputFile
{
public:
    TestGifOutputFile(const std::filesystem::path &path, const int width, const int left = 0, const int top = 0) :
        m_path(path)
    {
        write(width, left, top);
    }

    ~TestGifOutputFile()
    {
        std::filesystem::remove(m_path);
    }

    const std::filesystem::path &path() const
    {
        return m_path;
    }

private:
    void write(const int width, const int left, const int top) const
    {
        int error{};
        GifFileType *gif = EGifOpenFileName(m_path.string().c_str(), false, &error);
        if (gif == nullptr)
        {
            throw std::runtime_error("Failed to create test GIF");
        }
        ColorMapObject *color_map = GifMakeMapObject(2, nullptr);
        color_map->Colors[0].Red = 255;
        color_map->Colors[0].Green = 0;
        color_map->Colors[0].Blue = 0;
        color_map->Colors[1].Red = 0;
        color_map->Colors[1].Green = 0;
        color_map->Colors[1].Blue = 0;
        std::vector<GifPixelType> pixels(width, 0);
        const bool failed = EGifPutScreenDesc(gif, width + left, 1 + top, 2, 0, color_map) == GIF_ERROR ||
            EGifPutImageDesc(gif, left, top, width, 1, false, nullptr) == GIF_ERROR ||
            EGifPutLine(gif, pixels.data(), width) == GIF_ERROR || EGifCloseFile(gif, &error) == GIF_ERROR;
        GifFreeMapObject(color_map);
        if (failed)
        {
            throw std::runtime_error("Failed to write test GIF");
        }
    }

    std::filesystem::path m_path;
};

} // namespace

TEST(TestLegacy13GifPalette, detectsQuantizedPaletteInVersionRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 252;
    palette[0][1] = 168;
    palette[0][2] = 84;

    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 0, 0, false}, palette, 1));
    EXPECT_TRUE(backwards_id1_3_palette_needed(Version{1, 3, 1, 0, false}, palette, 1));
    EXPECT_TRUE(backwards_id1_3_palette_needed(Version{1, 3, 2, 9, false}, palette, 1));
    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 3, 0, false}, palette, 1));
}

TEST(TestLegacy13GifPalette, rejectsNonQuantizedPaletteInVersionRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 253;

    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 1, 0, false}, palette, 1));
}

TEST(TestLegacy13GifPalette, expandsQuantizedPaletteToFullRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 0;
    palette[0][1] = 4;
    palette[0][2] = 84;
    palette[1][0] = 168;
    palette[1][1] = 252;
    palette[1][2] = 128;

    ValueSaver saved_file_version{g_file_version, Version{1, 3, 1, 0, false}};

    backwards_id1_3_palette(palette, 2);

    EXPECT_EQ(0, palette[0][0]);
    EXPECT_EQ(4, palette[0][1]);
    EXPECT_EQ(85, palette[0][2]);
    EXPECT_EQ(170, palette[1][0]);
    EXPECT_EQ(255, palette[1][1]);
    EXPECT_EQ(130, palette[1][2]);
}

class TestGifViewLineWidth : public TestWithParam<int>
{
};

TEST_P(TestGifViewLineWidth, positiveCallbackResultPreservesFullWidthAndSuccessfulRestore)
{
    const int width{GetParam()};
    const TestGifOutputFile gif{
        std::filesystem::temp_directory_path() / ("id-gifview-" + std::to_string(width) + ".gif"), width};
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ValueSaver saved_read_filename{g_read_filename, gif.path()};
    ValueSaver<int (*)(Byte *, int)> saved_out_line{g_out_line, capture_positive_out_line};
    ValueSaver saved_read_color{g_read_color, true};
    ValueSaver saved_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver saved_map_set{g_map_set, false};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_dither_flag{g_dither_flag, false};
    ValueSaver saved_potential_16bit{g_potential.store_16bit, false};
    ValueSaver saved_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver saved_skip_y_dots{g_skip_y_dots, 0};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_busy{g_busy, false};
    s_line_lengths.clear();
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));
    ON_CALL(driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);

    EXPECT_EQ(0, restore_image());

    ASSERT_EQ(1, s_line_lengths.size());
    EXPECT_EQ(width, s_line_lengths[0]);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(gif.path()));
}

TEST_P(TestGifViewLineWidth, callbackFailureClosesFileAndClearsBusyState)
{
    const int width{GetParam()};
    const TestGifOutputFile gif{
        std::filesystem::temp_directory_path() / ("id-gifview-error-" + std::to_string(width) + ".gif"), width};
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ValueSaver saved_read_filename{g_read_filename, gif.path()};
    ValueSaver<decltype(g_out_line)> saved_out_line{g_out_line, [](Byte *, int)
        {
            EXPECT_TRUE(g_busy);
            return -73;
        }};
    ValueSaver saved_read_color{g_read_color, true};
    ValueSaver saved_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver saved_map_set{g_map_set, false};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_dither_flag{g_dither_flag, false};
    ValueSaver saved_potential_16bit{g_potential.store_16bit, false};
    ValueSaver saved_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver saved_skip_y_dots{g_skip_y_dots, 0};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_busy{g_busy, false};
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));

    EXPECT_EQ(-73, restore_image());

    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(gif.path()));
}

INSTANTIATE_TEST_SUITE_P(Widths, TestGifViewLineWidth, Values(640, MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

class TestGifViewSound : public TestWithParam<bool>
{
};

TEST_P(TestGifViewSound, restoresDecodedPixelsAcrossDisplayRowsWithoutPollingInput)
{
    const bool supported{GetParam()};
    const TestGifOutputFile gif{
        std::filesystem::temp_directory_path() / (supported ? "id-gifview-sound-on.gif" : "id-gifview-sound-off.gif"),
        8};
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ValueSaver saved_read_filename{g_read_filename, gif.path()};
    ValueSaver<decltype(g_out_line)> saved_out_line{g_out_line, out_line};
    ValueSaver saved_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver<decltype(g_put_color)> saved_put_color{g_put_color, capture_sound_pixel};
    ValueSaver saved_screen{g_logical_screen, LogicalScreen{4, 2, 0, 0, 3.0, 1.0}};
    ValueSaver saved_row_count{g_row_count};
    ValueSaver saved_read_color{g_read_color, true};
    ValueSaver saved_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver saved_map_set{g_map_set, false};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_dither_flag{g_dither_flag, false};
    ValueSaver saved_potential_16bit{g_potential.store_16bit, false};
    ValueSaver saved_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver saved_skip_y_dots{g_skip_y_dots, 0};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_busy{g_busy, false};
    ValueSaver saved_orbit_delay{g_orbit_delay, 0};
    ValueSaver saved_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver saved_base_hertz{g_base_hertz, 220};
    s_sound_pixels.clear();
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(driver, sound_on(220)).Times(8).WillRepeatedly(Return(supported));
    EXPECT_CALL(driver, sound_off()).Times(supported ? 8 : 0);
    EXPECT_CALL(driver, mute()).Times(0);
    EXPECT_CALL(driver, delay(_)).Times(0);
    ON_CALL(driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);

    EXPECT_EQ(0, restore_image());

    const std::vector<std::array<int, 3>> expected{
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {3, 1, 0}};
    EXPECT_EQ(expected, s_sound_pixels);
    EXPECT_EQ(2, g_row_count);
}

INSTANTIATE_TEST_SUITE_P(DriverSound, TestGifViewSound, Bool());

class TestGifViewMigs : public TestWithParam<bool>
{
};

TEST_P(TestGifViewMigs, reducesImageOffsetsAlongTheirOwnAxisWithoutPollingInput)
{
    const bool skip_x{GetParam()};
    const TestGifOutputFile gif{
        std::filesystem::temp_directory_path() / (skip_x ? "id-gifview-mig-x.gif" : "id-gifview-mig-y.gif"), 4, 3, 4};
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ValueSaver saved_read_filename{g_read_filename, gif.path()};
    ValueSaver<decltype(g_out_line)> saved_out_line{g_out_line, out_line};
    ValueSaver saved_screen{g_logical_screen, LogicalScreen{8, 8, 0, 0, 1.0, 1.0}};
    ValueSaver saved_screen_x{g_screen_x_dots, 8};
    ValueSaver saved_screen_y{g_screen_y_dots, 8};
    ValueSaver saved_row_count{g_row_count};
    ValueSaver saved_read_color{g_read_color, true};
    ValueSaver saved_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver saved_map_set{g_map_set, false};
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_dither_flag{g_dither_flag, false};
    ValueSaver saved_potential_16bit{g_potential.store_16bit, false};
    ValueSaver saved_skip_x_dots{g_skip_x_dots, skip_x ? 1 : 0};
    ValueSaver saved_skip_y_dots{g_skip_y_dots, skip_x ? 0 : 1};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_busy{g_busy, false};
    set_normal_dot();
    ON_CALL(driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(driver, key_pressed()).Times(0);
    EXPECT_CALL(driver, get_key()).Times(0);
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));
    const int left{skip_x ? 1 : 3};
    const int top{skip_x ? 4 : 2};
    const int width{skip_x ? 2 : 4};
    for (int col = left; col < left + width; ++col)
    {
        EXPECT_CALL(driver, write_pixel(col, top, 0));
    }

    EXPECT_EQ(0, restore_image());

    EXPECT_EQ(1, g_row_count);
    set_null_video();
}

INSTANTIATE_TEST_SUITE_P(SkipAxis, TestGifViewMigs, Bool());

} // namespace id::test
