// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <geometry/StereoView.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::misc;
using namespace testing;

namespace id::test
{

namespace
{

void ignore_pixel(int, int, int)
{
}

} // namespace

class TestStereoView : public Test
{
protected:
    void expect_full_screen() const
    {
        EXPECT_EQ(300, g_logical_screen.x_dots);
        EXPECT_EQ(180, g_logical_screen.y_dots);
        EXPECT_EQ(0, g_logical_screen.x_offset);
        EXPECT_EQ(0, g_logical_screen.y_offset);
        EXPECT_FALSE(g_viewport.enabled);
    }

    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{100, 60, 7, 9, 99.0, 59.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 300};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 180};
    ValueSaver<Viewport> m_saved_viewport{g_viewport};
    ValueSaver<GlassesType> m_saved_glasses{g_glasses_type, GlassesType::NONE};
    ValueSaver<StereoImage> m_saved_eye{g_which_image, StereoImage::BLUE};
    ValueSaver<Display3DMode> m_saved_display{g_display_3d, Display3DMode::MINUS_ONE};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::IFS};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<WritePixel> m_saved_put_color{g_put_color, ignore_pixel};
    ValueSaver<WritePixel> m_saved_plot{g_plot};
    ValueSaver<WritePixel> m_saved_standard_plot{g_standard_plot};
    ValueSaver<int> m_saved_x_rot{g_x_rot, 0};
    ValueSaver<int> m_saved_y_rot{g_y_rot, 0};
    ValueSaver<int> m_saved_shift_x{g_shift_x, 10};
    ValueSaver<int> m_saved_shift_y{g_shift_y, 20};
    ValueSaver<int> m_saved_separation{g_eye_separation, 20};
    ValueSaver<int> m_saved_convergence{g_converge_x_adjust, 3};
    ValueSaver<math::Point2i> m_saved_adjust{g_adjust_3d, math::Point2i{2, 4}};
    ValueSaver<int> m_saved_x_shift{g_x_shift};
    ValueSaver<int> m_saved_y_shift{g_y_shift};
    ValueSaver<int> m_saved_x_shift1{g_x_shift1};
    ValueSaver<int> m_saved_y_shift1{g_y_shift1};
    ValueSaver<int> m_saved_xx_adjust{g_xx_adjust};
    ValueSaver<int> m_saved_yy_adjust{g_yy_adjust};
    ValueSaver<int> m_saved_xx_adjust1{g_xx_adjust1};
};

TEST_F(TestStereoView, inactiveViewDoesNotChangeStereoState)
{
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_viewport.enabled = true;
    {
        StereoView view;
        EXPECT_FALSE(view.both_eyes());
        EXPECT_FALSE(view.next_view_required());
        EXPECT_THROW(view.next_view(), std::logic_error);
        view.finish();
    }

    EXPECT_EQ(StereoImage::BLUE, g_which_image);
    EXPECT_EQ(100, g_logical_screen.x_dots);
    EXPECT_EQ(7, g_logical_screen.x_offset);
    EXPECT_TRUE(g_viewport.enabled);
}

TEST_F(TestStereoView, monoViewSelectsNormalPlotAndProjection)
{
    StereoView view;

    view.start();

    EXPECT_EQ(StereoImage::NONE, g_which_image);
    EXPECT_EQ(ignore_pixel, g_standard_plot);
    EXPECT_EQ(g_standard_plot, g_plot);
    EXPECT_EQ(10, g_x_shift);
    EXPECT_EQ(12, g_y_shift);
    EXPECT_EQ(2, g_xx_adjust);
    EXPECT_EQ(-2, g_yy_adjust);
    EXPECT_FALSE(view.both_eyes());
    EXPECT_FALSE(view.next_view_required());
    EXPECT_THROW(view.start(), std::logic_error);
    EXPECT_THROW(view.next_view(), std::logic_error);
}

TEST_F(TestStereoView, separateViewsSelectEachEyeAndProjection)
{
    for (const GlassesType glasses : {GlassesType::PHOTO, GlassesType::STEREO_PAIR})
    {
        g_glasses_type = glasses;
        g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
        StereoView view;
        view.start();

        EXPECT_EQ(StereoImage::RED, g_which_image);
        EXPECT_EQ(g_standard_plot, g_plot);
        EXPECT_EQ(20, g_x_shift);
        EXPECT_EQ(0, g_x_shift1);
        EXPECT_EQ(5, g_xx_adjust);
        EXPECT_EQ(-1, g_xx_adjust1);
        EXPECT_FALSE(view.both_eyes());
        EXPECT_TRUE(view.next_view_required());
        if (glasses == GlassesType::STEREO_PAIR)
        {
            EXPECT_EQ(50, g_logical_screen.x_offset);
        }

        view.next_view();

        EXPECT_EQ(StereoImage::BLUE, g_which_image);
        EXPECT_EQ(g_standard_plot, g_plot);
        EXPECT_EQ(0, g_x_shift);
        EXPECT_EQ(-1, g_xx_adjust);
        EXPECT_FALSE(view.next_view_required());
        EXPECT_THROW(view.next_view(), std::logic_error);
        if (glasses == GlassesType::STEREO_PAIR)
        {
            EXPECT_EQ(150, g_logical_screen.x_offset);
        }
    }
}

