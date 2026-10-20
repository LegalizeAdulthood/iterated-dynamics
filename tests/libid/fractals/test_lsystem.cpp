// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/lsystem.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <math/fixed_pt.h>
#include <misc/stack_avail.h>
#include <misc/ValueSaver.h>
#include <ui/text_screen.h>
#include <ui/thinking.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <string>
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

class TestLSystem : public Test
{
protected:
    using Line = std::array<int, 5>;

    void SetUp() override
    {
        EXPECT_CALL(m_driver, draw_line(_, _, _, _, _))
            .WillRepeatedly(Invoke(
                [this](int x0, int y0, int x1, int y1, int color)
                {
                    EXPECT_FALSE(thinking_active());
                    m_lines.push_back({x0, y0, x1, y1, color});
                }));
        EXPECT_CALL(m_driver, stack_screen()).Times(AnyNumber());
        EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
        EXPECT_CALL(m_driver, put_string(_, _, _, _))
            .WillRepeatedly(Invoke(
                [](int, int col, int, const char *text) { g_text_col = col + static_cast<int>(std::strlen(text)); }));
        EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
        EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([this] { return m_cancel ? 'x' : 0; }));
        EXPECT_CALL(m_driver, unstack_screen()).WillRepeatedly(Invoke([this] { ++m_thinking_exits; }));
    }

    void render(const char *name, const int order = 0)
    {
        g_l_system_name = name;
        g_params[0] = order;
        ASSERT_FALSE(lsystem_load());
        char stack_top{};
        const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
        LSystem system;
        system.start();
        for (int i = 0; i < 50000 && !system.done() && !system.interrupted(); ++i)
        {
            system.iterate();
        }
        EXPECT_TRUE(system.done());
        EXPECT_FALSE(system.interrupted());
        EXPECT_FALSE(thinking_active());
    }

    void expect_lines(const std::initializer_list<Line> expected)
    {
        ASSERT_EQ(expected.size(), m_lines.size());
        std::size_t index{};
        for (const Line &line : expected)
        {
            SCOPED_TRACE(index);
            for (int coordinate = 0; coordinate < 4; ++coordinate)
            {
                EXPECT_NEAR(line[coordinate], m_lines[index][coordinate], 1);
            }
            EXPECT_EQ(line[4], m_lines[index][4]);
            ++index;
        }
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{110, 106, 0, 0, 109.0, 105.0}};
    ValueSaver<float> m_saved_aspect{g_screen_aspect, 106.0F / 110.0F};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<bool> m_saved_overflow{g_overflow, false};
    ValueSaver<double> m_saved_order{g_params[0]};
    ValueSaver<char> m_saved_angle{g_max_angle};
    ValueSaver<std::filesystem::path> m_saved_filename{
        g_l_system_filename, std::filesystem::path{data::ID_TEST_LSYSTEM_DIR} / data::ID_TEST_LSYSTEM_FILE};
    ValueSaver<std::string> m_saved_name{g_l_system_name};
    std::vector<Line> m_lines;
    bool m_cancel{};
    int m_thinking_exits{};
};

TEST_F(TestLSystem, fitsBothAxes)
{
    render("SizingBox");

    expect_lines({{5, 103, 105, 103, 15}, {105, 103, 105, 3, 15}, {105, 3, 5, 3, 15}, {5, 3, 5, 103, 15}});
}

TEST_F(TestLSystem, centersDegenerateAxis)
{
    render("SizingHorizontal");

    expect_lines({{5, 53, 55, 53, 15}, {55, 53, 105, 53, 15}});
}

TEST_F(TestLSystem, restoresNestedBranchTurtleButKeepsExtrema)
{
    render("SizingBranches");

    expect_lines({{30, 103, 55, 103, 15}, {55, 103, 55, 53, 15}, {55, 53, 80, 53, 15}, {55, 53, 55, 3, 15},
        {55, 103, 80, 103, 15}});
}

TEST_F(TestLSystem, restoresBranchesInsideExpandedRule)
{
    render("SizingBranchRule", 1);

    expect_lines({{30, 103, 55, 103, 15}, {55, 103, 55, 53, 15}, {55, 53, 80, 53, 15}, {55, 53, 55, 3, 15},
        {55, 103, 80, 103, 15}});
}

TEST_F(TestLSystem, restoresReverseAfterBranch)
{
    render("SizingReverse");

    expect_lines({{5, 53, 55, 53, 15}, {55, 53, 55, 103, 15}, {55, 53, 105, 53, 15}, {105, 53, 105, 3, 15}});
}

TEST_F(TestLSystem, restoresRealAngleAndScaleAfterBranch)
{
    render("SizingRealAngle");

    expect_lines({{5, 3, 55, 3, 15}, {55, 3, 55, 103, 15}, {55, 3, 105, 3, 15}});
}

TEST_F(TestLSystem, includesUnplottedMovesInBounds)
{
    render("SizingMove");

    expect_lines({{38, 53, 105, 53, 15}});
}

TEST_F(TestLSystem, honorsExpansionDepth)
{
    render("SizingExpansion", 0);
    EXPECT_TRUE(m_lines.empty());
    render("SizingExpansion", 1);
    expect_lines({{5, 53, 105, 53, 15}});
    m_lines.clear();
    render("SizingExpansion", 2);
    expect_lines({{30, 103, 80, 103, 15}, {80, 103, 80, 53, 15}, {80, 53, 80, 3, 15}});
}

TEST_F(TestLSystem, appliesAllMatchingRulesInSourceOrder)
{
    render("SizingRules", 1);

    expect_lines({{5, 103, 105, 103, 15}, {105, 103, 105, 3, 15}});
}

TEST_F(TestLSystem, acceptsEmptyReplacement)
{
    render("SizingEmptyRule", 1);

    expect_lines({{55, 103, 55, 3, 15}});
}

TEST_F(TestLSystem, sizesDeepExpansionWithoutRecursiveStack)
{
    render("SizingDeep", 10000);

    expect_lines({{5, 53, 105, 53, 15}});
}

TEST_F(TestLSystem, stopsBeforeDrawingOnOverflow)
{
    g_overflow = true;

    render("SizingBox");

    EXPECT_TRUE(m_lines.empty());
    EXPECT_EQ(0, m_thinking_exits);
}

TEST_F(TestLSystem, cancelsSizingAndRestoresThinkingScreen)
{
    m_cancel = true;

    render("Koch1", 9);

    EXPECT_TRUE(m_lines.empty());
    EXPECT_EQ(1, m_thinking_exits);
}

} // namespace id::test
