// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/file_get_window.h>

#include <ColorMapSaver.h>
#include <MockDriver.h>
#include <PassTestFrame.h>
#include <TemporaryDirectory.h>

#include <engine/Browse.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/pixel.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>
#include <io/GifWriter.h>
#include <io/library.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/zoom.h>

#include <gif_lib.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <tuple>

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

class TestBigFloatBrowser : public TestWithParam<std::tuple<int, int>>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void write_image(int stored_precision);
    int browse();
    void expect_rejected();

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    PassTestFrame m_frame{8, 8};
    ColorMapSaver m_palette;
    ValueSaver<Browse> m_saved_browse{g_browse, Browse{}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<const FractalSpecific *> m_saved_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL_Z_POWER)};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<int> m_saved_colors{g_colors, 2};
    ValueSaver<int> m_saved_mask{g_and_color, 1};
    ValueSaver<bool> m_saved_first{g_first_init, true};
    ValueSaver<bool> m_saved_current_dir{g_check_cur_dir, false};
    ValueSaver<int> m_saved_box_count{g_box_count, 0};
    TemporaryDirectory m_directory;
    std::filesystem::path m_path;
    GifFileType m_extensions{};
};

void TestBigFloatBrowser::write_image(const int stored_precision)
{
    FractalInfo info{};
    std::copy_n(INFO_ID, sizeof(info.info_id), info.info_id);
    info.info_version = FRACTAL_INFO_VERSION;
    info.fractal_type = +FractalType::MANDEL_Z_POWER;
    info.bf_math = stored_precision != 0;
    info.bf_length = static_cast<std::int16_t>(stored_precision);
    info.x_min = info.y_min = info.x3rd = info.y3rd = -2;
    info.x_max = info.y_max = 2;
    info.x_dots = info.y_dots = 1;
    info.colors = 2;
    put_fractal_info(&m_extensions, info);
    if (stored_precision != 0)
    {
        init_bf_length(stored_precision);
        g_bf_x_min = g_bf_y_min = g_bf_x_3rd = g_bf_y_3rd = -2;
        g_bf_x_max = g_bf_y_max = 2;
        put_extended_param_info(&m_extensions, pack_bf_parameters());
    }
    GifWriter writer{m_path, {1, 1}, {{0, 0, 0}, {255, 255, 255}}};
    ASSERT_TRUE(writer.write_pixels({0}));
    ASSERT_TRUE(writer.write_extensions(m_extensions));
    ASSERT_TRUE(writer.finish());
}

void TestBigFloatBrowser::SetUp()
{
    m_directory.create("id-bigfloat-browser-");
    m_path = m_directory.path() / "image.gif";
    ASSERT_NO_FATAL_FAILURE(write_image(std::get<1>(GetParam())));
    init_bf_length(std::get<0>(GetParam()));
    g_image_region = {{-4, -4}, {4, 4}, {-4, -4}};
    g_bf_x_min = g_bf_y_min = g_bf_x_3rd = g_bf_y_3rd = -4;
    g_bf_x_max = g_bf_y_max = 4;
    g_browse.mask = m_directory.path() / "*.gif";
    clear_read_library_path();
    add_read_library(m_directory.path());
    set_pixel_functions(driver_write_pixel, driver_read_pixel);
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, read_pixel(_, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, buzzer(_)).Times(AnyNumber());
}

void TestBigFloatBrowser::TearDown()
{
    free_bf_vars();
    clear_read_library_path();
    GifFreeExtensions(&m_extensions.ExtensionBlockCount, &m_extensions.ExtensionBlocks);
    std::error_code ignored;
    m_directory.remove(ignored);
}

int TestBigFloatBrowser::browse()
{
    g_browse.name.clear();
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0)).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    return file_get_window();
}

TEST_P(TestBigFloatBrowser, preservesExactCornersInputsAndTemporaryLifetimeAcrossBrowsing)
{
    const auto bytes = pack_bf_parameters();
    const auto saved = save_stack();
    const int bf_length = g_bf_length;
    const int bn_length = g_bn_length;
    const int decimals = g_decimals;

    EXPECT_EQ(ID_KEY_ENTER, browse());

    EXPECT_EQ(m_path, g_browse.selected_path);
    EXPECT_EQ(2, g_box_x[0]);
    EXPECT_EQ(2, g_box_y[0]);
    EXPECT_EQ(5, g_box_x[1]);
    EXPECT_EQ(2, g_box_y[1]);
    EXPECT_EQ(2, g_box_x[2]);
    EXPECT_EQ(5, g_box_y[2]);
    EXPECT_EQ(5, g_box_x[3]);
    EXPECT_EQ(5, g_box_y[3]);
    EXPECT_EQ(bytes, pack_bf_parameters());
    EXPECT_EQ(saved, save_stack());
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(decimals, g_decimals);
    const auto high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();

    EXPECT_EQ(ID_KEY_ENTER, browse());

    EXPECT_EQ(m_path, g_browse.selected_path);
    EXPECT_EQ(bytes, pack_bf_parameters());
    EXPECT_EQ(saved, save_stack());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(decimals, g_decimals);
}

TEST_P(TestBigFloatBrowser, preservesOrdinaryViewAcrossBrowsing)
{
    free_bf_vars();

    EXPECT_EQ(ID_KEY_ENTER, browse());

    EXPECT_EQ(m_path, g_browse.selected_path);
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_FALSE(g_bf_x_min);
    EXPECT_EQ(2, g_box_x[0]);
    EXPECT_EQ(2, g_box_y[0]);
    EXPECT_EQ(5, g_box_x[3]);
    EXPECT_EQ(5, g_box_y[3]);

    EXPECT_EQ(ID_KEY_ENTER, browse());

    EXPECT_EQ(m_path, g_browse.selected_path);
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_FALSE(g_bf_x_min);
}

void TestBigFloatBrowser::expect_rejected()
{
    EXPECT_CALL(m_driver, key_pressed()).Times(2).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, wait_key_pressed(false)).Times(2).WillRepeatedly(Return(0));
    const auto bytes = pack_bf_parameters();
    const auto saved = save_stack();

    EXPECT_EQ(0, file_get_window());

    EXPECT_FALSE(g_browse.sub_images);
    EXPECT_TRUE(g_browse.selected_path.empty());
    EXPECT_EQ(bytes, pack_bf_parameters());
    EXPECT_EQ(saved, save_stack());
    const auto high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();

    EXPECT_EQ(0, file_get_window());

    EXPECT_FALSE(g_browse.sub_images);
    EXPECT_EQ(bytes, pack_bf_parameters());
    EXPECT_EQ(saved, save_stack());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
}

TEST_P(TestBigFloatBrowser, releasesTemporaryValuesWhenWindowIsTooSmall)
{
    g_browse.smallest_window = 100;

    expect_rejected();
}

TEST_P(TestBigFloatBrowser, releasesTemporaryValuesWhenAllCornersAreOffScreen)
{
    g_image_region = {{8, 8}, {16, 16}, {8, 8}};
    g_bf_x_min = g_bf_y_min = g_bf_x_3rd = g_bf_y_3rd = 8;
    g_bf_x_max = g_bf_y_max = 16;

    expect_rejected();
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatBrowser, Combine(Values(32, 56), Values(0, 4, 32, 56, 200)));

} // namespace id::test
