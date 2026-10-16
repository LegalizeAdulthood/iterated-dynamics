// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include <fractals/julibrot.h>

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/trig_fns.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/parser.h>
#include <io/gif_extensions.h>
#include <io/loadfile.h>
#include <math/arg.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <ostream>
#include <tuple>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{
namespace
{

using FourD = std::array<double, 4>;
constexpr FourD PARAMETERS{0.0625, -0.125, 0.1875, -0.25};
constexpr FourD ZERO{};

MockFunction<int()> *s_orbit_observer{};
MockFunction<void(int, int, int)> *s_plot_observer{};

struct JulibrotVersion
{
    Version version;
    bool legacy;
};

void PrintTo(const JulibrotVersion &value, std::ostream *str)
{
    *str << to_par_string(value.version);
}

struct JulibrotOrbit
{
    FractalType type;
    const char *name;
    bool mandelbrot;
    std::array<int, 4> old_pixels;
    std::array<int, 4> new_pixels;
};

void PrintTo(const JulibrotOrbit &value, std::ostream *str)
{
    *str << value.name;
}

enum class JulibrotVersionSource
{
    PAR_RESET,
    GIF_METADATA
};

void PrintTo(const JulibrotVersionSource value, std::ostream *str)
{
    *str << (value == JulibrotVersionSource::PAR_RESET ? "PAR reset" : "GIF metadata");
}

class TestJulibrot : public TestWithParam<std::tuple<JulibrotVersion, JulibrotOrbit, JulibrotVersionSource>>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void expect_sample(const FourD &sample);
    void observe_orbit();

    MockFunction<int()> m_orbit;
    MockFunction<void(int, int, int)> m_plot;

private:
    std::array<double, MAX_PARAMS> m_saved_params{};
    std::array<Arg, 2> m_stack{};
    ValueSaver<Arg *> m_saved_arg1{g_arg1, &m_stack[1]};
    ValueSaver<Arg *> m_saved_arg2{g_arg2, &m_stack[0]};
    ValueSaver<CompiledFormula> m_saved_formula{g_formula};
    ValueSaver<MockFunction<int()> *> m_saved_orbit_observer{s_orbit_observer, &m_orbit};
    ValueSaver<MockFunction<void(int, int, int)> *> m_saved_plot_observer{s_plot_observer, &m_plot};
    ValueSaver<void (*)(int, int, int)> m_saved_plot{g_plot};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalSpecific *> m_saved_fractal{g_cur_fractal_specific};
    ValueSaver<FractalType> m_saved_orbit_type{g_new_orbit_type};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<DComplex *> m_saved_float_param{g_float_param};
    ValueSaver<DComplex> m_saved_old_z{g_old_z};
    ValueSaver<DComplex> m_saved_new_z{g_new_z};
    ValueSaver<double> m_saved_quaternion_c{g_quaternion_c};
    ValueSaver<double> m_saved_quaternion_ci{g_quaternion_ci};
    ValueSaver<double> m_saved_quaternion_cj{g_quaternion_cj};
    ValueSaver<double> m_saved_quaternion_ck{g_quaternion_ck};
    ValueSaver<double> m_saved_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_saved_sqr_y{g_temp_sqr_y};
    ValueSaver<double> m_saved_magnitude{g_magnitude};
    ValueSaver<double> m_saved_limit{g_magnitude_limit};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations};
    ValueSaver<int> m_saved_colors{g_colors};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_exponent{g_c_exponent};
    ValueSaver<double> m_saved_x_min{g_julibrot_x_min};
    ValueSaver<double> m_saved_x_max{g_julibrot_x_max};
    ValueSaver<double> m_saved_y_min{g_julibrot_y_min};
    ValueSaver<double> m_saved_y_max{g_julibrot_y_max};
    ValueSaver<float> m_saved_origin{g_julibrot_origin};
    ValueSaver<float> m_saved_depth{g_julibrot_depth};
    ValueSaver<float> m_saved_width{g_julibrot_width};
    ValueSaver<float> m_saved_height{g_julibrot_height};
    ValueSaver<float> m_saved_dist{g_julibrot_dist};
    ValueSaver<float> m_saved_eyes{g_eyes};
    ValueSaver<int> m_saved_z_dots{g_julibrot_z_dots};
    ValueSaver<Julibrot3DMode> m_saved_3d_mode{g_julibrot_3d_mode};
    ValueSaver<SaveDAC> m_saved_dac{g_save_dac};
    std::optional<ValueSaver<int (*)()>> m_saved_orbit;
};

