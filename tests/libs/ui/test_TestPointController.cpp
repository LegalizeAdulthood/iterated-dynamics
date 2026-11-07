// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/TestPointController.h>

#include <ui/NonStandardFractalTest.h>

#include <engine/resume.h>
#include <ui/TestRender.h>

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

TEST_P(TestStandaloneTestPoint, controllerAcceptsCompletedResumeWithoutComputingPixels)
{
    NonStandardControllerState controller_state;

    alloc_resume(20, 1);
    put_resume(HEIGHT, GetParam() == CalcMode::ONE_PASS ? 1 : 2);
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, calculation.select());

    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.iterate();
    EXPECT_TRUE(m_columns.empty());
    TestPointController controller{calculation};
    controller.start();
    controller.iterate();
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(m_columns.empty());
    calculation.finish(false);
}

TEST_P(TestStandaloneTestPoint, uiChecksInputBeforeComputingFirstPixel)
{
    NonStandardControllerState controller_state;

    int pending_key{'x'};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([&pending_key] { return pending_key; }));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Invoke([&pending_key] { return std::exchange(pending_key, 0); }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([&pending_key](int key) { pending_key = key; }));

    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ('x', pending_key);
    pending_key = 0;

    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_FALSE(m_columns.empty());
}

class TestSelectedTestPointController : public TestStandaloneTestPoint
{
protected:
    NonStandardControllerState m_controller_state;

    void TearDown() override;
    void prepare_controller();

    std::optional<NonStandardFractal> m_calculation;
    std::optional<TestPointController> m_controller;
};

void TestSelectedTestPointController::TearDown()
{
    m_controller.reset();
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestStandaloneTestPoint::TearDown();
}

void TestSelectedTestPointController::prepare_controller()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, m_calculation->select());
    m_controller.emplace(*m_calculation);
}

class TestPreparedTestPointController : public TestSelectedTestPointController
{
protected:
    void SetUp() override;
};

void TestPreparedTestPointController::SetUp()
{
    TestStandaloneTestPoint::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
}

TEST_P(TestPreparedTestPointController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_P(TestPreparedTestPointController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_P(TestPreparedTestPointController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedTestPointController : public TestPreparedTestPointController
{
protected:
    void SetUp() override;
};

void TestStartedTestPointController::SetUp()
{
    TestPreparedTestPointController::SetUp();
    m_controller->start();
}

TEST_P(TestStartedTestPointController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestSelectedTestPointController, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });
INSTANTIATE_TEST_SUITE_P(Modes, TestPreparedTestPointController, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });
INSTANTIATE_TEST_SUITE_P(Modes, TestStartedTestPointController, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });

TEST_P(TestSelectedTestPointController, controllerStepsExistingPixelsUntilCompletion)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    TestPointController &controller{*m_controller};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_columns.empty());
    controller.iterate();
    EXPECT_EQ(1, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(GetParam() == CalcMode::ONE_PASS ? 1U : 2U, m_columns.size());
    for (int chunks = 0; chunks < MAX_CHUNKS && !controller.done(); ++chunks)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    const std::vector<int> columns{m_columns};
    controller.iterate();
    EXPECT_EQ(columns, m_columns);
}

TEST_P(TestSelectedTestPointController, controllerInterruptsBeforeComputingAPixel)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return('x'));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    TestPointController &controller{*m_controller};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
}

TEST_P(TestSelectedTestPointController, controllerFailureTerminatesTheSelectedOperation)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    TestPointController &controller{*m_controller};
    controller.start();
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("Plot failed")));

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

} // namespace id::test
