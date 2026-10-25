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
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <io/disk_storage.h>
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
using namespace id::geometry;
using namespace id::io;
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
std::vector<std::complex<double>> s_standard_orbit;
std::vector<std::complex<double>> s_perturbation_orbit;

int record_standard_orbit()
{
    s_standard_orbit.emplace_back(g_old_z.x, g_old_z.y);
    return julia_orbit();
}

void record_perturbation_orbit(
    const std::complex<double> &reference, std::complex<double> &delta, const std::complex<double> &parameter_delta)
{
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

struct OrbitState
{
    long iterations;
    DComplex old_z;
    DComplex new_z;
    double magnitude;
    double square_x;
    double square_y;
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

class TestPerturbationIterations : public TestPerturbationPixels, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void initialize_reference(const std::complex<double> &center, double half_width = 0.0, double half_height = 0.0);
    void compare_pixel(const std::complex<double> &center, long max_iterations, double reference_offset = 0.0);
    void set_initial_offset(const DComplex &offset);
    void compare_frame_coordinates(
        double tolerance, const ImageRegion &region = ImageRegion{{-0.3, 0.1}, {0.2, 0.35}, {-0.3, 0.1}});

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
};

void TestPerturbationIterations::SetUp()
{
    TestPerturbationPixels::SetUp();
    set_bailout_formula(Bailout::MOD);
    g_dispatch = FractalDispatch{m_specific};
    alloc_pixel_grid();
}

void TestPerturbationIterations::TearDown()
{
    end_disk();
    free_pixel_grid();
    TestPerturbationPixels::TearDown();
}

void TestPerturbationIterations::initialize_reference(
    const std::complex<double> &center, const double half_width, const double half_height)
{
    if (GetParam())
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

void TestPerturbationIterations::set_initial_offset(const DComplex &offset)
{
    g_param_z1 = offset;
    g_params[0] = offset.x;
    g_params[1] = offset.y;
}

void TestPerturbationIterations::compare_pixel(
    const std::complex<double> &center, const long max_iterations, const double reference_offset)
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
    const int expected_color{standard.calculate_standard_pixel(false)};
    const OrbitState expected{g_real_color_iter, g_old_z, g_new_z, g_magnitude, g_temp_sqr_x, g_temp_sqr_y};
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

    EXPECT_EQ(expected_color, m_engine.calculate_pixel(g_col, g_row));

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
        EXPECT_NEAR(s_standard_orbit[i].real(), s_perturbation_orbit[first + i].real(), 1e-14);
        EXPECT_NEAR(s_standard_orbit[i].imag(), s_perturbation_orbit[first + i].imag(), 1e-14);
    }
}

TEST_P(TestPerturbationIterations, realInitialOffsetMatchesStandardOrbit)
{
    set_initial_offset({0.125, 0.0});

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ((std::complex<double>{0.375, 0.125}), s_perturbation_orbit.front());
}

TEST_P(TestPerturbationIterations, imaginaryInitialOffsetMatchesStandardOrbit)
{
    set_initial_offset({0.0, -0.0625});

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ((std::complex<double>{0.25, 0.0625}), s_perturbation_orbit.front());
}

TEST_P(TestPerturbationIterations, combinedInitialOffsetsDoNotChangeTheRecurrenceParameter)
{
    set_initial_offset({0.125, -0.0625});
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;

    compare_pixel({0.25, 0.125}, 8);

    EXPECT_EQ((std::complex<double>{0.375, 0.0625}), s_perturbation_orbit.front());
    EXPECT_THAT(s_reference_coordinates, Each(Eq(std::complex<double>{0.25, 0.125})));
}

TEST_P(TestPerturbationIterations, bofInitializationMatchesBothStandardCompatibilityPolicies)
{
    set_initial_offset({0.125, -0.0625});
    for (const ColorMethod method : {ColorMethod::BOF60, ColorMethod::BOF61})
    {
        for (const bool book : {false, true})
        {
            SCOPED_TRACE(static_cast<int>(method));
            SCOPED_TRACE(book);
            g_inside_method = method;
            g_bof_match_book_images = book;

            compare_pixel({0.25, 0.125}, 8);

            const std::complex<double> expected{
                book ? std::complex<double>{0.125, -0.0625} : std::complex<double>{0.375, 0.0625}};
            EXPECT_EQ(expected, s_perturbation_orbit.front());
            EXPECT_EQ(book ? 8 : 7, s_perturbation_orbit.size());
        }
    }
}

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

