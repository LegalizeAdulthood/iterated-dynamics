// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/NonStandardFractal.h>

#include <MockDriver.h>
#include <ColorMapSaver.h>
#include <test_data.h>
#include <ui/TestRender.h>

#include <engine/calc_frac_init.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/bailout_formula.h>
#include <engine/engine_timer.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/pixel_grid.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <engine/trig_fns.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <engine/work_list.h>
#include <fractals/bif_may.h>
#include <fractals/fractalp.h>
#include <fractals/ifs.h>
#include <fractals/population.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <io/decoder.h>
#include <io/library.h>
#include <io/loadmap.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/AntController.h>
#include <ui/BifurcationController.h>
#include <ui/DiffusionController.h>
#include <ui/help.h>
#include <ui/id_keys.h>
#include <ui/KeyboardHandler.h>
#include <ui/IFS2DController.h>
#include <ui/PopcornController.h>
#include <ui/Standard4DController.h>
#include <ui/TestPointController.h>
#include <ui/video.h>

#include <helpdefs.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

int s_orbit_calls{};

int stable_population()
{
    ++s_orbit_calls;
    g_population = 0.5;
    return 0;
}

int unexpected_renderer()
{
    throw std::logic_error("Calculation entered a renderer callback");
}

int completed_orbit(double *, double *, double *)
{
    ++s_orbit_calls;
    return 1;
}

int completed_inverse_orbit()
{
    ++s_orbit_calls;
    g_new_z = {23.0, 45.0};
    return -1;
}

bool unexpected_setup()
{
    throw std::logic_error("Calculation entered legacy per-image setup");
}

struct BifurcationCase
{
    const char *name;
    FractalType type;
};

void PrintTo(const BifurcationCase &value, std::ostream *out)
{
    *out << value.name;
}

constexpr BifurcationCase BIFURCATION_CASES[]{
    {"Verhulst", FractalType::BIFURCATION},
    {"Lambda", FractalType::BIF_LAMBDA},
    {"PlusSinPi", FractalType::BIF_PLUS_SIN_PI},
    {"EqualsSinPi", FractalType::BIF_EQ_SIN_PI},
    {"Stewart", FractalType::BIF_STEWART},
    {"May", FractalType::BIF_MAY},
};

} // namespace

class TestNonStandardFractal : public Test
{
protected:
    void SetUp() override
    {
        s_orbit_calls = 0;
        select_type(FractalType::BIFURCATION);
        set_normal_dot();
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int x, int y, int color)
                {
                    m_pixels.at(y * WIDTH + x) = color;
                    m_columns.push_back(x);
                }));
    }

    void TearDown() override
    {
        end_resume();
        reset_calc_interrupted();
    }

    void select_type(FractalType type)
    {
        g_fractal_type = type;
        m_specific = *get_fractal_specific(type);
        m_specific.calc_type = unexpected_renderer;
        m_specific.per_image = unexpected_setup;
        m_specific.orbit_calc = stable_population;
        g_dispatch = FractalDispatch{m_specific};
    }

    void complete(NonStandardFractal &calculation)
    {
        for (int chunks = 0; chunks < WIDTH + 1 && !calculation.done(); ++chunks)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    static constexpr int WIDTH{4};
    static constexpr int HEIGHT{6};
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::BIFURCATION)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<LogicalScreen> m_saved_screen{
        g_logical_screen, LogicalScreen{WIDTH, HEIGHT, 0, 0, WIDTH - 1.0, HEIGHT - 1.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, WIDTH};
    ValueSaver<int> m_saved_height{g_screen_y_dots, HEIGHT};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{2.0, 0.0}, {4.0, 1.0}, {2.0, 0.0}}};
    ValueSaver<LDouble> m_saved_dx{g_delta_x, 2.0 / (WIDTH - 1)};
    ValueSaver<LDouble> m_saved_dy{g_delta_y, 1.0 / (HEIGHT - 1)};
    ValueSaver<double> m_saved_filter{g_params[0], 4.0};
    ValueSaver<double> m_saved_seed{g_params[1], 0.66};
    ValueSaver<double> m_saved_beta{g_params[2], 3.0};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 8};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<long> m_saved_time{g_calc_time, 0};
    ValueSaver<long> m_saved_timer_start{g_engine_timer_start};
    ValueSaver<long> m_saved_interval{g_timer_interval};
    std::array<int, WIDTH * HEIGHT> m_pixels{};
    std::vector<int> m_columns;
};

class TestEmptyNonStandardFractal : public TestNonStandardFractal
{
protected:
    void TearDown() override;

    NonStandardFractal m_calculation;
};

void TestEmptyNonStandardFractal::TearDown()
{
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
    TestNonStandardFractal::TearDown();
}

TEST_F(TestEmptyNonStandardFractal, initiallyDone)
{
    EXPECT_TRUE(m_calculation.done());
}

