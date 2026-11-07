// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/BifurcationController.h>

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

TEST_P(TestStandaloneBifurcation, uiEntryUsesSelectedOperationWithoutLegacyCallbacks)
{
    NonStandardControllerState controller_state;

    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));

    EXPECT_EQ(0, render_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, g_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
}

TEST_P(TestStandaloneBifurcation, uiInterruptionLeavesMenuKeyPendingAndResumesAtNextColumn)
{
    NonStandardControllerState controller_state;

    int pending_key{};
    bool interrupt_sent{};
    EXPECT_CALL(m_driver, key_pressed())
        .WillRepeatedly(Invoke(
            [&]
            {
                if (!interrupt_sent && m_columns.size() >= HEIGHT)
                {
                    pending_key = 'x';
                    interrupt_sent = true;
                }
                return pending_key;
            }));
    EXPECT_CALL(m_driver, get_key())
        .WillRepeatedly(Invoke(
            [&pending_key]
            {
                const int key{pending_key};
                pending_key = 0;
                return key;
            }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([&pending_key](int key) { pending_key = key; }));

    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_EQ('x', pending_key);
    ASSERT_EQ(HEIGHT, m_columns.size());
    pending_key = 0;

    EXPECT_EQ(0, render_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, g_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    for (int x = 0; x < WIDTH; ++x)
    {
        EXPECT_EQ(HEIGHT, std::count(m_columns.begin(), m_columns.end(), x));
    }
}

class TestBifurcationController : public TestStandaloneBifurcation
{
protected:
    NonStandardControllerState m_controller_state;

    void TearDown() override;
    void prepare_controller();

    std::optional<NonStandardFractal> m_calculation;
    std::optional<BifurcationController> m_controller;
};

void TestBifurcationController::TearDown()
{
    m_controller.reset();
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestNonStandardFractal::TearDown();
}

void TestBifurcationController::prepare_controller()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, m_calculation->select());
    m_controller.emplace(*m_calculation);
}

class TestPreparedBifurcationController : public TestBifurcationController
{
protected:
    void SetUp() override;
};

void TestPreparedBifurcationController::SetUp()
{
    TestStandaloneBifurcation::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
}

TEST_P(TestPreparedBifurcationController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_P(TestPreparedBifurcationController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_P(TestPreparedBifurcationController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedBifurcationController : public TestPreparedBifurcationController
{
protected:
    void SetUp() override;
};

void TestStartedBifurcationController::SetUp()
{
    TestPreparedBifurcationController::SetUp();
    m_controller->start();
}

TEST_P(TestStartedBifurcationController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedBifurcationController : public TestPreparedBifurcationController
{
protected:
    void SetUp() override;
};

void TestCompletedBifurcationController::SetUp()
{
    TestPreparedBifurcationController::SetUp();
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    m_controller->start();
    m_controller->iterate();
    for (int chunks = 0; chunks < WIDTH + 1 && !m_controller->done(); ++chunks)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_P(TestCompletedBifurcationController, rejectsSuspensionAfterCompletion)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Types, TestBifurcationController, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) { return info.param.name; });
INSTANTIATE_TEST_SUITE_P(Types, TestPreparedBifurcationController, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) { return info.param.name; });
INSTANTIATE_TEST_SUITE_P(Types, TestStartedBifurcationController, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) { return info.param.name; });
INSTANTIATE_TEST_SUITE_P(Types, TestCompletedBifurcationController, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) { return info.param.name; });

TEST_P(TestBifurcationController, controllerAdvancesOneColumnPerStepUntilComplete)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    BifurcationController &controller{*m_controller};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_columns.empty());
    controller.iterate();
    EXPECT_EQ(HEIGHT, m_columns.size());
    EXPECT_FALSE(controller.done());
    for (int chunks = 0; chunks < WIDTH + 1 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    controller.iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
}

TEST_P(TestBifurcationController, controllerSuspensionResumesWithoutRepeatingColumns)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    BifurcationController &controller{*m_controller};
    controller.start();
    controller.iterate();

    controller.suspend();

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, resumed.select());
    BifurcationController continued{resumed};
    continued.start();
    for (int chunks = 0; chunks < WIDTH + 1 && !continued.done(); ++chunks)
    {
        continued.iterate();
    }
    ASSERT_TRUE(continued.done());
    EXPECT_EQ(0, continued.result());
    EXPECT_EQ(12 * WIDTH, g_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    resumed.finish(false);
}

TEST_P(TestBifurcationController, controllerFailureTerminatesTheSelectedOperation)
{
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    g_dispatch.set_orbit_calc([]() -> int { throw std::runtime_error("Orbit failed"); });
    BifurcationController &controller{*m_controller};
    controller.start();

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

} // namespace id::test
