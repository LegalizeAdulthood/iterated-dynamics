// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/fractalb.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_grid.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/sqr.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <limits>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

struct BigFloatBailoutInput
{
    std::string name;
    Bailout test;
    DComplex point;
    double metric;
    double limit;
};

void PrintTo(const BigFloatBailoutInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<BigFloatBailoutInput> inputs()
{
    std::vector<BigFloatBailoutInput> result;
    const std::array<const char *, 7> names{"Mod", "Real", "Imag", "Or", "And", "Manh", "Manr"};
    const std::array<DComplex, 5> points{{{1.5, -1}, {-0.5, 1.25}, {0, 0}, {-0.5, -1.25}, {2, 0}}};
    const std::array<std::array<double, 7>, 5> metrics{
        {{3.25, 2.25, 1, 2.25, 1, 6.25, 0.25}, {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 0.5625},
            {0, 0, 0, 0, 0, 0, 0}, {1.8125, 0.25, 1.5625, 1.5625, 0.25, 3.0625, 3.0625}, {4, 4, 0, 4, 0, 4, 4}}};
    for (int point = 0; point < 5; ++point)
    {
        for (int test = 0; test < 7; ++test)
        {
            for (int offset = -1; offset <= 1; ++offset)
            {
                const double metric = metrics[point][test];
                result.push_back(
                    {std::string{names[test]} + "_" + std::to_string(point) + "_" + std::to_string(offset + 1),
                        static_cast<Bailout>(test), points[point], metric, metric + offset * 0.125});
            }
        }
    }
    return result;
}


struct JuliaZPowerInput
{
    const char *name;
    std::complex<double> exponent;
    SymmetryType symmetry;
};

void PrintTo(const JuliaZPowerInput &input, std::ostream *out)
{
    *out << input.name;
}

const std::array<JuliaZPowerInput, 8> JULIA_Z_POWERS{
    {{"Even", {2, 0}, SymmetryType::ORIGIN}, {"Odd", {3, 0}, SymmetryType::NONE},
        {"NegativeEven", {-2, 0}, SymmetryType::ORIGIN}, {"NegativeOdd", {-3, 0}, SymmetryType::NONE},
        {"Fractional", {0.5, 0}, SymmetryType::NONE}, {"Complex", {0.5, 0.25}, SymmetryType::NONE},
        {"Imaginary", {0, -0.25}, SymmetryType::NONE}, {"Zero", {0, 0}, SymmetryType::ORIGIN}}};

} // namespace

class TestBigFloatBailout : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<FractalType> m_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test, Bailout::MOD};
    ValueSaver<double> m_limit{g_magnitude_limit};
    ValueSaver<double> m_magnitude{g_magnitude, -17};
    ValueSaver<decltype(g_bailout_float)> m_fp{g_bailout_float};
    ValueSaver<decltype(g_bailout_bignum)> m_bn{g_bailout_bignum};
    ValueSaver<decltype(g_bailout_bigfloat)> m_bf{g_bailout_bigfloat};
};

void TestBigFloatBailout::SetUp()
{
    init_bf_dec(40);
}

void TestBigFloatBailout::TearDown()
{
    free_bf_vars();
}

class TestBigFloatBailoutCases : public TestBigFloatBailout, public testing::WithParamInterface<BigFloatBailoutInput>
{
};

