// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/calcfrac.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/show_dot.h>
#include <engine/sound.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/fixed_pt.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>
#include <tuple>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{
namespace
{

int escaping_dem_orbit()
{
    g_new_z = DComplex{g_color_iter == 1 ? 4.0 : 32.0, 0.0};
    g_magnitude = g_new_z.x * g_new_z.x;
    return g_magnitude >= g_magnitude_limit;
}

class TestDistanceEstimator : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    FractalSpecific m_fractal{*get_fractal_specific(FractalType::MANDEL)};
    MockDriver m_driver;

private:
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<FractalSpecific *> m_saved_fractal{g_cur_fractal_specific, &m_fractal};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver<int> m_saved_screen_x{g_screen_x_dots, 2};
    ValueSaver<int> m_saved_screen_y{g_screen_y_dots, 2};
    ValueSaver<ImageRegion> m_saved_region{
        g_image_region, ImageRegion{DComplex{0.0, 0.0}, DComplex{1.0, 1.0}, DComplex{0.0, 0.0}}};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x, 1.0};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y, 1.0};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0.0};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0.0};
    ValueSaver<double> m_saved_delta_min{g_delta_min, 1.0};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<SymmetryType> m_saved_symmetry{g_force_symmetry, SymmetryType::NOT_FORCED};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 3};
    ValueSaver<long> m_saved_distance_estimator{g_distance_estimator, 1};
    ValueSaver<int> m_saved_dem_width{g_distance_estimator_width_factor, 1};
    ValueSaver<int> m_saved_dem_x{g_distance_estimator_x_dots, 0};
    ValueSaver<int> m_saved_dem_y{g_distance_estimator_y_dots, 0};
    ValueSaver<bool> m_saved_old_dem{g_use_old_distance_estimator};
    ValueSaver<bool> m_saved_old_dem_colors{g_old_demm_colors, false};
    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 0};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_and_color{g_and_color, 255};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<int> m_saved_keyboard_interval{g_max_keyboard_check_interval, 10000};
    ValueSaver<int> m_saved_show_dot{g_show_dot, -1};
    ValueSaver<int> m_saved_sound{g_sound_flag, 0};
    ValueSaver<int> m_saved_biomorph{g_biomorph, -1};
    ValueSaver<std::array<int, 2>> m_saved_decomp{g_decomp, std::array<int, 2>{}};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<InitOrbitMode> m_saved_init_mode{g_use_init_orbit, InitOrbitMode::VALUE};
    ValueSaver<DComplex> m_saved_init_orbit{g_init_orbit, DComplex{2.0, 0.0}};
    ValueSaver<bool> m_saved_start_show_orbit{g_start_show_orbit, false};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
};

void TestDistanceEstimator::SetUp()
{
    m_fractal.calc_type = standard_fractal_type;
    m_fractal.symmetry = SymmetryType::NONE;
    m_fractal.per_image = [] { return true; };
    m_fractal.per_pixel = []
    {
        g_old_z = g_init_orbit;
        g_new_z = DComplex{2.0, 0.0};
        return 0;
    };
    m_fractal.orbit_calc = escaping_dem_orbit;
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    set_normal_dot();
    alloc_pixel_grid();
    fill_pixel_grid();
}

void TestDistanceEstimator::TearDown()
{
    free_pixel_grid();
}

struct DemVersionCase
{
    Version version;
    bool old_dem;
};

void PrintTo(const DemVersionCase &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", old DEM: " << value.old_dem;
}

struct DemColorCase
{
    long mode;
    int old_color;
    int new_color;
};

void PrintTo(const DemColorCase &value, std::ostream *str)
{
    *str << "mode: " << value.mode << ", colors: " << value.old_color << ", " << value.new_color;
}

class TestDistanceEstimatorCompatibility : public TestDistanceEstimator,
                                           public WithParamInterface<std::tuple<DemVersionCase, DemColorCase, int>>
{
};

