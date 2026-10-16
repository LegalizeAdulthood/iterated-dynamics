// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/calcfrac.h>

#include "MockDriver.h"

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
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
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

enum class OutsideVersionSource
{
    PAR_RESET,
    GIF_METADATA
};

void PrintTo(const OutsideVersionSource value, std::ostream *str)
{
    *str << (value == OutsideVersionSource::PAR_RESET ? "PAR reset" : "GIF metadata");
}

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
    : public TestIterationColor,
      public WithParamInterface<
          std::tuple<IterationColorVersion, OutsideRoundingCase, FractalType, ColorEngine, OutsideVersionSource>>
{
private:
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<bool> m_saved_bad_outside{g_bad_outside};
    ValueSaver<bool> m_saved_bifurcation_functions{g_new_bifurcation_functions_loaded};
};

TEST_P(TestOutsideRounding, usesDosHalfUnitAdjustmentOnlyOnLegacyFastPath)
{
    const auto &[version, sample, type, engine, source] = GetParam();
    if (source == OutsideVersionSource::PAR_RESET)
    {
        ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET,
            cmd_arg("reset=" + to_par_string(version.version), CmdFile::AT_AFTER_STARTUP));
    }
    else
    {
        FractalInfo info{};
        info.info_version = FRACTAL_INFO_VERSION;
        if (version.version.legacy)
        {
            info.release = static_cast<std::int16_t>(100 * version.version.major + version.version.minor);
        }
        else
        {
            info.version_major = version.version.major;
            info.version_minor = version.version.minor;
            info.version_patch = version.version.patch;
            info.version_tweak = version.version.tweak;
        }
        g_file_version = fractal_info_version(info);
        g_version = g_file_version;
    }
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
        Values(OutsideVersionSource::PAR_RESET, OutsideVersionSource::GIF_METADATA)));

} // namespace
} // namespace id::test