TEST_F(TestBigFloatBailout, canonicalGlobalsOwnWorkingStorageThroughBindingAndCleanup)
{
    static_assert(std::is_same_v<decltype(g_old_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_new_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_saved_z_bf), Complex<BigFloat>>);
    static_assert(std::is_same_v<decltype(g_close_enough_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_tmp_sqr_x_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_tmp_sqr_y_bf), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp), BigFloat>);
    static_assert(std::is_same_v<decltype(g_param_z_bf), Complex<BigFloat>>);
    const std::array<BigFloat *, 16> values{&g_old_z_bf.x, &g_old_z_bf.y, &g_new_z_bf.x, &g_new_z_bf.y, &g_saved_z_bf.x,
        &g_saved_z_bf.y, &g_close_enough_bf, &g_tmp_sqr_x_bf, &g_tmp_sqr_y_bf, &g_bf_tmp, &g_delta_x_bf, &g_delta_y_bf,
        &g_delta2_x_bf, &g_delta2_y_bf, &g_param_z_bf.x, &g_param_z_bf.y};
    BigArena &working = bf_arena(BigLifetime::WORKING);
    for (BigFloat *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
        *value = -1.25L;
    }

    const std::size_t high_water = working.high_water_mark();
    init_bf_dec(40);
    EXPECT_EQ(high_water, working.high_water_mark());
    for (const BigFloat *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&working, &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
    }
    init_bf_dec(80);
    for (const BigFloat *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
    }
    free_bf_vars();
    for (const BigFloat *value : values)
    {
        EXPECT_FALSE(*value);
    }
    init_bf_dec(40);
    for (const BigFloat *value : values)
    {
        ASSERT_TRUE(*value);
        EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &value->arena());
        EXPECT_EQ(BigFloat{0}, *value);
    }
}

TEST_P(TestBigFloatBailoutCases, preservesInclusiveThresholdCachesAndOldOrbit)
{
    const auto &input = GetParam();
    const bool escaped = input.metric >= input.limit;
    const bool manh = input.test == Bailout::MANH;
    const bool manr = input.test == Bailout::MANR;
    set_bailout_formula(input.test);
    g_magnitude_limit = input.limit;
    g_old_z_bf.x = -7;
    g_old_z_bf.y = 9;
    g_new_z_bf.x = input.point.x;
    g_new_z_bf.y = input.point.y;
    BigArena &working = bf_arena(BigLifetime::WORKING);
    BigFloat expected_x{working};
    BigFloat expected_y{working};
    expected_x = g_new_z_bf.x.sqr();
    expected_y = g_new_z_bf.y.sqr();
    const auto marker = save_stack();

    EXPECT_EQ(escaped, g_bailout_bigfloat() != 0);

    EXPECT_EQ(marker, save_stack());
    EXPECT_EQ(expected_x, g_tmp_sqr_x_bf);
    EXPECT_EQ(expected_y, g_tmp_sqr_y_bf);
    EXPECT_EQ(BigFloat{escaped ? -7 : input.point.x}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{escaped ? 9 : input.point.y}, g_old_z_bf.y);
    EXPECT_EQ(BigFloat{input.point.x}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{input.point.y}, g_new_z_bf.y);
    if (manh || manr || input.test == Bailout::MOD)
    {
        const double sum = manh ? std::abs(input.point.x) + std::abs(input.point.y) : input.point.x + input.point.y;
        EXPECT_EQ(BigFloat{input.test == Bailout::MOD ? input.metric : sum}, g_bf_tmp);
    }
    EXPECT_EQ(-17, g_magnitude);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatBailoutCases, testing::ValuesIn(inputs()),
    [](const testing::TestParamInfo<BigFloatBailoutInput> &info) { return info.param.name; });

class TestBigFloatBailoutPrecision : public TestBigFloatBailout,
                                     public testing::WithParamInterface<std::tuple<Bailout, int>>
{
};

TEST_P(TestBigFloatBailoutPrecision, inclusiveComparisonRetainsDisplacementsBelowMachinePrecision)
{
    const std::array<double, 7> limits{0.5, 0.25, 0.25, 0.25, 0.25, 1, 1};
    const std::array<const char *, 3> values{
        "0.4999999999999999999999999999999", "0.5", "0.5000000000000000000000000000001"};
    const Bailout mode = std::get<0>(GetParam());
    const int point = std::get<1>(GetParam());
    set_bailout_formula(mode);
    g_magnitude_limit = limits[static_cast<std::size_t>(mode)];
    g_new_z_bf.x = BigFloat{values[point]};
    g_new_z_bf.y = BigFloat{values[point]};
    const BFComplex original{g_new_z_bf};

    EXPECT_EQ(point != 0, g_bailout_bigfloat() != 0);

    EXPECT_EQ(original, g_new_z_bf);
}

