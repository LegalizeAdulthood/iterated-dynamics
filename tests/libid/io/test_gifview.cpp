// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadfile.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/pixel_limits.h>
#include <engine/Potential.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/decoder.h>
#include <io/gifview.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/slideshw.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

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

int capture_out_line(Byte *, const int line_len)
{
    s_line_lengths.push_back(line_len);
    return 0;
}

class TestGifOutputFile
{
public:
    TestGifOutputFile(const std::filesystem::path &path, const int width) :
        m_path(path)
    {
        write(width);
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
    void write(const int width) const
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
        const bool failed = EGifPutScreenDesc(gif, width, 1, 2, 0, color_map) == GIF_ERROR ||
            EGifPutImageDesc(gif, 0, 0, width, 1, false, nullptr) == GIF_ERROR ||
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

TEST_P(TestGifViewLineWidth, decodesSingleLineAtFullWidth)
{
    const int width{GetParam()};
    const TestGifOutputFile gif{
        std::filesystem::temp_directory_path() / ("id-gifview-" + std::to_string(width) + ".gif"), width};
    MockDriver driver;
    ValueSaver<Driver *> saved_driver{g_driver, &driver};
    ValueSaver saved_read_filename{g_read_filename, gif.path()};
    ValueSaver<int (*)(Byte *, int)> saved_out_line{g_out_line, capture_out_line};
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

    EXPECT_EQ(0, gif_view());

    ASSERT_EQ(1, s_line_lengths.size());
    EXPECT_EQ(width, s_line_lengths[0]);
}

INSTANTIATE_TEST_SUITE_P(Widths, TestGifViewLineWidth, Values(640, MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

} // namespace id::test
