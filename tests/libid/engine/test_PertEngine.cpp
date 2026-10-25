// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/PertEngine.h>

#include "MockDriver.h"

#include <engine/BoundaryTrace.h>
#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

TEST(TestPertEngine, defaultGlitchToleranceScalesThreshold)
{
    PertEngine engine;

    EXPECT_DOUBLE_EQ(25.0e-12, engine.glitch_tolerance_threshold({3.0, 4.0}));
}

TEST(TestPertEngine, nonDefaultGlitchToleranceScalesThreshold)
{
    PertEngine engine;

    engine.set_glitch_tolerance(0.5);

    EXPECT_DOUBLE_EQ(6.25, engine.glitch_tolerance_threshold({3.0, 4.0}));
}

TEST(TestPertEngine, referenceSampleCountIncludesTheInitialValue)
{
    EXPECT_EQ(129, PertEngine::reference_sample_count(128));
}

TEST(TestPertEngine, referenceSampleCountRejectsNonpositiveIterations)
{
    EXPECT_THROW(PertEngine::reference_sample_count(0), std::logic_error);
    EXPECT_THROW(PertEngine::reference_sample_count(-1), std::logic_error);
    EXPECT_THROW(PertEngine::reference_sample_count(std::numeric_limits<long>::min()), std::logic_error);
}

TEST(TestPertEngine, referenceSampleCountHandlesTheWindowsLongMaximumWithoutAllocating)
{
    constexpr long WINDOWS_LONG_MAX{std::numeric_limits<std::int32_t>::max()};
    if constexpr (sizeof(std::size_t) > sizeof(std::int32_t))
    {
        EXPECT_EQ(2147483648ULL, PertEngine::reference_sample_count(WINDOWS_LONG_MAX));
    }
    else
    {
        EXPECT_THROW(PertEngine::reference_sample_count(WINDOWS_LONG_MAX), std::length_error);
    }
}

TEST(TestPertEngine, referenceSampleCountChecksContainerAndCombinedByteLimits)
{
    constexpr std::size_t BYTES_PER_SAMPLE{sizeof(std::complex<double>) + sizeof(double)};
    const std::uintmax_t max_samples{std::min({std::vector<std::complex<double>>{}.max_size(),
        std::vector<double>{}.max_size(), std::numeric_limits<std::size_t>::max() / BYTES_PER_SAMPLE})};
    const std::uintmax_t max_iterations{std::numeric_limits<long>::max()};
    const long largest_supported{static_cast<long>(std::min(max_iterations, max_samples - 1))};

    EXPECT_EQ(
        static_cast<std::uintmax_t>(largest_supported) + 1, PertEngine::reference_sample_count(largest_supported));
    if (max_samples <= max_iterations)
    {
        EXPECT_THROW(PertEngine::reference_sample_count(static_cast<long>(max_samples)), std::length_error);
        EXPECT_THROW(PertEngine::reference_sample_count(std::numeric_limits<long>::max()), std::length_error);
    }
}

namespace
{

constexpr int WIDTH{8};
constexpr int HEIGHT{6};
PertEngine *s_pixel_engine{};
std::vector<Point2i> s_requested_pixels;
std::vector<std::complex<double>> s_reference_coordinates;
std::complex<double> s_parameter_delta;

void fail_reference_bf(const BFComplex &, BFComplex &)
{
    throw std::bad_array_new_length{};
}

void record_reference(const std::complex<double> &center, std::complex<double> &z)
{
    s_reference_coordinates.push_back(center);
    mandel_ref_pt(center, z);
}

void record_reference_bf(const BFComplex &center, BFComplex &z)
{
    s_reference_coordinates.emplace_back(bf_to_float(center.x), bf_to_float(center.y));
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
};

void TestPerturbationPixels::SetUp()
{
    s_requested_pixels.clear();
    s_reference_coordinates.clear();
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
    const BFComplex center{m_engine.initialize_frame_bf(1e-40)};
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
        const BFComplex center{m_engine.initialize_frame_bf(1e-40)};
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
    const BFComplex center{m_engine.initialize_frame_bf(1e-40)};
    float_to_bf(center.x, 0.25);
    clear_bf(center.y);
    m_specific.pert_ref_bf = fail_reference_bf;

    EXPECT_THROW(m_engine.initialize_pixel_strategy(), std::bad_array_new_length);

    EXPECT_EQ(initial_stack, save_stack());
    EXPECT_EQ(0, m_engine.reference_count());
    m_engine.finish();
    EXPECT_EQ(initial_stack, save_stack());

    m_specific.pert_ref_bf = mandel_ref_pt_bf;
    const BFComplex next_center{m_engine.initialize_frame_bf(1e-40)};
    float_to_bf(next_center.x, 0.25);
    clear_bf(next_center.y);
    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(1, m_engine.reference_count());
}

TEST_F(TestPerturbationPixels, failedReferenceComputesTheRequestedPixelImmediately)
{
    m_engine.initialize_frame({1.0, 0.0}, 0.75);

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
    m_engine.initialize_frame({0.25, 0.0}, 1e-200);
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
    m_engine.initialize_frame({-0.1, 0.1}, 1e-10);
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
    control.initialize_frame(center, ZOOM_RADIUS);
    control.set_glitch_tolerance(2.0);
    m_engine.initialize_frame(center, ZOOM_RADIUS);

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
    m_engine.initialize_frame({0.25, 0.0}, 1e-10);
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
    m_engine.initialize_frame({2.0, 0.0}, 1.0);
    m_engine.set_glitch_tolerance(0.0);
    const std::complex<double> offset{8.0 / 7.0, -0.3};
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
    m_engine.initialize_frame({2.0, 0.0}, 1.0);
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
    m_engine.initialize_frame({2.0, 0.0}, 1.0);
    m_engine.set_glitch_tolerance(0.0);
    const std::complex<double> offset{8.0 / 7.0, -0.3};
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
    const BFComplex center{m_engine.initialize_frame_bf(1e-40)};
    float_to_bf(center.x, 1.5);
    clear_bf(center.y);

    EXPECT_EQ(1, m_engine.calculate_pixel(0, 2));

    ASSERT_FALSE(s_reference_coordinates.empty());
    EXPECT_NEAR(3.0, s_reference_coordinates.front().real(), 1e-14);
    EXPECT_NEAR(32e-40 / 7.0, s_parameter_delta.real(), 1e-52);
    EXPECT_NEAR(0.0, s_parameter_delta.imag(), 1e-52);

    m_engine.finish();
    const BFComplex replacement_center{m_engine.initialize_frame_bf(1e-40)};
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

TEST_F(TestPerturbationPixels, passSwitchAutomaticallySelectsPerturbationAndClearsReferences)
{
    check_pass_switch(PerturbationMode::AUTO);
}

TEST_F(TestPerturbationPixels, passSwitchRespectsDisabledPerturbation)
{
    check_pass_switch(PerturbationMode::NO);
}

} // namespace id::test