INSTANTIATE_TEST_SUITE_P(ModesAndDisplacements, TestBigFloatBailoutPrecision,
    testing::Combine(testing::Values(Bailout::MOD, Bailout::REAL, Bailout::IMAG, Bailout::OR, Bailout::AND,
                         Bailout::MANH, Bailout::MANR),
        testing::Values(0, 1, 2)));

class TestBigFloatBailoutContinuation : public TestBigFloatBailout,
                                        public testing::WithParamInterface<std::tuple<Bailout, bool>>
{
};

TEST_P(TestBigFloatBailoutContinuation, repeatedOrbitCallsMatchTheOrdinaryBailoutContract)
{
    const Bailout mode = std::get<0>(GetParam());
    const bool escape = std::get<1>(GetParam());
    ValueSaver<DComplex> saved_old{g_old_z, DComplex{1, 1}};
    ValueSaver<DComplex> saved_new{g_new_z};
    ValueSaver<DComplex> saved_parameter{g_param_z1, DComplex{1, 0}};
    ValueSaver<DComplex *> saved_float_parameter{g_float_param, &g_param_z1};
    ValueSaver<double> saved_sqr_x{g_temp_sqr_x, 1};
    ValueSaver<double> saved_sqr_y{g_temp_sqr_y, 1};
    set_bailout_formula(mode);
    g_magnitude_limit = escape ? 1 : 100;
    g_old_z_bf.x = 1;
    g_old_z_bf.y = 1;
    g_param_z_bf.x = 1;
    g_param_z_bf.y = 0;
    g_tmp_sqr_x_bf = sqr(g_old_z_bf.x);
    g_tmp_sqr_y_bf = sqr(g_old_z_bf.y);
    const std::array<DComplex, 2> expected{{{1, 2}, {-2, escape ? 2.0 : 4.0}}};

    for (int iteration = 0; iteration < 2; ++iteration)
    {
        const int ordinary_result = julia_orbit();
        const BigStackMark mark = save_stack();
        const std::size_t temporaries = big_memory_stats().temporaries;

        EXPECT_EQ(ordinary_result, julia_orbit_bf());

        EXPECT_EQ(mark, save_stack());
        if (iteration != 0)
        {
            EXPECT_EQ(temporaries, big_memory_stats().temporaries);
        }
        EXPECT_EQ(expected[iteration].x, g_new_z_bf.x.to_float());
        EXPECT_EQ(expected[iteration].y, g_new_z_bf.y.to_float());
        EXPECT_EQ(g_old_z.x, g_old_z_bf.x.to_float());
        EXPECT_EQ(g_old_z.y, g_old_z_bf.y.to_float());
        EXPECT_EQ(BigFloat{sqr(expected[iteration].x)}, g_tmp_sqr_x_bf);
        EXPECT_EQ(BigFloat{sqr(expected[iteration].y)}, g_tmp_sqr_y_bf);
    }
}

INSTANTIATE_TEST_SUITE_P(ModesAndOutcomes, TestBigFloatBailoutContinuation,
    testing::Combine(testing::Values(Bailout::MANH, Bailout::MANR), testing::Bool()));

class BigFloatMandelbrotInitializationTest : public TestBigFloatBailout
{
protected:
    void TearDown() override;
    void select_bigfloat_dispatch(FractalType type);

