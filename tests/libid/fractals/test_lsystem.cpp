// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/lsystem.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/resume.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <math/fixed_pt.h>
#include <misc/stack_avail.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
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

namespace
{

int unexpected_renderer()
{
    throw std::logic_error("L-system selection entered a renderer callback");
}

bool unexpected_setup()
{
    throw std::logic_error("L-system selection entered legacy per-image setup");
}

} // namespace

class TestLSystem : public Test
{
protected:
    using Line = std::array<int, 5>;

    void SetUp() override
    {
        m_specific.calc_type = unexpected_renderer;
        m_specific.per_image = unexpected_setup;
        g_dispatch = FractalDispatch{m_specific};
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
        char stack_top{};
        const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
        NonStandardFractal calculation;
        ASSERT_TRUE(calculation.start().empty());
        ASSERT_EQ(NonStandardFractal::Operation::L_SYSTEM, calculation.select());
        for (int i = 0; i < 50000 && !calculation.done(); ++i)
        {
            calculation.iterate();
        }
        EXPECT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    void expect_lines(const std::vector<Line> &expected);

    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::L_SYSTEM)};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::L_SYSTEM};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{110, 106, 0, 0, 109.0, 105.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 110};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 106};
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

void TestLSystem::expect_lines(const std::vector<Line> &expected)
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

struct LSystemRenderInput
{
    const char *name;
    const char *definition;
    int order;
    std::vector<std::array<int, 5>> lines;
};

void PrintTo(const LSystemRenderInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<LSystemRenderInput> lsystem_render_inputs()
{
    return {{"fitsBothAxes", "SizingBox", 0,
                {{5, 103, 105, 103, 15}, {105, 103, 105, 3, 15}, {105, 3, 5, 3, 15}, {5, 3, 5, 103, 15}}},
        {"centersDegenerateAxis", "SizingHorizontal", 0, {{5, 53, 55, 53, 15}, {55, 53, 105, 53, 15}}},
        {"restoresNestedBranchTurtleButKeepsExtrema", "SizingBranches", 0,
            {{30, 103, 55, 103, 15}, {55, 103, 55, 53, 15}, {55, 53, 80, 53, 15}, {55, 53, 55, 3, 15},
                {55, 103, 80, 103, 15}}},
        {"restoresBranchesInsideExpandedRule", "SizingBranchRule", 1,
            {{30, 103, 55, 103, 15}, {55, 103, 55, 53, 15}, {55, 53, 80, 53, 15}, {55, 53, 55, 3, 15},
                {55, 103, 80, 103, 15}}},
        {"restoresReverseAfterBranch", "SizingReverse", 0,
            {{5, 53, 55, 53, 15}, {55, 53, 55, 103, 15}, {55, 53, 105, 53, 15}, {105, 53, 105, 3, 15}}},
        {"restoresRealAngleAndScaleAfterBranch", "SizingRealAngle", 0,
            {{5, 3, 55, 3, 15}, {55, 3, 55, 103, 15}, {55, 3, 105, 3, 15}}},
        {"includesUnplottedMovesInBounds", "SizingMove", 0, {{38, 53, 105, 53, 15}}},
        {"ExpansionDepthZero", "SizingExpansion", 0, {}},
        {"ExpansionDepthOne", "SizingExpansion", 1, {{5, 53, 105, 53, 15}}},
        {"ExpansionDepthTwo", "SizingExpansion", 2,
            {{30, 103, 80, 103, 15}, {80, 103, 80, 53, 15}, {80, 53, 80, 3, 15}}},
        {"appliesAllMatchingRulesInSourceOrder", "SizingRules", 1, {{5, 103, 105, 103, 15}, {105, 103, 105, 3, 15}}},
        {"acceptsEmptyReplacement", "SizingEmptyRule", 1, {{55, 103, 55, 3, 15}}},
        {"sizesDeepExpansionWithoutRecursiveStack", "SizingDeep", 10000, {{5, 53, 105, 53, 15}}}};
}

class TestLSystemRender : public TestLSystem, public WithParamInterface<LSystemRenderInput>
{
};

TEST_P(TestLSystemRender, rendersDefinitionAtOrder)
{
    render(GetParam().definition, GetParam().order);

    if (GetParam().lines.empty())
    {
        EXPECT_TRUE(m_lines.empty());
    }
    else
    {
        expect_lines(GetParam().lines);
    }
}

INSTANTIATE_TEST_SUITE_P(Definitions, TestLSystemRender, ValuesIn(lsystem_render_inputs()),
    [](const TestParamInfo<LSystemRenderInput> &info) { return info.param.name; });

TEST_F(TestLSystem, stopsBeforeDrawingOnOverflow)
{
    g_overflow = true;

    render("SizingBox");

    EXPECT_TRUE(m_lines.empty());
}

class LSystemDefinitionTest : public TestLSystem
{
protected:
    void load_definition(const char *name, int order);
    void finish_sizing(LSystem &system);
    void start_selected(NonStandardFractal &calculation);
    void advance_to_drawing(NonStandardFractal &calculation);
};

void LSystemDefinitionTest::load_definition(const char *name, const int order)
{
    g_l_system_name = name;
    g_params[0] = order;
    ASSERT_NO_THROW(lsystem_load());
}

void LSystemDefinitionTest::finish_sizing(LSystem &system)
{
    for (int i = 0; i < 100 && system.sizing(); ++i)
    {
        system.iterate();
        EXPECT_TRUE(m_lines.empty());
    }
    ASSERT_FALSE(system.sizing());
    ASSERT_FALSE(system.done());
}

void LSystemDefinitionTest::start_selected(NonStandardFractal &calculation)
{
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::L_SYSTEM, calculation.select());
}

