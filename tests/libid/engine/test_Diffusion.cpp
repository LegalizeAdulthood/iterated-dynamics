// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/Diffusion.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
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

int calculate_pixel()
{
    if (static_cast<int>(s_calculated.size()) == s_interrupt_after)
    {
        return -1;
    }
    s_calculated.push_back({g_col, g_row});
    g_color = 7;
    g_plot(g_col, g_row, g_color);
    return g_color;
}

} // namespace

class TestDiffusionPass : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        s_interrupt_after = -1;
        set_frame(4, 4);
        g_dispatch.set_calc_type(calculate_pixel);
        set_normal_dot();
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
        g_num_work_list = 0;
        g_work_pass = 0;
        m_pixels.assign(width * height, 0);
        m_plots.clear();
        s_calculated.clear();
    }

    std::vector<Point2i> selection_chunk(Diffusion &pass)
    {
        std::vector<Point2i> selected;
        while (pass.select_pixel())
        {
            if (selected.size() >= 10000)
            {
                throw std::logic_error{"Diffusion selection did not reach its counter-chunk boundary"};
            }
            selected.push_back(pass.selected_pixel());
            pass.complete_pixel(7);
        }
        return selected;
    }

    std::vector<Point2i> finish_selection(Diffusion &pass)
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
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill{g_fill_color, 0};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_key_interval{g_keyboard_check_interval};
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

TEST_F(TestDiffusionPass, selectionRetainsItsCounterAndTileUntilCompletion)
{
    set_frame(11, 6);
    Diffusion pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0UL, pass.counter());
    EXPECT_EQ(16UL, pass.limit());
    EXPECT_EQ(4U, pass.bits());
    EXPECT_TRUE(g_reset_periodicity);
    g_col = 99;
    g_row = 99;

    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(0UL, pass.counter());
    EXPECT_TRUE(m_plots.empty());
    EXPECT_TRUE(s_calculated.empty());
    pass.complete_pixel(7);
    EXPECT_EQ(0UL, pass.counter());
    EXPECT_FALSE(g_reset_periodicity);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 4}), pass.selected_pixel());
}