    ValueSaver<const FractalSpecific *> m_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<FractalDispatch> m_dispatch{g_dispatch, FractalDispatch{FractalType::MANDEL}};
    ValueSaver<InitOrbitMode> m_initial_mode{g_use_init_orbit};
    ValueSaver<DComplex> m_initial_value{g_init_orbit, DComplex{0.5, -0.25}};
    ValueSaver<DComplex> m_parameter{g_param_z1, DComplex{0.125, -0.0625}};
    ValueSaver<double> m_real{g_params[0], 0.125};
    ValueSaver<double> m_imaginary{g_params[1], -0.0625};
    ValueSaver<double> m_power_real{g_params[2], 2};
    ValueSaver<double> m_power_imaginary{g_params[3], 0};
    ValueSaver<SymmetryType> m_symmetry{g_symmetry};
    ValueSaver<CalcMode> m_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen, LogicalScreen{5, 5, 0, 0, 4, 4}};
    ValueSaver<ImageRegion> m_region{g_image_region, ImageRegion{{-1, -1}, {1, 1}, {-0.5, -0.5}}};
    ValueSaver<Inversion> m_inversion{g_inversion, Inversion{}};
    ValueSaver<LDouble> m_delta_x{g_delta_x, 0.375};
    ValueSaver<LDouble> m_delta_y{g_delta_y, 0.375};
    ValueSaver<LDouble> m_delta2_x{g_delta_x2, 0.125};
    ValueSaver<LDouble> m_delta2_y{g_delta_y2, 0.125};
    ValueSaver<int> m_col{g_col, 1};
    ValueSaver<int> m_row{g_row, 2};
    ValueSaver<int> m_periodicity{g_periodicity_check, 0};
    ValueSaver<ColorMethod> m_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<bool> m_book{g_bof_match_book_images, false};
    ValueSaver<long> m_iteration{g_color_iter, 37};
    ValueSaver<int> m_exponent{g_c_exponent};
    ValueSaver<DComplex> m_power{g_power_z};
    ValueSaver<DComplex *> m_float_parameter{g_float_param};
    ValueSaver<DComplex> m_init{g_init};
    ValueSaver<DComplex> m_old{g_old_z};
    ValueSaver<DComplex> m_new{g_new_z};
    ValueSaver<DComplex> m_tmp{g_tmp_z};
    ValueSaver<double> m_sqr_x{g_temp_sqr_x};
    ValueSaver<double> m_sqr_y{g_temp_sqr_y};
    ValueSaver<DebugFlags> m_debug{g_debug_flag, DebugFlags::FORCE_STANDARD_FRACTAL};
};

void BigFloatMandelbrotInitializationTest::TearDown()
{
    free_pixel_grid();
    TestBigFloatBailout::TearDown();
}

void BigFloatMandelbrotInitializationTest::select_bigfloat_dispatch(const FractalType type)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_dec(80);
    g_fractal_type = type;
    g_cur_fractal_specific = get_fractal_specific(type);
    g_dispatch = FractalDispatch{type};
    if (type == FractalType::MANDEL)
    {
        // Mirror the disabled BigFloat Mandelbrot entry in g_alternate_math.
        const AlternateMath alternate{
            FractalType::MANDEL, BFMathType::BIG_FLT, julia_orbit_bf, mandel_per_pixel_bf, mandel_per_image_bf};
        g_dispatch.set_current_alternate_math(alternate);
    }
    else
    {
        ASSERT_TRUE(select_alternate_math_dispatch());
    }
}

class TestBigFloatMandelbrotInitialization
    : public BigFloatMandelbrotInitializationTest,
      public testing::WithParamInterface<std::tuple<std::pair<InitOrbitMode, bool>, std::pair<ColorMethod, bool>, bool>>
{
};

