// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/Ant.h>
#include <fractals/Cellular.h>
#include <fractals/Plasma.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/ifs.h>
#include <fractals/lorenz.h>
#include <fractals/lyapunov.h>
#include <fractals/population.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/Potential.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <engine/VideoInfo.h>
#include <geometry/3d.h>
#include <geometry/line3d.h>
#include <geometry/plot3d.h>
#include <io/loadmap.h>
#include <math/cmplx.h>
#include <math/Point.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include "MockDriver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
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

namespace id::test
{
namespace
{

constexpr int SEED{4567};
constexpr int FIRST_RANDOM15{14952};
constexpr int LOW_RANDOM_SEED{1};
constexpr int HIGH_RANDOM_SEED{10000};

std::vector<int> s_plotted_colors;

void ignore_plot(int, int, int)
{
}

void capture_plot(int, int, int color)
{
    s_plotted_colors.push_back(color);
}

int no_orbit()
{
    return 0;
}

class FractalParamSaver
{
public:
    FractalParamSaver()
    {
        std::copy(&g_params[0], &g_params[MAX_PARAMS], m_params.begin());
        m_param_text = g_param_text;
    }

    ~FractalParamSaver()
    {
        std::copy(m_params.begin(), m_params.end(), &g_params[0]);
        g_param_text = m_param_text;
    }

    FractalParamSaver(const FractalParamSaver &) = delete;
    FractalParamSaver(FractalParamSaver &&) = delete;
    FractalParamSaver &operator=(const FractalParamSaver &) = delete;
    FractalParamSaver &operator=(FractalParamSaver &&) = delete;

private:
    std::array<double, MAX_PARAMS> m_params{};
    std::array<std::string, MAX_PARAMS> m_param_text;
};

class MinimalImageState
{
public:
    MinimalImageState()
    {
        set_null_video();
        set_normal_span();
    }

    MinimalImageState(const MinimalImageState &) = delete;
    MinimalImageState(MinimalImageState &&) = delete;
    MinimalImageState &operator=(const MinimalImageState &) = delete;
    MinimalImageState &operator=(MinimalImageState &&) = delete;

private:
    ValueSaver<LogicalScreen> m_saved_logical_screen{
        g_logical_screen, LogicalScreen{8, 8, 0, 0, 7.0, 7.0}};
    ValueSaver<Point2i> m_saved_i_stop_pt{g_i_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_stop_pt{g_stop_pt, Point2i{7, 7}};
    ValueSaver<int> m_saved_screen_x_dots{g_screen_x_dots, 8};
    ValueSaver<int> m_saved_screen_y_dots{g_screen_y_dots, 8};
    ValueSaver<int> m_saved_colors{g_colors, 2};
    ValueSaver<int> m_saved_and_color{g_and_color, 255};
    ValueSaver<void (*)(int, int, int)> m_saved_put_color{g_put_color, ignore_plot};
    ValueSaver<void (*)(int, int, int)> m_saved_plot{g_plot, ignore_plot};
    ValueSaver<ColorMethod> m_saved_inside_method{g_inside_method, ColorMethod::COLOR};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 1};
    ValueSaver<ColorMethod> m_saved_outside_method{g_outside_method, ColorMethod::COLOR};
    ValueSaver<int> m_saved_outside_color{g_outside_color, 1};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_map_specified{g_map_specified, false};
    ValueSaver<bool> m_saved_colors_preloaded{g_colors_preloaded, false};
    ValueSaver<ColorState> m_saved_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<DComplex> m_saved_param_z1{g_param_z1, DComplex{50.0, 0.0}};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
};

class LyapunovState
{
public:
    LyapunovState() :
        m_lyapunov(get_fractal_specific(FractalType::LYAPUNOV)),
        m_saved_cur_fractal_specific(g_cur_fractal_specific, m_lyapunov),
        m_saved_orbit_calc(m_lyapunov->orbit_calc, no_orbit)
    {
        g_params[0] = 65.0;
        g_params[2] = 0.0;
        g_max_iterations = 2;
        g_logical_screen = LogicalScreen{2, 2, 0, 0, 1.0, 1.0};
        g_i_stop_pt = Point2i{1, 1};
        g_stop_pt = Point2i{1, 1};
        g_screen_x_dots = 2;
        g_screen_y_dots = 2;
        g_col = 0;
        g_row = 0;
        g_image_region.m_min = DComplex{0.0, 0.0};
        g_image_region.m_max = DComplex{1.0, 1.0};
        g_image_region.m_3rd = DComplex{0.0, 0.0};
        g_delta_x = 1.0;
        g_delta_y = 1.0;
        g_delta_x2 = 0.0;
        g_delta_y2 = 0.0;
        alloc_pixel_grid();
        fill_pixel_grid();
    }

