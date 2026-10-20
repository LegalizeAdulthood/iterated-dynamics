// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/ScanlinePass.h>

#include "MockDriver.h"

#include <engine/LogicalScreen.h>
#include <engine/resume.h>
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
std::vector<std::array<int, 3>> s_plotted;
bool s_interrupt_once{};
int s_color{7};

int calculate_pixel()
{
    s_calculated.push_back({g_col, g_row});
    if (s_interrupt_once)
    {
        s_interrupt_once = false;
        return -1;
    }
    g_color = s_color;
    return g_color;
}

void plot_pixel(int col, int row, int color)
{
    s_plotted.push_back({col, row, color});
}

} // namespace

class TestScanlinePass : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        s_calculated.clear();
        s_plotted.clear();
        s_interrupt_once = false;
        s_color = 7;
        g_dispatch.set_calc_type(calculate_pixel);
        set_normal_dot();
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    std::vector<Point2i> finish(ScanlinePass &pass)
    {
        std::vector<Point2i> selected;
        while (pass.select_pixel())
        {
            const Point2i point{pass.selected_pixel()};
            selected.push_back(point);
            pass.complete_pixel(1 + point.x + 4 * point.y);
        }
        return selected;
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{4, 3, 0, 0, 3, 2}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 4};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 3};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{3, 2}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{3, 2}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_col{g_col, 0};
    ValueSaver<int> m_saved_row{g_row, 0};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_inside{g_inside_color, 0};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, plot_pixel};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
};

TEST_F(TestScanlinePass, selectionDoesNotCalculateOrAdvance)
{
    OnePass pass;

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(s_plotted.empty());
}

TEST_F(TestScanlinePass, onePassSelectionCrossesRowsWithoutCalculation)
{
    OnePass pass;

    EXPECT_THAT(finish(pass),
        ElementsAre(Point2i{0, 0}, Point2i{1, 0}, Point2i{2, 0}, Point2i{3, 0}, Point2i{0, 1}, Point2i{1, 1},
            Point2i{2, 1}, Point2i{3, 1}, Point2i{0, 2}, Point2i{1, 2}, Point2i{2, 2}, Point2i{3, 2}));
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(s_plotted.empty());
}

TEST_F(TestScanlinePass, twoPassSelectionRetainsSparseThenRemainingOrder)
{
    TwoPass pass;

    EXPECT_THAT(finish(pass),
        ElementsAre(Point2i{0, 0}, Point2i{2, 0}, Point2i{0, 2}, Point2i{2, 2}, Point2i{1, 0}, Point2i{3, 0},
            Point2i{0, 1}, Point2i{1, 1}, Point2i{2, 1}, Point2i{3, 1}, Point2i{1, 2}, Point2i{3, 2}));
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(2, g_total_passes);
    EXPECT_EQ(2, g_current_pass);
}

TEST_F(TestScanlinePass, sparsePixelCopiesOnlyAfterCompletion)
{
    TwoPass pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_TRUE(s_plotted.empty());
    g_col = 99;
    g_row = 99;

    pass.complete_pixel(7);

    EXPECT_THAT(
        s_plotted, ElementsAre(std::array<int, 3>{0, 1, 7}, std::array<int, 3>{1, 1, 7}, std::array<int, 3>{1, 0, 7}));
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{2, 0}), pass.selected_pixel());
}

TEST_F(TestScanlinePass, secondPassCanStartFromDeferredWork)
{
    g_work_pass = 1;
    TwoPass pass;

    EXPECT_THAT(finish(pass),
        ElementsAre(Point2i{1, 0}, Point2i{3, 0}, Point2i{0, 1}, Point2i{1, 1}, Point2i{2, 1}, Point2i{3, 1},
            Point2i{1, 2}, Point2i{3, 2}));
    EXPECT_TRUE(s_plotted.empty());
}

TEST_F(TestScanlinePass, firstPassDefersSecondPassWhenWorkRemains)
{
    g_num_work_list = 1;
    g_work_list[0] = {{20, 20}, {24, 24}, {20, 20}, 0, 0};
    TwoPass pass;

    EXPECT_THAT(finish(pass), ElementsAre(Point2i{0, 0}, Point2i{2, 0}, Point2i{0, 2}, Point2i{2, 2}));

    ASSERT_EQ(2, g_num_work_list);
    EXPECT_EQ(1, g_work_list[1].pass);
    EXPECT_EQ((Point2i{0, 0}), g_work_list[1].begin);
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_EQ(2, g_num_work_list);
}

TEST_F(TestScanlinePass, suspendRetainsSelectedPixel)
{
    OnePass pass;
    ASSERT_TRUE(pass.select_pixel());
    pass.complete_pixel(7);
    ASSERT_TRUE(pass.select_pixel());

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ((Point2i{1, 0}), g_work_list[0].begin);
    EXPECT_EQ((Point2i{1, 0}), pass.selected_pixel());
}

TEST_F(TestScanlinePass, resumeStartsAtSavedPixel)
{
    g_begin_pt = {2, 1};
    OnePass pass;

    EXPECT_THAT(finish(pass),
        ElementsAre(Point2i{2, 1}, Point2i{3, 1}, Point2i{0, 2}, Point2i{1, 2}, Point2i{2, 2}, Point2i{3, 2}));
}

TEST_F(TestScanlinePass, quickCalculationSkipsPreviouslyComputedPixels)
{
    g_quick_calc = true;
    EXPECT_CALL(m_driver, read_pixel(0, 0)).WillOnce(Return(5));
    EXPECT_CALL(m_driver, read_pixel(1, 0)).WillOnce(Return(0));
    OnePass pass;

    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ((Point2i{1, 0}), pass.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
}

TEST_F(TestScanlinePass, periodicityResetsAtRowBoundaries)
{
    OnePass pass;
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_TRUE(g_reset_periodicity);
    for (int col = 0; col < 4; ++col)
    {
        pass.complete_pixel(7);
        EXPECT_FALSE(g_reset_periodicity);
        ASSERT_TRUE(pass.select_pixel());
        EXPECT_EQ(col == 3, g_reset_periodicity);
    }
    EXPECT_EQ((Point2i{0, 1}), pass.selected_pixel());
}

TEST_F(TestScanlinePass, completionPreservesLegacySignedColors)
{
    TwoPass pass;

    while (pass.select_pixel())
    {
        pass.complete_pixel(-2);
    }

    EXPECT_EQ(-2, g_color);
    ASSERT_FALSE(s_plotted.empty());
    for (const std::array<int, 3> &point : s_plotted)
    {
        EXPECT_EQ(-2, point[2]);
    }
}

TEST_F(TestScanlinePass, completionRequiresASelection)
{
    OnePass pass;
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());

    pass.complete_pixel(7);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
}

} // namespace id::test