TEST_P(TestBigFloatMandelbrotInitialization, dispatchMatchesRegularSeedParameterCachesAndFirstIterate)
{
    const auto [mode, zero] = std::get<0>(GetParam());
    const auto [inside, book] = std::get<1>(GetParam());
    const FractalType type = std::get<2>(GetParam()) ? FractalType::MANDEL_Z_POWER : FractalType::MANDEL;
    g_fractal_type = FractalType::MANDEL;
    g_use_init_orbit = mode;
    g_init_orbit = zero ? DComplex{} : DComplex{0.5, -0.25};
    g_inside_method = inside;
    g_bof_match_book_images = book;
    g_magnitude_limit = 4;
    set_bailout_formula(Bailout::MOD);
    const DComplex pixel{-0.375, 0.125};
    const bool bof = book && (inside == ColorMethod::BOF60 || inside == ColorMethod::BOF61);
    DComplex seed = bof ? DComplex{} : mode == InitOrbitMode::VALUE ? g_init_orbit : pixel;
    seed += g_param_z1;
    const long iteration = bof ? -1 : 37;
    alloc_pixel_grid();
    fill_pixel_grid();
    ASSERT_TRUE(g_dispatch.per_image()());
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    EXPECT_EQ(seed, g_old_z);
    EXPECT_EQ(pixel, g_init);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(sqr(seed.x), g_temp_sqr_x);
    EXPECT_EQ(sqr(seed.y), g_temp_sqr_y);
    ASSERT_EQ(0, g_dispatch.orbit_calc()());
    const DComplex first = g_new_z;
    EXPECT_EQ(sqr(seed.x) - sqr(seed.y) + pixel.x, first.x);
    EXPECT_EQ(2 * seed.x * seed.y + pixel.y, first.y);

    select_bigfloat_dispatch(type);
    g_bf_x_min = -1;
    g_bf_x_max = 1;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_x_3rd = -0.5;
    g_bf_y_3rd = -0.5;
    g_color_iter = 37;
    ASSERT_EQ(mandel_per_pixel_bf, g_dispatch.per_pixel());
    ASSERT_TRUE(g_dispatch.per_image()());
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);

    ASSERT_EQ(1, g_dispatch.per_pixel()());

    EXPECT_EQ(BigFloat{seed.x}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{seed.y}, g_old_z_bf.y);
    EXPECT_EQ(BigFloat{seed.x}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{seed.y}, g_new_z_bf.y);
    EXPECT_EQ(BigFloat{pixel.x}, g_param_z_bf.x);
    EXPECT_EQ(BigFloat{pixel.y}, g_param_z_bf.y);
    EXPECT_EQ(iteration, g_color_iter);
    EXPECT_EQ(BigFloat{sqr(seed.x)}, g_tmp_sqr_x_bf);
    EXPECT_EQ(BigFloat{sqr(seed.y)}, g_tmp_sqr_y_bf);

    ASSERT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_DOUBLE_EQ(first.x, static_cast<double>(g_new_z_bf.x.to_float()));
    EXPECT_DOUBLE_EQ(first.y, static_cast<double>(g_new_z_bf.y.to_float()));
    EXPECT_EQ(BigFloat{pixel.x}, g_param_z_bf.x);
    EXPECT_EQ(BigFloat{pixel.y}, g_param_z_bf.y);
    EXPECT_EQ(iteration, g_color_iter);
}

INSTANTIATE_TEST_SUITE_P(ModesAndBof, TestBigFloatMandelbrotInitialization,
    testing::Combine(testing::Values(std::pair{InitOrbitMode::NORMAL, false}, std::pair{InitOrbitMode::PIXEL, false},
                         std::pair{InitOrbitMode::VALUE, false}, std::pair{InitOrbitMode::VALUE, true}),
        testing::Values(std::pair{ColorMethod::COLOR, false}, std::pair{ColorMethod::BOF60, false},
            std::pair{ColorMethod::BOF60, true}, std::pair{ColorMethod::BOF61, false},
            std::pair{ColorMethod::BOF61, true}),
        testing::Bool()));

class TestBigFloatMandelbrotPreciseInitialization : public BigFloatMandelbrotInitializationTest,
                                                    public testing::WithParamInterface<InitOrbitMode>
{
};

