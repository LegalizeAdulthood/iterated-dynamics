// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/tesseral.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
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
int s_interrupt_after{-1};
bool s_gradient{};

int pixel_color(const Point2i &point)
{
    return s_gradient ? 1 + (3 * point.x + 5 * point.y) % 251 : 7;
}

void plot_pixel(const int col, const int row, const int color)
{
    put_color_a(col, row, color);
}

int calculate_pixel()
{
    if (static_cast<int>(s_calculated.size()) == s_interrupt_after)
    {
        return -1;
    }
    s_calculated.push_back({g_col, g_row});
    g_color = pixel_color({g_col, g_row});
    g_plot(g_col, g_row, g_color);
    return g_color;
}

} // namespace

class TestTesseral : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        s_interrupt_after = -1;
        s_gradient = false;
        set_frame(6, 5);
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

    void set_frame(const int width, const int height)
    {
        m_width = width;
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
        g_start_pt = {0, 0};
        g_stop_pt = {width - 1, height - 1};
        g_i_start_pt = g_start_pt;
        g_i_stop_pt = g_stop_pt;
        g_begin_pt = g_start_pt;
        g_work_pass = 0;
        g_num_work_list = 0;
        m_pixels.assign(width * height, 0);
        m_plots.clear();
        s_calculated.clear();
    }

    void provide_pixel(Tesseral &pass)
    {
        const Point2i point{pass.selected_pixel()};
        const int color{pixel_color(point)};
        g_plot(point.x, point.y, color);
        pass.complete_pixel(color);
        g_color = 99;
        g_col = 99;
        g_row = 99;
    }

    std::vector<Point2i> selection_chunk(Tesseral &pass)
    {
        std::vector<Point2i> selected;
        while (pass.select_pixel())
        {
            if (selected.size() >= 100000)
            {
                throw std::logic_error{"Tesseral selection did not reach its work-chunk boundary"};
            }
            selected.push_back(pass.selected_pixel());
            provide_pixel(pass);
        }
        return selected;
    }

    void finish_selection(Tesseral &pass)
    {
        for (int chunks = 0; chunks < 1000 && !pass.done(); ++chunks)
        {
            selection_chunk(pass);
        }
        EXPECT_TRUE(pass.done());
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
    ValueSaver<int> m_saved_fill{g_fill_color, -1};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    std::vector<Point2i> m_plots;
    int m_width{};
};

TEST_F(TestTesseral, rowSelectionRetainsThePendingPixelWithoutCalculation)
{
    Tesseral pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_TRUE(g_reset_periodicity);
    g_col = 99;
    g_row = 99;

    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_TRUE(m_plots.empty());
    EXPECT_TRUE(s_calculated.empty());
    provide_pixel(pass);
    EXPECT_FALSE(g_reset_periodicity);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{1, 0}), pass.selected_pixel());
}