void TestJulibrot::SetUp()
{
    std::copy_n(g_params, MAX_PARAMS, m_saved_params.begin());
    save_trig_functions();
    const auto &[version, orbit, source] = GetParam();
    if (source == JulibrotVersionSource::PAR_RESET)
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
    ASSERT_EQ(version.version, g_version);
    set_fractal_type(FractalType::JULIBROT);
    g_new_orbit_type = orbit.type;
    g_logical_screen = LogicalScreen{2, 2, 0, 0, 1.0, 1.0};
    g_image_region = ImageRegion{{-0.375, -0.25}, {0.625, 0.75}, {-0.375, -0.25}};
    g_julibrot_x_min = 0.125;
    g_julibrot_x_max = 0.625;
    g_julibrot_y_min = -0.5;
    g_julibrot_y_max = 0.0;
    g_julibrot_origin = 1.0F;
    g_julibrot_depth = 2.0F;
    g_julibrot_width = 2.0F;
    g_julibrot_height = 2.0F;
    g_julibrot_dist = 2.0F;
    g_eyes = 0.0F;
    g_julibrot_z_dots = 2;
    g_julibrot_3d_mode = Julibrot3DMode::MONOCULAR;
    g_save_dac = SaveDAC::YES;
    g_colors = 256;
    g_max_iterations = 8;
    g_magnitude_limit = 4.0;
    std::copy(PARAMETERS.begin(), PARAMETERS.end(), g_params);
    set_trig_array(0, "sqr");
    g_formula.vars.resize(5); // The square function stores its scratch value here.
    g_plot = [](const int x, const int y, const int color) { s_plot_observer->Call(x, y, color); };
    ASSERT_TRUE(julibrot_per_image());
}

void TestJulibrot::TearDown()
{
    restore_trig_functions();
    std::copy(m_saved_params.begin(), m_saved_params.end(), g_params);
}

void TestJulibrot::observe_orbit()
{
    m_saved_orbit.emplace(get_fractal_specific(g_new_orbit_type)->orbit_calc, [] { return s_orbit_observer->Call(); });
}

void TestJulibrot::expect_sample(const FourD &sample)
{
    const auto &[version, orbit, source] = GetParam();
    const bool modern_mandelbrot = !version.legacy && orbit.mandelbrot;
    const FourD seed = modern_mandelbrot ? ZERO : sample;
    const FourD constants = modern_mandelbrot ? sample : PARAMETERS;
    EXPECT_CALL(m_orbit, Call())
        .WillOnce(
            [seed, constants]
            {
                EXPECT_EQ(seed, (FourD{g_old_z.x, g_old_z.y, g_float_param->x, g_float_param->y}));
                EXPECT_EQ(constants, (FourD{g_quaternion_c, g_quaternion_ci, g_quaternion_cj, g_quaternion_ck}));
                EXPECT_DOUBLE_EQ(seed[0] * seed[0], g_temp_sqr_x);
                EXPECT_DOUBLE_EQ(seed[1] * seed[1], g_temp_sqr_y);
                return 1;
            });
}

TEST_P(TestJulibrot, initializesAllOrbitComponentsAndConstantsAtCompatibleRelease)
{
    observe_orbit();
    g_max_iterations = 1;
    const InSequence sequence;
    expect_sample(FourD{-0.375, 0.25, 0.125, -0.5});
    expect_sample(FourD{-0.625, 0.25, 0.375, -0.25});
    expect_sample(FourD{0.625, 0.25, 0.125, -0.5});
    expect_sample(FourD{0.875, 0.25, 0.375, -0.25});
    EXPECT_CALL(m_plot, Call(_, _, _)).Times(0);
    Standard4D slice;

    EXPECT_TRUE(slice.iterate());
}

TEST_P(TestJulibrot, rendersCompatibleSliceWithRealOrbitComputations)
{
    const auto &[version, orbit, source] = GetParam();
    std::array<int, 4> pixels{};
    EXPECT_CALL(m_plot, Call(_, _, _))
        .WillRepeatedly(
            [&pixels](const int x, const int y, const int color)
            {
                ASSERT_GE(x, 0);
                ASSERT_LT(x, 2);
                ASSERT_GE(y, 0);
                ASSERT_LT(y, 2);
                pixels[y * 2 + x] = color;
            });
    Standard4D slice;

    ASSERT_TRUE(slice.iterate());
    ASSERT_TRUE(slice.iterate());
    EXPECT_FALSE(slice.iterate());

    EXPECT_EQ(version.legacy ? orbit.old_pixels : orbit.new_pixels, pixels);
}

// The two depth samples map to colors 1 and 128; zero is unplotted.
// Golden pixels are row-major for the reference's eight-iteration slice.
INSTANTIATE_TEST_SUITE_P(ResetVersions, TestJulibrot,
    Combine(Values(JulibrotVersion{parse_legacy_version(2002), true},
                JulibrotVersion{parse_legacy_version(2003), false}, JulibrotVersion{current_id_version(), false}),
        Values(JulibrotOrbit{FractalType::QUAT, "quaternion", true, {1, 1, 1, 1}, {1, 1, 1, 0}},
            JulibrotOrbit{FractalType::HYPER_CMPLX, "hypercomplex", true, {1, 1, 1, 0}, {1, 128, 128, 0}},
            JulibrotOrbit{FractalType::QUAT_JUL, "quaternionj", false, {1, 1, 1, 1}, {1, 1, 1, 1}},
            JulibrotOrbit{FractalType::HYPER_CMPLX_J, "hypercomplexj", false, {1, 1, 1, 0}, {1, 1, 1, 0}}),
        Values(JulibrotVersionSource::PAR_RESET, JulibrotVersionSource::GIF_METADATA)));

} // namespace
} // namespace id::test