TEST_F(TestDiffusionPass, squareSelectionKeepsDiffusionCounterOrder)
{
    Diffusion pass;

    EXPECT_THAT(selection_chunk(pass),
        ElementsAre(Point2i{0, 0}, Point2i{2, 2}, Point2i{0, 2}, Point2i{2, 0}, Point2i{1, 1}, Point2i{3, 3},
            Point2i{1, 3}, Point2i{3, 1}, Point2i{0, 1}, Point2i{2, 3}, Point2i{0, 3}, Point2i{2, 1}, Point2i{1, 2},
            Point2i{3, 0}, Point2i{1, 0}, Point2i{3, 2}));

    EXPECT_EQ(16UL, pass.counter());
    EXPECT_TRUE(pass.done());
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, tileSelectionKeepsColumnMajorOrderAndPartialTiles)
{
    set_frame(11, 6);
    Diffusion pass;
    std::vector<Point2i> selected;
    for (int tile = 0; tile < 6; ++tile)
    {
        ASSERT_TRUE(pass.select_pixel());
        EXPECT_EQ(0UL, pass.counter());
        selected.push_back(pass.selected_pixel());
        pass.complete_pixel(tile + 1);
    }

    EXPECT_THAT(selected,
        ElementsAre(Point2i{0, 0}, Point2i{0, 4}, Point2i{4, 0}, Point2i{4, 4}, Point2i{8, 0}, Point2i{8, 4}));
    EXPECT_EQ(1UL, pass.counter());
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 2}), pass.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, partialTilesVisitEveryPixelExactlyOnce)
{
    set_frame(11, 6);
    Diffusion pass;

    const std::vector<Point2i> selected{finish_selection(pass)};

    ASSERT_EQ(m_pixels.size(), selected.size());
    std::vector<int> visits(m_pixels.size(), 0);
    for (const Point2i &point : selected)
    {
        EXPECT_EQ(0, visits.at(point.y * m_width + point.x)++);
    }
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, inferredBlocksClipBothPartialTileEdges)
{
    set_frame(11, 6);
    g_fill_color = 1;
    Diffusion pass;
    for (int tile = 0; tile < 6; ++tile)
    {
        ASSERT_TRUE(pass.select_pixel());
        EXPECT_EQ(0UL, pass.counter());
        pass.complete_pixel(tile + 1);
    }

    EXPECT_EQ(1UL, pass.counter());
    EXPECT_EQ(m_pixels.size(), m_plots.size());
    for (int row = 0; row < 6; ++row)
    {
        for (int col = 0; col < 11; ++col)
        {
            EXPECT_EQ((col / 4) * 2 + row / 4 + 1, m_pixels.at(row * m_width + col));
        }
    }
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, blockCompletionUsesTheSuppliedColorOnlyOnce)
{
    g_fill_color = 1;
    Diffusion pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_TRUE(m_plots.empty());
    g_color = 99;
    g_col = 99;
    g_row = 99;

    pass.complete_pixel(13);

    EXPECT_THAT(m_pixels, Each(13));
    EXPECT_EQ(16U, m_plots.size());
    EXPECT_THROW(pass.complete_pixel(13), std::logic_error);
    EXPECT_EQ(16U, m_plots.size());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, secondHalfPlotsPointsInsteadOfBlocks)
{
    g_fill_color = 1;
    Diffusion pass;
    for (int counter = 0; counter < 8; ++counter)
    {
        ASSERT_TRUE(pass.select_pixel());
        pass.complete_pixel(7);
    }
    ASSERT_EQ(8UL, pass.counter());
    const std::vector<int> before{m_pixels};
    const std::size_t plots{m_plots.size()};
    ASSERT_TRUE(pass.select_pixel());
    const Point2i point{pass.selected_pixel()};

    pass.complete_pixel(42);

    EXPECT_EQ(plots + 1, m_plots.size());
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_EQ((Point2i{col, row}) == point ? 42 : before.at(row * 4 + col), m_pixels.at(row * 4 + col));
        }
    }
}

TEST_F(TestDiffusionPass, selectionChunkCountsCountersInsteadOfTiles)
{
    set_frame(64, 32);
    Diffusion pass;

    EXPECT_EQ(1024U, selection_chunk(pass).size());

    EXPECT_EQ(512UL, pass.counter());
    EXPECT_FALSE(pass.done());
    EXPECT_EQ(1024U, selection_chunk(pass).size());
    EXPECT_TRUE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, suspensionKeepsTheExistingCounterResumeFormat)
{
    set_frame(11, 6);
    Diffusion pass;
    ASSERT_TRUE(pass.select_pixel());
    pass.complete_pixel(7);
    ASSERT_TRUE(pass.select_pixel());
    const Point2i pending{pass.selected_pixel()};

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(g_start_pt, g_work_list[0].start);
    EXPECT_EQ(g_stop_pt, g_work_list[0].stop);
    EXPECT_EQ((Point2i{0, 0}), g_work_list[0].begin);
    EXPECT_EQ(0, g_work_list[0].pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ(pending, pass.selected_pixel());
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_num_work_list = 0;
    Diffusion resumed;
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), resumed.selected_pixel()); // saved work restarts the counter's tile group
    finish_selection(resumed);
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, suspensionPreservesBothCounterWords)
{
    set_frame(512, 512);
    g_begin_pt.y = 1;
    g_work_pass = 7;
    Diffusion pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ(65543UL, pass.counter());
    const Point2i pending{pass.selected_pixel()};

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(1, g_work_list[0].begin.y);
    EXPECT_EQ(7, g_work_list[0].pass);
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_num_work_list = 0;
    Diffusion resumed;
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ(65543UL, resumed.counter());
    EXPECT_EQ(pending, resumed.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestDiffusionPass, completionRequiresASelection)
{
    Diffusion pass;
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());

    pass.complete_pixel(7);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
}

} // namespace id::test