TEST_F(TestTesseral, initialEdgesKeepRowAndColumnProbeOrder)
{
    Tesseral pass;

    EXPECT_THAT(selection_chunk(pass),
        ElementsAre(Point2i{0, 0}, Point2i{1, 0}, Point2i{2, 0}, Point2i{3, 0}, Point2i{4, 0}, Point2i{5, 0},
            Point2i{0, 4}, Point2i{1, 4}, Point2i{2, 4}, Point2i{3, 4}, Point2i{4, 4}, Point2i{5, 4}, Point2i{0, 1},
            Point2i{0, 2}, Point2i{0, 3}, Point2i{5, 1}, Point2i{5, 2}, Point2i{5, 3}));

    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, constantEdgesFillTheInteriorUsingCompletedColors)
{
    Tesseral pass;
    EXPECT_EQ(18U, selection_chunk(pass).size());
    EXPECT_EQ(0, m_pixels.at(2 * m_width + 1));

    EXPECT_THAT(selection_chunk(pass), ElementsAre(Point2i{2, 1}, Point2i{2, 2}, Point2i{2, 3}));

    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, splitProbeRetainsItsColumnAndBoxContinuation)
{
    s_gradient = true;
    Tesseral pass;
    EXPECT_EQ(18U, selection_chunk(pass).size());
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 1}), pass.selected_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    const Point2i pending{2, 2};
    EXPECT_EQ(pending, pass.selected_pixel());
    g_plot(pending.x, pending.y, pixel_color(pending));
    const std::size_t plots{m_plots.size()};
    g_col = 99;
    g_row = 99;

    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ(pending, pass.selected_pixel());
    EXPECT_EQ(plots, m_plots.size());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 3}), pass.selected_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{1, 2}), pass.selected_pixel());
    finish_selection(pass);
    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 6; ++col)
        {
            EXPECT_EQ(pixel_color({col, row}), m_pixels.at(row * m_width + col));
        }
    }
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, nonuniformMiddleRequiresSubdivision)
{
    Tesseral pass;
    selection_chunk(pass);
    for (int row = 1; row <= 3; ++row)
    {
        ASSERT_TRUE(pass.select_pixel());
        EXPECT_EQ((Point2i{2, row}), pass.selected_pixel());
        g_plot(2, row, row == 1 ? 8 : 7);
        pass.complete_pixel(row == 1 ? 8 : 7);
        g_color = 99;
    }

    ASSERT_TRUE(pass.select_pixel());

    // The split starts its own middle-edge probe after the color check.
    EXPECT_EQ((Point2i{2, 1}), pass.selected_pixel());
    EXPECT_EQ(0, m_pixels.at(2 * m_width + 1));
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, rowProbesKeepTheirExistingChunkSize)
{
    set_frame(600, 4);
    Tesseral pass;

    EXPECT_EQ(512U, selection_chunk(pass).size());
    EXPECT_EQ(600U, selection_chunk(pass).size());
    EXPECT_EQ(92U, selection_chunk(pass).size());

    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, columnProbesKeepTheirExistingChunkSize)
{
    set_frame(4, 600);
    Tesseral pass;

    EXPECT_EQ(520U, selection_chunk(pass).size());
    EXPECT_EQ(598U, selection_chunk(pass).size());
    EXPECT_EQ(86U, selection_chunk(pass).size());

    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, iterateKeepsBatchedEdgesAndItsSuccessContract)
{
    set_frame(600, 4);
    Tesseral pass;

    EXPECT_TRUE(pass.iterate());
    EXPECT_EQ(512U, s_calculated.size());
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(pass.iterate());
    EXPECT_EQ(1112U, s_calculated.size());
    EXPECT_TRUE(pass.iterate());
    EXPECT_EQ(1204U, s_calculated.size());
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(pass.iterate());
    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestTesseral, boxTraversalKeepsItsExistingChunkSize)
{
    set_frame(128, 128);
    s_gradient = true;
    Tesseral pass;
    ASSERT_EQ(508U, selection_chunk(pass).size());

    EXPECT_EQ(1286U, selection_chunk(pass).size());

    // The first 512 subdivisions end at this box.
    EXPECT_EQ(35, g_current_column);
    EXPECT_EQ(5, g_current_row);
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, spanFillsKeepTheirExistingChunkSize)
{
    set_frame(10, 140);
    Tesseral pass;
    ASSERT_EQ(296U, selection_chunk(pass).size());

    EXPECT_EQ(8U, selection_chunk(pass).size()); // horizontal middle probe, then 64 filled rows

    EXPECT_FALSE(pass.done());
    EXPECT_EQ(7, m_pixels.at(64 * m_width + 1));
    EXPECT_EQ(0, m_pixels.at(65 * m_width + 1));
    EXPECT_TRUE(selection_chunk(pass).empty());
    EXPECT_FALSE(pass.done());
    EXPECT_EQ(7, m_pixels.at(128 * m_width + 1));
    EXPECT_EQ(0, m_pixels.at(129 * m_width + 1));
    EXPECT_TRUE(selection_chunk(pass).empty());
    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, pixelFillsKeepTheirExistingChunkSize)
{
    set_frame(100, 100);
    g_plot = plot_pixel;
    Tesseral pass;
    ASSERT_EQ(396U, selection_chunk(pass).size());
    const std::size_t plots{m_plots.size()};

    EXPECT_EQ(98U, selection_chunk(pass).size());

    EXPECT_FALSE(pass.done());
    EXPECT_EQ(plots + 98 + 8192, m_plots.size());
    EXPECT_TRUE(selection_chunk(pass).empty());
    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, suspensionDuringInitialEdgesRestartsThePerimeter)
{
    Tesseral pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(g_start_pt, g_work_list[0].begin);
    EXPECT_EQ(0, g_work_list[0].pass);
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_num_work_list = 0;
    Tesseral resumed;
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), resumed.selected_pixel());
    finish_selection(resumed);
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, suspensionDuringSplitPreservesTheActiveBox)
{
    s_gradient = true;
    Tesseral pass;
    selection_chunk(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 2}), pass.selected_pixel());

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(8192, g_work_list[0].begin.y);
    EXPECT_EQ(8192, g_work_list[0].pass);
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_num_work_list = 0;
    Tesseral resumed;
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{2, 1}), resumed.selected_pixel());
    finish_selection(resumed);
    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 6; ++col)
        {
            EXPECT_EQ(pixel_color({col, row}), m_pixels.at(row * m_width + col));
        }
    }
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, interruptedKernelKeepsTheExistingFailureContract)
{
    s_interrupt_after = 3;
    Tesseral pass;

    EXPECT_FALSE(pass.iterate());

    EXPECT_FALSE(pass.done());
    EXPECT_EQ((Point2i{3, 0}), pass.selected_pixel());
    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}, Point2i{1, 0}, Point2i{2, 0}));
    EXPECT_FALSE(pass.select_pixel());
    pass.suspend();
    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(0, g_work_list[0].pass);
}

TEST_F(TestTesseral, narrowEdgesKeepTheirExistingSingleSample)
{
    set_frame(2, 2);
    Tesseral pass;

    EXPECT_THAT(selection_chunk(pass),
        ElementsAre(Point2i{0, 0}, Point2i{1, 0}, Point2i{0, 1}, Point2i{1, 1}, Point2i{0, 1}, Point2i{1, 1}));
    EXPECT_THAT(selection_chunk(pass), ElementsAre(Point2i{1, 0}));

    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestTesseral, completionRequiresASelection)
{
    Tesseral pass;
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());

    provide_pixel(pass);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
}

} // namespace id::test