TEST_F(TestStereoView, combinedViewsDoNotRequireAnotherEye)
{
    for (const GlassesType glasses : {GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE})
    {
        g_glasses_type = glasses;
        StereoView view;
        view.start();

        EXPECT_EQ(StereoImage::RED, g_which_image);
        EXPECT_NE(ignore_pixel, g_standard_plot);
        EXPECT_EQ(g_standard_plot, g_plot);
        EXPECT_TRUE(view.both_eyes());
        EXPECT_FALSE(view.next_view_required());
        g_which_image = StereoImage::BLUE;
        EXPECT_TRUE(view.both_eyes());
        EXPECT_FALSE(view.next_view_required());
        EXPECT_THROW(view.next_view(), std::logic_error);
    }
}

TEST_F(TestStereoView, restorationRequiresSeparateViewsForCombinedGlasses)
{
    for (const GlassesType glasses : {GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE})
    {
        g_glasses_type = glasses;
        StereoView view;
        view.start(false);

        EXPECT_EQ(StereoImage::RED, g_which_image);
        EXPECT_FALSE(view.both_eyes());
        ASSERT_TRUE(view.next_view_required());

        view.next_view();

        EXPECT_EQ(StereoImage::BLUE, g_which_image);
        EXPECT_FALSE(view.both_eyes());
        EXPECT_FALSE(view.next_view_required());
    }
}

TEST_F(TestStereoView, viewSelectionDoesNotUseTheLastPlottedEye)
{
    g_glasses_type = GlassesType::PHOTO;
    StereoView view;
    view.start();

    g_which_image = StereoImage::BLUE;

    EXPECT_TRUE(view.next_view_required());
    view.next_view();
    EXPECT_FALSE(view.next_view_required());
}

TEST_F(TestStereoView, disabled3dDisplayDoesNotRequireAnotherEye)
{
    g_glasses_type = GlassesType::PHOTO;
    g_display_3d = Display3DMode::NONE;
    StereoView view;
    view.start();

    EXPECT_FALSE(view.next_view_required());
    EXPECT_THROW(view.next_view(), std::logic_error);
    EXPECT_EQ(Display3DMode::NONE, g_display_3d);
}

TEST_F(TestStereoView, finishRestoresFullScreenFromEitherEye)
{
    g_glasses_type = GlassesType::STEREO_PAIR;
    StereoView view;
    for (const bool second_eye : {false, true})
    {
        g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
        g_viewport.enabled = true;
        view.start();
        if (second_eye)
        {
            view.next_view();
        }

        view.finish();

        expect_full_screen();
        EXPECT_FALSE(view.next_view_required());
        EXPECT_THROW(view.next_view(), std::logic_error);
        view.finish();
        expect_full_screen();
    }
}

TEST_F(TestStereoView, failureUnwindingRestoresFullScreenFromEitherEye)
{
    g_glasses_type = GlassesType::STEREO_PAIR;
    for (const bool second_eye : {false, true})
    {
        g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
        g_viewport.enabled = true;
        EXPECT_THROW(
            {
                StereoView view;
                view.start();
                if (second_eye)
                {
                    view.next_view();
                }
                throw std::runtime_error("View calculation failed");
            },
            std::runtime_error);

        expect_full_screen();
    }
}

TEST_F(TestStereoView, finishRetainsViewportWhenItIsNotASideBySidePair)
{
    for (const GlassesType glasses : {GlassesType::PHOTO, GlassesType::STEREO_PAIR})
    {
        g_glasses_type = glasses;
        g_screen_x_dots = 100;
        g_viewport.enabled = true;
        StereoView view;
        view.start();
        view.next_view();

        view.finish();

        EXPECT_EQ(100, g_logical_screen.x_dots);
        EXPECT_EQ(60, g_logical_screen.y_dots);
        EXPECT_EQ(7, g_logical_screen.x_offset);
        EXPECT_EQ(9, g_logical_screen.y_offset);
        EXPECT_TRUE(g_viewport.enabled);
    }
}

} // namespace id::test
