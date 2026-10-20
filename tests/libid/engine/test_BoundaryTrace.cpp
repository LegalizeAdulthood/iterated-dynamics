// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/BoundaryTrace.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/show_dot.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

std::vector<Point2i> s_calculated;
bool s_checkerboard{};

int pixel_color(const Point2i &point)
{
    return s_checkerboard ? 1 + (point.x + point.y) % 2 : 7;
}

int calculate_pixel()
{
    const Point2i point{g_col, g_row};
    s_calculated.push_back(point);
    g_color = pixel_color(point);
    g_plot(point.x, point.y, g_color);
    return g_color;
}

} // namespace

class TestBoundaryTrace : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        s_calculated.clear();
        s_checkerboard = false;
        set_frame(16, 12);
        g_dispatch.set_calc_type(calculate_pixel);
        set_normal_dot();
        set_normal_span();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * m_width + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int col, int row, int color)
                {
                    m_pixels.at(row * m_width + col) = color;
                    m_plots.push_back({col, row});
                }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    void set_frame(int width, int height)
    {
        m_width = width;
        m_height = height;
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
        g_start_pt = {0, 0};
        g_stop_pt = {width - 1, height - 1};
        g_i_start_pt = g_start_pt;
        g_i_stop_pt = g_stop_pt;
        g_begin_pt = g_start_pt;
        g_num_work_list = 0;
        g_color = 0;
        m_pixels.assign(width * height, 0);
        m_plots.clear();
    }

    void provide_pixel(BoundaryTrace &pass)
    {
        const Point2i point{pass.selected_pixel()};
        const int color{pixel_color(point)};
        g_plot(point.x, point.y, color);
        pass.complete_pixel(color);
    }

    std::vector<Point2i> selection_chunk(BoundaryTrace &pass)
    {
        std::vector<Point2i> selected;
        while (pass.select_pixel())
        {
            if (selected.size() >= 10000)
            {
                throw std::logic_error{"Boundary-trace selection did not reach its chunk boundary"};
            }
            selected.push_back(pass.selected_pixel());
            provide_pixel(pass);
        }
        return selected;
    }

    std::vector<Point2i> finish(BoundaryTrace &pass)
    {
        std::vector<Point2i> selected;
        for (int chunks = 0; chunks < 100 && !pass.done(); ++chunks)
        {
            const std::vector<Point2i> chunk{selection_chunk(pass)};
            selected.insert(selected.end(), chunk.begin(), chunk.end());
        }
        EXPECT_TRUE(pass.done());
        return selected;
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen};
    ValueSaver<int> m_saved_width{g_screen_x_dots};
    ValueSaver<int> m_saved_height{g_screen_y_dots};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill_color{g_fill_color, -1};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 0};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    std::vector<Point2i> m_plots;
    int m_width{};
    int m_height{};
};

TEST_F(TestBoundaryTrace, selectionRetainsScanPixelWithoutCalculation)
{
    BoundaryTrace pass;

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(m_plots.empty());
}

TEST_F(TestBoundaryTrace, completionKeepsTheScanAndOutlineContinuation)
{
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    g_color = 99;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{1, 0}), pass.selected_pixel());
    provide_pixel(pass);
    g_color = 99;

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 0}), pass.selected_pixel());
    provide_pixel(pass);
    g_row = 99;
    g_col = 99;
    g_color = 99;
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{3, 0}), pass.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestBoundaryTrace, outlineSelectionDoesNotSkipAnAlreadyPlottedPendingPixel)
{
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    const Point2i pending{pass.selected_pixel()};
    g_plot(pending.x, pending.y, 7);

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ(pending, pass.selected_pixel());
    pass.complete_pixel(7);
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{3, 0}), pass.selected_pixel());
}

TEST_F(TestBoundaryTrace, cachedPixelsDoNotBecomeCalculationRequests)
{
    std::fill(m_pixels.begin(), m_pixels.end(), 7);
    BoundaryTrace pass;

    EXPECT_FALSE(pass.select_pixel());

    EXPECT_TRUE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(m_plots.empty());
}

TEST_F(TestBoundaryTrace, fillingUsesCachedOutlineColorsWithoutRecalculatingPixels)
{
    BoundaryTrace pass;

    const std::vector<Point2i> selected{finish(pass)};

    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_LT(selected.size(), m_pixels.size());
    EXPECT_GT(m_plots.size(), selected.size());
    EXPECT_TRUE(s_calculated.empty());
    std::vector<int> visits(m_pixels.size(), 0);
    for (const Point2i &point : selected)
    {
        EXPECT_EQ(0, visits.at(point.y * m_width + point.x)++);
    }
}

TEST_F(TestBoundaryTrace, scanSelectionKeepsTheExistingChunkSize)
{
    set_frame(600, 1);
    s_checkerboard = true;
    BoundaryTrace pass;

    EXPECT_EQ(512U, selection_chunk(pass).size());
    EXPECT_FALSE(pass.done());
    EXPECT_EQ(88U, selection_chunk(pass).size());
    EXPECT_TRUE(pass.done());
}

TEST_F(TestBoundaryTrace, outlineSelectionKeepsTheExistingChunkSize)
{
    set_frame(600, 2);
    BoundaryTrace pass;

    EXPECT_EQ(258U, selection_chunk(pass).size());
    EXPECT_FALSE(pass.done());
    finish(pass);

    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestBoundaryTrace, suspensionRetainsAnUncomputedScanPixel)
{
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());

    pass.suspend();
    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ((Point2i{0, 0}), g_work_list[0].begin);
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(m_plots.empty());
}

TEST_F(TestBoundaryTrace, suspensionPreservesCompletedPixelsWithShowDotEnabled)
{
    g_show_dot = 15;
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    const std::vector<int> pixels{m_pixels};
    const std::vector<Point2i> plots{m_plots};

    pass.suspend();

    EXPECT_EQ(pixels, m_pixels);
    EXPECT_EQ(plots, m_plots);
    EXPECT_EQ(1, g_num_work_list);
}

TEST_F(TestBoundaryTrace, suspensionOfOutlineWorkRestartsAtItsScanCursor)
{
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ((Point2i{1, 0}), g_work_list[0].begin);
    EXPECT_EQ((Point2i{2, 0}), pass.selected_pixel());
    g_begin_pt = g_work_list[0].begin;
    g_num_work_list = 0;
    BoundaryTrace resumed;
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{2, 0}), resumed.selected_pixel());
    finish(resumed);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestBoundaryTrace, completionRequiresASelection)
{
    BoundaryTrace pass;
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());

    provide_pixel(pass);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{1, 0}), pass.selected_pixel());
}

} // namespace id::test
