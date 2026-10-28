// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/PertEngine.h>

#include "MockDriver.h"

#include <engine/bailout_formula.h>
#include <engine/BoundaryTrace.h>
#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/convert_corners.h>
#include <engine/fractals.h>
#include <engine/fractalb.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/log_map.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/resume.h>
#include <engine/SoundOutput.h>
#include <engine/spindac.h>
#include <engine/StandardFractal.h>
#include <engine/SynchronousOrbit.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/burning_ship.h>
#include <fractals/fractalp.h>
#include <fractals/frasetup.h>
#include <fractals/pickover_mandelbrot.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <io/disk_storage.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/stack_avail.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

struct GlitchToleranceInput
{
    const char *name;
    std::optional<double> tolerance;
    double threshold;
};

void PrintTo(const GlitchToleranceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestGlitchTolerance : public TestWithParam<GlitchToleranceInput>
{
protected:
    void SetUp() override;

    PertEngine m_engine;
};

void TestGlitchTolerance::SetUp()
{
    if (GetParam().tolerance)
    {
        m_engine.set_glitch_tolerance(*GetParam().tolerance);
    }
}

TEST_P(TestGlitchTolerance, scalesThreshold)
{
    EXPECT_DOUBLE_EQ(GetParam().threshold, m_engine.glitch_tolerance_threshold({3.0, 4.0}));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestGlitchTolerance,
    Values(GlitchToleranceInput{"Default", std::nullopt, 25.0e-12}, GlitchToleranceInput{"NonDefault", 0.5, 6.25}),
    [](const TestParamInfo<GlitchToleranceInput> &info) { return info.param.name; });

struct DerivativeStep
{
    DComplex point;
    bool continues;
    std::optional<DComplex> derivative;
};

struct DerivativeTrajectoryInput
{
    const char *name;
    bool mandelbrot;
    bool legacy;
    double limit;
    std::optional<DComplex> initial;
    std::vector<DerivativeStep> steps;
};

void PrintTo(const DerivativeTrajectoryInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDistanceDerivative : public TestWithParam<DerivativeTrajectoryInput>
{
};

TEST_P(TestDistanceDerivative, followsDerivativeTrajectory)
{
    const DerivativeTrajectoryInput &input{GetParam()};
    DistanceEstimator estimator{4.0, input.limit, input.mandelbrot, input.legacy};

    if (input.initial)
    {
        EXPECT_EQ(*input.initial, estimator.derivative());
    }
    for (const DerivativeStep &step : input.steps)
    {
        SCOPED_TRACE(PrintToString(step.point));
        EXPECT_EQ(step.continues, estimator.advance(step.point));
        if (step.derivative)
        {
            EXPECT_EQ(*step.derivative, estimator.derivative());
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceDerivative,
    Values(DerivativeTrajectoryInput{"Mandelbrot", true, false, 100.0, DComplex{1.0, 0.0},
               {{{0.5, 0.0}, true, DComplex{2.0, 0.0}}, {{0.75, 0.0}, true, DComplex{4.0, 0.0}},
                   {{1.0625, 0.0}, true, DComplex{9.5, 0.0}}}},
        DerivativeTrajectoryInput{"Julia", false, false, 100.0, std::nullopt,
            {{{0.5, 0.5}, true, DComplex{1.0, 1.0}}, {{0.25, 1.0}, true, DComplex{-1.5, 2.5}}}},
        DerivativeTrajectoryInput{"OrdinaryNormLimit", true, false, 5.0, std::nullopt,
            {{{0.5, 0.5}, true, DComplex{2.0, 1.0}}, {{0.0, 0.75}, true, DComplex{-0.5, 3.0}},
                {{1.0, 0.0}, false, std::nullopt}}},
        DerivativeTrajectoryInput{"LegacyNormLimit", true, true, 5.0, std::nullopt,
            {{{0.5, 0.5}, true, DComplex{2.0, 1.0}}, {{0.0, 0.75}, false, std::nullopt}}}),
    [](const TestParamInfo<DerivativeTrajectoryInput> &info) { return info.param.name; });

TEST(TestDistanceEstimator, legacyEscapeRetainsFirstEscapeAndRaisesBailout)
{
    DistanceEstimator estimator{4.0, 100.0, true, true};

    EXPECT_FALSE(estimator.escaped({3.0, 1.0}, 4, 10.0));
    EXPECT_DOUBLE_EQ(535.5, estimator.bailout());
    EXPECT_TRUE(estimator.escaped({30.0, 1.0}, 5, 901.0));
    EXPECT_EQ(4, estimator.escape_iteration());
    EXPECT_EQ((DComplex{3.0, 1.0}), estimator.escape_point());
}

struct DistanceEscapeInput
{
    const char *name;
    double bailout;
    bool legacy;
    DComplex point;
    double magnitude;
    std::optional<double> expected_bailout;
    std::optional<long> escape_iteration;
};

void PrintTo(const DistanceEscapeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDistanceEscape : public TestWithParam<DistanceEscapeInput>
{
};

TEST_P(TestDistanceEscape, stopsImmediately)
{
    const DistanceEscapeInput &input{GetParam()};
    DistanceEstimator estimator{input.bailout, 100.0, true, input.legacy};

    EXPECT_TRUE(estimator.escaped(input.point, 1, input.magnitude));
    if (input.expected_bailout)
    {
        EXPECT_DOUBLE_EQ(*input.expected_bailout, estimator.bailout());
    }
    if (input.escape_iteration)
    {
        EXPECT_EQ(*input.escape_iteration, estimator.escape_iteration());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceEscape,
    Values(DistanceEscapeInput{"Ordinary", 4.0, false, {3.0, 0.0}, 9.0, 4.0, -1},
        DistanceEscapeInput{"LargeLegacyBailout", 600.0, true, {30.0, 0.0}, 900.0, 600.0, std::nullopt},
        DistanceEscapeInput{"ZeroMagnitude", 4.0, true, {}, 0.0, std::nullopt, std::nullopt}),
    [](const TestParamInfo<DistanceEscapeInput> &info) { return info.param.name; });

struct DistanceEstimateInput
{
    const char *name;
    DComplex terminal;
    bool overflow;
    double expected;
};

void PrintTo(const DistanceEstimateInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDistanceEstimate : public TestWithParam<DistanceEstimateInput>
{
protected:
    void SetUp() override;

    DistanceEstimator m_estimator{4.0, 100.0, true, false};
};

void TestDistanceEstimate::SetUp()
{
    EXPECT_TRUE(m_estimator.advance({0.5, 0.5}));
}

TEST_P(TestDistanceEstimate, usesTerminalMagnitudeAndDerivative)
{
    const DistanceEstimateInput &input{GetParam()};

    EXPECT_DOUBLE_EQ(input.expected, m_estimator.distance_squared(input.terminal, input.overflow));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceEstimate,
    Values(DistanceEstimateInput{"TerminalMagnitude", {3.0, 4.0}, false, 25.0 * std::log(25.0) * std::log(25.0) / 5.0},
        DistanceEstimateInput{"ZeroMagnitude", {}, false, 0.0},
        DistanceEstimateInput{"Overflow", {3.0, 4.0}, true, 0.0}),
    [](const TestParamInfo<DistanceEstimateInput> &info) { return info.param.name; });

struct DistanceBoundaryInput
{
    const char *name;
    double distance_squared;
    bool expected;
};

void PrintTo(const DistanceBoundaryInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDistanceBoundary : public TestWithParam<DistanceBoundaryInput>
{
protected:
    const DistanceEstimator m_estimator{4.0, 100.0, true, false, 0.5};
};

TEST_P(TestDistanceBoundary, comparesBoundaryStrictly)
{
    EXPECT_EQ(GetParam().expected, m_estimator.on_boundary(GetParam().distance_squared));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBoundary,
    Values(DistanceBoundaryInput{"BelowThreshold", std::nextafter(0.5, 0.0), true},
        DistanceBoundaryInput{"AtThreshold", 0.5, false},
        DistanceBoundaryInput{"AboveThreshold", std::nextafter(0.5, 1.0), false}),
    [](const TestParamInfo<DistanceBoundaryInput> &info) { return info.param.name; });

struct DistanceBandInput
{
    std::string name;
    double width;
    bool legacy;
    bool old_colors;
    double distance_squared;
    long expected;
};

void PrintTo(const DistanceBandInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBandInput> distance_band_inputs()
{
    std::vector<DistanceBandInput> inputs;

    struct BandWidth
    {
        const char *name;
        double value;
    };

    for (const BandWidth &width : {BandWidth{"OneEighth", 0.125}, {"One", 1.0}, {"Four", 4.0}})
    {
        for (const bool legacy : {false, true})
        {
            for (const bool old_colors : {false, true})
            {
                const std::string prefix{std::string{width.name} + (legacy ? "_Legacy" : "_Ordinary") +
                    (old_colors ? "_OldColors" : "_CurrentColors")};
                inputs.push_back({prefix + "_Zero", width.value, legacy, old_colors, 0.0, 1});
                for (const long color : {2L, 3L, 8L, 16L})
                {
                    double threshold{width.value * (color - 1)};
                    if (old_colors)
                    {
                        threshold = sqr(width.value * (color * color - 1));
                    }
                    else if (legacy)
                    {
                        threshold = width.value * (color * color - 1);
                    }
                    const std::string name{prefix + "_Color" + std::to_string(color)};
                    inputs.push_back(
                        {name + "_Below", width.value, legacy, old_colors, threshold * (1.0 - 1.0 / 1024), color - 1});
                    inputs.push_back({name + "_At", width.value, legacy, old_colors, threshold, color});
                    inputs.push_back(
                        {name + "_Above", width.value, legacy, old_colors, threshold * (1.0 + 1.0 / 1024), color});
                }
            }
        }
    }
    return inputs;
}

class TestDistanceBand : public TestWithParam<DistanceBandInput>
{
};

TEST_P(TestDistanceBand, preservesIntegerBoundary)
{
    const DistanceBandInput &input{GetParam()};
    const DistanceEstimator estimator{4.0, 100.0, true, input.legacy, 0.0, input.width};

    EXPECT_EQ(input.expected, estimator.band_color(input.distance_squared, input.old_colors));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBand, ValuesIn(distance_band_inputs()),
    [](const TestParamInfo<DistanceBandInput> &info) { return info.param.name; });

struct FmodMagnitudeInput
{
    const char *name;
    Bailout bailout;
    DComplex point;
    double cached_magnitude;
    bool calculate;
    double expected;
};

void PrintTo(const FmodMagnitudeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFmodMagnitude : public TestWithParam<FmodMagnitudeInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<Bailout> m_saved_bailout{g_bailout_test};
    ValueSaver<bool> m_saved_calculate{g_magnitude_calc};
};

void TestFmodMagnitude::SetUp()
{
    g_bailout_test = GetParam().bailout;
    g_magnitude_calc = GetParam().calculate;
}

TEST_P(TestFmodMagnitude, usesSelectedBailoutMetric)
{
    const FmodMagnitudeInput &input{GetParam()};

    EXPECT_DOUBLE_EQ(input.expected, fmod_magnitude(input.point, input.cached_magnitude));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFmodMagnitude,
    Values(FmodMagnitudeInput{"ModCached", Bailout::MOD, {2.0, -3.0}, 13.0, false, 13.0},
        FmodMagnitudeInput{"Real", Bailout::REAL, {2.0, -3.0}, 13.0, false, 4.0},
        FmodMagnitudeInput{"Imaginary", Bailout::IMAG, {2.0, -3.0}, 13.0, false, 9.0},
        FmodMagnitudeInput{"Or", Bailout::OR, {2.0, -3.0}, 13.0, false, 9.0},
        FmodMagnitudeInput{"And", Bailout::AND, {2.0, -3.0}, 13.0, false, 13.0},
        FmodMagnitudeInput{"Manhattan", Bailout::MANH, {2.0, -3.0}, 13.0, false, 25.0},
        FmodMagnitudeInput{"ManhattanReal", Bailout::MANR, {2.0, -3.0}, 13.0, false, 1.0},
        FmodMagnitudeInput{"OrRealDominates", Bailout::OR, {-3.0, 2.0}, 13.0, false, 9.0},
        FmodMagnitudeInput{"ModUsesSuppliedCache", Bailout::MOD, {2.0, -3.0}, 7.0, false, 7.0},
        FmodMagnitudeInput{"ModComputesMissingCache", Bailout::MOD, {2.0, -3.0}, 0.0, false, 13.0},
        FmodMagnitudeInput{"ModRecalculates", Bailout::MOD, {2.0, -3.0}, 7.0, true, 13.0}),
    [](const TestParamInfo<FmodMagnitudeInput> &info) { return info.param.name; });

struct ReferenceSampleCountInput
{
    const char *name;
    long iterations;
    std::optional<std::uintmax_t> expected;
};

void PrintTo(const ReferenceSampleCountInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ReferenceSampleCountInput> reference_sample_count_inputs()
{
    std::vector<ReferenceSampleCountInput> inputs{{"IncludesInitialValue", 128, 129}};
    constexpr long WINDOWS_LONG_MAX{std::numeric_limits<std::int32_t>::max()};
    if constexpr (sizeof(std::size_t) > sizeof(std::int32_t))
    {
        inputs.push_back({"WindowsLongMaximum", WINDOWS_LONG_MAX, 2147483648ULL});
    }
    else
    {
        inputs.push_back({"WindowsLongMaximum", WINDOWS_LONG_MAX, std::nullopt});
    }
    constexpr std::size_t BYTES_PER_SAMPLE{sizeof(std::complex<double>) + sizeof(double)};
    const std::uintmax_t max_samples{std::min({std::vector<std::complex<double>>{}.max_size(),
        std::vector<double>{}.max_size(), std::numeric_limits<std::size_t>::max() / BYTES_PER_SAMPLE})};
    const std::uintmax_t max_iterations{std::numeric_limits<long>::max()};
    const long largest_supported{static_cast<long>(std::min(max_iterations, max_samples - 1))};
    inputs.push_back({"LargestSupported", largest_supported, static_cast<std::uintmax_t>(largest_supported) + 1});
    if (max_samples <= max_iterations)
    {
        inputs.push_back({"ContainerLimit", static_cast<long>(max_samples), std::nullopt});
        inputs.push_back({"LongMaximum", std::numeric_limits<long>::max(), std::nullopt});
    }
    return inputs;
}

class TestReferenceSampleCount : public TestWithParam<ReferenceSampleCountInput>
{
};

TEST_P(TestReferenceSampleCount, checksReferenceCapacity)
{
    const ReferenceSampleCountInput &input{GetParam()};

    if (input.expected)
    {
        EXPECT_EQ(*input.expected, PertEngine::reference_sample_count(input.iterations));
    }
    else
    {
        EXPECT_THROW(PertEngine::reference_sample_count(input.iterations), std::length_error);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestReferenceSampleCount, ValuesIn(reference_sample_count_inputs()),
    [](const TestParamInfo<ReferenceSampleCountInput> &info) { return info.param.name; });

struct InvalidReferenceSampleCountInput
{
    const char *name;
    long iterations;
};

void PrintTo(const InvalidReferenceSampleCountInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInvalidReferenceSampleCount : public TestWithParam<InvalidReferenceSampleCountInput>
{
};

TEST_P(TestInvalidReferenceSampleCount, rejectsNonpositiveIterations)
{
    EXPECT_THROW(PertEngine::reference_sample_count(GetParam().iterations), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInvalidReferenceSampleCount,
    Values(InvalidReferenceSampleCountInput{"Zero", 0}, InvalidReferenceSampleCountInput{"Negative", -1},
        InvalidReferenceSampleCountInput{"LongMinimum", std::numeric_limits<long>::min()}),
    [](const TestParamInfo<InvalidReferenceSampleCountInput> &info) { return info.param.name; });

namespace
{

constexpr int WIDTH{8};
constexpr int HEIGHT{6};
PertEngine *s_pixel_engine{};
std::vector<Point2i> s_requested_pixels;
std::vector<std::complex<double>> s_reference_coordinates;
std::vector<std::complex<double>> s_reference_values;
std::complex<double> s_parameter_delta;
std::vector<std::complex<double>> s_standard_orbit;
std::vector<std::complex<double>> s_perturbation_orbit;
const DistanceEstimator *s_standard_distance{};
std::vector<DComplex> s_standard_derivatives;
std::vector<DComplex> s_perturbation_derivatives;

int record_standard_orbit()
{
    if (s_standard_distance != nullptr)
    {
        s_standard_derivatives.push_back(s_standard_distance->derivative());
    }
    s_standard_orbit.emplace_back(g_old_z.x, g_old_z.y);
    return julia_orbit();
}

void record_perturbation_orbit(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter_delta)
{
    if (s_standard_distance != nullptr)
    {
        s_perturbation_derivatives.push_back(s_pixel_engine->distance_estimator().derivative());
    }
    s_perturbation_orbit.push_back(reference + delta);
    mandel_perturb(reference, delta, parameter_delta);
}

struct CoordinateSample
{
    Point2i pixel;
    std::complex<double> value;
    std::complex<double> parameter_delta;
};

std::vector<CoordinateSample> s_coordinates;

void record_coordinate(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter_delta)
{
    s_coordinates.push_back({{g_col, g_row}, reference + delta, parameter_delta});
    mandel_perturb(reference, delta, parameter_delta);
}

void fail_reference_bf(const BFComplex &, BFComplex &)
{
    throw std::bad_array_new_length{};
}

void record_reference(const std::complex<double> &center, std::complex<double> &z)
{
    s_reference_coordinates.push_back(center);
    s_reference_values.push_back(z);
    mandel_ref_pt(center, z);
}

void record_reference_bf(const BFComplex &center, BFComplex &z)
{
    s_reference_coordinates.emplace_back(bf_to_float(center.x), bf_to_float(center.y));
    s_reference_values.emplace_back(bf_to_float(z.x), bf_to_float(z.y));
    mandel_ref_pt_bf(center, z);
}

void record_deep_reference_bf(const BFComplex &center, BFComplex &z)
{
    BigStackSaver saved;
    const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
    const BigFloat scale{alloc_stack(g_r_bf_length + 2)};
    str_to_bf(scale, "1e-400");
    int_to_bf(offset, -2);
    sub_bf(offset, center.x, offset);
    div_bf(offset, offset, scale);
    const double x{static_cast<double>(bf_to_float(offset))};
    div_bf(offset, center.y, scale);
    const std::complex<double> coordinate{x, static_cast<double>(bf_to_float(offset))};
    if (s_reference_coordinates.empty() || s_reference_coordinates.back() != coordinate)
    {
        s_reference_coordinates.push_back(coordinate);
    }
    mandel_ref_pt_bf(center, z);
}

void record_delta(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter_delta)
{
    s_parameter_delta = parameter_delta;
    mandel_perturb(reference, delta, parameter_delta);
}

int calculate_requested_pixel()
{
    s_requested_pixels.push_back({g_col, g_row});
    return s_pixel_engine->calculate_pixel(g_col, g_row);
}

int calculate_control_pixel()
{
    s_requested_pixels.push_back({g_col, g_row});
    g_plot(g_col, g_row, 7);
    return 7;
}

struct OrbitState
{
    long iterations;
    DComplex old_z;
    DComplex new_z;
    double magnitude;
    double square_x;
    double square_y;
};

struct DistancePixelOptions
{
    long value{1};
    int width_factor{1};
    Point2i virtual_size{};
    ImageRegion region{{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}};
    ColorMethod inside{ColorMethod::ITER};
    bool compare_colors{};
    bool old_colors{};
};

void expect_orbit_state(const OrbitState &expected)
{
    EXPECT_EQ(expected.iterations, g_color_iter);
    EXPECT_EQ(expected.iterations, g_real_color_iter);
    EXPECT_NEAR(expected.old_z.x, g_old_z.x, 1e-14);
    EXPECT_NEAR(expected.old_z.y, g_old_z.y, 1e-14);
    EXPECT_NEAR(expected.new_z.x, g_new_z.x, 1e-14);
    EXPECT_NEAR(expected.new_z.y, g_new_z.y, 1e-14);
    EXPECT_NEAR(expected.magnitude, g_magnitude, 1e-14);
    EXPECT_NEAR(expected.square_x, g_temp_sqr_x, 1e-14);
    EXPECT_NEAR(expected.square_y, g_temp_sqr_y, 1e-14);
}

void plot_iteration_color(const int col, const int row, const int color)
{
    put_color_a(col, row, color);
    targa_color(col, row, color);
}

} // namespace

class TestPerturbationPixels : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    void initialize_deep_frame();
    void check_pass_switch(PerturbationMode mode);

    PertEngine m_engine;
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    std::array<int, WIDTH * HEIGHT> m_pixels{};
    std::vector<Point2i> m_plots;
    std::vector<StandardPassStatus> m_plot_statuses;
    bool m_big_float{};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<PertEngine *> m_saved_engine{s_pixel_engine, &m_engine};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<int> m_saved_width{g_screen_x_dots, WIDTH};
    ValueSaver<int> m_saved_height{g_screen_y_dots, HEIGHT};
    ValueSaver<LogicalScreen> m_saved_screen{
        g_logical_screen, LogicalScreen{WIDTH, HEIGHT, 0, 0, WIDTH - 1.0, HEIGHT - 1.0}};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_and_color{g_and_color, 255};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 128};
    ValueSaver<double> m_saved_bailout{g_magnitude_limit, 4.0};
    ValueSaver<int> m_saved_periodicity{g_periodicity_check, 0};
    ValueSaver<bool> m_saved_reset_periodicity{g_reset_periodicity, false};
    ValueSaver<long> m_saved_old_color_iter{g_old_color_iter, 0};
    ValueSaver<long> m_saved_first_saved_and{g_first_saved_and, 9};
    ValueSaver<int> m_saved_next_saved_incr{g_periodicity_next_saved_incr, 4};
    ValueSaver<double> m_saved_close_enough{g_close_enough, 1e-9};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 7};
    ValueSaver<int> m_saved_biomorph{g_biomorph, -1};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<long> m_saved_color_iter{g_color_iter};
    ValueSaver<long> m_saved_real_color_iter{g_real_color_iter};
    ValueSaver<DComplex> m_saved_old_z{g_old_z};
    ValueSaver<DComplex> m_saved_new_z{g_new_z};
    ValueSaver<double> m_saved_magnitude{g_magnitude};
    ValueSaver<double> m_saved_square_x{g_temp_sqr_x};
    ValueSaver<double> m_saved_square_y{g_temp_sqr_y};
};

void TestPerturbationPixels::SetUp()
{
    s_requested_pixels.clear();
    s_reference_coordinates.clear();
    s_reference_values.clear();
    s_parameter_delta = {};
    set_normal_dot();
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](const int col, const int row) { return m_pixels.at(row * WIDTH + col); }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](const int col, const int row, const int color)
            {
                m_pixels.at(row * WIDTH + col) = color;
                m_plots.push_back({col, row});
                if (const StandardFractal *calculation{active_standard_fractal()})
                {
                    m_plot_statuses.push_back(calculation->standard_pass_status());
                }
            }));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
}

void TestPerturbationPixels::TearDown()
{
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
    }
}

void TestPerturbationPixels::initialize_deep_frame()
{
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
    str_to_bf(center.x, "0.25");
    clear_bf(center.y);
    m_engine.set_glitch_tolerance(2.0);
}

TEST_F(TestPerturbationPixels, finishingReleasesReferenceWorkingStorageAndAllowsAnotherFrame)
{
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    const int initial_stack{save_stack()};

    for (int frame = 0; frame < 2; ++frame)
    {
        const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
        float_to_bf(center.x, 0.25);
        clear_bf(center.y);
        EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
        EXPECT_EQ(1, m_engine.reference_count());
        EXPECT_GT(save_stack(), initial_stack);

        m_engine.finish();

        EXPECT_EQ(initial_stack, save_stack());
        EXPECT_EQ(0, m_engine.reference_count());
        m_engine.finish();
        EXPECT_EQ(initial_stack, save_stack());
    }
}

TEST_F(TestPerturbationPixels, failedInitializationReleasesStorageAndPreservesTheAllocationError)
{
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    const int initial_stack{save_stack()};
    const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
    float_to_bf(center.x, 0.25);
    clear_bf(center.y);
    m_specific.pert_ref_bf = fail_reference_bf;

    EXPECT_THROW(m_engine.initialize_pixel_strategy(), std::bad_array_new_length);

    EXPECT_EQ(initial_stack, save_stack());
    EXPECT_EQ(0, m_engine.reference_count());
    m_engine.finish();
    EXPECT_EQ(initial_stack, save_stack());

    m_specific.pert_ref_bf = mandel_ref_pt_bf;
    const BFComplex next_center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
    float_to_bf(next_center.x, 0.25);
    clear_bf(next_center.y);
    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(1, m_engine.reference_count());
}

struct PerturbationBailoutInput
{
    std::string name;
    BFMathType math;
    double imag;
    Bailout bailout;
    double limit;
};

void PrintTo(const PerturbationBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PerturbationBailoutInput> perturbation_bailout_inputs()
{
    struct MathMode
    {
        const char *name;
        BFMathType value;
    };

    struct BailoutMetric
    {
        const char *name;
        Bailout value;
    };

    struct Threshold
    {
        const char *name;
        double value;
    };

    std::vector<PerturbationBailoutInput> inputs;
    for (const MathMode &math :
        {MathMode{"Double", BFMathType::NONE}, {"BigNum", BFMathType::BIG_NUM}, {"BigFloat", BFMathType::BIG_FLT}})
    {
        for (const double imag : {-1.0, 1.0})
        {
            const std::array<double, 7> metrics{5.0, 4.0, 1.0, 4.0, 1.0, 9.0, imag < 0.0 ? 1.0 : 9.0};
            for (const BailoutMetric &bailout : {BailoutMetric{"Mod", Bailout::MOD}, {"Real", Bailout::REAL},
                     {"Imaginary", Bailout::IMAG}, {"Or", Bailout::OR}, {"And", Bailout::AND},
                     {"Manhattan", Bailout::MANH}, {"ManhattanReal", Bailout::MANR}})
            {
                const double metric{metrics[static_cast<std::size_t>(bailout.value)]};
                for (const Threshold &threshold : {Threshold{"Below", std::nextafter(metric, 0.0)}, {"At", metric},
                         {"Above", std::nextafter(metric, std::numeric_limits<double>::infinity())}})
                {
                    inputs.push_back({std::string{math.name} + (imag < 0.0 ? "_NegativeImag_" : "_PositiveImag_") +
                            bailout.name + "_" + threshold.name,
                        math.value, imag, bailout.value, threshold.value});
                }
            }
        }
    }
    return inputs;
}

class TestPerturbationBailout : public TestPerturbationPixels, public WithParamInterface<PerturbationBailoutInput>
{
protected:
    void SetUp() override;

    ValueSaver<DComplex *> m_saved_parameter{g_float_param, &g_init};
    ValueSaver<Bailout> m_saved_test{g_bailout_test};
    ValueSaver<decltype(g_bailout_float)> m_saved_float_bailout{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_saved_bn_bailout{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_saved_bf_bailout{g_bailout_bigfloat};
};

void TestPerturbationBailout::SetUp()
{
    TestPerturbationPixels::SetUp();
    if (GetParam().math != BFMathType::NONE)
    {
        const ValueSaver saved_type{
            g_fractal_type, GetParam().math == BFMathType::BIG_NUM ? FractalType::MANDEL : FractalType::MANDEL_Z_POWER};
        init_bf_dec(60);
        m_big_float = true;
    }
    ASSERT_EQ(GetParam().math, g_bf_math);
    g_max_iterations = 4;
}

TEST_P(TestPerturbationBailout, boundaryDecisionsMatchNativeOrbitCallbacks)
{
    const PerturbationBailoutInput &input{GetParam()};
    g_bailout_test = input.bailout;
    set_bailout_formula(input.bailout);
    m_engine.finish();
    g_magnitude_limit = input.limit;
    g_init = {2.0, input.imag};
    g_old_z = {};
    g_temp_sqr_x = g_temp_sqr_y = 0.0;
    if (input.math == BFMathType::BIG_NUM)
    {
        float_to_bn(g_param_z_bn.x, 2.0);
        float_to_bn(g_param_z_bn.y, input.imag);
        clear_bn(g_old_z_bn.x);
        clear_bn(g_old_z_bn.y);
        square_bn(g_tmp_sqr_x_bn, g_old_z_bn.x);
        square_bn(g_tmp_sqr_y_bn, g_old_z_bn.y);
    }
    else if (input.math == BFMathType::BIG_FLT)
    {
        float_to_bf(g_param_z_bf.x, 2.0);
        float_to_bf(g_param_z_bf.y, input.imag);
        clear_bf(g_old_z_bf.x);
        clear_bf(g_old_z_bf.y);
        square_bf(g_tmp_sqr_x_bf, g_old_z_bf.x);
        square_bf(g_tmp_sqr_y_bf, g_old_z_bf.y);
    }
    std::vector<DComplex> expected;
    bool escaped{};
    for (int iteration = 0; iteration < g_max_iterations && !escaped; ++iteration)
    {
        switch (input.math)
        {
        case BFMathType::NONE:
            escaped = julia_orbit() != 0;
            expected.push_back(g_new_z);
            break;
        case BFMathType::BIG_NUM:
            escaped = julia_orbit_bn() != 0;
            expected.push_back(cmplx_bn_to_float(g_new_z_bn));
            break;
        case BFMathType::BIG_FLT:
            escaped = julia_orbit_bf() != 0;
            expected.push_back(cmplx_bf_to_float(g_new_z_bf));
            break;
        }
    }
    ASSERT_TRUE(escaped);
    if (input.math == BFMathType::NONE)
    {
        m_engine.initialize_frame({}, {}, {});
    }
    else
    {
        const BFComplex center{m_engine.initialize_frame_bf({}, {})};
        clear_bf(center.x);
        clear_bf(center.y);
    }
    m_engine.begin_orbit({2.0, input.imag}, {}, 0);
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        SCOPED_TRACE(i);
        const bool done{m_engine.iterate_orbit()};
        EXPECT_EQ(i + 1, m_engine.orbit_iteration());
        EXPECT_DOUBLE_EQ(expected[i].x, m_engine.orbit_value().real());
        EXPECT_DOUBLE_EQ(expected[i].y, m_engine.orbit_value().imag());
        EXPECT_EQ(i + 1 == expected.size(), m_engine.orbit_escaped());
        EXPECT_EQ(i + 1 == expected.size(), done);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationBailout, ValuesIn(perturbation_bailout_inputs()),
    [](const TestParamInfo<PerturbationBailoutInput> &info) { return info.param.name; });

class PerturbationIterationTest : public TestPerturbationPixels
{
public:
    explicit PerturbationIterationTest(bool use_big_float);

protected:
    void SetUp() override;
    void TearDown() override;
    void initialize_reference(const std::complex<double> &center, double half_width = 0.0, double half_height = 0.0);
    void compare_pixel(const std::complex<double> &center, long max_iterations, double reference_offset = 0.0,
        long early_iterations = 0, bool incremental = false);
    void set_initial_offset(const DComplex &offset);
    void compare_distance_pixel(const std::complex<double> &center, long max_iterations, bool legacy,
        double reference_offset = 0.0, const DistancePixelOptions &options = {}, bool incremental = false);

    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<long> m_saved_distance{g_distance_estimator, 0};
    ValueSaver<int> m_saved_decomp{g_decomp[0], 0};
    ValueSaver<int> m_saved_attractors{g_attractor.count, 0};
    ValueSaver<InitOrbitMode> m_saved_initial_orbit{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver<DComplex> m_saved_initial_value{g_init_orbit};
    ValueSaver<DComplex> m_saved_param{g_param_z1, DComplex{}};
    ValueSaver<double> m_saved_param_real{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_imag{g_params[1], 0.0};
    ValueSaver<bool> m_saved_bof{g_bof_match_book_images};
    ValueSaver<DComplex *> m_saved_float_param{g_float_param, &g_init};
    ValueSaver<int> m_saved_col{g_col, WIDTH / 2};
    ValueSaver<int> m_saved_row{g_row, HEIGHT / 2 - 1};
    ValueSaver<decltype(g_bailout_float)> m_saved_float_bailout{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_saved_bn_bailout{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_saved_bf_bailout{g_bailout_bigfloat};
    ValueSaver<Bailout> m_saved_bailout_test{g_bailout_test};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<int> m_saved_atan_colors{g_atan_colors};
    ValueSaver<double> m_saved_proximity{g_close_proximity, 0.01};
    ValueSaver<bool> m_saved_magnitude_calc{g_magnitude_calc, false};
    ValueSaver<std::vector<Byte>> m_saved_log_table{g_log_map_table, std::vector<Byte>{}};
    ValueSaver<long> m_saved_log_flag{g_log_map_flag, 0};
    ValueSaver<long> m_saved_log_max{g_log_map_table_max_size, 0};
    ValueSaver<bool> m_saved_log_calculate{g_log_map_calculate, false};
    ValueSaver<bool> m_saved_auto_log{g_log_map_auto_calculate, false};
    ValueSaver<LogMapCalculate> m_saved_log_fly{g_log_map_fly_calculate, LogMapCalculate::NONE};
    ValueSaver<std::vector<int>> m_saved_ranges{g_iteration_ranges, std::vector<int>{}};
    ValueSaver<int> m_saved_outside_color{g_outside_color};
    int m_reference_yields{};
    const bool m_use_big_float;
};

PerturbationIterationTest::PerturbationIterationTest(const bool use_big_float) :
    m_use_big_float{use_big_float}
{
}

class TestPerturbationIterations : public PerturbationIterationTest, public WithParamInterface<bool>
{
public:
    TestPerturbationIterations();
};

TestPerturbationIterations::TestPerturbationIterations() :
    PerturbationIterationTest{GetParam()}
{
}

void PerturbationIterationTest::SetUp()
{
    TestPerturbationPixels::SetUp();
    set_bailout_formula(Bailout::MOD);
    g_dispatch = FractalDispatch{m_specific};
    alloc_pixel_grid();
}

void PerturbationIterationTest::TearDown()
{
    end_disk();
    free_pixel_grid();
    TestPerturbationPixels::TearDown();
}

void PerturbationIterationTest::initialize_reference(
    const std::complex<double> &center, const double half_width, const double half_height)
{
    if (m_use_big_float)
    {
        init_bf_dec(60);
        m_big_float = true;
        if (g_periodicity_check != 0)
        {
            if (g_bf_math == BFMathType::BIG_NUM)
            {
                float_to_bn(g_close_enough_bn, g_close_enough);
            }
            else
            {
                float_to_bf(g_close_enough_bf, g_close_enough);
            }
        }
        const BFComplex center_bf{m_engine.initialize_frame_bf({half_width, 0.0}, {0.0, half_height})};
        float_to_bf(center_bf.x, center.real());
        float_to_bf(center_bf.y, center.imag());
    }
    else
    {
        m_engine.initialize_frame(center, {half_width, 0.0}, {0.0, half_height});
    }
}

void PerturbationIterationTest::set_initial_offset(const DComplex &offset)
{
    g_param_z1 = offset;
    g_params[0] = offset.x;
    g_params[1] = offset.y;
}

void PerturbationIterationTest::compare_pixel(const std::complex<double> &center, const long max_iterations,
    const double reference_offset, const long early_iterations, const bool incremental)
{
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
        m_big_float = false;
    }
    s_standard_orbit.clear();
    s_perturbation_orbit.clear();
    g_max_iterations = max_iterations;
    g_col = WIDTH - 1;
    const DComplex point{center.real(), center.imag()};
    g_image_region = ImageRegion{point, point, point};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    fill_pixel_grid();
    g_dispatch.set_orbit_calc(record_standard_orbit);
    const long initial_old_color_iter{g_old_color_iter};
    StandardFractal standard;
    const Point2i potential_pixel{g_col + g_logical_screen.x_offset, g_row + g_logical_screen.y_offset};
    if (g_potential.store_16bit)
    {
        write_disk_pixel(potential_pixel.x, potential_pixel.y, 255);
        write_disk_pixel(potential_pixel.x, potential_pixel.y + g_screen_y_dots, 255);
    }
    const int expected_color{standard.calculate_standard_pixel(false)};
    std::array<int, 2> expected_potential{};
    if (g_potential.store_16bit)
    {
        expected_potential = {read_disk_pixel(potential_pixel.x, potential_pixel.y),
            read_disk_pixel(potential_pixel.x, potential_pixel.y + g_screen_y_dots)};
        write_disk_pixel(potential_pixel.x, potential_pixel.y, 255);
        write_disk_pixel(potential_pixel.x, potential_pixel.y + g_screen_y_dots, 255);
    }
    // The standard epsilon-cross early exit does not publish its raw iteration count.
    const long expected_iterations{early_iterations != 0 ? early_iterations : g_real_color_iter};
    if (early_iterations != 0)
    {
        EXPECT_EQ(early_iterations, s_standard_orbit.size());
    }
    const OrbitState expected{expected_iterations, g_old_z, g_new_z, g_magnitude, g_temp_sqr_x, g_temp_sqr_y};
    const long expected_old_color_iter{g_old_color_iter};

    m_specific.pert_pt = record_perturbation_orbit;
    initialize_reference(center - std::complex<double>{reference_offset, 0.0}, reference_offset);
    g_old_color_iter = initial_old_color_iter;
    g_color_iter = -17;
    g_real_color_iter = -19;
    g_old_z = {-7.0, 9.0};
    g_new_z = {-11.0, 13.0};
    g_magnitude = g_temp_sqr_x = g_temp_sqr_y = -1.0;
    EXPECT_CALL(m_driver, write_pixel(g_col, g_row, expected_color))
        .WillOnce(Invoke([&expected](int, int, int) { expect_orbit_state(expected); }))
        .RetiresOnSaturation();

    m_reference_yields = 0;
    int color{CALC_YIELDED};
    for (int step = 0; step < 1000 && color == CALC_YIELDED; ++step)
    {
        const std::size_t before{s_reference_coordinates.size()};
        const std::size_t pixel_before{s_perturbation_orbit.size()};
        color = m_engine.calculate_pixel(g_col, g_row, incremental);
        if (incremental)
        {
            EXPECT_LE(s_reference_coordinates.size() - before, 4096);
            EXPECT_LE(s_perturbation_orbit.size() - pixel_before, 2048);
        }
        if (color == CALC_YIELDED)
        {
            ++m_reference_yields;
        }
    }
    EXPECT_EQ(expected_color, color);

    if (g_potential.store_16bit)
    {
        EXPECT_EQ(expected_potential[0], read_disk_pixel(potential_pixel.x, potential_pixel.y));
        EXPECT_EQ(expected_potential[1], read_disk_pixel(potential_pixel.x, potential_pixel.y + g_screen_y_dots));
    }
    expect_orbit_state(expected);
    EXPECT_EQ(expected_old_color_iter, g_old_color_iter);
    if (m_engine.reference_count() == 1)
    {
        EXPECT_EQ(s_standard_orbit.size(), s_perturbation_orbit.size());
    }
    ASSERT_GE(s_perturbation_orbit.size(), s_standard_orbit.size());
    const std::size_t first{s_perturbation_orbit.size() - s_standard_orbit.size()};
    for (std::size_t i = 0; i < s_standard_orbit.size(); ++i)
    {
        double real_tolerance{1e-14};
        double imag_tolerance{1e-14};
        if (g_inside_method == ColorMethod::STAR_TRAIL)
        {
            // Escaped star-trail coordinates can exceed the useful range of an absolute tolerance.
            constexpr double RELATIVE_TOLERANCE{4.0 * std::numeric_limits<double>::epsilon()};
            real_tolerance = std::max(real_tolerance, std::abs(s_standard_orbit[i].real()) * RELATIVE_TOLERANCE);
            imag_tolerance = std::max(imag_tolerance, std::abs(s_standard_orbit[i].imag()) * RELATIVE_TOLERANCE);
        }
        EXPECT_NEAR(s_standard_orbit[i].real(), s_perturbation_orbit[first + i].real(), real_tolerance);
        EXPECT_NEAR(s_standard_orbit[i].imag(), s_perturbation_orbit[first + i].imag(), imag_tolerance);
    }
}

TEST_P(TestPerturbationIterations, incompleteReferenceIsNotPublishedOrUsedForPixels)
{
    g_max_iterations = 5000;
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    initialize_reference({0.125, 0.125});

    EXPECT_FALSE(m_engine.initialize_pixel_strategy());
    EXPECT_EQ(2048, s_reference_coordinates.size());
    EXPECT_EQ(0, m_engine.reference_count());
    const int stack{save_stack()};
    EXPECT_FALSE(m_engine.initialize_pixel_strategy());
    EXPECT_EQ(4096, s_reference_coordinates.size());
    EXPECT_EQ(0, m_engine.reference_count());
    EXPECT_EQ(stack, save_stack());
    EXPECT_TRUE(m_engine.initialize_pixel_strategy());
    EXPECT_EQ(5000, s_reference_coordinates.size());
    EXPECT_EQ(1, m_engine.reference_count());
    EXPECT_EQ(stack, save_stack());
    EXPECT_TRUE(m_engine.initialize_pixel_strategy());
    EXPECT_EQ(5000, s_reference_coordinates.size());
    EXPECT_TRUE(m_plots.empty());
}

struct PerturbationOffsetInput
{
    const char *name;
    bool big_float;
    double offset;
};

void PrintTo(const PerturbationOffsetInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPerturbationReferenceOffset : public PerturbationIterationTest,
                                        public WithParamInterface<PerturbationOffsetInput>
{
public:
    TestPerturbationReferenceOffset();

protected:
    void SetUp() override;
};

TestPerturbationReferenceOffset::TestPerturbationReferenceOffset() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPerturbationReferenceOffset::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(GetParam().offset == 0.0 ? DEFAULT_PERTURBATION_TOLERANCE : 8.0);
}

TEST_P(TestPerturbationReferenceOffset, chunkedReferencesMatchAtomicContentsAndReplacementResults)
{
    const double offset{GetParam().offset};
    g_inside_method = ColorMethod::ITER;
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    s_reference_coordinates.clear();
    s_reference_values.clear();
    compare_pixel({0.125, 0.125}, 5000, offset);
    const std::vector<std::complex<double>> coordinates{s_reference_coordinates};
    const std::vector<std::complex<double>> values{s_reference_values};
    const int references{m_engine.reference_count()};
    EXPECT_EQ(offset == 0.0 ? 1 : 2, references);

    s_reference_coordinates.clear();
    s_reference_values.clear();
    compare_pixel({0.125, 0.125}, 5000, offset, 0, true);

    EXPECT_GT(m_reference_yields, 0);
    EXPECT_EQ(references, m_engine.reference_count());
    EXPECT_EQ(coordinates, s_reference_coordinates);
    EXPECT_EQ(values, s_reference_values);
}

TEST_P(TestPerturbationReferenceOffset, shortReferencesDoNotAddAYield)
{
    const double offset{GetParam().offset};

    compare_pixel({0.125, 0.125}, 128, offset, 0, true);

    EXPECT_EQ(0, m_reference_yields);
    EXPECT_EQ(offset == 0.0 ? 1 : 2, m_engine.reference_count());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationReferenceOffset,
    Values(PerturbationOffsetInput{"DoubleDefaultReference", false, 0.0},
        PerturbationOffsetInput{"DoubleReplacementReference", false, 0.125},
        PerturbationOffsetInput{"HighPrecisionDefaultReference", true, 0.0},
        PerturbationOffsetInput{"HighPrecisionReplacementReference", true, 0.125}),
    [](const TestParamInfo<PerturbationOffsetInput> &info) { return info.param.name; });

struct PerturbationPixelChunkInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
    double offset;
};

void PrintTo(const PerturbationPixelChunkInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PerturbationPixelChunkInput> perturbation_pixel_chunk_inputs()
{
    struct InsideMethod
    {
        const char *name;
        ColorMethod value;
    };

    std::vector<PerturbationPixelChunkInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const InsideMethod &inside : {InsideMethod{"Iterations", ColorMethod::ITER},
                 {"Magnitude", ColorMethod::ZMAG}, {"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61},
                 {"Fmod", ColorMethod::FMODI}, {"Atan", ColorMethod::ATANI}})
        {
            for (const double offset : {0.0, 0.125})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + inside.name +
                        (offset == 0.0 ? "_DefaultReference" : "_ReplacementReference"),
                    big_float, inside.value, offset});
            }
        }
    }
    return inputs;
}

class TestPerturbationPixelChunks : public PerturbationIterationTest,
                                    public WithParamInterface<PerturbationPixelChunkInput>
{
public:
    TestPerturbationPixelChunks();

protected:
    void SetUp() override;
};

TestPerturbationPixelChunks::TestPerturbationPixelChunks() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPerturbationPixelChunks::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = GetParam().inside;
    m_engine.set_glitch_tolerance(GetParam().offset == 0.0 ? DEFAULT_PERTURBATION_TOLERANCE : 8.0);
}

TEST_P(TestPerturbationPixelChunks, retainsColoringStatisticsAndRetryOrbit)
{
    const double offset{GetParam().offset};

    compare_pixel({0.125, 0.125}, 5000, offset);
    const std::vector<std::complex<double>> orbit{s_perturbation_orbit};

    compare_pixel({0.125, 0.125}, 5000, offset, 0, true);

    EXPECT_GE(m_reference_yields, 4);
    EXPECT_EQ(orbit, s_perturbation_orbit);
    EXPECT_EQ(offset == 0.0 ? 1 : 2, m_engine.reference_count());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationPixelChunks, ValuesIn(perturbation_pixel_chunk_inputs()),
    [](const TestParamInfo<PerturbationPixelChunkInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, incompletePixelPublishesNothingAndContinuesWithoutReplayingIterations)
{
    g_max_iterations = 5000;
    m_specific.pert_pt = record_perturbation_orbit;
    initialize_reference({0.125, 0.125});
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    const int stack{save_stack()};
    g_color_iter = -17;
    g_real_color_iter = -19;

    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_EQ(2048, s_perturbation_orbit.size());
    EXPECT_EQ(-17, g_color_iter);
    EXPECT_EQ(-19, g_real_color_iter);
    EXPECT_TRUE(m_plots.empty());
    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_EQ(4096, s_perturbation_orbit.size());
    EXPECT_TRUE(m_plots.empty());
    EXPECT_GE(m_engine.calculate_pixel(g_col, g_row, true), 0);
    EXPECT_EQ(4999, s_perturbation_orbit.size());
    EXPECT_EQ(5000, g_real_color_iter);
    EXPECT_EQ(1, m_plots.size());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestPerturbationIterations, finishingIncompletePixelDiscardsItsState)
{
    g_max_iterations = 5000;
    m_specific.pert_pt = record_perturbation_orbit;
    initialize_reference({0.125, 0.125});
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    ASSERT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    ASSERT_EQ(2048, s_perturbation_orbit.size());

    m_engine.finish();

    compare_pixel({0.125, 0.125}, 128, 0.0, 0, true);
    EXPECT_EQ(0, m_reference_yields);
}

TEST_P(TestPerturbationIterations, incompletePixelCannotBeReplacedByAnotherPixel)
{
    g_max_iterations = 5000;
    initialize_reference({0.125, 0.125});
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    ASSERT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));

    EXPECT_THROW(m_engine.calculate_pixel(g_col - 1, g_row, true), std::logic_error);
    EXPECT_TRUE(m_plots.empty());
}

TEST_P(TestPerturbationIterations, periodicityReplaySurvivesPixelChunkBoundaries)
{
    g_periodicity_check = 1;
    g_inside_method = ColorMethod::ITER;
    g_old_color_iter = 4500;
    compare_pixel({0.125, 0.125}, 5000);
    const std::vector<std::complex<double>> orbit{s_perturbation_orbit};
    g_old_color_iter = 4500;

    compare_pixel({0.125, 0.125}, 5000, 0.0, 0, true);

    EXPECT_GE(m_reference_yields, 4);
    EXPECT_EQ(orbit, s_perturbation_orbit);
    EXPECT_LT(s_perturbation_orbit.size(), 4999);
}

TEST_P(TestPerturbationIterations, pixelFailureAfterYieldPreservesTheErrorAndReleasesPixelState)
{
    g_max_iterations = 5000;
    initialize_reference({0.125, 0.125});
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    const int stack{save_stack()};
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        if (s_perturbation_orbit.size() == 2048)
        {
            throw std::bad_array_new_length{};
        }
        record_perturbation_orbit(reference, delta, parameter);
    };
    ASSERT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));

    EXPECT_THROW(m_engine.calculate_pixel(g_col, g_row, true), std::bad_array_new_length);

    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(1, m_engine.reference_count());
    EXPECT_TRUE(m_plots.empty());
    compare_pixel({0.125, 0.125}, 128, 0.0, 0, true);
    EXPECT_EQ(0, m_reference_yields);
}

TEST_P(TestPerturbationIterations, latePreciseBailoutReplayIsBoundedAndDoesNotRepeatDeltaUpdates)
{
    g_max_iterations = 5000;
    initialize_reference({0.125, 0.125});
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    const int stack{save_stack()};
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        record_perturbation_orbit(reference, delta, parameter);
        delta =
            s_perturbation_orbit.size() == 4097 ? std::complex<double>{2.0, 0.0} - reference : std::complex<double>{};
    };
    int color{CALC_YIELDED};
    int yields{};
    for (int chunk = 0; chunk < 20 && color == CALC_YIELDED; ++chunk)
    {
        const std::size_t before{s_perturbation_orbit.size() + s_reference_coordinates.size()};
        color = m_engine.calculate_pixel(g_col, g_row, true);
        EXPECT_LE(s_perturbation_orbit.size() + s_reference_coordinates.size() - before, 2048);
        if (color == CALC_YIELDED)
        {
            ++yields;
            EXPECT_TRUE(m_plots.empty());
        }
    }
    EXPECT_GE(color, 0);
    if (GetParam())
    {
        EXPECT_GE(yields, 4);
        EXPECT_EQ(4999, s_perturbation_orbit.size());
        EXPECT_EQ(4097, s_reference_coordinates.size());
        EXPECT_EQ(5000, g_real_color_iter);
    }
    EXPECT_EQ(1, m_plots.size());
    EXPECT_EQ(stack, save_stack());
}

TEST_P(TestPerturbationIterations, referenceFailureAfterYieldDiscardsPartialStorageAndPreservesError)
{
    g_max_iterations = 5000;
    m_specific.pert_ref = [](const std::complex<double> &center, std::complex<double> &z)
    {
        if (s_reference_coordinates.size() == 2048)
        {
            throw std::bad_array_new_length{};
        }
        record_reference(center, z);
    };
    m_specific.pert_ref_bf = [](const BFComplex &center, BFComplex &z)
    {
        if (s_reference_coordinates.size() == 2048)
        {
            throw std::bad_array_new_length{};
        }
        record_reference_bf(center, z);
    };
    initialize_reference({0.125, 0.125});
    ASSERT_FALSE(m_engine.initialize_pixel_strategy());

    EXPECT_THROW(m_engine.initialize_pixel_strategy(), std::bad_array_new_length);

    EXPECT_EQ(0, m_engine.reference_count());
    EXPECT_TRUE(m_plots.empty());
    m_engine.finish();
    m_specific.pert_ref = mandel_ref_pt;
    m_specific.pert_ref_bf = mandel_ref_pt_bf;
    compare_pixel({0.125, 0.125}, 128);
}

TEST_P(TestPerturbationIterations, replacementFailureAfterYieldDoesNotPublishOrPlotPartialReference)
{
    g_max_iterations = 5000;
    m_engine.set_glitch_tolerance(8.0);
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    initialize_reference({0.0, 0.125}, 0.125);
    for (int chunk = 0; chunk < 8 && m_engine.reference_count() == 0; ++chunk)
    {
        m_engine.initialize_pixel_strategy();
    }
    ASSERT_EQ(1, m_engine.reference_count());
    m_specific.pert_ref = [](const std::complex<double> &center, std::complex<double> &z)
    {
        if (s_reference_coordinates.size() == 5000 + 2048)
        {
            throw std::bad_array_new_length{};
        }
        record_reference(center, z);
    };
    m_specific.pert_ref_bf = [](const BFComplex &center, BFComplex &z)
    {
        if (s_reference_coordinates.size() == 5000 + 2048)
        {
            throw std::bad_array_new_length{};
        }
        record_reference_bf(center, z);
    };

    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(WIDTH - 1, HEIGHT / 2 - 1, true));
    EXPECT_EQ(1, m_engine.reference_count());
    EXPECT_EQ(5000 + 2048, s_reference_coordinates.size());
    EXPECT_THROW(m_engine.calculate_pixel(WIDTH - 1, HEIGHT / 2 - 1, true), std::bad_array_new_length);

    EXPECT_EQ(0, m_engine.reference_count());
    EXPECT_TRUE(m_plots.empty());
}

TEST_F(TestPerturbationPixels, finishingPartialReferenceReleasesPersistentBigFloatValues)
{
    g_max_iterations = 5000;
    init_bf_dec(60);
    m_big_float = true;
    const int stack{save_stack()};
    for (int frame = 0; frame < 2; ++frame)
    {
        const BFComplex center{m_engine.initialize_frame_bf({}, {})};
        float_to_bf(center.x, 0.125);
        float_to_bf(center.y, 0.125);
        ASSERT_FALSE(m_engine.initialize_pixel_strategy());
        EXPECT_GT(save_stack(), stack);

        m_engine.finish();

        EXPECT_EQ(stack, save_stack());
        EXPECT_EQ(0, m_engine.reference_count());
    }
}

struct InitialOffsetInput
{
    const char *name;
    bool big_float;
    DComplex offset;
    std::complex<double> expected;
};

void PrintTo(const InitialOffsetInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPerturbationInitialOffset : public PerturbationIterationTest, public WithParamInterface<InitialOffsetInput>
{
public:
    TestPerturbationInitialOffset();

protected:
    void SetUp() override;
};

TestPerturbationInitialOffset::TestPerturbationInitialOffset() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPerturbationInitialOffset::SetUp()
{
    PerturbationIterationTest::SetUp();
    set_initial_offset(GetParam().offset);
}

TEST_P(TestPerturbationInitialOffset, matchesStandardOrbit)
{
    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ(GetParam().expected, s_perturbation_orbit.front());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationInitialOffset,
    Values(InitialOffsetInput{"DoubleRealOffset", false, {0.125, 0.0}, {0.375, 0.125}},
        InitialOffsetInput{"DoubleImaginaryOffset", false, {0.0, -0.0625}, {0.25, 0.0625}},
        InitialOffsetInput{"HighPrecisionRealOffset", true, {0.125, 0.0}, {0.375, 0.125}},
        InitialOffsetInput{"HighPrecisionImaginaryOffset", true, {0.0, -0.0625}, {0.25, 0.0625}}),
    [](const TestParamInfo<InitialOffsetInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, combinedInitialOffsetsDoNotChangeTheRecurrenceParameter)
{
    set_initial_offset({0.125, -0.0625});
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ((std::complex<double>{0.375, 0.0625}), s_perturbation_orbit.front());
    EXPECT_THAT(s_reference_coordinates, Each(Eq(std::complex<double>{0.25, 0.125})));
}

struct BofInitializationInput
{
    std::string name;
    bool big_float;
    ColorMethod method;
    bool book;
};

void PrintTo(const BofInitializationInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BofInitializationInput> perturbation_bof_inputs()
{
    struct BofMethod
    {
        const char *name;
        ColorMethod value;
    };

    std::vector<BofInitializationInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const BofMethod &method : {BofMethod{"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61}})
        {
            for (const bool book : {false, true})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + method.name +
                        (book ? "_Book" : "_Ordinary"),
                    big_float, method.value, book});
            }
        }
    }
    return inputs;
}

class TestPerturbationBofInitialization : public PerturbationIterationTest,
                                          public WithParamInterface<BofInitializationInput>
{
public:
    TestPerturbationBofInitialization();

protected:
    void SetUp() override;
};

TestPerturbationBofInitialization::TestPerturbationBofInitialization() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPerturbationBofInitialization::SetUp()
{
    PerturbationIterationTest::SetUp();
    set_initial_offset({0.125, -0.0625});
    g_inside_method = GetParam().method;
    g_bof_match_book_images = GetParam().book;
}

TEST_P(TestPerturbationBofInitialization, matchesStandardCompatibilityPolicy)
{
    const bool book{GetParam().book};

    compare_pixel({0.25, 0.125}, 8);

    const std::complex<double> expected{
        book ? std::complex<double>{0.125, -0.0625} : std::complex<double>{0.375, 0.0625}};
    EXPECT_EQ(expected, s_perturbation_orbit.front());
    EXPECT_EQ(book ? 8 : 7, s_perturbation_orbit.size());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationBofInitialization, ValuesIn(perturbation_bof_inputs()),
    [](const TestParamInfo<BofInitializationInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, bofFirstUpdateCanEscapeAtIterationZero)
{
    set_initial_offset({2.0, 0.0});
    g_inside_method = ColorMethod::BOF60;
    g_bof_match_book_images = true;

    compare_pixel({0.25, 0.0}, 8);

    EXPECT_EQ(0, g_real_color_iter);
    EXPECT_EQ(1, g_color);
    ASSERT_EQ(1, s_perturbation_orbit.size());
    EXPECT_EQ((std::complex<double>{2.0, 0.0}), s_perturbation_orbit.front());
}

struct InitialOffsetReplacementInput
{
    std::string name;
    bool big_float;
    bool book;
    double tolerance;
};

void PrintTo(const InitialOffsetReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InitialOffsetReplacementInput> initial_offset_replacement_inputs()
{
    std::vector<InitialOffsetReplacementInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool book : {false, true})
        {
            for (const double tolerance : {0.0, 2.0})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (book ? "Book_" : "Ordinary_") + (tolerance == 0.0 ? "NoReplacement" : "Replacement"),
                    big_float, book, tolerance});
            }
        }
    }
    return inputs;
}

class TestInitialOffsetReplacement : public PerturbationIterationTest,
                                     public WithParamInterface<InitialOffsetReplacementInput>
{
public:
    TestInitialOffsetReplacement();

protected:
    void SetUp() override;
};

TestInitialOffsetReplacement::TestInitialOffsetReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInitialOffsetReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    set_initial_offset({0.125, -0.0625});
    g_inside_method = GetParam().book ? ColorMethod::BOF60 : ColorMethod::ITER;
    g_bof_match_book_images = GetParam().book;
    m_engine.set_glitch_tolerance(GetParam().tolerance);
}

TEST_P(TestInitialOffsetReplacement, preservesDisplacedInitialization)
{
    compare_pixel({0.25, 0.125}, 8, 0.125);

    if (GetParam().tolerance == 0.0)
    {
        EXPECT_EQ(1, m_engine.reference_count());
    }
    else
    {
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInitialOffsetReplacement, ValuesIn(initial_offset_replacement_inputs()),
    [](const TestParamInfo<InitialOffsetReplacementInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, fixedZeroInitialOrbitMatchesStandardOrbit)
{
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {};

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ(std::complex<double>{}, s_perturbation_orbit.front());
}

TEST_P(TestPerturbationIterations, fixedComplexInitialOrbitIncludesParameterOffsets)
{
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {0.25, 0.125};
    set_initial_offset({0.125, -0.0625});
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;

    compare_pixel({0.125, 0.0625}, 8);

    EXPECT_EQ((std::complex<double>{0.375, 0.0625}), s_perturbation_orbit.front());
    EXPECT_THAT(s_reference_coordinates, Each(Eq(std::complex<double>{0.125, 0.0625})));
}

TEST_P(TestPerturbationIterations, pixelInitializationIgnoresTheFixedInitialOrbit)
{
    g_use_init_orbit = InitOrbitMode::PIXEL;
    g_init_orbit = {1.5, 2.5};
    set_initial_offset({0.125, -0.0625});

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ((std::complex<double>{0.375, 0.0625}), s_perturbation_orbit.front());
}

class TestFixedBofInitialization : public TestPerturbationBofInitialization
{
protected:
    void SetUp() override;
};

void TestFixedBofInitialization::SetUp()
{
    TestPerturbationBofInitialization::SetUp();
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {0.25, 0.125};
}

TEST_P(TestFixedBofInitialization, retainsCompatibilityPrecedence)
{
    const bool book{GetParam().book};

    compare_pixel({0.125, 0.0625}, 8);

    const std::complex<double> expected{
        book ? std::complex<double>{0.125, -0.0625} : std::complex<double>{0.375, 0.0625}};
    EXPECT_EQ(expected, s_perturbation_orbit.front());
    EXPECT_EQ(book ? 8 : 7, s_perturbation_orbit.size());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFixedBofInitialization, ValuesIn(perturbation_bof_inputs()),
    [](const TestParamInfo<BofInitializationInput> &info) { return info.param.name; });

struct InitialOrbitReplacementInput
{
    std::string name;
    bool big_float;
    InitOrbitMode mode;
    DComplex initial;
    double tolerance;
};

void PrintTo(const InitialOrbitReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InitialOrbitReplacementInput> initial_orbit_replacement_inputs()
{
    struct InitialValue
    {
        const char *name;
        DComplex value;
    };

    std::vector<InitialOrbitReplacementInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const InitOrbitMode mode : {InitOrbitMode::VALUE, InitOrbitMode::PIXEL})
        {
            for (const InitialValue &initial : {InitialValue{"Zero", {}}, {"Complex", {0.25, 0.125}}})
            {
                for (const double tolerance : {0.0, 2.0})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                            (mode == InitOrbitMode::VALUE ? "Value_" : "Pixel_") + initial.name +
                            (tolerance == 0.0 ? "_NoReplacement" : "_Replacement"),
                        big_float, mode, initial.value, tolerance});
                }
            }
        }
    }
    return inputs;
}

class TestInitialOrbitReplacement : public PerturbationIterationTest,
                                    public WithParamInterface<InitialOrbitReplacementInput>
{
public:
    TestInitialOrbitReplacement();

protected:
    void SetUp() override;
};

TestInitialOrbitReplacement::TestInitialOrbitReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInitialOrbitReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    set_initial_offset({0.125, -0.0625});
    g_use_init_orbit = GetParam().mode;
    g_init_orbit = GetParam().initial;
    m_engine.set_glitch_tolerance(GetParam().tolerance);
}

TEST_P(TestInitialOrbitReplacement, preservesDisplacedInitialOrbit)
{
    compare_pixel({0.25, 0.125}, 8, 0.125);

    if (GetParam().tolerance == 0.0)
    {
        EXPECT_EQ(1, m_engine.reference_count());
    }
    else
    {
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInitialOrbitReplacement, ValuesIn(initial_orbit_replacement_inputs()),
    [](const TestParamInfo<InitialOrbitReplacementInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, firstUpdateEscapeMatchesStandardState)
{
    compare_pixel({1.0, 0.0}, 8);
    EXPECT_EQ(1, g_real_color_iter);
}

struct PerturbationColorMethod
{
    const char *name;
    ColorMethod method;
};

constexpr std::array<PerturbationColorMethod, 5> SPECIAL_OUTSIDE_METHODS{
    {{"Real", ColorMethod::REAL}, {"Imaginary", ColorMethod::IMAG}, {"Product", ColorMethod::MULT},
        {"Sum", ColorMethod::SUM}, {"Atan", ColorMethod::ATAN}}};
constexpr std::array<PerturbationColorMethod, 4> ORBIT_INSIDE_METHODS{{{"Iterations", ColorMethod::ITER},
    {"Magnitude", ColorMethod::ZMAG}, {"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61}}};

struct OutsideTerminalInput
{
    std::string name;
    bool big_float;
    ColorMethod outside;
    ColorMethod inside;
    std::complex<double> point;
};

void PrintTo(const OutsideTerminalInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<OutsideTerminalInput> outside_terminal_inputs()
{
    struct Point
    {
        const char *name;
        std::complex<double> value;
    };

    std::vector<OutsideTerminalInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : SPECIAL_OUTSIDE_METHODS)
        {
            for (const PerturbationColorMethod &inside : ORBIT_INSIDE_METHODS)
            {
                for (const Point &point : {Point{"UpperRight", {1.0, 0.5}}, {"LowerRight", {1.0, -0.5}},
                         {"UpperLeft", {-2.0, 1.0}}, {"LowerLeft", {-2.0, -1.0}}, {"PositiveImaginary", {0.0, 2.0}},
                         {"NegativeImaginary", {0.0, -2.0}}, {"PositiveReal", {1.0, 0.0}},
                         {"NegativeReal", {-3.0, 0.0}}})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + outside.name + "_" +
                            inside.name + "_" + point.name,
                        big_float, outside.method, inside.method, point.value});
                }
            }
        }
    }
    return inputs;
}

class TestOutsideTerminalColor : public PerturbationIterationTest, public WithParamInterface<OutsideTerminalInput>
{
public:
    TestOutsideTerminalColor();

protected:
    void SetUp() override;
};

TestOutsideTerminalColor::TestOutsideTerminalColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideTerminalColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_version = current_id_version();
    init_atan_colors();
    g_bof_match_book_images = false;
    g_outside_method = GetParam().outside;
    g_inside_method = GetParam().inside;
}

TEST_P(TestOutsideTerminalColor, matchesStandardTerminalOrbit)
{
    compare_pixel(GetParam().point, 512);

    EXPECT_LT(g_real_color_iter, g_max_iterations);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideTerminalColor, ValuesIn(outside_terminal_inputs()),
    [](const TestParamInfo<OutsideTerminalInput> &info) { return info.param.name; });

struct OutsideRangeSample
{
    const char *name;
    ColorMethod method;
    std::complex<double> point;
    long max_iterations;
    int color;
    bool corrected;
};

struct OutsideRangeInput
{
    std::string name;
    bool big_float;
    Version version;
    OutsideRangeSample sample;
};

void PrintTo(const OutsideRangeInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<OutsideRangeInput> outside_range_inputs()
{
    struct CompatibilityVersion
    {
        const char *name;
        Version version;
    };

    const OutsideRangeSample samples[]{
        {"RealPositive", ColorMethod::REAL, {1.0, 0.0}, 128, 10, false},
        {"RealZero", ColorMethod::REAL, {0.0, 3.0}, 128, 1, true},
        {"RealSmallPositive", ColorMethod::REAL, {0.5, 3.0}, 128, 1, true},
        {"RealOverIterationLimit", ColorMethod::REAL, {8.0, 0.0}, 16, 1, true},
        {"ImaginaryPositive", ColorMethod::IMAG, {0.0, 3.0}, 128, 11, false},
        {"ImaginaryNegativeTen", ColorMethod::IMAG, {0.0, -10.0}, 128, 1, true},
        {"ImaginaryNegativeEight", ColorMethod::IMAG, {0.0, -8.0}, 128, 1, true},
        {"ImaginaryOverIterationLimit", ColorMethod::IMAG, {1.0, 8.0}, 16, 1, true},
        {"ProductPositive", ColorMethod::MULT, {1.0, 0.5}, 128, 1, false},
        {"ProductNegativeReal", ColorMethod::MULT, {-2.0, 0.5}, 128, 1, true},
        {"ProductNegativeImaginary", ColorMethod::MULT, {1.0, -1.0}, 128, 1, true},
        {"ProductOverIterationLimit", ColorMethod::MULT, {2.0, 0.125}, 8, 1, true},
        {"SumPositive", ColorMethod::SUM, {1.0, 1.0}, 128, 5, false},
        {"SumZeroReal", ColorMethod::SUM, {0.0, 3.0}, 128, 1, true},
        {"SumSmallImaginary", ColorMethod::SUM, {0.0, 1.75}, 128, 1, true},
        {"SumOverIterationLimit", ColorMethod::SUM, {8.0, 1.0}, 16, 1, true},
        {"AtanPositiveReal", ColorMethod::ATAN, {1.0, 0.0}, 16, 1, true},
        {"AtanPositiveImaginary", ColorMethod::ATAN, {0.0, 2.0}, 16, 1, true},
    };

    std::vector<OutsideRangeInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const CompatibilityVersion &version : {CompatibilityVersion{"Legacy1960", parse_legacy_version(1960)},
                 {"Legacy1961", parse_legacy_version(1961)}, {"Current", current_id_version()}})
        {
            for (const OutsideRangeSample &sample : samples)
            {
                inputs.push_back(
                    {std::string{big_float ? "HighPrecision_" : "Double_"} + version.name + "_" + sample.name,
                        big_float, version.version, sample});
            }
        }
    }
    return inputs;
}

class TestOutsideRangeCorrection : public PerturbationIterationTest, public WithParamInterface<OutsideRangeInput>
{
public:
    TestOutsideRangeCorrection();

protected:
    void SetUp() override;
};

TestOutsideRangeCorrection::TestOutsideRangeCorrection() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideRangeCorrection::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_version = GetParam().version;
    init_atan_colors();
    g_outside_method = GetParam().sample.method;
}

TEST_P(TestOutsideRangeCorrection, matchesStandardRangeCorrection)
{
    const OutsideRangeSample &sample{GetParam().sample};

    compare_pixel(sample.point, sample.max_iterations);

    EXPECT_EQ(sample.corrected && g_version < 1961 ? 0 : sample.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideRangeCorrection, ValuesIn(outside_range_inputs()),
    [](const TestParamInfo<OutsideRangeInput> &info) { return info.param.name; });

struct OutsideAtanInput
{
    std::string name;
    bool big_float;
    Version version;
    int colors;
    double imag;
};

void PrintTo(const OutsideAtanInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<OutsideAtanInput> outside_atan_inputs()
{
    struct CompatibilityVersion
    {
        const char *name;
        Version version;
    };

    std::vector<OutsideAtanInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const CompatibilityVersion &version : {CompatibilityVersion{"Legacy2002", parse_legacy_version(2002)},
                 {"Legacy2003", parse_legacy_version(2003)}, {"Current", current_id_version()}})
        {
            for (const int colors : {16, 256})
            {
                for (const double imag : {2.0, -2.0})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + version.name + "_Colors" +
                            std::to_string(colors) + (imag > 0.0 ? "_PositiveImaginary" : "_NegativeImaginary"),
                        big_float, version.version, colors, imag});
                }
            }
        }
    }
    return inputs;
}

class TestOutsideAtanScale : public PerturbationIterationTest, public WithParamInterface<OutsideAtanInput>
{
public:
    TestOutsideAtanScale();

protected:
    void SetUp() override;
};

TestOutsideAtanScale::TestOutsideAtanScale() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideAtanScale::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::ATAN;
    g_version = GetParam().version;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
    init_atan_colors();
}

TEST_P(TestOutsideAtanScale, usesVersionAndPaletteScale)
{
    const int colors{GetParam().colors};
    const int expected{g_version <= 2002 ? colors == 16 ? 3 : 153 : colors == 16 ? 13 : 218};

    compare_pixel({0.0, GetParam().imag}, 512);

    EXPECT_EQ(expected, g_color);
    EXPECT_EQ(1, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideAtanScale, ValuesIn(outside_atan_inputs()),
    [](const TestParamInfo<OutsideAtanInput> &info) { return info.param.name; });

std::vector<OutsideTerminalInput> outside_inside_inputs()
{
    std::vector<OutsideTerminalInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : SPECIAL_OUTSIDE_METHODS)
        {
            for (const PerturbationColorMethod &inside : ORBIT_INSIDE_METHODS)
            {
                inputs.push_back(
                    {std::string{big_float ? "HighPrecision_" : "Double_"} + outside.name + "_" + inside.name,
                        big_float, outside.method, inside.method, {}});
            }
        }
    }
    return inputs;
}

class TestOutsideInsideColor : public PerturbationIterationTest, public WithParamInterface<OutsideTerminalInput>
{
public:
    TestOutsideInsideColor();

protected:
    void SetUp() override;
};

TestOutsideInsideColor::TestOutsideInsideColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideInsideColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_bof_match_book_images = false;
    g_outside_method = GetParam().outside;
    g_inside_method = GetParam().inside;
}

TEST_P(TestOutsideInsideColor, leavesInsidePixelsUnchanged)
{
    compare_pixel(GetParam().point, 128);

    EXPECT_EQ(128, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideInsideColor, ValuesIn(outside_inside_inputs()),
    [](const TestParamInfo<OutsideTerminalInput> &info) { return info.param.name; });

std::vector<OutsideTerminalInput> outside_replacement_inputs()
{
    std::vector<OutsideTerminalInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : SPECIAL_OUTSIDE_METHODS)
        {
            inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + outside.name, big_float,
                outside.method, ColorMethod::ITER, {-1.0, 1.0}});
        }
    }
    return inputs;
}

class TestOutsideReferenceReplacement : public PerturbationIterationTest,
                                        public WithParamInterface<OutsideTerminalInput>
{
public:
    TestOutsideReferenceReplacement();

protected:
    void SetUp() override;
};

TestOutsideReferenceReplacement::TestOutsideReferenceReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideReferenceReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_version = current_id_version();
    init_atan_colors();
    g_inside_method = GetParam().inside;
    m_engine.set_glitch_tolerance(2.0);
    g_outside_method = GetParam().outside;
}

TEST_P(TestOutsideReferenceReplacement, preservesOutsideColor)
{
    compare_pixel(GetParam().point, 512, 0.125);

    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideReferenceReplacement, ValuesIn(outside_replacement_inputs()),
    [](const TestParamInfo<OutsideTerminalInput> &info) { return info.param.name; });

struct FmodMetricInput
{
    std::string name;
    bool big_float;
    Bailout bailout;
    double proximity;
};

struct FmodProximity
{
    const char *name;
    double value;
};

void PrintTo(const FmodMetricInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<FmodMetricInput> fmod_metric_inputs(const std::vector<FmodProximity> &proximities)
{
    struct Metric
    {
        const char *name;
        Bailout value;
    };

    std::vector<FmodMetricInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const Metric &metric :
            {Metric{"Mod", Bailout::MOD}, {"Real", Bailout::REAL}, {"Imaginary", Bailout::IMAG}, {"Or", Bailout::OR},
                {"And", Bailout::AND}, {"Manhattan", Bailout::MANH}, {"ManhattanReal", Bailout::MANR}})
        {
            for (const FmodProximity &proximity : proximities)
            {
                inputs.push_back(
                    {std::string{big_float ? "HighPrecision_" : "Double_"} + metric.name + "_" + proximity.name,
                        big_float, metric.value, proximity.value});
            }
        }
    }
    return inputs;
}

class TestFmodMetric : public PerturbationIterationTest, public WithParamInterface<FmodMetricInput>
{
public:
    TestFmodMetric();

protected:
    void SetUp() override;
};

TestFmodMetric::TestFmodMetric() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestFmodMetric::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_bailout_test = GetParam().bailout;
    set_bailout_formula(GetParam().bailout);
    g_close_proximity = GetParam().proximity;
}

class TestOutsideFmodMetric : public TestFmodMetric
{
protected:
    void SetUp() override;
};

void TestOutsideFmodMetric::SetUp()
{
    TestFmodMetric::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::FMOD;
}

TEST_P(TestOutsideFmodMetric, matchesStandardBailoutMetric)
{
    compare_pixel({0.5, 0.5}, 8);

    EXPECT_LT(g_real_color_iter, g_max_iterations);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideFmodMetric,
    ValuesIn(fmod_metric_inputs({{"Half", 0.5}, {"OneAndQuarter", 1.25}, {"Two", 2.0}, {"TwoAndFiveSixteenths", 2.3125},
        {"Four", 4.0}, {"Sixteen", 16.0}, {"NegativeSixteen", -16.0}})),
    [](const TestParamInfo<FmodMetricInput> &info) { return info.param.name; });

class TestInsideFmodMetric : public TestFmodMetric
{
protected:
    void SetUp() override;
};

void TestInsideFmodMetric::SetUp()
{
    TestFmodMetric::SetUp();
    g_inside_method = ColorMethod::FMODI;
}

TEST_P(TestInsideFmodMetric, matchesStandardBailoutMetric)
{
    compare_pixel({0.125, 0.0625}, 8);

    EXPECT_EQ(8, g_real_color_iter);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideFmodMetric,
    ValuesIn(fmod_metric_inputs(
        {{"OneThousandth", 0.001}, {"OneHundredth", 0.01}, {"OneTenth", 0.1}, {"NegativeOneTenth", -0.1}})),
    [](const TestParamInfo<FmodMetricInput> &info) { return info.param.name; });

struct OutsideFmodSample
{
    const char *name;
    double proximity;
    int colors;
    int expected_color;
    std::optional<long> expected_iterations;
};

struct OutsideFmodInput
{
    std::string name;
    bool big_float;
    OutsideFmodSample sample;
};

void PrintTo(const OutsideFmodInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<OutsideFmodInput> outside_fmod_inputs()
{
    const OutsideFmodSample samples[]{
        {"ProximitySixteen", 16.0, 256, 46, 4},
        {"ProximityFour", 4.0, 256, 186, 4},
        {"ProximityTwoAndFiveSixteenths", 2.3125, 256, 138, 4},
        {"ProximityOneAndQuarter", 1.25, 256, 0, 4},
        {"ProximityNegativeSixteen", -16.0, 256, 0, 4},
        {"PaletteFour", 16.0, 4, 0, std::nullopt},
        {"PaletteSixteen", 16.0, 16, 2, std::nullopt},
        {"PaletteTwoHundredFiftySix", 16.0, 256, 46, std::nullopt},
    };

    std::vector<OutsideFmodInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const OutsideFmodSample &sample : samples)
        {
            inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + sample.name, big_float, sample});
        }
    }
    return inputs;
}

class TestOutsideFmodColor : public PerturbationIterationTest, public WithParamInterface<OutsideFmodInput>
{
public:
    TestOutsideFmodColor();

protected:
    void SetUp() override;
};

TestOutsideFmodColor::TestOutsideFmodColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestOutsideFmodColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::FMOD;
    g_close_proximity = GetParam().sample.proximity;
    g_colors = GetParam().sample.colors;
    g_and_color = g_colors - 1;
}

TEST_P(TestOutsideFmodColor, retainsLastQualifiedNonescapingValue)
{
    const OutsideFmodSample &sample{GetParam().sample};

    compare_pixel({0.5, 0.5}, 8);

    if (sample.expected_iterations)
    {
        EXPECT_EQ(*sample.expected_iterations, g_real_color_iter);
    }
    EXPECT_EQ(sample.expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOutsideFmodColor, ValuesIn(outside_fmod_inputs()),
    [](const TestParamInfo<OutsideFmodInput> &info) { return info.param.name; });

struct FmodOrbitInput
{
    std::string name;
    bool big_float;
    double proximity;
    std::complex<double> point;
    long iterations;
    int expected_color;
};

void PrintTo(const FmodOrbitInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<FmodOrbitInput> inside_fmod_value_inputs()
{
    std::vector<FmodOrbitInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const FmodProximity &proximity : {FmodProximity{"Two", 2.0}, {"One", 1.0}, {"NegativeTwo", -2.0}})
        {
            for (const long iterations : {2L, 3L, 4L, 5L})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + proximity.name +
                        "_Iterations" + std::to_string(iterations),
                    big_float, proximity.value, {-1.0, 0.0}, iterations,
                    proximity.value == 2.0 && iterations % 2 != 0 ? 128 : 0});
            }
        }
    }
    return inputs;
}

class TestInsideFmodValue : public PerturbationIterationTest, public WithParamInterface<FmodOrbitInput>
{
public:
    TestInsideFmodValue();

protected:
    void SetUp() override;
};

TestInsideFmodValue::TestInsideFmodValue() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsideFmodValue::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::FMODI;
    g_close_proximity = GetParam().proximity;
}

TEST_P(TestInsideFmodValue, retainsLastValueRatherThanMinimum)
{
    compare_pixel(GetParam().point, GetParam().iterations);

    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
    EXPECT_EQ(GetParam().expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideFmodValue, ValuesIn(inside_fmod_value_inputs()),
    [](const TestParamInfo<FmodOrbitInput> &info) { return info.param.name; });

class TestCombinedFmodSamples : public TestInsideFmodValue
{
protected:
    void SetUp() override;
};

void TestCombinedFmodSamples::SetUp()
{
    TestInsideFmodValue::SetUp();
    g_outside_method = ColorMethod::FMOD;
}

TEST_P(TestCombinedFmodSamples, sharesInsideAndOutsideOrbitSamples)
{
    compare_pixel(GetParam().point, GetParam().iterations);

    EXPECT_EQ(GetParam().expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCombinedFmodSamples,
    Values(FmodOrbitInput{"DoubleOutside", false, 16.0, {0.5, 0.5}, 8, 46},
        FmodOrbitInput{"DoubleInside", false, 2.0, {-1.0, 0.0}, 5, 128},
        FmodOrbitInput{"HighPrecisionOutside", true, 16.0, {0.5, 0.5}, 8, 46},
        FmodOrbitInput{"HighPrecisionInside", true, 2.0, {-1.0, 0.0}, 5, 128}),
    [](const TestParamInfo<FmodOrbitInput> &info) { return info.param.name; });

struct FmodReplacementInput
{
    const char *name;
    bool big_float;
    bool inside;
};

void PrintTo(const FmodReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFmodReferenceReplacement : public PerturbationIterationTest, public WithParamInterface<FmodReplacementInput>
{
public:
    TestFmodReferenceReplacement();

protected:
    void SetUp() override;
};

TestFmodReferenceReplacement::TestFmodReferenceReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestFmodReferenceReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(8.0);
    const bool inside{GetParam().inside};
    g_inside_method = inside ? ColorMethod::FMODI : ColorMethod::ITER;
    g_outside_method = inside ? ColorMethod::ITER : ColorMethod::FMOD;
    g_close_proximity = inside ? 2.0 : 16.0;
}

TEST_P(TestFmodReferenceReplacement, preservesFmodColor)
{
    const bool inside{GetParam().inside};

    compare_pixel(inside ? std::complex<double>{-1.0, 0.0} : std::complex<double>{0.5, 0.5}, inside ? 5 : 8, 0.125);

    EXPECT_EQ(inside ? 128 : 46, g_color);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFmodReferenceReplacement,
    Values(FmodReplacementInput{"DoubleOutside", false, false}, FmodReplacementInput{"DoubleInside", false, true},
        FmodReplacementInput{"HighPrecisionOutside", true, false},
        FmodReplacementInput{"HighPrecisionInside", true, true}),
    [](const TestParamInfo<FmodReplacementInput> &info) { return info.param.name; });

struct TotalDistanceInput
{
    const char *name;
    bool big_float;
    std::complex<double> point;
    long expected_iterations;
    int expected_color;
};

void PrintTo(const TotalDistanceInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTotalDistanceSample : public PerturbationIterationTest, public WithParamInterface<TotalDistanceInput>
{
public:
    TestTotalDistanceSample();

protected:
    void SetUp() override;
};

TestTotalDistanceSample::TestTotalDistanceSample() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistanceSample::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::TDIS;
}

TEST_P(TestTotalDistanceSample, excludesEscapingSegment)
{
    compare_pixel(GetParam().point, 8);

    EXPECT_EQ(GetParam().expected_iterations, g_real_color_iter);
    EXPECT_EQ(GetParam().expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceSample,
    Values(TotalDistanceInput{"DoubleUpperRight", false, {0.5, 0.5}, 4, 3},
        TotalDistanceInput{"DoubleLowerRight", false, {0.5, -0.5}, 4, 3},
        TotalDistanceInput{"DoubleUpperLeft", false, {-1.0, 1.0}, 2, 2},
        TotalDistanceInput{"DoubleRealAxis", false, {0.5, 0.0}, 4, 1},
        TotalDistanceInput{"HighPrecisionUpperRight", true, {0.5, 0.5}, 4, 3},
        TotalDistanceInput{"HighPrecisionLowerRight", true, {0.5, -0.5}, 4, 3},
        TotalDistanceInput{"HighPrecisionUpperLeft", true, {-1.0, 1.0}, 2, 2},
        TotalDistanceInput{"HighPrecisionRealAxis", true, {0.5, 0.0}, 4, 1}),
    [](const TestParamInfo<TotalDistanceInput> &info) { return info.param.name; });

class TestTotalDistanceFixedSeed : public TestTotalDistanceSample
{
protected:
    void SetUp() override;
};

void TestTotalDistanceFixedSeed::SetUp()
{
    TestTotalDistanceSample::SetUp();
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {-0.5, 0.0};
}

TEST_P(TestTotalDistanceFixedSeed, startsAtFixedSeed)
{
    compare_pixel(GetParam().point, 8);

    EXPECT_EQ(GetParam().expected_iterations, g_real_color_iter);
    EXPECT_EQ(GetParam().expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceFixedSeed,
    Values(TotalDistanceInput{"Double", false, {0.5, 0.0}, 4, 2},
        TotalDistanceInput{"HighPrecision", true, {0.5, 0.0}, 4, 2}),
    [](const TestParamInfo<TotalDistanceInput> &info) { return info.param.name; });

struct TotalDistanceOffsetInput
{
    const char *name;
    bool big_float;
    InitOrbitMode mode;
};

void PrintTo(const TotalDistanceOffsetInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTotalDistanceOffset : public PerturbationIterationTest, public WithParamInterface<TotalDistanceOffsetInput>
{
public:
    TestTotalDistanceOffset();

protected:
    void SetUp() override;
};

TestTotalDistanceOffset::TestTotalDistanceOffset() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistanceOffset::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::TDIS;
    g_use_init_orbit = GetParam().mode;
    g_init_orbit = {-0.5, 0.25};
    set_initial_offset({-0.25, 0.125});
}

TEST_P(TestTotalDistanceOffset, startsAtOffsetSeed)
{
    compare_pixel({0.5, 0.5}, 8);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceOffset,
    Values(TotalDistanceOffsetInput{"DoubleNormal", false, InitOrbitMode::NORMAL},
        TotalDistanceOffsetInput{"DoublePixel", false, InitOrbitMode::PIXEL},
        TotalDistanceOffsetInput{"DoubleValue", false, InitOrbitMode::VALUE},
        TotalDistanceOffsetInput{"HighPrecisionNormal", true, InitOrbitMode::NORMAL},
        TotalDistanceOffsetInput{"HighPrecisionPixel", true, InitOrbitMode::PIXEL},
        TotalDistanceOffsetInput{"HighPrecisionValue", true, InitOrbitMode::VALUE}),
    [](const TestParamInfo<TotalDistanceOffsetInput> &info) { return info.param.name; });

class TestTotalDistanceBofSeed : public PerturbationIterationTest, public WithParamInterface<BofInitializationInput>
{
public:
    TestTotalDistanceBofSeed();

protected:
    void SetUp() override;
};

TestTotalDistanceBofSeed::TestTotalDistanceBofSeed() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistanceBofSeed::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_outside_method = ColorMethod::TDIS;
    g_use_init_orbit = InitOrbitMode::NORMAL;
    g_init_orbit = {-0.5, 0.25};
    set_initial_offset({-0.25, 0.125});
    g_inside_method = GetParam().method;
    g_bof_match_book_images = GetParam().book;
}

TEST_P(TestTotalDistanceBofSeed, startsAtCompatibilitySeed)
{
    compare_pixel({0.5, 0.5}, 8);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceBofSeed, ValuesIn(perturbation_bof_inputs()),
    [](const TestParamInfo<BofInitializationInput> &info) { return info.param.name; });

struct TotalDistanceRangeInput
{
    std::string name;
    bool big_float;
    Version version;
    Bailout bailout;
    std::complex<double> point;
    long expected_iterations;
};

void PrintTo(const TotalDistanceRangeInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<TotalDistanceRangeInput> total_distance_range_inputs()
{
    struct CompatibilityVersion
    {
        const char *name;
        Version version;
    };
    struct Sample
    {
        const char *name;
        Bailout bailout;
        std::complex<double> point;
        long expected_iterations;
    };

    std::vector<TotalDistanceRangeInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const CompatibilityVersion &version : {CompatibilityVersion{"Legacy1960", parse_legacy_version(1960)},
                 {"Legacy1961", parse_legacy_version(1961)}, {"Current", current_id_version()}})
        {
            for (const Sample &sample :
                {Sample{"Mod", Bailout::MOD, {1.0, 0.0}, 1}, {"Imaginary", Bailout::IMAG, {8.0, 0.0625}, 2}})
            {
                inputs.push_back(
                    {std::string{big_float ? "HighPrecision_" : "Double_"} + version.name + "_" + sample.name,
                        big_float, version.version, sample.bailout, sample.point, sample.expected_iterations});
            }
        }
    }
    return inputs;
}

class TestTotalDistanceRange : public PerturbationIterationTest, public WithParamInterface<TotalDistanceRangeInput>
{
public:
    TestTotalDistanceRange();

protected:
    void SetUp() override;
};

TestTotalDistanceRange::TestTotalDistanceRange() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistanceRange::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::TDIS;
    g_version = GetParam().version;
    g_bailout_test = GetParam().bailout;
    set_bailout_formula(GetParam().bailout);
}

TEST_P(TestTotalDistanceRange, usesStandardRangeCorrection)
{
    compare_pixel(GetParam().point, 16);

    EXPECT_EQ(GetParam().expected_iterations, g_real_color_iter);
    EXPECT_EQ(g_version < 1961 ? 0 : 1, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceRange, ValuesIn(total_distance_range_inputs()),
    [](const TestParamInfo<TotalDistanceRangeInput> &info) { return info.param.name; });

struct TotalDistancePaletteInput
{
    const char *name;
    bool big_float;
    int colors;
    int expected_color;
};

void PrintTo(const TotalDistancePaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestTotalDistancePalette : public PerturbationIterationTest, public WithParamInterface<TotalDistancePaletteInput>
{
public:
    TestTotalDistancePalette();

protected:
    void SetUp() override;
};

TestTotalDistancePalette::TestTotalDistancePalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistancePalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::TDIS;
    g_bailout_test = Bailout::IMAG;
    set_bailout_formula(Bailout::IMAG);
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
}

TEST_P(TestTotalDistancePalette, usesStandardPaletteFolding)
{
    compare_pixel({8.0, 0.0625}, 128);

    EXPECT_EQ(GetParam().expected_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistancePalette,
    Values(TotalDistancePaletteInput{"DoubleFourColors", false, 4, 0},
        TotalDistancePaletteInput{"DoubleSixteenColors", false, 16, 4},
        TotalDistancePaletteInput{"DoubleTwoHundredFiftySixColors", false, 256, 64},
        TotalDistancePaletteInput{"HighPrecisionFourColors", true, 4, 0},
        TotalDistancePaletteInput{"HighPrecisionSixteenColors", true, 16, 4},
        TotalDistancePaletteInput{"HighPrecisionTwoHundredFiftySixColors", true, 256, 64}),
    [](const TestParamInfo<TotalDistancePaletteInput> &info) { return info.param.name; });

struct TotalDistanceInsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
};

void PrintTo(const TotalDistanceInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<TotalDistanceInsideInput> total_distance_inside_inputs()
{
    const std::array<PerturbationColorMethod, 7> methods{{{"Iterations", ColorMethod::ITER},
        {"Magnitude", ColorMethod::ZMAG}, {"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61},
        {"Fmod", ColorMethod::FMODI}, {"Period", ColorMethod::PERIOD}, {"FixedColor", ColorMethod::COLOR}}};
    std::vector<TotalDistanceInsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + inside.name, big_float, inside.method});
        }
    }
    return inputs;
}

class TestTotalDistanceInside : public PerturbationIterationTest, public WithParamInterface<TotalDistanceInsideInput>
{
public:
    TestTotalDistanceInside();

protected:
    void SetUp() override;
};

TestTotalDistanceInside::TestTotalDistanceInside() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTotalDistanceInside::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_outside_method = ColorMethod::TDIS;
    g_bof_match_book_images = false;
    g_inside_method = GetParam().inside;
}

TEST_P(TestTotalDistanceInside, preservesInsideColor)
{
    compare_pixel({}, 16);

    EXPECT_EQ(16, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTotalDistanceInside, ValuesIn(total_distance_inside_inputs()),
    [](const TestParamInfo<TotalDistanceInsideInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, totalDistanceRestartsWithAReplacementReference)
{
    g_inside_method = ColorMethod::ITER;
    g_outside_method = ColorMethod::TDIS;
    m_engine.set_glitch_tolerance(8.0);
    compare_pixel({0.5, 0.5}, 8, 0.125);
    EXPECT_EQ(3, g_color);
    EXPECT_GT(m_engine.reference_count(), 1);
}

struct InsideAtanSample
{
    const char *name;
    DComplex initial;
    int legacy_16;
    int legacy_256;
    int current_16;
    int current_256;
};

struct InsideAtanInput
{
    std::string name;
    bool big_float;
    Version version;
    int colors;
    InsideAtanSample sample;
};

void PrintTo(const InsideAtanInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InsideAtanInput> inside_atan_inputs()
{
    struct CompatibilityVersion
    {
        const char *name;
        Version version;
    };

    const InsideAtanSample samples[]{
        {"RealDominantUpper", {0.5, 0.25}, 8, 53, 4, 75},
        {"ImaginaryDominantUpper", {0.25, 0.5}, 6, 126, 11, 180},
        {"ImaginaryDominantLower", {0.25, -0.5}, 6, 126, 11, 180},
        {"RealDominantLower", {0.5, -0.25}, 8, 53, 4, 75},
        {"RealAxis", {0.5, 0.0}, 0, 0, 0, 0},
        {"ImaginaryAxis", {0.0, 0.5}, 15, 180, 1, 1},
    };

    std::vector<InsideAtanInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const CompatibilityVersion &version : {CompatibilityVersion{"Legacy2002", parse_legacy_version(2002)},
                 {"Legacy2003", parse_legacy_version(2003)}, {"Current", current_id_version()}})
        {
            for (const int colors : {16, 256})
            {
                for (const InsideAtanSample &sample : samples)
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + version.name + "_Colors" +
                            std::to_string(colors) + "_" + sample.name,
                        big_float, version.version, colors, sample});
                }
            }
        }
    }
    return inputs;
}

class TestInsideAtanScale : public PerturbationIterationTest, public WithParamInterface<InsideAtanInput>
{
public:
    TestInsideAtanScale();

protected:
    void SetUp() override;
};

TestInsideAtanScale::TestInsideAtanScale() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsideAtanScale::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ATANI;
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_version = GetParam().version;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
    init_atan_colors();
    g_init_orbit = GetParam().sample.initial;
}

TEST_P(TestInsideAtanScale, usesTerminalQuadrantAndVersionScale)
{
    const InsideAtanSample &sample{GetParam().sample};
    const int colors{GetParam().colors};

    compare_pixel({}, 2);

    EXPECT_EQ(2, g_real_color_iter);
    const int expected{g_version <= 2002 ? colors == 16 ? sample.legacy_16 : sample.legacy_256
            : colors == 16               ? sample.current_16
                                         : sample.current_256};
    EXPECT_EQ(expected, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideAtanScale, ValuesIn(inside_atan_inputs()),
    [](const TestParamInfo<InsideAtanInput> &info) { return info.param.name; });

struct AtanOutsideInput
{
    std::string name;
    bool big_float;
    ColorMethod outside;
};

void PrintTo(const AtanOutsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<AtanOutsideInput> atan_outside_inputs()
{
    const std::array<PerturbationColorMethod, 6> methods{
        {{"Iterations", ColorMethod::ITER}, {"Real", ColorMethod::REAL}, {"Imaginary", ColorMethod::IMAG},
            {"Atan", ColorMethod::ATAN}, {"Fmod", ColorMethod::FMOD}, {"TotalDistance", ColorMethod::TDIS}}};
    std::vector<AtanOutsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + outside.name, big_float, outside.method});
        }
    }
    return inputs;
}

class TestInsideAtanOutsideColor : public PerturbationIterationTest, public WithParamInterface<AtanOutsideInput>
{
public:
    TestInsideAtanOutsideColor();

protected:
    void SetUp() override;
};

TestInsideAtanOutsideColor::TestInsideAtanOutsideColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsideAtanOutsideColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ATANI;
    init_atan_colors();
    g_close_proximity = 16.0;
    g_outside_method = GetParam().outside;
}

TEST_P(TestInsideAtanOutsideColor, preservesOutsideColoring)
{
    compare_pixel({0.5, 0.5}, 8);

    EXPECT_EQ(4, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideAtanOutsideColor, ValuesIn(atan_outside_inputs()),
    [](const TestParamInfo<AtanOutsideInput> &info) { return info.param.name; });

struct AtanPeriodicityInput
{
    const char *name;
    bool big_float;
    int periodicity;
};

void PrintTo(const AtanPeriodicityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInsideAtanPeriodicity : public PerturbationIterationTest, public WithParamInterface<AtanPeriodicityInput>
{
public:
    TestInsideAtanPeriodicity();

protected:
    void SetUp() override;
};

TestInsideAtanPeriodicity::TestInsideAtanPeriodicity() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsideAtanPeriodicity::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ATANI;
    init_atan_colors();
    g_periodicity_check = GetParam().periodicity;
    g_old_color_iter = 0;
}

TEST_P(TestInsideAtanPeriodicity, matchesPeriodicityAndHighlighting)
{
    compare_pixel({-1.0, 0.0}, 64);

    EXPECT_EQ(64, g_real_color_iter);
    if (GetParam().periodicity < 0)
    {
        EXPECT_EQ(7, g_color);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideAtanPeriodicity,
    Values(AtanPeriodicityInput{"DoubleDisabled", false, 0}, AtanPeriodicityInput{"DoubleEnabled", false, 1},
        AtanPeriodicityInput{"DoubleHighlighted", false, -1}, AtanPeriodicityInput{"HighPrecisionDisabled", true, 0},
        AtanPeriodicityInput{"HighPrecisionEnabled", true, 1},
        AtanPeriodicityInput{"HighPrecisionHighlighted", true, -1}),
    [](const TestParamInfo<AtanPeriodicityInput> &info) { return info.param.name; });

struct AtanInitialOrbitInput
{
    const char *name;
    bool big_float;
    InitOrbitMode mode;
};

void PrintTo(const AtanInitialOrbitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInsideAtanReplacement : public PerturbationIterationTest, public WithParamInterface<AtanInitialOrbitInput>
{
public:
    TestInsideAtanReplacement();

protected:
    void SetUp() override;
};

TestInsideAtanReplacement::TestInsideAtanReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsideAtanReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ATANI;
    init_atan_colors();
    set_initial_offset({-0.125, 0.0625});
    m_engine.set_glitch_tolerance(8.0);
    g_use_init_orbit = GetParam().mode;
    g_init_orbit = {-0.25, 0.125};
}

TEST_P(TestInsideAtanReplacement, preservesOffsetsAcrossReferenceReplacement)
{
    compare_pixel({0.125, 0.0625}, 8, 0.125);

    EXPECT_EQ(8, g_real_color_iter);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsideAtanReplacement,
    Values(AtanInitialOrbitInput{"DoubleNormal", false, InitOrbitMode::NORMAL},
        AtanInitialOrbitInput{"DoublePixel", false, InitOrbitMode::PIXEL},
        AtanInitialOrbitInput{"DoubleValue", false, InitOrbitMode::VALUE},
        AtanInitialOrbitInput{"HighPrecisionNormal", true, InitOrbitMode::NORMAL},
        AtanInitialOrbitInput{"HighPrecisionPixel", true, InitOrbitMode::PIXEL},
        AtanInitialOrbitInput{"HighPrecisionValue", true, InitOrbitMode::VALUE}),
    [](const TestParamInfo<AtanInitialOrbitInput> &info) { return info.param.name; });

struct EpsilonAxisSample
{
    const char *name;
    std::complex<double> point;
    double proximity;
    long iterations;
    int color;
};

struct EpsilonAxisInput
{
    std::string name;
    bool big_float;
    bool negative;
    EpsilonAxisSample sample;
};

void PrintTo(const EpsilonAxisInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<EpsilonAxisInput> epsilon_axis_inputs()
{
    const EpsilonAxisSample samples[]{
        {"Origin", {0.0, 0.0}, 0.01, 1, 2},
        {"PositiveReal", {0.5, 0.0}, 0.01, 1, 6},
        {"UpperLeft", {-0.5, 0.5}, 0.01, 1, 6},
        {"LowerLeft", {-0.5, -0.5}, 0.01, 1, 6},
        {"UpperRightDelayed", {0.5, 0.5}, 0.3125, 2, 2},
        {"LowerRightDelayed", {0.5, -0.5}, 0.3125, 2, 2},
    };

    std::vector<EpsilonAxisInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const EpsilonAxisSample &sample : samples)
        {
            for (const bool negative : {false, true})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + sample.name +
                        (negative ? "_Negative" : "_Positive"),
                    big_float, negative, sample});
            }
        }
    }
    return inputs;
}

class TestEpsilonFirstAxis : public PerturbationIterationTest, public WithParamInterface<EpsilonAxisInput>
{
public:
    TestEpsilonFirstAxis();

protected:
    void SetUp() override;
};

TestEpsilonFirstAxis::TestEpsilonFirstAxis() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEpsilonFirstAxis::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::EPS_CROSS;
    g_close_proximity = GetParam().negative ? -GetParam().sample.proximity : GetParam().sample.proximity;
}

TEST_P(TestEpsilonFirstAxis, stopsAtFirstQualifiedAxis)
{
    const EpsilonAxisInput &input{GetParam()};
    const EpsilonAxisSample &sample{input.sample};

    compare_pixel(sample.point, 8, 0.0, sample.iterations);

    EXPECT_EQ(sample.iterations, g_real_color_iter);
    EXPECT_EQ(input.negative ? sample.iterations : sample.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonFirstAxis, ValuesIn(epsilon_axis_inputs()),
    [](const TestParamInfo<EpsilonAxisInput> &info) { return info.param.name; });

struct EpsilonResultInput
{
    const char *name;
    bool big_float;
    std::complex<double> point;
    double proximity;
    long max_iterations;
    long iterations;
    int color;
};

void PrintTo(const EpsilonResultInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestEpsilonResult : public PerturbationIterationTest, public WithParamInterface<EpsilonResultInput>
{
public:
    TestEpsilonResult();

protected:
    void SetUp() override;
};

TestEpsilonResult::TestEpsilonResult() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEpsilonResult::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::EPS_CROSS;
    g_close_proximity = GetParam().proximity;
}

class TestEpsilonThreshold : public TestEpsilonResult
{
};

TEST_P(TestEpsilonThreshold, excludesThresholdEquality)
{
    const EpsilonResultInput &input{GetParam()};

    compare_pixel(input.point, input.max_iterations);

    EXPECT_EQ(input.iterations, g_real_color_iter);
    EXPECT_EQ(input.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonThreshold,
    Values(EpsilonResultInput{"DoublePositive", false, {0.5, 0.5}, 0.25, 8, 4, 4},
        EpsilonResultInput{"DoubleNegative", false, {0.5, 0.5}, -0.25, 8, 4, 4},
        EpsilonResultInput{"HighPrecisionPositive", true, {0.5, 0.5}, 0.25, 8, 4, 4},
        EpsilonResultInput{"HighPrecisionNegative", true, {0.5, 0.5}, -0.25, 8, 4, 4}),
    [](const TestParamInfo<EpsilonResultInput> &info) { return info.param.name; });

class TestEpsilonSeedExclusion : public TestEpsilonResult
{
};

TEST_P(TestEpsilonSeedExclusion, doesNotTestInitialSeed)
{
    const EpsilonResultInput &input{GetParam()};

    compare_pixel(input.point, input.max_iterations);

    EXPECT_EQ(input.iterations, g_real_color_iter);
    EXPECT_EQ(input.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonSeedExclusion,
    Values(EpsilonResultInput{"Double", false, {0.0, 0.5}, 0.125, 3, 3, 3},
        EpsilonResultInput{"HighPrecision", true, {0.0, 0.5}, 0.125, 3, 3, 3}),
    [](const TestParamInfo<EpsilonResultInput> &info) { return info.param.name; });

class TestEpsilonNoHit : public TestEpsilonResult
{
};

TEST_P(TestEpsilonNoHit, reachesLimitWithoutAxisHit)
{
    const EpsilonResultInput &input{GetParam()};

    compare_pixel(input.point, input.max_iterations);

    EXPECT_EQ(input.iterations, g_real_color_iter);
    EXPECT_EQ(input.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonNoHit,
    Values(EpsilonResultInput{"DoubleDisabled", false, {}, 0.0, 8, 8, 8},
        EpsilonResultInput{"DoubleNoHit", false, {0.125, 0.0625}, 0.001, 8, 8, 8},
        EpsilonResultInput{"HighPrecisionDisabled", true, {}, 0.0, 8, 8, 8},
        EpsilonResultInput{"HighPrecisionNoHit", true, {0.125, 0.0625}, 0.001, 8, 8, 8}),
    [](const TestParamInfo<EpsilonResultInput> &info) { return info.param.name; });

class TestEpsilonEscapingUpdate : public TestEpsilonResult
{
protected:
    void SetUp() override;
};

void TestEpsilonEscapingUpdate::SetUp()
{
    TestEpsilonResult::SetUp();
    g_outside_method = ColorMethod::REAL;
}

TEST_P(TestEpsilonEscapingUpdate, doesNotTestEscapingUpdate)
{
    const EpsilonResultInput &input{GetParam()};

    compare_pixel(input.point, input.max_iterations);

    EXPECT_EQ(input.iterations, g_real_color_iter);
    EXPECT_EQ(input.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonEscapingUpdate,
    Values(EpsilonResultInput{"DoublePositive", false, {1.0, 0.0}, 0.01, 128, 1, 10},
        EpsilonResultInput{"DoubleNegative", false, {1.0, 0.0}, -0.01, 128, 1, 10},
        EpsilonResultInput{"HighPrecisionPositive", true, {1.0, 0.0}, 0.01, 128, 1, 10},
        EpsilonResultInput{"HighPrecisionNegative", true, {1.0, 0.0}, -0.01, 128, 1, 10}),
    [](const TestParamInfo<EpsilonResultInput> &info) { return info.param.name; });

struct EpsilonOutsideInput
{
    std::string name;
    bool big_float;
    ColorMethod outside;
    double proximity;
};

void PrintTo(const EpsilonOutsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<EpsilonOutsideInput> epsilon_outside_inputs()
{
    const std::array<PerturbationColorMethod, 3> methods{
        {{"Iterations", ColorMethod::ITER}, {"Real", ColorMethod::REAL}, {"TotalDistance", ColorMethod::TDIS}}};
    std::vector<EpsilonOutsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : methods)
        {
            for (const double proximity : {0.3125, -0.3125})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + outside.name +
                        (proximity > 0.0 ? "_Positive" : "_Negative"),
                    big_float, outside.method, proximity});
            }
        }
    }
    return inputs;
}

class TestEpsilonOutsideColor : public PerturbationIterationTest, public WithParamInterface<EpsilonOutsideInput>
{
public:
    TestEpsilonOutsideColor();

protected:
    void SetUp() override;
};

TestEpsilonOutsideColor::TestEpsilonOutsideColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEpsilonOutsideColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::EPS_CROSS;
    g_biomorph = 9;
    g_outside_method = GetParam().outside;
    g_close_proximity = GetParam().proximity;
}

TEST_P(TestEpsilonOutsideColor, bypassesOutsideAndBiomorphColors)
{
    compare_pixel({0.5, 0.5}, 8, 0.0, 2);

    EXPECT_EQ(2, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonOutsideColor, ValuesIn(epsilon_outside_inputs()),
    [](const TestParamInfo<EpsilonOutsideInput> &info) { return info.param.name; });

struct EpsilonPeriodicityInput
{
    const char *name;
    bool big_float;
    int periodicity;
};

void PrintTo(const EpsilonPeriodicityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestEpsilonPeriodicity : public PerturbationIterationTest, public WithParamInterface<EpsilonPeriodicityInput>
{
public:
    TestEpsilonPeriodicity();

protected:
    void SetUp() override;
};

TestEpsilonPeriodicity::TestEpsilonPeriodicity() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEpsilonPeriodicity::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::EPS_CROSS;
    g_close_proximity = 0.3125;
    g_periodicity_check = GetParam().periodicity;
    g_old_color_iter = 37;
}

TEST_P(TestEpsilonPeriodicity, preservesStateOnEarlyExit)
{
    compare_pixel({0.5, 0.5}, 64, 0.0, 2);

    EXPECT_EQ(37, g_old_color_iter);
    EXPECT_EQ(2, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonPeriodicity,
    Values(EpsilonPeriodicityInput{"DoubleEnabled", false, 1}, EpsilonPeriodicityInput{"DoubleHighlighted", false, -1},
        EpsilonPeriodicityInput{"HighPrecisionEnabled", true, 1},
        EpsilonPeriodicityInput{"HighPrecisionHighlighted", true, -1}),
    [](const TestParamInfo<EpsilonPeriodicityInput> &info) { return info.param.name; });

std::vector<EpsilonAxisInput> epsilon_mapping_inputs()
{
    const std::array<EpsilonAxisSample, 2> samples{
        {{"ImaginaryCoordinateZero", {0.5, 0.0}, 0.01, 1, 16}, {"Origin", {0.0, 0.0}, 0.01, 1, 12}}};
    std::vector<EpsilonAxisInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const EpsilonAxisSample &sample : samples)
        {
            for (const bool negative : {false, true})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + sample.name +
                        (negative ? "_Negative" : "_Positive"),
                    big_float, negative, sample});
            }
        }
    }
    return inputs;
}

class TestEpsilonMappedColor : public PerturbationIterationTest, public WithParamInterface<EpsilonAxisInput>
{
public:
    TestEpsilonMappedColor();

protected:
    void SetUp() override;
};

TestEpsilonMappedColor::TestEpsilonMappedColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEpsilonMappedColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_log_map_table = std::vector<Byte>(9, 44);
    g_log_map_table_max_size = 8L;
    g_log_map_flag = 1L;
    g_log_map_calculate = false;
    g_log_map_table[1] = 11;
    g_log_map_table[2] = 12;
    g_log_map_table[6] = 16;
    g_inside_method = ColorMethod::EPS_CROSS;
    g_outside_method = ColorMethod::REAL;
    g_potential = {{23.0, 10.0, 4.0}, false, true};
    g_biomorph = 9;
    g_close_proximity = GetParam().negative ? -GetParam().sample.proximity : GetParam().sample.proximity;
}

TEST_P(TestEpsilonMappedColor, mapsAxisColorBeforePotentialFinishing)
{
    const EpsilonAxisInput &input{GetParam()};

    compare_pixel(input.sample.point, 8, 0.0, 1);

    EXPECT_EQ(input.negative ? 11 : input.sample.color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEpsilonMappedColor, ValuesIn(epsilon_mapping_inputs()),
    [](const TestParamInfo<EpsilonAxisInput> &info) { return info.param.name; });

class TestEpsilonInitialization : public TestPerturbationIterations
{
protected:
    void SetUp() override;
};

void TestEpsilonInitialization::SetUp()
{
    TestPerturbationIterations::SetUp();
    g_inside_method = ColorMethod::EPS_CROSS;
    g_close_proximity = 0.01;
    g_init_orbit = {0.5, 0.5};
}

TEST_P(TestEpsilonInitialization, usesFixedInitialOrbit)
{
    g_use_init_orbit = InitOrbitMode::VALUE;

    compare_pixel({}, 8, 0.0, 1);

    EXPECT_EQ(2, g_color);
}

TEST_P(TestEpsilonInitialization, preservesOffsetAcrossReferenceReplacement)
{
    g_use_init_orbit = InitOrbitMode::NORMAL;
    set_initial_offset({-0.125, 0.0});
    m_engine.set_glitch_tolerance(8.0);

    compare_pixel({0.5, 0.0}, 8, 0.125, 1);

    EXPECT_EQ(6, g_color);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestEpsilonInitialization, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "HighPrecision" : "Double"; });

struct StarTrailHistoryInput
{
    std::string name;
    bool big_float;
    long limit;
    int periodicity;
    std::complex<double> point;
    int color;
};

void PrintTo(const StarTrailHistoryInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<StarTrailHistoryInput> star_trail_history_inputs()
{
    struct Sample
    {
        const char *name;
        std::complex<double> point;
        int color;
    };

    struct Periodicity
    {
        const char *name;
        int value;
    };

    std::vector<StarTrailHistoryInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const long limit : {1L, 8L, 128L})
        {
            for (const Periodicity &periodicity : {Periodicity{"Disabled", 0}, {"Enabled", 1}, {"Highlighted", -1}})
            {
                for (const Sample &sample : {Sample{"Origin", {0.0, 0.0}, 1}, {"NegativeReal", {-1.0, 0.0}, 1},
                         {"PositiveImaginary", {0.0, 1.0}, 2}, {"SmallComplex", {0.125, 0.125}, 0},
                         {"LeftOffset", {-0.75, 0.125}, 2}})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Limit" +
                            std::to_string(limit) + "_" + periodicity.name + "_" + sample.name,
                        big_float, limit, periodicity.value, sample.point, sample.color});
                }
            }
        }
    }
    return inputs;
}

class TestStarTrailHistory : public PerturbationIterationTest, public WithParamInterface<StarTrailHistoryInput>
{
public:
    TestStarTrailHistory();

protected:
    void SetUp() override;
};

TestStarTrailHistory::TestStarTrailHistory() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestStarTrailHistory::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_periodicity_check = GetParam().periodicity;
}

TEST_P(TestStarTrailHistory, matchesKnownHistoryWithoutPeriodicity)
{
    compare_pixel(GetParam().point, GetParam().limit);

    EXPECT_EQ(GetParam().color, g_color);
    EXPECT_EQ(16, g_real_color_iter);
    EXPECT_EQ(15, s_standard_orbit.size());
    EXPECT_EQ(GetParam().limit, g_max_iterations);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestStarTrailHistory, ValuesIn(star_trail_history_inputs()),
    [](const TestParamInfo<StarTrailHistoryInput> &info) { return info.param.name; });

struct StarTrailEscapeInput
{
    const char *name;
    bool big_float;
    std::complex<double> point;
    long limit;
    std::optional<DComplex> initial;
};

void PrintTo(const StarTrailEscapeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStarTrailEscape : public PerturbationIterationTest, public WithParamInterface<StarTrailEscapeInput>
{
public:
    TestStarTrailEscape();

protected:
    void SetUp() override;
};

TestStarTrailEscape::TestStarTrailEscape() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestStarTrailEscape::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::STAR_TRAIL;
    if (GetParam().initial)
    {
        g_use_init_orbit = InitOrbitMode::VALUE;
        g_init_orbit = *GetParam().initial;
    }
}

TEST_P(TestStarTrailEscape, continuesPastBailoutAndClampsCoordinates)
{
    compare_pixel(GetParam().point, GetParam().limit);

    EXPECT_EQ(1, g_color);
    EXPECT_EQ(16, g_real_color_iter);
    EXPECT_EQ(15, s_standard_orbit.size());
    EXPECT_DOUBLE_EQ(std::numeric_limits<float>::max(), g_new_z.x);
    EXPECT_EQ(GetParam().limit, g_max_iterations);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestStarTrailEscape,
    Values(StarTrailEscapeInput{"DoublePixelSeed", false, {1.0, 0.0}, 128, std::nullopt},
        StarTrailEscapeInput{"DoubleFixedSeed", false, {}, 8, DComplex{0.0, 2.0}},
        StarTrailEscapeInput{"HighPrecisionPixelSeed", true, {1.0, 0.0}, 128, std::nullopt},
        StarTrailEscapeInput{"HighPrecisionFixedSeed", true, {}, 8, DComplex{0.0, 2.0}}),
    [](const TestParamInfo<StarTrailEscapeInput> &info) { return info.param.name; });

struct StarTrailPaletteInput
{
    const char *name;
    bool big_float;
    int colors;
};

void PrintTo(const StarTrailPaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStarTrailPalette : public PerturbationIterationTest, public WithParamInterface<StarTrailPaletteInput>
{
public:
    TestStarTrailPalette();

protected:
    void SetUp() override;
};

TestStarTrailPalette::TestStarTrailPalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestStarTrailPalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
}

TEST_P(TestStarTrailPalette, usesPaletteSizedHistorySlots)
{
    compare_pixel({-0.5, 0.5}, 128);

    EXPECT_EQ(GetParam().colors < 16 ? 0 : 5, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestStarTrailPalette,
    Values(StarTrailPaletteInput{"DoubleTwoColors", false, 2}, StarTrailPaletteInput{"DoubleFourColors", false, 4},
        StarTrailPaletteInput{"DoubleSixteenColors", false, 16},
        StarTrailPaletteInput{"DoubleTwoHundredFiftySixColors", false, 256},
        StarTrailPaletteInput{"HighPrecisionTwoColors", true, 2},
        StarTrailPaletteInput{"HighPrecisionFourColors", true, 4},
        StarTrailPaletteInput{"HighPrecisionSixteenColors", true, 16},
        StarTrailPaletteInput{"HighPrecisionTwoHundredFiftySixColors", true, 256}),
    [](const TestParamInfo<StarTrailPaletteInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, starTrailRestartsHistoryWithAReplacementReference)
{
    g_inside_method = ColorMethod::STAR_TRAIL;
    m_engine.set_glitch_tolerance(8.0);
    compare_pixel({0.125, 0.125}, 128, 0.125);
    EXPECT_EQ(0, g_color);
    EXPECT_EQ(128, g_max_iterations);
    EXPECT_GT(m_engine.reference_count(), 1);
}

struct StarTrailFinishingInput
{
    const char *name;
    bool big_float;
    bool potential;
};

void PrintTo(const StarTrailFinishingInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStarTrailFinishing : public PerturbationIterationTest, public WithParamInterface<StarTrailFinishingInput>
{
public:
    TestStarTrailFinishing();

protected:
    void SetUp() override;
};

TestStarTrailFinishing::TestStarTrailFinishing() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestStarTrailFinishing::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_log_map_table.clear();
    g_log_map_calculate = false;
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_biomorph = 9;
    g_potential = GetParam().potential ? Potential{{23.0, 10.0, 4.0}, false, true} : Potential{};
}

TEST_P(TestStarTrailFinishing, usesInsideFinishingRatherThanBiomorphColor)
{
    compare_pixel({}, 128);

    EXPECT_EQ(GetParam().potential ? 23 : 1, g_color);
    if (GetParam().potential)
    {
        EXPECT_EQ(128, g_max_iterations);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestStarTrailFinishing,
    Values(StarTrailFinishingInput{"DoubleWithoutPotential", false, false},
        StarTrailFinishingInput{"DoubleWithPotential", false, true},
        StarTrailFinishingInput{"HighPrecisionWithoutPotential", true, false},
        StarTrailFinishingInput{"HighPrecisionWithPotential", true, true}),
    [](const TestParamInfo<StarTrailFinishingInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, starTrailBoundsReferencesAndRestoresTheLimitBeforeReturningControl)
{
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_max_iterations = std::numeric_limits<long>::max();
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt = record_perturbation_orbit;
    s_reference_coordinates.clear();
    s_perturbation_orbit.clear();
    initialize_reference({0.0, 0.0});
    m_engine.initialize_pixel_strategy();
    EXPECT_EQ(16, s_reference_coordinates.size());
    EXPECT_EQ(std::numeric_limits<long>::max(), g_max_iterations);

    EXPECT_EQ(1, m_engine.calculate_pixel(g_col, g_row));
    EXPECT_EQ(15, s_perturbation_orbit.size());
    EXPECT_EQ(16, s_reference_coordinates.size());
    EXPECT_EQ(std::numeric_limits<long>::max(), g_max_iterations);
    m_engine.finish();
    EXPECT_EQ(std::numeric_limits<long>::max(), g_max_iterations);
}

TEST_P(TestPerturbationIterations, starTrailRestoresTheLimitAfterReferenceFailure)
{
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_max_iterations = 201;
    m_specific.pert_ref = [](const std::complex<double> &, std::complex<double> &)
    {
        EXPECT_EQ(16, g_max_iterations);
        throw std::runtime_error{"reference failure"};
    };
    m_specific.pert_ref_bf = [](const BFComplex &, BFComplex &)
    {
        EXPECT_EQ(16, g_max_iterations);
        throw std::runtime_error{"reference failure"};
    };
    initialize_reference({0.0, 0.0});

    EXPECT_THROW(m_engine.initialize_pixel_strategy(), std::runtime_error);
    EXPECT_EQ(201, g_max_iterations);
}

class TestStarTrailFailure : public TestPerturbationIterations
{
protected:
    void SetUp() override;
    void TearDown() override;
};

void TestStarTrailFailure::SetUp()
{
    TestPerturbationIterations::SetUp();
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_max_iterations = 201;
    initialize_reference({0.0, 0.0});
}

void TestStarTrailFailure::TearDown()
{
    m_specific.pert_pt = mandel_perturb;
    TestPerturbationIterations::TearDown();
}

TEST_P(TestStarTrailFailure, restoresLimitAfterPixelFailure)
{
    m_specific.pert_pt = [](const std::complex<double> &, std::complex<double> &, const std::complex<double> &)
    {
        EXPECT_EQ(16, g_max_iterations);
        throw std::runtime_error{"pixel failure"};
    };

    EXPECT_THROW(m_engine.calculate_pixel(g_col, g_row), std::runtime_error);
    EXPECT_EQ(201, g_max_iterations);
}

TEST_P(TestStarTrailFailure, restoresLimitAfterOutputFailure)
{
    m_specific.pert_pt = mandel_perturb;
    EXPECT_CALL(m_driver, write_pixel(g_col, g_row, 1)).WillOnce(Throw(std::runtime_error{"output failure"}));

    EXPECT_THROW(m_engine.calculate_pixel(g_col, g_row), std::runtime_error);
    EXPECT_EQ(201, g_max_iterations);
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestStarTrailFailure, Values(false, true),
    [](const TestParamInfo<bool> &info) { return info.param ? "HighPrecision" : "Double"; });

struct LogMappedSampleInput
{
    const char *name;
    bool big_float;
    std::complex<double> point;
    long iterations;
    int color;
};

void PrintTo(const LogMappedSampleInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestFullIterationMapping : public PerturbationIterationTest, public WithParamInterface<LogMappedSampleInput>
{
public:
    TestFullIterationMapping();

protected:
    void SetUp() override;
};

TestFullIterationMapping::TestFullIterationMapping() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestFullIterationMapping::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_log_map_flag = 1;
    g_log_map_table_max_size = 512;
    g_log_map_table.assign(513, 44);
    g_log_map_table[4] = 80;
    g_log_map_table[80] = 99;
    g_log_map_table[512] = 13;
    g_colors = 16;
    g_and_color = 15;
}

TEST_P(TestFullIterationMapping, mapsFullCountBeforePaletteFolding)
{
    compare_pixel(GetParam().point, 512);

    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
    EXPECT_EQ(GetParam().color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFullIterationMapping,
    Values(LogMappedSampleInput{"DoubleOutside", false, {0.5, 0.0}, 4, 5},
        LogMappedSampleInput{"DoubleInside", false, {}, 512, 13},
        LogMappedSampleInput{"HighPrecisionOutside", true, {0.5, 0.0}, 4, 5},
        LogMappedSampleInput{"HighPrecisionInside", true, {}, 512, 13}),
    [](const TestParamInfo<LogMappedSampleInput> &info) { return info.param.name; });

struct MappedColorMethodInput
{
    std::string name;
    bool big_float;
    ColorMethod method;
};

void PrintTo(const MappedColorMethodInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<MappedColorMethodInput> mapped_color_method_inputs(const std::vector<PerturbationColorMethod> &methods)
{
    std::vector<MappedColorMethodInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &method : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + method.name, big_float, method.method});
        }
    }
    return inputs;
}

class MappedColorMethodTest : public PerturbationIterationTest, public WithParamInterface<MappedColorMethodInput>
{
public:
    MappedColorMethodTest();

protected:
    void SetUp() override;
};

MappedColorMethodTest::MappedColorMethodTest() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void MappedColorMethodTest::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_log_map_flag = 1;
    g_log_map_table_max_size = 128;
}

class TestMappedOutsideColor : public MappedColorMethodTest
{
protected:
    void SetUp() override;
};

void TestMappedOutsideColor::SetUp()
{
    MappedColorMethodTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_log_map_table.resize(129);
    for (std::size_t i = 0; i < g_log_map_table.size(); ++i)
    {
        g_log_map_table[i] = static_cast<Byte>((7 * i + 19) % 256);
    }
    g_close_proximity = 16.0;
    init_atan_colors();
    g_outside_method = GetParam().method;
}

TEST_P(TestMappedOutsideColor, mapsAfterOutsideFormula)
{
    compare_pixel({0.5, 0.5}, 128);

    EXPECT_EQ(4, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMappedOutsideColor,
    ValuesIn(mapped_color_method_inputs({{"Iterations", ColorMethod::ITER}, {"Real", ColorMethod::REAL},
        {"Imaginary", ColorMethod::IMAG}, {"Product", ColorMethod::MULT}, {"Sum", ColorMethod::SUM},
        {"Atan", ColorMethod::ATAN}, {"Fmod", ColorMethod::FMOD}, {"TotalDistance", ColorMethod::TDIS}})),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class TestMappedInsideColor : public MappedColorMethodTest
{
protected:
    void SetUp() override;
};

void TestMappedInsideColor::SetUp()
{
    MappedColorMethodTest::SetUp();
    g_log_map_table.assign(129, 44);
    g_log_map_table[0] = 17;
    g_log_map_table[1] = 18;
    g_log_map_table[2] = 19;
    g_log_map_table[128] = 20;
    g_bof_match_book_images = false;
    init_atan_colors();
    g_inside_method = GetParam().method;
}

TEST_P(TestMappedInsideColor, mapsAfterInsideFormula)
{
    compare_pixel({}, 128);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMappedInsideColor,
    ValuesIn(mapped_color_method_inputs({{"Iterations", ColorMethod::ITER}, {"Magnitude", ColorMethod::ZMAG},
        {"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61}, {"Fmod", ColorMethod::FMODI},
        {"Atan", ColorMethod::ATANI}, {"Period", ColorMethod::PERIOD}, {"StarTrail", ColorMethod::STAR_TRAIL}})),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class MappingBypassTest : public MappedColorMethodTest
{
protected:
    void SetUp() override;
};

void MappingBypassTest::SetUp()
{
    MappedColorMethodTest::SetUp();
    g_log_map_table.assign(129, 99);
    g_inside_color = 7;
}

class TestFixedInsideMappingBypass : public MappingBypassTest
{
protected:
    void SetUp() override;
};

void TestFixedInsideMappingBypass::SetUp()
{
    MappingBypassTest::SetUp();
    g_inside_method = ColorMethod::COLOR;
    g_outside_method = GetParam().method;
    if (g_outside_method == ColorMethod::COLOR)
    {
        g_outside_color = 4;
    }
}

TEST_P(TestFixedInsideMappingBypass, bypassesMappingForFixedInsideColor)
{
    compare_pixel({}, 128);

    EXPECT_EQ(7, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFixedInsideMappingBypass,
    Values(MappedColorMethodInput{"DoubleOutsideIterations", false, ColorMethod::ITER},
        MappedColorMethodInput{"DoubleOutsideFixed", false, ColorMethod::COLOR},
        MappedColorMethodInput{"HighPrecisionOutsideIterations", true, ColorMethod::ITER},
        MappedColorMethodInput{"HighPrecisionOutsideFixed", true, ColorMethod::COLOR}),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class TestUnfixedOutsideMapping : public MappingBypassTest
{
protected:
    void SetUp() override;
};

void TestUnfixedOutsideMapping::SetUp()
{
    MappingBypassTest::SetUp();
    g_inside_method = ColorMethod::COLOR;
    g_outside_method = GetParam().method;
}

TEST_P(TestUnfixedOutsideMapping, stillMapsUnfixedOutsideColor)
{
    compare_pixel({0.5, 0.0}, 128);

    EXPECT_EQ(99, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestUnfixedOutsideMapping,
    Values(MappedColorMethodInput{"Double", false, ColorMethod::ITER},
        MappedColorMethodInput{"HighPrecision", true, ColorMethod::ITER}),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class TestFixedOutsideMappingBypass : public MappingBypassTest
{
protected:
    void SetUp() override;
};

void TestFixedOutsideMappingBypass::SetUp()
{
    MappingBypassTest::SetUp();
    g_inside_method = GetParam().method;
    g_outside_method = ColorMethod::COLOR;
    g_outside_color = 4;
}

TEST_P(TestFixedOutsideMappingBypass, bypassesMappingForFixedOutsideColor)
{
    compare_pixel({0.5, 0.0}, 128);

    EXPECT_EQ(4, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFixedOutsideMappingBypass,
    Values(MappedColorMethodInput{"DoubleInsideFixed", false, ColorMethod::COLOR},
        MappedColorMethodInput{"DoubleInsideIterations", false, ColorMethod::ITER},
        MappedColorMethodInput{"HighPrecisionInsideFixed", true, ColorMethod::COLOR},
        MappedColorMethodInput{"HighPrecisionInsideIterations", true, ColorMethod::ITER}),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class TestCycleMappingBypass : public MappingBypassTest
{
protected:
    void SetUp() override;
};

void TestCycleMappingBypass::SetUp()
{
    MappingBypassTest::SetUp();
    g_inside_method = GetParam().method;
    g_outside_method = ColorMethod::ITER;
    g_outside_color = 4;
    g_periodicity_check = -1;
    g_old_color_iter = 0;
}

TEST_P(TestCycleMappingBypass, bypassesMappingForCycleHighlight)
{
    compare_pixel({}, 128);

    EXPECT_EQ(7, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCycleMappingBypass,
    Values(MappedColorMethodInput{"Double", false, ColorMethod::ITER},
        MappedColorMethodInput{"HighPrecision", true, ColorMethod::ITER}),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

class TestPotentialMapping : public MappingBypassTest
{
protected:
    void SetUp() override;
};

void TestPotentialMapping::SetUp()
{
    MappingBypassTest::SetUp();
    g_inside_method = GetParam().method;
    g_outside_method = ColorMethod::ITER;
    g_outside_color = 4;
    g_periodicity_check = 0;
    g_potential = {{23.0, 10.0, 4.0}, false, true};
}

TEST_P(TestPotentialMapping, mapsPotentialColor)
{
    compare_pixel({}, 128);

    EXPECT_EQ(99, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialMapping,
    Values(MappedColorMethodInput{"Double", false, ColorMethod::COLOR},
        MappedColorMethodInput{"HighPrecision", true, ColorMethod::COLOR}),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

struct LogMapPathInput
{
    std::string name;
    bool big_float;
    Version version;
    long flag;
    bool calculate;
    std::complex<double> point;
    int iterations;
};

void PrintTo(const LogMapPathInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<LogMapPathInput> log_map_path_inputs()
{
    struct CompatibilityVersion
    {
        const char *name;
        Version version;
    };

    struct Flag
    {
        const char *name;
        long value;
    };

    struct Sample
    {
        const char *name;
        std::complex<double> point;
        int iterations;
    };

    std::vector<LogMapPathInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const CompatibilityVersion &version : {CompatibilityVersion{"Legacy1920", parse_legacy_version(1920)},
                 {"Legacy1921", parse_legacy_version(1921)}, {"Legacy2001", parse_legacy_version(2001)},
                 {"Legacy2002", parse_legacy_version(2002)}, {"Current", current_id_version()}})
        {
            for (const Flag &flag :
                {Flag{"PositiveOne", 1L}, {"NegativeOne", -1L}, {"PositiveEight", 8L}, {"NegativeEight", -8L}})
            {
                for (const bool calculate : {false, true})
                {
                    for (const Sample &sample : {Sample{"Outside", {0.5, 0.0}, 4}, {"Inside", {}, 128}})
                    {
                        inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + version.name + "_" +
                                flag.name + (calculate ? "_OnTheFly_" : "_Table_") + sample.name,
                            big_float, version.version, flag.value, calculate, sample.point, sample.iterations});
                    }
                }
            }
        }
    }
    return inputs;
}

class TestLogMapPaths : public PerturbationIterationTest, public WithParamInterface<LogMapPathInput>
{
public:
    TestLogMapPaths();

protected:
    void SetUp() override;
};

TestLogMapPaths::TestLogMapPaths() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestLogMapPaths::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_colors = 16;
    g_and_color = 15;
    g_log_map_table_max_size = 128;
    g_version = GetParam().version;
    g_log_map_flag = GetParam().flag;
    g_log_map_calculate = GetParam().calculate;
    g_log_map_fly_calculate = GetParam().calculate ? LogMapCalculate::ON_THE_FLY : LogMapCalculate::USE_LOG_TABLE;
    g_log_map_table.assign(GetParam().calculate ? 0 : 129, 0);
    setup_log_table();
}

TEST_P(TestLogMapPaths, usesSharedTableOrOnTheFlyMapping)
{
    compare_pixel(GetParam().point, 128);

    EXPECT_EQ(standard_palette_color(log_table_calc(GetParam().iterations)), g_color);
    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLogMapPaths, ValuesIn(log_map_path_inputs()),
    [](const TestParamInfo<LogMapPathInput> &info) { return info.param.name; });

struct IterationRangeInput
{
    const char *name;
    bool big_float;
    bool striped;
    std::complex<double> point;
    int iterations;
    int color;
};

void PrintTo(const IterationRangeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestIterationRangeMapping : public PerturbationIterationTest, public WithParamInterface<IterationRangeInput>
{
public:
    TestIterationRangeMapping();

protected:
    void SetUp() override;
    void TearDown() override;

private:
    std::unique_ptr<ImageCalculation> m_calculation;
};

TestIterationRangeMapping::TestIterationRangeMapping() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestIterationRangeMapping::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_max_iterations = 128;
    g_iteration_ranges = GetParam().striped ? std::vector<int>{-1, 2, 8, 128} : std::vector<int>{2, 5, 128};
    g_log_map_flag = 1;
    m_calculation = std::make_unique<ImageCalculation>();
    EXPECT_TRUE(m_calculation->start().empty());
    EXPECT_EQ(0, g_log_map_flag);
}

void TestIterationRangeMapping::TearDown()
{
    m_calculation.reset();
    PerturbationIterationTest::TearDown();
}

TEST_P(TestIterationRangeMapping, mapsIterationRange)
{
    compare_pixel(GetParam().point, 128);

    EXPECT_EQ(GetParam().color, g_color);
    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestIterationRangeMapping,
    Values(IterationRangeInput{"DoubleOrdinaryOutside", false, false, {0.5, 0.0}, 4, 1},
        IterationRangeInput{"DoubleOrdinaryInside", false, false, {}, 128, 2},
        IterationRangeInput{"DoubleStripedOutside", false, true, {0.5, 0.0}, 4, 0},
        IterationRangeInput{"DoubleStripedInside", false, true, {}, 128, 2},
        IterationRangeInput{"HighPrecisionOrdinaryOutside", true, false, {0.5, 0.0}, 4, 1},
        IterationRangeInput{"HighPrecisionOrdinaryInside", true, false, {}, 128, 2},
        IterationRangeInput{"HighPrecisionStripedOutside", true, true, {0.5, 0.0}, 4, 0},
        IterationRangeInput{"HighPrecisionStripedInside", true, true, {}, 128, 2}),
    [](const TestParamInfo<IterationRangeInput> &info) { return info.param.name; });

class TestMappedReferenceReplacement : public PerturbationIterationTest, public WithParamInterface<LogMappedSampleInput>
{
public:
    TestMappedReferenceReplacement();

protected:
    void SetUp() override;
};

TestMappedReferenceReplacement::TestMappedReferenceReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestMappedReferenceReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_log_map_flag = 1;
    g_log_map_table_max_size = 128;
    g_log_map_table.assign(129, 44);
    g_log_map_table[4] = 77;
    g_log_map_table[128] = 88;
    m_engine.set_glitch_tolerance(8.0);
}

TEST_P(TestMappedReferenceReplacement, preservesMappedColor)
{
    compare_pixel(GetParam().point, 128, 0.125);

    EXPECT_EQ(GetParam().color, g_color);
    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMappedReferenceReplacement,
    Values(LogMappedSampleInput{"DoubleOutside", false, {0.5, 0.5}, 4, 77},
        LogMappedSampleInput{"DoubleInside", false, {0.125, 0.125}, 128, 88},
        LogMappedSampleInput{"HighPrecisionOutside", true, {0.5, 0.5}, 4, 77},
        LogMappedSampleInput{"HighPrecisionInside", true, {0.125, 0.125}, 128, 88}),
    [](const TestParamInfo<LogMappedSampleInput> &info) { return info.param.name; });

struct DecompositionInput
{
    std::string name;
    bool big_float;
    int sectors;
    int colors;
    std::complex<double> point;
};

void PrintTo(const DecompositionInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DecompositionInput> decomposition_inputs()
{
    struct Point
    {
        const char *name;
        std::complex<double> value;
    };

    const Point points[]{
        {"PositiveReal", {3.0, 0.0}},
        {"UpperRightReal", {3.0, 1.0}},
        {"UpperRightDiagonal", {3.0, 3.0}},
        {"UpperRightImaginary", {1.0, 3.0}},
        {"PositiveImaginary", {0.0, 3.0}},
        {"UpperLeftImaginary", {-1.0, 3.0}},
        {"UpperLeftReal", {-3.0, 1.0}},
        {"NegativeReal", {-3.0, 0.0}},
        {"LowerLeftReal", {-3.0, -1.0}},
        {"LowerLeftImaginary", {-1.0, -3.0}},
        {"NegativeImaginary", {0.0, -3.0}},
        {"LowerRightImaginary", {1.0, -3.0}},
        {"LowerRightReal", {3.0, -1.0}},
    };

    std::vector<DecompositionInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int sectors : {2, 4, 8, 16, 32, 64, 128, 256})
        {
            for (const int colors : {2, 4, 16, 256})
            {
                for (const Point &point : points)
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Sectors" +
                            std::to_string(sectors) + "_Colors" + std::to_string(colors) + "_" + point.name,
                        big_float, sectors, colors, point.value});
                }
            }
        }
    }
    return inputs;
}

class TestDecompositionPalette : public PerturbationIterationTest, public WithParamInterface<DecompositionInput>
{
public:
    TestDecompositionPalette();

protected:
    void SetUp() override;
};

TestDecompositionPalette::TestDecompositionPalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDecompositionPalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_decomp[0] = GetParam().sectors;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
    const std::complex<double> point{GetParam().point};
    set_initial_offset({-point.real(), -point.imag()});
}

TEST_P(TestDecompositionPalette, matchesStandardSectorsAndPalette)
{
    compare_pixel(GetParam().point, 8);

    EXPECT_EQ(1, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDecompositionPalette, ValuesIn(decomposition_inputs()),
    [](const TestParamInfo<DecompositionInput> &info) { return info.param.name; });

struct DecompositionQuadrantInput
{
    const char *name;
    bool big_float;
    std::complex<double> point;
    int color;
};

void PrintTo(const DecompositionQuadrantInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDecompositionQuadrant : public PerturbationIterationTest,
                                  public WithParamInterface<DecompositionQuadrantInput>
{
public:
    TestDecompositionQuadrant();

protected:
    void SetUp() override;
};

TestDecompositionQuadrant::TestDecompositionQuadrant() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDecompositionQuadrant::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_decomp[0] = 4;
    const std::complex<double> point{GetParam().point};
    set_initial_offset({-point.real(), -point.imag()});
}

TEST_P(TestDecompositionQuadrant, usesStandardQuadrantColor)
{
    compare_pixel(GetParam().point, 8);

    EXPECT_EQ(GetParam().color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDecompositionQuadrant,
    Values(DecompositionQuadrantInput{"DoubleUpperRight", false, {3.0, 1.0}, 1},
        DecompositionQuadrantInput{"DoubleUpperLeft", false, {-3.0, 1.0}, 2},
        DecompositionQuadrantInput{"DoubleLowerLeft", false, {-3.0, -1.0}, 3},
        DecompositionQuadrantInput{"DoubleLowerRight", false, {3.0, -1.0}, 4},
        DecompositionQuadrantInput{"HighPrecisionUpperRight", true, {3.0, 1.0}, 1},
        DecompositionQuadrantInput{"HighPrecisionUpperLeft", true, {-3.0, 1.0}, 2},
        DecompositionQuadrantInput{"HighPrecisionLowerLeft", true, {-3.0, -1.0}, 3},
        DecompositionQuadrantInput{"HighPrecisionLowerRight", true, {3.0, -1.0}, 4}),
    [](const TestParamInfo<DecompositionQuadrantInput> &info) { return info.param.name; });

class TestDecompositionOverride : public MappedColorMethodTest
{
protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_limit2{g_magnitude_limit2};
};

void TestDecompositionOverride::SetUp()
{
    MappedColorMethodTest::SetUp();
    g_magnitude_limit2 = 2.0;
    g_inside_method = ColorMethod::ITER;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_log_map_table.assign(129, 44);
    g_log_map_table[1] = 17;
    g_log_map_table[4] = 23;
    g_log_map_table[9] = 31;
    g_close_proximity = 16.0;
    init_atan_colors();
    g_outside_method = GetParam().method;
    if (g_outside_method == ColorMethod::COLOR)
    {
        g_outside_color = 6;
    }
    set_initial_offset({-3.0, 0.0});
}

TEST_P(TestDecompositionOverride, preservesFinishingPriority)
{
    compare_pixel({3.0, 0.0}, 128);

    EXPECT_EQ(GetParam().method == ColorMethod::COLOR ? 6 : 17, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDecompositionOverride,
    ValuesIn(mapped_color_method_inputs(
        {{"Iterations", ColorMethod::ITER}, {"Real", ColorMethod::REAL}, {"Imaginary", ColorMethod::IMAG},
            {"Product", ColorMethod::MULT}, {"Sum", ColorMethod::SUM}, {"Atan", ColorMethod::ATAN},
            {"Fmod", ColorMethod::FMOD}, {"TotalDistance", ColorMethod::TDIS}, {"FixedColor", ColorMethod::COLOR}})),
    [](const TestParamInfo<MappedColorMethodInput> &info) { return info.param.name; });

struct DecompositionInsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
};

void PrintTo(const DecompositionInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DecompositionInsideInput> decomposition_inside_inputs()
{
    const std::array<PerturbationColorMethod, 4> methods{{{"Iterations", ColorMethod::ITER},
        {"FixedColor", ColorMethod::COLOR}, {"Magnitude", ColorMethod::ZMAG}, {"Atan", ColorMethod::ATANI}}};
    std::vector<DecompositionInsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + inside.name, big_float, inside.method});
        }
    }
    return inputs;
}

class TestDecompositionInside : public PerturbationIterationTest, public WithParamInterface<DecompositionInsideInput>
{
public:
    TestDecompositionInside();

protected:
    void SetUp() override;
};

TestDecompositionInside::TestDecompositionInside() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDecompositionInside::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_decomp[0] = 256;
    init_atan_colors();
    g_inside_method = GetParam().inside;
}

TEST_P(TestDecompositionInside, preservesInsideColor)
{
    compare_pixel({0.125, 0.0625}, 8);

    EXPECT_EQ(8, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDecompositionInside, ValuesIn(decomposition_inside_inputs()),
    [](const TestParamInfo<DecompositionInsideInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, decompositionSurvivesReferenceReplacement)
{
    m_engine.set_glitch_tolerance(8.0);
    g_inside_method = ColorMethod::ITER;
    g_decomp[0] = 256;
    compare_pixel({0.5, 0.5}, 128, 0.125);
    EXPECT_GT(s_perturbation_orbit.size(), s_standard_orbit.size());
    EXPECT_GT(m_engine.reference_count(), 1);
}

class PerturbationBiomorphTest : public PerturbationIterationTest
{
public:
    explicit PerturbationBiomorphTest(bool use_big_float);

protected:
    void SetUp() override;

private:
    ValueSaver<double> m_saved_limit2{g_magnitude_limit2};
};

PerturbationBiomorphTest::PerturbationBiomorphTest(const bool use_big_float) :
    PerturbationIterationTest{use_big_float}
{
}

void PerturbationBiomorphTest::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_magnitude_limit2 = 2.0;
}

struct BiomorphBoundaryInput
{
    std::string name;
    bool big_float;
    double limit;
    int color;
    std::complex<double> point;
    bool hit;
};

void PrintTo(const BiomorphBoundaryInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BiomorphBoundaryInput> biomorph_boundary_inputs()
{
    struct Limit
    {
        const char *name;
        double value;
    };
    struct Sample
    {
        const char *name;
        std::complex<double> point;
        bool hit;
    };

    std::vector<BiomorphBoundaryInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const Limit &limit : {Limit{"Four", 4.0}, {"Hundred", 100.0}})
        {
            const double threshold{std::sqrt(limit.value)};
            const double below{threshold * (1.0 - 1.0 / 1024)};
            const Sample samples[]{
                {"PositiveRealAxis", {2 * threshold, 0.0}, true},
                {"PositiveImaginaryAxis", {0.0, 2 * threshold}, true},
                {"ImaginaryAtThreshold", {2 * threshold, threshold}, false},
                {"RealAtThreshold", {threshold, 2 * threshold}, false},
                {"ImaginaryBelowThreshold", {2 * threshold, below}, true},
                {"RealBelowThreshold", {below, 2 * threshold}, true},
                {"NegativeRealBelowThreshold", {-below, -2 * threshold}, true},
                {"NegativeRealAtThreshold", {-threshold, -2 * threshold}, false},
            };
            for (const int color : {0, 9})
            {
                for (const Sample &sample : samples)
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Limit" + limit.name +
                            "_Color" + std::to_string(color) + "_" + sample.name,
                        big_float, limit.value, color, sample.point, sample.hit});
                }
            }
        }
    }
    return inputs;
}

class TestBiomorphBoundary : public PerturbationBiomorphTest, public WithParamInterface<BiomorphBoundaryInput>
{
public:
    TestBiomorphBoundary();

protected:
    void SetUp() override;
};

TestBiomorphBoundary::TestBiomorphBoundary() :
    PerturbationBiomorphTest{GetParam().big_float}
{
}

void TestBiomorphBoundary::SetUp()
{
    PerturbationBiomorphTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_magnitude_limit = GetParam().limit;
    g_magnitude_limit2 = std::sqrt(GetParam().limit);
    g_biomorph = GetParam().color;
    const std::complex<double> point{GetParam().point};
    set_initial_offset({-point.real(), -point.imag()});
}

TEST_P(TestBiomorphBoundary, usesStrictComponentsAtSelectedBailout)
{
    compare_pixel(GetParam().point, 128);

    EXPECT_EQ(1, g_real_color_iter);
    EXPECT_EQ(GetParam().hit ? GetParam().color : 1, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBiomorphBoundary, ValuesIn(biomorph_boundary_inputs()),
    [](const TestParamInfo<BiomorphBoundaryInput> &info) { return info.param.name; });

struct BiomorphOutsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
    ColorMethod outside;
    std::complex<double> point;
    std::optional<int> color;
};

void PrintTo(const BiomorphOutsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BiomorphOutsideInput> biomorph_outside_inputs()
{
    const std::array<PerturbationColorMethod, 2> inside_methods{
        {{"FixedColor", ColorMethod::COLOR}, {"Iterations", ColorMethod::ITER}}};
    const std::array<PerturbationColorMethod, 9> outside_methods{
        {{"Iterations", ColorMethod::ITER}, {"FixedColor", ColorMethod::COLOR}, {"Real", ColorMethod::REAL},
            {"Imaginary", ColorMethod::IMAG}, {"Product", ColorMethod::MULT}, {"Sum", ColorMethod::SUM},
            {"Atan", ColorMethod::ATAN}, {"Fmod", ColorMethod::FMOD}, {"TotalDistance", ColorMethod::TDIS}}};
    std::vector<BiomorphOutsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : inside_methods)
        {
            for (const PerturbationColorMethod &outside : outside_methods)
            {
                for (const bool hit : {true, false})
                {
                    std::optional<int> color;
                    if (hit)
                    {
                        color = outside.method == ColorMethod::COLOR ? 6 : 9;
                    }
                    else if (outside.method == ColorMethod::ITER)
                    {
                        color = 1;
                    }
                    else if (outside.method == ColorMethod::REAL)
                    {
                        color = 10;
                    }
                    else if (outside.method == ColorMethod::COLOR)
                    {
                        color = 6;
                    }
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Inside" + inside.name +
                            "_Outside" + outside.name + (hit ? "_Hit" : "_NoHit"),
                        big_float, inside.method, outside.method,
                        hit ? std::complex<double>{0.5, 0.0} : std::complex<double>{2.0, 2.0}, color});
                }
            }
        }
    }
    return inputs;
}

class TestBiomorphOutside : public PerturbationBiomorphTest, public WithParamInterface<BiomorphOutsideInput>
{
public:
    TestBiomorphOutside();

protected:
    void SetUp() override;
};

TestBiomorphOutside::TestBiomorphOutside() :
    PerturbationBiomorphTest{GetParam().big_float}
{
}

void TestBiomorphOutside::SetUp()
{
    PerturbationBiomorphTest::SetUp();
    g_biomorph = 9;
    g_outside_color = 6;
    g_close_proximity = 16.0;
    init_atan_colors();
    g_inside_method = GetParam().inside;
    g_outside_method = GetParam().outside;
}

TEST_P(TestBiomorphOutside, retainsOutsideFormulaAndFixedColor)
{
    compare_pixel(GetParam().point, 128);

    if (GetParam().color)
    {
        EXPECT_EQ(*GetParam().color, g_color);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBiomorphOutside, ValuesIn(biomorph_outside_inputs()),
    [](const TestParamInfo<BiomorphOutsideInput> &info) { return info.param.name; });

struct BiomorphInsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
};

void PrintTo(const BiomorphInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BiomorphInsideInput> biomorph_inside_inputs()
{
    const std::array<PerturbationColorMethod, 10> methods{
        {{"FixedColor", ColorMethod::COLOR}, {"Iterations", ColorMethod::ITER}, {"Magnitude", ColorMethod::ZMAG},
            {"Bof60", ColorMethod::BOF60}, {"Bof61", ColorMethod::BOF61}, {"Atan", ColorMethod::ATANI},
            {"Fmod", ColorMethod::FMODI}, {"EpsilonCross", ColorMethod::EPS_CROSS}, {"Period", ColorMethod::PERIOD},
            {"StarTrail", ColorMethod::STAR_TRAIL}}};
    std::vector<BiomorphInsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + inside.name, big_float, inside.method});
        }
    }
    return inputs;
}

class TestBiomorphInside : public PerturbationBiomorphTest, public WithParamInterface<BiomorphInsideInput>
{
public:
    TestBiomorphInside();

protected:
    void SetUp() override;
};

TestBiomorphInside::TestBiomorphInside() :
    PerturbationBiomorphTest{GetParam().big_float}
{
}

void TestBiomorphInside::SetUp()
{
    PerturbationBiomorphTest::SetUp();
    init_atan_colors();
    g_inside_method = GetParam().inside;
}

TEST_P(TestBiomorphInside, doesNotReplaceInsideMethod)
{
    g_biomorph = -1;
    compare_pixel({0.125, 0.0625}, 128);
    const int expected{g_color};

    g_biomorph = 9;
    compare_pixel({0.125, 0.0625}, 128);

    EXPECT_EQ(expected, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBiomorphInside, ValuesIn(biomorph_inside_inputs()),
    [](const TestParamInfo<BiomorphInsideInput> &info) { return info.param.name; });

struct BiomorphReplacementInput
{
    std::string name;
    bool big_float;
    ColorMethod outside;
    std::complex<double> point;
    int decomposition;
    int color;
};

void PrintTo(const BiomorphReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BiomorphReplacementInput> biomorph_replacement_inputs()
{
    struct Method
    {
        const char *name;
        ColorMethod outside;
        int color;
    };

    const Method methods[]{
        {"Iterations", ColorMethod::ITER, 11},
        {"Real", ColorMethod::REAL, 23},
        {"Imaginary", ColorMethod::IMAG, 29},
        {"FixedColor", ColorMethod::COLOR, 6},
    };
    std::vector<BiomorphReplacementInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const Method &method : methods)
        {
            const std::string name{std::string{big_float ? "HighPrecision_" : "Double_"} + method.name};
            inputs.push_back({name + "_Hit", big_float, method.outside, {0.5, 0.0}, 0,
                method.outside == ColorMethod::COLOR ? 6 : 31});
            inputs.push_back({name + "_NoHit", big_float, method.outside, {2.0, 2.0}, 0, method.color});
            inputs.push_back({name + "_DecomposedHit", big_float, method.outside, {0.5, 0.0}, 16,
                method.outside == ColorMethod::COLOR ? 6 : 11});
        }
    }
    return inputs;
}

class TestBiomorphReplacement : public PerturbationBiomorphTest, public WithParamInterface<BiomorphReplacementInput>
{
public:
    TestBiomorphReplacement();

protected:
    void SetUp() override;
};

TestBiomorphReplacement::TestBiomorphReplacement() :
    PerturbationBiomorphTest{GetParam().big_float}
{
}

void TestBiomorphReplacement::SetUp()
{
    PerturbationBiomorphTest::SetUp();
    g_biomorph = 9;
    g_inside_method = ColorMethod::COLOR;
    g_outside_color = 6;
    g_log_map_flag = 1;
    g_log_map_table_max_size = 128;
    g_log_map_table.assign(129, 99);
    g_log_map_table[1] = 11;
    g_log_map_table[9] = 31;
    g_log_map_table[10] = 23;
    g_log_map_table[18] = 29;
    m_engine.set_glitch_tolerance(8.0);
    g_outside_method = GetParam().outside;
    g_decomp[0] = GetParam().decomposition;
}

TEST_P(TestBiomorphReplacement, retainsFinishingAndMapping)
{
    compare_pixel(GetParam().point, 128, 0.125);

    EXPECT_EQ(GetParam().color, g_color);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBiomorphReplacement, ValuesIn(biomorph_replacement_inputs()),
    [](const TestParamInfo<BiomorphReplacementInput> &info) { return info.param.name; });

void PerturbationIterationTest::compare_distance_pixel(const std::complex<double> &center, const long max_iterations,
    const bool legacy, const double reference_offset, const DistancePixelOptions &options, const bool incremental)
{
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
        m_big_float = false;
    }
    s_standard_orbit.clear();
    s_perturbation_orbit.clear();
    s_standard_derivatives.clear();
    s_perturbation_derivatives.clear();
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_legacy{g_use_old_distance_estimator};
    ValueSaver saved_old_colors{g_old_demm_colors, options.old_colors};
    ValueSaver saved_width{g_distance_estimator_width_factor, options.width_factor};
    ValueSaver saved_virtual_width{g_distance_estimator_x_dots, options.virtual_size.x};
    ValueSaver saved_virtual_height{g_distance_estimator_y_dots, options.virtual_size.y};
    ValueSaver saved_limit{g_magnitude_limit};
    g_distance_estimator = options.value;
    g_inside_method = options.inside;
    g_max_iterations = max_iterations;
    g_user.perturbation = PerturbationMode::NO;
    g_image_region = options.region;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    g_dispatch = FractalDispatch{m_specific};
    StandardFractal standard;
    ASSERT_TRUE(standard.resume().empty());
    bool selected{};
    for (int i = 0; i < 10 && !selected; ++i)
    {
        selected = standard.select_pixel();
    }
    ASSERT_TRUE(selected);
    g_use_old_distance_estimator = legacy;
    const double initial_bailout{g_magnitude_limit};
    const DComplex point{center.real(), center.imag()};
    g_image_region = ImageRegion{point, point, point};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    g_col = WIDTH - 1;
    fill_pixel_grid();
    g_dispatch.set_orbit_calc(record_standard_orbit);
    ValueSaver saved_recording{s_standard_distance, &standard.distance_estimator()};
    DistanceEstimator expected;
    long expected_iterations{};
    double expected_magnitude{};
    DComplex expected_new_z{};
    EXPECT_CALL(m_driver, write_pixel(g_col, g_row, _))
        .WillOnce(Invoke(
            [&](int, int, int)
            {
                expected = standard.distance_estimator();
                expected_iterations = g_real_color_iter;
                expected_magnitude = g_magnitude;
                expected_new_z = g_new_z;
            }))
        .RetiresOnSaturation();
    const int expected_color{standard.calculate_standard_pixel(false)};
    ASSERT_FALSE(s_standard_orbit.empty());
    const std::complex<double> terminal{s_standard_orbit.back() * s_standard_orbit.back() + center};
    const std::vector<Byte> log_table{g_log_map_table};
    standard.finish();
    g_log_map_table = log_table;
    g_magnitude_limit = initial_bailout;
    m_specific.pert_pt = record_perturbation_orbit;
    initialize_reference(center - std::complex<double>{reference_offset, 0.0}, reference_offset);

    int actual_color{CALC_YIELDED};
    for (int chunk = 0; chunk < 1000 && actual_color == CALC_YIELDED; ++chunk)
    {
        const std::size_t before{s_perturbation_derivatives.size()};
        actual_color = m_engine.calculate_pixel(g_col, g_row, incremental);
        if (incremental)
        {
            EXPECT_LE(s_perturbation_derivatives.size() - before, 2048);
        }
    }
    EXPECT_TRUE(calc_ok(actual_color));
    if (options.compare_colors)
    {
        EXPECT_EQ(expected_color, actual_color);
    }

    const DistanceEstimator &actual{m_engine.distance_estimator()};
    EXPECT_EQ(expected_iterations, g_real_color_iter);
    EXPECT_EQ(expected_iterations, g_color_iter);
    EXPECT_NEAR(expected_new_z.x, g_new_z.x, std::max(1.0, std::abs(expected_new_z.x)) * 1e-12);
    EXPECT_NEAR(expected_new_z.y, g_new_z.y, std::max(1.0, std::abs(expected_new_z.y)) * 1e-12);
    EXPECT_NEAR(expected_magnitude, g_magnitude, std::max(1.0, expected_magnitude) * 1e-12);
    EXPECT_DOUBLE_EQ(expected.bailout(), actual.bailout());
    EXPECT_EQ(expected.escape_iteration(), actual.escape_iteration());
    EXPECT_NEAR(expected.escape_point().x, actual.escape_point().x, 1e-12);
    EXPECT_NEAR(expected.escape_point().y, actual.escape_point().y, 1e-12);
    EXPECT_NEAR(
        expected.derivative().x, actual.derivative().x, std::max(1.0, std::abs(expected.derivative().x)) * 1e-12);
    EXPECT_NEAR(
        expected.derivative().y, actual.derivative().y, std::max(1.0, std::abs(expected.derivative().y)) * 1e-12);
    const double expected_distance{expected.distance_squared({terminal.real(), terminal.imag()})};
    ASSERT_FALSE(s_perturbation_orbit.empty());
    const std::complex<double> actual_terminal{s_perturbation_orbit.back() * s_perturbation_orbit.back() + center};
    EXPECT_NEAR(expected_distance, actual.distance_squared({actual_terminal.real(), actual_terminal.imag()}),
        std::max(1.0, expected_distance) * 1e-12);
    ASSERT_GE(s_perturbation_derivatives.size(), s_standard_derivatives.size());
    const std::size_t first{s_perturbation_derivatives.size() - s_standard_derivatives.size()};
    for (std::size_t i = 0; i < s_standard_derivatives.size(); ++i)
    {
        SCOPED_TRACE(i);
        const DComplex derivative{s_standard_derivatives[i]};
        EXPECT_NEAR(
            derivative.x, s_perturbation_derivatives[first + i].x, std::max(1.0, std::abs(derivative.x)) * 1e-12);
        EXPECT_NEAR(
            derivative.y, s_perturbation_derivatives[first + i].y, std::max(1.0, std::abs(derivative.y)) * 1e-12);
    }
}

struct DistanceDerivativeInput
{
    std::string name;
    bool big_float;
    bool legacy;
    std::complex<double> point;
    double reference_offset;
};

void PrintTo(const DistanceDerivativeInput &input, std::ostream *out)
{
    *out << input.name;
}

struct DistanceDerivativeSample
{
    const char *name;
    std::complex<double> point;
    double reference_offset;
};

std::vector<DistanceDerivativeInput> distance_derivative_inputs(const std::vector<DistanceDerivativeSample> &samples)
{
    std::vector<DistanceDerivativeInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const DistanceDerivativeSample &sample : samples)
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (legacy ? "Legacy_" : "Current_") + sample.name,
                    big_float, legacy, sample.point, sample.reference_offset});
            }
        }
    }
    return inputs;
}

class DistanceDerivativeTest : public PerturbationIterationTest, public WithParamInterface<DistanceDerivativeInput>
{
public:
    DistanceDerivativeTest();

protected:
    void SetUp() override;
};

DistanceDerivativeTest::DistanceDerivativeTest() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void DistanceDerivativeTest::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(GetParam().reference_offset == 0.0 ? DEFAULT_PERTURBATION_TOLERANCE : 8.0);
}

class TestDistanceDerivativeSamples : public DistanceDerivativeTest
{
};

TEST_P(TestDistanceDerivativeSamples, matchesStandardDerivativesAndTerminalValues)
{
    compare_distance_pixel(GetParam().point, 32, GetParam().legacy);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceDerivativeSamples,
    ValuesIn(distance_derivative_inputs({
        {"RealExterior", {0.5, 0.0}, 0.0},
        {"ComplexExterior", {0.5, 0.5}, 0.0},
        {"Interior", {0.125, 0.0625}, 0.0},
    })),
    [](const TestParamInfo<DistanceDerivativeInput> &info) { return info.param.name; });

class TestDistanceDerivativeReplacement : public DistanceDerivativeTest
{
};

TEST_P(TestDistanceDerivativeReplacement, restartsDerivativeOnReferenceReplacement)
{
    compare_distance_pixel(GetParam().point, 32, GetParam().legacy, GetParam().reference_offset);

    EXPECT_GT(m_engine.reference_count(), 1);
    EXPECT_GT(s_perturbation_derivatives.size(), s_standard_derivatives.size());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceDerivativeReplacement,
    ValuesIn(distance_derivative_inputs({{"ComplexExterior", {0.5, 0.5}, 0.125}})),
    [](const TestParamInfo<DistanceDerivativeInput> &info) { return info.param.name; });

class TestDistanceDerivativeChunks : public DistanceDerivativeTest
{
};

TEST_P(TestDistanceDerivativeChunks, retainsDerivativesAcrossYieldsAndRetries)
{
    compare_distance_pixel(GetParam().point, 5000, GetParam().legacy, GetParam().reference_offset, {}, true);

    EXPECT_EQ(5000, g_real_color_iter);
    EXPECT_EQ(GetParam().reference_offset == 0.0 ? 1 : 2, m_engine.reference_count());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceDerivativeChunks,
    ValuesIn(distance_derivative_inputs({
        {"OriginalReference", {0.125, 0.0625}, 0.0},
        {"ReplacementReference", {0.125, 0.0625}, 0.125},
    })),
    [](const TestParamInfo<DistanceDerivativeInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, distanceEstimatorStopsBeforeOrbitUpdateAtDerivativeLimit)
{
    DistancePixelOptions options;
    options.width_factor = 1000;
    compare_distance_pixel({0.25, 0.0}, 4096, false, 0.0, options);
    EXPECT_LT(g_real_color_iter, 4096);
    EXPECT_EQ(s_standard_derivatives.size() + 1, g_real_color_iter);
}

struct DistanceBoundaryColorInput
{
    std::string name;
    bool big_float;
    long mode;
    int width;
    std::complex<double> point;
    bool escaped;
    std::optional<int> color;
};

void PrintTo(const DistanceBoundaryColorInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBoundaryColorInput> distance_boundary_inputs()
{
    std::vector<DistanceBoundaryColorInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const long mode : {1L, -9L})
        {
            for (const int width : {0, 1, 5, 100, -1, -2, -100})
            {
                const std::string name{std::string{big_float ? "HighPrecision_" : "Double_"} +
                    (mode > 0 ? "PositiveMode_" : "NegativeMode_") + (width < 0 ? "NegativeWidth" : "Width") +
                    std::to_string(std::abs(width))};
                const std::optional<int> exterior_color{
                    width == 100 ? std::optional<int>{mode > 0 ? 32 : 9} : std::nullopt};
                inputs.push_back({name + "_Exterior", big_float, mode, width, {0.5, 0.0}, true, exterior_color});
                inputs.push_back({name + "_Interior", big_float, mode, width, {0.125, 0.0625}, false, 32});
            }
        }
    }
    return inputs;
}

class TestDistanceBoundarySamples : public PerturbationIterationTest,
                                    public WithParamInterface<DistanceBoundaryColorInput>
{
public:
    TestDistanceBoundarySamples();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceBoundarySamples::TestDistanceBoundarySamples() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceBoundarySamples::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.compare_colors = true;
    m_options.value = GetParam().mode;
    m_options.width_factor = GetParam().width;
}

TEST_P(TestDistanceBoundarySamples, matchesStandardInsideAndExteriorColors)
{
    compare_distance_pixel(GetParam().point, 32, false, 0.0, m_options);

    if (GetParam().escaped)
    {
        EXPECT_LT(g_real_color_iter, 32);
    }
    else
    {
        EXPECT_EQ(32, g_real_color_iter);
    }
    if (GetParam().color)
    {
        EXPECT_EQ(*GetParam().color, g_color);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBoundarySamples, ValuesIn(distance_boundary_inputs()),
    [](const TestParamInfo<DistanceBoundaryColorInput> &info) { return info.param.name; });

struct DistanceBoundaryInsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
};

void PrintTo(const DistanceBoundaryInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBoundaryInsideInput> distance_boundary_inside_inputs()
{
    const PerturbationColorMethod methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
        {"Magnitude", ColorMethod::ZMAG},
        {"Bof60", ColorMethod::BOF60},
        {"Bof61", ColorMethod::BOF61},
        {"Atan", ColorMethod::ATANI},
        {"Fmod", ColorMethod::FMODI},
        {"EpsilonCross", ColorMethod::EPS_CROSS},
        {"Period", ColorMethod::PERIOD},
    };
    std::vector<DistanceBoundaryInsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &method : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + method.name, big_float, method.method});
        }
    }
    return inputs;
}

class TestDistanceBoundaryInside : public PerturbationIterationTest,
                                   public WithParamInterface<DistanceBoundaryInsideInput>
{
public:
    TestDistanceBoundaryInside();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceBoundaryInside::TestDistanceBoundaryInside() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceBoundaryInside::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.width_factor = 100;
    m_options.compare_colors = true;
    m_options.inside = GetParam().inside;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_bof_match_book_images = false;
    init_atan_colors();
}

TEST_P(TestDistanceBoundaryInside, usesInsideMethodBeforeDecompositionAndBiomorph)
{
    compare_distance_pixel({0.5, 0.5}, 32, false, 0.0, m_options);

    EXPECT_LT(g_real_color_iter, 32);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBoundaryInside, ValuesIn(distance_boundary_inside_inputs()),
    [](const TestParamInfo<DistanceBoundaryInsideInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, negativeDistanceBoundaryUsesDirectColor)
{
    g_log_map_flag = 1;
    DistancePixelOptions options;
    options.value = -250;
    options.width_factor = 100;
    options.compare_colors = true;

    compare_distance_pixel({0.5, 0.0}, 512, false, 0.0, options);

    EXPECT_EQ(250, g_color);
    EXPECT_NE(log_table_calc(250), g_color);
}

struct TwoColorDistanceInput
{
    std::string name;
    bool big_float;
    int inside_color;
    int width;
};

void PrintTo(const TwoColorDistanceInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<TwoColorDistanceInput> two_color_distance_inputs()
{
    std::vector<TwoColorDistanceInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int color : {0, 1})
        {
            for (const int width : {1, 100})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "InsideColor" +
                        std::to_string(color) + "_Width" + std::to_string(width),
                    big_float, color, width});
            }
        }
    }
    return inputs;
}

class TestTwoColorDistance : public PerturbationIterationTest, public WithParamInterface<TwoColorDistanceInput>
{
public:
    TestTwoColorDistance();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestTwoColorDistance::TestTwoColorDistance() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestTwoColorDistance::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_log_map_flag = 0;
    g_colors = 2;
    g_and_color = 1;
    g_inside_color = GetParam().inside_color;
    m_options.value = 1;
    m_options.width_factor = GetParam().width;
    m_options.compare_colors = true;
    m_options.inside = ColorMethod::COLOR;
}

TEST_P(TestTwoColorDistance, usesDirectBoundaryAndExteriorColors)
{
    compare_distance_pixel({0.5, 0.0}, 32, false, 0.0, m_options);

    EXPECT_EQ(GetParam().width == 1 ? !GetParam().inside_color : GetParam().inside_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestTwoColorDistance, ValuesIn(two_color_distance_inputs()),
    [](const TestParamInfo<TwoColorDistanceInput> &info) { return info.param.name; });

struct VirtualDistanceInput
{
    std::string name;
    bool big_float;
    bool rotated;
    bool narrow;
};

void PrintTo(const VirtualDistanceInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<VirtualDistanceInput> virtual_distance_inputs()
{
    std::vector<VirtualDistanceInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool rotated : {false, true})
        {
            for (const bool narrow : {true, false})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (rotated ? "Rotated_" : "Unrotated_") + (narrow ? "Narrow" : "FullSize"),
                    big_float, rotated, narrow});
            }
        }
    }
    return inputs;
}

class TestVirtualDistance : public PerturbationIterationTest, public WithParamInterface<VirtualDistanceInput>
{
public:
    TestVirtualDistance();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestVirtualDistance::TestVirtualDistance() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestVirtualDistance::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.width_factor = 5;
    m_options.compare_colors = true;
    m_options.region = GetParam().rotated ? ImageRegion{{-2.0, -2.0}, {2.0, 2.0}, {1.0, -2.0}}
                                          : ImageRegion{{-2.0, -2.0}, {2.0, 2.0}, {-2.0, 1.0}};
    m_options.virtual_size =
        GetParam().narrow ? (GetParam().rotated ? Point2i{800, 2} : Point2i{2, 600}) : Point2i{800, 600};
}

TEST_P(TestVirtualDistance, honorsVirtualImageDimensions)
{
    compare_distance_pixel({0.5, 0.0}, 32, false, 0.0, m_options);

    if (GetParam().narrow)
    {
        EXPECT_EQ(32, g_color);
    }
    else
    {
        EXPECT_EQ(g_real_color_iter, g_color);
        EXPECT_LT(g_color, 32);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestVirtualDistance, ValuesIn(virtual_distance_inputs()),
    [](const TestParamInfo<VirtualDistanceInput> &info) { return info.param.name; });

struct DistanceInsideMappingInput
{
    std::string name;
    bool big_float;
    ColorMethod outside;
    ColorMethod inside;
};

void PrintTo(const DistanceInsideMappingInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceInsideMappingInput> distance_inside_mapping_inputs()
{
    const PerturbationColorMethod outside_methods[]{
        {"Iterations", ColorMethod::ITER},
        {"FixedColor", ColorMethod::COLOR},
        {"Real", ColorMethod::REAL},
        {"Atan", ColorMethod::ATAN},
        {"Fmod", ColorMethod::FMOD},
        {"TotalDistance", ColorMethod::TDIS},
    };
    const PerturbationColorMethod inside_methods[]{
        {"Iterations", ColorMethod::ITER},
        {"FixedColor", ColorMethod::COLOR},
    };
    std::vector<DistanceInsideMappingInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &outside : outside_methods)
        {
            for (const PerturbationColorMethod &inside : inside_methods)
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Outside" + outside.name +
                        "_Inside" + inside.name,
                    big_float, outside.method, inside.method});
            }
        }
    }
    return inputs;
}

class TestDistanceInsideMapping : public PerturbationIterationTest,
                                  public WithParamInterface<DistanceInsideMappingInput>
{
public:
    TestDistanceInsideMapping();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceInsideMapping::TestDistanceInsideMapping() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceInsideMapping::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_log_map_flag = 1;
    init_atan_colors();
    m_options.width_factor = 100;
    m_options.compare_colors = true;
    m_options.inside = GetParam().inside;
    g_outside_method = GetParam().outside;
}

TEST_P(TestDistanceInsideMapping, usesInsideMappingInsteadOfOutsideAdjustments)
{
    compare_distance_pixel({0.5, 0.5}, 512, false, 0.0, m_options);

    if (GetParam().inside == ColorMethod::ITER)
    {
        EXPECT_EQ(standard_palette_color(log_table_calc(512)), g_color);
        EXPECT_NE(standard_palette_color(512), g_color);
    }
    else
    {
        EXPECT_EQ(g_inside_color, g_color);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceInsideMapping, ValuesIn(distance_inside_mapping_inputs()),
    [](const TestParamInfo<DistanceInsideMappingInput> &info) { return info.param.name; });

struct DistanceClassificationInput
{
    std::string name;
    bool big_float;
    long mode;
};

void PrintTo(const DistanceClassificationInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceClassificationInput> distance_classification_inputs()
{
    std::vector<DistanceClassificationInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const long mode : {1L, -9L})
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + (mode > 0 ? "PositiveMode" : "NegativeMode"),
                    big_float, mode});
        }
    }
    return inputs;
}

class TestDistanceClassification : public PerturbationIterationTest,
                                   public WithParamInterface<DistanceClassificationInput>
{
public:
    TestDistanceClassification();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceClassification::TestDistanceClassification() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceClassification::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(8.0);
    m_options.width_factor = 100;
    m_options.compare_colors = true;
    m_options.value = GetParam().mode;
}

TEST_P(TestDistanceClassification, retainsClassificationAfterReferenceReplacement)
{
    compare_distance_pixel({0.5, 0.5}, 32, false, 0.125, m_options);

    EXPECT_GT(m_engine.reference_count(), 1);
    EXPECT_EQ(GetParam().mode > 0 ? 32 : 9, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceClassification, ValuesIn(distance_classification_inputs()),
    [](const TestParamInfo<DistanceClassificationInput> &info) { return info.param.name; });

struct DistanceBandSampleInput
{
    std::string name;
    bool big_float;
    bool legacy;
    bool old_colors;
    long mode;
    std::complex<double> point;
};

void PrintTo(const DistanceBandSampleInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBandSampleInput> distance_band_sample_inputs()
{
    struct Sample
    {
        const char *name;
        std::complex<double> point;
    };

    const Sample samples[]{
        {"RealExterior", {0.5, 0.0}},
        {"ComplexExterior", {0.5, 0.5}},
        {"FarExterior", {2.0, 2.0}},
    };
    std::vector<DistanceBandSampleInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const bool old_colors : {false, true})
            {
                for (const long mode : {2L, 64L, 256L, 1024L})
                {
                    for (const Sample &sample : samples)
                    {
                        inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                                (legacy ? "Legacy_" : "Current_") + (old_colors ? "OldColors_" : "NewColors_") +
                                "Mode" + std::to_string(mode) + "_" + sample.name,
                            big_float, legacy, old_colors, mode, sample.point});
                    }
                }
            }
        }
    }
    return inputs;
}

class TestDistanceBandSamples : public PerturbationIterationTest, public WithParamInterface<DistanceBandSampleInput>
{
public:
    TestDistanceBandSamples();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceBandSamples::TestDistanceBandSamples() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceBandSamples::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.compare_colors = true;
    m_options.old_colors = GetParam().old_colors;
    m_options.value = GetParam().mode;
}

TEST_P(TestDistanceBandSamples, matchesCurrentAndLegacyStandardColors)
{
    compare_distance_pixel(GetParam().point, 32, GetParam().legacy, 0.0, m_options);

    EXPECT_LT(g_real_color_iter, 32);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBandSamples, ValuesIn(distance_band_sample_inputs()),
    [](const TestParamInfo<DistanceBandSampleInput> &info) { return info.param.name; });

struct DistanceBandOutsideInput
{
    std::string name;
    bool big_float;
    bool legacy;
    bool old_colors;
    ColorMethod outside;
};

void PrintTo(const DistanceBandOutsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBandOutsideInput> distance_band_outside_inputs()
{
    const PerturbationColorMethod methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Real", ColorMethod::REAL},
        {"Atan", ColorMethod::ATAN},
        {"Fmod", ColorMethod::FMOD},
        {"TotalDistance", ColorMethod::TDIS},
    };
    std::vector<DistanceBandOutsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const bool old_colors : {false, true})
            {
                for (const PerturbationColorMethod &method : methods)
                {
                    inputs.push_back(
                        {std::string{big_float ? "HighPrecision_" : "Double_"} + (legacy ? "Legacy_" : "Current_") +
                                (old_colors ? "OldColors_" : "NewColors_") + method.name,
                            big_float, legacy, old_colors, method.method});
                }
            }
        }
    }
    return inputs;
}

class TestDistanceBandOutside : public PerturbationIterationTest, public WithParamInterface<DistanceBandOutsideInput>
{
public:
    TestDistanceBandOutside();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceBandOutside::TestDistanceBandOutside() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceBandOutside::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.value = 1024;
    m_options.inside = ColorMethod::COLOR;
    m_options.compare_colors = true;
    m_options.old_colors = GetParam().old_colors;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_log_map_flag = 1;
    g_iteration_ranges = {512};
    g_outside_color = 6;
    g_outside_method = GetParam().outside;
}

TEST_P(TestDistanceBandOutside, bypassesOtherColoringAndMapping)
{
    compare_distance_pixel({2.0, 2.0}, 512, GetParam().legacy, 0.0, m_options);

    const DistanceEstimator &estimator{m_engine.distance_estimator()};
    EXPECT_FALSE(estimator.on_boundary(estimator.distance_squared(g_new_z)));
    EXPECT_EQ(standard_palette_color(estimator.band_color(estimator.distance_squared(g_new_z), GetParam().old_colors)),
        g_color);
    EXPECT_EQ(0, log_table_calc(g_color));
    EXPECT_GT(g_color, 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBandOutside, ValuesIn(distance_band_outside_inputs()),
    [](const TestParamInfo<DistanceBandOutsideInput> &info) { return info.param.name; });

struct DistanceBandReplacementInput
{
    std::string name;
    bool big_float;
    bool legacy;
    bool old_colors;
};

void PrintTo(const DistanceBandReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DistanceBandReplacementInput> distance_band_replacement_inputs()
{
    std::vector<DistanceBandReplacementInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const bool old_colors : {false, true})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (legacy ? "Legacy_" : "Current_") + (old_colors ? "OldColors" : "NewColors"),
                    big_float, legacy, old_colors});
            }
        }
    }
    return inputs;
}

class TestDistanceBandReplacement : public PerturbationIterationTest,
                                    public WithParamInterface<DistanceBandReplacementInput>
{
public:
    TestDistanceBandReplacement();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestDistanceBandReplacement::TestDistanceBandReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDistanceBandReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(8.0);
    m_options.value = 1024;
    m_options.compare_colors = true;
    m_options.old_colors = GetParam().old_colors;
}

TEST_P(TestDistanceBandReplacement, survivesReferenceReplacement)
{
    compare_distance_pixel({2.0, 2.0}, 32, GetParam().legacy, 0.125, m_options);

    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDistanceBandReplacement, ValuesIn(distance_band_replacement_inputs()),
    [](const TestParamInfo<DistanceBandReplacementInput> &info) { return info.param.name; });

struct RegularDistanceColorInput
{
    std::string name;
    bool big_float;
    bool legacy;
    long mode;
    ColorMethod outside;
};

void PrintTo(const RegularDistanceColorInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<RegularDistanceColorInput> regular_distance_color_inputs()
{
    const PerturbationColorMethod methods[]{
        {"Iterations", ColorMethod::ITER},
        {"FixedColor", ColorMethod::COLOR},
        {"Real", ColorMethod::REAL},
        {"Imaginary", ColorMethod::IMAG},
        {"Product", ColorMethod::MULT},
        {"Sum", ColorMethod::SUM},
        {"Atan", ColorMethod::ATAN},
        {"Fmod", ColorMethod::FMOD},
        {"TotalDistance", ColorMethod::TDIS},
    };
    std::vector<RegularDistanceColorInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const long mode : {1L, -9L})
            {
                for (const PerturbationColorMethod &method : methods)
                {
                    inputs.push_back(
                        {std::string{big_float ? "HighPrecision_" : "Double_"} + (legacy ? "Legacy_" : "Current_") +
                                (mode > 0 ? "PositiveMode_" : "NegativeMode_") + method.name,
                            big_float, legacy, mode, method.method});
                }
            }
        }
    }
    return inputs;
}

class TestRegularDistanceColor : public PerturbationIterationTest, public WithParamInterface<RegularDistanceColorInput>
{
public:
    TestRegularDistanceColor();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestRegularDistanceColor::TestRegularDistanceColor() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestRegularDistanceColor::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_options.inside = ColorMethod::COLOR;
    m_options.compare_colors = true;
    m_options.value = GetParam().mode;
    g_outside_color = 6;
    init_atan_colors();
    g_outside_method = GetParam().outside;
}

TEST_P(TestRegularDistanceColor, matchesStandardOutsideFinishing)
{
    compare_distance_pixel({2.0, 2.0}, 32, GetParam().legacy, 0.0, m_options);

    if (GetParam().legacy)
    {
        if (GetParam().mode == 1)
        {
            EXPECT_LT(m_engine.distance_estimator().escape_iteration(), g_real_color_iter);
        }
        else
        {
            EXPECT_EQ(m_engine.distance_estimator().escape_iteration(), g_real_color_iter);
        }
        EXPECT_EQ(m_engine.distance_estimator().escape_point(), g_new_z);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestRegularDistanceColor, ValuesIn(regular_distance_color_inputs()),
    [](const TestParamInfo<RegularDistanceColorInput> &info) { return info.param.name; });

struct LegacyDistanceMappingInput
{
    std::string name;
    bool big_float;
    int decomposition;
    int biomorph;
    long mode;
};

void PrintTo(const LegacyDistanceMappingInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<LegacyDistanceMappingInput> legacy_distance_mapping_inputs()
{
    std::vector<LegacyDistanceMappingInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int decomposition : {0, 16})
        {
            for (const int biomorph : {-1, 9})
            {
                for (const long mode : {1L, -9L})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Decomposition" +
                            std::to_string(decomposition) + (biomorph < 0 ? "_NoBiomorph_" : "_BiomorphNine_") +
                            (mode > 0 ? "PositiveMode" : "NegativeMode"),
                        big_float, decomposition, biomorph, mode});
                }
            }
        }
    }
    return inputs;
}

class TestLegacyDistanceMapping : public PerturbationIterationTest,
                                  public WithParamInterface<LegacyDistanceMappingInput>
{
public:
    TestLegacyDistanceMapping();

protected:
    void SetUp() override;
    DistancePixelOptions m_options;
};

TestLegacyDistanceMapping::TestLegacyDistanceMapping() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestLegacyDistanceMapping::SetUp()
{
    PerturbationIterationTest::SetUp();
    m_engine.set_glitch_tolerance(8.0);
    m_options.compare_colors = true;
    m_options.value = GetParam().mode;
    g_log_map_flag = 1;
    g_decomp[0] = GetParam().decomposition;
    g_biomorph = GetParam().biomorph;
}

TEST_P(TestLegacyDistanceMapping, retainsDecompositionBiomorphAndMapping)
{
    compare_distance_pixel({2.0, 2.0}, 512, true, 0.125, m_options);

    EXPECT_GT(m_engine.reference_count(), 1);
    EXPECT_GE(m_engine.distance_estimator().escape_iteration(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLegacyDistanceMapping, ValuesIn(legacy_distance_mapping_inputs()),
    [](const TestParamInfo<LegacyDistanceMappingInput> &info) { return info.param.name; });

struct PeriodicityScheduleInput
{
    std::string name;
    bool big_float;
    bool legacy;
    bool reset;
    int periodicity;
    double real;
};

void PrintTo(const PeriodicityScheduleInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PeriodicityScheduleInput> periodicity_schedule_inputs()
{
    std::vector<PeriodicityScheduleInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool legacy : {false, true})
        {
            for (const bool reset : {false, true})
            {
                for (const int periodicity : {0, 1, 5})
                {
                    for (const double real : {0.0, -1.0})
                    {
                        inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                                (legacy ? "LegacySchedule_" : "CurrentSchedule_") + (reset ? "Reset_" : "NoReset_") +
                                "Periodicity" + std::to_string(periodicity) + (real == 0.0 ? "_Zero" : "_MinusOne"),
                            big_float, legacy, reset, periodicity, real});
                    }
                }
            }
        }
    }
    return inputs;
}

class TestPeriodicitySchedule : public PerturbationIterationTest, public WithParamInterface<PeriodicityScheduleInput>
{
public:
    TestPeriodicitySchedule();

protected:
    void SetUp() override;
};

TestPeriodicitySchedule::TestPeriodicitySchedule() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPeriodicitySchedule::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_first_saved_and = GetParam().legacy ? 1 : 9;
    g_periodicity_next_saved_incr = GetParam().legacy ? 1 : 4;
    g_reset_periodicity = GetParam().reset;
    g_periodicity_check = GetParam().periodicity;
    g_close_enough = std::ldexp(1e-5, -GetParam().periodicity);
    g_old_color_iter = 0;
}

TEST_P(TestPeriodicitySchedule, matchesStandardSaveSchedule)
{
    compare_pixel({GetParam().real, 0.0}, 512);

    EXPECT_EQ(512, g_real_color_iter);
    EXPECT_EQ(7, g_color);
    if (GetParam().periodicity == 0)
    {
        EXPECT_EQ(511, s_standard_orbit.size());
    }
    else
    {
        EXPECT_LT(s_standard_orbit.size(), 511);
        if (GetParam().reset)
        {
            EXPECT_GT(s_standard_orbit.size(), 255);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPeriodicitySchedule, ValuesIn(periodicity_schedule_inputs()),
    [](const TestParamInfo<PeriodicityScheduleInput> &info) { return info.param.name; });

struct DetectedCycleInput
{
    std::string name;
    bool big_float;
    int periodicity;
};

void PrintTo(const DetectedCycleInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DetectedCycleInput> detected_cycle_inputs()
{
    std::vector<DetectedCycleInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int periodicity : {1, -1, -5})
        {
            inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                    (periodicity < 0 ? "NegativePeriodicity" : "Periodicity") + std::to_string(std::abs(periodicity)),
                big_float, periodicity});
        }
    }
    return inputs;
}

class TestDetectedCycleHighlight : public PerturbationIterationTest, public WithParamInterface<DetectedCycleInput>
{
public:
    TestDetectedCycleHighlight();

protected:
    void SetUp() override;
};

TestDetectedCycleHighlight::TestDetectedCycleHighlight() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestDetectedCycleHighlight::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_color = 3;
    g_periodicity_check = GetParam().periodicity;
    g_old_color_iter = 0;
}

TEST_P(TestDetectedCycleHighlight, highlightsDetectedCycleOnlyForNegativePeriodicity)
{
    compare_pixel({0.0, 0.0}, 128);

    EXPECT_EQ(GetParam().periodicity < 0 ? 7 : 3, g_color);
    EXPECT_LT(s_standard_orbit.size(), 127);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDetectedCycleHighlight, ValuesIn(detected_cycle_inputs()),
    [](const TestParamInfo<DetectedCycleInput> &info) { return info.param.name; });

struct UndetectedCycleInput
{
    std::string name;
    bool big_float;
    double real;
    long iterations;
    int color;
    std::optional<std::size_t> orbit_size;
};

void PrintTo(const UndetectedCycleInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<UndetectedCycleInput> undetected_cycle_inputs()
{
    std::vector<UndetectedCycleInput> inputs;
    for (const bool big_float : {false, true})
    {
        const std::string prefix{big_float ? "HighPrecision_" : "Double_"};
        inputs.push_back({prefix + "NoCycle", big_float, 0.25, 128, 3, 127});
        inputs.push_back({prefix + "IterationLimit", big_float, 0.0, 8, 3, std::nullopt});
        inputs.push_back({prefix + "Exterior", big_float, 0.5, 8, 4, std::nullopt});
    }
    return inputs;
}

class TestUndetectedCycleHighlight : public PerturbationIterationTest, public WithParamInterface<UndetectedCycleInput>
{
public:
    TestUndetectedCycleHighlight();

protected:
    void SetUp() override;
};

TestUndetectedCycleHighlight::TestUndetectedCycleHighlight() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestUndetectedCycleHighlight::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_color = 3;
    g_periodicity_check = -1;
    g_close_enough = 1e-14;
}

TEST_P(TestUndetectedCycleHighlight, doesNotHighlightUndetectedCycle)
{
    compare_pixel({GetParam().real, 0.0}, GetParam().iterations);

    EXPECT_EQ(GetParam().color, g_color);
    if (GetParam().orbit_size)
    {
        EXPECT_EQ(*GetParam().orbit_size, s_standard_orbit.size());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestUndetectedCycleHighlight, ValuesIn(undetected_cycle_inputs()),
    [](const TestParamInfo<UndetectedCycleInput> &info) { return info.param.name; });

struct PeriodColorInput
{
    std::string name;
    bool big_float;
    int periodicity;
    double real;
    int color;
};

void PrintTo(const PeriodColorInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PeriodColorInput> period_color_inputs()
{
    std::vector<PeriodColorInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int periodicity : {0, 1, -1})
        {
            for (const double real : {0.0, -1.0, 0.25})
            {
                const int period{real == 0.0 ? 1 : 2};
                const int color{periodicity == 0 || real == 0.25 ? 128 : periodicity < 0 ? 7 : period};
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (periodicity < 0 ? "NegativePeriodicity" : "Periodicity") +
                        std::to_string(std::abs(periodicity)) +
                        (real == 0.0           ? "_Zero"
                                : real == -1.0 ? "_MinusOne"
                                               : "_Quarter"),
                    big_float, periodicity, real, color});
            }
        }
    }
    return inputs;
}

class TestPeriodColors : public PerturbationIterationTest, public WithParamInterface<PeriodColorInput>
{
public:
    TestPeriodColors();

protected:
    void SetUp() override;
};

TestPeriodColors::TestPeriodColors() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPeriodColors::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::PERIOD;
    g_close_enough = 1e-14;
    g_periodicity_check = GetParam().periodicity;
}

TEST_P(TestPeriodColors, matchesStandardCycleLengthAndNoCycleResult)
{
    compare_pixel({GetParam().real, 0.0}, 128);

    EXPECT_EQ(GetParam().color, g_color);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPeriodColors, ValuesIn(period_color_inputs()),
    [](const TestParamInfo<PeriodColorInput> &info) { return info.param.name; });

struct CycleColorInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
    int biomorph;
    int color;
    std::optional<std::size_t> orbit_size;
};

void PrintTo(const CycleColorInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<CycleColorInput> cycle_color_inputs()
{
    const PerturbationColorMethod methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
        {"Period", ColorMethod::PERIOD},
        {"Bof60", ColorMethod::BOF60},
        {"Bof61", ColorMethod::BOF61},
    };
    std::vector<CycleColorInput> inputs;
    for (const bool big_float : {false, true})
    {
        const std::string prefix{big_float ? "HighPrecision_" : "Double_"};
        for (const PerturbationColorMethod &method : methods)
        {
            inputs.push_back({prefix + method.name + "_BiomorphFive", big_float, method.method, 5, 7, std::nullopt});
        }
        inputs.push_back({prefix + "Magnitude_NoBiomorph", big_float, ColorMethod::ZMAG, -1, 1, 127});
    }
    return inputs;
}

class TestCycleColors : public PerturbationIterationTest, public WithParamInterface<CycleColorInput>
{
public:
    TestCycleColors();

protected:
    void SetUp() override;
};

TestCycleColors::TestCycleColors() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestCycleColors::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_periodicity_check = -1;
    g_inside_color = 3;
    g_biomorph = GetParam().biomorph;
    g_outside_method = ColorMethod::REAL;
    g_inside_method = GetParam().inside;
}

TEST_P(TestCycleColors, retainsCycleHighlightingPrecedence)
{
    compare_pixel({0.0, 0.0}, 128);

    EXPECT_EQ(GetParam().color, g_color);
    if (GetParam().orbit_size)
    {
        EXPECT_EQ(*GetParam().orbit_size, s_standard_orbit.size());
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestCycleColors, ValuesIn(cycle_color_inputs()),
    [](const TestParamInfo<CycleColorInput> &info) { return info.param.name; });

std::vector<PeriodColorInput> mapped_period_color_inputs()
{
    std::vector<PeriodColorInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int periodicity : {1, -1})
        {
            for (const double real : {0.0, -1.0, 0.25})
            {
                const int color{real == 0.25 ? 13 : periodicity < 0 ? 7 : real == 0.0 ? 11 : 12};
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (periodicity < 0 ? "NegativePeriodicity" : "Periodicity") +
                        (real == 0.0           ? "_Zero"
                                : real == -1.0 ? "_MinusOne"
                                               : "_Quarter"),
                    big_float, periodicity, real, color});
            }
        }
    }
    return inputs;
}

class TestMappedPeriodColors : public TestPeriodColors
{
protected:
    void SetUp() override;
};

void TestMappedPeriodColors::SetUp()
{
    TestPeriodColors::SetUp();
    g_log_map_table.assign(129, 44);
    g_log_map_table_max_size = 128;
    g_log_map_flag = 1;
    g_log_map_calculate = false;
    g_log_map_table[1] = 11;
    g_log_map_table[2] = 12;
    g_log_map_table[7] = 99;
    g_log_map_table[128] = 13;
}

TEST_P(TestMappedPeriodColors, mapsPeriodColorsButNotHighlights)
{
    compare_pixel({GetParam().real, 0.0}, 128);

    EXPECT_EQ(GetParam().color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestMappedPeriodColors, ValuesIn(mapped_period_color_inputs()),
    [](const TestParamInfo<PeriodColorInput> &info) { return info.param.name; });

class PerturbationPotentialTest : public PerturbationIterationTest
{
public:
    explicit PerturbationPotentialTest(bool use_big_float);

protected:
    void SetUp() override;
};

PerturbationPotentialTest::PerturbationPotentialTest(const bool use_big_float) :
    PerturbationIterationTest{use_big_float}
{
}

void PerturbationPotentialTest::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_potential = {{23.0, 10.0, 4.0}, false, true};
}

struct PotentialPeriodicityInput
{
    std::string name;
    bool big_float;
    bool mapped;
    ColorMethod inside;
};

void PrintTo(const PotentialPeriodicityInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialPeriodicityInput> potential_periodicity_inputs()
{
    const PerturbationColorMethod methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Period", ColorMethod::PERIOD},
    };
    std::vector<PotentialPeriodicityInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool mapped : {false, true})
        {
            for (const PerturbationColorMethod &method : methods)
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                        (mapped ? "Mapped_" : "Unmapped_") + method.name,
                    big_float, mapped, method.method});
            }
        }
    }
    return inputs;
}

class TestPotentialPeriodicity : public PerturbationPotentialTest, public WithParamInterface<PotentialPeriodicityInput>
{
public:
    TestPotentialPeriodicity();

protected:
    void SetUp() override;
};

TestPotentialPeriodicity::TestPotentialPeriodicity() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void TestPotentialPeriodicity::SetUp()
{
    PerturbationPotentialTest::SetUp();
    g_log_map_table.clear();
    g_log_map_table_max_size = 128;
    g_log_map_flag = 0;
    g_log_map_calculate = false;
    g_periodicity_check = -1;
    g_inside_color = 3;
    if (GetParam().mapped)
    {
        g_log_map_flag = 1;
        g_log_map_table.assign(129, 44);
        g_log_map_table[3] = 31;
        g_log_map_table[23] = 41;
    }
    g_inside_method = GetParam().inside;
}

TEST_P(TestPotentialPeriodicity, takesPrecedenceOverPeriodicityColors)
{
    compare_pixel({0.0, 0.0}, 128);

    EXPECT_EQ(GetParam().inside == ColorMethod::COLOR ? GetParam().mapped ? 31 : 3
            : GetParam().mapped                       ? 41
                                                      : 23,
        g_color);
    EXPECT_LT(s_standard_orbit.size(), 127);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialPeriodicity, ValuesIn(potential_periodicity_inputs()),
    [](const TestParamInfo<PotentialPeriodicityInput> &info) { return info.param.name; });

struct PotentialExteriorInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
    int decomposition;
    int biomorph;
    ColorMethod outside;
};

void PrintTo(const PotentialExteriorInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialExteriorInput> potential_exterior_inputs()
{
    const PerturbationColorMethod inside_methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
    };
    const PerturbationColorMethod outside_methods[]{
        {"Iterations", ColorMethod::ITER},
        {"FixedColor", ColorMethod::COLOR},
        {"Real", ColorMethod::REAL},
        {"Imaginary", ColorMethod::IMAG},
        {"Product", ColorMethod::MULT},
        {"Sum", ColorMethod::SUM},
        {"Atan", ColorMethod::ATAN},
        {"Fmod", ColorMethod::FMOD},
        {"TotalDistance", ColorMethod::TDIS},
    };
    std::vector<PotentialExteriorInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : inside_methods)
        {
            for (const int decomposition : {0, 16})
            {
                for (const int biomorph : {-1, 9})
                {
                    for (const PerturbationColorMethod &outside : outside_methods)
                    {
                        inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Inside" +
                                inside.name + "_Decomposition" + std::to_string(decomposition) +
                                (biomorph < 0 ? "_NoBiomorph_" : "_BiomorphNine_") + "Outside" + outside.name,
                            big_float, inside.method, decomposition, biomorph, outside.method});
                    }
                }
            }
        }
    }
    return inputs;
}

class TestPotentialExterior : public PerturbationPotentialTest, public WithParamInterface<PotentialExteriorInput>
{
public:
    TestPotentialExterior();

protected:
    void SetUp() override;
};

TestPotentialExterior::TestPotentialExterior() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void TestPotentialExterior::SetUp()
{
    PerturbationPotentialTest::SetUp();
    g_outside_color = 6;
    g_inside_method = GetParam().inside;
    g_decomp[0] = GetParam().decomposition;
    g_biomorph = GetParam().biomorph;
    g_outside_method = GetParam().outside;
}

TEST_P(TestPotentialExterior, overridesExteriorColoring)
{
    compare_pixel({0.5, 0.5}, 128);

    EXPECT_EQ(20, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialExterior, ValuesIn(potential_exterior_inputs()),
    [](const TestParamInfo<PotentialExteriorInput> &info) { return info.param.name; });

struct PotentialInsideInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
};

void PrintTo(const PotentialInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialInsideInput> potential_inside_inputs()
{
    const PerturbationColorMethod methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
        {"Magnitude", ColorMethod::ZMAG},
        {"Bof60", ColorMethod::BOF60},
        {"Bof61", ColorMethod::BOF61},
        {"Atan", ColorMethod::ATANI},
        {"Fmod", ColorMethod::FMODI},
        {"StarTrail", ColorMethod::STAR_TRAIL},
        {"Period", ColorMethod::PERIOD},
        {"EpsilonCross", ColorMethod::EPS_CROSS},
    };
    std::vector<PotentialInsideInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &method : methods)
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + method.name, big_float, method.method});
        }
    }
    return inputs;
}

class TestPotentialInside : public PerturbationPotentialTest, public WithParamInterface<PotentialInsideInput>
{
public:
    TestPotentialInside();

protected:
    void SetUp() override;
};

TestPotentialInside::TestPotentialInside() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void TestPotentialInside::SetUp()
{
    PerturbationPotentialTest::SetUp();
    g_outside_method = ColorMethod::COLOR;
    g_outside_color = 6;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_inside_method = GetParam().inside;
}

TEST_P(TestPotentialInside, overridesInsideColoring)
{
    compare_pixel({0.125, 0.0625}, 128);

    EXPECT_EQ(GetParam().inside == ColorMethod::COLOR ? g_inside_color : 23, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialInside, ValuesIn(potential_inside_inputs()),
    [](const TestParamInfo<PotentialInsideInput> &info) { return info.param.name; });

struct PotentialMappedSampleInput
{
    std::string name;
    bool big_float;
    ColorMethod inside;
    ColorMethod outside;
    std::complex<double> point;
    int color;
};

void PrintTo(const PotentialMappedSampleInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialMappedSampleInput> potential_mapped_sample_inputs()
{
    const PerturbationColorMethod inside_methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
        {"Magnitude", ColorMethod::ZMAG},
    };
    const PerturbationColorMethod outside_methods[]{
        {"FixedColor", ColorMethod::COLOR},
        {"Iterations", ColorMethod::ITER},
        {"Real", ColorMethod::REAL},
    };
    std::vector<PotentialMappedSampleInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationColorMethod &inside : inside_methods)
        {
            for (const PerturbationColorMethod &outside : outside_methods)
            {
                const std::string prefix{std::string{big_float ? "HighPrecision_" : "Double_"} + "Inside" +
                    inside.name + "_Outside" + outside.name};
                inputs.push_back({prefix + "_Exterior", big_float, inside.method, outside.method, {0.5, 0.5}, 31});
                inputs.push_back({prefix + "_Interior", big_float, inside.method, outside.method, {0.125, 0.0625},
                    inside.method == ColorMethod::COLOR ? 37 : 41});
            }
        }
    }
    return inputs;
}

class TestPotentialMappedSamples : public PerturbationPotentialTest,
                                   public WithParamInterface<PotentialMappedSampleInput>
{
public:
    TestPotentialMappedSamples();

protected:
    void SetUp() override;
};

TestPotentialMappedSamples::TestPotentialMappedSamples() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void TestPotentialMappedSamples::SetUp()
{
    PerturbationPotentialTest::SetUp();
    g_log_map_flag = 1;
    g_log_map_table_max_size = 128;
    g_log_map_table.assign(129, 99);
    g_log_map_table[20] = 31;
    g_log_map_table[23] = 41;
    g_log_map_table[7] = 37;
    g_outside_color = 6;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_inside_method = GetParam().inside;
    g_outside_method = GetParam().outside;
}

TEST_P(TestPotentialMappedSamples, mapsFixedAndSpecialColorsExactlyOnce)
{
    compare_pixel(GetParam().point, 128);

    EXPECT_EQ(GetParam().color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialMappedSamples, ValuesIn(potential_mapped_sample_inputs()),
    [](const TestParamInfo<PotentialMappedSampleInput> &info) { return info.param.name; });

struct PotentialLogMapInput
{
    std::string name;
    bool big_float;
    bool calculate;
};

void PrintTo(const PotentialLogMapInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialLogMapInput> potential_log_map_inputs()
{
    std::vector<PotentialLogMapInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool calculate : {false, true})
        {
            inputs.push_back(
                {std::string{big_float ? "HighPrecision_" : "Double_"} + (calculate ? "Calculated" : "Table"),
                    big_float, calculate});
        }
    }
    return inputs;
}

class TestPotentialLogMap : public PerturbationPotentialTest, public WithParamInterface<PotentialLogMapInput>
{
public:
    TestPotentialLogMap();

protected:
    void SetUp() override;
};

TestPotentialLogMap::TestPotentialLogMap() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void TestPotentialLogMap::SetUp()
{
    PerturbationPotentialTest::SetUp();
    g_inside_method = ColorMethod::COLOR;
    g_inside_color = 11;
    g_outside_method = ColorMethod::COLOR;
    g_colors = 16;
    g_and_color = 15;
    g_log_map_flag = -1;
    g_log_map_table_max_size = 128;
    g_max_iterations = 128;
    g_log_map_calculate = GetParam().calculate;
    g_log_map_fly_calculate = GetParam().calculate ? LogMapCalculate::ON_THE_FLY : LogMapCalculate::USE_LOG_TABLE;
    g_log_map_table.assign(GetParam().calculate ? 0 : 129, 0);
    setup_log_table();
}

TEST_P(TestPotentialLogMap, usesSelectedMapWithFixedColor)
{
    compare_pixel({0.125, 0.0625}, 128);

    EXPECT_EQ(log_table_calc(g_inside_color), g_color);
    EXPECT_NE(g_inside_color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialLogMap, ValuesIn(potential_log_map_inputs()),
    [](const TestParamInfo<PotentialLogMapInput> &info) { return info.param.name; });

struct PotentialStorageInput
{
    std::string name;
    bool big_float;
    bool mapped;
    bool retry;
    int row;
};

void PrintTo(const PotentialStorageInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<PotentialStorageInput> potential_storage_inputs()
{
    std::vector<PotentialStorageInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const bool mapped : {false, true})
        {
            for (const bool retry : {false, true})
            {
                for (const int row : {0, HEIGHT - 1})
                {
                    inputs.push_back(
                        {std::string{big_float ? "HighPrecision_" : "Double_"} + (mapped ? "Mapped_" : "Unmapped_") +
                                (retry ? "Retry_" : "NoRetry_") + (row == 0 ? "TopRow" : "BottomRow"),
                            big_float, mapped, retry, row});
                }
            }
        }
    }
    return inputs;
}

class PotentialStorageTest : public PerturbationPotentialTest, public WithParamInterface<PotentialStorageInput>
{
public:
    PotentialStorageTest();

protected:
    void SetUp() override;
};

PotentialStorageTest::PotentialStorageTest() :
    PerturbationPotentialTest{GetParam().big_float}
{
}

void PotentialStorageTest::SetUp()
{
    PerturbationPotentialTest::SetUp();
    init_memory();
    std::uint64_t blocks{};
    ASSERT_TRUE(disk_video_memory_blocks(WIDTH, HEIGHT * 2, 256, 0, blocks));
    start_disk_storage(WIDTH, HEIGHT * 2, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, blocks, MemoryLocation::MEMORY));
    g_potential = {{23.0, 10.0, 4.0}, true, true};
    g_outside_method = ColorMethod::COLOR;
    g_outside_color = 6;
    g_decomp[0] = 16;
    g_biomorph = 9;
    g_log_map_table_max_size = 128;
    g_log_map_flag = GetParam().mapped ? 1 : 0;
    if (GetParam().mapped)
    {
        g_log_map_table.assign(129, 99);
        g_log_map_table[20] = 31;
        g_log_map_table[7] = 37;
        g_log_map_table[2] = 12;
    }
    else
    {
        g_log_map_table.clear();
    }
    m_engine.set_glitch_tolerance(GetParam().retry ? 8.0 : DEFAULT_PERTURBATION_TOLERANCE);
    g_row = GetParam().row;
    g_inside_method = ColorMethod::COLOR;
}

class TestPotentialExteriorStorage : public PotentialStorageTest
{
};

TEST_P(TestPotentialExteriorStorage, storesRawPotentialSeparatelyFromMappedPixel)
{
    compare_pixel({0.5, 0.5}, 128, GetParam().retry ? 0.125 : 0.0);

    EXPECT_EQ(GetParam().mapped ? 31 : 20, g_color);
    EXPECT_EQ(20, read_disk_pixel(g_col, g_row));
    EXPECT_GT(read_disk_pixel(g_col, g_row + HEIGHT), 0);
    EXPECT_LT(read_disk_pixel(g_col, g_row + HEIGHT), 255);
    if (GetParam().retry)
    {
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialExteriorStorage, ValuesIn(potential_storage_inputs()),
    [](const TestParamInfo<PotentialStorageInput> &info) { return info.param.name; });

class TestPotentialInsideStorage : public PotentialStorageTest
{
};

TEST_P(TestPotentialInsideStorage, storesUnmappedFixedInsideColor)
{
    compare_pixel({0.125, 0.0625}, 128);

    EXPECT_EQ(GetParam().mapped ? 37 : 7, g_color);
    EXPECT_EQ(7, read_disk_pixel(g_col, g_row));
    EXPECT_EQ(0, read_disk_pixel(g_col, g_row + HEIGHT));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialInsideStorage, ValuesIn(potential_storage_inputs()),
    [](const TestParamInfo<PotentialStorageInput> &info) { return info.param.name; });

class TestPotentialEpsilonStorage : public PotentialStorageTest
{
};

TEST_P(TestPotentialEpsilonStorage, doesNotWritePotentialForEpsilonCrossExit)
{
    g_inside_method = ColorMethod::EPS_CROSS;

    compare_pixel({}, 128, 0.0, 1);

    EXPECT_EQ(GetParam().mapped ? 12 : 2, g_color);
    EXPECT_EQ(255, read_disk_pixel(g_col, g_row));
    EXPECT_EQ(255, read_disk_pixel(g_col, g_row + HEIGHT));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPotentialEpsilonStorage, ValuesIn(potential_storage_inputs()),
    [](const TestParamInfo<PotentialStorageInput> &info) { return info.param.name; });

class TestPeriodicityReplacement : public PerturbationIterationTest, public WithParamInterface<DetectedCycleInput>
{
public:
    TestPeriodicityReplacement();

protected:
    void SetUp() override;
};

TestPeriodicityReplacement::TestPeriodicityReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPeriodicityReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_color = 3;
    m_engine.set_glitch_tolerance(2.0);
    g_periodicity_check = GetParam().periodicity;
    g_inside_method = GetParam().periodicity < 0 ? ColorMethod::COLOR : ColorMethod::PERIOD;
}

TEST_P(TestPeriodicityReplacement, retainsPeriodicityColorAfterReferenceReplacement)
{
    compare_pixel({-1.0, 0.0}, 128, 0.125);

    EXPECT_EQ(GetParam().periodicity < 0 ? 7 : 2, g_color);
    EXPECT_GT(m_engine.reference_count(), 1);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPeriodicityReplacement,
    Values(DetectedCycleInput{"Double_PositivePeriodicity", false, 1},
        DetectedCycleInput{"Double_NegativePeriodicity", false, -1},
        DetectedCycleInput{"HighPrecision_PositivePeriodicity", true, 1},
        DetectedCycleInput{"HighPrecision_NegativePeriodicity", true, -1}),
    [](const TestParamInfo<DetectedCycleInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, periodicityPreservesEscapingPixelsAndAdaptiveStart)
{
    g_periodicity_check = 1;
    g_old_color_iter = 0;
    compare_pixel({0.5, 0.0}, 8);
    EXPECT_EQ(4, g_real_color_iter);
    EXPECT_EQ(14, g_old_color_iter);

    compare_pixel({0.0, 0.0}, 64);
    EXPECT_GT(s_standard_orbit.size(), 14);
    EXPECT_LT(s_standard_orbit.size(), 63);
    EXPECT_EQ(0, g_old_color_iter);
}

class TestReconstructedPeriodicity : public PerturbationIterationTest,
                                     public WithParamInterface<PerturbationOffsetInput>
{
public:
    TestReconstructedPeriodicity();

protected:
    void SetUp() override;
};

TestReconstructedPeriodicity::TestReconstructedPeriodicity() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestReconstructedPeriodicity::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_periodicity_check = 1;
    g_close_enough = 1e-14;
    m_engine.set_glitch_tolerance(0.0);
    g_old_color_iter = 0;
}

TEST_P(TestReconstructedPeriodicity, checksReconstructedOrbitInsteadOfReferenceOrDelta)
{
    compare_pixel({0.25, 0.0}, 128, GetParam().offset);

    EXPECT_EQ(127, s_perturbation_orbit.size());
    EXPECT_EQ(1, m_engine.reference_count());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestReconstructedPeriodicity,
    Values(PerturbationOffsetInput{"Double_NoDisplacement", false, 0.0},
        PerturbationOffsetInput{"Double_QuarterDisplacement", false, 0.25},
        PerturbationOffsetInput{"HighPrecision_NoDisplacement", true, 0.0},
        PerturbationOffsetInput{"HighPrecision_QuarterDisplacement", true, 0.25}),
    [](const TestParamInfo<PerturbationOffsetInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, periodicityToleranceMatchesStandardAndSurvivesReferenceReplacement)
{
    g_periodicity_check = 1;
    std::size_t loose_iterations{};
    for (const double tolerance : {1e-4, 1e-12})
    {
        SCOPED_TRACE(tolerance);
        g_close_enough = tolerance;
        g_old_color_iter = 0;
        m_engine.set_glitch_tolerance(8.0);

        compare_pixel({0.125, 0.0625}, 128, 0.125);

        EXPECT_GT(m_engine.reference_count(), 1);
        EXPECT_LT(s_standard_orbit.size(), 127);
        if (loose_iterations == 0)
        {
            loose_iterations = s_standard_orbit.size();
        }
        else
        {
            EXPECT_GT(s_standard_orbit.size(), loose_iterations);
        }
    }
}

struct OrbitDependentInsideInput
{
    const char *name;
    ColorMethod inside;
};

void PrintTo(const OrbitDependentInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrbitDependentPeriodicity : public TestPerturbationPixels,
                                      public WithParamInterface<OrbitDependentInsideInput>
{
protected:
    void SetUp() override;
};

void TestOrbitDependentPeriodicity::SetUp()
{
    TestPerturbationPixels::SetUp();
    g_periodicity_check = 1;
    m_specific.pert_pt = record_perturbation_orbit;
    g_inside_method = GetParam().inside;
    m_engine.initialize_frame({}, {}, {});
    s_perturbation_orbit.clear();
}

TEST_P(TestOrbitDependentPeriodicity, disablesPeriodicityForOrbitDependentInsideMethod)
{
    EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

    const long iterations{GetParam().inside == ColorMethod::STAR_TRAIL ? 16 : g_max_iterations};
    EXPECT_EQ(iterations - 1, s_perturbation_orbit.size());
    EXPECT_EQ(iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestOrbitDependentPeriodicity,
    Values(OrbitDependentInsideInput{"Magnitude", ColorMethod::ZMAG},
        OrbitDependentInsideInput{"StarTrail", ColorMethod::STAR_TRAIL}),
    [](const TestParamInfo<OrbitDependentInsideInput> &info) { return info.param.name; });

struct DeepPeriodicityInput
{
    std::string name;
    BFMathType math;
    double tolerance;
    bool replace_reference;
};

void PrintTo(const DeepPeriodicityInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<DeepPeriodicityInput> deep_periodicity_inputs()
{
    std::vector<DeepPeriodicityInput> inputs;
    for (const BFMathType math : {BFMathType::BIG_NUM, BFMathType::BIG_FLT})
    {
        for (const double tolerance : {1e-25, 1e-50})
        {
            for (const bool replace_reference : {false, true})
            {
                inputs.push_back({std::string{math == BFMathType::BIG_NUM ? "BigNum_" : "BigFloat_"} +
                        (tolerance == 1e-25 ? "Tolerance1eMinus25_" : "Tolerance1eMinus50_") +
                        (replace_reference ? "ReplacementReference" : "OriginalReference"),
                    math, tolerance, replace_reference});
            }
        }
    }
    return inputs;
}

class TestDeepPeriodicity : public TestPerturbationPixels, public WithParamInterface<DeepPeriodicityInput>
{
protected:
    void SetUp() override;

private:
    ValueSaver<InitOrbitMode> m_saved_mode{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver<double> m_saved_real{g_params[0], 0.0};
    ValueSaver<double> m_saved_imag{g_params[1], 0.0};
};

void TestDeepPeriodicity::SetUp()
{
    TestPerturbationPixels::SetUp();
    g_periodicity_check = 1;
    g_first_saved_and = 1;
    g_periodicity_next_saved_incr = 1;
    g_max_iterations = 64;
    m_specific.pert_pt = record_perturbation_orbit;
    m_engine.finish();
    {
        ValueSaver saved_type{
            g_fractal_type, GetParam().math == BFMathType::BIG_NUM ? FractalType::MANDEL : FractalType::JULIA_Z_POWER};
        init_bf_dec(80);
    }
    m_big_float = true;
    ASSERT_EQ(GetParam().math, g_bf_math);
    if (GetParam().math == BFMathType::BIG_NUM)
    {
        float_to_bn(g_close_enough_bn, GetParam().tolerance);
    }
    else
    {
        float_to_bf(g_close_enough_bf, GetParam().tolerance);
    }
    const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {})};
    float_to_bf(center.x, -2.0);
    clear_bf(center.y);
    m_engine.set_glitch_tolerance(GetParam().replace_reference ? 2.0 : 0.0);
    g_old_color_iter = 0;
    s_perturbation_orbit.clear();
}

TEST_P(TestDeepPeriodicity, usesActivePrecisionTolerance)
{
    EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, 0));

    EXPECT_EQ(64, g_real_color_iter);
    EXPECT_EQ(
        (GetParam().tolerance > 1e-40 ? 3 : 63) + (GetParam().replace_reference ? 1 : 0), s_perturbation_orbit.size());
    EXPECT_EQ(GetParam().replace_reference ? 2 : 1, m_engine.reference_count());
    EXPECT_EQ(0, g_old_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestDeepPeriodicity, ValuesIn(deep_periodicity_inputs()),
    [](const TestParamInfo<DeepPeriodicityInput> &info) { return info.param.name; });

struct PrecisionReplayInput
{
    const char *name;
    bool big_float;
    bool incremental;
};

void PrintTo(const PrecisionReplayInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestPeriodicityReplay : public PerturbationIterationTest, public WithParamInterface<PrecisionReplayInput>
{
public:
    TestPeriodicityReplay();

protected:
    void SetUp() override;

private:
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0};
};

TestPeriodicityReplay::TestPeriodicityReplay() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPeriodicityReplay::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_user.bailout_value = 4;
    g_max_iterations = 1024;
    g_periodicity_check = 1;
    g_reset_periodicity = true;
    g_inside_method = ColorMethod::ITER;
    g_image_region = ImageRegion{};
    fill_pixel_grid();
    if (GetParam().big_float)
    {
        init_bf_dec(1610);
        m_big_float = true;
        ASSERT_EQ(BFMathType::BIG_NUM, g_bf_math);
        clear_bn(g_x_min_bn);
        clear_bn(g_y_max_bn);
        clear_bn(g_delta_x_bn);
        clear_bn(g_delta_y_bn);
        clear_bn(g_delta2_x_bn);
        clear_bn(g_delta2_y_bn);
        float_to_bn(g_close_enough_bn, g_close_enough);
        g_dispatch.set_per_pixel(mandel_per_pixel_bn);
        g_dispatch.set_orbit_calc(julia_orbit_bn);
    }
}

TEST_P(TestPeriodicityReplay, fitsAcceptedPrecision)
{
    StandardFractal standard;
    const int expected_color{standard.calculate_standard_pixel(false)};
    const long expected_iterations{g_real_color_iter};
    EXPECT_EQ(g_max_iterations, expected_iterations);
    const int initial_stack{save_stack()};
    if (GetParam().big_float)
    {
        const BFComplex center{m_engine.initialize_frame_bf({}, {})};
        clear_bf(center.x);
        clear_bf(center.y);
    }
    else
    {
        m_engine.initialize_frame({}, {}, {});
    }
    g_old_color_iter = 0;
    m_specific.pert_pt = record_perturbation_orbit;
    s_perturbation_orbit.clear();
    int color{CALC_YIELDED};
    for (int step = 0; color == CALC_YIELDED && step < 100; ++step)
    {
        ASSERT_NO_THROW(color = m_engine.calculate_pixel(g_col, g_row, GetParam().incremental));
    }
    EXPECT_EQ(expected_color, color);
    EXPECT_EQ(expected_iterations, g_real_color_iter);
    EXPECT_LT(s_perturbation_orbit.size(), g_max_iterations - 1);
    m_engine.finish();
    EXPECT_EQ(initial_stack, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPeriodicityReplay,
    Values(PrecisionReplayInput{"Double_Atomic", false, false}, PrecisionReplayInput{"Double_Incremental", false, true},
        PrecisionReplayInput{"HighPrecision_Atomic", true, false},
        PrecisionReplayInput{"HighPrecision_Incremental", true, true}),
    [](const TestParamInfo<PrecisionReplayInput> &info) { return info.param.name; });

TEST_F(TestPerturbationPixels, subDoublePeriodicityToleranceStillDetectsExactCycles)
{
    {
        ValueSaver saved_type{g_fractal_type, FractalType::JULIA_Z_POWER};
        init_bf_dec(400);
    }
    m_big_float = true;
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    str_to_bf(g_close_enough_bf, "1e-350");
    g_periodicity_check = 1;
    g_first_saved_and = 1;
    g_periodicity_next_saved_incr = 1;
    g_max_iterations = 16;
    m_specific.pert_pt = record_perturbation_orbit;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    clear_bf(center.x);
    clear_bf(center.y);
    s_perturbation_orbit.clear();

    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));

    EXPECT_EQ(16, g_real_color_iter);
    EXPECT_EQ(3, s_perturbation_orbit.size());
}

struct PerturbationBailoutMethod
{
    const char *name;
    Bailout test;
};

constexpr std::array<PerturbationBailoutMethod, 7> TERMINATION_BAILOUTS{{
    {"Modulus", Bailout::MOD},
    {"Real", Bailout::REAL},
    {"Imaginary", Bailout::IMAG},
    {"Or", Bailout::OR},
    {"And", Bailout::AND},
    {"Manhattan", Bailout::MANH},
    {"ManhattanReal", Bailout::MANR},
}};

struct BailoutTerminationInput
{
    std::string name;
    bool big_float;
    Bailout test;
    std::complex<double> point;
    double limit;
    std::optional<long> iterations;
};

void PrintTo(const BailoutTerminationInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BailoutTerminationInput> bailout_termination_inputs()
{
    struct Sample
    {
        const char *name;
        std::complex<double> point;
        std::array<long, 7> iterations;
    };

    const Sample samples[]{
        {"OneReal", {1.0, 0.0}, {1, 1, 6, 1, 6, 1, 1}},
        {"OneImaginary", {0.0, 1.0}, {6, 6, 6, 6, 6, 1, 6}},
        {"OneMinusImaginary", {1.0, -1.0}, {1, 2, 1, 1, 2, 1, 1}},
        {"HalfComplex", {0.5, 0.5}, {4, 4, 5, 4, 5, 4, 4}},
    };
    std::vector<BailoutTerminationInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationBailoutMethod &method : TERMINATION_BAILOUTS)
        {
            for (const Sample &sample : samples)
            {
                for (const double limit : {1.0, 4.0, 16.0})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + method.name + "_" +
                            sample.name + "_Limit" + std::to_string(static_cast<int>(limit)),
                        big_float, method.test, sample.point, limit,
                        limit == 4.0 ? std::optional<long>{sample.iterations[static_cast<std::size_t>(method.test)]}
                                     : std::nullopt});
                }
            }
        }
    }
    return inputs;
}

class TestBailoutTermination : public PerturbationIterationTest, public WithParamInterface<BailoutTerminationInput>
{
public:
    TestBailoutTermination();

protected:
    void SetUp() override;
};

TestBailoutTermination::TestBailoutTermination() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestBailoutTermination::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_bailout_test = GetParam().test;
    set_bailout_formula(GetParam().test);
    g_magnitude_limit = GetParam().limit;
}

TEST_P(TestBailoutTermination, matchesStandardTermination)
{
    compare_pixel(GetParam().point, 6);

    if (GetParam().iterations)
    {
        EXPECT_EQ(*GetParam().iterations, g_real_color_iter);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBailoutTermination, ValuesIn(bailout_termination_inputs()),
    [](const TestParamInfo<BailoutTerminationInput> &info) { return info.param.name; });

struct BailoutReplacementInput
{
    std::string name;
    bool big_float;
    Bailout test;
    double tolerance;
};

void PrintTo(const BailoutReplacementInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BailoutReplacementInput> bailout_replacement_inputs()
{
    std::vector<BailoutReplacementInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationBailoutMethod &method : TERMINATION_BAILOUTS)
        {
            for (const double tolerance : {0.0, 2.0})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + method.name +
                        (tolerance == 0.0 ? "_NoReplacement" : "_ForcedReplacement"),
                    big_float, method.test, tolerance});
            }
        }
    }
    return inputs;
}

class TestBailoutReplacement : public PerturbationIterationTest, public WithParamInterface<BailoutReplacementInput>
{
public:
    TestBailoutReplacement();

protected:
    void SetUp() override;
};

TestBailoutReplacement::TestBailoutReplacement() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestBailoutReplacement::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_bailout_test = GetParam().test;
    set_bailout_formula(GetParam().test);
    // Fixed-point bailout truncates limits; compare precisions at an integer.
    g_magnitude_limit = 1.0;
    m_engine.set_glitch_tolerance(GetParam().tolerance);
}

TEST_P(TestBailoutReplacement, retainsSelectedBailoutWithDisplacedReference)
{
    compare_pixel({0.25, 0.125}, 8, 0.125);

    if (GetParam().tolerance == 0.0)
    {
        EXPECT_EQ(1, m_engine.reference_count());
    }
    else
    {
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBailoutReplacement, ValuesIn(bailout_replacement_inputs()),
    [](const TestParamInfo<BailoutReplacementInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, escapeOnLastAllowedUpdateMatchesStandardState)
{
    compare_pixel({0.5, 0.0}, 5);
    EXPECT_EQ(4, g_real_color_iter);
}

TEST_P(TestPerturbationIterations, reachingTheLimitDoesNotPerformAnotherUpdate)
{
    compare_pixel({0.5, 0.0}, 4);
    EXPECT_EQ(4, g_real_color_iter);
    EXPECT_DOUBLE_EQ(1.62890625, g_new_z.x);
    EXPECT_EQ(7, g_color);
}

TEST_P(TestPerturbationIterations, smallestImageLimitMatchesStandardState)
{
    compare_pixel({0.5, 0.0}, 2);
    EXPECT_EQ(2, g_real_color_iter);
    EXPECT_DOUBLE_EQ(0.75, g_new_z.x);
}

TEST_P(TestPerturbationIterations, boundedTerminalOrbitMatchesStandardState)
{
    compare_pixel({0.25, 0.125}, 8);
    EXPECT_EQ(8, g_real_color_iter);
}

TEST_P(TestPerturbationIterations, referenceGenerationDoesNotDiscardAnExtraUpdate)
{
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;

    compare_pixel({0.5, 0.0}, 4);

    EXPECT_EQ(4, s_reference_coordinates.size());
}

TEST_P(TestPerturbationIterations, displacedPixelPublishesItsOwnTerminalState)
{
    g_max_iterations = 5;
    initialize_reference({0.25, 0.0}, 7.0 / 12.0, 0.0);

    EXPECT_EQ(4, m_engine.calculate_pixel(5, HEIGHT / 2 - 1));

    expect_orbit_state({4, {1.62890625, 0.0}, {3.1533355712890625, 0.0}, 3.1533355712890625 * 3.1533355712890625,
        3.1533355712890625 * 3.1533355712890625, 0.0});
}

struct EscapePaletteInput
{
    std::string name;
    bool big_float;
    int colors;
    ColorMethod inside;
    double real;
    long iterations;
    int color;
};

void PrintTo(const EscapePaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<EscapePaletteInput> escape_palette_inputs()
{
    struct Sample
    {
        double real;
        long iterations;
        std::array<int, 4> colors;
    };

    // Midpoints between escape-count boundaries avoid ambiguous bailout comparisons.
    constexpr std::array<Sample, 6> SAMPLES{{
        {0.25014833044660284, 255, {1, 3, 15, 255}},
        {0.2501471869631891, 256, {0, 0, 1, 1}},
        {0.25014605665218975, 257, {1, 1, 2, 2}},
        {0.2500373634283777, 511, {1, 3, 1, 1}},
        {0.25003721845435867, 512, {0, 0, 2, 2}},
        {0.25000935820314196, 1024, {0, 0, 4, 4}},
    }};
    constexpr std::array<int, 4> PALETTE_SIZES{2, 4, 16, 256};
    std::vector<EscapePaletteInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (std::size_t palette = 0; palette < PALETTE_SIZES.size(); ++palette)
        {
            for (const Sample &sample : SAMPLES)
            {
                for (const ColorMethod inside : {ColorMethod::ITER, ColorMethod::COLOR})
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Palette" +
                            std::to_string(PALETTE_SIZES[palette]) + "_Iterations" + std::to_string(sample.iterations) +
                            (inside == ColorMethod::ITER ? "_InsideIterations" : "_InsideFixed"),
                        big_float, PALETTE_SIZES[palette], inside, sample.real, sample.iterations,
                        sample.colors[palette]});
                }
            }
        }
    }
    return inputs;
}

class TestEscapePalette : public PerturbationIterationTest, public WithParamInterface<EscapePaletteInput>
{
public:
    TestEscapePalette();

protected:
    void SetUp() override;

private:
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0};
};

TestEscapePalette::TestEscapePalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestEscapePalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
    g_inside_method = GetParam().inside;
    g_max_iterations = 2048;
    const DComplex point{GetParam().real, 0.0};
    g_image_region = ImageRegion{point, point, point};
    fill_pixel_grid();
}

TEST_P(TestEscapePalette, foldsEscapeCountOnlyAtActivePaletteSize)
{
    StandardFractal standard;
    const int expected_color{standard.calculate_standard_pixel(false)};
    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
    EXPECT_EQ(GetParam().color, expected_color);
    initialize_reference({GetParam().real, 0.0});

    EXPECT_EQ(expected_color, m_engine.calculate_pixel(g_col, g_row));
    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
    EXPECT_EQ(expected_color, m_pixels[g_row * WIDTH + g_col]);
    EXPECT_GT(m_engine.reference_count(), 0);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestEscapePalette, ValuesIn(escape_palette_inputs()),
    [](const TestParamInfo<EscapePaletteInput> &info) { return info.param.name; });

struct InsidePaletteInput
{
    std::string name;
    bool big_float;
    int colors;
    long iterations;
};

void PrintTo(const InsidePaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InsidePaletteInput> inside_palette_inputs()
{
    std::vector<InsidePaletteInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int colors : {2, 4, 16, 256})
        {
            for (const long iterations : {255L, 256L, 257L, 511L, 512L, 1024L})
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Palette" +
                        std::to_string(colors) + "_Iterations" + std::to_string(iterations),
                    big_float, colors, iterations});
            }
        }
    }
    return inputs;
}

class TestInsidePalette : public PerturbationIterationTest, public WithParamInterface<InsidePaletteInput>
{
public:
    TestInsidePalette();

protected:
    void SetUp() override;
};

TestInsidePalette::TestInsidePalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestInsidePalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_inside_method = ColorMethod::ITER;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
}

TEST_P(TestInsidePalette, foldsInsideIterationCountAtActivePaletteSize)
{
    compare_pixel({}, GetParam().iterations);

    EXPECT_EQ(GetParam().iterations, g_real_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInsidePalette, ValuesIn(inside_palette_inputs()),
    [](const TestParamInfo<InsidePaletteInput> &info) { return info.param.name; });

struct BoundaryPaletteInput
{
    std::string name;
    bool big_float;
    int colors;
    ColorMethod inside;
    int inside_color;
    DebugFlags debug;
    int color;
};

void PrintTo(const BoundaryPaletteInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BoundaryPaletteInput> boundary_palette_inputs()
{
    std::vector<BoundaryPaletteInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int colors : {2, 4, 16, 256})
        {
            const std::string prefix{
                std::string{big_float ? "HighPrecision_" : "Double_"} + "Palette" + std::to_string(colors)};
            // The original sweep changed the fixed inside color after its first case.
            inputs.push_back({prefix + "_InsideIterations", big_float, colors, ColorMethod::ITER, colors == 2 ? 7 : 0,
                DebugFlags::NONE, 1});
            inputs.push_back({prefix + "_FixedInside", big_float, colors, ColorMethod::COLOR, 0, DebugFlags::NONE, 1});
            inputs.push_back({prefix + "_FixedInsideForcedError", big_float, colors, ColorMethod::COLOR, 0,
                DebugFlags::FORCE_BOUNDARY_TRACE_ERROR, 0});
        }
    }
    return inputs;
}

class TestBoundaryPalette : public PerturbationIterationTest, public WithParamInterface<BoundaryPaletteInput>
{
public:
    TestBoundaryPalette();

protected:
    void SetUp() override;

private:
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
};

TestBoundaryPalette::TestBoundaryPalette() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestBoundaryPalette::SetUp()
{
    PerturbationIterationTest::SetUp();
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    g_colors = GetParam().colors;
    g_and_color = g_colors - 1;
    g_inside_method = GetParam().inside;
    g_inside_color = GetParam().inside_color;
    g_debug_flag = GetParam().debug;
}

TEST_P(TestBoundaryPalette, preservesNonzeroBoundaryTraceColors)
{
    compare_pixel({}, 256);

    EXPECT_EQ(GetParam().color, g_color);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBoundaryPalette, ValuesIn(boundary_palette_inputs()),
    [](const TestParamInfo<BoundaryPaletteInput> &info) { return info.param.name; });

TEST_P(TestPerturbationIterations, trueColorIterationOutputUsesTheFullCountBeforePaletteOutput)
{
    ValueSaver saved_true_mode{g_true_mode, TrueColorMode::ITERATE};
    ValueSaver saved_fill{g_fill_type, FillType::POINTS};
    ValueSaver saved_haze{g_haze, 0};
    init_memory();
    std::uint64_t blocks{};
    ASSERT_TRUE(disk_video_memory_blocks(WIDTH * 3, HEIGHT, 256, 0, blocks));
    const MemoryHandle backing{memory_alloc(DISK_STORAGE_BLOCK_SIZE, blocks, MemoryLocation::MEMORY)};
    start_disk_storage(WIDTH * 3, HEIGHT, 256, backing);
    g_plot = plot_iteration_color;

    compare_pixel({}, 257);

    EXPECT_EQ(257, g_real_color_iter);
    EXPECT_EQ(7, g_color);
    Byte red{};
    Byte green{};
    Byte blue{};
    read_targa_pixel(g_col, g_row, &red, &green, &blue);
    EXPECT_EQ(0, red);
    EXPECT_EQ(1, green);
    EXPECT_EQ(1, blue);
}

struct FrameCoordinateTransform
{
    double x_magnification;
    double rotation;
    double skew;
};

struct FrameCoordinateContext
{
    const char *name;
    std::optional<FrameCoordinateTransform> transform;
    Inversion inversion;
    CalcMode pass;
    Point2i offset;
};

struct FrameCoordinateInput
{
    std::string name;
    bool big_float;
    Point2i size;
    FractalType type;
    double tolerance;
    FrameCoordinateContext context;
};

void PrintTo(const FrameCoordinateInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<FrameCoordinateInput> frame_coordinate_inputs()
{
    const Inversion inversion{3, 0.5, {2.0, 1.0}, {0.5, 2.0, 1.0}};
    const FrameCoordinateTransform reduced_transform{0.6, 37.0, 21.0};
    const FrameCoordinateContext contexts[]{
        {"Default", std::nullopt, {}, CalcMode::ONE_PASS, {}},
        {"Rotation37", FrameCoordinateTransform{1.0, 37.0, 0.0}, {}, CalcMode::ONE_PASS, {}},
        {"Skew21", FrameCoordinateTransform{1.0, 0.0, 21.0}, {}, CalcMode::ONE_PASS, {}},
        {"XMagnification2p5", FrameCoordinateTransform{2.5, 0.0, 0.0}, {}, CalcMode::ONE_PASS, {}},
        {"CombinedNegative", FrameCoordinateTransform{0.6, -72.0, -18.0}, {}, CalcMode::ONE_PASS, {}},
        {"QuarterTurn", FrameCoordinateTransform{1.0, 90.0, 0.0}, {}, CalcMode::ONE_PASS, {}},
        {"ReducedBoundaryTrace", reduced_transform, {}, CalcMode::BOUNDARY_TRACE, {3, 2}},
        {"ReducedSolidGuess", reduced_transform, {}, CalcMode::SOLID_GUESS, {3, 2}},
        {"ReducedInvertedBoundaryTrace", reduced_transform, inversion, CalcMode::BOUNDARY_TRACE, {3, 2}},
        {"ReducedInvertedSolidGuess", reduced_transform, inversion, CalcMode::SOLID_GUESS, {3, 2}},
        {"InvertedAffine", reduced_transform, inversion, CalcMode::ONE_PASS, {}},
    };
    std::vector<FrameCoordinateInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const FrameCoordinateContext &context : contexts)
        {
            for (const double tolerance : {1e-6, 2.0})
            {
                for (const Point2i &size : {Point2i{8, 6}, Point2i{6, 8}, Point2i{17, 9}, Point2i{9, 17}})
                {
                    for (const FractalType type : {FractalType::MANDEL, FractalType::BURNING_SHIP})
                    {
                        inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + context.name +
                                (tolerance > 1.0 ? "_ReplacementTolerance_" : "_SmallTolerance_") + "Width" +
                                std::to_string(size.x) + "_Height" + std::to_string(size.y) +
                                (type == FractalType::MANDEL ? "_Mandel" : "_BurningShip"),
                            big_float, size, type, tolerance, context});
                    }
                }
            }
        }
    }
    return inputs;
}

class TestPerturbationFrameCoordinates : public PerturbationIterationTest,
                                         public WithParamInterface<FrameCoordinateInput>
{
public:
    TestPerturbationFrameCoordinates();

protected:
    void SetUp() override;
    ImageRegion m_frame_region{{-0.3, 0.1}, {0.2, 0.35}, {-0.3, 0.1}};
    std::vector<Point2i> m_frame_plots;
    std::vector<int> m_frame_pixels;

private:
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0};
};

TestPerturbationFrameCoordinates::TestPerturbationFrameCoordinates() :
    PerturbationIterationTest{GetParam().big_float}
{
}

void TestPerturbationFrameCoordinates::SetUp()
{
    PerturbationIterationTest::SetUp();
    const FrameCoordinateInput &input{GetParam()};
    if (input.context.transform)
    {
        const FrameCoordinateTransform &transform{*input.context.transform};
        cvt_corners(-0.05, 0.225, 8.0, transform.x_magnification, transform.rotation, transform.skew);
        m_frame_region = g_image_region;
    }
    g_inversion = input.context.inversion;
    g_user.perturbation = PerturbationMode::YES;
    g_user.perturbation_tolerance = input.tolerance;
    g_max_iterations = 2;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt = [](const std::complex<double> &reference, std::complex<double> &delta,
                             const std::complex<double> &parameter_delta)
    {
        // Geometry describes the parameter, not the independently initialized seed.
        s_coordinates.push_back({{g_col, g_row}, s_reference_coordinates.back() + parameter_delta, parameter_delta});
        mandel_perturb(reference, delta, parameter_delta);
    };
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            Invoke([this](const int col, const int row) { return m_frame_pixels.at(row * g_screen_x_dots + col); }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](const int col, const int row, const int color)
            {
                if (const StandardFractal *calculation{active_standard_fractal()})
                {
                    EXPECT_TRUE(calculation->standard_pass_status().perturbation_active);
                }
                m_frame_pixels.at(row * g_screen_x_dots + col) = color;
                m_frame_plots.push_back({col, row});
            }));

    const Point2i size{input.size};
    const Point2i offset{input.context.offset};
    const ImageRegion &region{m_frame_region};
    g_fractal_type = input.type;
    g_screen_x_dots = size.x + 2 * offset.x;
    g_screen_y_dots = size.y + 2 * offset.y;
    g_logical_screen = {size.x, size.y, offset.x, offset.y, size.x - 1.0, size.y - 1.0};
    g_start_pt = g_i_start_pt = g_begin_pt = {};
    g_stop_pt = g_i_stop_pt = {size.x - 1, size.y - 1};
    g_image_region = region;
    g_delta_x = (region.m_max.x - region.m_3rd.x) / (size.x - 1);
    g_delta_y = (region.m_max.y - region.m_3rd.y) / (size.y - 1);
    g_delta_x2 = (region.m_3rd.x - region.m_min.x) / (size.y - 1);
    g_delta_y2 = (region.m_3rd.y - region.m_min.y) / (size.x - 1);
    alloc_pixel_grid();
    fill_pixel_grid();
    if (input.big_float)
    {
        init_bf_dec(60);
        m_big_float = true;
        float_to_bf(g_bf_x_min, g_image_region.m_min.x);
        float_to_bf(g_bf_x_max, g_image_region.m_max.x);
        float_to_bf(g_bf_y_min, g_image_region.m_min.y);
        float_to_bf(g_bf_y_max, g_image_region.m_max.y);
        float_to_bf(g_bf_x_3rd, region.m_3rd.x);
        float_to_bf(g_bf_y_3rd, region.m_3rd.y);
    }
    g_std_calc_mode = input.context.pass;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_dispatch = FractalDispatch{m_specific};
    s_coordinates.clear();
    s_reference_coordinates.clear();
    m_frame_plots.clear();
    m_frame_pixels.assign(g_screen_x_dots * g_screen_y_dots, 0);
}

TEST_P(TestPerturbationFrameCoordinates, matchesStandardGridAndPlotCoverage)
{
    const FrameCoordinateInput &input{GetParam()};
    const Point2i size{input.size};
    const Point2i offset{input.context.offset};
    const CalcMode pass{input.context.pass};
    StandardFractal calculation;
    calculation.resume();
    for (int increments = 0; increments < size.x * size.y + 10 && !calculation.done(); ++increments)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(pass, g_std_calc_mode);
    EXPECT_EQ(input.big_float, g_bf_math != BFMathType::NONE);
    ASSERT_FALSE(s_reference_coordinates.empty());
    DComplex expected_center{m_frame_region.center()};
    if (g_inversion.invert != 0)
    {
        g_inversion.apply(expected_center);
    }
    EXPECT_NEAR(expected_center.x, s_reference_coordinates.front().real(), 1e-14);
    EXPECT_NEAR(input.type == FractalType::BURNING_SHIP ? -expected_center.y : expected_center.y,
        s_reference_coordinates.front().imag(), 1e-14);
    if (input.tolerance > 1.0)
    {
        EXPECT_GT(s_reference_coordinates.size(), g_max_iterations);
    }
    std::vector<int> visits(size.x * size.y);
    for (const CoordinateSample &sample : s_coordinates)
    {
        g_col = sample.pixel.x;
        g_row = sample.pixel.y;
        DComplex expected{dx_pixel(), dy_pixel()};
        if (g_inversion.invert != 0)
        {
            g_inversion.apply(expected);
        }
        EXPECT_NEAR(expected.x, sample.value.real(), 1e-14);
        EXPECT_NEAR(input.type == FractalType::BURNING_SHIP ? -expected.y : expected.y, sample.value.imag(), 1e-14);
        ++visits.at(g_row * size.x + g_col);
    }
    EXPECT_THAT(visits, Contains(Gt(0)));
    if (pass == CalcMode::ONE_PASS)
    {
        EXPECT_THAT(visits, Each(Ge(1)));
        ASSERT_EQ(size.x * size.y, m_frame_plots.size());
        for (int i = 0; i < size.x * size.y; ++i)
        {
            EXPECT_EQ((Point2i{offset.x + i % size.x, offset.y + i / size.x}), m_frame_plots[i]);
        }
    }
    std::vector<int> plotted(size.x * size.y);
    for (const Point2i &pixel : m_frame_plots)
    {
        ASSERT_GE(pixel.x, offset.x);
        ASSERT_GE(pixel.y, offset.y);
        ASSERT_LT(pixel.x, offset.x + size.x);
        ASSERT_LT(pixel.y, offset.y + size.y);
        ++plotted.at((pixel.y - offset.y) * size.x + pixel.x - offset.x);
    }
    EXPECT_THAT(plotted, Each(Ge(1)));
    calculation.finish();
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestPerturbationFrameCoordinates, ValuesIn(frame_coordinate_inputs()),
    [](const TestParamInfo<FrameCoordinateInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPerturbationIterations, Bool());

class BurningShipPerturbationTest : public PerturbationIterationTest
{
public:
    explicit BurningShipPerturbationTest(bool use_big_float);

protected:
    void compare_native_pixel(
        const std::complex<double> &point, double reference_offset = 0.0, bool replace_reference = false);
    static int record_native_orbit();
    static void record_perturbation(const std::complex<double> &reference, std::complex<double> &delta,
        const std::complex<double> &parameter_delta);

    ValueSaver<double> m_saved_degree{g_params[2]};
};

BurningShipPerturbationTest::BurningShipPerturbationTest(const bool use_big_float) :
    PerturbationIterationTest{use_big_float}
{
}

int BurningShipPerturbationTest::record_native_orbit()
{
    const DComplex value{g_bf_math == BFMathType::NONE ? g_old_z : cmplx_bf_to_float(g_old_z_bf)};
    s_standard_orbit.emplace_back(value.x, value.y);
    return g_bf_math == BFMathType::NONE ? burning_ship_orbit() : burning_ship_bf_fractal();
}

void BurningShipPerturbationTest::record_perturbation(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter_delta)
{
    const std::complex<double> value{reference + delta};
    s_perturbation_orbit.push_back(static_cast<int>(g_params[2]) > 2 ? std::conj(value) : value);
    burning_ship_perturb(reference, delta, parameter_delta);
}

void BurningShipPerturbationTest::compare_native_pixel(
    const std::complex<double> &point, const double reference_offset, const bool replace_reference)
{
    SCOPED_TRACE(reference_offset);
    SCOPED_TRACE(replace_reference);
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
        m_big_float = false;
    }
    g_fractal_type = FractalType::BURNING_SHIP;
    m_specific = *get_fractal_specific(g_fractal_type);
    g_dispatch = FractalDispatch{m_specific};
    g_max_iterations = 16;
    g_col = WIDTH - 1;
    g_row = 0;
    s_standard_orbit.clear();
    s_perturbation_orbit.clear();
    initialize_reference(std::conj(point) - std::complex<double>{reference_offset, -reference_offset}, reference_offset,
        reference_offset);
    m_engine.set_glitch_tolerance(0.0);

    const DComplex pixel{point.real(), point.imag()};
    g_image_region = ImageRegion{pixel, pixel, pixel};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    fill_pixel_grid();
    if (m_use_big_float)
    {
        g_dispatch.set_per_pixel(mandel_per_pixel_bf);
        float_to_bf(g_bf_x_min, point.real());
        float_to_bf(g_bf_y_max, point.imag());
        clear_bf(g_delta_x_bf);
        clear_bf(g_delta_y_bf);
        clear_bf(g_delta2_x_bf);
        clear_bf(g_delta2_y_bf);
    }
    g_dispatch.set_orbit_calc(record_native_orbit);
    m_specific.pert_pt = record_perturbation;
    if (replace_reference)
    {
        m_specific.pert_pt = [](const std::complex<double> &reference, std::complex<double> &delta,
                                 const std::complex<double> &parameter_delta)
        {
            record_perturbation(reference, delta, parameter_delta);
            if (s_perturbation_orbit.size() == 1)
            {
                delta = {std::numeric_limits<double>::quiet_NaN(), 0.0};
            }
        };
    }
    StandardFractal standard;
    const int expected_color{standard.calculate_standard_pixel(false)};
    const long expected_iterations{g_real_color_iter};
    const DComplex expected_value{m_use_big_float ? cmplx_bf_to_float(g_new_z_bf) : g_new_z};
    const DComplex expected_previous{g_old_z};

    const int color{m_engine.calculate_pixel(g_col, g_row)};

    EXPECT_EQ(expected_color, color);
    EXPECT_EQ(expected_iterations, g_real_color_iter);
    EXPECT_NEAR(expected_value.x, g_new_z.x, 1e-12);
    EXPECT_NEAR(expected_value.y, g_new_z.y, 1e-12);
    if (!m_use_big_float)
    {
        EXPECT_NEAR(expected_previous.x, g_old_z.x, 1e-12);
        EXPECT_NEAR(expected_previous.y, g_old_z.y, 1e-12);
    }
    EXPECT_EQ(replace_reference ? 2 : 1, m_engine.reference_count());
    ASSERT_GE(s_perturbation_orbit.size(), s_standard_orbit.size());
    const std::size_t first{s_perturbation_orbit.size() - s_standard_orbit.size()};
    for (std::size_t i = 0; i < s_standard_orbit.size(); ++i)
    {
        SCOPED_TRACE(i);
        const double tolerance{1e-12 * std::max(1.0, std::abs(s_standard_orbit[i]))};
        EXPECT_NEAR(s_standard_orbit[i].real(), s_perturbation_orbit[first + i].real(), tolerance);
        EXPECT_NEAR(s_standard_orbit[i].imag(), s_perturbation_orbit[first + i].imag(), tolerance);
    }
}

struct BurningShipReference
{
    const char *name;
    double offset;
    bool replace;
};

const std::vector<BurningShipReference> SHIP_ORIGINAL_AND_REPLACED{
    {"Original", 0.0, false},
    {"DisplacedReplacement", 0.125, true},
};

struct ShipReferenceInput
{
    std::string name;
    bool big_float;
    int degree;
    double offset;
    bool replace;
};

void PrintTo(const ShipReferenceInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ShipReferenceInput> ship_reference_inputs(
    const std::vector<int> &degrees, const std::vector<BurningShipReference> &references)
{
    std::vector<ShipReferenceInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int degree : degrees)
        {
            for (const BurningShipReference &reference : references)
            {
                inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + "Degree" +
                        std::to_string(degree) + "_" + reference.name,
                    big_float, degree, reference.offset, reference.replace});
            }
        }
    }
    return inputs;
}

class BurningShipReferenceTest : public BurningShipPerturbationTest, public WithParamInterface<ShipReferenceInput>
{
public:
    BurningShipReferenceTest();

protected:
    void SetUp() override;
};

BurningShipReferenceTest::BurningShipReferenceTest() :
    BurningShipPerturbationTest{GetParam().big_float}
{
}

void BurningShipReferenceTest::SetUp()
{
    BurningShipPerturbationTest::SetUp();
    g_params[2] = GetParam().degree;
}

class TestShipManrCoordinates : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipManrCoordinates::SetUp()
{
    BurningShipReferenceTest::SetUp();
    g_bailout_test = Bailout::MANR;
    set_bailout_formula(Bailout::MANR);
    g_magnitude_limit = 2.0;
}

TEST_P(TestShipManrCoordinates, observesNativeCoordinates)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipManrCoordinates,
    ValuesIn(ship_reference_inputs({2, 3, 4, 5}, SHIP_ORIGINAL_AND_REPLACED)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

class TestShipImaginaryColor : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipImaginaryColor::SetUp()
{
    BurningShipReferenceTest::SetUp();
    g_outside_method = ColorMethod::IMAG;
    g_magnitude_limit = 1.5;
}

TEST_P(TestShipImaginaryColor, observesNativeCoordinates)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipImaginaryColor,
    ValuesIn(ship_reference_inputs({2, 3, 4, 5}, SHIP_ORIGINAL_AND_REPLACED)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

class TestShipPreciseManr : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipPreciseManr::SetUp()
{
    BurningShipReferenceTest::SetUp();
    g_bailout_test = Bailout::MANR;
    set_bailout_formula(Bailout::MANR);
    // Near, but not equal to, the native metric: BF uses a strict comparison.
    g_magnitude_limit = std::nextafter(4.0, 0.0);
}

TEST_P(TestShipPreciseManr, replaysNativeCoordinates)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipPreciseManr,
    ValuesIn(ship_reference_inputs({2, 3, 4, 5}, SHIP_ORIGINAL_AND_REPLACED)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

const std::vector<BurningShipReference> SHIP_REFERENCES{
    {"Original", 0.0, false},
    {"Displaced", 0.125, false},
    {"DisplacedReplacement", 0.125, true},
};

class TestShipFmodInside : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipFmodInside::SetUp()
{
    BurningShipReferenceTest::SetUp();
    g_bailout_test = Bailout::MANR;
    set_bailout_formula(g_bailout_test);
    g_magnitude_limit = 8.0;
    g_inside_method = ColorMethod::FMODI;
    g_close_proximity = 10.0;
}

TEST_P(TestShipFmodInside, usesNativeManrMetric)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipFmodInside,
    ValuesIn(ship_reference_inputs({2, 3}, SHIP_ORIGINAL_AND_REPLACED)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

struct ShipInitializerInput
{
    std::string name;
    bool big_float;
    int degree;
    InitOrbitMode mode;
    double offset;
    bool replace;
};

void PrintTo(const ShipInitializerInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ShipInitializerInput> ship_initializer_inputs()
{
    std::vector<ShipInitializerInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int degree : {2, 3, 4, 5})
        {
            for (const InitOrbitMode mode : {InitOrbitMode::NORMAL, InitOrbitMode::PIXEL})
            {
                for (const BurningShipReference &reference : SHIP_REFERENCES)
                {
                    inputs.push_back(
                        {std::string{big_float ? "HighPrecision_" : "Double_"} + "Degree" + std::to_string(degree) +
                                (mode == InitOrbitMode::NORMAL ? "_Normal_" : "_Pixel_") + reference.name,
                            big_float, degree, mode, reference.offset, reference.replace});
                }
            }
        }
    }
    return inputs;
}

class TestShipImaginaryOffset : public BurningShipPerturbationTest, public WithParamInterface<ShipInitializerInput>
{
public:
    TestShipImaginaryOffset();

protected:
    void SetUp() override;
};

TestShipImaginaryOffset::TestShipImaginaryOffset() :
    BurningShipPerturbationTest{GetParam().big_float}
{
}

void TestShipImaginaryOffset::SetUp()
{
    BurningShipPerturbationTest::SetUp();
    set_initial_offset({0.0, -1.0});
    g_magnitude_limit = 8.0;
    g_params[2] = GetParam().degree;
    g_use_init_orbit = GetParam().mode;
}

TEST_P(TestShipImaginaryOffset, seedsNativeOrbitSequence)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipImaginaryOffset, ValuesIn(ship_initializer_inputs()),
    [](const TestParamInfo<ShipInitializerInput> &info) { return info.param.name; });

class TestShipExplicitInitializer : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipExplicitInitializer::SetUp()
{
    BurningShipReferenceTest::SetUp();
    set_initial_offset({0.03125, -0.0625});
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {0.25, -0.125};
}

TEST_P(TestShipExplicitInitializer, followsSelectedNativeInitializer)
{
    compare_native_pixel({0.125, 0.0625}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipExplicitInitializer,
    ValuesIn(ship_reference_inputs({2, 3, 4, 5}, SHIP_REFERENCES)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

struct ShipBofInput
{
    std::string name;
    bool big_float;
    int degree;
    bool match_book;
    ColorMethod method;
    double offset;
    bool replace;
};

void PrintTo(const ShipBofInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ShipBofInput> ship_bof_inputs()
{
    const PerturbationColorMethod methods[]{
        {"Bof60", ColorMethod::BOF60},
        {"Bof61", ColorMethod::BOF61},
    };
    std::vector<ShipBofInput> inputs;
    for (const ShipReferenceInput &reference : ship_reference_inputs({2, 3, 4, 5}, SHIP_REFERENCES))
    {
        for (const bool match_book : {false, true})
        {
            for (const PerturbationColorMethod &method : methods)
            {
                inputs.push_back(
                    {reference.name + (match_book ? "_Book_" : "_Current_") + method.name, reference.big_float,
                        reference.degree, match_book, method.method, reference.offset, reference.replace});
            }
        }
    }
    return inputs;
}

class TestShipBofSeed : public BurningShipPerturbationTest, public WithParamInterface<ShipBofInput>
{
public:
    TestShipBofSeed();

protected:
    void SetUp() override;
};

TestShipBofSeed::TestShipBofSeed() :
    BurningShipPerturbationTest{GetParam().big_float}
{
}

void TestShipBofSeed::SetUp()
{
    BurningShipPerturbationTest::SetUp();
    set_initial_offset({0.03125, -0.0625});
    g_magnitude_calc = true;
    g_params[2] = GetParam().degree;
    g_bof_match_book_images = GetParam().match_book;
    g_inside_method = GetParam().method;
}

TEST_P(TestShipBofSeed, followsSelectedNativeInitializer)
{
    compare_native_pixel({0.125, 0.0625}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipBofSeed, ValuesIn(ship_bof_inputs()),
    [](const TestParamInfo<ShipBofInput> &info) { return info.param.name; });

class TestShipPreciseOffset : public BurningShipReferenceTest
{
protected:
    void SetUp() override;
};

void TestShipPreciseOffset::SetUp()
{
    BurningShipReferenceTest::SetUp();
    set_initial_offset({0.0, -1.0});
    g_magnitude_limit = std::nextafter(5.0, 0.0);
}

TEST_P(TestShipPreciseOffset, retainsNativeOffsetSeedDuringReplay)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipPreciseOffset, ValuesIn(ship_reference_inputs({2, 3, 4, 5}, SHIP_REFERENCES)),
    [](const TestParamInfo<ShipReferenceInput> &info) { return info.param.name; });

struct ShipBailoutEqualityInput
{
    std::string name;
    bool big_float;
    Bailout test;
    double limit;
    double offset;
    bool replace;
};

void PrintTo(const ShipBailoutEqualityInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<ShipBailoutEqualityInput> ship_bailout_equality_inputs()
{
    const std::array<double, 7> metrics{2.0, 1.0, 1.0, 1.0, 1.0, 4.0, 4.0};

    struct Boundary
    {
        const char *name;
        double limit;
    };

    std::vector<ShipBailoutEqualityInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const PerturbationBailoutMethod &method : TERMINATION_BAILOUTS)
        {
            const double metric{metrics[static_cast<std::size_t>(method.test)]};
            const Boundary boundaries[]{
                {"Below", std::nextafter(metric, 0.0)},
                {"At", metric},
                {"Above", std::nextafter(metric, std::numeric_limits<double>::infinity())},
            };
            for (const Boundary &boundary : boundaries)
            {
                for (const BurningShipReference &reference : SHIP_ORIGINAL_AND_REPLACED)
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} + method.name + "_" +
                            boundary.name + "_" + reference.name,
                        big_float, method.test, boundary.limit, reference.offset, reference.replace});
                }
            }
        }
    }
    return inputs;
}

class TestShipBailoutEquality : public BurningShipPerturbationTest, public WithParamInterface<ShipBailoutEqualityInput>
{
public:
    TestShipBailoutEquality();

protected:
    void SetUp() override;
};

TestShipBailoutEquality::TestShipBailoutEquality() :
    BurningShipPerturbationTest{GetParam().big_float}
{
}

void TestShipBailoutEquality::SetUp()
{
    BurningShipPerturbationTest::SetUp();
    g_params[2] = 2;
    g_bailout_test = GetParam().test;
    set_bailout_formula(GetParam().test);
    g_magnitude_limit = GetParam().limit;
}

TEST_P(TestShipBailoutEquality, matchesNativePixelColorAtBailoutBoundary)
{
    compare_native_pixel({1.0, 1.0}, GetParam().offset, GetParam().replace);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipBailoutEquality, ValuesIn(ship_bailout_equality_inputs()),
    [](const TestParamInfo<ShipBailoutEqualityInput> &info) { return info.param.name; });

struct ShipPassInput
{
    const char *name;
    bool big_float;
    CalcMode pass;
};

void PrintTo(const ShipPassInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestShipPassBailout : public BurningShipPerturbationTest, public WithParamInterface<ShipPassInput>
{
public:
    TestShipPassBailout();

protected:
    void SetUp() override;

private:
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<SymmetryType> m_saved_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2, 0};
};

TestShipPassBailout::TestShipPassBailout() :
    BurningShipPerturbationTest{GetParam().big_float}
{
}

void TestShipPassBailout::SetUp()
{
    BurningShipPerturbationTest::SetUp();
    g_fractal_type = FractalType::BURNING_SHIP;
    g_params[2] = 2;
    g_image_region = ImageRegion{{1.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}};
    g_user.perturbation = PerturbationMode::YES;
    m_specific = *get_fractal_specific(g_fractal_type);
    m_specific.symmetry = SymmetryType::NONE;
    if (GetParam().big_float)
    {
        init_bf_dec(60);
        m_big_float = true;
        float_to_bf(g_bf_x_min, 1.0);
        float_to_bf(g_bf_x_max, 1.0);
        float_to_bf(g_bf_x_3rd, 1.0);
        clear_bf(g_bf_y_min);
        clear_bf(g_bf_y_max);
        clear_bf(g_bf_y_3rd);
        m_specific.per_image = mandel_per_image_bf;
        m_specific.per_pixel = mandel_per_pixel_bf;
        m_specific.orbit_calc = burning_ship_bf_fractal;
    }
    g_dispatch = FractalDispatch{m_specific};
    fill_pixel_grid();
    g_std_calc_mode = GetParam().pass;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    m_pixels.fill(0);
    m_plot_statuses.clear();
}

TEST_P(TestShipPassBailout, retainsPerturbationAtBailoutEquality)
{
    StandardFractal calculation;
    ASSERT_TRUE(calculation.resume().empty());
    for (int step = 0; step < 1000 && !calculation.done(); ++step)
    {
        calculation.iterate();
    }

    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(GetParam().pass, g_std_calc_mode);
    EXPECT_THAT(m_pixels, Each(GetParam().big_float ? 2 : 1));
    ASSERT_FALSE(m_plot_statuses.empty());
    for (const StandardPassStatus &status : m_plot_statuses)
    {
        EXPECT_TRUE(status.perturbation_active);
        EXPECT_GT(status.perturbation_reference_count, 0);
    }
    calculation.finish();
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestShipPassBailout,
    Values(ShipPassInput{"Double_BoundaryTrace", false, CalcMode::BOUNDARY_TRACE},
        ShipPassInput{"Double_SolidGuess", false, CalcMode::SOLID_GUESS},
        ShipPassInput{"HighPrecision_BoundaryTrace", true, CalcMode::BOUNDARY_TRACE},
        ShipPassInput{"HighPrecision_SolidGuess", true, CalcMode::SOLID_GUESS}),
    [](const TestParamInfo<ShipPassInput> &info) { return info.param.name; });

class PerturbationContinuationTest : public PerturbationIterationTest
{
public:
    explicit PerturbationContinuationTest(bool use_big_float);

protected:
    void prepare(long iterations = 24);
    void check_orbit(const std::complex<double> &parameter, std::complex<double> value, long iteration);
};

PerturbationContinuationTest::PerturbationContinuationTest(const bool use_big_float) :
    PerturbationIterationTest{use_big_float}
{
}

void PerturbationContinuationTest::prepare(const long iterations)
{
    g_max_iterations = iterations;
    m_specific.pert_pt = record_perturbation_orbit;
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    s_perturbation_orbit.clear();
    s_reference_coordinates.clear();
    s_reference_values.clear();
    initialize_reference({-0.5, 0.25});
}

void PerturbationContinuationTest::check_orbit(
    const std::complex<double> &parameter, std::complex<double> value, long iteration)
{
    m_engine.begin_orbit(parameter, value, iteration);
    EXPECT_EQ(value, m_engine.orbit_value());
    EXPECT_EQ(iteration, m_engine.orbit_iteration());
    bool done{};
    for (int call = 0; call < 20000 && !done; ++call)
    {
        done = m_engine.iterate_orbit();
        if (m_engine.orbit_iteration() != iteration)
        {
            EXPECT_EQ(iteration + 1, m_engine.orbit_iteration());
            value = value * value + parameter;
            ++iteration;
        }
        EXPECT_NEAR(value.real(), m_engine.orbit_value().real(), 1e-13);
        EXPECT_NEAR(value.imag(), m_engine.orbit_value().imag(), 1e-13);
        EXPECT_EQ(iteration == g_max_iterations || std::norm(value) >= g_magnitude_limit, done);
    }
    EXPECT_TRUE(done);
    EXPECT_TRUE(m_plots.empty());
    EXPECT_TRUE(m_engine.iterate_orbit());
    EXPECT_EQ(iteration, m_engine.orbit_iteration());
    EXPECT_NEAR(value.real(), m_engine.orbit_value().real(), 1e-13);
    EXPECT_NEAR(value.imag(), m_engine.orbit_value().imag(), 1e-13);
}

class TestPerturbationContinuation : public PerturbationContinuationTest, public WithParamInterface<bool>
{
public:
    TestPerturbationContinuation();
};

TestPerturbationContinuation::TestPerturbationContinuation() :
    PerturbationContinuationTest{GetParam()}
{
}

TEST_P(TestPerturbationContinuation, suppliedStateOverridesPixelSeedsAndColoringOptions)
{
    prepare();
    set_initial_offset({0.5, -0.75});
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {0.75, 0.5};
    g_inside_method = ColorMethod::STAR_TRAIL;
    g_show_orbit = true;
    g_distance_estimator = 1;
    g_periodicity_check = 1;
    g_real_color_iter = -19;

    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);

    EXPECT_EQ(-19, g_real_color_iter);
    EXPECT_EQ(17, s_perturbation_orbit.size());
    ASSERT_FALSE(s_reference_values.empty());
    EXPECT_EQ((std::complex<double>{0.03125, -0.0625}), s_reference_values.front());
    EXPECT_THAT(s_reference_coordinates, Each(Eq(std::complex<double>{0.125, 0.0625})));
}

TEST_P(TestPerturbationContinuation, parameterAndOrbitDisplacementsAreIndependent)
{
    prepare();
    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
    ASSERT_EQ(1, m_engine.reference_count());
    const std::size_t before{s_perturbation_orbit.size()};

    check_orbit({0.0625, 0.03125}, {-0.0625, 0.125}, 7);

    EXPECT_EQ(1, m_engine.reference_count());
    ASSERT_GT(s_perturbation_orbit.size(), before);
    EXPECT_EQ((std::complex<double>{-0.0625, 0.125}), s_perturbation_orbit[before]);
    check_orbit({0.0625, 0.03125}, {0.0625, -0.03125}, 10);
    EXPECT_EQ(1, m_engine.reference_count());
}

TEST_P(TestPerturbationContinuation, earlierContinuationReplacesReferenceWithoutDefaultInitialization)
{
    prepare();
    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);

    check_orbit({0.0625, 0.03125}, {-0.0625, 0.125}, 3);

    EXPECT_EQ(2, m_engine.reference_count());
}

TEST_P(TestPerturbationContinuation, referenceReplacementPreservesAcceptedValuesAndIterationPosition)
{
    prepare();
    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
    s_perturbation_orbit.clear();
    s_reference_values.clear();
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        record_perturbation_orbit(reference, delta, parameter);
        if (s_perturbation_orbit.size() == 4)
        {
            delta = {std::numeric_limits<double>::quiet_NaN(), 0.0};
        }
    };
    const std::complex<double> parameter{0.0625, 0.03125};
    std::complex<double> replacement{-0.0625, 0.125};
    for (int i = 0; i < 3; ++i)
    {
        replacement = replacement * replacement + parameter;
    }

    check_orbit(parameter, {-0.0625, 0.125}, 7);

    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_EQ(18, s_perturbation_orbit.size());
    ASSERT_FALSE(s_reference_values.empty());
    EXPECT_NEAR(replacement.real(), s_reference_values.front().real(), 1e-14);
    EXPECT_NEAR(replacement.imag(), s_reference_values.front().imag(), 1e-14);
}

TEST_P(TestPerturbationContinuation, escapingContinuationRetainsTheActualTerminalState)
{
    prepare();

    check_orbit({1.0, 0.0}, {0.5, 0.0}, 12);

    EXPECT_EQ(14, m_engine.orbit_iteration());
    EXPECT_EQ((std::complex<double>{2.5625, 0.0}), m_engine.orbit_value());
}

TEST_P(TestPerturbationContinuation, iterationLimitDoesNotPublishAdditionalContinuationSamples)
{
    prepare();
    const std::size_t before{s_perturbation_orbit.size()};

    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 24);

    EXPECT_EQ(before, s_perturbation_orbit.size());
}

class TestChunkedContinuation : public TestPerturbationContinuation
{
protected:
    void SetUp() override;
};

void TestChunkedContinuation::SetUp()
{
    TestPerturbationContinuation::SetUp();
    prepare(5000);
    m_engine.begin_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
}

TEST_P(TestChunkedContinuation, doesNotPublishSpeculativeContinuationValues)
{
    EXPECT_FALSE(m_engine.iterate_orbit());

    EXPECT_EQ(0, m_engine.reference_count());
    EXPECT_EQ(7, m_engine.orbit_iteration());
    EXPECT_EQ((std::complex<double>{0.03125, -0.0625}), m_engine.orbit_value());
    EXPECT_TRUE(s_perturbation_orbit.empty());
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestChunkedContinuation, Bool());

class TestPendingChunkedContinuation : public TestChunkedContinuation
{
protected:
    void SetUp() override;
};

void TestPendingChunkedContinuation::SetUp()
{
    TestChunkedContinuation::SetUp();
    ASSERT_FALSE(m_engine.iterate_orbit());
}

TEST_P(TestPendingChunkedContinuation, rejectsBeginningAnotherOrbit)
{
    EXPECT_THROW(m_engine.begin_orbit({}, {}, 0), std::logic_error);
}

TEST_P(TestPendingChunkedContinuation, rejectsPixelCalculation)
{
    EXPECT_THROW(m_engine.calculate_pixel(0, 0), std::logic_error);
}

TEST_P(TestPendingChunkedContinuation, rejectsPixelStrategyInitialization)
{
    EXPECT_THROW(m_engine.initialize_pixel_strategy(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPendingChunkedContinuation, Bool());

TEST_P(TestPerturbationContinuation, preciseReplayUsesTheContinuationAnchorAndRetainsAcceptedProgress)
{
    if (!GetParam())
    {
        return;
    }
    prepare(5000);
    set_initial_offset({0.5, 0.75});
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        record_perturbation_orbit(reference, delta, parameter);
        if (s_perturbation_orbit.size() == 4097)
        {
            delta = std::complex<double>{2.0, 0.0} - reference;
        }
    };

    check_orbit({}, {}, 17);

    EXPECT_EQ(4983, s_perturbation_orbit.size());
    EXPECT_EQ(1, m_engine.reference_count());
}

class TestCompletedContinuation : public TestPerturbationContinuation
{
protected:
    void SetUp() override;
};

void TestCompletedContinuation::SetUp()
{
    TestPerturbationContinuation::SetUp();
    prepare();
    check_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
}

TEST_P(TestCompletedContinuation, canReturnToPixelCalculation)
{
    EXPECT_GE(m_engine.calculate_pixel(g_col, g_row), 0);

    EXPECT_EQ(1, m_plots.size());
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestCompletedContinuation, Bool());

class TestPixelAfterContinuation : public TestCompletedContinuation
{
protected:
    void SetUp() override;
};

void TestPixelAfterContinuation::SetUp()
{
    TestCompletedContinuation::SetUp();
    ASSERT_GE(m_engine.calculate_pixel(g_col, g_row), 0);
}

TEST_P(TestPixelAfterContinuation, rejectsOrbitIteration)
{
    EXPECT_THROW(m_engine.iterate_orbit(), std::logic_error);
}

TEST_P(TestPixelAfterContinuation, rejectsOrbitValue)
{
    EXPECT_THROW(m_engine.orbit_value(), std::logic_error);
}

TEST_P(TestPixelAfterContinuation, rejectsOrbitIterationCount)
{
    EXPECT_THROW(m_engine.orbit_iteration(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPixelAfterContinuation, Bool());

struct InvalidContinuationStartInput
{
    const char *name;
    bool big_float;
    long iteration;
};

void PrintTo(const InvalidContinuationStartInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInvalidContinuationStart : public PerturbationContinuationTest,
                                     public WithParamInterface<InvalidContinuationStartInput>
{
public:
    TestInvalidContinuationStart();

protected:
    void SetUp() override;
};

TestInvalidContinuationStart::TestInvalidContinuationStart() :
    PerturbationContinuationTest{GetParam().big_float}
{
}

void TestInvalidContinuationStart::SetUp()
{
    PerturbationContinuationTest::SetUp();
    prepare();
}

TEST_P(TestInvalidContinuationStart, rejectsInvalidStartingIteration)
{
    EXPECT_THROW(m_engine.begin_orbit({}, {}, GetParam().iteration), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestInvalidContinuationStart,
    Values(InvalidContinuationStartInput{"Double_NegativeIteration", false, -1},
        InvalidContinuationStartInput{"Double_BeyondIterationLimit", false, 25},
        InvalidContinuationStartInput{"HighPrecision_NegativeIteration", true, -1},
        InvalidContinuationStartInput{"HighPrecision_BeyondIterationLimit", true, 25}),
    [](const TestParamInfo<InvalidContinuationStartInput> &info) { return info.param.name; });

TEST_P(TestPerturbationContinuation, rejectsOrbitIterationWithoutBeginning)
{
    prepare();

    EXPECT_THROW(m_engine.iterate_orbit(), std::logic_error);
}

class TestActiveContinuation : public TestPerturbationContinuation
{
protected:
    void SetUp() override;
};

void TestActiveContinuation::SetUp()
{
    TestPerturbationContinuation::SetUp();
    prepare();
    m_engine.begin_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
    ASSERT_FALSE(m_engine.iterate_orbit());
}

TEST_P(TestActiveContinuation, rejectsBeginningAnotherOrbitWithoutReplacingState)
{
    EXPECT_THROW(m_engine.begin_orbit({}, {}, 0), std::logic_error);

    EXPECT_EQ(8, m_engine.orbit_iteration());
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestActiveContinuation, Bool());

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPerturbationContinuation, Bool());

class PerturbationIntegerPowerTest : public PerturbationContinuationTest
{
public:
    explicit PerturbationIntegerPowerTest(bool use_big_float);

protected:
    void SetUp() override;
    void prepare_orbit_reference(
        const std::complex<double> &parameter, const std::complex<double> &seed, int max_calls);
    DComplex m_native_parameter{};

private:
    ValueSaver<int> m_saved_exponent{g_c_exponent};
    ValueSaver<double> m_saved_power{g_params[2]};
    ValueSaver<double> m_saved_power_imaginary{g_params[3], 0.0};
    ValueSaver<DComplex *> m_saved_native_parameter{g_float_param};
};

PerturbationIntegerPowerTest::PerturbationIntegerPowerTest(const bool use_big_float) :
    PerturbationContinuationTest{use_big_float}
{
}

void PerturbationIntegerPowerTest::SetUp()
{
    PerturbationContinuationTest::SetUp();
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
}

void PerturbationIntegerPowerTest::prepare_orbit_reference(
    const std::complex<double> &parameter, const std::complex<double> &seed, const int max_calls)
{
    // Retain the shared first-input reference used by the original sweep.
    m_engine.begin_orbit(parameter, seed, 0);
    for (int call = 0; call < max_calls && m_engine.orbit_iteration() == 0; ++call)
    {
        m_engine.iterate_orbit();
    }
    ASSERT_EQ(1, m_engine.orbit_iteration());
    m_engine.finish_orbit();
}

struct NamedOrbitValue
{
    const char *name;
    std::complex<double> value;
};

struct IntegerOrbitInput
{
    std::string name;
    bool big_float;
    int exponent;
    std::complex<double> parameter;
    std::complex<double> seed;
};

void PrintTo(const IntegerOrbitInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<IntegerOrbitInput> integer_orbit_inputs(const std::vector<int> &exponents,
    const std::vector<NamedOrbitValue> &parameters, const std::vector<NamedOrbitValue> &seeds)
{
    std::vector<IntegerOrbitInput> inputs;
    for (const bool big_float : {false, true})
    {
        for (const int exponent : exponents)
        {
            for (const NamedOrbitValue &parameter : parameters)
            {
                for (const NamedOrbitValue &seed : seeds)
                {
                    inputs.push_back({std::string{big_float ? "HighPrecision_" : "Double_"} +
                            (exponent < 0 ? "NegativePower" : "Power") + std::to_string(std::abs(exponent)) + "_" +
                            parameter.name + "_" + seed.name,
                        big_float, exponent, parameter.value, seed.value});
                }
            }
        }
    }
    return inputs;
}

class TestLargeIntegerOrbit : public PerturbationIntegerPowerTest, public WithParamInterface<IntegerOrbitInput>
{
public:
    TestLargeIntegerOrbit();

protected:
    void SetUp() override;
};

TestLargeIntegerOrbit::TestLargeIntegerOrbit() :
    PerturbationIntegerPowerTest{GetParam().big_float}
{
}

void TestLargeIntegerOrbit::SetUp()
{
    PerturbationIntegerPowerTest::SetUp();
    g_max_iterations = 24;
    g_c_exponent = GetParam().exponent;
    g_params[2] = GetParam().exponent;
    initialize_reference({-0.125, 0.000125});
    m_native_parameter = {-0.125, 0.000125};
    g_float_param = &m_native_parameter;
    ASSERT_NO_FATAL_FAILURE(prepare_orbit_reference({-0.125, 0.000125}, {0.75, -0.000125}, 200));
    m_native_parameter = {GetParam().parameter.real(), GetParam().parameter.imag()};
}

TEST_P(TestLargeIntegerOrbit, matchesStandardOrbitSequence)
{
    std::complex<double> expected{GetParam().seed};
    m_engine.begin_orbit(GetParam().parameter, expected, 0);
    long iteration{};
    bool done{};
    for (int call = 0; call < 200 && !done; ++call)
    {
        done = m_engine.iterate_orbit();
        if (m_engine.orbit_iteration() != iteration)
        {
            ASSERT_EQ(iteration + 1, m_engine.orbit_iteration());
            g_old_z = {expected.real(), expected.imag()};
            const bool escaped{mandel_z_power_orbit() != 0};
            expected = {g_new_z.x, g_new_z.y};
            ++iteration;
            EXPECT_EQ(escaped || iteration == g_max_iterations, done);
        }
        const double tolerance{1e-12 * std::max(1.0, std::abs(expected))};
        ASSERT_NEAR(expected.real(), m_engine.orbit_value().real(), tolerance);
        ASSERT_NEAR(expected.imag(), m_engine.orbit_value().imag(), tolerance);
    }
    ASSERT_TRUE(done);
    EXPECT_EQ(GetParam().exponent, g_c_exponent);
    EXPECT_TRUE(m_plots.empty());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestLargeIntegerOrbit,
    ValuesIn(integer_orbit_inputs({28, 29, 32, 64, 128, 2048},
        {{"ReferenceParameter", {-0.125, 0.000125}}, {"DisplacedParameter", {-0.125125, -0.000125}}},
        {{"Seed0p75", {0.75, -0.000125}}, {"Seed0p999", {0.999, -0.000125}}, {"Seed1p125", {1.125, -0.000125}}})),
    [](const TestParamInfo<IntegerOrbitInput> &info) { return info.param.name; });

class TestSignedIntegerOrbit : public PerturbationIntegerPowerTest, public WithParamInterface<IntegerOrbitInput>
{
public:
    TestSignedIntegerOrbit();

protected:
    void SetUp() override;
};

TestSignedIntegerOrbit::TestSignedIntegerOrbit() :
    PerturbationIntegerPowerTest{GetParam().big_float}
{
}

void TestSignedIntegerOrbit::SetUp()
{
    PerturbationIntegerPowerTest::SetUp();
    g_max_iterations = 16;
    g_c_exponent = GetParam().exponent;
    g_params[2] = GetParam().exponent;
    initialize_reference({0.125, 0.0625});
    if (GetParam().big_float)
    {
        init_big_pi();
    }
    m_native_parameter = {};
    g_float_param = &m_native_parameter;
    if (GetParam().big_float)
    {
        clear_bf(g_param_z_bf.x);
        clear_bf(g_param_z_bf.y);
        clear_bf(g_old_z_bf.x);
        clear_bf(g_old_z_bf.y);
    }
    ASSERT_NO_FATAL_FAILURE(prepare_orbit_reference({}, {}, 2000));
    m_native_parameter = {GetParam().parameter.real(), GetParam().parameter.imag()};
    if (GetParam().big_float)
    {
        float_to_bf(g_param_z_bf.x, GetParam().parameter.real());
        float_to_bf(g_param_z_bf.y, GetParam().parameter.imag());
        float_to_bf(g_old_z_bf.x, GetParam().seed.real());
        float_to_bf(g_old_z_bf.y, GetParam().seed.imag());
    }
}

TEST_P(TestSignedIntegerOrbit, matchesNativeOrbitAndBailout)
{
    std::complex<double> expected{GetParam().seed};
    m_engine.begin_orbit(GetParam().parameter, GetParam().seed, 0);
    long iteration{};
    bool done{};
    for (int call = 0; call < 2000 && !done; ++call)
    {
        done = m_engine.iterate_orbit();
        if (m_engine.orbit_iteration() != iteration)
        {
            ASSERT_EQ(iteration + 1, m_engine.orbit_iteration());
            bool escaped;
            if (GetParam().big_float)
            {
                escaped = julia_z_power_orbit_bf() != 0;
                expected = {
                    static_cast<double>(bf_to_float(g_new_z_bf.x)), static_cast<double>(bf_to_float(g_new_z_bf.y))};
            }
            else
            {
                g_old_z = {expected.real(), expected.imag()};
                escaped = mandel_z_power_orbit() != 0;
                expected = {g_new_z.x, g_new_z.y};
            }
            ++iteration;
            EXPECT_EQ(escaped, m_engine.orbit_escaped());
            EXPECT_EQ(escaped || iteration == g_max_iterations, done);
        }
        const double tolerance{1e-11 * std::max(1.0, std::abs(expected))};
        ASSERT_NEAR(expected.real(), m_engine.orbit_value().real(), tolerance);
        ASSERT_NEAR(expected.imag(), m_engine.orbit_value().imag(), tolerance);
    }
    ASSERT_TRUE(done);
    EXPECT_TRUE(m_plots.empty());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSignedIntegerOrbit,
    ValuesIn(
        integer_orbit_inputs({0, 1, -1, -2, -3, -29}, {{"ZeroParameter", {}}, {"ComplexParameter", {0.125, 0.0625}}},
            {{"ZeroSeed", {}}, {"Seed0p75", {0.75, 0.125}}, {"Seed1p25", {1.25, -0.25}}})),
    [](const TestParamInfo<IntegerOrbitInput> &info) { return info.param.name; });

struct PrecisePowerInput
{
    const char *name;
    int exponent;
};

void PrintTo(const PrecisePowerInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSignedSubDoublePower : public PerturbationIntegerPowerTest, public WithParamInterface<PrecisePowerInput>
{
public:
    TestSignedSubDoublePower();

protected:
    void SetUp() override;
};

TestSignedSubDoublePower::TestSignedSubDoublePower() :
    PerturbationIntegerPowerTest{true}
{
}

void TestSignedSubDoublePower::SetUp()
{
    PerturbationIntegerPowerTest::SetUp();
    g_max_iterations = 4;
    g_c_exponent = GetParam().exponent;
    g_params[2] = GetParam().exponent;
    initialize_reference({0.125, 0.0});
    init_big_pi();
    while (!m_engine.initialize_pixel_strategy())
    {
    }
}

TEST_P(TestSignedSubDoublePower, resolvesNearZeroInputAfterReferenceReplacement)
{
    BigStackSaver saved;
    const BFComplex parameter{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    float_to_bf(parameter.x, 0.125);
    clear_bf(parameter.y);
    int_to_bf(value.x, 1);
    clear_bf(value.y);
    m_engine.begin_orbit_bf(parameter, value, 0);
    for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
    {
        m_engine.iterate_orbit();
    }
    ASSERT_EQ(1, m_engine.orbit_iteration());
    m_engine.finish_orbit();
    const int references{m_engine.reference_count()};
    str_to_bf(value.x, "1e-40");
    copy_bf(g_param_z_bf.x, parameter.x);
    copy_bf(g_param_z_bf.y, parameter.y);
    copy_bf(g_old_z_bf.x, value.x);
    copy_bf(g_old_z_bf.y, value.y);
    const bool escaped{julia_z_power_orbit_bf() != 0};
    const double expected{static_cast<double>(bf_to_float(g_new_z_bf.x))};

    m_engine.begin_orbit_bf(parameter, value, 0);
    for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
    {
        m_engine.iterate_orbit();
    }

    ASSERT_EQ(1, m_engine.orbit_iteration());
    EXPECT_EQ(escaped, m_engine.orbit_escaped());
    EXPECT_NEAR(expected, m_engine.orbit_value().real(), 1e-12 * std::abs(expected));
    EXPECT_DOUBLE_EQ(0.0, m_engine.orbit_value().imag());
    EXPECT_EQ(references + 1, m_engine.reference_count());
    m_engine.finish_orbit();
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestSignedSubDoublePower,
    Values(PrecisePowerInput{"Zero", 0}, PrecisePowerInput{"NegativeOne", -1}, PrecisePowerInput{"NegativeTwo", -2}),
    [](const TestParamInfo<PrecisePowerInput> &info) { return info.param.name; });

TEST_F(TestPerturbationPixels, zeroPowerDistinguishesExactZeroFromAnUnderflowedReference)
{
    ValueSaver saved_exponent{g_c_exponent, 0};
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
    g_max_iterations = 2;
    init_bf_dec(60);
    m_big_float = true;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    clear_bf(center.x);
    clear_bf(center.y);
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    BigStackSaver saved;
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    clear_bf(value.y);
    str_to_bf(value.x, "1e-400");
    m_engine.begin_orbit_bf(center, value, 0);
    for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
    {
        m_engine.iterate_orbit();
    }
    ASSERT_EQ(1, m_engine.orbit_iteration());
    EXPECT_DOUBLE_EQ(1.0, m_engine.orbit_value().real());
    m_engine.finish_orbit();
    clear_bf(value.x);
    m_engine.begin_orbit_bf(center, value, 0);
    for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
    {
        m_engine.iterate_orbit();
    }
    ASSERT_EQ(1, m_engine.orbit_iteration());
    EXPECT_DOUBLE_EQ(0.0, m_engine.orbit_value().real());
    EXPECT_DOUBLE_EQ(0.0, m_engine.orbit_value().imag());
    EXPECT_FALSE(m_engine.orbit_escaped());
    m_engine.finish_orbit();
}

TEST_P(TestPerturbationContinuation, standardFramePreservesIntegerPowers)
{
    ValueSaver saved_exponent{g_c_exponent};
    ValueSaver saved_power{g_params[2]};
    ValueSaver saved_power_imaginary{g_params[3], 0.0};
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver saved_resuming{g_resuming, false};
    g_user.perturbation = PerturbationMode::YES;
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.per_image = GetParam() ? mandel_per_image_bf : mandel_per_image;
    m_specific.pert_pt =
        [](const std::complex<double> &ref, std::complex<double> &delta, const std::complex<double> &parameter_delta)
    {
        s_perturbation_orbit.push_back(ref + delta);
        mandel_z_power_perturb(ref, delta, parameter_delta);
    };
    g_dispatch = FractalDispatch{m_specific};
    g_max_iterations = 4;
    g_image_region = ImageRegion{{0.875, 0.000125}, {0.875, 0.000125}, {0.875, 0.000125}};
    if (GetParam())
    {
        init_bf_dec(60);
        m_big_float = true;
        float_to_bf(g_bf_x_min, 0.875);
        float_to_bf(g_bf_x_max, 0.875);
        float_to_bf(g_bf_x_3rd, 0.875);
        float_to_bf(g_bf_y_min, 0.000125);
        float_to_bf(g_bf_y_max, 0.000125);
        float_to_bf(g_bf_y_3rd, 0.000125);
    }

    for (const int exponent : {0, 1, -1, -2, -3, -29, 28, 29, 64, 2048})
    {
        SCOPED_TRACE(exponent);
        g_c_exponent = exponent;
        g_params[2] = exponent;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        s_perturbation_orbit.clear();
        StandardFractal calculation;
        calculation.resume();
        for (int chunk = 0; chunk < 200 && !calculation.done(); ++chunk)
        {
            calculation.iterate();
            EXPECT_EQ(exponent, g_c_exponent);
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
        EXPECT_FALSE(s_perturbation_orbit.empty());
        calculation.finish();
    }
}

class TestComplexPerturbationContinuation : public TestPerturbationIterations
{
protected:
    static void record_point(
        const std::complex<double> &ref, std::complex<double> &delta, const std::complex<double> &parameter_delta)
    {
        s_perturbation_orbit.push_back(ref + delta);
        mandel_z_power_perturb(ref, delta, parameter_delta);
    }

    void prepare(const std::complex<double> &power)
    {
        m_engine.finish();
        if (m_big_float)
        {
            free_bf_vars();
            m_big_float = false;
        }
        g_fractal_type = FractalType::MANDEL_Z_POWER;
        g_params[2] = power.real();
        g_params[3] = power.imag();
        g_param_z2 = {power.real(), power.imag()};
        g_c_exponent = static_cast<int>(power.real());
        g_max_iterations = 8;
        m_specific = *get_fractal_specific(g_fractal_type);
        m_specific.pert_pt = record_point;
        s_perturbation_orbit.clear();
        initialize_reference({0.0625, 0.03125});
    }

    void check_orbit(const std::complex<double> &parameter, const std::complex<double> &seed)
    {
        SCOPED_TRACE(parameter);
        SCOPED_TRACE(seed);
        DComplex native_parameter{parameter.real(), parameter.imag()};
        ValueSaver saved_parameter{g_float_param, &native_parameter};
        if (GetParam())
        {
            float_to_bf(g_param_z_bf.x, parameter.real());
            float_to_bf(g_param_z_bf.y, parameter.imag());
            float_to_bf(g_old_z_bf.x, seed.real());
            float_to_bf(g_old_z_bf.y, seed.imag());
        }
        std::complex<double> expected{seed};
        m_engine.begin_orbit(parameter, seed, 0);
        long iteration{};
        bool done{};
        for (int call = 0; call < 2000 && !done; ++call)
        {
            done = m_engine.iterate_orbit();
            if (m_engine.orbit_iteration() != iteration)
            {
                ASSERT_EQ(iteration + 1, m_engine.orbit_iteration());
                bool escaped;
                if (GetParam())
                {
                    escaped = julia_z_power_orbit_bf() != 0;
                    expected = {
                        static_cast<double>(bf_to_float(g_new_z_bf.x)), static_cast<double>(bf_to_float(g_new_z_bf.y))};
                }
                else
                {
                    g_old_z = {expected.real(), expected.imag()};
                    escaped = mandel_z_power_cmplx_orbit() != 0;
                    expected = {g_new_z.x, g_new_z.y};
                }
                ++iteration;
                EXPECT_EQ(escaped, m_engine.orbit_escaped());
                EXPECT_EQ(escaped || iteration == g_max_iterations, done);
            }
            const double tolerance{1e-11 * std::max(1.0, std::abs(expected))};
            ASSERT_NEAR(expected.real(), m_engine.orbit_value().real(), tolerance);
            ASSERT_NEAR(expected.imag(), m_engine.orbit_value().imag(), tolerance);
        }
        ASSERT_TRUE(done);
        EXPECT_TRUE(m_plots.empty());
    }

    ValueSaver<int> m_saved_exponent{g_c_exponent};
    ValueSaver<double> m_saved_real_power{g_params[2]};
    ValueSaver<double> m_saved_imag_power{g_params[3]};
    ValueSaver<DComplex> m_saved_complex_power{g_param_z2};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
};

TEST_P(TestComplexPerturbationContinuation, pointSequencesMatchTheNativeComplexPowerKernels)
{
    for (const std::complex<double> power :
        {std::complex<double>{2.5, 0.0}, {0.5, 0.0}, {-0.5, 0.0}, {2.5, 0.25}, {0.25, -0.5}, {0.0, 0.5}})
    {
        SCOPED_TRACE(power);
        prepare(power);
        g_max_iterations = 4;
        for (const std::complex<double> parameter : {std::complex<double>{}, {0.0625, 0.03125}})
        {
            for (const std::complex<double> seed :
                {std::complex<double>{}, {0.5, 0.125}, {-0.5, 0.000125}, {-0.5, -0.000125}})
            {
                check_orbit(parameter, seed);
            }
        }
        EXPECT_FALSE(s_perturbation_orbit.empty());
    }
}

TEST_P(TestComplexPerturbationContinuation, referenceReplacementPreservesAcceptedProgress)
{
    prepare({0.5, 0.25});
    check_orbit({}, {0.5, 0.25});
    s_perturbation_orbit.clear();
    m_specific.pert_pt =
        [](const std::complex<double> &ref, std::complex<double> &delta, const std::complex<double> &parameter_delta)
    {
        record_point(ref, delta, parameter_delta);
        if (s_perturbation_orbit.size() == 4)
        {
            delta = {std::numeric_limits<double>::quiet_NaN(), 0.0};
        }
    };
    check_orbit({}, {0.25, 0.125});
    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_EQ(g_max_iterations + 1, s_perturbation_orbit.size());
}

TEST_P(TestComplexPerturbationContinuation, standardFrameUsesBothExponentComponents)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver saved_resuming{g_resuming, false};
    g_user.perturbation = PerturbationMode::YES;
    for (const std::complex<double> power : {std::complex<double>{2.5, 0.0}, {0.5, 0.25}, {2.5, -0.25}, {0.0, 0.5}})
    {
        SCOPED_TRACE(power);
        prepare(power);
        m_specific.symmetry = SymmetryType::NONE;
        m_specific.per_image = GetParam() ? mandel_per_image_bf : mandel_per_image;
        g_dispatch = FractalDispatch{m_specific};
        g_image_region = ImageRegion{{0.125, 0.0625}, {0.125, 0.0625}, {0.125, 0.0625}};
        if (GetParam())
        {
            float_to_bf(g_bf_x_min, 0.125);
            float_to_bf(g_bf_x_max, 0.125);
            float_to_bf(g_bf_x_3rd, 0.125);
            float_to_bf(g_bf_y_min, 0.0625);
            float_to_bf(g_bf_y_max, 0.0625);
            float_to_bf(g_bf_y_3rd, 0.0625);
        }
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        StandardFractal calculation;
        calculation.resume();
        for (int chunk = 0; chunk < 200 && !calculation.done(); ++chunk)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_FALSE(s_perturbation_orbit.empty());
        EXPECT_DOUBLE_EQ(power.real(), g_params[2]);
        EXPECT_DOUBLE_EQ(power.imag(), g_params[3]);
        calculation.finish();
    }
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestComplexPerturbationContinuation, Bool());

TEST_F(TestPerturbationPixels, complexPowerRetainsSignedZeroBranchSides)
{
    ValueSaver saved_exponent{g_c_exponent, 0};
    ValueSaver saved_real{g_params[2], 0.5};
    ValueSaver saved_imag{g_params[3], 0.0};
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
    g_max_iterations = 2;
    m_engine.initialize_frame({}, {}, {});
    for (const int sign : {1, -1})
    {
        m_engine.begin_orbit({}, {-1.0, std::copysign(0.0, static_cast<double>(sign))}, 0);
        for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
        {
            m_engine.iterate_orbit();
        }
        ASSERT_EQ(1, m_engine.orbit_iteration());
        EXPECT_NEAR(0.0, m_engine.orbit_value().real(), 1e-12);
        EXPECT_NEAR(sign, m_engine.orbit_value().imag(), 1e-12);
        m_engine.finish_orbit();
    }
}

TEST_F(TestPerturbationPixels, complexPowerMatchesNativeBranchesBelowDoubleRange)
{
    ValueSaver saved_exponent{g_c_exponent, 0};
    ValueSaver saved_real{g_params[2], 0.5};
    ValueSaver saved_imag{g_params[3], 0.0};
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
    g_max_iterations = 2;
    init_bf_dec(60);
    m_big_float = true;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    clear_bf(center.x);
    clear_bf(center.y);
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    BigStackSaver saved;
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex native{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    BFComplex exponent{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    float_to_bf(exponent.x, 0.5);
    clear_bf(exponent.y);
    int_to_bf(value.x, -1);
    for (const int sign : {1, -1})
    {
        str_to_bf(value.y, sign > 0 ? "1e-400" : "-1e-400");
        EXPECT_DOUBLE_EQ(-1.0, static_cast<double>(bf_to_float(value.x)));
        EXPECT_EQ(sign, sign_bf(value.y));
        EXPECT_NEAR(std::acos(-1.0), static_cast<double>(bf_to_float(g_bf_pi)), 1e-12);
        BFComplex native_seed{value};
        cmplx_pow_bf(&native, &native_seed, &exponent);
        // Preserve the native kernel's behavior beyond its initial floating-
        // point approximation range, rather than substituting a new policy.
        const std::complex<double> expected{
            static_cast<double>(bf_to_float(native.x)), static_cast<double>(bf_to_float(native.y))};
        m_engine.begin_orbit_bf(center, value, 0);
        for (int call = 0; call < 20 && m_engine.orbit_iteration() == 0; ++call)
        {
            m_engine.iterate_orbit();
        }
        ASSERT_EQ(1, m_engine.orbit_iteration());
        EXPECT_NEAR(expected.real(), m_engine.orbit_value().real(), 1e-12);
        EXPECT_NEAR(expected.imag(), m_engine.orbit_value().imag(), 1e-12);
        m_engine.finish_orbit();
    }
}

struct ContinuationExitInput
{
    const char *name;
    bool big_float;
    std::complex<double> parameter;
    std::complex<double> value;
    long iteration;
    bool escaped;
};

void PrintTo(const ContinuationExitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestContinuationExitReason : public PerturbationContinuationTest, public WithParamInterface<ContinuationExitInput>
{
public:
    TestContinuationExitReason();

protected:
    void SetUp() override;
};

TestContinuationExitReason::TestContinuationExitReason() :
    PerturbationContinuationTest{GetParam().big_float}
{
}

void TestContinuationExitReason::SetUp()
{
    PerturbationContinuationTest::SetUp();
    prepare();
}

TEST_P(TestContinuationExitReason, distinguishesBailoutFromIterationLimit)
{
    check_orbit(GetParam().parameter, GetParam().value, GetParam().iteration);

    EXPECT_EQ(GetParam().escaped, m_engine.orbit_escaped());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestContinuationExitReason,
    Values(ContinuationExitInput{"Double_IterationLimit", false, {0.125, 0.0625}, {0.03125, -0.0625}, 7, false},
        ContinuationExitInput{"Double_Escaped", false, {1.0, 0.0}, {0.5, 0.0}, 12, true},
        ContinuationExitInput{"HighPrecision_IterationLimit", true, {0.125, 0.0625}, {0.03125, -0.0625}, 7, false},
        ContinuationExitInput{"HighPrecision_Escaped", true, {1.0, 0.0}, {0.5, 0.0}, 12, true}),
    [](const TestParamInfo<ContinuationExitInput> &info) { return info.param.name; });

TEST_P(TestPerturbationContinuation, unfinishedOrbitCanBeReleasedWithoutDiscardingTheReference)
{
    prepare();
    m_engine.begin_orbit({0.125, 0.0625}, {0.03125, -0.0625}, 7);
    EXPECT_FALSE(m_engine.iterate_orbit());

    m_engine.finish_orbit();

    EXPECT_EQ(1, m_engine.reference_count());
    EXPECT_THROW(m_engine.orbit_escaped(), std::logic_error);
    check_orbit({0.0625, 0.03125}, {-0.0625, 0.125}, 7);
    EXPECT_EQ(1, m_engine.reference_count());
    m_engine.finish_orbit();
    EXPECT_GE(m_engine.calculate_pixel(g_col, g_row), 0);
    EXPECT_EQ(1, m_plots.size());
}

TEST_P(TestPerturbationContinuation, preciseReferenceReplacementYieldsWithoutPublishingOrReplayingFailedWork)
{
    if (!GetParam())
    {
        return;
    }
    prepare(5000);
    while (!m_engine.initialize_pixel_strategy())
    {
    }
    BigStackSaver saved;
    const BFComplex parameter{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(parameter.x, 0.25);
    clear_bf(parameter.y);
    copy_bf(value.x, parameter.x);
    clear_bf(value.y);
    m_engine.begin_orbit_bf(parameter, value, 0);
    while (!m_engine.iterate_orbit())
    {
    }
    m_engine.finish_orbit();
    float_to_bf(parameter.x, 0.125);
    float_to_bf(parameter.y, 0.0625);
    float_to_bf(value.x, 0.03125);
    float_to_bf(value.y, -0.0625);
    float_to_bf(offset, 1e-40);
    add_a_bf(parameter.x, offset);
    add_a_bf(value.x, offset);
    s_perturbation_orbit.clear();
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &displacement)
    {
        record_perturbation_orbit(reference, delta, displacement);
        if (s_perturbation_orbit.size() == 4097)
        {
            delta = {std::numeric_limits<double>::quiet_NaN(), 0.0};
        }
    };
    m_engine.begin_orbit_bf(parameter, value, 0);
    std::complex<double> expected{0.03125, -0.0625};
    int replay_yields{};
    bool done{};
    for (int call = 0; call < 5010 && !done; ++call)
    {
        const long before{m_engine.orbit_iteration()};
        done = m_engine.iterate_orbit();
        if (m_engine.orbit_iteration() != before)
        {
            EXPECT_EQ(before + 1, m_engine.orbit_iteration());
            expected = expected * expected + std::complex<double>{0.125, 0.0625};
        }
        else if (before == 4096)
        {
            ++replay_yields;
        }
        EXPECT_NEAR(expected.real(), m_engine.orbit_value().real(), 1e-14);
        EXPECT_NEAR(expected.imag(), m_engine.orbit_value().imag(), 1e-14);
    }
    EXPECT_TRUE(done);
    EXPECT_EQ(5000, m_engine.orbit_iteration());
    EXPECT_EQ(5001, s_perturbation_orbit.size());
    EXPECT_GE(replay_yields, 2);
    EXPECT_TRUE(m_plots.empty());
    m_engine.finish_orbit();
}

TEST_F(TestPerturbationPixels, preciseContinuationPreservesSubDoubleParameterAndSeedDisplacements)
{
    init_bf_dec(60);
    m_big_float = true;
    g_max_iterations = 4;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    float_to_bf(center.x, 0.25);
    clear_bf(center.y);
    ASSERT_TRUE(m_engine.initialize_pixel_strategy());
    BigStackSaver saved;
    const BFComplex parameter{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(parameter.x, 0.25);
    clear_bf(parameter.y);
    copy_bf(value.x, parameter.x);
    clear_bf(value.y);
    m_engine.begin_orbit_bf(parameter, value, 0);
    while (!m_engine.iterate_orbit())
    {
    }
    m_engine.finish_orbit();
    float_to_bf(offset, 1e-40);
    const int references{m_engine.reference_count()};
    add_a_bf(parameter.x, offset);
    add_a_bf(value.x, offset);
    s_perturbation_orbit.clear();
    m_specific.pert_pt = record_coordinate;
    s_coordinates.clear();

    m_engine.begin_orbit_bf(parameter, value, 0);
    EXPECT_FALSE(m_engine.iterate_orbit());

    ASSERT_EQ(1, s_coordinates.size());
    EXPECT_NEAR(1e-40, s_coordinates.front().parameter_delta.real(), 1e-52);
    EXPECT_DOUBLE_EQ(0.0, s_coordinates.front().parameter_delta.imag());
    EXPECT_EQ(references, m_engine.reference_count());
    EXPECT_TRUE(m_plots.empty());
    m_engine.finish_orbit();
}

class TestSynchronousPerturbation : public TestPerturbationIterations
{
protected:
    void prepare(const int width = 8, const int height = 6,
        const ImageRegion &region = {{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}}, const long iterations = 128)
    {
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
        g_image_region = region;
        g_max_iterations = iterations;
        m_frame.assign(width * height, -1);
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this, width, height](const int col, const int row, const int color)
                {
                    if (col < width && row < height)
                    {
                        m_frame.at(row * width + col) = color;
                        m_plots.push_back({col, row});
                        if (const StandardFractal *calculation{active_standard_fractal()})
                        {
                            m_plot_statuses.push_back(calculation->standard_pass_status());
                        }
                    }
                }));
        initialize_reference({-0.5, 0.0}, 2.0, 1.5);
        if (GetParam())
        {
            float_to_bf(g_bf_x_min, region.m_min.x);
            float_to_bf(g_bf_x_max, region.m_max.x);
            float_to_bf(g_bf_y_min, region.m_min.y);
            float_to_bf(g_bf_y_max, region.m_max.y);
            copy_bf(g_bf_x_3rd, g_bf_x_min);
            copy_bf(g_bf_y_3rd, g_bf_y_min);
        }
        m_specific.pert_pt = record_perturbation_orbit;
        m_specific.pert_ref = record_reference;
        m_specific.pert_ref_bf = record_reference_bf;
        g_dispatch.set_orbit_calc(record_standard_orbit);
        s_perturbation_orbit.clear();
        s_standard_orbit.clear();
        s_reference_coordinates.clear();
        s_reference_values.clear();
    }

    void complete(SynchronousOrbit &pass, PertEngine *perturbation = nullptr)
    {
        bool done{};
        for (int chunk = 0; chunk < 20000 && !done; ++chunk)
        {
            done = pass.iterate(perturbation);
        }
        ASSERT_TRUE(done);
    }

    void forbid_ordinary_orbits()
    {
        g_dispatch.set_orbit_calc([]() -> int { throw std::logic_error("SOI bypassed perturbation"); });
        s_standard_orbit.clear();
        m_frame.assign(m_frame.size(), -1);
        m_plots.clear();
    }

    std::vector<int> m_frame;
};

TEST_P(TestSynchronousPerturbation, scanMatchesOrdinarySoiWithoutCallingTheOrdinaryOrbit)
{
    prepare();
    SynchronousOrbit ordinary;
    complete(ordinary);
    const std::vector<int> expected{m_frame};
    ASSERT_FALSE(s_standard_orbit.empty());
    forbid_ordinary_orbits();
    SynchronousOrbit perturbation;

    complete(perturbation, &m_engine);

    EXPECT_EQ(expected, m_frame);
    EXPECT_FALSE(s_perturbation_orbit.empty());
    EXPECT_TRUE(s_standard_orbit.empty());
    EXPECT_GT(m_engine.reference_count(), 0);
}

TEST_P(TestSynchronousPerturbation, rhombusTraversalContinuesTheInterpolatedValueAndIterationCount)
{
    char stack_marker{};
    ValueSaver saved_stack_top{g_top_of_stack, &stack_marker};
    prepare(32, 32, {{-0.76, 0.09}, {-0.74, 0.11}, {-0.76, 0.09}});
    SynchronousOrbit ordinary;
    complete(ordinary);
    const std::vector<int> expected{m_frame};
    ASSERT_FALSE(s_standard_orbit.empty());
    const std::complex<double> initial{s_standard_orbit.front()};
    EXPECT_NE((std::complex<double>{-0.76, 0.11}), initial);
    const int expected_depth{g_max_rhombus_depth};
    forbid_ordinary_orbits();
    SynchronousOrbit perturbation;

    complete(perturbation, &m_engine);

    EXPECT_EQ(expected_depth, g_max_rhombus_depth);
    EXPECT_EQ(expected, m_frame);
    ASSERT_FALSE(s_reference_values.empty());
    EXPECT_NEAR(initial.real(), s_reference_values.front().real(), 1e-14);
    EXPECT_NEAR(initial.imag(), s_reference_values.front().imag(), 1e-14);
    EXPECT_FALSE(s_perturbation_orbit.empty());
    EXPECT_TRUE(s_standard_orbit.empty());
}

TEST_P(TestSynchronousPerturbation, narrowRegionRetainsTheSoiInterpolationPolicy)
{
    prepare(8, 6, {{-0.75 - 1e-10, 0.1 - 1e-10}, {-0.75 + 1e-10, 0.1 + 1e-10}, {-0.75 - 1e-10, 0.1 - 1e-10}});
    SynchronousOrbit ordinary;
    complete(ordinary);
    const std::vector<int> expected{m_frame};
    forbid_ordinary_orbits();
    SynchronousOrbit perturbation;

    complete(perturbation, &m_engine);

    EXPECT_EQ(expected, m_frame);
    EXPECT_FALSE(s_perturbation_orbit.empty());
    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(g_image_region.m_min.x, s_reference_coordinates.front().real(), 1e-15);
    EXPECT_NEAR(g_image_region.m_max.y, s_reference_coordinates.front().imag(), 1e-15);
}

TEST_P(TestSynchronousPerturbation, unfinishedScanRetainsItsOrbitAcrossReferenceAndIterationChunks)
{
    prepare(8, 6, {{0.124, 0.124}, {0.126, 0.126}, {0.124, 0.124}}, 5000);
    SynchronousOrbit ordinary;
    complete(ordinary);
    const std::vector<int> expected{m_frame};
    forbid_ordinary_orbits();
    SynchronousOrbit perturbation;

    EXPECT_FALSE(perturbation.iterate(&m_engine));
    EXPECT_TRUE(s_perturbation_orbit.empty());
    EXPECT_TRUE(m_plots.empty());
    EXPECT_FALSE(perturbation.iterate(&m_engine));
    EXPECT_FALSE(perturbation.iterate(&m_engine));
    EXPECT_EQ(2048, s_perturbation_orbit.size());
    EXPECT_TRUE(m_plots.empty());
    EXPECT_FALSE(perturbation.iterate(&m_engine));
    EXPECT_EQ(4096, s_perturbation_orbit.size());
    EXPECT_TRUE(m_plots.empty());

    complete(perturbation, &m_engine);

    EXPECT_EQ(expected, m_frame);
    EXPECT_TRUE(s_standard_orbit.empty());
}

TEST_P(TestSynchronousPerturbation, standardFractalRoutesRequestedAndAutomaticPerturbationIntoSoi)
{
    prepare();
    SynchronousOrbit ordinary;
    complete(ordinary);
    const std::vector<int> expected{m_frame};
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_force_symmetry{g_force_symmetry, SymmetryType::NONE};
    g_std_calc_mode = CalcMode::SYNCHRONOUS_ORBIT;
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.per_image = []
    {
        g_dispatch.set_orbit_calc(record_standard_orbit);
        return true;
    };
    for (const PerturbationMode mode : {PerturbationMode::YES, PerturbationMode::AUTO})
    {
        SCOPED_TRACE(static_cast<int>(mode));
        const bool requested{mode == PerturbationMode::YES || (mode == PerturbationMode::AUTO && GetParam())};
        g_user.perturbation = mode;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        m_frame.assign(m_frame.size(), -1);
        m_plot_statuses.clear();
        s_perturbation_orbit.clear();
        s_standard_orbit.clear();
        StandardFractal calculation;
        calculation.resume();
        for (int chunk = 0; chunk < 1000 && !calculation.done(); ++chunk)
        {
            calculation.iterate();
        }
        ASSERT_TRUE(calculation.done());
        EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
        EXPECT_EQ(CalcMode::SYNCHRONOUS_ORBIT, g_std_calc_mode);
        EXPECT_EQ(requested, !s_perturbation_orbit.empty());
        EXPECT_EQ(requested, s_standard_orbit.empty());
        ASSERT_FALSE(m_plot_statuses.empty());
        for (const StandardPassStatus &status : m_plot_statuses)
        {
            EXPECT_EQ(requested, status.perturbation_active);
        }
        EXPECT_EQ(expected, m_frame);
    }
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestSynchronousPerturbation, Bool());

class MockPerturbationSound : public SoundOutput
{
public:
    MOCK_METHOD(bool, play, (const SoundSample &), (override));
    MOCK_METHOD(void, begin_orbit, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(bool, ready, (), (const, override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, finish, (), (override));
};

class TestPerturbationOutput : public TestPerturbationIterations
{
protected:
    void SetUp() override;
    void prepare(const std::complex<double> &center, long iterations = 16, double offset = 0.0);
    int complete_pixel(bool incremental = true);
    std::vector<SoundSample> expected_samples(const std::complex<double> &center) const;
    void expect_samples(const std::vector<SoundSample> &expected) const;

    std::shared_ptr<StrictMock<MockPerturbationSound>> m_output{std::make_shared<StrictMock<MockPerturbationSound>>()};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, m_output};
    ValueSaver<double> m_saved_mx1{g_plot_mx1, -2.0};
    ValueSaver<double> m_saved_mx2{g_plot_mx2, 0.0};
    ValueSaver<double> m_saved_my1{g_plot_my1, 1.25};
    ValueSaver<double> m_saved_my2{g_plot_my2, 0.0};
    ValueSaver<int> m_saved_orbit_color{g_orbit_color, 15};
    ValueSaver<bool> m_saved_orbit_flag{g_orbit_save_flag, false};
    std::vector<SoundSample> m_samples;
    std::array<int, WIDTH * HEIGHT> m_background{};
    int m_boundaries{};
    bool m_ready{true};
};

void TestPerturbationOutput::SetUp()
{
    TestPerturbationIterations::SetUp();
    for (std::size_t i = 0; i < m_background.size(); ++i)
    {
        m_background[i] = static_cast<int>(i % 16);
    }
    g_show_orbit = true;
    EXPECT_CALL(*m_output, begin_orbit()).WillRepeatedly([this] { ++m_boundaries; });
    EXPECT_CALL(*m_output, ready()).WillRepeatedly([this] { return m_ready; });
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                return true;
            });
}

void TestPerturbationOutput::prepare(const std::complex<double> &center, const long iterations, const double offset)
{
    m_engine.finish();
    if (m_big_float)
    {
        free_bf_vars();
        m_big_float = false;
    }
    m_pixels = m_background;
    m_plots.clear();
    m_samples.clear();
    m_boundaries = 0;
    s_perturbation_orbit.clear();
    g_max_iterations = iterations;
    g_image_region = ImageRegion{{-2.0, -1.5}, {2.0, 1.5}, {-2.0, -1.5}};
    g_col = WIDTH - 1;
    m_specific.pert_pt = record_perturbation_orbit;
    initialize_reference(center - std::complex<double>{offset, 0.0}, offset);
}

int TestPerturbationOutput::complete_pixel(const bool incremental)
{
    int color{CALC_YIELDED};
    for (int chunk = 0; chunk < 1000 && color == CALC_YIELDED; ++chunk)
    {
        color = m_engine.calculate_pixel(g_col, g_row, incremental);
    }
    EXPECT_TRUE(calc_ok(color));
    return color;
}

std::vector<SoundSample> TestPerturbationOutput::expected_samples(const std::complex<double> &center) const
{
    std::vector<SoundSample> samples;
    std::complex<double> z{center};
    for (long iteration = 1; iteration < g_max_iterations; ++iteration)
    {
        z = z * z + center;
        if (std::norm(z) >= 4.0)
        {
            break;
        }
        const int x{static_cast<int>((z.imag() - 1.5) * -2.0)};
        const int y{static_cast<int>((z.real() + 2.0) * 1.25)};
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        {
            samples.push_back({static_cast<double>(x), static_cast<double>(y), 0.0});
        }
    }
    return samples;
}

void TestPerturbationOutput::expect_samples(const std::vector<SoundSample> &expected) const
{
    ASSERT_EQ(expected.size(), m_samples.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(expected[i].x, m_samples[i].x);
        EXPECT_EQ(expected[i].y, m_samples[i].y);
        EXPECT_EQ(expected[i].z, m_samples[i].z);
    }
}

TEST_P(TestPerturbationOutput, reconstructedSamplesMatchOrbitOrderAndOverlayRestoresBeforePixelOutput)
{
    for (const std::complex<double> center : {std::complex<double>{-0.5, 0.5}, {0.5, 0.5}})
    {
        SCOPED_TRACE(center);
        prepare(center);
        g_show_orbit = false;
        const int expected_color{complete_pixel(false)};
        const std::array<int, WIDTH * HEIGHT> expected_pixels{m_pixels};
        prepare(center);
        g_show_orbit = true;
        const std::vector<SoundSample> samples{expected_samples(center)};
        ASSERT_FALSE(samples.empty());

        EXPECT_EQ(expected_color, complete_pixel());

        expect_samples(samples);
        EXPECT_EQ(1, m_boundaries);
        EXPECT_FALSE(g_orbit_save_flag);
        EXPECT_EQ(expected_pixels, m_pixels);
        EXPECT_EQ(2 * samples.size() + 1, m_plots.size());
    }
}

TEST_P(TestPerturbationOutput, atomicAndIncrementalOutputHaveIdenticalSamplesAndPlots)
{
    prepare({-0.5, 0.5});
    const int expected_color{complete_pixel(false)};
    const std::vector<SoundSample> samples{m_samples};
    const std::vector<Point2i> plots{m_plots};
    const std::array<int, WIDTH * HEIGHT> pixels{m_pixels};
    prepare({-0.5, 0.5});

    EXPECT_EQ(expected_color, complete_pixel());

    expect_samples(samples);
    EXPECT_EQ(plots, m_plots);
    EXPECT_EQ(pixels, m_pixels);
}

TEST_P(TestPerturbationOutput, replacementAfterAcceptedPointsDoesNotRepeatSamplesOrBoundary)
{
    const std::complex<double> center{-0.5, 0.5};
    prepare(center, 16, 0.125);
    m_specific.pert_pt =
        [](const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter)
    {
        record_perturbation_orbit(reference, delta, parameter);
        if (s_perturbation_orbit.size() == 4)
        {
            delta = {std::numeric_limits<double>::quiet_NaN(), 0.0};
        }
    };

    EXPECT_GE(complete_pixel(), 0);

    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_EQ(1, m_boundaries);
    expect_samples(expected_samples(center));
    EXPECT_FALSE(g_orbit_save_flag);
}

TEST_P(TestPerturbationOutput, refusedSamplesRetainMathAndDoNotPlotOrRepeatOrbitBoundaries)
{
    prepare({-0.5, 0.5});
    EXPECT_CALL(*m_output, play(_)).WillOnce(Return(false)).WillOnce(Return(false)).RetiresOnSaturation();

    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row));
    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row));

    EXPECT_EQ(1, s_perturbation_orbit.size());
    EXPECT_EQ(1, m_boundaries);
    EXPECT_TRUE(m_plots.empty());
    EXPECT_TRUE(m_samples.empty());
    EXPECT_GE(complete_pixel(), 0);
    expect_samples(expected_samples({-0.5, 0.5}));
    EXPECT_EQ(1, m_boundaries);
}

TEST_P(TestPerturbationOutput, finalSampleRetainsOverlayUntilNormalReleaseIsServiced)
{
    prepare({-0.5, 0.5}, 3);
    EXPECT_CALL(*m_output, play(_))
        .WillRepeatedly(
            [this](const SoundSample &sample)
            {
                m_samples.push_back(sample);
                m_ready = m_samples.size() < 2;
                return true;
            });
    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_TRUE(g_orbit_save_flag);
    EXPECT_EQ(2, m_samples.size());
    EXPECT_EQ(2, m_plots.size());
    EXPECT_EQ(2, s_perturbation_orbit.size());
    m_ready = true;

    EXPECT_GE(m_engine.calculate_pixel(g_col, g_row, true), 0);

    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_EQ(5, m_plots.size());
    EXPECT_EQ(2, m_samples.size());
    EXPECT_EQ(1, m_boundaries);
}

TEST_P(TestPerturbationOutput, abandoningPixelRestoresOverlayWithoutMutingOrFinishingOutput)
{
    prepare({-0.5, 0.5});
    ASSERT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    ASSERT_TRUE(g_orbit_save_flag);

    m_engine.finish();

    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_EQ(m_background, m_pixels);
    EXPECT_EQ(1, m_samples.size());
}

TEST_P(TestPerturbationOutput, outputFailureRestoresOverlayAndPreservesOriginalException)
{
    prepare({-0.5, 0.5});
    ASSERT_EQ(CALC_YIELDED, m_engine.calculate_pixel(g_col, g_row, true));
    EXPECT_CALL(*m_output, play(_)).WillOnce(Throw(std::bad_array_new_length{}));

    EXPECT_THROW(m_engine.calculate_pixel(g_col, g_row, true), std::bad_array_new_length);

    EXPECT_FALSE(g_orbit_save_flag);
    EXPECT_EQ(m_background, m_pixels);
    EXPECT_EQ(1, m_engine.reference_count());
}

TEST_P(TestPerturbationOutput, burningShipOverlayAndSoundObserveNativeOrbitSequences)
{
    ValueSaver saved_degree{g_params[2]};
    g_fractal_type = FractalType::BURNING_SHIP;
    const std::complex<double> point{0.25, 0.125};
    for (const int degree : {2, 3, 4, 5})
    {
        SCOPED_TRACE(degree);
        g_params[2] = degree;
        m_specific = *get_fractal_specific(g_fractal_type);
        prepare(std::conj(point), 8);
        m_specific.pert_pt = burning_ship_perturb;
        g_init = {point.real(), point.imag()};
        g_old_z = g_init;
        if (GetParam())
        {
            float_to_bf(g_param_z_bf.x, point.real());
            float_to_bf(g_param_z_bf.y, point.imag());
            copy_bf(g_old_z_bf.x, g_param_z_bf.x);
            copy_bf(g_old_z_bf.y, g_param_z_bf.y);
        }
        std::vector<SoundSample> samples;
        for (long iteration = 1; iteration < g_max_iterations; ++iteration)
        {
            const bool escaped{GetParam() ? burning_ship_bf_fractal() != 0 : burning_ship_orbit() != 0};
            if (escaped)
            {
                break;
            }
            const DComplex value{GetParam() ? cmplx_bf_to_float(g_new_z_bf) : g_new_z};
            const int x{static_cast<int>((value.y - 1.5) * -2.0)};
            const int y{static_cast<int>((value.x + 2.0) * 1.25)};
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            {
                samples.push_back({static_cast<double>(x), static_cast<double>(y), 0.0});
            }
        }
        ASSERT_FALSE(samples.empty());

        EXPECT_TRUE(calc_ok(complete_pixel()));

        expect_samples(samples);
        EXPECT_EQ(2 * samples.size() + 1, m_plots.size());
        EXPECT_FALSE(g_orbit_save_flag);
    }
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPerturbationOutput, Bool());

TEST_F(TestPerturbationPixels, highPrecisionInitialOffsetPreservesCancellation)
{
    ValueSaver saved_param{g_param_z1, DComplex{-0.5, 0.0}};
    ValueSaver saved_real{g_params[0], -0.5};
    ValueSaver saved_imag{g_params[1], 0.0};
    g_max_iterations = 2;
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    float_to_bf(center.x, 0.5);
    clear_bf(center.y);
    {
        BigStackSaver saved;
        const BigFloat delta{alloc_stack(g_bf_length + 2)};
        float_to_bf(delta, 1e-40);
        add_a_bf(center.x, delta);
    }
    m_specific.pert_pt = record_perturbation_orbit;
    s_perturbation_orbit.clear();

    EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

    ASSERT_EQ(1, s_perturbation_orbit.size());
    EXPECT_NEAR(1e-40, s_perturbation_orbit.front().real(), 1e-52);
    EXPECT_DOUBLE_EQ(0.0, s_perturbation_orbit.front().imag());
}

TEST_F(TestPerturbationPixels, highPrecisionBailoutBoundariesPreserveSmallDisplacements)
{
    ValueSaver saved_test{g_bailout_test};
    ValueSaver saved_mode{g_use_init_orbit, InitOrbitMode::VALUE};
    ValueSaver saved_initial{g_init_orbit, DComplex{}};
    ValueSaver saved_real{g_params[0], 0.0};
    ValueSaver saved_imag{g_params[1], 0.0};
    g_max_iterations = 2;

    struct Boundary
    {
        Bailout test;
        DComplex point;
        bool imaginary;
        int outward_sign;
        double limit;
    };

    const std::array<Boundary, 8> boundaries{{
        {Bailout::MOD, {-2.0, 0.0}, false, -1, 4.0},
        {Bailout::REAL, {-2.0, 0.0}, false, -1, 4.0},
        {Bailout::IMAG, {0.0, 2.0}, true, 1, 4.0},
        {Bailout::OR, {0.0, 2.0}, true, 1, 4.0},
        {Bailout::AND, {2.0, 3.0}, false, 1, 4.0},
        {Bailout::MANH, {1.0, -1.0}, false, 1, 4.0},
        {Bailout::MANR, {3.0, -1.0}, false, 1, 4.0},
        {Bailout::MOD, {3.0, 4.0}, false, 1, 25.0},
    }};
    for (const Boundary &boundary : boundaries)
    {
        SCOPED_TRACE(static_cast<int>(boundary.test));
        SCOPED_TRACE(boundary.limit);
        g_bailout_test = boundary.test;
        g_magnitude_limit = boundary.limit;
        for (const int reference_mode : {0, 1, 2})
        {
            SCOPED_TRACE(reference_mode);
            for (const int sign : {-1, 0, 1})
            {
                SCOPED_TRACE(sign);
                m_engine.finish();
                if (m_big_float)
                {
                    free_bf_vars();
                }
                init_bf_dec(60);
                m_big_float = true;
                g_bf_math = BFMathType::BIG_FLT;
                const double offset{sign * 1e-40};
                const std::complex<double> half_horizontal{
                    reference_mode != 0 && !boundary.imaginary ? offset : 0.0, 0.0};
                const std::complex<double> half_vertical{0.0, reference_mode != 0 && boundary.imaginary ? offset : 0.0};
                const BFComplex center{m_engine.initialize_frame_bf(half_horizontal, half_vertical)};
                float_to_bf(center.x, boundary.point.x);
                float_to_bf(center.y, boundary.point.y);
                if (reference_mode == 0)
                {
                    BigStackSaver saved;
                    const BigFloat delta{alloc_stack(g_r_bf_length + 2)};
                    float_to_bf(delta, offset);
                    add_a_bf(boundary.imaginary ? center.y : center.x, delta);
                }
                DComplex expected;
                {
                    BigStackSaver saved;
                    const BFComplex coordinate{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
                    const BigFloat delta{alloc_stack(g_r_bf_length + 2)};
                    float_to_bf(coordinate.x, boundary.point.x);
                    float_to_bf(coordinate.y, boundary.point.y);
                    float_to_bf(delta, offset);
                    add_a_bf(boundary.imaginary ? coordinate.y : coordinate.x, delta);
                    expected = {
                        static_cast<double>(bf_to_float(coordinate.x)), static_cast<double>(bf_to_float(coordinate.y))};
                }
                m_engine.set_glitch_tolerance(reference_mode == 2 ? 2.0 : 0.0);
                const bool escaped{sign == boundary.outward_sign};

                EXPECT_EQ(escaped ? 1 : 7, m_engine.calculate_pixel(WIDTH - 1, 0));

                EXPECT_EQ(escaped ? 1 : 2, g_real_color_iter);
                EXPECT_EQ(escaped ? 1 : 2, g_color_iter);
                EXPECT_DOUBLE_EQ(expected.x, g_new_z.x);
                EXPECT_DOUBLE_EQ(expected.y, g_new_z.y);
                EXPECT_DOUBLE_EQ(escaped ? 0.0 : expected.x, g_old_z.x);
                EXPECT_DOUBLE_EQ(escaped ? 0.0 : expected.y, g_old_z.y);
                EXPECT_DOUBLE_EQ(expected.x * expected.x, g_temp_sqr_x);
                EXPECT_DOUBLE_EQ(expected.y * expected.y, g_temp_sqr_y);
                EXPECT_DOUBLE_EQ(g_temp_sqr_x + g_temp_sqr_y, g_magnitude);
                EXPECT_EQ(reference_mode == 2 ? 2 : 1, m_engine.reference_count());
            }
        }
    }
}

TEST_F(TestPerturbationPixels, highPrecisionMandelBoundaryRetainsOnlyTheCurrentPreciseOrbitValue)
{
    ValueSaver saved_test{g_bailout_test, Bailout::MOD};
    ValueSaver saved_mode{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver saved_real{g_params[0], 0.0};
    ValueSaver saved_imag{g_params[1], 0.0};
    g_max_iterations = 16;
    m_specific.pert_ref_bf = record_reference_bf;
    for (const int sign : {-1, 0, 1})
    {
        SCOPED_TRACE(sign);
        m_engine.finish();
        if (m_big_float)
        {
            free_bf_vars();
        }
        init_bf_dec(60);
        m_big_float = true;
        g_bf_math = BFMathType::BIG_FLT;
        const BFComplex center{m_engine.initialize_frame_bf({}, {})};
        float_to_bf(center.x, -2.0);
        clear_bf(center.y);
        {
            BigStackSaver saved;
            const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
            float_to_bf(offset, sign * 1e-40);
            add_a_bf(center.x, offset);
        }
        double expected_old;
        double expected_new;
        {
            BigStackSaver saved;
            const BigFloat z{alloc_stack(g_r_bf_length + 2)};
            const BigFloat square{alloc_stack(g_r_bf_length + 2)};
            copy_bf(z, center.x);
            for (long i = 0; i < (sign >= 0 ? g_max_iterations - 1 : 1); ++i)
            {
                expected_old = static_cast<double>(bf_to_float(z));
                square_bf(square, z);
                add_bf(z, square, center.x);
            }
            expected_new = static_cast<double>(bf_to_float(z));
            if (sign >= 0)
            {
                expected_old = expected_new;
            }
        }
        s_reference_coordinates.clear();

        EXPECT_EQ(sign >= 0 ? 7 : 1, m_engine.calculate_pixel(0, 0));

        EXPECT_EQ(sign >= 0 ? 16 : 1, g_real_color_iter);
        EXPECT_DOUBLE_EQ(expected_new, g_new_z.x);
        EXPECT_DOUBLE_EQ(0.0, g_new_z.y);
        EXPECT_DOUBLE_EQ(expected_old, g_old_z.x);
        EXPECT_DOUBLE_EQ(expected_new * expected_new, g_magnitude);
        EXPECT_EQ(1, m_engine.reference_count());
        EXPECT_EQ(g_max_iterations + (sign >= 0 ? g_max_iterations - 1 : 1), s_reference_coordinates.size());
    }
}

TEST_F(TestPerturbationPixels, highPrecisionFixedOrbitPreservesSmallParameterOffsets)
{
    ValueSaver saved_mode{g_use_init_orbit, InitOrbitMode::VALUE};
    ValueSaver saved_initial{g_init_orbit, DComplex{0.25, 0.125}};
    ValueSaver saved_real{g_params[0], 1e-40};
    ValueSaver saved_imag{g_params[1], -2e-40};
    g_max_iterations = 2;
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    const BFComplex center{m_engine.initialize_frame_bf({}, {})};
    float_to_bf(center.x, 0.5);
    float_to_bf(center.y, -0.125);
    m_specific.pert_ref_bf = [](const BFComplex &parameter, BFComplex &z)
    {
        BigStackSaver saved;
        const BFComplex displacement{alloc_stack(g_bf_length + 2), alloc_stack(g_bf_length + 2)};
        float_to_bf(displacement.x, 0.25);
        float_to_bf(displacement.y, 0.125);
        sub_bf(displacement.x, z.x, displacement.x);
        sub_bf(displacement.y, z.y, displacement.y);
        s_reference_coordinates.emplace_back(bf_to_float(displacement.x), bf_to_float(displacement.y));
        mandel_ref_pt_bf(parameter, z);
    };

    EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(1e-40, s_reference_coordinates.front().real(), 1e-52);
    EXPECT_NEAR(-2e-40, s_reference_coordinates.front().imag(), 1e-52);
}

TEST_F(TestPerturbationPixels, deepAffineFramePreservesBothBasisVectors)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_region{g_image_region, ImageRegion{{0.24, 0.12}, {0.26, 0.13}, {0.24, 0.12}}};
    g_user.perturbation = PerturbationMode::YES;
    g_user.perturbation_tolerance = 0.0;
    g_max_iterations = 2;
    init_bf_dec(60);
    m_big_float = true;
    const auto set_offset = [](const BigFloat value, const double center, const double offset)
    {
        BigStackSaver saved;
        const BigFloat delta{alloc_stack(g_bf_length + 2)};
        float_to_bf(value, center);
        float_to_bf(delta, offset);
        add_a_bf(value, delta);
    };
    set_offset(g_bf_x_min, 0.25, -3e-40);
    set_offset(g_bf_x_max, 0.25, 3e-40);
    set_offset(g_bf_x_3rd, 0.25, -1e-40);
    set_offset(g_bf_y_min, 0.125, -1e-40);
    set_offset(g_bf_y_max, 0.125, 1e-40);
    set_offset(g_bf_y_3rd, 0.125, -3e-40);
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt = record_coordinate;
    g_dispatch = FractalDispatch{m_specific};
    s_coordinates.clear();
    StandardFractal calculation;
    calculation.resume();

    for (int increments = 0; increments < WIDTH * HEIGHT + 10 && !calculation.done(); ++increments)
    {
        calculation.iterate();
    }

    ASSERT_TRUE(calculation.done());
    ASSERT_EQ(WIDTH * HEIGHT, s_coordinates.size());
    EXPECT_EQ(g_max_iterations, s_reference_coordinates.size());
    for (const CoordinateSample &sample : s_coordinates)
    {
        const double x{2.0 * sample.pixel.x / (WIDTH - 1) - 1.0};
        const double y{1.0 - 2.0 * sample.pixel.y / (HEIGHT - 1)};
        EXPECT_NEAR((2.0 * x - y) * 1e-40, sample.parameter_delta.real(), 1e-52);
        EXPECT_NEAR((x + 2.0 * y) * 1e-40, sample.parameter_delta.imag(), 1e-52);
    }
    calculation.finish();
}

TEST_F(TestPerturbationPixels, failedReferenceComputesTheRequestedPixelImmediately)
{
    m_engine.initialize_frame({1.0, 0.0}, {0.75, 0.0}, {0.0, 0.75});

    for (int request = 0; request < WIDTH * HEIGHT * 2; ++request)
    {
        EXPECT_EQ(7, m_engine.calculate_pixel(0, 2));
        EXPECT_EQ(7, m_pixels[2 * WIDTH]);
    }

    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_THAT(m_plots, Each(Point2i{0, 2}));
}

TEST_F(TestPerturbationPixels, replacesTheCurrentReferenceWithoutRetainingHistoryAtTinyZoomScales)
{
    m_engine.initialize_frame({0.25, 0.0}, {1e-200, 0.0}, {0.0, 1e-200});
    m_engine.set_glitch_tolerance(2.0);
    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, HEIGHT - 1));
    EXPECT_EQ(3, m_engine.reference_count());

    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));

    EXPECT_EQ(4, m_engine.reference_count());
    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(4, m_engine.reference_count());
    EXPECT_THAT(m_plots, ElementsAre(Point2i{0, 0}, Point2i{WIDTH - 1, HEIGHT - 1}, Point2i{0, 0}, Point2i{0, 0}));
}

TEST_F(TestPerturbationPixels, stringentToleranceCompletesRepeatedReferenceReplacements)
{
    g_max_iterations = 10000;
    m_engine.initialize_frame({-0.1, 0.1}, {1e-10, 0.0}, {0.0, 1e-10});
    m_engine.set_glitch_tolerance(2.0);
    m_engine.initialize_pixel_strategy();
    int references{1};

    for (int pass = 0; pass < 3; ++pass)
    {
        for (int row = 0; row < HEIGHT; ++row)
        {
            for (int col = 0; col < WIDTH; ++col)
            {
                SCOPED_TRACE(row * WIDTH + col);
                EXPECT_EQ(7, m_engine.calculate_pixel(col, row));
                EXPECT_EQ(++references, m_engine.reference_count());
                EXPECT_EQ(7, m_engine.calculate_pixel(col, row));
                EXPECT_EQ(references, m_engine.reference_count());
            }
        }
    }

    EXPECT_THAT(m_pixels, Each(7));
    EXPECT_EQ(2 * WIDTH * HEIGHT * 3, m_plots.size());
    m_engine.finish();
    EXPECT_EQ(0, m_engine.reference_count());
}

TEST_F(TestPerturbationPixels, defaultToleranceMatchesPerPixelReferences)
{
    const std::complex<double> center{-0.5, 0.0};
    constexpr double ZOOM_RADIUS{1.5};
    PertEngine control;
    control.initialize_frame(center, {ZOOM_RADIUS, 0.0}, {0.0, ZOOM_RADIUS});
    control.set_glitch_tolerance(2.0);
    m_engine.initialize_frame(center, {ZOOM_RADIUS, 0.0}, {0.0, ZOOM_RADIUS});

    for (int row = 0; row < HEIGHT; ++row)
    {
        for (int col = 0; col < WIDTH; ++col)
        {
            SCOPED_TRACE(row * WIDTH + col);
            const int expected{control.calculate_pixel(col, row)};
            EXPECT_GE(expected, 0);
            EXPECT_EQ(expected, m_engine.calculate_pixel(col, row));
            EXPECT_EQ(expected, m_pixels[row * WIDTH + col]);
        }
    }

    EXPECT_LT(m_engine.reference_count(), control.reference_count());
    control.finish();
}

TEST_F(TestPerturbationPixels, deepZoomKeepsTheCallersPixelOrder)
{
    initialize_deep_frame();
    const std::vector<Point2i> order{{5, 3}, {0, 5}, {7, 0}, {1, 1}};

    for (const Point2i &pixel : order)
    {
        EXPECT_EQ(7, m_engine.calculate_pixel(pixel.x, pixel.y));
        EXPECT_EQ(7, m_pixels[pixel.y * WIDTH + pixel.x]);
    }

    EXPECT_EQ(order, m_plots);
    EXPECT_GT(m_engine.reference_count(), 1);
}

TEST_F(TestPerturbationPixels, deepZoomBoundaryTraceCompletesWithoutDeferredOrRepeatedPixels)
{
    initialize_deep_frame();
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    m_specific.calc_type = calculate_requested_pixel;
    g_dispatch = FractalDispatch{m_specific};
    BoundaryTrace trace;

    for (int increments = 0; increments < 100 && !trace.done(); ++increments)
    {
        trace.begin_chunk();
        while (trace.select_pixel())
        {
            trace.complete_pixel(calculate_requested_pixel());
        }
    }

    ASSERT_TRUE(trace.done());
    EXPECT_GT(m_engine.reference_count(), 1);
    EXPECT_THAT(m_pixels, Each(7));
    std::array<int, WIDTH * HEIGHT> visits{};
    ASSERT_FALSE(s_requested_pixels.empty());
    for (const Point2i &pixel : s_requested_pixels)
    {
        EXPECT_EQ(0, visits.at(pixel.y * WIDTH + pixel.x)++);
    }
}

TEST_F(TestPerturbationPixels, boundaryTraceReceivesTheStoredNonzeroPaletteColor)
{
    m_engine.initialize_frame({0.25, 0.0}, {1e-10, 0.0}, {0.0, 1e-10});
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    g_inside_method = ColorMethod::ITER;
    g_max_iterations = 256;

    const int color{m_engine.calculate_pixel(0, 0)};

    EXPECT_GT(color, 0);
    EXPECT_LT(color, g_colors);
    EXPECT_EQ(color, m_pixels[0]);
}

TEST_F(TestPerturbationPixels, standardFractalEnablesPerturbationForBoundaryTrace)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_region{
        g_image_region, ImageRegion{{0.25 - 1e-10, -1e-10}, {0.25 + 1e-10, 1e-10}, {0.25 - 1e-10, -1e-10}}};
    g_user.perturbation = PerturbationMode::YES;
    g_user.perturbation_tolerance = 2.0;
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    g_dispatch = FractalDispatch{m_specific};
    StandardFractal calculation;
    calculation.resume();
    bool used_replacement_reference{};

    for (int increments = 0; increments < 100 && !calculation.done(); ++increments)
    {
        calculation.iterate();
        const StandardPassStatus status{calculation.standard_pass_status()};
        used_replacement_reference |= status.perturbation_active && status.perturbation_reference_count > 1;
    }

    EXPECT_TRUE(calculation.done());
    EXPECT_TRUE(used_replacement_reference);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_P(TestPerturbationIterations, standardPassesPreserveFixedInsideAndOutsideColors)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_force_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 1.0 / (WIDTH - 1)};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 0.5 / (HEIGHT - 1)};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    g_image_region = ImageRegion{{-0.5, 0.0}, {0.5, 0.5}, {-0.5, 0.0}};
    fill_pixel_grid();
    g_inside_method = g_outside_method = ColorMethod::COLOR;
    g_inside_color = 1;
    g_outside_color = 7;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.pert_pt = record_perturbation_orbit;
    g_dispatch = FractalDispatch{m_specific};
    std::array<int, WIDTH * HEIGHT> expected{};

    for (const CalcMode pass : {CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS})
    {
        for (const PerturbationMode mode : {PerturbationMode::NO, PerturbationMode::YES})
        {
            SCOPED_TRACE(static_cast<char>(pass));
            SCOPED_TRACE(static_cast<int>(mode));
            g_std_calc_mode = pass;
            g_user.perturbation = mode;
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            m_pixels.fill(0);
            m_plot_statuses.clear();
            s_perturbation_orbit.clear();
            if (GetParam() && mode == PerturbationMode::YES)
            {
                init_bf_dec(60);
                m_big_float = true;
                float_to_bf(g_bf_x_min, -0.5);
                float_to_bf(g_bf_x_max, 0.5);
                float_to_bf(g_bf_x_3rd, -0.5);
                float_to_bf(g_bf_y_min, 0.0);
                float_to_bf(g_bf_y_max, 0.5);
                float_to_bf(g_bf_y_3rd, 0.0);
            }
            StandardFractal calculation;
            calculation.resume();
            for (int increments = 0; increments < 1000 && !calculation.done(); ++increments)
            {
                calculation.iterate();
            }

            ASSERT_TRUE(calculation.done());
            EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
            EXPECT_EQ(pass, g_std_calc_mode);
            EXPECT_THAT(m_pixels, Each(AnyOf(1, 7)));
            EXPECT_THAT(m_pixels, Contains(1));
            EXPECT_THAT(m_pixels, Contains(7));
            EXPECT_EQ(mode == PerturbationMode::YES, !s_perturbation_orbit.empty());
            ASSERT_FALSE(m_plot_statuses.empty());
            for (const StandardPassStatus &status : m_plot_statuses)
            {
                EXPECT_EQ(mode == PerturbationMode::YES, status.perturbation_active);
                if (mode == PerturbationMode::YES)
                {
                    EXPECT_GT(status.perturbation_reference_count, 0);
                }
            }
            if (mode == PerturbationMode::NO)
            {
                expected = m_pixels;
            }
            else
            {
                EXPECT_EQ(expected, m_pixels);
            }
            calculation.finish();
            if (m_big_float)
            {
                free_bf_vars();
                m_big_float = false;
            }
        }
    }
}

TEST_F(TestPerturbationPixels, inversionKeepsPerturbationActiveForStandardPasses)
{
    ValueSaver saved_user{g_user, UserData{}};
    g_inversion = {3, 1.0, {1.0, 0.1}, {1.0, 1.0, 0.1}};
    ValueSaver saved_status{g_calc_status};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_region{
        g_image_region, ImageRegion{{1.5 - 1e-10, -1e-10}, {1.5 + 1e-10, 1e-10}, {1.5 - 1e-10, -1e-10}}};
    m_specific.calc_type = calculate_control_pixel;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    g_dispatch = FractalDispatch{m_specific};

    for (const CalcMode pass : {CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS})
    {
        for (const PerturbationMode mode : {PerturbationMode::NO, PerturbationMode::YES})
        {
            SCOPED_TRACE(static_cast<char>(pass));
            SCOPED_TRACE(static_cast<int>(mode));
            g_std_calc_mode = pass;
            g_user.perturbation = mode;
            g_user.perturbation_tolerance = 2.0;
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            m_pixels.fill(0);
            m_plot_statuses.clear();
            s_requested_pixels.clear();
            StandardFractal calculation;
            calculation.resume();

            for (int increments = 0; increments < 100 && !calculation.done(); ++increments)
            {
                calculation.iterate();
            }

            ASSERT_TRUE(calculation.done());
            EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
            EXPECT_EQ(pass, g_std_calc_mode);
            EXPECT_EQ(mode == PerturbationMode::YES, s_requested_pixels.empty());
            EXPECT_THAT(m_pixels, Each(mode == PerturbationMode::YES ? 1 : 7));
            ASSERT_FALSE(m_plot_statuses.empty());
            for (const StandardPassStatus &status : m_plot_statuses)
            {
                EXPECT_EQ(mode == PerturbationMode::YES, status.perturbation_active);
                if (mode == PerturbationMode::YES)
                {
                    EXPECT_GT(status.perturbation_reference_count, 1);
                }
                else
                {
                    EXPECT_EQ(0, status.perturbation_reference_count);
                }
            }
            calculation.finish();
        }
    }
}

TEST_F(TestPerturbationPixels, inversionTransformsReferenceAndRequestedCoordinates)
{
    ValueSaver saved_region{g_image_region, ImageRegion{{1.0, -1.0}, {3.0, 1.0}, {1.0, -1.0}}};
    g_inversion = {3, 2.0, {1.0, 0.5}, {2.0, 1.0, 0.5}};
    m_specific.pert_ref = record_reference;
    m_specific.pert_pt = record_delta;
    m_engine.initialize_frame({2.0, 0.0}, {1.0, 0.0}, {0.0, 1.0});
    m_engine.set_glitch_tolerance(0.0);
    const std::complex<double> offset{1.0, -0.5};
    const std::complex<double> reference{std::complex<double>{1.0, 0.5} + 2.0 * offset / std::norm(offset)};

    EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(reference.real(), s_reference_coordinates.front().real(), 1e-14);
    EXPECT_NEAR(reference.imag(), s_reference_coordinates.front().imag(), 1e-14);
    EXPECT_NEAR(1.0 - reference.real(), s_parameter_delta.real(), 1e-14);
    EXPECT_NEAR(4.5 - reference.imag(), s_parameter_delta.imag(), 1e-14);
}

TEST_F(TestPerturbationPixels, inversionCenterRetainsTheStandardSingularityBehavior)
{
    ValueSaver saved_region{g_image_region, ImageRegion{{1.0, -1.0}, {3.0, 1.0}, {1.0, -1.0}}};
    g_inversion = {3, 2.0, {1.0, 1.0}, {2.0, 1.0, 1.0}};
    m_specific.pert_ref = record_reference;
    m_engine.initialize_frame({2.0, 0.0}, {1.0, 0.0}, {0.0, 1.0});
    m_engine.set_glitch_tolerance(2.0);

    EXPECT_EQ(1, m_engine.calculate_pixel(0, 0));

    EXPECT_EQ(2, m_engine.reference_count());
    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_EQ(std::complex<double>(1.0, 1.0), s_reference_coordinates.back());
}

TEST_F(TestPerturbationPixels, inversionPreservesTheBurningShipReferenceOrientation)
{
    ValueSaver saved_type{g_fractal_type, FractalType::BURNING_SHIP};
    ValueSaver saved_region{g_image_region, ImageRegion{{1.0, -1.0}, {3.0, 1.0}, {1.0, -1.0}}};
    g_inversion = {3, 2.0, {1.0, 0.5}, {2.0, 1.0, 0.5}};
    m_specific.pert_ref = record_reference;
    m_specific.pert_pt = record_delta;
    m_engine.initialize_frame({2.0, 0.0}, {1.0, 0.0}, {0.0, 1.0});
    m_engine.set_glitch_tolerance(0.0);
    const std::complex<double> offset{1.0, -0.5};
    const std::complex<double> reference{std::conj(std::complex<double>{1.0, 0.5} + 2.0 * offset / std::norm(offset))};

    EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(reference.real(), s_reference_coordinates.front().real(), 1e-14);
    EXPECT_NEAR(reference.imag(), s_reference_coordinates.front().imag(), 1e-14);
    EXPECT_NEAR(1.0 - reference.real(), s_parameter_delta.real(), 1e-14);
    EXPECT_NEAR(-4.5 - reference.imag(), s_parameter_delta.imag(), 1e-14);
}

TEST_F(TestPerturbationPixels, invertedDeepZoomPreservesDeltasAcrossReferenceReplacement)
{
    init_bf_dec(60);
    m_big_float = true;
    g_bf_math = BFMathType::BIG_FLT;
    str_to_bf(g_bf_x_min, "1.4999999999999999999999999999999999999999");
    str_to_bf(g_bf_x_max, "1.5000000000000000000000000000000000000001");
    str_to_bf(g_bf_y_min, "-1e-40");
    str_to_bf(g_bf_y_max, "1e-40");
    copy_bf(g_bf_x_3rd, g_bf_x_min);
    copy_bf(g_bf_y_3rd, g_bf_y_min);
    g_inversion = {3, 1.0, {1.0, 0.0}, {1.0, 1.0, 0.0}};
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt = record_delta;
    const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
    float_to_bf(center.x, 1.5);
    clear_bf(center.y);

    EXPECT_EQ(1, m_engine.calculate_pixel(0, 2));

    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(3.0, s_reference_coordinates.front().real(), 1e-14);
    EXPECT_NEAR(4e-40, s_parameter_delta.real(), 1e-52);
    EXPECT_NEAR(0.8e-40, s_parameter_delta.imag(), 1e-52);

    m_engine.finish();
    const BFComplex replacement_center{m_engine.initialize_frame_bf({1e-40, 0.0}, {0.0, 1e-40})};
    float_to_bf(replacement_center.x, 1.5);
    clear_bf(replacement_center.y);
    m_engine.set_glitch_tolerance(2.0);
    int references{1};
    for (const Point2i &point : {Point2i{0, 2}, Point2i{WIDTH - 1, 2}, Point2i{0, 2}})
    {
        EXPECT_EQ(1, m_engine.calculate_pixel(point.x, point.y));
        EXPECT_EQ(++references, m_engine.reference_count());
        EXPECT_EQ(std::complex<double>{}, s_parameter_delta);
    }
}

TEST_F(TestPerturbationPixels, inversionUsesTheSameNearCenterLimitAtBothPrecisions)
{
    init_bf_dec(60);
    m_big_float = true;
    BigStackSaver saved;
    const BFComplex point_bf{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const Inversion inversion{3, 2.0, {}, {2.0, 0.0, 0.0}};

    for (const double value : {0.0, 1e-40, 1.0})
    {
        SCOPED_TRACE(value);
        DComplex point{value, 0.0};
        float_to_bf(point_bf.x, value);
        clear_bf(point_bf.y);

        const double scale{inversion.apply(point)};
        inversion.apply(point_bf);

        EXPECT_DOUBLE_EQ(value == 1.0 ? 2.0 : std::numeric_limits<float>::max(), scale);
        EXPECT_NEAR(point.x, bf_to_float(point_bf.x), 1e-16);
        EXPECT_DOUBLE_EQ(0.0, bf_to_float(point_bf.y));
    }
}

void TestPerturbationPixels::check_pass_switch(const PerturbationMode mode)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_region{
        g_image_region, ImageRegion{{0.25 - 1e-10, -1e-10}, {0.25 + 1e-10, 1e-10}, {0.25 - 1e-10, -1e-10}}};
    init_bf_dec(60);
    m_big_float = true;
    float_to_bf(g_bf_x_min, g_image_region.m_min.x);
    float_to_bf(g_bf_x_max, g_image_region.m_max.x);
    float_to_bf(g_bf_y_min, g_image_region.m_min.y);
    float_to_bf(g_bf_y_max, g_image_region.m_max.y);
    copy_bf(g_bf_x_3rd, g_bf_x_min);
    copy_bf(g_bf_y_3rd, g_bf_y_min);
    g_user.perturbation = mode;
    g_user.perturbation_tolerance = 2.0;
    m_specific.calc_type = calculate_control_pixel;
    m_specific.symmetry = SymmetryType::NONE;
    g_dispatch = FractalDispatch{m_specific};
    const int initial_stack{save_stack()};
    StandardFractal calculation;

    for (const CalcMode pass : {CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS, CalcMode::BOUNDARY_TRACE})
    {
        SCOPED_TRACE(static_cast<char>(pass));
        g_bf_math = BFMathType::BIG_NUM;
        g_std_calc_mode = pass;
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        m_pixels.fill(0);
        m_plot_statuses.clear();
        s_requested_pixels.clear();
        calculation.resume();
        EXPECT_FALSE(calculation.standard_pass_status().perturbation_active);
        EXPECT_EQ(0, calculation.standard_pass_status().perturbation_reference_count);

        for (int increments = 0; increments < 100 && !calculation.done(); ++increments)
        {
            calculation.iterate();
        }

        ASSERT_TRUE(calculation.done());
        EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
        EXPECT_EQ(pass, g_std_calc_mode);
        EXPECT_EQ(BFMathType::BIG_NUM, g_bf_math);
        EXPECT_THAT(m_pixels, Each(7));
        ASSERT_FALSE(m_plot_statuses.empty());
        for (const StandardPassStatus &status : m_plot_statuses)
        {
            EXPECT_EQ(mode == PerturbationMode::AUTO, status.perturbation_active);
        }
        if (mode == PerturbationMode::AUTO)
        {
            EXPECT_TRUE(s_requested_pixels.empty());
            EXPECT_EQ(2, m_plot_statuses.front().perturbation_reference_count);
        }
        else
        {
            EXPECT_FALSE(s_requested_pixels.empty());
            EXPECT_EQ(0, m_plot_statuses.front().perturbation_reference_count);
        }
        EXPECT_FALSE(calculation.standard_pass_status().perturbation_active);
        EXPECT_EQ(0, calculation.standard_pass_status().perturbation_reference_count);
        EXPECT_EQ(initial_stack, save_stack());
        EXPECT_EQ(calculate_control_pixel, g_dispatch.calc_type());
        calculation.finish();
    }
}

TEST_F(TestPerturbationPixels, automaticLogMappingUsesRawCountsAndThenRendersWithPerturbation)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_float_param{g_float_param, &g_init};
    ValueSaver saved_float_bailout{g_bailout_float};
    ValueSaver saved_bn_bailout{g_bailout_bignum};
    ValueSaver saved_bf_bailout{g_bailout_bigfloat};
    ValueSaver saved_bailout_test{g_bailout_test, Bailout::MOD};
    ValueSaver saved_region{g_image_region, ImageRegion{{0.5, 0.0}, {0.5, 0.0}, {0.5, 0.0}}};
    ValueSaver saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver saved_potential{g_potential, Potential{}};
    ValueSaver saved_distance{g_distance_estimator, 0L};
    ValueSaver saved_start_orbit{g_start_show_orbit, false};
    ValueSaver saved_log_table{g_log_map_table, std::vector<Byte>{}};
    ValueSaver saved_log_flag{g_log_map_flag, 2L};
    ValueSaver saved_log_max{g_log_map_table_max_size};
    ValueSaver saved_log_calculate{g_log_map_calculate, false};
    ValueSaver saved_auto_log{g_log_map_auto_calculate, false};
    ValueSaver saved_log_fly{g_log_map_fly_calculate, LogMapCalculate::NONE};
    ValueSaver saved_ranges{g_iteration_ranges, std::vector<int>{}};
    ValueSaver<LDouble> saved_delta_x{g_delta_x, 0};
    ValueSaver<LDouble> saved_delta_y{g_delta_y, 0};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    g_user.perturbation = PerturbationMode::YES;
    set_bailout_formula(Bailout::MOD);
    g_inside_method = ColorMethod::ITER;
    g_colors = 16;
    g_and_color = 15;
    g_max_iterations = 512;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    g_dispatch = FractalDispatch{m_specific};
    bool saw_mapped_sample{};
    bool saw_perturbation{};
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [&](const int col, const int row, const int color)
            {
                m_pixels.at(row * WIDTH + col) = color;
                if (const StandardFractal *calculation{active_standard_fractal()})
                {
                    const bool active{calculation->standard_pass_status().perturbation_active};
                    saw_mapped_sample |= !active && g_real_color_iter == 4 && color == 2;
                    saw_perturbation |= active;
                }
            }));
    alloc_pixel_grid();
    fill_pixel_grid();
    {
        StandardFractal calculation;
        EXPECT_TRUE(calculation.resume().empty());
        EXPECT_EQ(512, g_log_map_table_max_size);
        EXPECT_EQ(2, log_table_calc(4));
        for (int increments = 0; increments < 100 && !calculation.done(); ++increments)
        {
            calculation.iterate();
        }
        EXPECT_TRUE(calculation.done());
        EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
        EXPECT_EQ(4, g_log_map_flag);
        EXPECT_EQ(512, g_max_iterations);
        EXPECT_EQ(4, g_real_color_iter);
        EXPECT_TRUE(saw_mapped_sample);
        EXPECT_TRUE(saw_perturbation);
        EXPECT_THAT(m_pixels, Each(1));
    }
    free_pixel_grid();
}

TEST_F(TestPerturbationPixels, passSwitchAutomaticallySelectsPerturbationAndClearsReferences)
{
    check_pass_switch(PerturbationMode::AUTO);
}

TEST_F(TestPerturbationPixels, passSwitchRespectsDisabledPerturbation)
{
    check_pass_switch(PerturbationMode::NO);
}

class TestSubnormalPerturbation : public TestPerturbationPixels
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_exponent{g_c_exponent, 0};
    ValueSaver<double> m_saved_real_power{g_params[2], -0.001862};
    ValueSaver<double> m_saved_imag_power{g_params[3], 0.0};
    ValueSaver<DComplex *> m_saved_parameter{g_float_param, &g_init};
    ValueSaver<DComplex> m_saved_offset{g_param_z1, DComplex{}};
    ValueSaver<InitOrbitMode> m_saved_initial_orbit{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver<bool> m_saved_bof{g_bof_match_book_images, false};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<decltype(g_bailout_float)> m_saved_float_bailout{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_saved_bn_bailout{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_saved_bf_bailout{g_bailout_bigfloat};
    ValueSaver<Bailout> m_saved_bailout_test{g_bailout_test};
};

void TestSubnormalPerturbation::SetUp()
{
    TestPerturbationPixels::SetUp();
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    m_specific = *get_fractal_specific(g_fractal_type);
    init_bf_dec(60);
    m_big_float = true;
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    g_max_iterations = 4;
    g_magnitude_limit = 16;
    set_bailout_formula(Bailout::MOD);
    str_to_bf(g_bf_x_min, "-4e-324");
    str_to_bf(g_bf_x_max, "4e-324");
    copy_bf(g_bf_x_3rd, g_bf_x_min);
    clear_bf(g_bf_y_min);
    clear_bf(g_bf_y_max);
    clear_bf(g_bf_y_3rd);
    m_engine.initialize_frame_bf({g_bf_x_min, g_bf_y_min}, {g_bf_x_max, g_bf_y_max}, {g_bf_x_3rd, g_bf_y_3rd});
    ASSERT_TRUE(m_engine.initialize_pixel_strategy());
}

TEST_F(TestSubnormalPerturbation, continuationReplacesSubnormalParameterAndSeedDisplacements)
{
    BigStackSaver saved;
    const BFComplex parameter{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    for (const char *coordinate : {"0", "4e-324"})
    {
        SCOPED_TRACE(coordinate);
        str_to_bf(parameter.x, coordinate);
        clear_bf(parameter.y);
        copy_bf(g_param_z_bf.x, parameter.x);
        clear_bf(g_param_z_bf.y);
        copy_bf(g_old_z_bf.x, parameter.x);
        clear_bf(g_old_z_bf.y);
        m_engine.begin_orbit_bf(parameter, parameter, 0);
        long iteration{};
        bool done{};
        for (int step = 0; step < 100 && !done; ++step)
        {
            done = m_engine.iterate_orbit();
            if (m_engine.orbit_iteration() != iteration)
            {
                ASSERT_EQ(iteration + 1, m_engine.orbit_iteration());
                const bool escaped{julia_z_power_orbit_bf() != 0};
                const std::complex<double> expected{
                    static_cast<double>(bf_to_float(g_new_z_bf.x)), static_cast<double>(bf_to_float(g_new_z_bf.y))};
                ++iteration;
                EXPECT_EQ(escaped, m_engine.orbit_escaped());
                EXPECT_EQ(escaped || iteration == g_max_iterations, done);
                EXPECT_NEAR(expected.real(), m_engine.orbit_value().real(), 1e-12);
                EXPECT_NEAR(expected.imag(), m_engine.orbit_value().imag(), 1e-12);
            }
        }
        ASSERT_TRUE(done);
        m_engine.finish_orbit();
    }
    EXPECT_EQ(3, m_engine.reference_count());
}

TEST_F(TestSubnormalPerturbation, displacedPixelMatchesNativeBigFloatOrbit)
{
    copy_bf(g_param_z_bf.x, g_bf_x_max);
    clear_bf(g_param_z_bf.y);
    copy_bf(g_old_z_bf.x, g_bf_x_max);
    clear_bf(g_old_z_bf.y);
    // Native BF conversion also depends on the platform's floating-point range.
    long iteration{};
    bool escaped{};
    // Standard pixels stop before computing the iteration-limit orbit.
    while (!escaped && iteration + 1 < g_max_iterations)
    {
        ++iteration;
        escaped = julia_z_power_orbit_bf() != 0;
    }
    const std::complex<double> expected{
        static_cast<double>(bf_to_float(g_new_z_bf.x)), static_cast<double>(bf_to_float(g_new_z_bf.y))};

    EXPECT_EQ(escaped ? iteration : g_inside_color, m_engine.calculate_pixel(WIDTH - 1, 0));

    EXPECT_EQ(escaped ? iteration : g_max_iterations, g_color_iter);
    EXPECT_NEAR(expected.real(), g_new_z.x, 1e-12);
    EXPECT_NEAR(expected.imag(), g_new_z.y, 1e-12);
    EXPECT_EQ(2, m_engine.reference_count());
}

class TestDeepPerturbationGeometry : public TestPerturbationPixels
{
protected:
    void SetUp() override;
    void TearDown() override;
    void initialize_corners();
    void initialize_frame();

    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-2, 0}, {-2, 0}, {-2, 0}}};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<SymmetryType> m_saved_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, false};
    ValueSaver<long> m_saved_distance{g_distance_estimator, 0};
    ValueSaver<InitOrbitMode> m_saved_initial_orbit{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver<DComplex> m_saved_param{g_param_z1, DComplex{}};
    ValueSaver<double> m_saved_param_real{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_imag{g_params[1], 0.0};
    ValueSaver<DComplex *> m_saved_float_param{g_float_param, &g_init};
    ValueSaver<decltype(g_bailout_float)> m_saved_float_bailout{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_saved_bn_bailout{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_saved_bf_bailout{g_bailout_bigfloat};
    ValueSaver<Bailout> m_saved_bailout_test{g_bailout_test};
};

void TestDeepPerturbationGeometry::SetUp()
{
    TestPerturbationPixels::SetUp();
    {
        const ValueSaver saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
        init_bf_dec(450);
        m_big_float = true;
    }
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    g_max_iterations = 16;
    set_bailout_formula(Bailout::MOD);
    initialize_corners();
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.per_image = mandel_per_image_bf;
    m_specific.per_pixel = mandel_per_pixel_bf;
    m_specific.orbit_calc = julia_orbit_bf;
    m_specific.pert_ref_bf = record_deep_reference_bf;
    g_dispatch = FractalDispatch{m_specific};
    alloc_pixel_grid();
    fill_pixel_grid();
}

void TestDeepPerturbationGeometry::initialize_corners()
{
    {
        BigStackSaver saved;
        const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
        str_to_bf(offset, "3e-400");
        int_to_bf(g_bf_x_min, -2);
        sub_a_bf(g_bf_x_min, offset);
        int_to_bf(g_bf_x_max, -2);
        add_a_bf(g_bf_x_max, offset);
    }
    copy_bf(g_bf_x_3rd, g_bf_x_min);
    clear_bf(g_bf_y_min);
    clear_bf(g_bf_y_max);
    clear_bf(g_bf_y_3rd);
}

void TestDeepPerturbationGeometry::TearDown()
{
    end_disk();
    free_pixel_grid();
    TestPerturbationPixels::TearDown();
}

void TestDeepPerturbationGeometry::initialize_frame()
{
    m_engine.initialize_frame_bf({g_bf_x_min, g_bf_y_min}, {g_bf_x_max, g_bf_y_max}, {g_bf_x_3rd, g_bf_y_3rd});
    ASSERT_TRUE(m_engine.initialize_pixel_strategy());
}

TEST_F(TestDeepPerturbationGeometry, passesMatchNativePixelsAcrossSubDoubleViewport)
{
    // Mandelbrot's production pass setup selects the fixed-point allocator.
    free_bf_vars();
    init_bf_dec(450);
    ASSERT_EQ(BFMathType::BIG_NUM, g_bf_math);
    initialize_corners();
    m_specific.per_image = mandel_per_image_bn;
    m_specific.per_pixel = mandel_per_pixel_bn;
    m_specific.orbit_calc = julia_orbit_bn;
    g_dispatch = FractalDispatch{m_specific};
    for (const CalcMode pass : {CalcMode::BOUNDARY_TRACE, CalcMode::SOLID_GUESS})
    {
        SCOPED_TRACE(static_cast<char>(pass));
        std::array<int, WIDTH * HEIGHT> native_pixels{};
        for (const PerturbationMode mode : {PerturbationMode::NO, PerturbationMode::YES})
        {
            SCOPED_TRACE(static_cast<int>(mode));
            g_std_calc_mode = pass;
            g_user.perturbation = mode;
            g_bf_math = BFMathType::BIG_NUM;
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            m_pixels.fill(0);
            m_plot_statuses.clear();
            s_reference_coordinates.clear();
            StandardFractal calculation;
            ASSERT_TRUE(calculation.resume().empty());
            for (int step = 0; step < 1000 && !calculation.done(); ++step)
            {
                calculation.iterate();
            }
            ASSERT_TRUE(calculation.done());
            EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
            EXPECT_EQ(pass, g_std_calc_mode);
            EXPECT_EQ(BFMathType::BIG_NUM, g_bf_math);
            for (int row = 0; row < HEIGHT; ++row)
            {
                for (int col = 0; col < WIDTH; ++col)
                {
                    EXPECT_EQ(col < WIDTH / 2 ? 1 : 7, m_pixels[row * WIDTH + col]);
                }
            }
            ASSERT_FALSE(m_plot_statuses.empty());
            for (const StandardPassStatus &status : m_plot_statuses)
            {
                EXPECT_EQ(mode == PerturbationMode::YES, status.perturbation_active);
            }
            if (mode == PerturbationMode::NO)
            {
                native_pixels = m_pixels;
            }
            else
            {
                EXPECT_EQ(native_pixels, m_pixels);
                EXPECT_THAT(s_reference_coordinates,
                    Contains(Truly([](const std::complex<double> &point)
                        { return std::abs(point - std::complex<double>{-3.0, 0.0}) < 1e-12; })));
                EXPECT_THAT(s_reference_coordinates,
                    Contains(Truly([](const std::complex<double> &point)
                        { return std::abs(point - std::complex<double>{3.0, 0.0}) < 1e-12; })));
            }
            calculation.finish();
        }
    }
}

TEST_F(TestDeepPerturbationGeometry, frameRetainsPreciseGeometryForReplacementReferences)
{
    str_to_bf(g_bf_y_min, "-3e-400");
    str_to_bf(g_bf_y_max, "3e-400");
    str_to_bf(g_bf_y_3rd, "-1e-400");
    {
        BigStackSaver saved;
        const BigFloat offset{alloc_stack(g_r_bf_length + 2)};
        str_to_bf(offset, "1e-400");
        int_to_bf(g_bf_x_3rd, -2);
        sub_a_bf(g_bf_x_3rd, offset);
    }
    initialize_frame();
    int_to_bf(g_bf_x_min, -2);
    int_to_bf(g_bf_x_max, -2);
    int_to_bf(g_bf_x_3rd, -2);
    clear_bf(g_bf_y_min);
    clear_bf(g_bf_y_max);
    clear_bf(g_bf_y_3rd);

    EXPECT_EQ(1, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, HEIGHT - 1));
    EXPECT_EQ(1, m_engine.calculate_pixel(0, HEIGHT - 1));
    EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, 0));

    EXPECT_EQ(5, m_engine.reference_count());
    EXPECT_THAT(s_reference_coordinates,
        Contains(Truly([](const std::complex<double> &point)
            { return std::abs(point - std::complex<double>{-3.0, 3.0}) < 1e-12; })));
    EXPECT_THAT(s_reference_coordinates,
        Contains(Truly([](const std::complex<double> &point)
            { return std::abs(point - std::complex<double>{3.0, -3.0}) < 1e-12; })));
    EXPECT_THAT(s_reference_coordinates,
        Contains(Truly([](const std::complex<double> &point)
            { return std::abs(point - std::complex<double>{-1.0, -1.0}) < 1e-12; })));
    EXPECT_THAT(s_reference_coordinates,
        Contains(Truly([](const std::complex<double> &point)
            { return std::abs(point - std::complex<double>{1.0, 1.0}) < 1e-12; })));
}

TEST_F(TestDeepPerturbationGeometry, continuationReplacesUnrepresentableParameterAndSeedDisplacements)
{
    initialize_frame();
    BigStackSaver saved;
    const BFComplex parameter{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    const BFComplex value{alloc_stack(g_r_bf_length + 2), alloc_stack(g_r_bf_length + 2)};
    int_to_bf(parameter.x, -2);
    clear_bf(parameter.y);
    copy_bf(value.x, parameter.x);
    clear_bf(value.y);
    m_engine.begin_orbit_bf(parameter, value, 0);
    bool done{};
    for (int step = 0; step < 100 && !done; ++step)
    {
        done = m_engine.iterate_orbit();
    }
    ASSERT_TRUE(done);
    EXPECT_FALSE(m_engine.orbit_escaped());
    EXPECT_EQ(g_max_iterations, m_engine.orbit_iteration());
    m_engine.finish_orbit();
    const int references{m_engine.reference_count()};
    copy_bf(parameter.x, g_bf_x_min);
    copy_bf(value.x, parameter.x);

    m_engine.begin_orbit_bf(parameter, value, 0);
    done = false;
    for (int step = 0; step < 100 && !done; ++step)
    {
        done = m_engine.iterate_orbit();
    }

    EXPECT_TRUE(done);
    EXPECT_TRUE(m_engine.orbit_escaped());
    EXPECT_EQ(1, m_engine.orbit_iteration());
    EXPECT_EQ(references + 1, m_engine.reference_count());
    EXPECT_THAT(s_reference_coordinates,
        Contains(Truly([](const std::complex<double> &point)
            { return std::abs(point - std::complex<double>{-3.0, 0.0}) < 1e-12; })));
    m_engine.finish_orbit();
}

} // namespace id::test