void LSystemDefinitionTest::advance_to_drawing(NonStandardFractal &calculation)
{
    for (int i = 0; i < 100 && calculation.sizing(); ++i)
    {
        calculation.iterate();
    }
    ASSERT_FALSE(calculation.sizing());
    ASSERT_FALSE(calculation.done());
}

class TestGrowingLSystem : public LSystemDefinitionTest
{
protected:
    void SetUp() override;
};

void TestGrowingLSystem::SetUp()
{
    LSystemDefinitionTest::SetUp();
    ASSERT_NO_FATAL_FAILURE(load_definition("Koch1", 9));
}

TEST_F(TestGrowingLSystem, startInitializesSizingWithoutTraversing)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;

    system.start();

    EXPECT_TRUE(system.sizing());
    EXPECT_FALSE(system.done());
    EXPECT_FALSE(system.interrupted());
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestGrowingLSystem, boundedSizingRemainsIncomplete)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();

    for (int i = 0; i < 1024; ++i)
    {
        system.iterate();
    }

    EXPECT_TRUE(system.sizing());
    EXPECT_FALSE(system.done());
    EXPECT_TRUE(m_lines.empty());
}

class TestBoxLSystem : public LSystemDefinitionTest
{
protected:
    void SetUp() override;
};

void TestBoxLSystem::SetUp()
{
    LSystemDefinitionTest::SetUp();
    ASSERT_NO_FATAL_FAILURE(load_definition("SizingBox", 0));
}

TEST_F(TestBoxLSystem, sizingEndsBeforeDrawingStarts)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();

    finish_sizing(system);
}

TEST_F(TestBoxLSystem, firstDrawingIncrementPlotsOneLine)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();
    ASSERT_NO_FATAL_FAILURE(finish_sizing(system));

    system.iterate();

    EXPECT_EQ(1U, m_lines.size());
}

TEST_F(TestBoxLSystem, drawingCompletesAfterSizing)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    LSystem system;
    system.start();
    ASSERT_NO_FATAL_FAILURE(finish_sizing(system));
    system.iterate();

    for (int i = 0; i < 100 && !system.done(); ++i)
    {
        system.iterate();
    }

    EXPECT_TRUE(system.done());
    EXPECT_FALSE(system.sizing());
}

