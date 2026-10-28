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
#include <algorithm>
#include <filesystem>
#include <memory>
#include <ostream>
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

struct GifPaletteDetectionInput
{
    const char *name;
    Version version;
    std::array<Byte, 3> color;
    int count;
    bool expected;
};

void PrintTo(const GifPaletteDetectionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLegacy13GifPaletteDetection : public TestWithParam<GifPaletteDetectionInput>
{
protected:
    void SetUp() override;

    Byte m_palette[256][3]{};
};

void TestLegacy13GifPaletteDetection::SetUp()
{
    Test::SetUp();
    std::copy(GetParam().color.begin(), GetParam().color.end(), m_palette[0]);
}

TEST_P(TestLegacy13GifPaletteDetection, detectsPaletteCompatibility)
{
    EXPECT_EQ(GetParam().expected, backwards_id1_3_palette_needed(GetParam().version, m_palette, GetParam().count));
}

INSTANTIATE_TEST_SUITE_P(DetectionInputs, TestLegacy13GifPaletteDetection,
    Values(GifPaletteDetectionInput{"BeforeRange", Version{1, 3, 0, 0, false}, {252, 168, 84}, 1, false},
        GifPaletteDetectionInput{"RangeStart", Version{1, 3, 1, 0, false}, {252, 168, 84}, 1, true},
        GifPaletteDetectionInput{"WithinRange", Version{1, 3, 2, 9, false}, {252, 168, 84}, 1, true},
        GifPaletteDetectionInput{"AfterRange", Version{1, 3, 3, 0, false}, {252, 168, 84}, 1, false},
        GifPaletteDetectionInput{"NonQuantized", Version{1, 3, 1, 0, false}, {253, 0, 0}, 1, false}),
    [](const TestParamInfo<GifPaletteDetectionInput> &info) { return info.param.name; });

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

class TestGifViewRestore : public Test
{
protected:
    void SetUp() override;
    void create_gif(int width, int left = 0, int top = 0);
    void expect_no_input();

    MockDriver m_driver;
    std::unique_ptr<TestGifOutputFile> m_gif;

private:
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<std::filesystem::path> m_read_filename{g_read_filename};
    ValueSaver<OutLine> m_out_line{g_out_line, out_line};
    ValueSaver<bool> m_read_color{g_read_color, true};
    ValueSaver<ColorState> m_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver<Display3DMode> m_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_map_set{g_map_set, false};
    ValueSaver<int> m_colors{g_colors, 256};
    ValueSaver<bool> m_dither_flag{g_dither_flag, false};
    ValueSaver<bool> m_potential_16bit{g_potential.store_16bit, false};
    ValueSaver<int> m_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver<int> m_skip_y_dots{g_skip_y_dots, 0};
    ValueSaver<CalcStatus> m_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<bool> m_busy{g_busy, false};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen};
    ValueSaver<int> m_row_count{g_row_count};
    ValueSaver<std::vector<int>> m_line_lengths{s_line_lengths};
    ValueSaver<std::vector<std::array<int, 3>>> m_sound_pixels{s_sound_pixels};
};

void TestGifViewRestore::SetUp()
{
    Test::SetUp();
    s_line_lengths.clear();
    s_sound_pixels.clear();
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
}

void TestGifViewRestore::create_gif(const int width, const int left, const int top)
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{std::string{info->test_suite_name()} + "-" + info->name()};
    std::replace(name.begin(), name.end(), '/', '-');
    m_gif = std::make_unique<TestGifOutputFile>(
        std::filesystem::temp_directory_path() / ("id-gifview-" + name + ".gif"), width, left, top);
    g_read_filename = m_gif->path();
}

void TestGifViewRestore::expect_no_input()
{
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
}

class TestGifViewLineWidth : public TestGifViewRestore, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestGifViewLineWidth::SetUp()
{
    TestGifViewRestore::SetUp();
    create_gif(GetParam());
}

TEST_P(TestGifViewLineWidth, positiveCallbackResultPreservesFullWidthAndSuccessfulRestore)
{
    g_out_line = capture_positive_out_line;
    expect_no_input();

    EXPECT_EQ(0, restore_image());

    ASSERT_EQ(1, s_line_lengths.size());
    EXPECT_EQ(GetParam(), s_line_lengths[0]);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(m_gif->path()));
}

TEST_P(TestGifViewLineWidth, callbackFailureClosesFileAndClearsBusyState)
{
    g_out_line = [](Byte *, int)
    {
        EXPECT_TRUE(g_busy);
        return -73;
    };

    EXPECT_EQ(-73, restore_image());

    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(m_gif->path()));
}

INSTANTIATE_TEST_SUITE_P(Widths, TestGifViewLineWidth, Values(640, MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

class TestGifViewSound : public TestGifViewRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

private:
    ValueSaver<int> m_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver<decltype(g_put_color)> m_put_color{g_put_color, capture_sound_pixel};
    ValueSaver<int> m_orbit_delay{g_orbit_delay, 0};
    ValueSaver<int> m_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_base_hertz{g_base_hertz, 220};
};

void TestGifViewSound::SetUp()
{
    TestGifViewRestore::SetUp();
    create_gif(8);
    g_logical_screen = LogicalScreen{4, 2, 0, 0, 3.0, 1.0};
}

TEST_P(TestGifViewSound, restoresDecodedPixelsAcrossDisplayRowsWithoutPollingInput)
{
    const bool supported{GetParam()};
    EXPECT_CALL(m_driver, sound_on(220)).Times(8).WillRepeatedly(Return(supported));
    EXPECT_CALL(m_driver, sound_off()).Times(supported ? 8 : 0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    expect_no_input();

    EXPECT_EQ(0, restore_image());

    const std::vector<std::array<int, 3>> expected{
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {3, 1, 0}};
    EXPECT_EQ(expected, s_sound_pixels);
    EXPECT_EQ(2, g_row_count);
}

INSTANTIATE_TEST_SUITE_P(DriverSound, TestGifViewSound, Bool());

class TestGifViewMigs : public TestGifViewRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    void TearDown() override;

private:
    ValueSaver<int> m_screen_x{g_screen_x_dots, 8};
    ValueSaver<int> m_screen_y{g_screen_y_dots, 8};
};

void TestGifViewMigs::SetUp()
{
    TestGifViewRestore::SetUp();
    const bool skip_x{GetParam()};
    create_gif(4, 3, 4);
    g_logical_screen = LogicalScreen{8, 8, 0, 0, 1.0, 1.0};
    g_skip_x_dots = skip_x ? 1 : 0;
    g_skip_y_dots = skip_x ? 0 : 1;
    set_normal_dot();
}

void TestGifViewMigs::TearDown()
{
    set_null_video();
    TestGifViewRestore::TearDown();
}

TEST_P(TestGifViewMigs, reducesImageOffsetsAlongTheirOwnAxisWithoutPollingInput)
{
    const bool skip_x{GetParam()};
    expect_no_input();
    const int left{skip_x ? 1 : 3};
    const int top{skip_x ? 4 : 2};
    const int width{skip_x ? 2 : 4};
    for (int col = left; col < left + width; ++col)
    {
        EXPECT_CALL(m_driver, write_pixel(col, top, 0));
    }

    EXPECT_EQ(0, restore_image());

    EXPECT_EQ(1, g_row_count);
}

INSTANTIATE_TEST_SUITE_P(SkipAxis, TestGifViewMigs, Bool());

} // namespace id::test
