// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/pixel.h>
#include <engine/SolidGuess.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <misc/debug_flags.h>
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

class TestSolidGuess : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        s_calculated.clear();
        s_interrupt_after = -1;
        g_screen_x_dots = 16;
        g_screen_y_dots = 12;
        g_logical_screen = {16, 12, 0, 0, 15.0, 11.0};
        g_start_pt = {0, 0};
        g_stop_pt = {15, 11};
        g_i_start_pt = g_start_pt;
        g_i_stop_pt = g_stop_pt;
        g_begin_pt = g_start_pt;
        g_dispatch.set_calc_type(calculate_pixel);
        set_normal_dot();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * 16 + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int col, int row, int color)
                {
                    m_pixels.at(row * 16 + col) = color;
                    m_plots.push_back({col, row});
                }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    void provide_pixel(SolidGuess &pass, const int color = 7)
    {
        const Point2i point{pass.selected_pixel()};
        g_plot(point.x, point.y, color);
        pass.complete_pixel(color);
    }

    void complete_seed_row(SolidGuess &pass, const Point2i changed = {-1, -1})
    {
        for (int seed = 0; seed < 4; ++seed)
        {
            ASSERT_TRUE(pass.select_pixel());
            provide_pixel(pass, pass.selected_pixel() == changed ? 8 : 7);
        }
    }

    void expect_probe(SolidGuess &pass, const Point2i point, const int color)
    {
        ASSERT_TRUE(pass.select_pixel());
        EXPECT_EQ(point, pass.selected_pixel());
        provide_pixel(pass, color);
        g_color = 99;
        g_col = 99;
        g_row = 99;
    }

    std::vector<Point2i> selection_chunk(SolidGuess &pass)
    {
        std::vector<Point2i> selected;
        while (pass.select_pixel())
        {
            if (selected.size() >= 1000)
            {
                throw std::logic_error{"Solid-guess selection did not reach its row boundary"};
            }
            selected.push_back(pass.selected_pixel());
            provide_pixel(pass);
        }
        return selected;
    }

    void finish(SolidGuess &pass)
    {
        for (int chunks = 0; chunks < 100 && !pass.done(); ++chunks)
        {
            selection_chunk(pass);
        }
        EXPECT_TRUE(pass.done());
    }

    std::vector<Point2i> finish_selection(SolidGuess &pass)
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
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels = std::vector<int>(16 * 12, 0);
    std::vector<Point2i> m_plots;
};

TEST_F(TestSolidGuess, selectionRetainsTopRowSeedWithoutCalculation)
{
    SolidGuess pass;
    EXPECT_FALSE(pass.select_pixel()); // initialize the scan
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_TRUE(g_reset_periodicity);

    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_THAT(m_pixels, Each(0));
}

TEST_F(TestSolidGuess, completionAdvancesOnlyTheSeedCursor)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    ASSERT_TRUE(pass.select_pixel());

    provide_pixel(pass);

    EXPECT_FALSE(g_reset_periodicity);
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{4, 0}), pass.selected_pixel());
    EXPECT_FALSE(g_reset_periodicity);
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestSolidGuess, topRowSelectionKeepsBlockSpacingAndRowBoundary)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());

    EXPECT_THAT(selection_chunk(pass), ElementsAre(Point2i{0, 0}, Point2i{4, 0}, Point2i{8, 0}, Point2i{12, 0}));

    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 4}), pass.selected_pixel());
    EXPECT_TRUE(g_reset_periodicity);
}

TEST_F(TestSolidGuess, lowerRowSelectionRetainsPendingSeedBeforeGuessing)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{4, 4}), pass.selected_pixel());

    g_color = 99;
    g_row = 99;
    g_col = 99;
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{4, 4}), pass.selected_pixel());
    EXPECT_EQ(4, g_row);
    EXPECT_EQ(4, g_col);
    EXPECT_FALSE(g_reset_periodicity);
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestSolidGuess, lowerRowCompletionContinuesToTheNextSeedRow)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass);

    EXPECT_THAT(selection_chunk(pass),
        ElementsAre(Point2i{0, 4}, Point2i{4, 4}, Point2i{8, 4}, Point2i{12, 4}, Point2i{14, 2}, Point2i{14, 0}));
    EXPECT_TRUE(s_calculated.empty());

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 8}), pass.selected_pixel());
    finish(pass);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSolidGuess, firstPassDefersRefinementWhenOtherWorkIsQueued)
{
    add_work_list(100, 100, 110, 110, 100, 100, 0, 0);
    SolidGuess pass;

    finish(pass);

    ASSERT_EQ(2, g_num_work_list);
    EXPECT_EQ(0, g_work_pass);
    const WorkList *deferred =
        std::find_if(g_work_list, g_work_list + g_num_work_list, [](const WorkList &work) { return work.pass == 1; });
    ASSERT_NE(g_work_list + g_num_work_list, deferred);
    EXPECT_EQ(g_start_pt, deferred->start);
    EXPECT_EQ(g_stop_pt, deferred->stop);
    EXPECT_EQ(g_start_pt, deferred->begin);
}

