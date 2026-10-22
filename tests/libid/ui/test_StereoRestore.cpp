// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/Potential.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/lorenz.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <io/decoder.h>
#include <io/gifview.h>
#include <io/GifWriter.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/id_keys.h>
#include <ui/restore_image.h>
#include <ui/slideshw.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{
namespace
{

std::function<int(Byte *, int)> s_output;
std::function<void()> s_cleanup;

int record_line(Byte *pixels, const int length)
{
    return s_output(pixels, length);
}

void clean_up()
{
    s_cleanup();
}

} // namespace

class TestStereoRestore : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void expect_full_screen() const;

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{4, 3, 0, 0, 3.0, 2.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 12};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 9};
    ValueSaver<Viewport> m_saved_viewport{g_viewport};
    ValueSaver<GlassesType> m_saved_glasses{g_glasses_type, GlassesType::STEREO_PAIR};
    ValueSaver<StereoImage> m_saved_eye{g_which_image};
    ValueSaver<Display3DMode> m_saved_display{g_display_3d, Display3DMode::YES};
    ValueSaver<WritePixel> m_saved_plot{g_plot};
    ValueSaver<WritePixel> m_saved_standard_plot{g_standard_plot};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_row{g_row_count, 0};
    ValueSaver<OutLine> m_saved_output{g_out_line, record_line};
    ValueSaver<decltype(g_out_line_cleanup)> m_saved_cleanup{g_out_line_cleanup, clean_up};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<bool> m_saved_read_color{g_read_color, true};
    ValueSaver<ColorState> m_saved_color_state{g_color_state};
    ValueSaver<bool> m_saved_map{g_map_set, false};
    ValueSaver<bool> m_saved_dither{g_dither_flag, false};
    ValueSaver<int> m_saved_skip_x{g_skip_x_dots, 0};
    ValueSaver<int> m_saved_skip_y{g_skip_y_dots, 0};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<bool> m_saved_busy{g_busy, false};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::filesystem::path> m_saved_filename{g_read_filename};
    Byte m_palette[256][3]{};
    std::filesystem::path m_directory;
    std::vector<StereoImage> m_lines;
    int m_cleanup_count{};
};

void TestStereoRestore::SetUp()
{
    std::memcpy(m_palette, g_dac_box, sizeof(m_palette));
    m_directory = std::filesystem::temp_directory_path() /
        ("id-stereo-restore-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    set_save_library(m_directory);
    g_read_filename = m_directory / "image.gif";
    GifWriter::Palette palette(256);
    palette[1] = {255, 0, 0};
    GifWriter writer{g_read_filename, {4, 3}, palette};
    for (int row = 0; row < 3; ++row)
    {
        ASSERT_TRUE(writer.write_pixels({1, 2, 3, 4}));
    }
    ASSERT_TRUE(writer.finish());
    g_viewport.enabled = true;
    s_output = [this](Byte *pixels, const int length)
    {
        EXPECT_EQ(4, length);
        EXPECT_THAT(std::vector<Byte>(pixels, pixels + length), ElementsAre(1, 2, 3, 4));
        EXPECT_EQ(0, m_cleanup_count);
        m_lines.push_back(g_which_image);
        ++g_row_count;
        return 0;
    };
    s_cleanup = [this]
    {
        EXPECT_EQ(nullptr, g_out_line_cleanup);
        ++m_cleanup_count;
    };
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, check_memory()).Times(AnyNumber());
    EXPECT_CALL(m_driver, write_palette()).Times(AnyNumber());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(_)).Times(0);
}

void TestStereoRestore::TearDown()
{
    s_output = {};
    s_cleanup = {};
    clear_save_library();
    std::memcpy(g_dac_box, m_palette, sizeof(m_palette));
    std::error_code ignored;
    std::filesystem::remove_all(m_directory, ignored);
}

void TestStereoRestore::expect_full_screen() const
{
    EXPECT_EQ(12, g_logical_screen.x_dots);
    EXPECT_EQ(9, g_logical_screen.y_dots);
    EXPECT_EQ(0, g_logical_screen.x_offset);
    EXPECT_EQ(0, g_logical_screen.y_offset);
    EXPECT_FALSE(g_viewport.enabled);
}

