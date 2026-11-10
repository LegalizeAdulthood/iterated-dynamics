// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/zoom.h>
#include <math/BigFloat.h>

#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/Viewport.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <ostream>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

struct ResumeCase
{
    bool translated;
    double zoom_width;
    CalcStatus status;
    CalcStatus expected;
};

void PrintTo(const ResumeCase &value, std::ostream *out)
{
    *out << "translated=" << value.translated << ", width=" << value.zoom_width
         << ", status=" << static_cast<int>(value.status);
}

class TestZoomResume : public TestWithParam<ResumeCase>
{
};

TEST_P(TestZoomResume, invalidatesOnlyTranslatedResumableWorkWithFullSizeBox)
{
    const ResumeCase &value{GetParam()};
    ValueSaver saved_status{g_calc_status, value.status};
    ValueSaver saved_width{g_zoom_box_width, value.zoom_width};

    invalidate_translated_resume(value.translated);

    EXPECT_EQ(value.expected, g_calc_status);
}

INSTANTIATE_TEST_SUITE_P(BoundsAdjustment, TestZoomResume,
    Values(ResumeCase{true, 1.0, CalcStatus::RESUMABLE, CalcStatus::PARAMS_CHANGED},
        ResumeCase{false, 1.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 0.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 0.5, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 2.0, CalcStatus::RESUMABLE, CalcStatus::RESUMABLE},
        ResumeCase{true, 1.0, CalcStatus::NO_FRACTAL, CalcStatus::NO_FRACTAL},
        ResumeCase{true, 1.0, CalcStatus::PARAMS_CHANGED, CalcStatus::PARAMS_CHANGED},
        ResumeCase{true, 1.0, CalcStatus::IN_PROGRESS, CalcStatus::IN_PROGRESS},
        ResumeCase{true, 1.0, CalcStatus::NON_RESUMABLE, CalcStatus::NON_RESUMABLE},
        ResumeCase{true, 1.0, CalcStatus::COMPLETED, CalcStatus::COMPLETED}));

class TestBigFloatZoom : public TestWithParam<int>
{
protected:
    void SetUp() override
    {
        init_bf_dec(80);
        g_zoom_box_height = 0.5;
        g_zoom_box_width = 0.5;
        g_zoom_box_x = 0.25;
        g_zoom_box_y = 0.25;
        g_zoom_box_rotation = GetParam();
        g_zoom_box_skew = 0.125;
        g_viewport.final_aspect_ratio = 0.75f;
        const BigFloatPtr corners[]{g_bf_save_x_min.data(), g_bf_save_x_max.data(), g_bf_save_y_min.data(),
            g_bf_save_y_max.data(), g_bf_save_x_3rd.data(), g_bf_save_y_3rd.data()};
        const double values[]{-2.0, 2.0, -1.5, 1.5, -1.75, -1.25};
        for (std::size_t i = 0; i < std::size(corners); ++i)
        {
            float_to_bf(corners[i], values[i]);
        }
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    void expect_current_region(const ImageRegion &expected)
    {
        const BigFloatPtr corners[]{g_bf_x_min.data(), g_bf_x_max.data(), g_bf_y_min.data(), g_bf_y_max.data(),
            g_bf_x_3rd.data(), g_bf_y_3rd.data()};
        const double values[]{
            expected.m_min.x, expected.m_max.x, expected.m_min.y, expected.m_max.y, expected.m_3rd.x, expected.m_3rd.y};
        for (std::size_t i = 0; i < std::size(corners); ++i)
        {
            EXPECT_NEAR(values[i], static_cast<double>(bf_to_float(corners[i])), 1e-12);
        }
    }

    std::vector<Byte> saved_corners() const
    {
        std::vector<Byte> result;
        for (BigFloatConstPtr corner : {g_bf_save_x_min.data(), g_bf_save_x_max.data(), g_bf_save_y_min.data(),
                 g_bf_save_y_max.data(), g_bf_save_x_3rd.data(), g_bf_save_y_3rd.data()})
        {
            result.insert(result.end(), corner, corner + g_bf_length + 2);
        }
        return result;
    }

    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{641, 481, 0, 0, 640.0, 480.0}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<ImageRegion> m_saved_original{
        g_save_image_region, ImageRegion{{-2.0, -1.5}, {2.0, 1.5}, {-1.75, -1.25}}};
    ValueSaver<Viewport> m_saved_viewport{g_viewport};
    ValueSaver<int> m_saved_count{g_box_count, 0};
    ValueSaver<int> m_saved_rotation{g_zoom_box_rotation};
    ValueSaver<double> m_saved_height{g_zoom_box_height};
    ValueSaver<double> m_saved_width{g_zoom_box_width};
    ValueSaver<double> m_saved_x{g_zoom_box_x};
    ValueSaver<double> m_saved_y{g_zoom_box_y};
    ValueSaver<double> m_saved_skew{g_zoom_box_skew};
};

TEST_P(TestBigFloatZoom, preservesZoomCoordinatesSavedBytesAndStackLifetime)
{
    const auto original{saved_corners()};
    const auto stack{save_stack()};
    g_bf_math = BFMathType::BIG_FLT;

    draw_box(false);

    expect_current_region(g_image_region);
    EXPECT_EQ(original, saved_corners());
    EXPECT_EQ(stack, save_stack());
    const ImageRegion zoomed{g_image_region};
    g_bf_math = BFMathType::NONE;
    zoom_out();
    const ImageRegion expected{g_image_region};
    g_image_region = zoomed;
    g_bf_math = BFMathType::BIG_FLT;

    zoom_out();

    expect_current_region(expected);
    EXPECT_EQ(original, saved_corners());
    EXPECT_EQ(stack, save_stack());
    g_zoom_box_width = 0;
    draw_box(false);
    expect_current_region(g_save_image_region);
    EXPECT_EQ(original, saved_corners());
}

INSTANTIATE_TEST_SUITE_P(Rotations, TestBigFloatZoom, Values(0, 9, -7));

} // namespace id::test
