// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/calculate_image.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/Cellular.h>
#include <fractals/fractalp.h>
#include <io/loadmap.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/CellularController.h>
#include <ui/id_keys.h>
#include <ui/text_screen.h>
#include <ui/thinking.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

int unexpected_renderer()
{
    throw std::logic_error("Cellular selection entered a renderer callback");
}

bool unexpected_setup()
{
    throw std::logic_error("Cellular selection entered legacy per-image setup");
}

} // namespace

class TestCellularController : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::CELLULAR)};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::CELLULAR};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{32, 6, 0, 0, 31.0, 5.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 32};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 6};
    ValueSaver<Point2i> m_saved_stop{g_i_stop_pt, Point2i{31, 5}};
    ValueSaver<int> m_saved_colors{g_colors, 2};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_seed_flag{g_random_seed_flag, true};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_next_screen{g_cellular_next_screen, false};
    ValueSaver<bool> m_saved_map{g_map_specified, false};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::vector<Byte>> m_saved_resume{g_resume_data, std::vector<Byte>{}};
    std::array<double, MAX_PARAMS> m_saved_params{};
    std::array<Byte, sizeof(g_dac_box)> m_saved_palette{};
    std::vector<int> m_rows;
    std::string m_error;
    int m_thinking_entries{};
    int m_thinking_exits{};
    int m_unstacks{};
    int m_spinner_updates{};
    bool m_thinking_visible{};
    bool m_interrupt{};
    bool m_fail_input{};
    bool m_fail_plot{};
};

void TestCellularController::SetUp()
{
    std::copy_n(g_params, MAX_PARAMS, m_saved_params.begin());
    std::memcpy(m_saved_palette.data(), g_dac_box, sizeof(g_dac_box));
    std::fill_n(g_params, MAX_PARAMS, 0.0);
    g_params[1] = 1010;
    g_params[2] = 21;
    m_specific.calc_type = unexpected_renderer;
    m_specific.per_image = unexpected_setup;
    g_dispatch = FractalDispatch{m_specific};
    set_normal_dot();
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](const int col, const int row, int)
            {
                EXPECT_FALSE(m_thinking_visible);
                if (m_fail_plot && m_thinking_entries > 0)
                {
                    throw std::runtime_error{"injected plot failure"};
                }
                if (col == 0)
                {
                    m_rows.push_back(row);
                }
            }));
    EXPECT_CALL(m_driver, stack_screen()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, unstack_screen())
        .WillRepeatedly(Invoke(
            [this]
            {
                ++m_unstacks;
                if (m_thinking_visible)
                {
                    ++m_thinking_exits;
                    m_thinking_visible = false;
                }
            }));
    EXPECT_CALL(m_driver, put_string(_, _, _, _))
        .WillRepeatedly(Invoke(
            [this](const int row, const int col, int, const char *text)
            {
                g_text_col = col + static_cast<int>(std::strlen(text));
                if (row == 4 && col == 10)
                {
                    ++m_thinking_entries;
                    m_thinking_visible = true;
                }
                else if (row == 4 && col == 0)
                {
                    EXPECT_FALSE(m_thinking_visible);
                    m_error = text;
                }
                else if (row == 4)
                {
                    ++m_spinner_updates;
                }
            }));
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [this]
            {
                if (m_fail_input && m_thinking_visible)
                {
                    throw std::runtime_error{"injected input failure"};
                }
                return m_interrupt && m_thinking_visible ? 'x' : 0;
            }));
    EXPECT_CALL(m_driver, get_key()).WillRepeatedly(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, move_cursor(_, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, buzzer(_)).Times(AnyNumber());
}

void TestCellularController::TearDown()
{
    EXPECT_FALSE(thinking_active());
    std::copy(m_saved_params.begin(), m_saved_params.end(), g_params);
    std::memcpy(g_dac_box, m_saved_palette.data(), sizeof(g_dac_box));
}

TEST_F(TestCellularController, generationZeroNeedsNoThinkingScreen)
{
    EXPECT_EQ(0, calculate_image());

    EXPECT_EQ(0, m_thinking_entries);
    EXPECT_EQ(0, m_thinking_exits);
    EXPECT_EQ(0, m_unstacks);
    EXPECT_THAT(m_rows, ElementsAre(0, 1, 2, 3, 4, 5));
}

TEST_F(TestCellularController, preparationEndsBeforePlottingAndCleansUpOnce)
{
    g_params[3] = 205;

    EXPECT_EQ(0, calculate_image());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(1, m_unstacks);
    EXPECT_GT(m_spinner_updates, 0);
    EXPECT_THAT(m_rows, ElementsAre(0, 0, 1, 2, 3, 4, 5));
}