TEST_P(TestBigFloatMandelbrotPreciseInitialization, preservesPixelBitsBeyondMachinePrecision)
{
    g_use_init_orbit = GetParam();
    g_params[0] = g_params[1] = 0;
    g_col = g_row = 0;
    g_magnitude_limit = 4;
    set_bailout_formula(Bailout::MOD);
    select_bigfloat_dispatch(FractalType::MANDEL);
    const BigFloat pixel_x = BigFloat{0.25} + BigFloat{0x1p-80};
    const BigFloat pixel_y{0.125};
    g_bf_x_min = pixel_x;
    g_bf_x_max = pixel_x;
    g_bf_x_3rd = pixel_x;
    g_bf_y_min = pixel_y;
    g_bf_y_max = pixel_y;
    g_bf_y_3rd = pixel_y;
    ASSERT_TRUE(g_dispatch.per_image()());

    ASSERT_EQ(1, g_dispatch.per_pixel()());

    const bool fixed = GetParam() == InitOrbitMode::VALUE;
    const BigFloat seed_x = fixed ? BigFloat{0.5} : pixel_x;
    const BigFloat seed_y = fixed ? BigFloat{-0.25} : pixel_y;
    EXPECT_EQ(pixel_x, g_param_z_bf.x);
    EXPECT_EQ(pixel_y, g_param_z_bf.y);
    EXPECT_EQ(seed_x, g_old_z_bf.x);
    EXPECT_EQ(seed_y, g_old_z_bf.y);
    const BigFloat squared_x = fixed ? BigFloat{0.25} : BigFloat{0.0625} + BigFloat{0x1p-81} + BigFloat{0x1p-160};
    EXPECT_EQ(squared_x, g_tmp_sqr_x_bf);
    EXPECT_EQ(BigFloat{fixed ? 0.0625 : 0.015625}, g_tmp_sqr_y_bf);

    ASSERT_EQ(0, g_dispatch.orbit_calc()());

    const BigFloat first_x =
        fixed ? BigFloat{0.4375} + BigFloat{0x1p-80} : BigFloat{0.296875} + BigFloat{0x1.8p-80} + BigFloat{0x1p-160};
    const BigFloat first_y = fixed ? BigFloat{-0.125} : BigFloat{0.1875} + BigFloat{0x1p-82};
    EXPECT_EQ(first_x, g_new_z_bf.x);
    EXPECT_EQ(first_y, g_new_z_bf.y);
    EXPECT_EQ(pixel_x, g_param_z_bf.x);
    EXPECT_EQ(pixel_y, g_param_z_bf.y);
}

TEST_P(TestBigFloatMandelbrotPreciseInitialization, burningShipKeepsItsExistingSeedSelection)
{
    g_use_init_orbit = GetParam();
    select_bigfloat_dispatch(FractalType::BURNING_SHIP);
    g_bf_x_min = -1;
    g_bf_x_max = 1;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_x_3rd = -0.5;
    g_bf_y_3rd = -0.5;
    ASSERT_TRUE(g_dispatch.per_image()());

    ASSERT_EQ(1, g_dispatch.per_pixel()());

    EXPECT_EQ(BigFloat{-0.25}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0.0625}, g_old_z_bf.y);
    EXPECT_EQ(BigFloat{-0.375}, g_param_z_bf.x);
    EXPECT_EQ(BigFloat{0.125}, g_param_z_bf.y);
    EXPECT_EQ(37, g_color_iter);
}

INSTANTIATE_TEST_SUITE_P(Modes, TestBigFloatMandelbrotPreciseInitialization,
    testing::Values(InitOrbitMode::NORMAL, InitOrbitMode::VALUE, InitOrbitMode::PIXEL));

class TestBigFloatJulia : public TestBigFloatBailout
{
protected:
    void SetUp() override;

    ValueSaver<CalcMode> m_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen, LogicalScreen{257, 513, 0, 0, 256, 512}};
    ValueSaver<int> m_col{g_col, 0};
    ValueSaver<int> m_row{g_row, 0};
    ValueSaver<long> m_iteration{g_color_iter, 37};
    ValueSaver<int> m_periodicity{g_periodicity_check, 0};
    ValueSaver<ColorMethod> m_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<bool> m_book{g_bof_match_book_images, false};
    ValueSaver<int> m_exponent{g_c_exponent};
    ValueSaver<double> m_real{g_params[0], 0.5};
    ValueSaver<double> m_imaginary{g_params[1], 0.375};
};

void TestBigFloatJulia::SetUp()
{
    TestBigFloatBailout::SetUp();
    g_fractal_type = FractalType::JULIA;
    set_bailout_formula(Bailout::MOD);
}

