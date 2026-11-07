// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/DiffusionController.h>

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

class TestDiffusionController : public TestStandaloneDiffusion
{
protected:
    NonStandardControllerState m_controller_state;

    void TearDown() override;
    void prepare_calculation();

    std::optional<NonStandardFractal> m_calculation;
};

void TestDiffusionController::TearDown()
{
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestNonStandardFractal::TearDown();
}

void TestDiffusionController::prepare_calculation()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::DIFFUSION, m_calculation->select());
}

class TestPreparedDiffusionController : public TestDiffusionController
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<DiffusionController> m_controller;
};

void TestPreparedDiffusionController::SetUp()
{
    TestStandaloneDiffusion::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    m_controller.emplace(*m_calculation);
}

void TestPreparedDiffusionController::TearDown()
{
    m_controller.reset();
    TestDiffusionController::TearDown();
}

TEST_P(TestPreparedDiffusionController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_P(TestPreparedDiffusionController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_P(TestPreparedDiffusionController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedDiffusionController : public TestPreparedDiffusionController
{
protected:
    void SetUp() override;
};

void TestStartedDiffusionController::SetUp()
{
    TestPreparedDiffusionController::SetUp();
    m_controller->start();
}

TEST_P(TestStartedDiffusionController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestDiffusionController, Values(0, 1, 2),
    [](const TestParamInfo<int> &info) { return "Mode" + std::to_string(info.param); });
INSTANTIATE_TEST_SUITE_P(Modes, TestPreparedDiffusionController, Values(0, 1, 2),
    [](const TestParamInfo<int> &info) { return "Mode" + std::to_string(info.param); });
INSTANTIATE_TEST_SUITE_P(Modes, TestStartedDiffusionController, Values(0, 1, 2),
    [](const TestParamInfo<int> &info) { return "Mode" + std::to_string(info.param); });

TEST_P(TestDiffusionController, controllerSuspendsWithoutStartingCalculationWork)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    const std::array<int, SIDE * SIDE> initial{m_frame};
    DiffusionController controller{calculation};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(initial, m_frame);
    controller.suspend();
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_P(TestDiffusionController, controllerRetainsCalculationChunksAndFinalPixels)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    fractals::Diffusion reference;
    int expected_chunks{1};
    while (expected_chunks <= MAX_CHUNKS && reference.iterate())
    {
        ++expected_chunks;
    }
    ASSERT_LE(expected_chunks, MAX_CHUNKS);
    const std::array<int, SIDE * SIDE> expected{m_frame};
    const int expected_next_random{random15()};
    m_frame.fill(0);
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    DiffusionController controller{calculation};
    controller.start();
    int chunks{};

    for (; chunks < MAX_CHUNKS && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(expected_chunks, chunks);
    EXPECT_EQ(expected, m_frame);
    EXPECT_EQ(expected_next_random, random15());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    controller.iterate();
    EXPECT_EQ(expected, m_frame);
}

TEST_P(TestDiffusionController, controllerInterruptionKeepsMenuKeyAndPositiveResult)
{
    int pending_key{'x'};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([&pending_key] { return pending_key; }));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Invoke([&pending_key] { return std::exchange(pending_key, 0); }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([&pending_key](int key) { pending_key = key; }));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    DiffusionController controller{calculation};
    controller.start();

    controller.iterate();

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(1, controller.result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', pending_key);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_P(TestDiffusionController, controllerOrbitToggleDoesNotInterrupt)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    DiffusionController controller{calculation};
    controller.start();

    EXPECT_TRUE(dispatch_keyboard_key('o'));
    EXPECT_TRUE(g_show_orbit);
    EXPECT_TRUE(dispatch_keyboard_key('O'));
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    controller.iterate();
    EXPECT_FALSE(controller.done());
    controller.suspend();
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_P(TestDiffusionController, controllerFailureReleasesItsKeyboardHandler)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    DiffusionController controller{calculation};
    controller.start();
    EXPECT_CALL(m_driver, read_pixel(_, _)).WillOnce(Throw(std::runtime_error("Read failed")));

    EXPECT_THROW(
        {
            for (int chunks = 0; chunks < MAX_CHUNKS && !controller.done(); ++chunks)
            {
                controller.iterate();
            }
        },
        std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

} // namespace id::test