TEST_P(TestDistanceEstimatorCompatibility, selectsCompatibilityAfterEnablingDistanceEstimator)
{
    const auto &[version, mode, colors] = GetParam();
    g_version = version.version;
    g_colors = colors;
    g_and_color = colors - 1;
    g_use_old_distance_estimator = !version.old_dem;
    g_distance_estimator = 0;
    EXPECT_CALL(m_driver, write_pixel(_, _, 1)).Times(4).RetiresOnSaturation();
    ASSERT_EQ(0, calc_fract());

    g_distance_estimator = mode.mode;
    g_magnitude_limit = 4.0;
    const int expected_color = colors == 2 ? 1 : version.old_dem ? mode.old_color : mode.new_color;
    EXPECT_CALL(m_driver, write_pixel(_, _, expected_color)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(version.old_dem, g_use_old_distance_estimator);
    EXPECT_EQ(mode.mode == 1 && colors != 2 && !version.old_dem ? 1 : 2, g_real_color_iter);
    EXPECT_DOUBLE_EQ(mode.mode == 1 && colors != 2 && !version.old_dem ? 4.0 : 535.5, g_magnitude_limit);
    EXPECT_DOUBLE_EQ(mode.mode == 1 && colors != 2 ? 4.0 : 32.0, g_new_z.x);
}

// The scripted orbit escapes at z = 32 with derivative 41, giving
// distance 29.27 and distest=4 width 0.5: legacy color 7, current color 59.
INSTANTIATE_TEST_SUITE_P(ResetVersions, TestDistanceEstimatorCompatibility,
    Combine(Values(DemVersionCase{parse_legacy_version(1826), true}, DemVersionCase{parse_legacy_version(1827), false},
                DemVersionCase{current_id_version(), false}),
        Values(DemColorCase{-7, 2, 2}, DemColorCase{1, 1, 1}, DemColorCase{4, 7, 59}), Values(2, 256)));

struct DemIterationCase
{
    Version version;
    long iterations;
};

void PrintTo(const DemIterationCase &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", iterations: " << value.iterations;
}

class TestDistanceEstimatorDerivativeBound : public TestDistanceEstimator,
                                             public WithParamInterface<std::tuple<DemIterationCase, DComplex, long>>
{
};

TEST_P(TestDistanceEstimatorDerivativeBound, appliesBoundAtCompatibleRelease)
{
    const auto &[version, initial, mode] = GetParam();
    g_version = version.version;
    g_init_orbit = initial;
    g_distance_estimator = mode;
    m_fractal.orbit_calc = []
    {
        g_new_z = g_old_z;
        g_magnitude = g_new_z.x * g_new_z.x + g_new_z.y * g_new_z.y;
        return 0;
    };
    EXPECT_CALL(m_driver, write_pixel(_, _, 0)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(version.iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestDistanceEstimatorDerivativeBound,
    Combine(Values(DemIterationCase{parse_legacy_version(1826), 1}, DemIterationCase{parse_legacy_version(1950), 3},
                DemIterationCase{parse_legacy_version(1951), 1}, DemIterationCase{current_id_version(), 1}),
        Values(DComplex{1.0e8, 0.0}, DComplex{0.0, 1.0e8}, DComplex{1.0e308, 0.0}, DComplex{0.0, 1.0e308}),
        Values(1L, 4L)));

class TestDistanceEstimatorOverflow : public TestDistanceEstimator,
                                      public WithParamInterface<std::tuple<DemIterationCase, long, int>>
{
};

TEST_P(TestDistanceEstimatorOverflow, treatsOverflowAsEscapeAtCompatibleRelease)
{
    const auto &[version, mode, colors] = GetParam();
    g_version = version.version;
    g_distance_estimator = mode;
    g_colors = colors;
    g_and_color = colors - 1;
    m_fractal.orbit_calc = []
    {
        g_new_z = DComplex{2.0, 0.0};
        g_magnitude = 4.0;
        g_overflow = true;
        return 0;
    };
    const int expected_color = version.iterations == 3 || mode > 0 ? 0 : colors == 2 ? 1 : 7;
    EXPECT_CALL(m_driver, write_pixel(_, _, expected_color)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(version.iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestDistanceEstimatorOverflow,
    Combine(Values(DemIterationCase{parse_legacy_version(1826), 3}, DemIterationCase{parse_legacy_version(1827), 1},
                DemIterationCase{current_id_version(), 1}),
        Values(-7L, 1L, 4L), Values(2, 256)));

} // namespace
} // namespace id::test
