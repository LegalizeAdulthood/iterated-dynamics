// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/PopcornController.h>

#include <ui/NonStandardFractalTest.h>

#include <engine/resume.h>
#include <ui/KeyboardHandler.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;
using namespace id::test::non_standard;
using namespace id::ui;

namespace id::test
{

class TestPopcornController : public TestStandalonePopcorn
{
protected:
    NonStandardControllerState m_controller_state;

    void TearDown() override;
    void prepare_calculation();

    std::optional<NonStandardFractal> m_calculation;
};

void TestPopcornController::TearDown()
{
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestStandalonePopcorn::TearDown();
}

void TestPopcornController::prepare_calculation()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::POPCORN, m_calculation->select());
}

class TestPreparedPopcornController : public TestPopcornController
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<PopcornController> m_controller;
};

void TestPreparedPopcornController::SetUp()
{
    TestStandalonePopcorn::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    m_controller.emplace(*m_calculation);
}

void TestPreparedPopcornController::TearDown()
{
    m_controller.reset();
    TestPopcornController::TearDown();
}

TEST_P(TestPreparedPopcornController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_P(TestPreparedPopcornController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_P(TestPreparedPopcornController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedPopcornController : public TestPreparedPopcornController
{
protected:
    void SetUp() override;
};

void TestStartedPopcornController::SetUp()
{
    TestPreparedPopcornController::SetUp();
    m_controller->start();
}

TEST_P(TestStartedPopcornController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedPopcornController : public TestPreparedPopcornController
{
protected:
    void SetUp() override;
};

void TestCompletedPopcornController::SetUp()
{
    TestPreparedPopcornController::SetUp();
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    m_controller->start();
    for (long i = 0; i <= WIDTH * HEIGHT * g_max_iterations && !m_controller->done(); ++i)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_P(TestCompletedPopcornController, rejectsSuspensionAfterCompletion)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Types, TestPopcornController, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info)
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });
INSTANTIATE_TEST_SUITE_P(Types, TestPreparedPopcornController, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info)
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });
INSTANTIATE_TEST_SUITE_P(Types, TestStartedPopcornController, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info)
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });
INSTANTIATE_TEST_SUITE_P(Types, TestCompletedPopcornController, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info)
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });

TEST_P(TestPopcornController, stepwiseControllerPreservesCalculationIncrements)
{
    struct WorkState
    {
        int row;
        int col;
        long iteration;
        std::array<int, WIDTH * HEIGHT> pixels;
        std::size_t plots;
        bool done;
    };

    std::vector<WorkState> expected;
    {
        NonStandardFractal control;
        control.start();
        ASSERT_EQ(NonStandardFractal::Operation::POPCORN, control.select());
        for (long i = 0; i <= WIDTH * HEIGHT * g_max_iterations && !control.done(); ++i)
        {
            control.iterate();
            expected.push_back({g_row, g_col, g_color_iter, m_pixels, m_columns.size(), control.done()});
        }
        ASSERT_TRUE(control.done());
        control.finish(false);
    }
    m_pixels.fill(0);
    m_columns.clear();
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    PopcornController controller{calculation};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_columns.empty());
    for (const WorkState &work : expected)
    {
        controller.iterate();
        EXPECT_EQ(work.row, g_row);
        EXPECT_EQ(work.col, g_col);
        EXPECT_EQ(work.iteration, g_color_iter);
        EXPECT_EQ(work.pixels, m_pixels);
        EXPECT_EQ(work.plots, m_columns.size());
        EXPECT_EQ(work.done, controller.done());
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_FALSE(dispatch_keyboard_key('x'));
    const std::size_t plots{m_columns.size()};
    controller.iterate();
    EXPECT_EQ(plots, m_columns.size());
}

TEST_P(TestPopcornController, stepwiseMenuInterruptionFollowsWorkAndPreservesResumeRow)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(1, g_color_iter);
                return 'x';
            }));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, unget_key('x'));
    PopcornController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
    ASSERT_EQ(1, start_resume());
    int row{-1};
    get_resume(row);
    EXPECT_EQ(0, row);
}

TEST_P(TestPopcornController, stepwiseSuspensionRetainsTheCurrentRow)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    PopcornController controller{calculation};
    controller.start();
    for (long chunks = 0; chunks < (WIDTH + 1) * g_max_iterations; ++chunks)
    {
        controller.iterate();
    }
    ASSERT_EQ(1, g_row);
    ASSERT_EQ(1, g_col);

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
    ASSERT_EQ(1, start_resume());
    int row{-1};
    get_resume(row);
    EXPECT_EQ(1, row);
}

TEST_P(TestPopcornController, stepwiseInputFailureReleasesHandlerAndCalculation)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(std::runtime_error("input failed")));
    PopcornController controller{calculation};
    controller.start();

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
}

TEST_P(TestPopcornController, stepwisePlotFailureReleasesHandlerAndCalculation)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("plot failed")));
    PopcornController controller{calculation};
    controller.start();

    EXPECT_THROW(
        {
            for (long i = 0; i <= WIDTH * HEIGHT * g_max_iterations && !controller.done(); ++i)
            {
                controller.iterate();
            }
        },
        std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('x'));
}

TEST_P(TestPopcornController, stepwiseDestructionOnlyReleasesHandler)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    {
        PopcornController controller{calculation};
        controller.start();
        controller.iterate();
        ASSERT_FALSE(controller.done());
    }

    EXPECT_FALSE(dispatch_keyboard_key('x'));
    EXPECT_FALSE(calculation.done());
}

} // namespace id::test