TEST_F(TestGrowingLSystem, suspensionStopsSizing)
{
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

TEST_F(TestGrowingLSystem, overflowDuringSizingStopsBeforeDrawing)
{
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

class TestSelectedLSystem : public LSystemDefinitionTest
{
protected:
    void SetUp() override;
};

void TestSelectedLSystem::SetUp()
{
    LSystemDefinitionTest::SetUp();
    ASSERT_NO_FATAL_FAILURE(load_definition("Koch1", 3));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
}

TEST_F(TestSelectedLSystem, preservesSizingAndDrawingIncrements)
{
    struct WorkState
    {
        bool sizing;
        bool done;
        std::size_t lines;
    };

    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    std::vector<WorkState> expected;
    {
        LSystem system;
        system.start();
        for (int i = 0; i < 5000 && !system.done() && !system.interrupted(); ++i)
        {
            system.iterate();
            expected.push_back({system.sizing(), system.done(), m_lines.size()});
        }
        ASSERT_TRUE(system.done());
        ASSERT_FALSE(system.interrupted());
    }
    const std::vector<Line> expected_lines{m_lines};
    m_lines.clear();
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    ASSERT_EQ(NonStandardFractal::Operation::L_SYSTEM, calculation.select());

    EXPECT_TRUE(calculation.sizing());
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_lines.empty());
    for (const WorkState &work : expected)
    {
        calculation.iterate();
        EXPECT_EQ(work.sizing, calculation.sizing());
        EXPECT_EQ(work.done, calculation.done());
        EXPECT_EQ(work.lines, m_lines.size());
    }
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(expected_lines, m_lines);
    calculation.finish(false);
}

class TestCachedLSystemSelection : public LSystemDefinitionTest
{
protected:
    void SetUp() override;
};

void TestCachedLSystemSelection::SetUp()
{
    LSystemDefinitionTest::SetUp();
    g_l_system_name = "SizingBox";
    lsystem_load();
    g_l_system_filename.replace_filename("missing.l");
}

TEST_F(TestCachedLSystemSelection, usesAlreadyLoadedDefinition)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    NonStandardFractal calculation;

    ASSERT_NO_FATAL_FAILURE(start_selected(calculation));

    EXPECT_TRUE(calculation.sizing());
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestCachedLSystemSelection, suspensionFinalizesNonResumableWork)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    NonStandardFractal calculation;
    ASSERT_NO_FATAL_FAILURE(start_selected(calculation));

    calculation.suspend();

    EXPECT_TRUE(calculation.done());
    EXPECT_FALSE(lsystem_loaded());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
}

class TestSelectedDrawingLSystem : public LSystemDefinitionTest
{
protected:
    void SetUp() override;
};

void TestSelectedDrawingLSystem::SetUp()
{
    LSystemDefinitionTest::SetUp();
    g_l_system_name = "Koch1";
    g_params[0] = 0;
}

TEST_F(TestSelectedDrawingLSystem, overflowTerminatesSelectedOperation)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    NonStandardFractal calculation;
    ASSERT_NO_FATAL_FAILURE(start_selected(calculation));
    ASSERT_NO_FATAL_FAILURE(advance_to_drawing(calculation));
    g_overflow = true;

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(m_lines.empty());
    calculation.finish(false);
}

struct LSystemCompletionInput
{
    const char *name;
    bool overflow;
};

