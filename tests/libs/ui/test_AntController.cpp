// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/AntController.h>

#include <ui/NonStandardFractalTest.h>

#include <engine/resume.h>
#include <engine/spindac.h>
#include <helpdefs.h>
#include <io/loadmap.h>
#include <ui/KeyboardHandler.h>
#include <ui/help.h>
#include <ui/id_keys.h>

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

class TestAntController : public TestSelectedAnt
{
protected:
    NonStandardControllerState m_controller_state;

    ValueSaver<help::HelpLabels> m_saved_help{help::g_help_mode, help::HelpLabels::NONE};

    void TearDown() override;
    void prepare_calculation();
    void prepare_existing_image();

    std::optional<NonStandardFractal> m_calculation;
};

void TestAntController::TearDown()
{
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    TestNonStandardFractal::TearDown();
}

void TestAntController::prepare_calculation()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::ANT, m_calculation->select());
}

void TestAntController::prepare_existing_image()
{
    m_calculation.emplace();
    m_calculation->select_ant_on_image();
}

class TestPreparedAntController : public TestAntController
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::size_t m_expected_batches{};
    std::optional<AntController> m_controller;
};

void TestPreparedAntController::SetUp()
{
    TestSelectedAnt::SetUp();
    m_expected_batches = reference_frames().size();
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    m_controller.emplace(*m_calculation);
}

void TestPreparedAntController::TearDown()
{
    m_controller.reset();
    TestAntController::TearDown();
}

TEST_P(TestPreparedAntController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_P(TestPreparedAntController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_P(TestPreparedAntController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedAntController : public TestPreparedAntController
{
protected:
    void SetUp() override;
};

void TestStartedAntController::SetUp()
{
    TestPreparedAntController::SetUp();
    m_controller->start();
}

TEST_P(TestStartedAntController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedAntController : public TestPreparedAntController
{
protected:
    void SetUp() override;
};

void TestCompletedAntController::SetUp()
{
    TestPreparedAntController::SetUp();
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    m_controller->start();
    for (std::size_t batch = 0; batch < m_expected_batches; ++batch)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_P(TestCompletedAntController, rejectsSuspensionAfterCompletion)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Variants, TestAntController, Values(AntType::ONE, AntType::TWO),
    [](const TestParamInfo<AntType> &info) { return info.param == AntType::ONE ? "One" : "Two"; });
INSTANTIATE_TEST_SUITE_P(Variants, TestPreparedAntController, Values(AntType::ONE, AntType::TWO),
    [](const TestParamInfo<AntType> &info) { return info.param == AntType::ONE ? "One" : "Two"; });
INSTANTIATE_TEST_SUITE_P(Variants, TestStartedAntController, Values(AntType::ONE, AntType::TWO),
    [](const TestParamInfo<AntType> &info) { return info.param == AntType::ONE ? "One" : "Two"; });
INSTANTIATE_TEST_SUITE_P(Variants, TestCompletedAntController, Values(AntType::ONE, AntType::TWO),
    [](const TestParamInfo<AntType> &info) { return info.param == AntType::ONE ? "One" : "Two"; });

TEST_P(TestAntController, uiStepsTheSelectedObjectAtExistingBatchBoundaries)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    const help::HelpLabels previous_help{help::g_help_mode};
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, flush()).Times(2);
    EXPECT_CALL(m_driver, delay(0)).Times(2);
    {
        InSequence keys;
        EXPECT_CALL(m_driver, key_pressed())
            .WillOnce(Invoke(
                [&]
                {
                    EXPECT_EQ(expected[0], m_pixels);
                    EXPECT_EQ(help::HelpLabels::HELP_ANT_COMMANDS, help::g_help_mode);
                    return ID_KEY_SPACE;
                }));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_SPACE));
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key())
            .WillOnce(Invoke(
                [&]
                {
                    EXPECT_EQ(expected[1], m_pixels);
                    return ID_KEY_ENTER;
                }));
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key())
            .WillOnce(Invoke(
                [&]
                {
                    EXPECT_EQ(expected[2], m_pixels);
                    return ID_KEY_ENTER_2;
                }));
    }

    AntController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    EXPECT_EQ(0, controller.result());

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(expected.back(), m_pixels);
    EXPECT_EQ(previous_help, help::g_help_mode);
}

TEST_P(TestAntController, controllerStepsExistingBatchesAndRestoresHelpOnCompletion)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(help::HelpLabels::HELP_ANT_COMMANDS, help::g_help_mode);
    EXPECT_TRUE(m_columns.empty());
    for (const std::array<int, WIDTH *HEIGHT> &frame : expected)
    {
        ASSERT_FALSE(controller.done());
        controller.iterate();
        EXPECT_EQ(frame, m_pixels);
    }
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
    const std::vector<int> columns{m_columns};
    controller.iterate();
    EXPECT_EQ(columns, m_columns);
}