TEST_P(TestPerturbationIterations, initialOffsetsSurviveDisplacementAndReferenceReplacement)
{
    set_initial_offset({0.125, -0.0625});
    for (const bool book : {false, true})
    {
        SCOPED_TRACE(book);
        g_inside_method = book ? ColorMethod::BOF60 : ColorMethod::ITER;
        g_bof_match_book_images = book;
        m_engine.set_glitch_tolerance(0.0);
        compare_pixel({0.25, 0.125}, 8, 0.125);
        EXPECT_EQ(1, m_engine.reference_count());

        m_engine.set_glitch_tolerance(2.0);
        compare_pixel({0.25, 0.125}, 8, 0.125);
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

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

TEST_P(TestPerturbationIterations, fixedInitialOrbitRetainsBofCompatibilityPrecedence)
{
    g_use_init_orbit = InitOrbitMode::VALUE;
    g_init_orbit = {0.25, 0.125};
    set_initial_offset({0.125, -0.0625});
    for (const ColorMethod method : {ColorMethod::BOF60, ColorMethod::BOF61})
    {
        for (const bool book : {false, true})
        {
            SCOPED_TRACE(static_cast<int>(method));
            SCOPED_TRACE(book);
            g_inside_method = method;
            g_bof_match_book_images = book;

            compare_pixel({0.125, 0.0625}, 8);

            const std::complex<double> expected{
                book ? std::complex<double>{0.125, -0.0625} : std::complex<double>{0.375, 0.0625}};
            EXPECT_EQ(expected, s_perturbation_orbit.front());
            EXPECT_EQ(book ? 8 : 7, s_perturbation_orbit.size());
        }
    }
}

TEST_P(TestPerturbationIterations, initialOrbitModesSurviveDisplacementAndReferenceReplacement)
{
    set_initial_offset({0.125, -0.0625});
    for (const InitOrbitMode mode : {InitOrbitMode::VALUE, InitOrbitMode::PIXEL})
    {
        for (const DComplex &initial : {DComplex{}, DComplex{0.25, 0.125}})
        {
            SCOPED_TRACE(static_cast<int>(mode));
            SCOPED_TRACE(initial.x);
            SCOPED_TRACE(initial.y);
            g_use_init_orbit = mode;
            g_init_orbit = initial;
            m_engine.set_glitch_tolerance(0.0);
            compare_pixel({0.25, 0.125}, 8, 0.125);
            EXPECT_EQ(1, m_engine.reference_count());

            m_engine.set_glitch_tolerance(2.0);
            compare_pixel({0.25, 0.125}, 8, 0.125);
            EXPECT_GT(m_engine.reference_count(), 1);
        }
    }
}

TEST_P(TestPerturbationIterations, firstUpdateEscapeMatchesStandardState)
{
    compare_pixel({1.0, 0.0}, 8);
    EXPECT_EQ(1, g_real_color_iter);
}

TEST_P(TestPerturbationIterations, periodicityMatchesStandardSaveSchedules)
{
    for (const bool legacy : {false, true})
    {
        SCOPED_TRACE(legacy);
        g_first_saved_and = legacy ? 1 : 9;
        g_periodicity_next_saved_incr = legacy ? 1 : 4;
        for (const bool reset : {false, true})
        {
            SCOPED_TRACE(reset);
            g_reset_periodicity = reset;
            for (const int periodicity : {0, 1, 5})
            {
                SCOPED_TRACE(periodicity);
                g_periodicity_check = periodicity;
                g_close_enough = std::ldexp(1e-5, -periodicity);
                for (const double real : {0.0, -1.0})
                {
                    SCOPED_TRACE(real);
                    g_old_color_iter = 0;

                    compare_pixel({real, 0.0}, 512);

                    EXPECT_EQ(512, g_real_color_iter);
                    EXPECT_EQ(7, g_color);
                    if (periodicity == 0)
                    {
                        EXPECT_EQ(511, s_standard_orbit.size());
                    }
                    else
                    {
                        EXPECT_LT(s_standard_orbit.size(), 511);
                        if (reset)
                        {
                            EXPECT_GT(s_standard_orbit.size(), 255);
                        }
                    }
                }
            }
        }
    }
}

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

TEST_P(TestPerturbationIterations, periodicityChecksReconstructedOrbitsRatherThanReferencesOrDeltas)
{
    g_periodicity_check = 1;
    g_close_enough = 1e-14;
    m_engine.set_glitch_tolerance(0.0);
    for (const double displacement : {0.0, 0.25})
    {
        SCOPED_TRACE(displacement);
        g_old_color_iter = 0;

        compare_pixel({0.25, 0.0}, 128, displacement);

        EXPECT_EQ(127, s_perturbation_orbit.size());
        EXPECT_EQ(1, m_engine.reference_count());
    }
}

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

TEST_F(TestPerturbationPixels, orbitDependentInsideModesDisablePeriodicity)
{
    g_periodicity_check = 1;
    m_specific.pert_pt = record_perturbation_orbit;
    for (const ColorMethod inside : {ColorMethod::ZMAG, ColorMethod::STAR_TRAIL})
    {
        SCOPED_TRACE(static_cast<int>(inside));
        g_inside_method = inside;
        m_engine.initialize_frame({}, {}, {});
        s_perturbation_orbit.clear();

        EXPECT_GE(m_engine.calculate_pixel(0, 0), 0);

        EXPECT_EQ(g_max_iterations - 1, s_perturbation_orbit.size());
        EXPECT_EQ(g_max_iterations, g_real_color_iter);
    }
}

TEST_F(TestPerturbationPixels, deepPeriodicityUsesTheActivePrecisionTolerance)
{
    ValueSaver saved_mode{g_use_init_orbit, InitOrbitMode::NORMAL};
    ValueSaver saved_real{g_params[0], 0.0};
    ValueSaver saved_imag{g_params[1], 0.0};
    g_periodicity_check = 1;
    g_first_saved_and = 1;
    g_periodicity_next_saved_incr = 1;
    g_max_iterations = 64;
    m_specific.pert_pt = record_perturbation_orbit;
    for (const BFMathType math : {BFMathType::BIG_NUM, BFMathType::BIG_FLT})
    {
        SCOPED_TRACE(static_cast<int>(math));
        for (const double tolerance : {1e-25, 1e-50})
        {
            SCOPED_TRACE(tolerance);
            for (const bool replace_reference : {false, true})
            {
                SCOPED_TRACE(replace_reference);
                m_engine.finish();
                if (m_big_float)
                {
                    free_bf_vars();
                }
                {
                    ValueSaver saved_type{
                        g_fractal_type, math == BFMathType::BIG_NUM ? FractalType::MANDEL : FractalType::JULIA_Z_POWER};
                    init_bf_dec(80);
                }
                m_big_float = true;
                ASSERT_EQ(math, g_bf_math);
                if (math == BFMathType::BIG_NUM)
                {
                    float_to_bn(g_close_enough_bn, tolerance);
                }
                else
                {
                    float_to_bf(g_close_enough_bf, tolerance);
                }
                const BFComplex center{m_engine.initialize_frame_bf({1e-40, 0.0}, {})};
                float_to_bf(center.x, -2.0);
                clear_bf(center.y);
                m_engine.set_glitch_tolerance(replace_reference ? 2.0 : 0.0);
                g_old_color_iter = 0;
                s_perturbation_orbit.clear();

                EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, 0));

                EXPECT_EQ(64, g_real_color_iter);
                EXPECT_EQ((tolerance > 1e-40 ? 3 : 63) + (replace_reference ? 1 : 0), s_perturbation_orbit.size());
                EXPECT_EQ(replace_reference ? 2 : 1, m_engine.reference_count());
                EXPECT_EQ(0, g_old_color_iter);
            }
        }
    }
}

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

