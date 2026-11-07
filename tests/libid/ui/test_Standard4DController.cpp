// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/Standard4DController.h>

#include <ui/NonStandardFractalTest.h>

#include <ColorMapSaver.h>
#include <engine/bailout_formula.h>
#include <engine/color_state.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <fstream>
#include <io/library.h>
#include <io/loadmap.h>
#include <test_data.h>

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

class TestStandard4DController : public TestNonStandardFractal
{
protected:
    NonStandardControllerState m_controller_state;

    void prepare_controller();

    void SetUp() override;

    void TearDown() override;

    void write_palette(const char *name, const char *entry);

    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<SaveDAC> m_saved_palette{g_save_dac, SaveDAC::YES};
    ValueSaver<Julibrot3DMode> m_saved_mode{g_julibrot_3d_mode, Julibrot3DMode::MONOCULAR};
    ValueSaver<FractalType> m_saved_orbit_type{g_new_orbit_type, FractalType::JULIA};
    ValueSaver<int> m_saved_depth{g_julibrot_z_dots, 1};
    ValueSaver<decltype(g_bailout_float)> m_saved_bailout{g_bailout_float, +[] { return 0; }};
    ValueSaver<DComplex *> m_saved_parameter{g_float_param};
    ValueSaver<bool> m_saved_check_cur_dir{io::g_check_cur_dir, false};
    ValueSaver<bool> m_saved_true_color{g_is_true_color, false};
    ValueSaver<ColorState> m_saved_color_state{g_color_state};
    ValueSaver<std::string> m_saved_map_name{io::g_last_map_name};
    ColorMapSaver m_saved_color_map;
    std::filesystem::path m_library;
    std::optional<NonStandardFractal> m_calculation;
    std::optional<Standard4DController> m_controller;
};

void TestStandard4DController::prepare_controller()
{
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::STANDARD_4D, m_calculation->select());
    m_controller.emplace(*m_calculation);
}

class TestPreparedStandard4DController : public TestStandard4DController
{
protected:
    void SetUp() override;
};

void TestPreparedStandard4DController::SetUp()
{
    TestStandard4DController::SetUp();
    EXPECT_CALL(m_driver, write_palette()).Times(0);
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
}

void TestStandard4DController::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::JULIBROT);
    m_library = std::filesystem::path{data::ID_TEST_DATA_DIR} /
        ("selected-4d-" + std::string{UnitTest::GetInstance()->current_test_info()->name()});
    std::filesystem::create_directories(m_library / "map");
    io::clear_read_library_path();
    io::add_read_library(m_library);
}

void TestStandard4DController::TearDown()
{
    m_controller.reset();
    if (m_calculation)
    {
        m_calculation->finish(!m_calculation->completed());
    }
    m_calculation.reset();
    io::clear_read_library_path();
    std::error_code ignored;
    std::filesystem::remove_all(m_library, ignored);
    TestNonStandardFractal::TearDown();
}

void TestStandard4DController::write_palette(const char *name, const char *entry)
{
    std::ofstream file{m_library / "map" / name};
    ASSERT_TRUE(file);
    for (int i = 0; i < 256; ++i)
    {
        file << entry << '\n';
    }
}

TEST_F(TestPreparedStandard4DController, iterationRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);
}

TEST_F(TestPreparedStandard4DController, resultRequiresStart)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);
}

TEST_F(TestPreparedStandard4DController, suspensionRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

class TestStartedStandard4DController : public TestPreparedStandard4DController
{
protected:
    void SetUp() override;
};

void TestStartedStandard4DController::SetUp()
{
    TestPreparedStandard4DController::SetUp();
    m_controller->start();
}

TEST_F(TestStartedStandard4DController, rejectsDuplicateStart)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

class TestCompletedStandard4DController : public TestPreparedStandard4DController
{
protected:
    void SetUp() override;
};

void TestCompletedStandard4DController::SetUp()
{
    TestPreparedStandard4DController::SetUp();
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    m_controller->start();
    m_controller->iterate();
    for (int step = 0; step < WIDTH * HEIGHT && !m_controller->done(); ++step)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
}

TEST_F(TestCompletedStandard4DController, rejectsSuspensionAfterCompletion)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);
}

TEST_F(TestStandard4DController, stepsSymmetricPixelPairsUntilCompletion)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, write_palette()).Times(0);
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    Standard4DController &controller{*m_controller};

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_TRUE(m_columns.empty());
    controller.iterate();
    EXPECT_EQ(2U, m_columns.size());
    EXPECT_FALSE(controller.done());
    for (int step = 0; step < WIDTH * HEIGHT && !controller.done(); ++step)
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

TEST_F(TestStandard4DController, interruptsAfterComputingOneSymmetricPairAndKeepsCommandPending)
{
    EXPECT_CALL(m_driver, key_pressed())
        .WillOnce(Invoke(
            [this]
            {
                EXPECT_EQ(2U, m_columns.size());
                return 'x';
            }));
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    Standard4DController &controller{*m_controller};
    controller.start();

    controller.iterate();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestStandard4DController, explicitSuspensionIsNonResumable)
{
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    Standard4DController &controller{*m_controller};
    controller.start();

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(m_columns.empty());
}

TEST_F(TestStandard4DController, plotFailureTerminatesTheSelectedOperationBeforePolling)
{
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    Standard4DController &controller{*m_controller};
    controller.start();
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).WillOnce(Throw(std::runtime_error("Plot failed")));

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestStandard4DController, startLoadsAlternPaletteOnceAndHonorsNextTime)
{
    write_palette("altern.map", "10 20 30");
    g_save_dac = SaveDAC::NEXT_TIME;
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    EXPECT_CALL(m_driver, write_palette())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(10, g_dac_box[0][0]);
                EXPECT_EQ(20, g_dac_box[0][1]);
                EXPECT_EQ(30, g_dac_box[0][2]);
            }));
    Standard4DController &controller{*m_controller};

    controller.start();

    EXPECT_EQ(SaveDAC::YES, g_save_dac);
    EXPECT_EQ("altern.map", io::g_last_map_name);
    EXPECT_TRUE(m_columns.empty());
    controller.suspend();
}

TEST_F(TestStandard4DController, redBlueOverridesPreloadedPaletteWithGlassesMap)
{
    write_palette("glasses1.map", "11 21 31");
    g_julibrot_3d_mode = Julibrot3DMode::RED_BLUE;
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    EXPECT_CALL(m_driver, write_palette())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(11, g_dac_box[0][0]);
                EXPECT_EQ(21, g_dac_box[0][1]);
                EXPECT_EQ(31, g_dac_box[0][2]);
            }));
    Standard4DController &controller{*m_controller};

    controller.start();

    EXPECT_EQ(SaveDAC::NO, g_save_dac);
    EXPECT_EQ("glasses1.map", io::g_last_map_name);
    EXPECT_TRUE(m_columns.empty());
    controller.suspend();
}

TEST_F(TestStandard4DController, paletteInstallationFailureClosesWithoutComputingOrPolling)
{
    write_palette("altern.map", "10 20 30");
    g_save_dac = SaveDAC::NO;
    ASSERT_NO_FATAL_FAILURE(prepare_controller());
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_CALL(m_driver, write_palette()).WillOnce(Throw(std::runtime_error("Palette failed")));
    Standard4DController &controller{*m_controller};

    EXPECT_THROW(controller.start(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(m_columns.empty());
    controller.iterate();
}

} // namespace id::test