TEST_P(TestAntController, controllerRetainsSingleStepEnterKeypadEnterAndSpace)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    g_orbit_delay = 1;
    EXPECT_CALL(m_driver, flush()).WillRepeatedly(Return());
    EXPECT_CALL(m_driver, delay(0)).WillRepeatedly(Return());
    {
        InSequence keys;
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER_2));
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_SPACE));
    }
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    for (const std::array<int, WIDTH *HEIGHT> &frame : expected)
    {
        controller.iterate();
        EXPECT_EQ(frame, m_pixels);
    }

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerRetainsAdjustedDelayAcrossIterationsWithoutSingleStepping)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    const ValueSaver saved_first_init{g_first_init, false};
    const ValueSaver saved_real_palette{g_got_real_dac, false};
    g_orbit_delay = 2;
    std::vector<std::string> messages;
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, flush()).WillRepeatedly(Return());
    EXPECT_CALL(m_driver, delay(_)).WillRepeatedly(Return());
    EXPECT_CALL(m_driver, display_string(_, _, _, _, _))
        .WillRepeatedly(Invoke([&messages](int, int, int, int, const char *text) { messages.emplace_back(text); }));
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y)
            { return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT ? m_pixels.at(y * WIDTH + x) : 0; }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
                {
                    m_pixels.at(y * WIDTH + x) = color;
                }
            }));
    {
        InSequence keys;
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_UP_ARROW));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_UP_ARROW));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_CTL_UP_ARROW));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_UP_ARROW));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    }
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    controller.iterate();
    EXPECT_EQ(expected[0], m_pixels);
    EXPECT_FALSE(controller.done());
    controller.iterate();
    EXPECT_EQ(expected[1], m_pixels);
    EXPECT_FALSE(controller.done());

    EXPECT_THAT(messages, ElementsAre("Delay    2     ", "Delay   12     ", "Delay  112     ", "Delay  112     "));
    controller.suspend();
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerLeavesPeekedEscapeForTheOuterContext)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, unget_key(_)).Times(0);
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(expected.front(), m_pixels);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerRequeuesEscapeReadWhileSingleStepping)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    g_orbit_delay = 1;
    EXPECT_CALL(m_driver, flush()).WillOnce(Return());
    EXPECT_CALL(m_driver, delay(0)).WillOnce(Return());
    {
        InSequence keys;
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));
        EXPECT_CALL(m_driver, unget_key(ID_KEY_ESC)).WillOnce(Return());
    }
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(expected.front(), m_pixels);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerSuspensionTerminatesWorkAndRestoresHelp)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(0));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();
    controller.iterate();
    const std::vector<int> columns{m_columns};

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
    controller.iterate();
    EXPECT_EQ(columns, m_columns);
}

TEST_P(TestAntController, controllerPlotFailureTerminatesWorkAndRestoresHelp)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("Plot failed")));

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerInputFailureTerminatesWorkAndRestoresHelp)
{
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(std::runtime_error("Input failed")));
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerWithNoBatchesCompletesWithoutInputOrPixelOutput)
{
    g_params[1] = 0;
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};

    controller.start();
    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
}

TEST_P(TestAntController, controllerPreservesExistingImageLifecycle)
{
    m_pixels.fill(3);
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    m_pixels.fill(3);
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    alloc_resume(20, 1);
    put_resume(42);
    const std::vector<Byte> resume{g_resume_data};
    g_calc_time = 1234;
    g_timer_interval = 5678;
    ASSERT_NO_FATAL_FAILURE(prepare_existing_image());
    NonStandardFractal &calculation{*m_calculation};
    AntController controller{calculation};
    controller.start();

    for (const std::array<int, WIDTH *HEIGHT> &frame : expected)
    {
        controller.iterate();
        EXPECT_EQ(frame, m_pixels);
    }

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
    calculation.finish(false);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(1234, g_calc_time);
    EXPECT_EQ(5678, g_timer_interval);
    EXPECT_EQ(resume, g_resume_data);
}

TEST_P(TestAntController, controllerDestructionRestoresHelpWithoutFinishingTheCalculation)
{
    ASSERT_NO_FATAL_FAILURE(prepare_calculation());
    NonStandardFractal &calculation{*m_calculation};
    {
        AntController controller{calculation};
        controller.start();
        EXPECT_EQ(help::HelpLabels::HELP_ANT_COMMANDS, help::g_help_mode);
    }

    EXPECT_EQ(help::HelpLabels::NONE, help::g_help_mode);
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_columns.empty());
}

} // namespace id::test
