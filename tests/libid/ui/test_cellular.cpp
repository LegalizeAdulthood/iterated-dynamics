// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/cellular.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/Cellular.h>
#include <io/loadmap.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/text_screen.h>
#include <ui/thinking.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstring>
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

class TestCellularController : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
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
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::IN_PROGRESS};
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
    set_normal_dot();
    set_normal_span();
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
    EXPECT_EQ(1, cellular_type());

    EXPECT_EQ(0, m_thinking_entries);
    EXPECT_EQ(0, m_thinking_exits);
    EXPECT_EQ(0, m_unstacks);
    EXPECT_THAT(m_rows, ElementsAre(0, 1, 2, 3, 4, 5));
}

TEST_F(TestCellularController, preparationEndsBeforePlottingAndCleansUpOnce)
{
    g_params[3] = 205;

    EXPECT_EQ(1, cellular_type());

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

    EXPECT_EQ(-1, cellular_type());

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

    EXPECT_EQ(-1, cellular_type());

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

    EXPECT_EQ(-1, cellular_type());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(2, m_unstacks);
    EXPECT_EQ("injected plot failure", m_error);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestCellularController, initializationFailureDoesNotOpenThinkingScreen)
{
    g_params[2] = 99;

    EXPECT_EQ(-1, cellular_type());

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
    Cellular cellular;
    int generations{};

    while (cellular.preparing() && generations < 10)
    {
        const int row{g_row};
        EXPECT_TRUE(cellular.iterate());
        EXPECT_EQ(row + 1, g_row);
        ++generations;
    }

    EXPECT_EQ(9, generations);
    EXPECT_FALSE(cellular.preparing());
    EXPECT_THAT(m_rows, ElementsAre(0));
    EXPECT_TRUE(cellular.iterate());
    EXPECT_THAT(m_rows, ElementsAre(0, 0));
}

TEST_F(TestCellularController, calculationCannotResumeUnplottedPreparation)
{
    g_params[3] = 10;
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
    EXPECT_CALL(m_driver, unstack_screen()).Times(0);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    Cellular cellular;

    EXPECT_THROW(cellular.suspend(), CellularError);

    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

} // namespace id::test