TEST_F(TestEmptyNonStandardFractal, selectionRequiresStart)
{
    EXPECT_THROW(m_calculation.select(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, viewCompletionRequiresSelection)
{
    EXPECT_THROW(m_calculation.view_done(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, viewAdvanceRequiresSelection)
{
    EXPECT_THROW(m_calculation.next_view(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, iterationRequiresSelection)
{
    EXPECT_THROW(m_calculation.iterate(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, preparationQueryRequiresSelection)
{
    EXPECT_THROW(m_calculation.preparing(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, sizingQueryRequiresSelection)
{
    EXPECT_THROW(m_calculation.sizing(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, palettePreparationRequiresSelection)
{
    EXPECT_THROW(m_calculation.prepare_palette(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, overlayNotificationRequiresSelection)
{
    EXPECT_THROW(m_calculation.consume_orbit_overlay_displayed(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, overlayScrubbingRequiresSelection)
{
    EXPECT_THROW(m_calculation.scrub_overlay(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, doesNotRequestPotentialStorage)
{
    EXPECT_FALSE(m_calculation.potential_storage_required());
}

TEST_F(TestEmptyNonStandardFractal, potentialStorageCompletionRequiresSelection)
{
    EXPECT_THROW(m_calculation.complete_potential_storage(false), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, doesNotRequestInverseJuliaQueue)
{
    EXPECT_FALSE(m_calculation.inverse_julia_queue_required());
}

TEST_F(TestEmptyNonStandardFractal, inverseJuliaQueueCompletionRequiresSelection)
{
    EXPECT_THROW(m_calculation.complete_inverse_julia_queue(false), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, batchNotificationRequiresSelection)
{
    EXPECT_THROW(m_calculation.consume_batch_complete(), std::logic_error);
}

TEST_F(TestEmptyNonStandardFractal, suspensionRequiresSelection)
{
    EXPECT_THROW(m_calculation.suspend(), std::logic_error);
}

class TestUnsupportedNonStandardFractal : public TestEmptyNonStandardFractal
{
protected:
    void SetUp() override;
};

void TestUnsupportedNonStandardFractal::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::MANDEL);
    ASSERT_TRUE(m_calculation.start().empty());
}

TEST_F(TestUnsupportedNonStandardFractal, selectionRejectsUnsupportedType)
{
    EXPECT_THROW(m_calculation.select(), std::logic_error);
}

TEST_F(TestUnsupportedNonStandardFractal, unsupportedTypeRemainsDone)
{
    EXPECT_TRUE(m_calculation.done());
}

TEST_F(TestUnsupportedNonStandardFractal, iterationRejectsUnsupportedType)
{
    EXPECT_THROW(m_calculation.iterate(), std::logic_error);
}

TEST_F(TestNonStandardFractal, activeOperationCannotBeReplaced)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    EXPECT_THROW(calculation.select(), std::logic_error);

    calculation.iterate();
    EXPECT_EQ(HEIGHT, m_columns.size());
    EXPECT_FALSE(calculation.done());
}

struct NonStandardTypeInput
{
    const char *name;
    FractalType type;
};

void PrintTo(const NonStandardTypeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestNonStandardOrbitSelection : public TestNonStandardFractal, public WithParamInterface<NonStandardTypeInput>
{
};

TEST_P(TestNonStandardOrbitSelection, selectionDoesNotInvokeRendererOrLegacySetup)
{
    const FractalType type{GetParam().type};
    select_type(type);
    EXPECT_EQ(nullptr, get_fractal_specific(type)->calc_type);
    NonStandardFractal calculation;
    calculation.start();

    EXPECT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
    calculation.suspend();
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    end_resume();
}

INSTANTIATE_TEST_SUITE_P(Types, TestNonStandardOrbitSelection,
    Values(NonStandardTypeInput{"Kam", FractalType::KAM}, NonStandardTypeInput{"Lorenz", FractalType::LORENZ},
        NonStandardTypeInput{"Henon", FractalType::HENON},
        NonStandardTypeInput{"Gingerbread", FractalType::GINGERBREAD},
        NonStandardTypeInput{"Hopalong", FractalType::HOPALONG}, NonStandardTypeInput{"Martin", FractalType::MARTIN},
        NonStandardTypeInput{"Icon", FractalType::ICON}, NonStandardTypeInput{"Chip", FractalType::CHIP},
        NonStandardTypeInput{"QuadrupTwo", FractalType::QUADRUP_TWO},
        NonStandardTypeInput{"Threeply", FractalType::THREEPLY}, NonStandardTypeInput{"Latoo", FractalType::LATOO}),
    [](const TestParamInfo<NonStandardTypeInput> &info) { return info.param.name; });

TEST_F(TestNonStandardFractal, selected2dOrbitCompletesOneIncrementWithoutCallingUi)
{
    select_type(FractalType::HENON);
    g_dispatch.set_orbit_calc(reinterpret_cast<OrbitCalc>(completed_orbit));
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ORBIT_2D, calculation.select());

    calculation.iterate();

    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.iterate();
    EXPECT_EQ(1, s_orbit_calls);
    calculation.finish(false);
}

class TestDynamicSelection : public TestNonStandardFractal, public WithParamInterface<NonStandardTypeInput>
{
};

TEST_P(TestDynamicSelection, selectedTypeHasResumableState)
{
    const FractalType type{GetParam().type};
    select_type(type);
    g_params[0] = -2.0;
    g_dispatch.set_orbit_calc(reinterpret_cast<OrbitCalc>(completed_orbit));
    EXPECT_EQ(nullptr, get_fractal_specific(type)->calc_type);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::DYNAMIC_2D, calculation.select());
    calculation.iterate();

    calculation.suspend();

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::DYNAMIC_2D, resumed.select());
    for (int increments = 0; increments < 100 && !resumed.done(); ++increments)
    {
        resumed.iterate();
    }
    EXPECT_TRUE(resumed.done());
    EXPECT_TRUE(resumed.completed());
    resumed.finish(false);
}

INSTANTIATE_TEST_SUITE_P(Types, TestDynamicSelection,
    Values(NonStandardTypeInput{"Dynamic", FractalType::DYNAMIC},
        NonStandardTypeInput{"MandelCloud", FractalType::MANDEL_CLOUD}),
    [](const TestParamInfo<NonStandardTypeInput> &info) { return info.param.name; });

class TestInverseJuliaSelection : public TestNonStandardFractal
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<Major> m_saved_major{g_major_method, Major::BREADTH_FIRST};
    NonStandardFractal m_calculation;
};

void TestInverseJuliaSelection::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::INVERSE_JULIA);
    g_dispatch.set_orbit_calc(completed_inverse_orbit);
    m_calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, m_calculation.select());
}

void TestInverseJuliaSelection::TearDown()
{
    m_calculation.finish(!m_calculation.completed());
    TestNonStandardFractal::TearDown();
}

TEST_F(TestInverseJuliaSelection, selectedTypeHasNoLegacyRenderer)
{
    EXPECT_EQ(nullptr, get_fractal_specific(FractalType::INVERSE_JULIA)->calc_type);
}

TEST_F(TestInverseJuliaSelection, selectionRequestsQueueBeforeOrbitWork)
{
    EXPECT_TRUE(m_calculation.inverse_julia_queue_required());
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_F(TestInverseJuliaSelection, iterationRequiresQueueDecision)
{
    EXPECT_THROW(m_calculation.iterate(), std::logic_error);
    EXPECT_EQ(0, s_orbit_calls);
}

class TestInverseJuliaFallback : public TestInverseJuliaSelection
{
protected:
    void SetUp() override;
};

void TestInverseJuliaFallback::SetUp()
{
    TestInverseJuliaSelection::SetUp();
    m_calculation.complete_inverse_julia_queue(false);
}

TEST_F(TestInverseJuliaFallback, queueRejectionSelectsRandomWalk)
{
    EXPECT_EQ(Major::RANDOM_WALK, g_major_method);
    EXPECT_FALSE(m_calculation.inverse_julia_queue_required());
}

TEST_F(TestInverseJuliaFallback, queueCompletionRejectsDuplicateDecision)
{
    EXPECT_THROW(m_calculation.complete_inverse_julia_queue(false), std::logic_error);
}

TEST_F(TestInverseJuliaFallback, fallbackCompletesAPointOnce)
{
    m_calculation.iterate();

    EXPECT_EQ(1, s_orbit_calls);
    EXPECT_EQ((DComplex{23.0, 45.0}), g_old_z);
    EXPECT_TRUE(m_calculation.completed());
    m_calculation.iterate();
    EXPECT_EQ(1, s_orbit_calls);
}

TEST_F(TestNonStandardFractal, randomWalkInverseJuliaSuspendsWithoutResumeData)
{
    ValueSaver saved_major{g_major_method, Major::RANDOM_WALK};
    select_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    EXPECT_FALSE(calculation.inverse_julia_queue_required());
    EXPECT_FALSE(calculation.done());

    calculation.suspend();

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestNonStandardFractal, inverseJuliaCannotResumeAnOldImage)
{
    select_type(FractalType::INVERSE_JULIA);
    g_calc_status = CalcStatus::RESUMABLE;
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());

    EXPECT_TRUE(calculation.done());
    EXPECT_FALSE(calculation.inverse_julia_queue_required());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    calculation.finish(false);
    EXPECT_FALSE(calculation.completed());
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_F(TestNonStandardFractal, selected4dOperationCompletesWithoutLegacyCallbacks)
{
    ValueSaver saved_colors{g_colors, 256};
    ValueSaver saved_bailout{g_bailout_float, +[] { return 1; }};
    select_type(FractalType::JULIBROT);
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::STANDARD_4D, calculation.select());

    EXPECT_EQ(nullptr, get_fractal_specific(FractalType::JULIBROT)->calc_type);
    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(m_columns.empty());
    for (int increments = 0; increments < WIDTH * HEIGHT && !calculation.done(); ++increments)
    {
        calculation.iterate();
    }
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(0, s_orbit_calls);
    calculation.finish(false);
}

TEST_F(TestNonStandardFractal, selected4dOperationSuspendsWithoutResumeData)
{
    ValueSaver saved_colors{g_colors, 256};
    select_type(FractalType::JULIBROT);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::STANDARD_4D, calculation.select());

    calculation.suspend();

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
}

class TestStandard4DController : public TestNonStandardFractal
{
protected:
    void prepare_controller();

    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(FractalType::JULIBROT);
        m_library = std::filesystem::path{data::ID_TEST_DATA_DIR} /
            ("selected-4d-" + std::string{UnitTest::GetInstance()->current_test_info()->name()});
        std::filesystem::create_directories(m_library / "map");
        io::clear_read_library_path();
        io::add_read_library(m_library);
    }

    void TearDown() override
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

    void write_palette(const char *name, const char *entry)
    {
        std::ofstream file{m_library / "map" / name};
        ASSERT_TRUE(file);
        for (int i = 0; i < 256; ++i)
        {
            file << entry << '\n';
        }
    }

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

TEST_F(TestNonStandardFractal, failedSelectedSetupDiscardsWorkWithoutCallingRenderer)
{
    select_type(FractalType::JULIBROT);
    EXPECT_THROW(
        {
            NonStandardFractal calculation;
            calculation.start();
            calculation.select();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

class TestMaySelection : public TestNonStandardFractal, public WithParamInterface<double>
{
protected:
    void SetUp() override;
};

void TestMaySelection::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::BIF_MAY);
}

TEST_P(TestMaySelection, selectionInitializesBetaWithoutLegacySetup)
{
    const double beta{GetParam()};
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_params[2] = beta;
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    const double exponent{std::max(beta, 2.0)};
    EXPECT_EQ(exponent, g_params[2]);
    g_rate = 1.25;
    g_population = 0.5;
    EXPECT_EQ(0, bifurc_may_orbit());
    EXPECT_NEAR(1.25 * 0.5 / std::pow(1.5, exponent), g_population, 1e-15);
}

INSTANTIATE_TEST_SUITE_P(Beta, TestMaySelection, Values(3.0, 1.0),
    [](const TestParamInfo<double> &info) { return info.param == 3.0 ? "Three" : "One"; });

TEST_F(TestNonStandardFractal, abandonedOperationDiscardsWorkWithoutChargingAnOldTimer)
{
    g_timer_interval = 12345;
    {
        NonStandardFractal calculation;
        calculation.start();
        ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());
    }

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0L, g_calc_time);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestNonStandardFractal, failedIncrementCleansUpInsteadOfEnteringUi)
{
    EXPECT_THROW(
        {
            NonStandardFractal calculation;
            calculation.start();
            ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());
            g_dispatch.set_orbit_calc([]() -> int { throw std::runtime_error("Orbit failed"); });
            calculation.iterate();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(m_columns.empty());
}

class TestSelectedIFS : public TestNonStandardFractal
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        g_max_iterations = 1;
        g_params[0] = 1;
        select_dimension(IFSDimension::TWO);
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels.at(y * WIDTH + x); }));
        std::string name{UnitTest::GetInstance()->current_test_info()->name()};
        std::replace(name.begin(), name.end(), '/', '_');
        m_path = std::filesystem::path{data::ID_TEST_DATA_DIR} / ("selected-ifs-" + name + ".ifs");
        g_ifs_filename = m_path;
        g_ifs_name = "entry";
    }

    void TearDown() override
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
        TestNonStandardFractal::TearDown();
    }

    void select_dimension(IFSDimension dimension)
    {
        select_type(FractalType::IFS);
        g_ifs_dim = dimension;
        g_num_affine_transforms = 1;
        g_ifs_definition = dimension == IFSDimension::TWO
            ? std::vector<float>{0, 0, 0, 0, 3, 0.5f, 1}
            : std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0.5f, 0, 1};
        g_calc_status = CalcStatus::PARAMS_CHANGED;
    }

    void complete_view(NonStandardFractal &calculation)
    {
        for (int point = 0; point < 1025; ++point)
        {
            calculation.iterate();
        }
        EXPECT_EQ(1025L, g_color_iter);
    }

    ValueSaver<std::filesystem::path> m_saved_filename{g_ifs_filename};
    ValueSaver<std::string> m_saved_name{g_ifs_name};
    ValueSaver<std::vector<float>> m_saved_definition{g_ifs_definition};
    ValueSaver<int> m_saved_count{g_num_affine_transforms};
    ValueSaver<IFSDimension> m_saved_dimension{g_ifs_dim};
    ValueSaver<int> m_saved_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag, true};
    ValueSaver<int> m_saved_orbit_save{g_orbit_save_flags, 0};
    ValueSaver<GlassesType> m_saved_glasses{g_glasses_type, GlassesType::NONE};
    ValueSaver<StereoImage> m_saved_eye{g_which_image, StereoImage::NONE};
    ValueSaver<Display3DMode> m_saved_display{g_display_3d, Display3DMode::NONE};
    ValueSaver<Viewport> m_saved_viewport{g_viewport};
    ValueSaver<WritePixel> m_saved_plot{g_plot};
    ValueSaver<WritePixel> m_saved_standard_plot{g_standard_plot};
    ValueSaver<int> m_saved_x_shift{g_x_shift};
    ValueSaver<int> m_saved_y_shift{g_y_shift};
    ValueSaver<int> m_saved_x_shift1{g_x_shift1};
    ValueSaver<int> m_saved_y_shift1{g_y_shift1};
    ValueSaver<int> m_saved_xx_adjust{g_xx_adjust};
    ValueSaver<int> m_saved_yy_adjust{g_yy_adjust};
    ValueSaver<int> m_saved_xx_adjust1{g_xx_adjust1};
    ValueSaver<int> m_saved_yy_adjust1{g_yy_adjust1};
    std::filesystem::path m_path;
};

struct IFSDimensionInput
{
    const char *name;
    IFSDimension dimension;
};

void PrintTo(const IFSDimensionInput &input, std::ostream *out)
{
    *out << input.name;
}

constexpr IFSDimensionInput IFS_DIMENSIONS[]{{"Two", IFSDimension::TWO}, {"Three", IFSDimension::THREE}};

class TestIFSDimensionSelection : public TestSelectedIFS, public WithParamInterface<IFSDimensionInput>
{
};

struct IFSCachedSelectionInput
{
    const char *name;
    IFSDimension dimension;
    FractalType type;
};

void PrintTo(const IFSCachedSelectionInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestIFSCachedSelection : public TestSelectedIFS, public WithParamInterface<IFSCachedSelectionInput>
{
};

TEST_P(TestIFSCachedSelection, cachedDefinitionSelectsDimensionsWithoutLegacyCallbacks)
{
    const IFSDimension dimension{GetParam().dimension};
    const FractalType type{GetParam().type};
    select_dimension(dimension);
    select_type(type);
    const ValueSaver saved_force_symmetry{g_force_symmetry, SymmetryType::X_AXIS};
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(
        dimension == IFSDimension::TWO ? NonStandardFractal::Operation::IFS_2D : NonStandardFractal::Operation::IFS_3D,
        calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_EQ(SymmetryType::NONE, g_symmetry);
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_EQ(type, g_fractal_type);
    calculation.suspend();
}

INSTANTIATE_TEST_SUITE_P(Definitions, TestIFSCachedSelection,
    Values(IFSCachedSelectionInput{"TwoRequestedTwo", IFSDimension::TWO, FractalType::IFS},
        IFSCachedSelectionInput{"TwoRequestedThree", IFSDimension::TWO, FractalType::IFS_3D},
        IFSCachedSelectionInput{"ThreeRequestedTwo", IFSDimension::THREE, FractalType::IFS},
        IFSCachedSelectionInput{"ThreeRequestedThree", IFSDimension::THREE, FractalType::IFS_3D}),
    [](const TestParamInfo<IFSCachedSelectionInput> &info) { return info.param.name; });

TEST_P(TestIFSDimensionSelection, loadingDefinitionSelectsItsHeaderDimensions)
{
    const IFSDimension dimension{GetParam().dimension};
    {
        std::ofstream file{m_path};
        ASSERT_TRUE(file);
        file << (dimension == IFSDimension::TWO ? "entry {\n0 0 0 0 3 0.5 1\n}\n"
                                                : "entry (3d) {\n0 0 0 0 0 0 0 0 0 3 0.5 0 1\n}\n");
    }
    g_ifs_definition.clear();
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    NonStandardFractal calculation;
    calculation.start();

    EXPECT_EQ(
        dimension == IFSDimension::TWO ? NonStandardFractal::Operation::IFS_2D : NonStandardFractal::Operation::IFS_3D,
        calculation.select());

    EXPECT_EQ(dimension, g_ifs_dim);
    EXPECT_EQ(1, g_num_affine_transforms);
    EXPECT_EQ(0L, g_color_iter);
    calculation.suspend();
}

TEST_F(TestSelectedIFS, missingDefinitionUnwindsWithoutRenderingOrPolling)
{
    g_ifs_definition.clear();

    EXPECT_THROW(
        {
            NonStandardFractal calculation;
            calculation.start();
            calculation.select();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_ifs_definition.empty());
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
}

TEST_P(TestIFSDimensionSelection, incrementalCalculationCompletesWithoutInput)
{
    const IFSDimension dimension{GetParam().dimension};
    select_dimension(dimension);
    NonStandardFractal calculation;
    calculation.start();
    calculation.select();

    calculation.iterate();
    EXPECT_EQ(1L, g_color_iter);
    EXPECT_FALSE(calculation.done());
    for (int point = 1; point < 1025; ++point)
    {
        calculation.iterate();
    }

    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.iterate();
    EXPECT_EQ(1025L, g_color_iter);
    calculation.finish(false);
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_P(TestIFSDimensionSelection, suspensionIsNonResumableAndTheNextSelectionRestarts)
{
    const IFSDimension dimension{GetParam().dimension};
    select_dimension(dimension);
    NonStandardFractal calculation;
    calculation.start();
    calculation.select();
    calculation.iterate();

    calculation.suspend();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    NonStandardFractal restarted;
    restarted.start();
    restarted.select();
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_FALSE(restarted.done());
}

class TestIFSController : public TestSelectedIFS
{
protected:
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

INSTANTIATE_TEST_SUITE_P(Dimensions, TestIFSDimensionSelection, ValuesIn(IFS_DIMENSIONS),
    [](const TestParamInfo<IFSDimensionInput> &info) { return info.param.name; });

class TestSuspendedIFSSelection : public TestIFSDimensionSelection
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<NonStandardFractal> m_calculation;
};

void TestSuspendedIFSSelection::SetUp()
{
    TestSelectedIFS::SetUp();
    select_dimension(GetParam().dimension);
    m_calculation.emplace();
    m_calculation->start();
    m_calculation->select();
    m_calculation->iterate();
    m_calculation->suspend();
    ASSERT_TRUE(m_calculation->done());
}

void TestSuspendedIFSSelection::TearDown()
{
    m_calculation.reset();
    TestSelectedIFS::TearDown();
}

TEST_P(TestSuspendedIFSSelection, suspensionRejectsReselection)
{
    EXPECT_THROW(m_calculation->select(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Dimensions, TestSuspendedIFSSelection, ValuesIn(IFS_DIMENSIONS),
    [](const TestParamInfo<IFSDimensionInput> &info) { return info.param.name; });

struct IFSGlassesInput
{
    const char *name;
    GlassesType glasses;
};

void PrintTo(const IFSGlassesInput &input, std::ostream *out)
{
    *out << input.name;
}

constexpr IFSGlassesInput SEPARATE_IFS_GLASSES[]{
    {"Photo", GlassesType::PHOTO}, {"StereoPair", GlassesType::STEREO_PAIR}};

constexpr IFSGlassesInput COMBINED_IFS_GLASSES[]{
    {"Alternating", GlassesType::ALTERNATING}, {"Superimposed", GlassesType::SUPERIMPOSE}};

class IFSStereoViewFixture : public TestSelectedIFS, public WithParamInterface<IFSGlassesInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<NonStandardFractal> m_calculation;
};

void IFSStereoViewFixture::SetUp()
{
    TestSelectedIFS::SetUp();
    select_dimension(IFSDimension::THREE);
    g_glasses_type = GetParam().glasses;
    m_calculation.emplace();
    m_calculation->start();
    ASSERT_EQ(NonStandardFractal::Operation::IFS_3D, m_calculation->select());
}

void IFSStereoViewFixture::TearDown()
{
    m_calculation->finish(!m_calculation->completed());
    m_calculation.reset();
    TestSelectedIFS::TearDown();
}

class TestSeparateIFSStereo : public IFSStereoViewFixture
{
};

TEST_P(TestSeparateIFSStereo, separateViewsWaitForExplicitAdvance)
{
    NonStandardFractal &calculation{*m_calculation};
    EXPECT_EQ(StereoImage::RED, g_which_image);
    EXPECT_FALSE(calculation.view_done());

    complete_view(calculation);

    EXPECT_TRUE(calculation.view_done());
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(CalcStatus::IN_PROGRESS, g_calc_status);
    calculation.next_view();
    EXPECT_EQ(StereoImage::BLUE, g_which_image);
    EXPECT_EQ(0L, g_color_iter);
    EXPECT_FALSE(calculation.view_done());
    complete_view(calculation);
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
}

TEST_P(TestSeparateIFSStereo, unfinishedViewRejectsAdvance)
{
    EXPECT_THROW(m_calculation->next_view(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Glasses, TestSeparateIFSStereo, ValuesIn(SEPARATE_IFS_GLASSES),
    [](const TestParamInfo<IFSGlassesInput> &info) { return info.param.name; });

class TestIFSStereoBoundary : public IFSStereoViewFixture
{
protected:
    void SetUp() override;
};

void TestIFSStereoBoundary::SetUp()
{
    IFSStereoViewFixture::SetUp();
    ASSERT_NO_FATAL_FAILURE(complete_view(*m_calculation));
    ASSERT_TRUE(m_calculation->view_done());
    ASSERT_FALSE(m_calculation->done());
}

TEST_P(TestIFSStereoBoundary, completedViewRejectsIteration)
{
    EXPECT_THROW(m_calculation->iterate(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Glasses, TestIFSStereoBoundary, ValuesIn(SEPARATE_IFS_GLASSES),
    [](const TestParamInfo<IFSGlassesInput> &info) { return info.param.name; });

class TestCompletedIFSStereo : public IFSStereoViewFixture
{
protected:
    void SetUp() override;
};

void TestCompletedIFSStereo::SetUp()
{
    IFSStereoViewFixture::SetUp();
    ASSERT_NO_FATAL_FAILURE(complete_view(*m_calculation));
    if (GetParam().glasses == GlassesType::PHOTO || GetParam().glasses == GlassesType::STEREO_PAIR)
    {
        m_calculation->next_view();
        ASSERT_NO_FATAL_FAILURE(complete_view(*m_calculation));
    }
    ASSERT_TRUE(m_calculation->done());
    ASSERT_TRUE(m_calculation->completed());
}

TEST_P(TestCompletedIFSStereo, completedCalculationRejectsViewAdvance)
{
    EXPECT_THROW(m_calculation->next_view(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Glasses, TestCompletedIFSStereo,
    Values(SEPARATE_IFS_GLASSES[0], SEPARATE_IFS_GLASSES[1], COMBINED_IFS_GLASSES[0], COMBINED_IFS_GLASSES[1]),
    [](const TestParamInfo<IFSGlassesInput> &info) { return info.param.name; });

class FullScreenIFSStereoFixture : public TestSelectedIFS
{
protected:
    void SetUp() override;
    void expect_full_screen();
};

void FullScreenIFSStereoFixture::SetUp()
{
    TestSelectedIFS::SetUp();
    select_dimension(IFSDimension::THREE);
    g_glasses_type = GlassesType::STEREO_PAIR;
    g_screen_x_dots = WIDTH * 2;
    g_logical_screen = {WIDTH, HEIGHT, 0, 0, WIDTH - 1.0, HEIGHT - 1.0};
    g_viewport.enabled = true;
}

void FullScreenIFSStereoFixture::expect_full_screen()
{
    EXPECT_EQ(WIDTH * 2, g_logical_screen.x_dots);
    EXPECT_EQ(HEIGHT, g_logical_screen.y_dots);
    EXPECT_EQ(0, g_logical_screen.x_offset);
    EXPECT_EQ(0, g_logical_screen.y_offset);
    EXPECT_FALSE(g_viewport.enabled);
}

class TestIFSStereoRestoration : public FullScreenIFSStereoFixture, public WithParamInterface<bool>
{
};

TEST_P(TestIFSStereoRestoration, terminationRestoresFullScreen)
{
    NonStandardFractal calculation;
    calculation.start();
    calculation.select();

    if (GetParam())
    {
        calculation.suspend();
    }
    else
    {
        complete_view(calculation);
        calculation.next_view();
        EXPECT_EQ(WIDTH, g_logical_screen.x_offset);
        complete_view(calculation);
        calculation.finish(false);
    }

    expect_full_screen();
}

INSTANTIATE_TEST_SUITE_P(Interrupted, TestIFSStereoRestoration, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "Suspended" : "Completed"; });

class TestIFSStereoFailure : public FullScreenIFSStereoFixture, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_row_count{io::g_row_count, 0};
};

void TestIFSStereoFailure::SetUp()
{
    FullScreenIFSStereoFixture::SetUp();
    g_image_region = ImageRegion{{-1.0, -1.0}, {1.0, 1.0}, {-1.0, -1.0}};
}

TEST_P(TestIFSStereoFailure, calculationFailureRestoresFullScreen)
{
    EXPECT_THROW(
        {
            NonStandardFractal calculation;
            calculation.start();
            calculation.select();
            if (GetParam())
            {
                complete_view(calculation);
                calculation.next_view();
            }
            EXPECT_CALL(m_driver, write_pixel(_, _, _))
                .WillOnce(Throw(std::runtime_error("Pixel output failed")))
                .RetiresOnSaturation();
            complete_view(calculation);
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    expect_full_screen();
}

INSTANTIATE_TEST_SUITE_P(SecondEye, TestIFSStereoFailure, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "SecondEye" : "FirstEye"; });

class TestStandaloneBifurcation : public TestNonStandardFractal, public WithParamInterface<BifurcationCase>
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(GetParam().type);
    }
};

TEST_P(TestStandaloneBifurcation, selectionDoesNoIterationAndOneIncrementComputesOneColumn)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(12, s_orbit_calls);
    EXPECT_THAT(m_columns, ElementsAre(0, 0, 0, 0, 0, 0));
}

TEST_P(TestStandaloneBifurcation, selectedOperationCompletesWithoutRendererCallbacks)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    complete(calculation);

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    const long completed_time{g_calc_time};
    calculation.finish(false);
    EXPECT_EQ(completed_time, g_calc_time);
}

TEST_P(TestStandaloneBifurcation, suspensionResumesAtTheNextUncomputedColumn)
{
    NonStandardFractal control;
    control.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, control.select());
    complete(control);
    const std::array<int, WIDTH * HEIGHT> expected{m_pixels};
    m_pixels.fill(0);
    m_columns.clear();
    s_orbit_calls = 0;
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());
    calculation.iterate();
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    const long suspended_time{g_calc_time};

    NonStandardFractal resumed;
    resumed.start();
    EXPECT_EQ(suspended_time, g_calc_time);
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, resumed.select());
    EXPECT_TRUE(g_resume_data.empty());
    complete(resumed);

    EXPECT_EQ(expected, m_pixels);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    for (int x = 0; x < WIDTH; ++x)
    {
        EXPECT_EQ(HEIGHT, std::count(m_columns.begin(), m_columns.end(), x));
    }
}

TEST_P(TestStandaloneBifurcation, tableHasNoRendererOrPerImageCallback)
{
    const FractalSpecific &specific{*get_fractal_specific(GetParam().type)};

    EXPECT_EQ(nullptr, specific.calc_type);
    EXPECT_EQ(nullptr, specific.per_image);
    EXPECT_NE(nullptr, specific.orbit_calc);
}

TEST_P(TestStandaloneBifurcation, uiEntryUsesSelectedOperationWithoutLegacyCallbacks)
{
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));

    EXPECT_EQ(0, render_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
}

TEST_P(TestStandaloneBifurcation, uiInterruptionLeavesMenuKeyPendingAndResumesAtNextColumn)
{
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
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
    EXPECT_EQ(WIDTH * HEIGHT, m_columns.size());
    for (int x = 0; x < WIDTH; ++x)
    {
        EXPECT_EQ(HEIGHT, std::count(m_columns.begin(), m_columns.end(), x));
    }
}

class TestBifurcationController : public TestStandaloneBifurcation
{
protected:
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
    EXPECT_EQ(12 * WIDTH, s_orbit_calls);
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

INSTANTIATE_TEST_SUITE_P(Bifurcations, TestStandaloneBifurcation, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) -> std::string { return info.param.name; });

class TestStandalonePopcorn : public TestNonStandardFractal, public WithParamInterface<FractalType>
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(GetParam());
        g_params[0] = 0.05;
        g_params[1] = 0;
        g_params[2] = 3;
        g_params[3] = 0;
        g_image_region = ImageRegion{{-0.5, -0.5}, {0.5, 0.5}, {-0.5, -0.5}};
        g_delta_x = 1.0 / (WIDTH - 1);
        g_delta_y = 1.0 / (HEIGHT - 1);
        alloc_pixel_grid();
        fill_pixel_grid();
        save_trig_functions();
        set_trig_array(0, "sin");
        set_trig_array(1, "tan");
        set_trig_array(2, "sin");
        set_trig_array(3, "tan");
    }

    void TearDown() override
    {
        restore_trig_functions();
        free_pixel_grid();
        TestNonStandardFractal::TearDown();
    }

    void complete(NonStandardFractal &calculation)
    {
        for (long chunks = 0; chunks <= WIDTH * HEIGHT * g_max_iterations && !calculation.done(); ++chunks)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    ValueSaver<double> m_saved_param_3{g_params[3]};
    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
    ValueSaver<LDouble> m_saved_dx2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_dy2{g_delta_y2, 0};
    ValueSaver<double> m_saved_mx1{g_plot_mx1, 0};
    ValueSaver<double> m_saved_mx2{g_plot_mx2, -(WIDTH - 1.0)};
    ValueSaver<double> m_saved_my1{g_plot_my1, 0};
    ValueSaver<double> m_saved_my2{g_plot_my2, HEIGHT - 1.0};
    ValueSaver<SymmetryType> m_saved_forced_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<FiniteAttractor> m_saved_attractor{g_attractor, FiniteAttractor{}};
    ValueSaver<Version> m_saved_version{g_version, current_id_version()};
};

TEST_P(TestStandalonePopcorn, selectionInitializesWithoutRenderingOrLegacyCallbacks)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::POPCORN, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(&g_param_z1, g_float_param);
    EXPECT_EQ(0, s_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
}

TEST_P(TestStandalonePopcorn, incrementalOperationCompletesWithoutInput)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::POPCORN, calculation.select());

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(1L, g_color_iter);
    complete(calculation);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_FALSE(m_columns.empty());
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_P(TestStandalonePopcorn, suspensionRestartsSavedRowWithSameFinalPixels)
{
    NonStandardFractal control;
    control.start();
    ASSERT_EQ(NonStandardFractal::Operation::POPCORN, control.select());
    complete(control);
    const std::array<int, WIDTH * HEIGHT> expected{m_pixels};
    m_pixels.fill(0);
    m_columns.clear();
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::POPCORN, calculation.select());
    for (long chunks = 0; chunks < (WIDTH + 1) * g_max_iterations; ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_EQ(1, g_row);
    ASSERT_EQ(1, g_col);
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());

    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::POPCORN, resumed.select());
    EXPECT_TRUE(g_resume_data.empty());
    resumed.iterate();
    EXPECT_EQ(1, g_row);
    EXPECT_EQ(0, g_col);
    complete(resumed);

    EXPECT_EQ(expected, m_pixels);
}