TEST_P(TestPerturbationIterations, selectedBailoutMatchesStandardTermination)
{
    struct Sample
    {
        std::complex<double> point;
        std::array<long, 7> iterations;
    };

    const std::array<Sample, 4> samples{{
        {{1.0, 0.0}, {1, 1, 6, 1, 6, 1, 1}},
        {{0.0, 1.0}, {6, 6, 6, 6, 6, 1, 6}},
        {{1.0, -1.0}, {1, 2, 1, 1, 2, 1, 1}},
        {{0.5, 0.5}, {4, 4, 5, 4, 5, 4, 4}},
    }};
    for (const Bailout test :
        {Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND, Bailout::MANH, Bailout::MANR})
    {
        SCOPED_TRACE(static_cast<int>(test));
        g_bailout_test = test;
        set_bailout_formula(test);
        for (const Sample &sample : samples)
        {
            SCOPED_TRACE(sample.point.real());
            SCOPED_TRACE(sample.point.imag());
            for (const double limit : {1.0, 4.0, 16.0})
            {
                SCOPED_TRACE(limit);
                g_magnitude_limit = limit;

                compare_pixel(sample.point, 6);

                if (limit == 4.0)
                {
                    EXPECT_EQ(sample.iterations[static_cast<std::size_t>(test)], g_real_color_iter);
                }
            }
        }
    }
}

