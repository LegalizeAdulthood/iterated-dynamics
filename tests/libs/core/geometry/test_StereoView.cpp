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

#include <ostream>
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
    void start_view(StereoView &view, GlassesType glasses);
    void finish_pair_view(StereoView &view, bool second_eye);

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

void TestStereoView::start_view(StereoView &view, const GlassesType glasses)
{
    g_glasses_type = glasses;
    view.start();
}

void TestStereoView::finish_pair_view(StereoView &view, const bool second_eye)
{
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
    g_viewport.enabled = true;
    view.start();
    if (second_eye)
    {
        view.next_view();
    }
    view.finish();
}

class TestInactiveStereoView : public TestStereoView
{
protected:
    void SetUp() override;
    void expect_unchanged() const;
};

void TestInactiveStereoView::SetUp()
{
    TestStereoView::SetUp();
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_viewport.enabled = true;
}

void TestInactiveStereoView::expect_unchanged() const
{
    EXPECT_EQ(StereoImage::BLUE, g_which_image);
    EXPECT_EQ(100, g_logical_screen.x_dots);
    EXPECT_EQ(7, g_logical_screen.x_offset);
    EXPECT_TRUE(g_viewport.enabled);
}

TEST_F(TestInactiveStereoView, reportsNoCombinedEyes)
{
    StereoView view;

    EXPECT_FALSE(view.both_eyes());
    expect_unchanged();
}

TEST_F(TestInactiveStereoView, requiresNoNextView)
{
    StereoView view;

    EXPECT_FALSE(view.next_view_required());
    expect_unchanged();
}

TEST_F(TestInactiveStereoView, rejectsNextView)
{
    StereoView view;

    EXPECT_THROW(view.next_view(), std::logic_error);
    expect_unchanged();
}

TEST_F(TestInactiveStereoView, finishPreservesStereoState)
{
    StereoView view;

    view.finish();

    expect_unchanged();
}

TEST_F(TestInactiveStereoView, destructionPreservesStereoState)
{
    {
        StereoView view;
    }

    expect_unchanged();
}

class TestMonoStereoView : public TestStereoView
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestMonoStereoView::SetUp()
{
    TestStereoView::SetUp();
    m_view.start();
}

TEST_F(TestMonoStereoView, selectsNormalPlotAndProjection)
{
    EXPECT_EQ(StereoImage::NONE, g_which_image);
    EXPECT_EQ(ignore_pixel, g_standard_plot);
    EXPECT_EQ(g_standard_plot, g_plot);
    EXPECT_EQ(10, g_x_shift);
    EXPECT_EQ(12, g_y_shift);
    EXPECT_EQ(2, g_xx_adjust);
    EXPECT_EQ(-2, g_yy_adjust);
    EXPECT_FALSE(m_view.both_eyes());
    EXPECT_FALSE(m_view.next_view_required());
}

TEST_F(TestMonoStereoView, rejectsDuplicateStart)
{
    EXPECT_THROW(m_view.start(), std::logic_error);
}

TEST_F(TestMonoStereoView, rejectsNextView)
{
    EXPECT_THROW(m_view.next_view(), std::logic_error);
}

class TestSeparateStereoModes : public TestStereoView, public WithParamInterface<GlassesType>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestSeparateStereoModes::SetUp()
{
    TestStereoView::SetUp();
    g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
    start_view(m_view, GetParam());
}

TEST_P(TestSeparateStereoModes, selectsEachEyeAndProjection)
{
    EXPECT_EQ(StereoImage::RED, g_which_image);
    EXPECT_EQ(g_standard_plot, g_plot);
    EXPECT_EQ(20, g_x_shift);
    EXPECT_EQ(0, g_x_shift1);
    EXPECT_EQ(5, g_xx_adjust);
    EXPECT_EQ(-1, g_xx_adjust1);
    EXPECT_FALSE(m_view.both_eyes());
    EXPECT_TRUE(m_view.next_view_required());
    if (GetParam() == GlassesType::STEREO_PAIR)
    {
        EXPECT_EQ(50, g_logical_screen.x_offset);
    }

    m_view.next_view();

    EXPECT_EQ(StereoImage::BLUE, g_which_image);
    EXPECT_EQ(g_standard_plot, g_plot);
    EXPECT_EQ(0, g_x_shift);
    EXPECT_EQ(-1, g_xx_adjust);
    EXPECT_FALSE(m_view.next_view_required());
    if (GetParam() == GlassesType::STEREO_PAIR)
    {
        EXPECT_EQ(150, g_logical_screen.x_offset);
    }
}