TEST_F(TestBigFloatJulia, affineSeedsKeepPreciseFixedParameterAndSquareCaches)
{
    g_bf_x_min = -2;
    g_bf_x_max = 2;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_x_3rd = -1.5;
    g_bf_y_3rd = -0.75;
    g_bf_params[0] = BigFloat{"0.25000000000000000001"};
    g_bf_params[1] = BigFloat{"-0.12500000000000000001"};
    const BFComplex parameter{g_bf_params[0], g_bf_params[1]};
    // These bounded reference values use double, including scalar conversion.
    constexpr double tolerance = 64 * std::numeric_limits<double>::epsilon();
    g_magnitude_limit = 1024;

    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    EXPECT_NE(BigFloat{0.25}, g_param_z_bf.x);
    EXPECT_NE(BigFloat{-0.125}, g_param_z_bf.y);
    for (const int col : {0, 128, 256})
    {
        for (const int row : {0, 256, 512})
        {
            SCOPED_TRACE(col);
            SCOPED_TRACE(row);
            g_col = col;
            g_row = row;
            const double real = -2 + col * (7.0 / 512) + row * (1.0 / 1024);
            const double imag = 1 - (row * (7.0 / 2048) + col * (1.0 / 1024));

            EXPECT_EQ(1, julia_per_pixel_bf());

            EXPECT_EQ(37, g_color_iter);
            EXPECT_NEAR(real, static_cast<double>(g_old_z_bf.x.to_float()), tolerance);
            EXPECT_NEAR(imag, static_cast<double>(g_old_z_bf.y.to_float()), tolerance);
            EXPECT_NEAR(real, static_cast<double>(g_new_z_bf.x.to_float()), tolerance);
            EXPECT_NEAR(imag, static_cast<double>(g_new_z_bf.y.to_float()), tolerance);
            EXPECT_NEAR(real * real, static_cast<double>(g_tmp_sqr_x_bf.to_float()), tolerance);
            EXPECT_NEAR(imag * imag, static_cast<double>(g_tmp_sqr_y_bf.to_float()), tolerance);
            EXPECT_EQ(parameter, g_param_z_bf);

            EXPECT_EQ(0, julia_orbit_bf());

            EXPECT_NEAR(real * real - imag * imag + 0.25, static_cast<double>(g_new_z_bf.x.to_float()), tolerance);
            EXPECT_NEAR(2 * real * imag - 0.125, static_cast<double>(g_new_z_bf.y.to_float()), tolerance);
            EXPECT_EQ(parameter, g_param_z_bf);
        }
    }
}

