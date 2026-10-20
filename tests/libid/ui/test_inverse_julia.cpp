// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/inverse_julia.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/editpal.h>
#include <ui/help.h>
#include <ui/id_keys.h>
#include <ui/mouse.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestInverseJuliaController : public Test
{
protected:
    using Plot = void (*)(int, int, int);

    void SetUp() override
    {
        set_fractal_type(FractalType::MANDEL);
        set_normal_dot();
        set_normal_span();
        alloc_pixel_grid();
        EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels[y * 100 + x]; }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int x, int y, int color) { m_pixels[y * 100 + x] = color; }));
    }

    void TearDown() override
    {
        free_pixel_grid();
    }

    void expect_exit_key(int key)
    {
        EXPECT_CALL(m_driver, key_pressed())
            .WillRepeatedly(Invoke(
                [this, key]()
                {
                    EXPECT_TRUE(g_using_jiim);
                    EXPECT_EQ(MouseLook::POSITION, g_look_at_mouse);
                    EXPECT_TRUE(g_cursor_mouse_tracking);
                    return m_key_read ? 0 : key;
                }));
        EXPECT_CALL(m_driver, get_key())
            .WillOnce(Invoke(
                [this, key]()
                {
                    m_key_read = true;
                    return key;
                }));
        EXPECT_CALL(m_driver, unget_key(key))
            .WillOnce(Invoke(
                [](int)
                {
                    EXPECT_FALSE(g_using_jiim);
                    EXPECT_EQ(MouseLook::IGNORE_MOUSE, g_look_at_mouse);
                    EXPECT_FALSE(g_cursor_mouse_tracking);
                }));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{100, 100, 0, 0, 99, 99}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 100};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 100};
    ValueSaver<int> m_saved_vesa_width{g_vesa_x_res};
    ValueSaver<int> m_saved_vesa_height{g_vesa_y_res};
    ValueSaver<int> m_saved_video_x{g_video_start_x, 0};
    ValueSaver<int> m_saved_video_y{g_video_start_y, 0};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
    ValueSaver<Plot> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<bool> m_saved_inverse{g_has_inverse, true};
    ValueSaver<bool> m_saved_dac{g_got_real_dac, false};
    ValueSaver<bool> m_saved_using_jiim{g_using_jiim, false};
    ValueSaver<MouseLook> m_saved_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    ValueSaver<bool> m_saved_tracking{g_cursor_mouse_tracking, false};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::PREVENT_MIIM};
    ValueSaver<HelpLabels> m_saved_help{g_help_mode, HelpLabels::NONE};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}}};
    ValueSaver<LDouble> m_saved_dx{g_delta_x, 4.0 / 99};
    ValueSaver<LDouble> m_saved_dy{g_delta_y, 3.0 / 99};
    ValueSaver<LDouble> m_saved_dx2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_dy2{g_delta_y2, 0};
    ValueSaver<DComplex> m_saved_selection{g_save_c, DComplex{-0.5, 0}};
    ValueSaver<DComplex> m_saved_julia_c{g_julia_c};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<std::vector<Byte>> m_saved_line{g_line_buff};
    std::vector<int> m_pixels = std::vector<int>(100 * 100, 3);
    bool m_key_read{};
};

TEST_F(TestInverseJuliaController, selectionRestoresDisplayAndRequeuesKey)
{
    expect_exit_key(ID_KEY_SPACE);

    jiim(JIIMType::JIIM);

    EXPECT_DOUBLE_EQ(-0.5, g_julia_c.x);
    EXPECT_DOUBLE_EQ(0, g_julia_c.y);
    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_TRUE(g_line_buff.empty());
    EXPECT_EQ(DebugFlags::PREVENT_MIIM, g_debug_flag);
    EXPECT_EQ(HelpLabels::NONE, g_help_mode);
    EXPECT_FALSE(dispatch_keyboard_key(ID_KEY_LEFT_ARROW));
}

TEST_F(TestInverseJuliaController, menuExitRestoresControllerState)
{
    expect_exit_key('x');

    jiim(JIIMType::JIIM);

    EXPECT_DOUBLE_EQ(JULIA_C_NOT_SET, g_julia_c.x);
    EXPECT_DOUBLE_EQ(JULIA_C_NOT_SET, g_julia_c.y);
    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_TRUE(g_line_buff.empty());
    EXPECT_FALSE(dispatch_keyboard_key(ID_KEY_LEFT_ARROW));
}

TEST_F(TestInverseJuliaController, mouseScopeTracksCursorAndUnsubscribes)
{
    CrossHairCursor cursor;
    {
        InverseJuliaMouseScope scope{cursor};

        mouse_notify_move(23, 31, 0);

        EXPECT_EQ(23, cursor.get_x());
        EXPECT_EQ(31, cursor.get_y());
        EXPECT_EQ(MouseLook::POSITION, g_look_at_mouse);
        EXPECT_TRUE(g_cursor_mouse_tracking);
    }

    mouse_notify_move(70, 80, 0);

    EXPECT_EQ(23, cursor.get_x());
    EXPECT_EQ(31, cursor.get_y());
    EXPECT_EQ(MouseLook::IGNORE_MOUSE, g_look_at_mouse);
    EXPECT_FALSE(g_cursor_mouse_tracking);
}

} // namespace id::test