class TestPopcornController : public TestStandalonePopcorn
{
protected:
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

INSTANTIATE_TEST_SUITE_P(PopcornTypes, TestStandalonePopcorn, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info) -> std::string
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });

class TestStandaloneDiffusion : public TestNonStandardFractal, public WithParamInterface<int>
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(FractalType::DIFFUSION);
        g_params[0] = 10;
        g_params[1] = GetParam();
        g_params[2] = 3;
        g_logical_screen = LogicalScreen{SIDE, SIDE, 0, 0, SIDE - 1.0, SIDE - 1.0};
        g_screen_x_dots = SIDE;
        g_screen_y_dots = SIDE;
        g_put_color = put_color_a;
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int x, int y) { return m_frame.at(y * SIDE + x); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int x, int y, int color)
                {
                    if (x < SIDE && y < SIDE)
                    {
                        m_frame.at(y * SIDE + x) = color;
                    }
                }));
    }

    void complete(NonStandardFractal &calculation)
    {
        for (int chunks = 0; chunks < MAX_CHUNKS && !calculation.done(); ++chunks)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    static constexpr int SIDE{32};
    static constexpr int MAX_CHUNKS{100000};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag, true};
    ValueSaver<int> m_saved_random_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_start_show_orbit{g_start_show_orbit, false};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color};
    std::array<int, SIDE * SIDE> m_frame{};
};