TEST_F(TestStereoRestore, separateDecodesRestoreAllScanlinesForBothEyes)
{
    for (const GlassesType glasses : {GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE, GlassesType::STEREO_PAIR})
    {
        g_glasses_type = glasses;
        g_logical_screen = {4, 3, 0, 0, 3.0, 2.0};
        g_out_line_cleanup = clean_up;
        m_cleanup_count = 0;
        m_lines.clear();

        EXPECT_EQ(0, restore_image_views());

        EXPECT_THAT(m_lines,
            ElementsAre(StereoImage::RED, StereoImage::RED, StereoImage::RED, StereoImage::BLUE, StereoImage::BLUE,
                StereoImage::BLUE));
        EXPECT_EQ(1, m_cleanup_count);
        EXPECT_EQ(nullptr, g_out_line_cleanup);
        EXPECT_FALSE(g_busy);
    }
    expect_full_screen();
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestStereoRestore, monoAndDisabled3dRestoreOnlyOnce)
{
    for (const bool disable_3d : {false, true})
    {
        g_glasses_type = disable_3d ? GlassesType::STEREO_PAIR : GlassesType::NONE;
        g_display_3d = disable_3d ? Display3DMode::NONE : Display3DMode::YES;
        m_cleanup_count = 0;
        g_out_line_cleanup = clean_up;
        m_lines.clear();

        EXPECT_EQ(0, restore_image_views());

        EXPECT_THAT(m_lines, Each(disable_3d ? StereoImage::RED : StereoImage::NONE));
        EXPECT_EQ(3, m_lines.size());
    }
}

TEST_F(TestStereoRestore, failedScanlineStopsRestorationAndCleansUpEitherEye)
{
    for (const StereoImage eye : {StereoImage::RED, StereoImage::BLUE})
    {
        g_logical_screen = {4, 3, 0, 0, 3.0, 2.0};
        g_out_line_cleanup = clean_up;
        m_cleanup_count = 0;
        m_lines.clear();
        s_output = [this, eye](Byte *, int)
        {
            EXPECT_EQ(0, m_cleanup_count);
            m_lines.push_back(g_which_image);
            ++g_row_count;
            return g_which_image == eye ? -1 : 0;
        };

        EXPECT_EQ(-1, restore_image_views());

        EXPECT_EQ(eye == StereoImage::RED ? 1 : 4, m_lines.size());
        EXPECT_EQ(1, m_cleanup_count);
        expect_full_screen();
        EXPECT_FALSE(g_busy);
    }
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestStereoRestore, exceptionalScanlineRestoresScreenAndReleasesOutput)
{
    s_output = [](Byte *, int) -> int { throw std::runtime_error("Restore failed"); };
    s_cleanup = [this]
    {
        expect_full_screen();
        ++m_cleanup_count;
    };

    EXPECT_THROW(restore_image_views(), std::runtime_error);

    EXPECT_EQ(1, m_cleanup_count);
    EXPECT_EQ(nullptr, g_out_line_cleanup);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

TEST_F(TestStereoRestore, missingImageStillRestoresScreenAndReleasesOutput)
{
    ASSERT_TRUE(std::filesystem::remove(g_read_filename));

    EXPECT_EQ(-1, restore_image_views());

    EXPECT_TRUE(m_lines.empty());
    EXPECT_EQ(1, m_cleanup_count);
    expect_full_screen();
}

TEST_F(TestStereoRestore, cleanupFailureReportsErrorAndAbandonsBatchWithoutSaving)
{
    g_init_batch = BatchMode::FINISH_CALC_BEFORE_SAVE;
    s_cleanup = [this]
    {
        expect_full_screen();
        ++m_cleanup_count;
        throw std::runtime_error("Output cleanup failed");
    };
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, stack_screen());
    EXPECT_CALL(m_driver, unstack_screen());
    EXPECT_CALL(m_driver, move_cursor(_, _));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _));
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("Output cleanup failed")));

    EXPECT_EQ(-1, restore_image_views());

    EXPECT_EQ(1, m_cleanup_count);
    EXPECT_EQ(nullptr, g_out_line_cleanup);
    EXPECT_EQ(BatchMode::BAILOUT_ERROR_NO_SAVE, g_init_batch);
}

} // namespace id::test
