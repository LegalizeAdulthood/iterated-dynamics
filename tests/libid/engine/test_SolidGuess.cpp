// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
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

int calculate_seed()
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
        g_dispatch.set_calc_type(calculate_seed);
        set_normal_dot();
        set_normal_span();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * 16 + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int col, int row, int color) { m_pixels.at(row * 16 + col) = color; }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    void provide_pixel(SolidGuess &pass)
    {
        const Point2i point{pass.selected_pixel()};
        g_plot(point.x, point.y, 7);
        pass.complete_pixel(7);
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
            pass.iterate();
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
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels = std::vector<int>(16 * 12, 0);
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

    EXPECT_THAT(selection_chunk(pass), ElementsAre(Point2i{0, 4}, Point2i{4, 4}, Point2i{8, 4}, Point2i{12, 4}));

    ASSERT_TRUE(pass.select_pixel());
    EXPECT_EQ((Point2i{0, 8}), pass.selected_pixel());
    finish(pass);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSolidGuess, iterateKeepsSeedCalculationsBatchedByRow)
{
    SolidGuess pass;
    EXPECT_FALSE(pass.iterate()); // initialize the scan
    EXPECT_TRUE(s_calculated.empty());

    EXPECT_FALSE(pass.iterate());

    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}, Point2i{4, 0}, Point2i{8, 0}, Point2i{12, 0}));
    s_calculated.clear();
    EXPECT_FALSE(pass.iterate());
    ASSERT_GE(s_calculated.size(), 4U);
    EXPECT_THAT(std::vector<Point2i>(s_calculated.begin(), s_calculated.begin() + 4),
        ElementsAre(Point2i{0, 4}, Point2i{4, 4}, Point2i{8, 4}, Point2i{12, 4}));
    EXPECT_EQ(0, g_current_row);
    s_calculated.clear();
    EXPECT_FALSE(pass.iterate());
    ASSERT_GE(s_calculated.size(), 4U);
    EXPECT_THAT(std::vector<Point2i>(s_calculated.begin(), s_calculated.begin() + 4),
        ElementsAre(Point2i{0, 8}, Point2i{4, 8}, Point2i{8, 8}, Point2i{12, 8}));
    EXPECT_EQ(4, g_current_row);
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

TEST_F(TestSolidGuess, interruptedSeedUsesTheExistingSuspendPath)
{
    s_interrupt_after = 2;
    SolidGuess pass;
    ASSERT_FALSE(pass.iterate());

    EXPECT_TRUE(pass.iterate());

    EXPECT_TRUE(pass.done());
    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}, Point2i{4, 0}));
    EXPECT_EQ((Point2i{8, 0}), pass.selected_pixel());
    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(g_start_pt, g_work_list[0].begin);
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

} // namespace id::test