TEST_P(TestStandaloneDiffusion, selectionSeedsImageWithoutLegacyCallbacksOrInput)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::DIFFUSION, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(std::any_of(m_frame.begin(), m_frame.end(), [](int color) { return color != 0; }));
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_P(TestStandaloneDiffusion, incrementsMatchDirectCalculationAndFinishOnce)
{
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

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::DIFFUSION, calculation.select());
    int actual_chunks{};
    while (actual_chunks < MAX_CHUNKS && !calculation.done())
    {
        calculation.iterate();
        ++actual_chunks;
    }

    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(expected_chunks, actual_chunks);
    EXPECT_EQ(expected, m_frame);
    EXPECT_EQ(expected_next_random, random15());
    calculation.finish(false);
    const long completed_time{g_calc_time};
    calculation.finish(false);
    EXPECT_EQ(completed_time, g_calc_time);
}

TEST_P(TestStandaloneDiffusion, suspensionRestoresTheExistingBoundsRecord)
{
    fractals::Diffusion reference;
    reference.suspend();
    const std::vector<Byte> initial_resume{g_resume_data};
    int chunks{};
    do
    {
        ASSERT_TRUE(reference.iterate());
        ++chunks;
        reference.suspend();
    } while (chunks < MAX_CHUNKS && g_resume_data == initial_resume);
    ASSERT_NE(initial_resume, g_resume_data);
    const std::vector<Byte> expected_resume{g_resume_data};
    end_resume();
    m_frame.fill(0);
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::DIFFUSION, calculation.select());
    for (int i = 0; i < chunks; ++i)
    {
        calculation.iterate();
    }
    ASSERT_FALSE(calculation.done());
    calculation.suspend();

    EXPECT_TRUE(calculation.done());
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(expected_resume, g_resume_data);

    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::DIFFUSION, resumed.select());
    EXPECT_TRUE(g_resume_data.empty());
    resumed.suspend();
    EXPECT_EQ(expected_resume, g_resume_data);

    NonStandardFractal continued;
    continued.start();
    ASSERT_EQ(NonStandardFractal::Operation::DIFFUSION, continued.select());
    complete(continued);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