TEST_P(TestPerturbationIterations, selectedBailoutSurvivesDisplacementAndReferenceReplacement)
{
    for (const Bailout test :
        {Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND, Bailout::MANH, Bailout::MANR})
    {
        SCOPED_TRACE(static_cast<int>(test));
        g_bailout_test = test;
        set_bailout_formula(test);
        g_magnitude_limit = 0.25;
        m_engine.set_glitch_tolerance(0.0);
        compare_pixel({0.25, 0.125}, 8, 0.125);
        EXPECT_EQ(1, m_engine.reference_count());

        m_engine.set_glitch_tolerance(2.0);
        compare_pixel({0.25, 0.125}, 8, 0.125);
        EXPECT_GT(m_engine.reference_count(), 1);
    }
}

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

void TestPerturbationIterations::compare_frame_coordinates(const double tolerance, const ImageRegion &region)
{
    ValueSaver saved_user{g_user, UserData{}};
    ValueSaver saved_status{g_calc_status};
    ValueSaver saved_resuming{g_resuming, false};
    ValueSaver<LDouble> saved_delta_x{g_delta_x};
    ValueSaver<LDouble> saved_delta_y{g_delta_y};
    ValueSaver<LDouble> saved_delta_x2{g_delta_x2, 0};
    ValueSaver<LDouble> saved_delta_y2{g_delta_y2, 0};
    g_user.perturbation = PerturbationMode::YES;
    g_user.perturbation_tolerance = tolerance;
    g_max_iterations = 2;
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = [] { return true; };
    m_specific.symmetry = SymmetryType::NONE;
    m_specific.pert_ref = record_reference;
    m_specific.pert_ref_bf = record_reference_bf;
    m_specific.pert_pt = record_coordinate;
    std::vector<Point2i> plots;
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke([&plots](const int col, const int row, int) { plots.push_back({col, row}); }));

    for (const Point2i &size : {Point2i{8, 6}, Point2i{6, 8}, Point2i{17, 9}, Point2i{9, 17}})
    {
        for (const FractalType type : {FractalType::MANDEL, FractalType::BURNING_SHIP})
        {
            SCOPED_TRACE(size.x);
            SCOPED_TRACE(size.y);
            SCOPED_TRACE(static_cast<int>(type));
            g_fractal_type = type;
            g_screen_x_dots = size.x;
            g_screen_y_dots = size.y;
            g_logical_screen = {size.x, size.y, 0, 0, size.x - 1.0, size.y - 1.0};
            g_start_pt = g_i_start_pt = g_begin_pt = {};
            g_stop_pt = g_i_stop_pt = {size.x - 1, size.y - 1};
            g_image_region = region;
            g_delta_x = (region.m_max.x - region.m_3rd.x) / (size.x - 1);
            g_delta_y = (region.m_max.y - region.m_3rd.y) / (size.y - 1);
            g_delta_x2 = (region.m_3rd.x - region.m_min.x) / (size.y - 1);
            g_delta_y2 = (region.m_3rd.y - region.m_min.y) / (size.x - 1);
            alloc_pixel_grid();
            fill_pixel_grid();
            if (GetParam())
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
            g_std_calc_mode = CalcMode::ONE_PASS;
            g_calc_status = CalcStatus::PARAMS_CHANGED;
            g_dispatch = FractalDispatch{m_specific};
            s_coordinates.clear();
            s_reference_coordinates.clear();
            plots.clear();
            StandardFractal calculation;
            calculation.resume();
            for (int increments = 0; increments < size.x * size.y + 10 && !calculation.done(); ++increments)
            {
                calculation.iterate();
            }
            ASSERT_TRUE(calculation.done());
            EXPECT_EQ(GetParam(), g_bf_math != BFMathType::NONE);
            ASSERT_FALSE(s_reference_coordinates.empty());
            DComplex expected_center{region.center()};
            if (g_inversion.invert != 0)
            {
                g_inversion.apply(expected_center);
            }
            EXPECT_NEAR(expected_center.x, s_reference_coordinates.front().real(), 1e-14);
            EXPECT_NEAR(type == FractalType::BURNING_SHIP ? -expected_center.y : expected_center.y,
                s_reference_coordinates.front().imag(), 1e-14);
            if (tolerance > 1.0)
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
                EXPECT_NEAR(type == FractalType::BURNING_SHIP ? -expected.y : expected.y, sample.value.imag(), 1e-14);
                ++visits.at(g_row * size.x + g_col);
            }
            EXPECT_THAT(visits, Each(Ge(1)));
            ASSERT_EQ(size.x * size.y, plots.size());
            for (int i = 0; i < size.x * size.y; ++i)
            {
                EXPECT_EQ((Point2i{i % size.x, i / size.x}), plots[i]);
            }
            calculation.finish();
            if (m_big_float)
            {
                free_bf_vars();
                m_big_float = false;
                g_bf_math = BFMathType::NONE;
            }
        }
    }
}

