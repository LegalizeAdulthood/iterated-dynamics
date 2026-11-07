// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <MockDriver.h>

#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/OrbitPlot.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/engine_timer.h>
#include <engine/fractals.h>
#include <engine/pixel.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <fractals/fractalp.h>
#include <fractals/ifs.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <geometry/stereo_params.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <array>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

namespace id::test::non_standard
{

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

extern int g_orbit_calls;

struct BifurcationCase
{
    const char *name;
    FractalType type;
};

void PrintTo(const BifurcationCase &value, std::ostream *out);

constexpr BifurcationCase BIFURCATION_CASES[]{
    {"Verhulst", FractalType::BIFURCATION},
    {"Lambda", FractalType::BIF_LAMBDA},
    {"PlusSinPi", FractalType::BIF_PLUS_SIN_PI},
    {"EqualsSinPi", FractalType::BIF_EQ_SIN_PI},
    {"Stewart", FractalType::BIF_STEWART},
    {"May", FractalType::BIF_MAY},
};

class TestNonStandardFractal : public Test
{
protected:
    void SetUp() override;

    void TearDown() override;

    void select_type(FractalType type);

    void complete(NonStandardFractal &calculation);

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

class TestSelectedIFS : public TestNonStandardFractal
{
protected:
    void SetUp() override;

    void TearDown() override;

    void select_dimension(IFSDimension dimension);

    void complete_view(NonStandardFractal &calculation);

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

class TestStandaloneBifurcation : public TestNonStandardFractal, public WithParamInterface<BifurcationCase>
{
protected:
    void SetUp() override;
};

class TestStandalonePopcorn : public TestNonStandardFractal, public WithParamInterface<FractalType>
{
protected:
    void SetUp() override;

    void TearDown() override;

    void complete(NonStandardFractal &calculation);

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

class TestStandaloneDiffusion : public TestNonStandardFractal, public WithParamInterface<int>
{
protected:
    void SetUp() override;

    void complete(NonStandardFractal &calculation);

    static constexpr int SIDE{32};
    static constexpr int MAX_CHUNKS{100000};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag, true};
    ValueSaver<int> m_saved_random_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_start_show_orbit{g_start_show_orbit, false};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color};
    std::array<int, SIDE * SIDE> m_frame{};
};

class TestStandaloneTestPoint : public TestNonStandardFractal, public WithParamInterface<CalcMode>
{
protected:
    void SetUp() override;

    void TearDown() override;

    void complete(NonStandardFractal &calculation);

    static constexpr int MAX_CHUNKS{2 * (WIDTH + 1) * (HEIGHT + 1)};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode};
    ValueSaver<LDouble> m_saved_dx2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_dy2{g_delta_y2, 0};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 7};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot};
};

class SelectedAntFixture : public TestNonStandardFractal
{
protected:
    void initialize_ant(AntType type);

    std::vector<std::array<int, WIDTH * HEIGHT>> reference_frames();

    ValueSaver<double> m_saved_param3{g_params[3]};
    ValueSaver<double> m_saved_param4{g_params[4]};
    ValueSaver<double> m_saved_param5{g_params[5]};
    ValueSaver<std::string> m_saved_rule{g_param_text[0]};
    ValueSaver<int> m_saved_seed{g_random_seed, 12345};
    ValueSaver<bool> m_saved_fixed_seed{g_random_seed_flag, true};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
};

class TestSelectedAnt : public SelectedAntFixture, public WithParamInterface<AntType>
{
protected:
    void SetUp() override;
};

} // namespace id::test::non_standard