class TestDiffusionController : public TestStandaloneDiffusion
{
protected:
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

INSTANTIATE_TEST_SUITE_P(DiffusionModes, TestStandaloneDiffusion, Values(0, 1, 2));

class TestStandaloneTestPoint : public TestNonStandardFractal, public WithParamInterface<CalcMode>
{
protected:
    void SetUp() override
    {
        TestNonStandardFractal::SetUp();
        select_type(FractalType::TEST);
        g_std_calc_mode = GetParam();
        g_params[0] = 0;
        g_params[1] = 0;
        g_params[2] = 0;
        g_image_region = ImageRegion{{-2.0, -1.5}, {1.0, 1.5}, {-2.0, -1.5}};
        g_delta_x = 3.0 / (WIDTH - 1);
        g_delta_y = 3.0 / (HEIGHT - 1);
        g_param_z1 = {};
        g_i_stop_pt = {WIDTH - 1, HEIGHT - 1};
        g_plot = put_color_a;
        alloc_pixel_grid();
        fill_pixel_grid();
    }

    void TearDown() override
    {
        free_pixel_grid();
        TestNonStandardFractal::TearDown();
    }

    void complete(NonStandardFractal &calculation)
    {
        for (int chunks = 0; chunks < MAX_CHUNKS && !calculation.done(); ++chunks)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_TRUE(calculation.completed());
        calculation.finish(false);
    }