    ~LyapunovState()
    {
        free_pixel_grid();
    }

    LyapunovState(const LyapunovState &) = delete;
    LyapunovState(LyapunovState &&) = delete;
    LyapunovState &operator=(const LyapunovState &) = delete;
    LyapunovState &operator=(LyapunovState &&) = delete;

    MockDriver &driver()
    {
        return m_driver;
    }

private:
    MockDriver m_driver;
    FractalParamSaver m_saved_params;
    MinimalImageState m_image;
    FractalSpecific *m_lyapunov;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<FractalSpecific *> m_saved_cur_fractal_specific;
    ValueSaver<int (*)(void)> m_saved_orbit_calc;
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<ImageRegion> m_saved_image_region{g_image_region};
    ValueSaver<LDouble> m_saved_delta_x{g_delta_x};
    ValueSaver<LDouble> m_saved_delta_y{g_delta_y};
    ValueSaver<LDouble> m_saved_delta_x2{g_delta_x2};
    ValueSaver<LDouble> m_saved_delta_y2{g_delta_y2};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
};

class IFSState
{
public:
    IFSState()
    {
        g_logical_screen = LogicalScreen{16, 16, 0, 0, 15.0, 15.0};
        g_i_stop_pt = Point2i{15, 15};
        g_stop_pt = Point2i{15, 15};
        g_screen_x_dots = 16;
        g_screen_y_dots = 16;
        g_colors = 16;
        g_params[0] = 1.0;
        g_max_iterations = 1;
        g_orbit_save_flags = 0;
        g_image_region.m_min = DComplex{-1.0, -1.0};
        g_image_region.m_max = DComplex{1.0, 1.0};
        g_image_region.m_3rd = DComplex{-1.0, -1.0};
        g_ifs_definition = {
            0.0F, 0.0F, 0.0F, 0.0F, -0.5F, -0.5F, 0.5F,
            0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.5F};
        g_num_affine_transforms = 2;
        s_plotted_colors.clear();
    }

    ~IFSState()
    {
        s_plotted_colors.clear();
    }

    IFSState(const IFSState &) = delete;
    IFSState(IFSState &&) = delete;
    IFSState &operator=(const IFSState &) = delete;
    IFSState &operator=(IFSState &&) = delete;

private:
    FractalParamSaver m_saved_params;
    MinimalImageState m_image;
    ValueSaver<void (*)(int, int, int)> m_saved_plot{g_plot, capture_plot};
    ValueSaver<ImageRegion> m_saved_image_region{g_image_region};
    ValueSaver<std::vector<float>> m_saved_ifs_definition{g_ifs_definition};
    ValueSaver<int> m_saved_num_affine_transforms{g_num_affine_transforms};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations};
    ValueSaver<int> m_saved_orbit_save_flags{g_orbit_save_flags};
    ValueSaver<int> m_saved_x_rot{g_x_rot, 0};
    ValueSaver<int> m_saved_y_rot{g_y_rot, 0};
    ValueSaver<int> m_saved_z_rot{g_z_rot, 0};
    ValueSaver<int> m_saved_viewer_z{g_viewer_z, 0};
    ValueSaver<int> m_saved_x_shift{g_x_shift, 0};
    ValueSaver<int> m_saved_x_shift1{g_x_shift1, 0};
    ValueSaver<int> m_saved_xx_adjust{g_xx_adjust, 0};
    ValueSaver<int> m_saved_xx_adjust1{g_xx_adjust1, 0};
    ValueSaver<int> m_saved_yy_adjust{g_yy_adjust, 0};
    ValueSaver<int> m_saved_yy_adjust1{g_yy_adjust1, 0};
    ValueSaver<GlassesType> m_saved_glasses_type{g_glasses_type, GlassesType::NONE};
    ValueSaver<StereoImage> m_saved_which_image{g_which_image, StereoImage::NONE};
    ValueSaver<Display3DMode> m_saved_display_3d{g_display_3d, Display3DMode::NONE};
};

void clear_params()
{
    std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
    g_param_text.fill({});
}

double lyapunov_population_from_seed(const int seed)
{
    LyapunovState state;
    ValueSaver saved_random_seed{g_random_seed, seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};
    ValueSaver saved_population{g_population, 0.25};

    g_params[1] = 1.0;
    EXPECT_CALL(state.driver(), key_pressed()).WillOnce(testing::Return(0));

    EXPECT_TRUE(lyapunov_per_image());
    lyapunov_type();

    return g_population;
}

std::vector<int> ifs2d_colors_from_seed(const int seed)
{
    IFSState state;
    ValueSaver saved_random_seed{g_random_seed, seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    IFS2D ifs;
    ifs.iterate();

    return s_plotted_colors;
}

std::vector<int> ifs3d_colors_from_seed(const int seed)
{
    IFSState state;
    ValueSaver saved_random_seed{g_random_seed, seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    g_ifs_definition = {
        0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, -0.5F, -0.5F, 0.0F, 0.5F,
        0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 0.5F, 0.0F, 0.5F};

    IFS3D ifs;
    for (int i = 0; i < 120; ++i)
    {
        ifs.iterate();
    }

    return s_plotted_colors;
}

} // namespace

TEST(TestSeededFractals, antReuseLastRandomPreservesGeneratedSeed)
{
    FractalParamSaver saved_params;
    MinimalImageState image;
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, false};

    clear_params();
    g_params[0] = 1100.0;
    g_params[1] = 100.0;
    g_params[2] = 1.0;
    g_params[3] = 1.0;
    g_params[5] = 1.0;

    Ant ant;

    EXPECT_EQ(SEED, g_random_seed);
}

TEST(TestSeededFractals, cellularReuseLastRandomPreservesGeneratedSeed)
{
    FractalParamSaver saved_params;
    MinimalImageState image;
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, false};

