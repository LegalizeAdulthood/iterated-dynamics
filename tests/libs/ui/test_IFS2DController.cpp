// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/IFS2DController.h>

#include <ui/NonStandardFractalTest.h>

#include <engine/resume.h>

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

class TestIFSController : public TestSelectedIFS
{
protected:
    NonStandardControllerState m_controller_state;

    void TearDown() override;
    void prepare_controller();

    std::optional<NonStandardFractal> m_calculation;
    std::optional<IFS2DController> m_controller;
};

void TestIFSController::TearDown()
{
    m_controller.reset();
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestSelectedIFS::TearDown();
}

void TestIFSController::prepare_controller()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::IFS_2D, m_calculation->select());
    m_controller.emplace(*m_calculation);
}

class TestPreparedIFSController : public TestIFSController
{
protected:
    void SetUp() override;
};

void TestPreparedIFSController::SetUp()
{
    TestSelectedIFS::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
}

TEST_F(TestPreparedIFSController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_F(TestPreparedIFSController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_F(TestPreparedIFSController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedIFSController : public TestPreparedIFSController
{
protected:
    void SetUp() override;
};

void TestStartedIFSController::SetUp()
{
    TestPreparedIFSController::SetUp();
    m_controller->start();
}

TEST_F(TestStartedIFSController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedIFSController : public TestPreparedIFSController
{
protected:
    void SetUp() override;
};

void TestCompletedIFSController::SetUp()
{
    TestPreparedIFSController::SetUp();
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    m_controller->start();
    m_controller->iterate();
    for (int point = 1; point < 1025 && !m_controller->done(); ++point)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_F(TestCompletedIFSController, rejectsSuspensionAfterCompletion)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

TEST_F(TestIFSController, controllerStepsOnePointAtATimeUntilCompletion)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    IFS2DController &controller{*m_controller};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_TRUE(m_columns.empty());
    controller.iterate();
    EXPECT_EQ(1L, g_color_iter);
    EXPECT_EQ(1U, m_columns.size());
    EXPECT_FALSE(controller.done());
    for (int point = 1; point < 1025 && !controller.done(); ++point)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(1025L, g_color_iter);
    EXPECT_EQ(1025U, m_columns.size());
    controller.iterate();
    EXPECT_EQ(1025L, g_color_iter);
}

TEST_F(TestIFSController, controllerInterruptsBeforeFirstPointAndKeepsCommandPending)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return('x'));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    IFS2DController &controller{*m_controller};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_TRUE(m_columns.empty());
}

TEST_F(TestIFSController, controllerSuspendsWithoutResumeDataAndNextSelectionRestarts)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    IFS2DController &controller{*m_controller};
    controller.start();
    controller.iterate();
    ASSERT_EQ(1L, g_color_iter);

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    NonStandardFractal restarted;
    restarted.start();
    ASSERT_EQ(NonStandardFractal::Operation::IFS_2D, restarted.select());
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_FALSE(restarted.done());
}

TEST_F(TestIFSController, controllerPlotFailureTerminatesTheSelectedOperation)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    IFS2DController &controller{*m_controller};
    controller.start();
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("Plot failed")));

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestIFSController, controllerInputFailureTerminatesBeforeComputingAPoint)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(std::runtime_error("Input failed")));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    IFS2DController &controller{*m_controller};
    controller.start();

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(m_columns.empty());
}

} // namespace id::test