    static constexpr int MAX_CHUNKS{2 * (WIDTH + 1) * (HEIGHT + 1)};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode};
    ValueSaver<LDouble> m_saved_dx2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_dy2{g_delta_y2, 0};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 7};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot};
};

TEST_P(TestStandaloneTestPoint, selectionStartsWithoutRenderingOrLegacyCallbacks)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::TEST_POINT, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ(0, s_orbit_calls);
}

TEST_P(TestStandaloneTestPoint, incrementsMatchDirectPixelTraversal)
{
    TestPoint reference;
    ASSERT_FALSE(reference.start());
    int expected_chunks{};
    while (expected_chunks < MAX_CHUNKS && !reference.done())
    {
        reference.iterate();
        ++expected_chunks;
    }
    ASSERT_TRUE(reference.done());
    reference.finish();
    const std::array<int, WIDTH * HEIGHT> expected_pixels{m_pixels};
    const std::vector<int> expected_columns{m_columns};
    m_pixels.fill(0);
    m_columns.clear();

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, calculation.select());
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(1, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(GetParam() == CalcMode::ONE_PASS ? 1U : 2U, m_columns.size());
    int actual_chunks{1};
    while (actual_chunks < MAX_CHUNKS && !calculation.done())
    {
        calculation.iterate();
        ++actual_chunks;
    }

    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(expected_chunks, actual_chunks);
    EXPECT_EQ(expected_pixels, m_pixels);
    EXPECT_EQ(expected_columns, m_columns);
    calculation.finish(false);
    const long completed_time{g_calc_time};
    calculation.finish(false);
    EXPECT_EQ(completed_time, g_calc_time);
}