TEST_F(TestCellularController, preparationInterruptionCleansUpBeforeReportingNonResumable)
{
    g_params[3] = 205;
    m_interrupt = true;

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("Interrupted, can't resume", m_error);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_THAT(m_rows, ElementsAre(0));
}

TEST_F(TestCellularController, preparationExceptionCleansUpBeforeReportingFailure)
{
    g_params[3] = 205;
    m_fail_input = true;

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("injected input failure", m_error);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, plottingExceptionDoesNotEndThinkingTwice)
{
    g_params[3] = 5;
    m_fail_plot = true;

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("injected plot failure", m_error);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, initializationFailureDoesNotOpenThinkingScreen)
{
    g_params[2] = 99;

    EXPECT_EQ(-1, calculate_image());

    EXPECT_EQ(0, m_thinking_entries);
    EXPECT_EQ(0, m_thinking_exits);
    EXPECT_EQ(1, m_unstacks);
    EXPECT_THAT(m_error, StartsWith("Type must be"));
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, calculationExposesPreparationWithoutPresentingOrPolling)
{
    g_params[3] = 10;
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
    EXPECT_CALL(m_driver, unstack_screen()).Times(0);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    int generations{};

    while (calculation.preparing() && generations < 10)
    {
        const int row{g_row};
        calculation.iterate();
        EXPECT_FALSE(calculation.done());
        EXPECT_EQ(row + 1, g_row);
        ++generations;
    }

    EXPECT_EQ(9, generations);
    EXPECT_FALSE(calculation.preparing());
    EXPECT_THAT(m_rows, ElementsAre(0));
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_THAT(m_rows, ElementsAre(0, 0));
}

TEST_F(TestCellularController, calculationCannotResumeUnplottedPreparation)
{
    g_params[3] = 10;
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
    EXPECT_CALL(m_driver, unstack_screen()).Times(0);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());

    EXPECT_THROW(calculation.suspend(), CellularError);

    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, newImageResetsNextScreenBeforeConstructingCellular)
{
    g_cellular_next_screen = true;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, _)).Times(0);
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());

    EXPECT_FALSE(g_cellular_next_screen);
    EXPECT_FALSE(calculation.preparing());
    EXPECT_THAT(m_rows, ElementsAre(0));
    EXPECT_EQ(1, g_row);
    for (int generation = 0; generation < 6 && !calculation.done(); ++generation)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_THAT(m_rows, ElementsAre(0, 1, 2, 3, 4, 5));
    calculation.iterate();
    EXPECT_THAT(m_rows, ElementsAre(0, 1, 2, 3, 4, 5));
    calculation.finish(false);
}

TEST_F(TestCellularController, selectedOperationPreservesSavedRowContinuation)
{
    alloc_resume(10, 1);
    put_resume(2);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, 2)).Times(32).WillRepeatedly(Return(1));
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());

    EXPECT_TRUE(g_resuming);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(m_rows.empty());
    EXPECT_EQ(3, g_row);
    calculation.iterate();
    EXPECT_THAT(m_rows, ElementsAre(3));
    calculation.suspend();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_EQ(1, start_resume());
    int row{};
    get_resume(row);
    EXPECT_EQ(4, row);
}

TEST_F(TestCellularController, nextScreenReadsLastRowAndAdvancesGeneration)
{
    g_calc_status = CalcStatus::RESUMABLE;
    g_cellular_next_screen = true;
    g_params[3] = 12;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, 5)).Times(32).WillRepeatedly(Return(1));
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());

    EXPECT_TRUE(g_cellular_next_screen);
    EXPECT_TRUE(g_resuming);
    EXPECT_FALSE(calculation.preparing());
    EXPECT_TRUE(m_rows.empty());
    EXPECT_EQ(18, g_params[3]);
    EXPECT_EQ(0, g_row);
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_THAT(m_rows, ElementsAre(0));
    calculation.suspend();
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, stepwiseControllerPreparesOneGenerationAndRendersOneRowPerCall)
{
    g_params[3] = 4;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [&calculation]
            {
                EXPECT_FALSE(calculation.done());
                return 0;
            }));
    CellularController controller{calculation};
    EXPECT_THROW(controller.iterate(), std::logic_error);
    EXPECT_THROW(controller.result(), std::logic_error);
    EXPECT_THROW(controller.suspend(), std::logic_error);

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, g_row);
    EXPECT_THAT(m_rows, ElementsAre(0));
    EXPECT_THROW(controller.start(), std::logic_error);
    for (int row = 2; row <= 4; ++row)
    {
        controller.iterate();
        EXPECT_EQ(row, g_row);
        EXPECT_TRUE(thinking_active());
        EXPECT_THAT(m_rows, ElementsAre(0));
    }
    EXPECT_FALSE(calculation.preparing());
    controller.iterate();
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, g_row);
    EXPECT_THAT(m_rows, ElementsAre(0, 0));
    for (int generation = 0; generation < 8 && !controller.done(); ++generation)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(1, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_THAT(m_rows, ElementsAre(0, 0, 1, 2, 3, 4, 5));
    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(1, m_unstacks);
    const std::vector<int> rows{m_rows};
    controller.iterate();
    EXPECT_EQ(rows, m_rows);
    EXPECT_THROW(controller.suspend(), std::logic_error);
    calculation.finish(false);
}