TEST_F(TestBigFloatJulia, viewportTransitionRestoresJuliaParameterAndKeepsBailoutEquality)
{
    g_bf_x_min = 1;
    g_bf_x_max = 3;
    g_bf_x_3rd = 1;
    g_bf_y_min = -1;
    g_bf_y_max = 1;
    g_bf_y_3rd = -1;
    g_bf_params[0] = 0;
    g_bf_params[1] = 0;
    g_col = 128;
    g_row = 256;
    g_magnitude_limit = 16;
    g_fractal_type = FractalType::MANDEL;
    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(1, mandel_per_pixel_bf());
    ASSERT_EQ(BigFloat{2}, g_param_z_bf.x);

    g_fractal_type = FractalType::JULIA;
    ASSERT_TRUE(mandel_per_image_bf());
    ASSERT_EQ(BigFloat{0}, g_param_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_param_z_bf.y);
    ASSERT_EQ(1, julia_per_pixel_bf());
    EXPECT_EQ(37, g_color_iter);
    EXPECT_EQ(BigFloat{2}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(1, julia_orbit_bf());

    EXPECT_EQ(BigFloat{4}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    EXPECT_EQ(BigFloat{2}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(1, julia_orbit_bf());

    EXPECT_EQ(BigFloat{16}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    EXPECT_EQ(BigFloat{2}, g_old_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_old_z_bf.y);
}

class TestBigFloatJuliaZPower : public TestBigFloatJulia, public testing::WithParamInterface<JuliaZPowerInput>
{
protected:
    void SetUp() override;

    ValueSaver<double> m_power_real{g_params[2]};
    ValueSaver<double> m_power_imaginary{g_params[3]};
    ValueSaver<SymmetryType> m_symmetry{g_symmetry, SymmetryType::ORIGIN};
    ValueSaver<FractalDispatch> m_dispatch{g_dispatch};
    ValueSaver<const FractalSpecific *> m_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA_Z_POWER)};
};

void TestBigFloatJuliaZPower::SetUp()
{
    TestBigFloatJulia::SetUp();
    g_fractal_type = FractalType::JULIA_Z_POWER;
    g_params[2] = GetParam().exponent.real();
    g_params[3] = GetParam().exponent.imag();
    g_dispatch = FractalDispatch{*g_cur_fractal_specific};
    ASSERT_TRUE(select_alternate_math_dispatch());
    g_bf_x_min = 0;
    g_bf_x_max = 1;
    g_bf_x_3rd = 0;
    g_bf_y_min = 0;
    g_bf_y_max = 0.5;
    g_bf_y_3rd = 0;
    g_bf_params[0] = BigFloat{"0.12500000000000000001"};
    g_bf_params[1] = BigFloat{"-0.06250000000000000001"};
    g_magnitude_limit = 1024;
}

TEST_P(TestBigFloatJuliaZPower, setupSeedsAndOrbitKeepTheFixedParameterAndExponentRules)
{
    const BFComplex parameter{g_bf_params[0], g_bf_params[1]};
    EXPECT_EQ(mandel_per_image_bf, g_dispatch.per_image());
    EXPECT_EQ(julia_per_pixel_bf, g_dispatch.per_pixel());
    EXPECT_EQ(julia_z_power_orbit_bf, g_dispatch.orbit_calc());
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_ref);
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_ref_bf);
    EXPECT_EQ(nullptr, g_cur_fractal_specific->pert_pt);

    ASSERT_TRUE(g_dispatch.per_image()());

    EXPECT_EQ(static_cast<int>(GetParam().exponent.real()), g_c_exponent);
    EXPECT_EQ(GetParam().symmetry, g_symmetry);
    EXPECT_EQ(parameter, g_param_z_bf);
    g_col = 128;
    g_row = 256;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    EXPECT_EQ(37, g_color_iter);
    EXPECT_NEAR(0.5, static_cast<double>(g_old_z_bf.x.to_float()), 1e-14);
    EXPECT_NEAR(0.25, static_cast<double>(g_old_z_bf.y.to_float()), 1e-14);
    const std::complex<double> expected =
        std::pow(std::complex<double>{0.5, 0.25}, GetParam().exponent) + std::complex<double>{0.125, -0.0625};
    const double tolerance = 1e-12 * std::max(1.0, std::abs(expected));

    EXPECT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_NEAR(expected.real(), static_cast<double>(g_new_z_bf.x.to_float()), tolerance);
    EXPECT_NEAR(expected.imag(), static_cast<double>(g_new_z_bf.y.to_float()), tolerance);
    EXPECT_EQ(parameter, g_param_z_bf);
}

TEST_P(TestBigFloatJuliaZPower, zeroSeedRetainsTheLegacyPowerConvention)
{
    ASSERT_TRUE(g_dispatch.per_image()());
    g_col = 0;
    g_row = 512;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.y);

    EXPECT_EQ(0, g_dispatch.orbit_calc()());

    EXPECT_NEAR(0.125, static_cast<double>(g_new_z_bf.x.to_float()), 1e-14);
    EXPECT_NEAR(-0.0625, static_cast<double>(g_new_z_bf.y.to_float()), 1e-14);
}

TEST_P(TestBigFloatJuliaZPower, unitSeedKeepsTheInclusiveBailoutBoundary)
{
    g_bf_params[0] = 0;
    g_bf_params[1] = 0;
    ASSERT_TRUE(g_dispatch.per_image()());
    g_col = 256;
    g_row = 512;
    ASSERT_EQ(1, g_dispatch.per_pixel()());
    ASSERT_EQ(BigFloat{1}, g_old_z_bf.x);
    ASSERT_EQ(BigFloat{0}, g_old_z_bf.y);
    g_magnitude_limit = 1;

    EXPECT_EQ(1, g_dispatch.orbit_calc()());

    EXPECT_EQ(BigFloat{1}, g_new_z_bf.x);
    EXPECT_EQ(BigFloat{0}, g_new_z_bf.y);
    g_magnitude_limit = 0.5;

    EXPECT_EQ(1, g_dispatch.orbit_calc()());
}

INSTANTIATE_TEST_SUITE_P(Exponents, TestBigFloatJuliaZPower, testing::ValuesIn(JULIA_Z_POWERS),
    [](const testing::TestParamInfo<JuliaZPowerInput> &info) { return info.param.name; });

} // namespace id::test
