// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/StandardPass.h>

#include "MockDriver.h"

#include <engine/LogicalScreen.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
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
#include <ostream>
#include <stdexcept>
#include <string>

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

int unexpected_calculation()
{
    throw std::logic_error{"Pixel routing must not dispatch calculation"};
}

struct PixelPassCase
{
    const char *name;
    CalcMode mode;
    Point2i next;
};

void PrintTo(const PixelPassCase &value, std::ostream *out)
{
    *out << value.name;
}

} // namespace

class TestStandardPass : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        g_dispatch.set_calc_type(unexpected_calculation);
        set_normal_dot();
        set_normal_span();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * 8 + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int col, int row, int color)
                {
                    m_pixels.at(row * 8 + col) = color;
                    ++m_plot_count;
                }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    bool select_next(StandardPass &pass)
    {
        for (int chunks = 0; chunks < 16; ++chunks)
        {
            if (pass.select_pixel())
            {
                return true;
            }
        }
        return false;
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 8, 0, 0, 7, 7}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 8};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 8};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill_color{g_fill_color, -1};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 0};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 0};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::array<int, 64> m_pixels{};
    int m_plot_count{};
};

class TestStandardPassPixel : public TestStandardPass, public WithParamInterface<PixelPassCase>
{
};

TEST_P(TestStandardPassPixel, selectionIsRetainedWithoutCalculationOrPlotting)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    g_col = 99;
    g_row = 99;

    pass.select(GetParam().mode);
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ(GetParam().mode, pass.calc_mode());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassPixel, completionAdvancesTheActivePassOnce)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    g_color = 99;
    g_col = 99;
    g_row = 99;

    pass.complete_pixel(13);

    EXPECT_EQ(13, g_color);
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    if (GetParam().mode == CalcMode::TWO_PASS)
    {
        EXPECT_EQ(13, m_pixels[1]);
        EXPECT_EQ(13, m_pixels[8]);
        EXPECT_EQ(13, m_pixels[9]);
    }
    if (GetParam().mode == CalcMode::DIFFUSION)
    {
        EXPECT_EQ(13, m_pixels[7 * 8 + 7]);
    }
}

TEST_P(TestStandardPassPixel, completionRequiresSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);

    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

TEST_P(TestStandardPassPixel, resetDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));

    pass.reset();

    EXPECT_EQ(CalcMode::NONE, pass.calc_mode());
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

TEST_P(TestStandardPassPixel, changingPassDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    pass.complete_pixel(13);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    const CalcMode other{GetParam().mode == CalcMode::ONE_PASS ? CalcMode::TWO_PASS : CalcMode::ONE_PASS};

    pass.select(other);

    EXPECT_EQ(other, pass.calc_mode());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestStandardPassPixel,
    Values(PixelPassCase{"OnePass", CalcMode::ONE_PASS, {1, 0}}, PixelPassCase{"TwoPass", CalcMode::TWO_PASS, {2, 0}},
        PixelPassCase{"BoundaryTrace", CalcMode::BOUNDARY_TRACE, {1, 0}},
        PixelPassCase{"SolidGuess", CalcMode::SOLID_GUESS, {4, 0}},
        PixelPassCase{"Diffusion", CalcMode::DIFFUSION, {4, 4}}, PixelPassCase{"Tesseral", CalcMode::TESSERAL, {1, 0}}),
    [](const TestParamInfo<PixelPassCase> &info) -> std::string { return info.param.name; });

TEST_F(TestStandardPass, emptyPassHasNoPixelWork)
{
    StandardPass pass;

    EXPECT_EQ(CalcMode::NONE, pass.calc_mode());
    EXPECT_FALSE(pass.select_pixel());
    EXPECT_THROW(pass.selected_pixel(), std::logic_error);
    EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
    EXPECT_TRUE(pass.iterate());
}

TEST_F(TestStandardPass, orbitAlgorithmsDoNotOfferPixelRequests)
{
    StandardPass pass;
    for (const CalcMode mode : {CalcMode::SYNCHRONOUS_ORBIT, CalcMode::ORBIT})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        pass.select(mode);

        EXPECT_THROW(pass.select_pixel(), std::logic_error);
        EXPECT_THROW(pass.selected_pixel(), std::logic_error);
        EXPECT_THROW(pass.complete_pixel(7), std::logic_error);
        EXPECT_EQ(mode, pass.calc_mode());
    }
    EXPECT_EQ(0, m_plot_count);
}

} // namespace id::test