TEST_P(TestStandaloneTestPoint, suspensionRestartsSavedRowAndPassWithSameFinalPixels)
{
    NonStandardFractal control;
    control.start();
    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, control.select());
    complete(control);
    const std::array<int, WIDTH * HEIGHT> expected{m_pixels};
    m_pixels.fill(0);
    m_columns.clear();
    g_calc_status = CalcStatus::PARAMS_CHANGED;

    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, calculation.select());
    const int row{GetParam() == CalcMode::ONE_PASS ? 2 : 1};
    for (int chunks = 0; chunks < MAX_CHUNKS && (g_row != row || g_col != 2); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_FALSE(calculation.done());
    ASSERT_EQ(row, g_row);
    ASSERT_EQ(2, g_col);
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_EQ(1, start_resume());
    int saved_row{};
    int saved_pass{};
    get_resume(saved_row, saved_pass);
    EXPECT_EQ(row, saved_row);
    EXPECT_EQ(GetParam() == CalcMode::ONE_PASS ? 0 : 1, saved_pass);

    NonStandardFractal resumed;
    resumed.start();
    ASSERT_EQ(NonStandardFractal::Operation::TEST_POINT, resumed.select());
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(row, g_row);
    EXPECT_EQ(0, g_col);
    complete(resumed);

    EXPECT_EQ(expected, m_pixels);
}

