// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ColorMapSaver.h"
#include "MockDriver.h"
#include "RestoreDecodeTestState.h"
#include "StopMessageTestSupport.h"
#include "TemporaryDirectory.h"

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

#include <filesystem>
#include <functional>
#include <optional>
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
    RestoreDecodeTestState m_saved_decode;
    ValueSaver<OutLine> m_saved_output{g_out_line, record_line};
    ValueSaver<decltype(g_out_line_cleanup)> m_saved_cleanup{g_out_line_cleanup, clean_up};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    std::optional<ColorMapSaver> m_saved_palette;
    TemporaryDirectory m_directory;
    std::vector<StereoImage> m_lines;
    int m_cleanup_count{};
};

void TestStereoRestore::SetUp()
{
    m_saved_palette.emplace();
    m_directory.create("id-stereo-restore-");
    set_save_library(m_directory.path());
    g_read_filename = m_directory.path() / "image.gif";
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
    m_saved_palette.reset();
    std::error_code ignored;
    m_directory.remove(ignored);
}

void TestStereoRestore::expect_full_screen() const
{
    EXPECT_EQ(12, g_logical_screen.x_dots);
    EXPECT_EQ(9, g_logical_screen.y_dots);
    EXPECT_EQ(0, g_logical_screen.x_offset);
    EXPECT_EQ(0, g_logical_screen.y_offset);
    EXPECT_FALSE(g_viewport.enabled);
}

class TestStereoRestoreGlasses : public TestStereoRestore, public WithParamInterface<GlassesType>
{
};

TEST_P(TestStereoRestoreGlasses, restoresAllScanlinesForBothEyes)
{
    g_glasses_type = GetParam();

    EXPECT_EQ(0, restore_image_views());

    EXPECT_THAT(m_lines,
        ElementsAre(StereoImage::RED, StereoImage::RED, StereoImage::RED, StereoImage::BLUE, StereoImage::BLUE,
            StereoImage::BLUE));
    EXPECT_EQ(1, m_cleanup_count);
    EXPECT_EQ(nullptr, g_out_line_cleanup);
    EXPECT_FALSE(g_busy);
    if (GetParam() == GlassesType::STEREO_PAIR)
    {
        expect_full_screen();
    }
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

INSTANTIATE_TEST_SUITE_P(Glasses, TestStereoRestoreGlasses,
    Values(GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE, GlassesType::STEREO_PAIR),
    [](const TestParamInfo<GlassesType> &info)
    {
        switch (info.param)
        {
        case GlassesType::ALTERNATING:
            return "Alternating";
        case GlassesType::SUPERIMPOSE:
            return "Superimpose";
        default:
            return "StereoPair";
        }
    });

class TestStereoRestoreSingleView : public TestStereoRestore, public WithParamInterface<bool>
{
};

TEST_P(TestStereoRestoreSingleView, restoresOnlyOnce)
{
    const bool disable_3d{GetParam()};
    g_glasses_type = disable_3d ? GlassesType::STEREO_PAIR : GlassesType::NONE;
    g_display_3d = disable_3d ? Display3DMode::NONE : Display3DMode::YES;

    EXPECT_EQ(0, restore_image_views());

    EXPECT_THAT(m_lines, Each(disable_3d ? StereoImage::RED : StereoImage::NONE));
    EXPECT_EQ(3, m_lines.size());
}

INSTANTIATE_TEST_SUITE_P(Configuration, TestStereoRestoreSingleView, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "Disabled3d" : "Mono"; });

class TestStereoRestoreFailedEye : public TestStereoRestore, public WithParamInterface<StereoImage>
{
};

TEST_P(TestStereoRestoreFailedEye, stopsRestorationAndCleansUp)
{
    const StereoImage eye{GetParam()};
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
    EXPECT_TRUE(std::filesystem::remove(g_read_filename));
}

INSTANTIATE_TEST_SUITE_P(Eye, TestStereoRestoreFailedEye, Values(StereoImage::RED, StereoImage::BLUE),
    [](const TestParamInfo<StereoImage> &info) { return info.param == StereoImage::RED ? "Red" : "Blue"; });

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
    expect_stop_messages(m_driver);
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("Output cleanup failed")));

    EXPECT_EQ(-1, restore_image_views());

    EXPECT_EQ(1, m_cleanup_count);
    EXPECT_EQ(nullptr, g_out_line_cleanup);
    EXPECT_EQ(BatchMode::BAILOUT_ERROR_NO_SAVE, g_init_batch);
}

} // namespace id::test
