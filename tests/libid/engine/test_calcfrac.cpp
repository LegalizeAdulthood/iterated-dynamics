// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include <engine/calcfrac.h>

#include "MockDriver.h"

#include <engine/bailout_formula.h>
#include <engine/calc_frac_init.h>
#include <engine/calmanfp.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/show_dot.h>
#include <engine/sound.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/peterson_variations.h>
#include <io/gif_extensions.h>
#include <io/loadfile.h>
#include <math/fixed_pt.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <tuple>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
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

class TestStandardFractal : public Test
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
    ValueSaver<long> m_saved_distance_estimator{g_distance_estimator, 0};
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

void TestStandardFractal::SetUp()
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

void TestStandardFractal::TearDown()
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

class TestDistanceEstimatorCompatibility : public TestStandardFractal,
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

class TestDistanceEstimatorDerivativeBound : public TestStandardFractal,
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

class TestDistanceEstimatorOverflow : public TestStandardFractal,
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

constexpr double STAR_TRAIL_LIMIT{std::numeric_limits<float>::max()};

class TestStarTrail : public TestStandardFractal
{
protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_saved_sqr_y{g_temp_sqr_y};
    ValueSaver<double> m_saved_magnitude{g_magnitude};
};

void TestStarTrail::SetUp()
{
    TestStandardFractal::SetUp();
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_max_iterations = 32;
    m_fractal.per_pixel = []
    {
        g_old_z = DComplex{7.0, 11.0};
        g_temp_sqr_x = 11.0;
        g_temp_sqr_y = 13.0;
        g_magnitude = 17.0;
        return 0;
    };
}

struct StarTrailVersionCase
{
    Version version;
    bool updated;
};

void PrintTo(const StarTrailVersionCase &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", updated: " << value.updated;
}

class TestStarTrailIterationLimit : public TestStarTrail,
                                    public WithParamInterface<std::tuple<StarTrailVersionCase, long>>
{
};

