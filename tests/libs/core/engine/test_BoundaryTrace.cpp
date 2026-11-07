// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/BoundaryTrace.h>

#include <MockDriver.h>
#include <PassTestFrame.h>
#include <WorkListSnapshot.h>

#include <engine/calcfrac.h>
#include <engine/pixel.h>
#include <engine/show_dot.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
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
        s_calculated.clear();
        s_checkerboard = false;
        set_frame(16, 12);
        g_dispatch.set_calc_type(calculate_pixel);
        set_pixel_functions(driver_write_pixel, driver_read_pixel);
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_frame.read_pixel(col, row); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int col, int row, int color) { m_frame.write_pixel(col, row, color); }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void set_frame(int width, int height)
    {
        m_frame.reset(width, height);
        g_num_work_list = 0;
        g_color = 0;
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
    PassTestFrame m_frame;
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill_color{g_fill_color, -1};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 0};
    WorkListSnapshot m_work_list;
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
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
    EXPECT_TRUE(m_frame.plots().empty());
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
    std::fill(m_frame.pixels().begin(), m_frame.pixels().end(), 7);
    BoundaryTrace pass;

    EXPECT_FALSE(pass.select_pixel());

    EXPECT_TRUE(pass.done());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_TRUE(m_frame.plots().empty());
}

TEST_F(TestBoundaryTrace, fillingUsesCachedOutlineColorsWithoutRecalculatingPixels)
{
    BoundaryTrace pass;

    const std::vector<Point2i> selected{finish(pass)};

    EXPECT_THAT(m_frame.pixels(), Each(7));
    EXPECT_LT(selected.size(), m_frame.pixels().size());
    EXPECT_GT(m_frame.plots().size(), selected.size());
    EXPECT_TRUE(s_calculated.empty());
    std::vector<int> visits(m_frame.pixels().size(), 0);
    for (const Point2i &point : selected)
    {
        EXPECT_EQ(0, visits.at(point.y * m_frame.width() + point.x)++);
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
    EXPECT_THAT(m_frame.pixels(), Each(7));
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
    EXPECT_TRUE(m_frame.plots().empty());
}

TEST_F(TestBoundaryTrace, suspensionPreservesCompletedPixelsWithShowDotEnabled)
{
    g_show_dot = 15;
    BoundaryTrace pass;
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    ASSERT_TRUE(pass.select_pixel());
    provide_pixel(pass);
    const std::vector<int> pixels{m_frame.pixels()};
    const std::vector<Point2i> plots{m_frame.plots()};

    pass.suspend();

    EXPECT_EQ(pixels, m_frame.pixels());
    EXPECT_EQ(plots, m_frame.plots());
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
    EXPECT_THAT(m_frame.pixels(), Each(7));
}

struct BoundaryColorInput
{
    const char *name;
    ColorMethod inside;
    ColorMethod outside;
};

void PrintTo(const BoundaryColorInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestBoundaryTraceInvalidColors : public TestBoundaryTrace, public WithParamInterface<BoundaryColorInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<int> m_inside_color{g_inside_color, 0};
    ValueSaver<int> m_outside_color{g_outside_color, 0};
};

void TestBoundaryTraceInvalidColors::SetUp()
{
    TestBoundaryTrace::SetUp();
    g_inside_method = GetParam().inside;
    g_outside_method = GetParam().outside;
}

TEST_P(TestBoundaryTraceInvalidColors, reportsFailureAndAllowsCorrectedSettings)
{
    BoundaryTrace pass;

    EXPECT_THROW(pass.select_pixel(), std::runtime_error);
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(m_frame.plots().empty());
    EXPECT_TRUE(s_calculated.empty());

    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::ITER;
    EXPECT_TRUE(pass.select_pixel());
}

INSTANTIATE_TEST_SUITE_P(ColorInputs, TestBoundaryTraceInvalidColors,
    Values(BoundaryColorInput{"InsideZero", ColorMethod::COLOR, ColorMethod::ITER},
        BoundaryColorInput{"OutsideZero", ColorMethod::ITER, ColorMethod::COLOR}),
    [](const TestParamInfo<BoundaryColorInput> &info) { return info.param.name; });

TEST_F(TestBoundaryTrace, insufficientColorsReportFailureWithoutCompletingOrPolling)
{
    g_colors = 2;
    BoundaryTrace pass;

    EXPECT_THROW(pass.select_pixel(), std::runtime_error);
    EXPECT_FALSE(pass.done());
    EXPECT_TRUE(m_frame.plots().empty());
    EXPECT_TRUE(s_calculated.empty());

    g_colors = 16;
    EXPECT_TRUE(pass.select_pixel());
}

struct BoundaryTraceSelectionInput
{
    const char *name;
    bool consumed;
    Point2i next;
};

void PrintTo(const BoundaryTraceSelectionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestBoundaryTraceSelectionGuard : public TestBoundaryTrace, public WithParamInterface<BoundaryTraceSelectionInput>
{
protected:
    void SetUp() override;

    std::optional<BoundaryTrace> m_pass;
};

void TestBoundaryTraceSelectionGuard::SetUp()
{
    TestBoundaryTrace::SetUp();
    m_pass.emplace();
    if (GetParam().consumed)
    {
        ASSERT_TRUE(m_pass->select_pixel());
        provide_pixel(*m_pass);
    }
}

TEST_P(TestBoundaryTraceSelectionGuard, accessorRejectionPreservesNextSelection)
{
    EXPECT_THROW(m_pass->selected_pixel(), std::logic_error);

    ASSERT_TRUE(m_pass->select_pixel());
    EXPECT_EQ(GetParam().next, m_pass->selected_pixel());
}

TEST_P(TestBoundaryTraceSelectionGuard, completionRejectionPreservesNextSelection)
{
    EXPECT_THROW(m_pass->complete_pixel(7), std::logic_error);

    ASSERT_TRUE(m_pass->select_pixel());
    EXPECT_EQ(GetParam().next, m_pass->selected_pixel());
}

INSTANTIATE_TEST_SUITE_P(SelectionStates, TestBoundaryTraceSelectionGuard,
    Values(BoundaryTraceSelectionInput{"NeverSelected", false, {0, 0}},
        BoundaryTraceSelectionInput{"Consumed", true, {1, 0}}),
    [](const TestParamInfo<BoundaryTraceSelectionInput> &info) { return info.param.name; });

} // namespace id::test
