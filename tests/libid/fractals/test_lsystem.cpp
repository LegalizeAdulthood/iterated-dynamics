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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
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
                    m_lines.push_back({x0, y0, x1, y1, color});
                }));
    }

    void render(const char *name, const int order = 0)
    {
        g_l_system_name = name;
        g_params[0] = order;
        ASSERT_NO_THROW(lsystem_load());
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
}

TEST_F(TestLSystem, startInitializesSizingWithoutTraversing)
{
    g_l_system_name = "Koch1";
    g_params[0] = 9;
    ASSERT_NO_THROW(lsystem_load());
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;

    system.start();

    EXPECT_TRUE(system.sizing());
    EXPECT_FALSE(system.done());
    EXPECT_FALSE(system.interrupted());
    for (int i = 0; i < 1024; ++i)
    {
        system.iterate();
    }
    EXPECT_TRUE(system.sizing());
    EXPECT_FALSE(system.done());
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestLSystem, sizingEndsBeforeDrawingStarts)
{
    g_l_system_name = "SizingBox";
    g_params[0] = 0;
    ASSERT_NO_THROW(lsystem_load());
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();

    for (int i = 0; i < 100 && system.sizing(); ++i)
    {
        system.iterate();
        EXPECT_TRUE(m_lines.empty());
    }

    EXPECT_FALSE(system.sizing());
    EXPECT_FALSE(system.done());
    system.iterate();
    EXPECT_EQ(1U, m_lines.size());
    for (int i = 0; i < 100 && !system.done(); ++i)
    {
        system.iterate();
    }
    EXPECT_TRUE(system.done());
    EXPECT_FALSE(system.sizing());
}

TEST_F(TestLSystem, suspensionStopsSizing)
{
    g_l_system_name = "Koch1";
    g_params[0] = 9;
    ASSERT_NO_THROW(lsystem_load());
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();
    system.iterate();

    system.suspend();
    system.iterate();

    EXPECT_TRUE(system.interrupted());
    EXPECT_FALSE(system.done());
    EXPECT_TRUE(system.sizing());
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestLSystem, overflowDuringSizingStopsBeforeDrawing)
{
    g_l_system_name = "Koch1";
    g_params[0] = 9;
    ASSERT_NO_THROW(lsystem_load());
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();
    system.iterate();

    g_overflow = true;
    system.iterate();

    EXPECT_TRUE(system.done());
    EXPECT_FALSE(system.sizing());
    EXPECT_FALSE(system.interrupted());
    EXPECT_TRUE(m_lines.empty());
}

class TestLSystemLoad : public TestLSystem
{
protected:
    void SetUp() override
    {
        TestLSystem::SetUp();
        m_path = std::filesystem::path{data::ID_TEST_DATA_DIR} /
            ("lsystem-" + std::string{UnitTest::GetInstance()->current_test_info()->name()} + ".l");
        g_l_system_filename = m_path;
        g_l_system_name = "entry";
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
        EXPECT_CALL(m_driver, stack_screen()).Times(0);
    }

    void TearDown() override
    {
        LSystem cleanup;
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }

    void write_definition(const std::string &text)
    {
        std::ofstream out{m_path};
        ASSERT_TRUE(out);
        out << text;
    }

    void expect_failure(const char *message)
    {
        try
        {
            lsystem_load();
            FAIL() << "Expected a loader diagnostic";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_THAT(error.what(), HasSubstr(message));
        }
        EXPECT_FALSE(lsystem_loaded());
        EXPECT_EQ(0, g_max_angle);
        LSystem system;
        EXPECT_THROW(system.start(), std::logic_error);
        EXPECT_TRUE(system.done());
        EXPECT_FALSE(system.sizing());
        EXPECT_TRUE(m_lines.empty());
    }

    std::filesystem::path m_path;
};

TEST_F(TestLSystemLoad, reportsMissingFileWithoutUI)
{
    expect_failure("'entry' file entry item not found");
}

TEST_F(TestLSystemLoad, reportsMissingEntryWithoutUI)
{
    write_definition("other {\naxiom f\nangle 4\n}\n");

    expect_failure("'entry' file entry item not found");
}

TEST_F(TestLSystemLoad, reportsEmptyEntryNameWithoutUI)
{
    g_l_system_name.clear();

    expect_failure("File entry item name is empty");
}

TEST_F(TestLSystemLoad, rejectsMissingAxiom)
{
    write_definition("entry {\nangle 4\n}\n");

    expect_failure("Error:  no axiom");
}

TEST_F(TestLSystemLoad, rejectsMissingAxiomValue)
{
    write_definition("entry {\naxiom\nangle 4\n}\n");

    expect_failure("Error:  no axiom");
}

TEST_F(TestLSystemLoad, rejectsMissingAngle)
{
    write_definition("entry {\naxiom f\n}\n");

    expect_failure("Error:  illegal or missing angle");
}

TEST_F(TestLSystemLoad, rejectsInvalidAngle)
{
    for (const char *angle : {"", "two", "4junk", "2", "51", "260", "-4", "99999999999999999999"})
    {
        SCOPED_TRACE(angle);
        write_definition("entry {\naxiom f\nangle " + std::string{angle} + "\n}\n");

        expect_failure("Error:  illegal or missing angle");
    }
}

TEST_F(TestLSystemLoad, rejectsRedefinedReservedSymbol)
{
    write_definition("entry {\naxiom f\nangle 4\n+ = f\n}\n");

    expect_failure("Redefined reserved symbol +");
}

TEST_F(TestLSystemLoad, rejectsExtraText)
{
    write_definition("entry {\naxiom f extra\nangle 4\n}\n");

    expect_failure("Extra text after command line 2: extra");
}

TEST_F(TestLSystemLoad, rejectsMalformedCommand)
{
    write_definition("entry {\naxiom f\nangle 4\n=\n}\n");

    expect_failure("missing command");
}

TEST_F(TestLSystemLoad, rejectsUnknownCommand)
{
    write_definition("entry {\naxiom f\nangle 4\nunknown f\n}\n");

    expect_failure("Syntax error line 4: unknown");
}

TEST_F(TestLSystemLoad, rejectsUnterminatedDefinition)
{
    write_definition("entry {\naxiom f\nangle 4\n");

    expect_failure("'entry' file entry item not found");
}

TEST_F(TestLSystemLoad, failedReloadCannotReusePreviousAxiom)
{
    write_definition("entry {\naxiom f\nangle 4\nf = ff\n}\n");
    ASSERT_NO_THROW(lsystem_load());
    ASSERT_TRUE(lsystem_loaded());
    write_definition("entry {\nangle 4\n}\n");

    expect_failure("Error:  no axiom");
}

TEST_F(TestLSystemLoad, validDefinitionAfterFailureCanSizeAndDraw)
{
    write_definition("entry {\naxiom f\nangle 2\n}\n");
    expect_failure("Error:  illegal or missing angle");
    write_definition("entry {\naxiom f\nangle 4\n}\n");

    render("entry");

    EXPECT_EQ(1U, m_lines.size());
}

} // namespace id::test