TEST_F(TestSolidGuess, suspensionOfTopRowRestartsItsSeedRow)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());

    pass.suspend();
    pass.suspend();

    EXPECT_TRUE(pass.done());
    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(g_start_pt, g_work_list[0].begin);
    EXPECT_EQ(0, g_work_list[0].pass);
    EXPECT_TRUE(s_calculated.empty());
    g_begin_pt = g_work_list[0].begin;
    g_num_work_list = 0;
    SolidGuess resumed;
    ASSERT_FALSE(resumed.select_pixel());
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), resumed.selected_pixel());
    finish(resumed);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSolidGuess, suspensionOfLowerRowRetainsItsFirstPassResumePosition)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass); // top row
    selection_chunk(pass); // seed row below y=0, then guess that row
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{4, 8}), pass.selected_pixel());

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ((Point2i{0, 4}), g_work_list[0].begin);
    EXPECT_EQ(0, g_work_list[0].pass);
    g_begin_pt = g_work_list[0].begin;
    g_num_work_list = 0;
    SolidGuess resumed;
    ASSERT_FALSE(resumed.select_pixel());
    ASSERT_TRUE(resumed.select_pixel());
    EXPECT_EQ((Point2i{0, 8}), resumed.selected_pixel());
    finish(resumed);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSolidGuess, completionRequiresASelection)
{
    SolidGuess pass;
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_FALSE(pass.select_pixel());
    ASSERT_TRUE(pass.select_pixel());

    provide_pixel(pass);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
}

TEST_F(TestSolidGuess, blockProbeRetainsSelectionEvenAfterItsPixelWasPlotted)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass); // top row
    complete_seed_row(pass, {4, 4});
    ASSERT_TRUE(pass.select_pixel());
    const Point2i pending{2, 2};
    EXPECT_EQ(pending, pass.selected_pixel());
    g_plot(pending.x, pending.y, 9);
    const std::size_t plots{m_plots.size()};
    g_color = 99;
    g_col = 99;
    g_row = 99;

    ASSERT_TRUE(pass.select_pixel());
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ(pending, pass.selected_pixel());
    EXPECT_EQ(2, g_col);
    EXPECT_EQ(2, g_row);
    EXPECT_EQ(plots, m_plots.size());
    EXPECT_TRUE(s_calculated.empty());
    pass.complete_pixel(9);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 0}), pass.selected_pixel());
}

TEST_F(TestSolidGuess, completedProbeColorsDriveTheNextProbeAndBlockFill)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass);
    complete_seed_row(pass, {4, 4});

    expect_probe(pass, {2, 2}, 9);
    EXPECT_THROW(pass.complete_pixel(9), std::logic_error);
    expect_probe(pass, {2, 0}, 10);
    expect_probe(pass, {0, 2}, 11);
    ASSERT_TRUE(pass.select_pixel());      // finish this block before selecting the next

    EXPECT_EQ((Point2i{6, 2}), pass.selected_pixel());
    EXPECT_EQ(0, m_pixels.at(3 * 16 + 3)); // fills stay buffered until the row is complete
    selection_chunk(pass);
    EXPECT_EQ(9, m_pixels.at(3 * 16 + 3));
    EXPECT_EQ(10, m_pixels.at(1 * 16 + 3));
    EXPECT_EQ(11, m_pixels.at(3 * 16 + 1));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestSolidGuess, repairsRetainThePreviousBlocksTopAndCenterColors)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass);
    complete_seed_row(pass, {8, 4});

    expect_probe(pass, {6, 2}, 9);
    expect_probe(pass, {6, 0}, 7);
    expect_probe(pass, {4, 2}, 9);
    expect_probe(pass, {2, 0}, 11);
    const std::size_t plots{m_plots.size()};
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 2}), pass.selected_pixel());
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ(plots, m_plots.size());
    provide_pixel(pass, 12);

    EXPECT_EQ(11, m_pixels.at(1 * 16 + 3));
    EXPECT_EQ(12, m_pixels.at(3 * 16 + 3));
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_THROW(pass.complete_pixel(12), std::logic_error);
}

TEST_F(TestSolidGuess, repairsRetainTheUpperRowColors)
{
    SolidGuess pass;
    ASSERT_FALSE(pass.select_pixel());
    selection_chunk(pass);
    selection_chunk(pass); // row y=0 is filled before the next seed row
    complete_seed_row(pass, {4, 8});

    expect_probe(pass, {2, 6}, 9);
    expect_probe(pass, {2, 4}, 10);
    expect_probe(pass, {0, 6}, 9);
    expect_probe(pass, {2, 2}, 11);
    expect_probe(pass, {6, 6}, 7);
    expect_probe(pass, {4, 6}, 7);
    const std::size_t plots{m_plots.size()};
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{4, 2}), pass.selected_pixel());
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ(plots, m_plots.size());
    provide_pixel(pass, 12);

    EXPECT_EQ(11, m_pixels.at(3 * 16 + 3));
    EXPECT_EQ(12, m_pixels.at(3 * 16 + 5));
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestSolidGuess, fillsAndRefinementUseSelectionWithoutCalculating)
{
    SolidGuess pass;

    const std::vector<Point2i> selected{finish_selection(pass)};

    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_LT(selected.size(), m_pixels.size());
    EXPECT_GT(m_plots.size(), selected.size());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestSolidGuess, refinementSuspensionRetainsTheActiveRowAndPass)
{
    SolidGuess pass;
    bool selected{};
    for (int requests = 0; requests < 1000 && !pass.done(); ++requests)
    {
        if (pass.select_pixel())
        {
            if (g_work_pass > 0)
            {
                selected = true;
                break;
            }
            provide_pixel(pass);
        }
    }
    ASSERT_TRUE(selected);
    const int row{g_current_row};
    const int work_pass{g_work_pass};

    pass.suspend();
    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ((Point2i{0, row}), g_work_list[0].begin);
    EXPECT_EQ(work_pass, g_work_list[0].pass);
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_num_work_list = 0;
    SolidGuess resumed;
    finish_selection(resumed);
    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_TRUE(s_calculated.empty());
}

} // namespace id::test