TEST_P(TestStarTrailIterationLimit, usesCompatibleLimitAndRestoresUserLimit)
{
    const auto &[version, max_iterations] = GetParam();
    g_version = version.version;
    g_max_iterations = max_iterations;
    m_fractal.orbit_calc = []
    {
        g_new_z = DComplex{1.0, 0.0};
        return 1; // Star trails continue after the ordinary orbit bailout.
    };
    EXPECT_CALL(m_driver, write_pixel(_, _, 1)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(version.updated ? 16 : max_iterations, g_real_color_iter);
    EXPECT_EQ(max_iterations, g_max_iterations);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestStarTrailIterationLimit,
    Combine(
        Values(StarTrailVersionCase{parse_legacy_version(1824), false},
            StarTrailVersionCase{parse_legacy_version(1825), true}, StarTrailVersionCase{current_id_version(), true}),
        Values(8L, 16L, 32L)));

struct StarTrailOrbitCase
{
    DComplex value;
    DComplex clamped;
};

void PrintTo(const StarTrailOrbitCase &value, std::ostream *str)
{
    *str << '(' << value.value.x << ", " << value.value.y << ')';
}

class TestStarTrailOrbitState : public TestStarTrail,
                                public WithParamInterface<std::tuple<StarTrailVersionCase, StarTrailOrbitCase>>
{
};

TEST_P(TestStarTrailOrbitState, appliesCompatibleClampAndStateUpdates)
{
    const auto &[version, orbit] = GetParam();
    g_version = version.version;
    g_init_orbit = orbit.value;
    m_fractal.orbit_calc = []
    {
        g_new_z = g_init_orbit;
        return 0;
    };
    EXPECT_CALL(m_driver, write_pixel(_, _, 1)).Times(4);

    ASSERT_EQ(0, calc_fract());

    const DComplex expected_new = version.updated ? orbit.clamped : orbit.value;
    const DComplex expected_old = version.updated ? orbit.clamped : DComplex{7.0, 11.0};
    const double expected_sqr_x = version.updated ? orbit.clamped.x * orbit.clamped.x : 11.0;
    const double expected_sqr_y = version.updated ? orbit.clamped.y * orbit.clamped.y : 13.0;
    EXPECT_EQ(expected_new, g_new_z);
    EXPECT_EQ(expected_old, g_old_z);
    EXPECT_DOUBLE_EQ(expected_sqr_x, g_temp_sqr_x);
    EXPECT_DOUBLE_EQ(expected_sqr_y, g_temp_sqr_y);
    EXPECT_DOUBLE_EQ(version.updated ? expected_sqr_x + expected_sqr_y : 17.0, g_magnitude);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestStarTrailOrbitState,
    Combine(
        Values(StarTrailVersionCase{parse_legacy_version(1824), false},
            StarTrailVersionCase{parse_legacy_version(1825), true}, StarTrailVersionCase{current_id_version(), true}),
        Values(StarTrailOrbitCase{DComplex{2 * STAR_TRAIL_LIMIT, 4 * STAR_TRAIL_LIMIT},
                   DComplex{STAR_TRAIL_LIMIT, STAR_TRAIL_LIMIT}},
            StarTrailOrbitCase{
                DComplex{-2 * STAR_TRAIL_LIMIT, -4 * STAR_TRAIL_LIMIT}, DComplex{-STAR_TRAIL_LIMIT, -STAR_TRAIL_LIMIT}},
            StarTrailOrbitCase{DComplex{2.0, 3.0}, DComplex{2.0, 3.0}})));

class TestStarTrailColor : public TestStarTrail, public WithParamInterface<StarTrailVersionCase>
{
};

TEST_P(TestStarTrailColor, usesCompatibleOrbitSlopesForColor)
{
    const StarTrailVersionCase &version = GetParam();
    g_version = version.version;
    m_fractal.orbit_calc = []
    {
        if (g_color_iter == 1)
        {
            g_new_z = DComplex{2 * STAR_TRAIL_LIMIT, 4 * STAR_TRAIL_LIMIT};
        }
        else
        {
            g_new_z = DComplex{1.0, g_color_iter == 2 ? 1.0 : 3.0};
        }
        return 0;
    };
    // Clamping changes the first slope from 2 to 1, matching the second.
    const int expected_color = version.updated ? 1 : 0;
    EXPECT_CALL(m_driver, write_pixel(_, _, expected_color)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestStarTrailColor,
    Values(StarTrailVersionCase{parse_legacy_version(1824), false},
        StarTrailVersionCase{parse_legacy_version(1825), true}, StarTrailVersionCase{current_id_version(), true}));

struct DecompositionVersionCase
{
    Version version;
    bool legacy;
};

void PrintTo(const DecompositionVersionCase &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", legacy: " << value.legacy;
}

struct DecompositionColorCase
{
    DComplex point;
    int legacy_two_colors;
    int legacy_many_colors;
    int modern_two_colors;
    int modern_many_colors;
};

void PrintTo(const DecompositionColorCase &value, std::ostream *str)
{
    *str << '(' << value.point.x << ", " << value.point.y << ')';
}

class TestTwoWayDecomposition
    : public TestStandardFractal,
      public WithParamInterface<std::tuple<DecompositionVersionCase, DecompositionColorCase, int>>
{
};

TEST_P(TestTwoWayDecomposition, usesCompatibleQuadrantMappingAndPaletteOffset)
{
    const auto &[version, point, colors] = GetParam();
    g_version = version.version;
    g_init_orbit = point.point;
    g_decomp[0] = 2;
    g_colors = colors;
    g_and_color = colors - 1;
    m_fractal.orbit_calc = []
    {
        g_new_z = g_init_orbit;
        return 1;
    };
    const int expected_color = version.legacy ? colors == 2 ? point.legacy_two_colors : point.legacy_many_colors
        : colors == 2                         ? point.modern_two_colors
                                              : point.modern_many_colors;
    EXPECT_CALL(m_driver, write_pixel(_, _, expected_color)).Times(4);

    ASSERT_EQ(0, calc_fract());

    EXPECT_EQ(expected_color, g_color);
    EXPECT_EQ(1, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestTwoWayDecomposition,
    Combine(Values(DecompositionVersionCase{parse_legacy_version(1826), true},
                DecompositionVersionCase{parse_legacy_version(1827), false},
                DecompositionVersionCase{current_id_version(), false}),
        Values(DecompositionColorCase{DComplex{3.0, 4.0}, 0, 1, 1, 1},
            DecompositionColorCase{DComplex{-3.0, 4.0}, 1, 2, 1, 1},
            DecompositionColorCase{DComplex{-3.0, -4.0}, 0, 1, 0, 2},
            DecompositionColorCase{DComplex{3.0, -4.0}, 1, 2, 0, 2},
            DecompositionColorCase{DComplex{3.0, 0.0}, 0, 1, 1, 1},
            DecompositionColorCase{DComplex{-3.0, 0.0}, 1, 2, 1, 1},
            DecompositionColorCase{DComplex{0.0, 4.0}, 0, 1, 1, 1},
            DecompositionColorCase{DComplex{0.0, -4.0}, 1, 2, 0, 2},
            DecompositionColorCase{DComplex{0.0, 0.0}, 0, 1, 1, 1}),
        Values(2, 256)));

enum class ColorEngine
{
    STANDARD,
    FAST
};

void PrintTo(const ColorEngine value, std::ostream *str)
{
    *str << (value == ColorEngine::STANDARD ? "standard" : "fast");
}

class TestIterationColor : public TestStandardFractal
{
protected:
    void SetUp() override;
    int calculate(ColorEngine engine);

private:
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<DComplex> m_saved_param_z1{g_param_z1, DComplex{}};
    ValueSaver<FiniteAttractor> m_saved_attractor{g_attractor, FiniteAttractor{}};
    ValueSaver<int> m_saved_col{g_col, 0};
    ValueSaver<int> m_saved_row{g_row, 0};
    ValueSaver<int> m_saved_outside_color{g_outside_color};
    ValueSaver<int> m_saved_keyboard_interval{g_keyboard_check_interval, 1000000};
    ValueSaver<void (*)(int, int, int)> m_saved_plot{g_plot};
};

void TestIterationColor::SetUp()
{
    TestStandardFractal::SetUp();
    g_plot = [](int, int, int) {};
    g_image_region = ImageRegion{{2.0, 2.0}, {3.0, 3.0}, {2.0, 2.0}};
    fill_pixel_grid();
    g_outside_method = ColorMethod::COLOR;
    m_fractal.orbit_calc = []
    {
        g_new_z = DComplex{3.0, 4.0};
        return 1;
    };
}

int TestIterationColor::calculate(const ColorEngine engine)
{
    if (engine == ColorEngine::FAST)
    {
        calc_mandelbrot_init();
        return calc_mandelbrot_type();
    }
    return standard_fractal_type();
}

struct IterationColorVersion
{
    Version version;
    bool legacy;
};

void PrintTo(const IterationColorVersion &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", legacy: " << value.legacy;
}

struct IterationPalette
{
    int colors;
    int index;
};

void PrintTo(const IterationPalette &value, std::ostream *str)
{
    *str << value.colors << " colors";
}

struct IterationColorCase
{
    long value;
    std::array<int, 3> legacy;
    std::array<int, 3> modern;
};

void PrintTo(const IterationColorCase &value, std::ostream *str)
{
    *str << value.value;
}

static const IterationColorCase ITERATION_COLORS[]{
    {0, {0, 0, 0}, {0, 0, 0}},
    {7, {7, 7, 7}, {7, 7, 7}},
    {8, {0, 8, 8}, {0, 8, 8}},
    {16, {0, 1, 16}, {0, 1, 16}},
    {32766, {6, 6, 126}, {6, 6, 126}},
    {32767, {7, 7, 127}, {7, 7, 127}},
    {32768, {0, 8, 128}, {0, 8, 128}},
    {32769, {7, 7, 127}, {1, 9, 129}},
    {65534, {2, 2, 2}, {6, 14, 254}},
    {65535, {1, 1, 1}, {7, 15, 255}},
    {65536, {0, 0, 0}, {0, 1, 1}},
    {65537, {1, 1, 1}, {1, 2, 2}},
    {98304, {0, 8, 128}, {0, 9, 129}},
    {2147483647, {1, 1, 1}, {7, 7, 127}},
};

class TestIterationColorWrapping
    : public TestIterationColor,
      public WithParamInterface<std::tuple<IterationColorVersion, IterationColorCase, IterationPalette, ColorEngine>>
{
};

TEST_P(TestIterationColorWrapping, usesCompatibleColorWidthBeforePaletteWrapping)
{
    const auto &[version, color, palette, engine] = GetParam();
    g_version = version.version;
    g_outside_color = static_cast<int>(color.value);
    g_colors = palette.colors;
    g_and_color = palette.colors - 1;
    const int expected = (version.legacy ? color.legacy : color.modern)[palette.index];

    EXPECT_EQ(expected, calculate(engine));

    EXPECT_EQ(expected, g_color);
    EXPECT_EQ(color.value, g_color_iter);
    EXPECT_EQ(1, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestIterationColorWrapping,
    Combine(Values(IterationColorVersion{parse_legacy_version(1950), true},
                IterationColorVersion{parse_legacy_version(1951), false},
                IterationColorVersion{current_id_version(), false}),
        ValuesIn(ITERATION_COLORS), Values(IterationPalette{8, 0}, IterationPalette{16, 1}, IterationPalette{256, 2}),
        Values(ColorEngine::STANDARD, ColorEngine::FAST)));

class TestIterationColorIterations
    : public TestIterationColor,
      public WithParamInterface<std::tuple<IterationColorVersion, IterationColorCase, IterationPalette, ColorEngine>>
{
};

TEST_P(TestIterationColorIterations, wrapsActualInteriorIterationCounts)
{
    const auto &[version, color, palette, engine] = GetParam();
    g_version = version.version;
    g_max_iterations = color.value;
    g_inside_method = ColorMethod::ITER;
    g_colors = palette.colors;
    g_and_color = palette.colors - 1;
    g_image_region = ImageRegion{};
    fill_pixel_grid();
    m_fractal.orbit_calc = []
    {
        g_new_z = DComplex{};
        return 0;
    };
    const int expected = (version.legacy ? color.legacy : color.modern)[palette.index];

    EXPECT_EQ(expected, calculate(engine));

    EXPECT_EQ(color.value, g_color_iter);
    EXPECT_EQ(color.value, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestIterationColorIterations,
    Combine(Values(IterationColorVersion{parse_legacy_version(1950), true},
                IterationColorVersion{parse_legacy_version(1951), false},
                IterationColorVersion{current_id_version(), false}),
        Values(IterationColorCase{32768, {0, 8, 128}, {0, 8, 128}}, IterationColorCase{65535, {1, 1, 1}, {7, 15, 255}},
            IterationColorCase{65536, {0, 0, 0}, {0, 1, 1}}),
        Values(IterationPalette{8, 0}, IterationPalette{16, 1}, IterationPalette{256, 2}),
        Values(ColorEngine::STANDARD, ColorEngine::FAST)));

struct SignedIterationColor
{
    int value;
    int legacy;
    int modern;
};

void PrintTo(const SignedIterationColor &value, std::ostream *str)
{
    *str << value.value;
}

class TestIterationColorSigned
    : public TestIterationColor,
      public WithParamInterface<std::tuple<IterationColorVersion, SignedIterationColor, bool>>
{
};

TEST_P(TestIterationColorSigned, preservesDosSignedConversionAndAbsoluteValue)
{
    const auto &[version, color, boundary_trace] = GetParam();
    g_version = version.version;
    g_outside_color = color.value;
    g_std_calc_mode = boundary_trace ? CalcMode::BOUNDARY_TRACE : CalcMode::ONE_PASS;
    const int unadjusted = version.legacy ? color.legacy : color.modern;
    const int expected = boundary_trace && unadjusted <= 0 ? 1 : unadjusted;

    EXPECT_EQ(expected, calculate(ColorEngine::STANDARD));

    EXPECT_EQ(color.value, g_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestIterationColorSigned,
    Combine(Values(IterationColorVersion{parse_legacy_version(1950), true},
                IterationColorVersion{parse_legacy_version(1951), false},
                IterationColorVersion{current_id_version(), false}),
        Values(SignedIterationColor{-1, 1, 1}, SignedIterationColor{-32767, 32767, 32767},
            SignedIterationColor{-32768, -32768, 32768}, SignedIterationColor{-32769, 32767, 32769},
            SignedIterationColor{-65535, 1, 65535}, SignedIterationColor{-65536, 0, 65536},
            SignedIterationColor{-2147483647, 1, 2147483647}),
        Bool()));

class TestIterationColorBoundaryTrace
    : public TestIterationColor,
      public WithParamInterface<std::tuple<IterationColorVersion, IterationPalette, ColorEngine>>
{
};

TEST_P(TestIterationColorBoundaryTrace, repairsZeroAfterPaletteWrapping)
{
    const auto &[version, palette, engine] = GetParam();
    g_version = version.version;
    g_outside_color = 65536;
    g_colors = palette.colors;
    g_and_color = palette.colors - 1;
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;

    EXPECT_EQ(1, calculate(engine));

    EXPECT_EQ(65536, g_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestIterationColorBoundaryTrace,
    Combine(Values(IterationColorVersion{parse_legacy_version(1950), true},
                IterationColorVersion{parse_legacy_version(1951), false},
                IterationColorVersion{current_id_version(), false}),
        Values(IterationPalette{8, 0}, IterationPalette{16, 1}, IterationPalette{256, 2}),
        Values(ColorEngine::STANDARD, ColorEngine::FAST)));

enum class CompatibilityVersionSource
{
    PAR_RESET,
    GIF_METADATA
};

void PrintTo(const CompatibilityVersionSource value, std::ostream *str)
{
    *str << (value == CompatibilityVersionSource::PAR_RESET ? "PAR reset" : "GIF metadata");
}

CmdArgFlags load_compatibility_version(const Version &version, const CompatibilityVersionSource source)
{
    if (source == CompatibilityVersionSource::PAR_RESET)
    {
        return cmd_arg("reset=" + to_par_string(version), CmdFile::AT_AFTER_STARTUP);
    }
    FractalInfo info{};
    info.info_version = FRACTAL_INFO_VERSION;
    if (version.legacy)
    {
        info.release = static_cast<std::int16_t>(100 * version.major + version.minor);
    }
    else
    {
        info.version_major = version.major;
        info.version_minor = version.minor;
        info.version_patch = version.patch;
        info.version_tweak = version.tweak;
    }
    g_file_version = fractal_info_version(info);
    g_version = g_file_version;
    return CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET;
}

class TestOutsideColor : public TestIterationColor
{
private:
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<bool> m_saved_bad_outside{g_bad_outside};
    ValueSaver<bool> m_saved_bifurcation_functions{g_new_bifurcation_functions_loaded};
};

struct OutsideRoundingCase
{
    ColorMethod method;
    DComplex julia_seed;
    DComplex escaped;
    long iterations;
    long legacy;
    long modern;
};

void PrintTo(const OutsideRoundingCase &value, std::ostream *str)
{
    *str << +value.method << ", seed: " << value.julia_seed.x << ", " << value.julia_seed.y;
}

class TestOutsideRounding
    : public TestOutsideColor,
      public WithParamInterface<
          std::tuple<IterationColorVersion, OutsideRoundingCase, FractalType, ColorEngine, CompatibilityVersionSource>>
{
};

TEST_P(TestOutsideRounding, usesDosHalfUnitAdjustmentOnlyOnLegacyFastPath)
{
    const auto &[version, sample, type, engine, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    g_fractal_type = type;
    g_cur_fractal_specific = &m_fractal;
    g_outside_method = sample.method;
    g_inside_method = ColorMethod::COLOR;
    g_inside_color = 0;
    g_max_iterations = 64;
    g_magnitude_limit = sample.iterations == 2 ? 0.3 : 4.0;
    g_new_bifurcation_functions_loaded = true;
    g_bad_outside = !version.legacy;
    backwards_legacy_v20();
    ASSERT_EQ(version.version, g_version);
    ASSERT_EQ(version.legacy, g_bad_outside);

    // With c = -seed^2, Julia's preliminary step and Mandel's shifted
    // seed are both zero. Their first counted step is c.
    const DComplex c{sample.julia_seed.y * sample.julia_seed.y - sample.julia_seed.x * sample.julia_seed.x,
        -2.0 * sample.julia_seed.x * sample.julia_seed.y};
    const DComplex pixel = type == FractalType::JULIA ? sample.julia_seed : c;
    g_param_z1 = type == FractalType::JULIA ? c : DComplex{-c.x, -c.y};
    g_image_region = ImageRegion{
        DComplex{pixel.x, pixel.y - 1.0}, DComplex{pixel.x + 1.0, pixel.y}, DComplex{pixel.x, pixel.y - 1.0}};
    fill_pixel_grid();
    m_fractal.orbit_calc = []
    {
        const auto &sample = std::get<1>(GetParam());
        g_new_z = sample.escaped;
        return g_color_iter == sample.iterations ? 1 : 0;
    };
    const long expected = version.legacy && engine == ColorEngine::FAST ? sample.legacy : sample.modern;

    EXPECT_EQ(expected, calculate(engine));

    EXPECT_EQ(expected, g_color_iter);
    EXPECT_EQ(sample.iterations, g_real_color_iter);
    EXPECT_DOUBLE_EQ(sample.escaped.x, g_new_z.x);
    EXPECT_DOUBLE_EQ(sample.escaped.y, g_new_z.y);
}

// Golden colors follow DOS's trunc(value - 0.5), not round(value).
// C/Xfractint and newer DOS releases truncate value directly.
INSTANTIATE_TEST_SUITE_P(ResetVersions, TestOutsideRounding,
    Combine(Values(IterationColorVersion{parse_legacy_version(1960), true},
                IterationColorVersion{parse_legacy_version(1961), false},
                IterationColorVersion{current_id_version(), false}),
        Values(OutsideRoundingCase{ColorMethod::REAL, {-1.0, 1.5}, {1.25, 3.0}, 1, 8, 9},
            OutsideRoundingCase{ColorMethod::REAL, {1.0, 1.5}, {1.25, -3.0}, 1, 8, 9},
            OutsideRoundingCase{ColorMethod::REAL, {0.5, 1.5}, {2.0, -1.5}, 1, 9, 10},
            OutsideRoundingCase{ColorMethod::REAL, {1.75, 0.5}, {-2.8125, -1.75}, 1, 5, 6},
            OutsideRoundingCase{ColorMethod::REAL, {1.75, -0.5}, {-2.8125, 1.75}, 1, 5, 6},
            OutsideRoundingCase{ColorMethod::REAL, {1.5, -1.0}, {-1.25, 3.0}, 1, 7, 7},
            OutsideRoundingCase{ColorMethod::IMAG, {0.5, 1.5}, {2.0, -1.5}, 1, 6, 7},
            OutsideRoundingCase{ColorMethod::IMAG, {0.5, 2.5}, {6.0, -2.5}, 1, 5, 6},
            OutsideRoundingCase{ColorMethod::IMAG, {-1.0, 1.5}, {1.25, 3.0}, 1, 10, 11},
            OutsideRoundingCase{ColorMethod::IMAG, {-1.75, 0.5}, {-2.8125, 1.75}, 1, 9, 9},
            OutsideRoundingCase{ColorMethod::IMAG, {1.75, 0.5}, {-2.8125, -1.75}, 1, 6, 7},
            OutsideRoundingCase{ColorMethod::IMAG, {1.5, 1.0}, {-1.25, -3.0}, 1, 5, 5},
            OutsideRoundingCase{ColorMethod::MULT, {-0.5, 2.5}, {6.0, 2.5}, 1, 1, 2},
            OutsideRoundingCase{ColorMethod::MULT, {2.25, 0.5}, {-4.8125, -2.25}, 1, 1, 2},
            OutsideRoundingCase{ColorMethod::MULT, {0.0, 2.0}, {4.0, 0.0}, 1, 1, 1},
            OutsideRoundingCase{ColorMethod::MULT, {-0.25, 2.0}, {3.9375, 1.0}, 1, 3, 3},
            OutsideRoundingCase{ColorMethod::SUM, {-1.0, 1.5}, {1.25, 3.0}, 1, 4, 5},
            OutsideRoundingCase{ColorMethod::SUM, {0.0, 2.0}, {4.0, 0.0}, 1, 4, 5},
            OutsideRoundingCase{ColorMethod::SUM, {1.5, -1.0}, {-1.25, 3.0}, 1, 2, 2},
            OutsideRoundingCase{ColorMethod::SUM, {0.5, 1.5}, {2.0, -1.5}, 1, 1, 1},
            OutsideRoundingCase{ColorMethod::SUM, {0.5, 0.5}, {-0.25, -0.5}, 2, 1, 2}),
        Values(FractalType::MANDEL, FractalType::JULIA), Values(ColorEngine::STANDARD, ColorEngine::FAST),
        Values(CompatibilityVersionSource::PAR_RESET, CompatibilityVersionSource::GIF_METADATA)));

struct OutsideClampCase
{
    ColorMethod method;
    DComplex julia_seed;
    long max_iterations;
    long legacy_standard;
    long legacy_fast;
    long modern;
    int outside_color{};
};

void PrintTo(const OutsideClampCase &value, std::ostream *str)
{
    *str << +value.method << ", seed: " << value.julia_seed.x << ", " << value.julia_seed.y
         << ", maxit: " << value.max_iterations << ", outside color: " << value.outside_color;
}

class TestOutsideClamping
    : public TestOutsideColor,
      public WithParamInterface<
          std::tuple<IterationColorVersion, OutsideClampCase, FractalType, ColorEngine, CompatibilityVersionSource>>
{
private:
    ValueSaver<int> m_saved_atan_colors{g_atan_colors, 180};
};

TEST_P(TestOutsideClamping, clampsSpecialColorsAtCompatibleRelease)
{
    const auto &[version, sample, type, engine, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    g_fractal_type = type;
    g_cur_fractal_specific = &m_fractal;
    g_outside_method = sample.method;
    g_outside_color = sample.outside_color;
    g_inside_method = ColorMethod::COLOR;
    g_inside_color = 0;
    g_max_iterations = sample.max_iterations;
    g_magnitude_limit = 0.01;
    g_new_bifurcation_functions_loaded = true;
    backwards_legacy_v20();
    ASSERT_EQ(version.version, g_version);

    const DComplex c{sample.julia_seed.y * sample.julia_seed.y - sample.julia_seed.x * sample.julia_seed.x,
        -2.0 * sample.julia_seed.x * sample.julia_seed.y};
    const DComplex pixel = type == FractalType::JULIA ? sample.julia_seed : c;
    g_param_z1 = type == FractalType::JULIA ? c : DComplex{-c.x, -c.y};
    g_image_region = ImageRegion{
        DComplex{pixel.x, pixel.y - 1.0}, DComplex{pixel.x + 1.0, pixel.y}, DComplex{pixel.x, pixel.y - 1.0}};
    fill_pixel_grid();
    m_fractal.orbit_calc = []
    {
        const DComplex seed = std::get<1>(GetParam()).julia_seed;
        g_new_z = DComplex{seed.y * seed.y - seed.x * seed.x, -2.0 * seed.x * seed.y};
        return g_color_iter == 1 ? 1 : 0;
    };
    const long legacy = engine == ColorEngine::FAST ? sample.legacy_fast : sample.legacy_standard;
    const long expected = version.legacy ? legacy : sample.modern;

    EXPECT_EQ(expected, calculate(engine));

    EXPECT_EQ(expected, g_color);
    EXPECT_EQ(expected, g_color_iter);
    EXPECT_EQ(1, g_real_color_iter);
}

// Use the float-only C reference's signed bounds check. DOS's wordwise
// comparison differs for large colors, such as the seed (0, 256).
// Separate old fast colors account for DOS's half-unit adjustment.
INSTANTIATE_TEST_SUITE_P(ResetVersions, TestOutsideClamping,
    Combine(Values(IterationColorVersion{parse_legacy_version(1960), true},
                IterationColorVersion{parse_legacy_version(1961), false},
                IterationColorVersion{current_id_version(), false}),
        Values(OutsideClampCase{ColorMethod::REAL, {3.0, 1.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::REAL, {3.25, 1.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::REAL, {0.5, 1.5}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::REAL, {4.0, 3.0}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::REAL, {1.0, 1.0}, 8, 8, 8, 8},
            OutsideClampCase{ColorMethod::REAL, {0.0, 256.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::IMAG, {2.0, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::IMAG, {1.5, 3.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::IMAG, {-2.0, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::IMAG, {1.0, 3.5}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::IMAG, {0.0, 2.0}, 8, 8, 8, 8},
            OutsideClampCase{ColorMethod::MULT, {2.0, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::MULT, {0.5, 2.5}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::MULT, {-0.0625, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::MULT, {-0.5, 2.0}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::MULT, {0.0, 3.0}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::SUM, {1.0, 0.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::SUM, {2.0, 0.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::SUM, {0.0, 3.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::SUM, {-0.5, 0.5}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::SUM, {0.0, 2.0}, 4, 0, 4, 1},
            OutsideClampCase{ColorMethod::SUM, {0.5, 0.5}, 8, 1, 0, 1},
            OutsideClampCase{ColorMethod::ATAN, {0.0, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::ATAN, {2.0, 2.0}, 8, 0, 0, 1},
            OutsideClampCase{ColorMethod::ITER, {0.0, 2.0}, 8, 1, 1, 1},
            OutsideClampCase{ColorMethod::COLOR, {0.0, 2.0}, 8, 0, 0, 0, 0},
            OutsideClampCase{ColorMethod::COLOR, {0.0, 2.0}, 8, 16, 16, 16, 16}),
        Values(FractalType::MANDEL, FractalType::JULIA), Values(ColorEngine::STANDARD, ColorEngine::FAST),
        Values(CompatibilityVersionSource::PAR_RESET, CompatibilityVersionSource::GIF_METADATA)));

class TestOutsideClampBoundaryTrace : public TestOutsideColor,
                                      public WithParamInterface<std::tuple<IterationColorVersion, ColorEngine>>
{
};

TEST_P(TestOutsideClampBoundaryTrace, repairsLegacyZeroOnlyAfterClamping)
{
    const auto &[version, engine] = GetParam();
    g_version = version.version;
    g_outside_method = ColorMethod::ATAN;
    g_max_iterations = 8;
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    g_image_region = ImageRegion{{2.0, -1.0}, {3.0, 0.0}, {2.0, -1.0}};
    fill_pixel_grid();
    m_fractal.orbit_calc = []
    {
        g_new_z = DComplex{6.0, 0.0};
        return 1;
    };

    EXPECT_EQ(1, calculate(engine));

    EXPECT_EQ(version.legacy ? 0 : 1, g_color_iter);
    EXPECT_EQ(1, g_color);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestOutsideClampBoundaryTrace,
    Combine(Values(IterationColorVersion{parse_legacy_version(1960), true},
                IterationColorVersion{parse_legacy_version(1961), false},
                IterationColorVersion{current_id_version(), false}),
        Values(ColorEngine::STANDARD, ColorEngine::FAST)));

enum class FmodColoring
{
    INSIDE,
    OUTSIDE,
    BOTH
};

void PrintTo(const FmodColoring value, std::ostream *str)
{
    *str << (value == FmodColoring::INSIDE ? "inside" : value == FmodColoring::OUTSIDE ? "outside" : "both");
}

struct FmodMetric
{
    Bailout type;
    const char *name;
};

void PrintTo(const FmodMetric &value, std::ostream *str)
{
    *str << value.name;
}

struct FmodOrbitCase
{
    DComplex value;
    std::array<int, 7> colors;
};

void PrintTo(const FmodOrbitCase &value, std::ostream *str)
{
    *str << value.value.x << ", " << value.value.y;
}

struct FmodMagnitudeCase
{
    double cached;
    bool calculate;
    int modulus_color;
};

void PrintTo(const FmodMagnitudeCase &value, std::ostream *str)
{
    *str << "cached: " << value.cached << ", recalculate: " << value.calculate;
}

class TestFmodColor : public TestIterationColor,
                      public WithParamInterface<std::tuple<IterationColorVersion, FmodMetric, FmodOrbitCase,
                          FmodMagnitudeCase, FmodColoring, CompatibilityVersionSource>>
{
private:
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<Bailout> m_saved_bailout{g_bailout_test};
    ValueSaver<bool> m_saved_magnitude_calc{g_magnitude_calc};
    ValueSaver<double> m_saved_magnitude{g_magnitude};
    ValueSaver<double> m_saved_close_proximity{g_close_proximity};
};

TEST_P(TestFmodColor, usesCompatibleMetricAndMagnitudeCache)
{
    const auto &[version, metric, orbit, magnitude, coloring, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    g_cur_fractal_specific = &m_fractal;
    g_inside_method = coloring == FmodColoring::OUTSIDE ? ColorMethod::COLOR : ColorMethod::FMODI;
    g_inside_color = 0;
    g_outside_method = coloring == FmodColoring::INSIDE ? ColorMethod::ITER : ColorMethod::FMOD;
    g_max_iterations = 3;
    g_bailout_test = metric.type;
    g_magnitude_calc = magnitude.calculate;
    g_close_proximity = 1.0;
    m_fractal.orbit_calc = []
    {
        const auto &[version, metric, orbit, magnitude, coloring, source] = GetParam();
        g_new_z = orbit.value;
        g_magnitude = magnitude.cached;
        return coloring != FmodColoring::INSIDE && g_color_iter == 2 ? 1 : 0;
    };
    const int selected_color =
        metric.type == Bailout::MOD ? magnitude.modulus_color : orbit.colors[static_cast<int>(metric.type)];
    const int expected = version.legacy && coloring != FmodColoring::OUTSIDE ? magnitude.modulus_color : selected_color;

    EXPECT_EQ(expected, calculate(ColorEngine::STANDARD));

    EXPECT_EQ(expected, g_color);
    EXPECT_EQ(expected, g_color_iter);
    EXPECT_EQ(coloring == FmodColoring::INSIDE ? 3 : 2, g_real_color_iter);
    EXPECT_DOUBLE_EQ(orbit.value.x, g_new_z.x);
    EXPECT_DOUBLE_EQ(orbit.value.y, g_new_z.y);
    EXPECT_DOUBLE_EQ(magnitude.cached, g_magnitude);
}

// At proximity 1 and 256 colors, squared modulus 0.3125 gives color 80.
// Columns follow MOD, REAL, IMAG, OR, AND, MANH, MANR. A retained cache
// of 0.125 gives color 32; a cache at the threshold leaves color zero.
INSTANTIATE_TEST_SUITE_P(ResetVersions, TestFmodColor,
    Combine(Values(IterationColorVersion{parse_legacy_version(2000), true},
                IterationColorVersion{parse_legacy_version(2001), false},
                IterationColorVersion{current_id_version(), false}),
        Values(FmodMetric{Bailout::MOD, "mod"}, FmodMetric{Bailout::REAL, "real"}, FmodMetric{Bailout::IMAG, "imag"},
            FmodMetric{Bailout::OR, "or"}, FmodMetric{Bailout::AND, "and"}, FmodMetric{Bailout::MANH, "manh"},
            FmodMetric{Bailout::MANR, "manr"}),
        Values(FmodOrbitCase{{0.25, -0.5}, {80, 16, 64, 64, 80, 144, 16}},
            FmodOrbitCase{{-0.5, 0.25}, {80, 64, 16, 64, 80, 144, 16}}),
        Values(FmodMagnitudeCase{0.125, false, 32}, FmodMagnitudeCase{0.125, true, 80},
            FmodMagnitudeCase{0.0, false, 80}, FmodMagnitudeCase{1.0, false, 0}),
        Values(FmodColoring::INSIDE, FmodColoring::OUTSIDE, FmodColoring::BOTH),
        Values(CompatibilityVersionSource::PAR_RESET, CompatibilityVersionSource::GIF_METADATA)));

struct PixelInitializerCase
{
    FractalType type;
    bool adds_parameter;
    bool caches_squares;
    int iterations;
};

void PrintTo(const PixelInitializerCase &value, std::ostream *str)
{
    *str << get_fractal_specific(value.type)->name;
}

const PixelInitializerCase PIXEL_INITIALIZERS[]{
    {FractalType::MANDEL, true, true, 1},
    {FractalType::MANDEL_Z_POWER, true, false, 1},
    {FractalType::MARKS_MANDEL, true, true, 1},
    {FractalType::COMPLEX_MARKS_MAND, true, true, 1},
    {FractalType::HALLEY, false, false, 0},
    {FractalType::MAND_PHOENIX, true, true, 1},
};

class TestPixelInitializer
    : public TestOutsideColor,
      public WithParamInterface<std::tuple<IterationColorVersion, PixelInitializerCase, CompatibilityVersionSource>>
{
protected:
    void configure(FractalType type);
    void expect_seed(const PixelInitializerCase &initializer, DComplex pixel);

private:
    ValueSaver<DComplex> m_saved_init{g_init};
    ValueSaver<DComplex> m_saved_old{g_old_z};
    ValueSaver<DComplex> m_saved_new{g_new_z};
    ValueSaver<DComplex> m_saved_tmp{g_tmp_z};
    ValueSaver<double> m_saved_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_saved_sqr_y{g_temp_sqr_y};
    ValueSaver<int> m_saved_exponent{g_c_exponent, 2};
    ValueSaver<DComplex> m_saved_power{g_power_z, DComplex{2.0, 0.0}};
    ValueSaver<DComplex> m_saved_coefficient{g_marks_coefficient};
};

void TestPixelInitializer::configure(const FractalType type)
{
    g_fractal_type = type;
    g_cur_fractal_specific = &m_fractal;
    m_fractal.per_pixel = get_fractal_specific(type)->per_pixel;
    g_image_region = ImageRegion{{2.0, 2.0}, {3.0, 3.0}, {2.0, 2.0}};
    fill_pixel_grid();
    g_inversion = Inversion{};
    g_param_z1 = {0.125, -0.25};
    g_use_init_orbit = InitOrbitMode::NORMAL;
    g_row = 0;
    g_col = 0;
    // The orbit viewer supplies an exact coordinate before per_pixel().
    g_init = {2.125, 2.75};
}

void TestPixelInitializer::expect_seed(const PixelInitializerCase &initializer, const DComplex pixel)
{
    const DComplex seed = initializer.adds_parameter ? pixel + g_param_z1 : pixel;
    EXPECT_DOUBLE_EQ(pixel.x, g_init.x);
    EXPECT_DOUBLE_EQ(pixel.y, g_init.y);
    EXPECT_DOUBLE_EQ(seed.x, g_old_z.x);
    EXPECT_DOUBLE_EQ(seed.y, g_old_z.y);
    if (initializer.caches_squares)
    {
        EXPECT_DOUBLE_EQ(seed.x * seed.x, g_temp_sqr_x);
        EXPECT_DOUBLE_EQ(seed.y * seed.y, g_temp_sqr_y);
    }
}

TEST_P(TestPixelInitializer, preservesCallerYBeforeReset2004)
{
    const auto &[version, initializer, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    configure(initializer.type);

    EXPECT_EQ(initializer.iterations, per_pixel());

    expect_seed(initializer, {2.0, version.legacy ? 2.75 : 3.0});
}

TEST_P(TestPixelInitializer, standardRenderingPreloadsGridYForEveryReset)
{
    const auto &[version, initializer, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    configure(initializer.type);
    g_max_iterations = 100;
    g_outside_method = ColorMethod::IMAG;
    m_fractal.orbit_calc = []
    {
        g_new_z = g_old_z;
        return 1;
    };
    const int expected_color = initializer.adds_parameter ? 10 : 11;

    EXPECT_EQ(expected_color, calculate(ColorEngine::STANDARD));

    expect_seed(initializer, {2.0, 3.0});
    EXPECT_EQ(expected_color, g_color);
    EXPECT_EQ(1, g_real_color_iter);
}

TEST_P(TestPixelInitializer, inversionSamplesBothCoordinatesForEveryReset)
{
    const auto &[version, initializer, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    configure(initializer.type);
    g_inversion.invert = 1;
    g_inversion.radius = 26.0;

    EXPECT_EQ(initializer.iterations, per_pixel());

    expect_seed(initializer, {4.0, 6.0});
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestPixelInitializer,
    Combine(Values(IterationColorVersion{parse_legacy_version(2003), true},
                IterationColorVersion{parse_legacy_version(2004), false},
                IterationColorVersion{current_id_version(), false}),
        ValuesIn(PIXEL_INITIALIZERS),
        Values(CompatibilityVersionSource::PAR_RESET, CompatibilityVersionSource::GIF_METADATA)));

struct PeriodicityColoring
{
    ColorMethod inside;
    ColorMethod outside;
};

void PrintTo(const PeriodicityColoring &value, std::ostream *str)
{
    *str << "inside: " << +value.inside << ", outside: " << +value.outside;
}

class TestInsideAtanPeriodicity
    : public TestOutsideColor,
      public WithParamInterface<std::tuple<IterationColorVersion, int, PeriodicityColoring, CompatibilityVersionSource>>
{
protected:
    void initialize(int periodicity, const PeriodicityColoring &coloring);

private:
    ValueSaver<UserData> m_saved_user{g_user};
    ValueSaver<DComplex> m_saved_init{g_init};
    ValueSaver<DComplex> m_saved_old{g_old_z};
    ValueSaver<DComplex> m_saved_new{g_new_z};
    ValueSaver<long> m_saved_old_color{g_old_color_iter};
    ValueSaver<long> m_saved_first_saved_and{g_first_saved_and};
    ValueSaver<int> m_saved_saved_incr{g_periodicity_next_saved_incr};
    ValueSaver<bool> m_saved_reset_periodicity{g_reset_periodicity};
    ValueSaver<double> m_saved_close_enough{g_close_enough};
    ValueSaver<int> m_saved_atan_colors{g_atan_colors};
};

void TestInsideAtanPeriodicity::initialize(const int periodicity, const PeriodicityColoring &coloring)
{
    g_fractal_type = FractalType::MANDEL;
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    g_user = UserData{-1, 0, periodicity, CalcMode::ONE_PASS, 4, false};
    g_inside_method = coloring.inside;
    g_outside_method = coloring.outside;
    g_inside_color = 5;
    g_image_region = ImageRegion{{0.0, 0.0}, {1.0, 1.0}, {0.0, 0.0}};
    g_potential = Potential{};
    EXPECT_CALL(m_driver, check_memory()).Times(1);

    calc_frac_init();

    g_cur_fractal_specific = &m_fractal;
}

TEST_P(TestInsideAtanPeriodicity, appliesVersionGateWithoutChangingRequestedPeriodicity)
{
    const auto &[version, periodicity, coloring, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    const int expected = coloring.inside == ColorMethod::ATANI && !version.legacy ? 0 : periodicity;

    initialize(periodicity, coloring);

    EXPECT_EQ(expected, g_periodicity_check);
    EXPECT_EQ(periodicity, g_user.periodicity_value);
}

TEST_P(TestInsideAtanPeriodicity, colorsCyclingOrbitAtCompatibleStoppingPoint)
{
    const auto &[version, periodicity, coloring, source] = GetParam();
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, load_compatibility_version(version.version, source));
    initialize(periodicity, coloring);
    g_max_iterations = 10;
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {1.0, 0.0};
    g_old_color_iter = 0;
    g_first_saved_and = 1;
    g_periodicity_next_saved_incr = 4;
    g_reset_periodicity = false;
    g_close_enough = 0.0001;
    g_atan_colors = 256;
    m_fractal.orbit_calc = []
    {
        g_new_z = {g_old_z.y, g_old_z.x};
        g_old_z = g_new_z;
        return 0;
    };
    // Cycle detection stops at (1, 0); full iteration ends at (0, 1).
    const bool detects_cycle = periodicity != 0 && (version.legacy || coloring.inside != ColorMethod::ATANI);
    const int expected_color = detects_cycle && periodicity < 0 ? 7
        : coloring.inside == ColorMethod::COLOR                 ? 5
        : coloring.inside == ColorMethod::ATANI                 ? detects_cycle ? 0 : 128
                                                                : 10;

    EXPECT_EQ(expected_color, calculate(ColorEngine::STANDARD));

    EXPECT_EQ(expected_color, g_color);
    EXPECT_DOUBLE_EQ(detects_cycle ? 1.0 : 0.0, g_new_z.x);
    EXPECT_DOUBLE_EQ(detects_cycle ? 0.0 : 1.0, g_new_z.y);
    EXPECT_EQ(periodicity, g_user.periodicity_value);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestInsideAtanPeriodicity,
    Combine(Values(IterationColorVersion{parse_legacy_version(2003), true},
                IterationColorVersion{parse_legacy_version(2004), false},
                IterationColorVersion{current_id_version(), false}),
        Values(0, 1, -1, 24, -24),
        Values(PeriodicityColoring{ColorMethod::ATANI, ColorMethod::ITER},
            PeriodicityColoring{ColorMethod::COLOR, ColorMethod::ITER},
            PeriodicityColoring{ColorMethod::ITER, ColorMethod::ITER},
            PeriodicityColoring{ColorMethod::ITER, ColorMethod::ATAN}),
        Values(CompatibilityVersionSource::PAR_RESET, CompatibilityVersionSource::GIF_METADATA)));

} // namespace
} // namespace id::test