TEST_F(TestCellularController, stepwiseRenderingInterruptionLeavesTheCommandPending)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    EXPECT_CALL(m_driver, key_pressed())
        .WillOnce(Invoke(
            [this]
            {
                EXPECT_THAT(m_rows, ElementsAre(0, 1));
                return 'x';
            }));
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, unget_key(_)).Times(0);
    CellularController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_EQ(1, start_resume());
    int row{};
    get_resume(row);
    EXPECT_EQ(2, row);
    EXPECT_EQ(0, m_unstacks);
    EXPECT_TRUE(m_error.empty());
}

TEST_F(TestCellularController, stepwiseSuspensionRetainsTheExistingResumeRow)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    CellularController controller{calculation};
    controller.start();
    controller.iterate();

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_EQ(1, start_resume());
    int row{};
    get_resume(row);
    EXPECT_EQ(2, row);
    EXPECT_FALSE(thinking_active());
    EXPECT_TRUE(m_error.empty());
}

TEST_F(TestCellularController, stepwisePreparationSuspensionCleansUpBeforeReporting)
{
    g_params[3] = 10;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    CellularController controller{calculation};
    controller.start();
    controller.iterate();
    ASSERT_TRUE(thinking_active());

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("Interrupted, can't resume", m_error);
}

TEST_F(TestCellularController, stepwiseInputFailureClosesCalculationAndThinkingScreen)
{
    g_params[3] = 10;
    m_fail_input = true;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    CellularController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("injected input failure", m_error);
}

TEST_F(TestCellularController, stepwisePlotFailureDoesNotEndThinkingTwice)
{
    g_params[3] = 3;
    m_fail_plot = true;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    CellularController controller{calculation};
    controller.start();
    for (int generation = 0; generation < 4 && !controller.done(); ++generation)
    {
        controller.iterate();
    }

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("injected plot failure", m_error);
}

TEST_F(TestCellularController, stepwiseThinkingDisplayFailureRestoresTheScreenBeforeReporting)
{
    g_params[3] = 10;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    EXPECT_CALL(m_driver, put_string(4, 10, _, _)).WillOnce(Throw(std::runtime_error("thinking display failed")));
    CellularController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("thinking display failed", m_error);
}

TEST_F(TestCellularController, stepwiseAllocationFailureRetainsItsDiagnostic)
{
    g_params[3] = 10;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(std::bad_alloc{})).RetiresOnSaturation();
    CellularController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ("Insufficient free memory for calculation", m_error);
}

TEST_F(TestCellularController, stepwiseUnknownFailureRetainsItsDiagnostic)
{
    g_params[3] = 10;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(42)).RetiresOnSaturation();
    CellularController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ("Unknown error in cellular calculation", m_error);
}

TEST_F(TestCellularController, stepwiseDestructionClosesThinkingWithoutFinishingCalculation)
{
    g_params[3] = 10;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    {
        CellularController controller{calculation};
        controller.start();
        controller.iterate();
        ASSERT_TRUE(thinking_active());
    }

    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(1, m_unstacks);
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_error.empty());
    calculation.finish(true);
}

TEST_F(TestCellularController, stepwiseNextScreenUsesTheSelectedContinuation)
{
    g_calc_status = CalcStatus::RESUMABLE;
    g_cellular_next_screen = true;
    g_params[3] = 12;
    EXPECT_CALL(m_driver, read_pixel(_, 5)).Times(32).WillRepeatedly(Return(1));
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::CELLULAR, calculation.select());
    CellularController controller{calculation};
    controller.start();
    EXPECT_EQ(18, g_params[3]);
    EXPECT_EQ(0, g_row);

    controller.iterate();

    EXPECT_FALSE(controller.done());
    EXPECT_THAT(m_rows, ElementsAre(0));
    EXPECT_EQ(1, g_row);
    EXPECT_FALSE(thinking_active());
    EXPECT_EQ(0, m_unstacks);
    controller.suspend();
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_cellular_next_screen);
}

} // namespace id::test
