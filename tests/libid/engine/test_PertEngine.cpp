// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/PertEngine.h>

#include "MockDriver.h"

#include <engine/BoundaryTrace.h>
#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
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

#include <array>
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

namespace
{

constexpr int WIDTH{8};
constexpr int HEIGHT{6};
PertEngine *s_pixel_engine{};
std::vector<Point2i> s_requested_pixels;

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
    set_normal_dot();
    set_normal_span();
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

TEST_F(TestPerturbationPixels, reusesTheClosestReferenceAtTinyZoomScales)
{
    m_engine.initialize_frame({0.25, 0.0}, 1e-200);
    m_engine.set_glitch_tolerance(2.0);
    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));
    EXPECT_EQ(2, m_engine.reference_count());
    EXPECT_EQ(7, m_engine.calculate_pixel(WIDTH - 1, HEIGHT - 1));
    EXPECT_EQ(3, m_engine.reference_count());

    EXPECT_EQ(7, m_engine.calculate_pixel(0, 0));

    EXPECT_EQ(3, m_engine.reference_count());
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
    bool done{};

    for (int increments = 0; increments < 100 && !done; ++increments)
    {
        done = trace.iterate();
    }

    ASSERT_TRUE(done);
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