TEST_P(TestSeparateStereoModes, rejectsThirdView)
{
    m_view.next_view();

    EXPECT_THROW(m_view.next_view(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestSeparateStereoModes, Values(GlassesType::PHOTO, GlassesType::STEREO_PAIR),
    [](const TestParamInfo<GlassesType> &info) { return info.param == GlassesType::PHOTO ? "Photo" : "StereoPair"; });

struct CombinedStereoInput
{
    const char *name;
    GlassesType glasses;
    StereoImage eye;
};

void PrintTo(const CombinedStereoInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCombinedStereoModes : public TestStereoView, public WithParamInterface<CombinedStereoInput>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestCombinedStereoModes::SetUp()
{
    TestStereoView::SetUp();
    start_view(m_view, GetParam().glasses);
    ASSERT_EQ(StereoImage::RED, g_which_image);
    g_which_image = GetParam().eye;
}

TEST_P(TestCombinedStereoModes, doesNotRequireAnotherEye)
{
    EXPECT_NE(ignore_pixel, g_standard_plot);
    EXPECT_EQ(g_standard_plot, g_plot);
    EXPECT_TRUE(m_view.both_eyes());
    EXPECT_FALSE(m_view.next_view_required());
}

INSTANTIATE_TEST_SUITE_P(ModesAndEyes, TestCombinedStereoModes,
    Values(CombinedStereoInput{"Alternating_Red", GlassesType::ALTERNATING, StereoImage::RED},
        CombinedStereoInput{"Alternating_Blue", GlassesType::ALTERNATING, StereoImage::BLUE},
        CombinedStereoInput{"Superimposed_Red", GlassesType::SUPERIMPOSE, StereoImage::RED},
        CombinedStereoInput{"Superimposed_Blue", GlassesType::SUPERIMPOSE, StereoImage::BLUE}),
    [](const TestParamInfo<CombinedStereoInput> &info) { return info.param.name; });

class TestCombinedStereoRejection : public TestStereoView, public WithParamInterface<GlassesType>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestCombinedStereoRejection::SetUp()
{
    TestStereoView::SetUp();
    start_view(m_view, GetParam());
    g_which_image = StereoImage::BLUE;
}

TEST_P(TestCombinedStereoRejection, rejectsNextView)
{
    EXPECT_THROW(m_view.next_view(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestCombinedStereoRejection, Values(GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE),
    [](const TestParamInfo<GlassesType> &info)
    { return info.param == GlassesType::ALTERNATING ? "Alternating" : "Superimposed"; });

class TestRestoredStereoModes : public TestStereoView, public WithParamInterface<GlassesType>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestRestoredStereoModes::SetUp()
{
    TestStereoView::SetUp();
    g_glasses_type = GetParam();
    m_view.start(false);
}

TEST_P(TestRestoredStereoModes, requiresSeparateViewsForCombinedGlasses)
{
    EXPECT_EQ(StereoImage::RED, g_which_image);
    EXPECT_FALSE(m_view.both_eyes());
    ASSERT_TRUE(m_view.next_view_required());

    m_view.next_view();

    EXPECT_EQ(StereoImage::BLUE, g_which_image);
    EXPECT_FALSE(m_view.both_eyes());
    EXPECT_FALSE(m_view.next_view_required());
}

INSTANTIATE_TEST_SUITE_P(Modes, TestRestoredStereoModes, Values(GlassesType::ALTERNATING, GlassesType::SUPERIMPOSE),
    [](const TestParamInfo<GlassesType> &info)
    { return info.param == GlassesType::ALTERNATING ? "Alternating" : "Superimposed"; });

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

class TestDisabledStereoView : public TestStereoView
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestDisabledStereoView::SetUp()
{
    TestStereoView::SetUp();
    g_glasses_type = GlassesType::PHOTO;
    g_display_3d = Display3DMode::NONE;
    m_view.start();
}

TEST_F(TestDisabledStereoView, requiresNoNextView)
{
    EXPECT_FALSE(m_view.next_view_required());
    EXPECT_EQ(Display3DMode::NONE, g_display_3d);
}

TEST_F(TestDisabledStereoView, rejectsNextView)
{
    EXPECT_THROW(m_view.next_view(), std::logic_error);
}

class TestFinishedStereoEye : public TestStereoView, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestFinishedStereoEye::SetUp()
{
    TestStereoView::SetUp();
    finish_pair_view(m_view, GetParam());
}

TEST_P(TestFinishedStereoEye, restoresFullScreenAndFinishIsIdempotent)
{
    expect_full_screen();

    m_view.finish();

    expect_full_screen();
}

TEST_P(TestFinishedStereoEye, requiresNoNextView)
{
    EXPECT_FALSE(m_view.next_view_required());
}

TEST_P(TestFinishedStereoEye, rejectsNextView)
{
    EXPECT_THROW(m_view.next_view(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Eyes, TestFinishedStereoEye, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "SecondEye" : "FirstEye"; });

TEST_F(TestStereoView, completedViewCanRestartForAnotherEye)
{
    StereoView view;
    finish_pair_view(view, false);

    finish_pair_view(view, true);

    expect_full_screen();
}

class TestFailedStereoEye : public TestStereoView, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
};

void TestFailedStereoEye::SetUp()
{
    TestStereoView::SetUp();
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_logical_screen = {100, 60, 7, 9, 99.0, 59.0};
    g_viewport.enabled = true;
}

TEST_P(TestFailedStereoEye, unwindingRestoresFullScreen)
{
    EXPECT_THROW(
        {
            StereoView view;
            view.start();
            if (GetParam())
            {
                view.next_view();
            }
            throw std::runtime_error("View calculation failed");
        },
        std::runtime_error);

    expect_full_screen();
}

INSTANTIATE_TEST_SUITE_P(Eyes, TestFailedStereoEye, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "SecondEye" : "FirstEye"; });

class TestStereoViewportMode : public TestStereoView, public WithParamInterface<GlassesType>
{
protected:
    void SetUp() override;
    StereoView m_view;
};

void TestStereoViewportMode::SetUp()
{
    TestStereoView::SetUp();
    g_glasses_type = GetParam();
    g_screen_x_dots = 100;
    g_viewport.enabled = true;
    m_view.start();
    m_view.next_view();
}

TEST_P(TestStereoViewportMode, finishRetainsViewportWhenNotSideBySide)
{
    m_view.finish();

    EXPECT_EQ(100, g_logical_screen.x_dots);
    EXPECT_EQ(60, g_logical_screen.y_dots);
    EXPECT_EQ(7, g_logical_screen.x_offset);
    EXPECT_EQ(9, g_logical_screen.y_offset);
    EXPECT_TRUE(g_viewport.enabled);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestStereoViewportMode, Values(GlassesType::PHOTO, GlassesType::STEREO_PAIR),
    [](const TestParamInfo<GlassesType> &info) { return info.param == GlassesType::PHOTO ? "Photo" : "StereoPair"; });

} // namespace id::test