TEST_P(TestPerturbationIterations, fullFrameCoordinatesMatchStandardGrid)
{
    compare_frame_coordinates(1e-6);
}

TEST_P(TestPerturbationIterations, replacementReferencesPreserveStandardGridCoordinates)
{
    compare_frame_coordinates(2.0);
}

TEST_P(TestPerturbationIterations, affineFrameCoordinatesMatchStandardGrid)
{
    struct Transform
    {
        double x_magnification;
        double rotation;
        double skew;
    };

    for (const Transform &transform : {Transform{1.0, 37.0, 0.0}, Transform{1.0, 0.0, 21.0}, Transform{2.5, 0.0, 0.0},
             Transform{0.6, -72.0, -18.0}, Transform{1.0, 90.0, 0.0}})
    {
        SCOPED_TRACE(transform.x_magnification);
        SCOPED_TRACE(transform.rotation);
        SCOPED_TRACE(transform.skew);
        cvt_corners(-0.05, 0.225, 8.0, transform.x_magnification, transform.rotation, transform.skew);
        const ImageRegion region{g_image_region};
        compare_frame_coordinates(1e-6, region);
        compare_frame_coordinates(2.0, region);
    }
}

TEST_P(TestPerturbationIterations, invertedAffineCoordinatesMatchStandardGrid)
{
    g_inversion = {3, 0.5, {2.0, 1.0}, {0.5, 2.0, 1.0}};
    cvt_corners(-0.05, 0.225, 8.0, 0.6, 37.0, 21.0);
    const ImageRegion region{g_image_region};

    compare_frame_coordinates(1e-6, region);
    compare_frame_coordinates(2.0, region);
}

INSTANTIATE_TEST_SUITE_P(ReferencePrecision, TestPerturbationIterations, Bool());

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
        {Bailout::AND, {2.0, 2.0}, false, 1, 4.0},
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
                const bool escaped{sign == 0 || sign == boundary.outward_sign};

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
            for (long i = 0; i < (sign > 0 ? g_max_iterations - 1 : 1); ++i)
            {
                expected_old = static_cast<double>(bf_to_float(z));
                square_bf(square, z);
                add_bf(z, square, center.x);
            }
            expected_new = static_cast<double>(bf_to_float(z));
            if (sign > 0)
            {
                expected_old = expected_new;
            }
        }
        s_reference_coordinates.clear();

        EXPECT_EQ(sign > 0 ? 7 : 1, m_engine.calculate_pixel(0, 0));

        EXPECT_EQ(sign > 0 ? 16 : 1, g_real_color_iter);
        EXPECT_DOUBLE_EQ(expected_new, g_new_z.x);
        EXPECT_DOUBLE_EQ(0.0, g_new_z.y);
        EXPECT_DOUBLE_EQ(expected_old, g_old_z.x);
        EXPECT_DOUBLE_EQ(expected_new * expected_new, g_magnitude);
        EXPECT_EQ(1, m_engine.reference_count());
        EXPECT_EQ(g_max_iterations + (sign > 0 ? g_max_iterations - 1 : 1), s_reference_coordinates.size());
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

TEST_F(TestPerturbationPixels, passSwitchAutomaticallySelectsPerturbationAndClearsReferences)
{
    check_pass_switch(PerturbationMode::AUTO);
}

TEST_F(TestPerturbationPixels, passSwitchRespectsDisabledPerturbation)
{
    check_pass_switch(PerturbationMode::NO);
}

} // namespace id::test