void PrintTo(const LSystemCompletionInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCompletedLSystemSelection : public LSystemDefinitionTest, public WithParamInterface<LSystemCompletionInput>
{
protected:
    void SetUp() override;
    void complete_selected(NonStandardFractal &calculation);
};

void TestCompletedLSystemSelection::SetUp()
{
    LSystemDefinitionTest::SetUp();
    g_l_system_name = "Koch1";
    g_params[0] = GetParam().overflow ? 0 : 3;
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
}

void TestCompletedLSystemSelection::complete_selected(NonStandardFractal &calculation)
{
    ASSERT_NO_FATAL_FAILURE(start_selected(calculation));
    if (GetParam().overflow)
    {
        ASSERT_NO_FATAL_FAILURE(advance_to_drawing(calculation));
        g_overflow = true;
        calculation.iterate();
        ASSERT_TRUE(m_lines.empty());
    }
    else
    {
        for (int i = 0; i < 5000 && !calculation.done(); ++i)
        {
            calculation.iterate();
        }
    }
    ASSERT_TRUE(calculation.done());
    if (!GetParam().overflow)
    {
        ASSERT_TRUE(calculation.completed());
    }
}

TEST_P(TestCompletedLSystemSelection, iterateAfterCompletionDoesNotPlot)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    NonStandardFractal calculation;
    ASSERT_NO_FATAL_FAILURE(complete_selected(calculation));
    const std::vector<Line> expected_lines{m_lines};

    calculation.iterate();

    EXPECT_EQ(expected_lines, m_lines);
    calculation.finish(false);
}

TEST_P(TestCompletedLSystemSelection, finishUnloadsDefinitionAndClearsOverflow)
{
    char stack_top{};
    const ValueSaver<char *> saved_stack{g_top_of_stack, &stack_top};
    NonStandardFractal calculation;
    ASSERT_NO_FATAL_FAILURE(complete_selected(calculation));

    calculation.finish(false);

    EXPECT_FALSE(lsystem_loaded());
    EXPECT_FALSE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(Completion, TestCompletedLSystemSelection,
    Values(LSystemCompletionInput{"Normal", false}, LSystemCompletionInput{"DrawingOverflow", true}),
    [](const TestParamInfo<LSystemCompletionInput> &info) { return info.param.name; });

class TestLSystemLoad : public TestLSystem
{
protected:
    void SetUp() override;

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

    void expect_failure(const char *message);

    std::filesystem::path m_path;
};

void TestLSystemLoad::SetUp()
{
    TestLSystem::SetUp();
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '-');
    m_path = std::filesystem::path{data::ID_TEST_DATA_DIR} / ("lsystem-" + name + ".l");
    g_l_system_filename = m_path;
    g_l_system_name = "entry";
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, stack_screen()).Times(0);
}

void TestLSystemLoad::expect_failure(const char *message)
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
    EXPECT_TRUE(m_lines.empty());
}

struct LSystemLoadInput
{
    const char *name;
    std::optional<std::string> definition;
    const char *entry;
    const char *message;
    std::optional<std::string> previous_definition;
};