    clear_params();
    g_params[0] = -1.0;
    g_params[1] = 1010.0;
    g_params[2] = 21.0;

    Cellular cellular;

    EXPECT_EQ(SEED, g_random_seed);
}

TEST(TestSeededFractals, plasmaReuseLastRandomPreservesGeneratedSeed)
{
    FractalParamSaver saved_params;
    MinimalImageState image;
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, false};

    clear_params();
    g_params[0] = 1.0;
    g_params[2] = 1.0;

    Plasma plasma;

    EXPECT_EQ(SEED, g_random_seed);
}

TEST(TestSeededFractals, lyapunovRandomPopulationRepeatsWithFixedSeed)
{
    EXPECT_EQ(lyapunov_population_from_seed(SEED), lyapunov_population_from_seed(SEED));
}

TEST(TestSeededFractals, lyapunovRandomPopulationChangesWithFixedSeed)
{
    EXPECT_NE(lyapunov_population_from_seed(SEED), lyapunov_population_from_seed(SEED + 1));
}

TEST(TestSeededFractals, lyapunovExplicitPopulationDoesNotConsumeImageRng)
{
    LyapunovState state;
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};
    ValueSaver saved_population{g_population, 0.0};

    set_random_seed(SEED);
    g_params[1] = 0.25;
    EXPECT_CALL(state.driver(), key_pressed()).WillOnce(testing::Return(0));

    EXPECT_TRUE(lyapunov_per_image());
    lyapunov_type();

    EXPECT_EQ(0.25, g_population);
    EXPECT_EQ(FIRST_RANDOM15, random15());
}

TEST(TestSeededFractals, ifs2dRepeatsWithFixedSeed)
{
    const std::vector<int> first{ifs2d_colors_from_seed(LOW_RANDOM_SEED)};
    const std::vector<int> second{ifs2d_colors_from_seed(LOW_RANDOM_SEED)};

    ASSERT_FALSE(first.empty());
    EXPECT_EQ(first, second);
}

TEST(TestSeededFractals, ifs2dChangesWithFixedSeed)
{
    const std::vector<int> first{ifs2d_colors_from_seed(LOW_RANDOM_SEED)};
    const std::vector<int> second{ifs2d_colors_from_seed(HIGH_RANDOM_SEED)};

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    EXPECT_NE(first, second);
}

TEST(TestSeededFractals, ifs3dRepeatsWithFixedSeed)
{
    const std::vector<int> first{ifs3d_colors_from_seed(LOW_RANDOM_SEED)};
    const std::vector<int> second{ifs3d_colors_from_seed(LOW_RANDOM_SEED)};

    ASSERT_FALSE(first.empty());
    EXPECT_EQ(first, second);
}

TEST(TestSeededFractals, ifs3dChangesWithFixedSeed)
{
    const std::vector<int> first{ifs3d_colors_from_seed(LOW_RANDOM_SEED)};
    const std::vector<int> second{ifs3d_colors_from_seed(HIGH_RANDOM_SEED)};

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    EXPECT_NE(first, second);
}

TEST(TestSeededFractals, inverseJuliaRandomWalkSeedsImageRng)
{
    FractalParamSaver saved_params;
    MinimalImageState image;
    ValueSaver saved_fractal_type{g_fractal_type, FractalType::INVERSE_JULIA};
    ValueSaver saved_major_method{g_major_method, Major::RANDOM_WALK};
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    clear_params();
    set_random_seed(LOW_RANDOM_SEED);

    EXPECT_TRUE(orbit3d_per_image());

    EXPECT_EQ(FIRST_RANDOM15, random15());
}

} // namespace id::test
