// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/Ant.h>
#include <fractals/Cellular.h>
#include <fractals/Plasma.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
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

using namespace id::engine;
using namespace id::fractals;
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

void ignore_plot(int, int, int)
{
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

} // namespace id::test
