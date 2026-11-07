// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/NonStandardFractal.h>

#include <NonStandardFractalTest.h>
#include <MockDriver.h>

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
using namespace testing;
using namespace id::test::non_standard;

namespace id::test
{

namespace
{

int completed_orbit(double *, double *, double *)
{
    ++g_orbit_calls;
    return 1;
}

int completed_inverse_orbit()
{
    ++g_orbit_calls;
    g_new_z = {23.0, 45.0};
    return -1;
}

} // namespace

class TestEmptyNonStandardFractal : public TestNonStandardFractal
{
protected:
    void TearDown() override;

    NonStandardFractal m_calculation;
};

void TestEmptyNonStandardFractal::TearDown()
{
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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

    EXPECT_EQ(1, g_orbit_calls);
    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.iterate();
    EXPECT_EQ(1, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
}

TEST_F(TestInverseJuliaSelection, iterationRequiresQueueDecision)
{
    EXPECT_THROW(m_calculation.iterate(), std::logic_error);
    EXPECT_EQ(0, g_orbit_calls);
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

    EXPECT_EQ(1, g_orbit_calls);
    EXPECT_EQ((DComplex{23.0, 45.0}), g_old_z);
    EXPECT_TRUE(m_calculation.completed());
    m_calculation.iterate();
    EXPECT_EQ(1, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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

TEST_P(TestStandaloneBifurcation, selectionDoesNoIterationAndOneIncrementComputesOneColumn)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_orbit_calls);
    EXPECT_TRUE(m_columns.empty());
    calculation.iterate();
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(12, g_orbit_calls);
    EXPECT_THAT(m_columns, ElementsAre(0, 0, 0, 0, 0, 0));
}

TEST_P(TestStandaloneBifurcation, selectedOperationCompletesWithoutRendererCallbacks)
{
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::BIFURCATION, calculation.select());

    complete(calculation);

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(12 * WIDTH, g_orbit_calls);
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
    g_orbit_calls = 0;
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
    EXPECT_EQ(12 * WIDTH, g_orbit_calls);
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

INSTANTIATE_TEST_SUITE_P(Bifurcations, TestStandaloneBifurcation, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) -> std::string { return info.param.name; });

TEST_P(TestStandalonePopcorn, selectionInitializesWithoutRenderingOrLegacyCallbacks)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::POPCORN, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(&g_param_z1, g_float_param);
    EXPECT_EQ(0, g_orbit_calls);
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
    EXPECT_EQ(0, g_orbit_calls);
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

INSTANTIATE_TEST_SUITE_P(PopcornTypes, TestStandalonePopcorn, Values(FractalType::POPCORN, FractalType::POPCORN_JUL),
    [](const TestParamInfo<FractalType> &info) -> std::string
    { return info.param == FractalType::POPCORN ? "Popcorn" : "PopcornJulia"; });

TEST_P(TestStandaloneDiffusion, selectionSeedsImageWithoutLegacyCallbacksOrInput)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::DIFFUSION, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_TRUE(std::any_of(m_frame.begin(), m_frame.end(), [](int color) { return color != 0; }));
    EXPECT_EQ(0, g_orbit_calls);
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

INSTANTIATE_TEST_SUITE_P(DiffusionModes, TestStandaloneDiffusion, Values(0, 1, 2));

TEST_P(TestStandaloneTestPoint, selectionStartsWithoutRenderingOrLegacyCallbacks)
{
    NonStandardFractal calculation;
    ASSERT_TRUE(calculation.start().empty());

    EXPECT_EQ(NonStandardFractal::Operation::TEST_POINT, calculation.select());

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_TRUE(m_columns.empty());
    EXPECT_EQ(0, g_orbit_calls);
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
    calculation.finish(false);
}

INSTANTIATE_TEST_SUITE_P(TestPointModes, TestStandaloneTestPoint, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) -> std::string
    { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });

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

INSTANTIATE_TEST_SUITE_P(AntVariants, TestSelectedAnt, Values(AntType::ONE, AntType::TWO));

} // namespace id::test
