// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/lsystem.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <fractals/lsystem.h>
#include <math/fixed_pt.h>
#include <misc/stack_avail.h>
#include <misc/ValueSaver.h>
#include <ui/text_screen.h>
#include <ui/thinking.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
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

class TestLSystemController : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    int render();

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{110, 106, 0, 0, 109.0, 105.0}};
    ValueSaver<float> m_saved_aspect{g_screen_aspect, 106.0F / 110.0F};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<bool> m_saved_overflow{g_overflow, false};
    ValueSaver<double> m_saved_order{g_params[0], 0.0};
    ValueSaver<char> m_saved_angle{g_max_angle};
    ValueSaver<std::filesystem::path> m_saved_filename{
        g_l_system_filename, std::filesystem::path{data::ID_TEST_LSYSTEM_DIR} / data::ID_TEST_LSYSTEM_FILE};
    ValueSaver<std::string> m_saved_name{g_l_system_name, "Koch1"};
    ValueSaver<int> m_saved_text_col{g_text_col};
    int m_thinking_entries{};
    int m_thinking_exits{};
    int m_spinner_updates{};
    int m_lines{};
    bool m_visible{};
    bool m_cancel_sizing{};
    bool m_cancel_drawing{};
    bool m_fail_input{};
    bool m_fail_display{};
    bool m_fail_plot{};
    bool m_overflow_on_spinner{};
};

void TestLSystemController::SetUp()
{
    EXPECT_CALL(m_driver, stack_screen()).WillRepeatedly(Invoke([this] { ++m_thinking_entries; }));
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, unstack_screen())
        .WillRepeatedly(Invoke(
            [this]
            {
                EXPECT_TRUE(m_visible);
                m_visible = false;
                ++m_thinking_exits;
            }));
    EXPECT_CALL(m_driver, put_string(_, _, _, _))
        .WillRepeatedly(Invoke(
            [this](int row, int col, int, const char *text)
            {
                g_text_col = col + static_cast<int>(std::strlen(text));
                if (row == 4 && col == 10)
                {
                    m_visible = true;
                    if (m_fail_display)
                    {
                        throw std::runtime_error{"injected thinking display failure"};
                    }
                }
                else if (row == 4)
                {
                    ++m_spinner_updates;
                    if (m_overflow_on_spinner)
                    {
                        g_overflow = true;
                    }
                }
            }));
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [this]
            {
                if (m_visible && m_fail_input)
                {
                    throw std::runtime_error{"injected input failure"};
                }
                return (m_visible && m_cancel_sizing) || (!m_visible && m_cancel_drawing && m_lines > 0) ? 'x' : 0;
            }));
    EXPECT_CALL(m_driver, draw_line(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [this](int, int, int, int, int)
            {
                EXPECT_FALSE(m_visible);
                EXPECT_FALSE(thinking_active());
                if (m_fail_plot)
                {
                    throw std::runtime_error{"injected plot failure"};
                }
                ++m_lines;
            }));
}

void TestLSystemController::TearDown()
{
    EXPECT_FALSE(thinking_active());
    EXPECT_FALSE(m_visible);
}

int TestLSystemController::render()
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    return lsystem_type();
}

TEST_F(TestLSystemController, closesThinkingBeforeFirstLine)
{
    EXPECT_EQ(0, render());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(0, m_spinner_updates);
    EXPECT_EQ(3, m_lines);
}

TEST_F(TestLSystemController, updatesThinkingDuringSizing)
{
    g_params[0] = 6;

    EXPECT_EQ(0, render());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_GT(m_spinner_updates, 0);
    EXPECT_EQ(12288, m_lines);
}

TEST_F(TestLSystemController, sizingInterruptionClosesThinkingWithoutConsumingKey)
{
    g_params[0] = 9;
    m_cancel_sizing = true;

    EXPECT_EQ(0, render());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_GT(m_spinner_updates, 0);
    EXPECT_EQ(0, m_lines);
    EXPECT_FALSE(lsystem_loaded());
}

TEST_F(TestLSystemController, drawingInterruptionDoesNotCloseThinkingTwice)
{
    g_params[0] = 3;
    m_cancel_drawing = true;

    EXPECT_EQ(0, render());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_GT(m_lines, 0);
    EXPECT_LT(m_lines, 192);
}

TEST_F(TestLSystemController, inputExceptionClosesThinking)
{
    g_params[0] = 9;
    m_fail_input = true;

    EXPECT_THROW(render(), std::runtime_error);

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(0, m_lines);
    EXPECT_FALSE(lsystem_loaded());
}

TEST_F(TestLSystemController, displayExceptionClosesThinking)
{
    m_fail_display = true;

    EXPECT_THROW(render(), std::runtime_error);

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(0, m_lines);
}

TEST_F(TestLSystemController, plotExceptionDoesNotCloseThinkingTwice)
{
    m_fail_plot = true;

    EXPECT_THROW(render(), std::runtime_error);

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
}

TEST_F(TestLSystemController, sizingFailureClosesThinkingWithoutDrawing)
{
    g_params[0] = 9;
    m_overflow_on_spinner = true;

    EXPECT_EQ(0, render());

    EXPECT_EQ(1, m_thinking_entries);
    EXPECT_EQ(1, m_thinking_exits);
    EXPECT_EQ(1, m_spinner_updates);
    EXPECT_EQ(0, m_lines);
}

TEST_F(TestLSystemController, initializationFailureDoesNotOpenThinking)
{
    g_overflow = true;

    EXPECT_EQ(0, render());

    EXPECT_EQ(0, m_thinking_entries);
    EXPECT_EQ(0, m_thinking_exits);
    EXPECT_EQ(0, m_lines);
}

TEST_F(TestLSystemController, definitionFailureDoesNotStartSizingOrDrawing)
{
    g_l_system_name = "missing";

    EXPECT_THROW(render(), std::runtime_error);

    EXPECT_FALSE(lsystem_loaded());
    EXPECT_EQ(0, m_thinking_entries);
    EXPECT_EQ(0, m_thinking_exits);
    EXPECT_EQ(0, m_lines);
}

} // namespace id::test