TEST_P(TestStandaloneTestPoint, uiChecksInputBeforeComputingFirstPixel)
{
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

TEST_P(TestStandaloneTestPoint, completedResumeDoesNotComputeAnotherPixel)
{
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

class TestSelectedTestPointController : public TestStandaloneTestPoint
{
protected:
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

INSTANTIATE_TEST_SUITE_P(TestPointModes, TestStandaloneTestPoint, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) -> std::string
    { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });

class SelectedAntFixture : public TestNonStandardFractal
{
protected:
    void initialize_ant(AntType type);

    std::vector<std::array<int, WIDTH * HEIGHT>> reference_frames()
    {
        std::vector<std::array<int, WIDTH * HEIGHT>> result;
        Ant ant;
        while (!ant.done())
        {
            ant.iterate();
            result.push_back(m_pixels);
        }
        m_pixels.fill(0);
        m_columns.clear();
        return result;
    }

    ValueSaver<double> m_saved_param3{g_params[3]};
    ValueSaver<double> m_saved_param4{g_params[4]};
    ValueSaver<double> m_saved_param5{g_params[5]};
    ValueSaver<std::string> m_saved_rule{g_param_text[0]};
    ValueSaver<int> m_saved_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag, true};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<help::HelpLabels> m_saved_help{help::g_help_mode, help::HelpLabels::NONE};
};

void SelectedAntFixture::initialize_ant(const AntType type)
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::ANT);
    g_params[0] = 1100;
    g_param_text[0] = "1100";
    g_params[1] = 400;
    g_params[2] = 2;
    g_params[3] = static_cast<int>(type);
    g_params[4] = 1;
    g_params[5] = 12345;
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels.at(y * WIDTH + x); }));
}

class TestSelectedAnt : public SelectedAntFixture, public WithParamInterface<AntType>
{
protected:
    void SetUp() override;
};

void TestSelectedAnt::SetUp()
{
    initialize_ant(GetParam());
}

class TestAntController : public TestSelectedAnt
{
protected:
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

TEST_P(TestSelectedAnt, selectionPreservesIterationBatchesWithoutRendererCallbacks)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    const int next_random{random15()};
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    ASSERT_EQ(NonStandardFractal::Operation::ANT, calculation.select());

    EXPECT_TRUE(m_columns.empty());
    EXPECT_FALSE(calculation.done());
    EXPECT_FALSE(calculation.consume_batch_complete());
    for (size_t batch = 0; batch < expected.size(); ++batch)
    {
        calculation.iterate();
        EXPECT_EQ(expected[batch], m_pixels);
        EXPECT_EQ((batch + 1) * (GetParam() == AntType::ONE ? 200 : 400), m_columns.size());
        EXPECT_EQ(batch + 1 == expected.size(), calculation.done());
        EXPECT_TRUE(calculation.consume_batch_complete());
        EXPECT_FALSE(calculation.consume_batch_complete());
    }
    EXPECT_EQ(next_random, random15());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    const size_t writes{m_columns.size()};
    calculation.iterate();
    EXPECT_EQ(writes, m_columns.size());
    EXPECT_FALSE(calculation.consume_batch_complete());
    calculation.finish(false);
}

TEST_P(TestSelectedAnt, interruptedAntRestartsInsteadOfSavingResumeState)
{
    const std::vector<std::array<int, WIDTH * HEIGHT>> expected{reference_frames()};
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::ANT, calculation.select());
    calculation.iterate();

    calculation.suspend();

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(g_resume_data.empty());
    m_pixels.fill(0);
    NonStandardFractal restarted;
    restarted.start();
    ASSERT_EQ(NonStandardFractal::Operation::ANT, restarted.select());
    complete(restarted);
    EXPECT_EQ(expected.back(), m_pixels);
}

TEST_P(TestSelectedAnt, drawingOnExistingImagePreservesImageLifecycle)
{
    alloc_resume(20, 1);
    put_resume(42);
    const std::vector<Byte> resume{g_resume_data};
    g_calc_time = 1234;
    g_timer_interval = 5678;
    m_pixels.fill(3);
    NonStandardFractal calculation;

    calculation.select_ant_on_image();

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(1234, g_calc_time);
    EXPECT_TRUE(m_columns.empty());
    for (int batch = 0; batch < 4; ++batch)
    {
        calculation.iterate();
        EXPECT_TRUE(calculation.consume_batch_complete());
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_FALSE(m_columns.empty());
    calculation.finish(false);

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(1234, g_calc_time);
    EXPECT_EQ(5678, g_timer_interval);
    EXPECT_EQ(resume, g_resume_data);
}

struct AntImageEntryInput
{
    const char *name;
    AntType type;
    bool selected;
};

void PrintTo(const AntImageEntryInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAntImageEntry : public SelectedAntFixture, public WithParamInterface<AntImageEntryInput>
{
protected:
    void SetUp() override;

    NonStandardFractal m_calculation;
};

void TestAntImageEntry::SetUp()
{
    initialize_ant(GetParam().type);
    m_calculation.start();
    if (GetParam().selected)
    {
        ASSERT_EQ(NonStandardFractal::Operation::ANT, m_calculation.select());
    }
}

TEST_P(TestAntImageEntry, existingImageEntryRejectsActiveImageCalculation)
{
    EXPECT_THROW(m_calculation.select_ant_on_image(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(States, TestAntImageEntry,
    Values(AntImageEntryInput{"OneUnselected", AntType::ONE, false},
        AntImageEntryInput{"OneSelected", AntType::ONE, true}, AntImageEntryInput{"TwoUnselected", AntType::TWO, false},
        AntImageEntryInput{"TwoSelected", AntType::TWO, true}),
    [](const TestParamInfo<AntImageEntryInput> &info) { return info.param.name; });

TEST_P(TestSelectedAnt, zeroBatchesCompletesWithoutInputOrPixelOutput)
{
    g_params[1] = 0;
    NonStandardFractal calculation;
    calculation.start();

    ASSERT_EQ(NonStandardFractal::Operation::ANT, calculation.select());

    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.iterate();
    EXPECT_FALSE(calculation.consume_batch_complete());
    EXPECT_TRUE(m_columns.empty());
}

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

INSTANTIATE_TEST_SUITE_P(AntVariants, TestSelectedAnt, Values(AntType::ONE, AntType::TWO));

} // namespace id::test