void PrintTo(const LSystemLoadInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<LSystemLoadInput> lsystem_load_inputs()
{
    std::vector<LSystemLoadInput> inputs{
        {"MissingFile", std::nullopt, "entry", "'entry' file entry item not found", std::nullopt},
        {"MissingEntry", "other {\naxiom f\nangle 4\n}\n", "entry", "'entry' file entry item not found", std::nullopt},
        {"EmptyEntryName", std::nullopt, "", "File entry item name is empty", std::nullopt},
        {"MissingAxiom", "entry {\nangle 4\n}\n", "entry", "Error:  no axiom", std::nullopt},
        {"MissingAxiomValue", "entry {\naxiom\nangle 4\n}\n", "entry", "Error:  no axiom", std::nullopt},
        {"MissingAngle", "entry {\naxiom f\n}\n", "entry", "Error:  illegal or missing angle", std::nullopt},
        {"ReservedSymbol", "entry {\naxiom f\nangle 4\n+ = f\n}\n", "entry", "Redefined reserved symbol +",
            std::nullopt},
        {"ExtraText", "entry {\naxiom f extra\nangle 4\n}\n", "entry", "Extra text after command line 2: extra",
            std::nullopt},
        {"MalformedCommand", "entry {\naxiom f\nangle 4\n=\n}\n", "entry", "missing command", std::nullopt},
        {"UnknownCommand", "entry {\naxiom f\nangle 4\nunknown f\n}\n", "entry", "Syntax error line 4: unknown",
            std::nullopt},
        {"UnterminatedDefinition", "entry {\naxiom f\nangle 4\n", "entry", "'entry' file entry item not found",
            std::nullopt},
        {"FailedReloadCannotReusePreviousAxiom", "entry {\nangle 4\n}\n", "entry", "Error:  no axiom",
            "entry {\naxiom f\nangle 4\nf = ff\n}\n"}};

    struct Angle
    {
        const char *name;
        const char *value;
    };

    const std::array<Angle, 8> angles{{{"EmptyAngle", ""}, {"WordAngle", "two"}, {"TrailingAngle", "4junk"},
        {"AngleTwo", "2"}, {"AngleFiftyOne", "51"}, {"AngleTwoSixty", "260"}, {"NegativeAngle", "-4"},
        {"OverflowAngle", "99999999999999999999"}}};
    for (const Angle &angle : angles)
    {
        inputs.push_back({angle.name, "entry {\naxiom f\nangle " + std::string{angle.value} + "\n}\n", "entry",
            "Error:  illegal or missing angle", std::nullopt});
    }
    return inputs;
}

class TestInvalidLSystemLoad : public TestLSystemLoad, public WithParamInterface<LSystemLoadInput>
{
protected:
    void SetUp() override;
};

void TestInvalidLSystemLoad::SetUp()
{
    TestLSystemLoad::SetUp();
    g_l_system_name = GetParam().entry;
    if (GetParam().previous_definition)
    {
        write_definition(*GetParam().previous_definition);
        ASSERT_NO_THROW(lsystem_load());
        ASSERT_TRUE(lsystem_loaded());
    }
    if (GetParam().definition)
    {
        write_definition(*GetParam().definition);
    }
}

TEST_P(TestInvalidLSystemLoad, reportsDiagnosticAndClearsDefinition)
{
    expect_failure(GetParam().message);
}

class TestFailedLSystemStart : public TestInvalidLSystemLoad
{
protected:
    void SetUp() override;
};

void TestFailedLSystemStart::SetUp()
{
    ASSERT_NO_FATAL_FAILURE(TestInvalidLSystemLoad::SetUp());
    ASSERT_NO_FATAL_FAILURE(expect_failure(GetParam().message));
}

TEST_P(TestFailedLSystemStart, rejectsStartWithoutAValidDefinition)
{
    LSystem system;

    EXPECT_THROW(system.start(), std::logic_error);

    EXPECT_TRUE(system.done());
    EXPECT_FALSE(system.sizing());
    EXPECT_TRUE(m_lines.empty());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInvalidLSystemLoad, ValuesIn(lsystem_load_inputs()),
    [](const TestParamInfo<LSystemLoadInput> &info) { return info.param.name; });
INSTANTIATE_TEST_SUITE_P(Inputs, TestFailedLSystemStart, ValuesIn(lsystem_load_inputs()),
    [](const TestParamInfo<LSystemLoadInput> &info) { return info.param.name; });

class TestLSystemLoadRecovery : public TestLSystemLoad
{
protected:
    void SetUp() override;
};

void TestLSystemLoadRecovery::SetUp()
{
    TestLSystemLoad::SetUp();
    write_definition("entry {\naxiom f\nangle 2\n}\n");
    ASSERT_NO_FATAL_FAILURE(expect_failure("Error:  illegal or missing angle"));
    LSystem system;
    ASSERT_THROW(system.start(), std::logic_error);
    ASSERT_TRUE(system.done());
    ASSERT_FALSE(system.sizing());
    ASSERT_TRUE(m_lines.empty());
}

TEST_F(TestLSystemLoadRecovery, validDefinitionAfterFailureCanSizeAndDraw)
{
    write_definition("entry {\naxiom f\nangle 4\n}\n");

    render("entry");

    EXPECT_EQ(1U, m_lines.size());
}

} // namespace id::test
