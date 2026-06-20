// SPDX-License-Identifier: GPL-3.0-only
//
#include "test_data.h"
#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>

#include <engine/calcfrac.h>
#include <engine/fractals.h>
#include <engine/trig_fns.h>
#include <fractals/formula.h>
#include <fractals/frasetup.h>
#include <fractals/lambda_fn.h>
#include <fractals/parser.h>
#include <fractals/phoenix.h>
#include <fractals/pickover_mandelbrot.h>
#include <fractals/popcorn.h>
#include <fractals/taylor_skinner_variations.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <math/big.h>
#include <misc/debug_flags.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace testing;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::test::data;

namespace id::test
{

static int test_orbit_calc()
{
    return 17;
}

static int test_per_pixel()
{
    return 23;
}

static bool test_per_image()
{
    return true;
}

static int test_calc_type()
{
    return 31;
}

struct TableFunctions
{
    OrbitCalc orbit_calc{};
    PerPixel per_pixel{};
    PerImage per_image{};
};

static TableFunctions table_functions(const FractalType type)
{
    const FractalSpecific *specific{get_fractal_specific(type)};
    return {specific->orbit_calc, specific->per_pixel, specific->per_image};
}

static void expect_table_functions(const FractalType type, const TableFunctions &expected)
{
    const FractalSpecific *specific{get_fractal_specific(type)};
    EXPECT_EQ(expected.orbit_calc, specific->orbit_calc);
    EXPECT_EQ(expected.per_pixel, specific->per_pixel);
    EXPECT_EQ(expected.per_image, specific->per_image);
}

class DispatchSelectionState
{
public:
    explicit DispatchSelectionState(const FractalType type)
    {
        set_fractal_type(type);
    }

private:
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_fractal_dispatch};
    ValueSaver<FiniteAttractor> m_saved_attractor{g_attractor, FiniteAttractor{}};
    ValueSaver<CalcMode> m_saved_std_calc_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<BFMathType> m_saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_c_exponent{g_c_exponent};
    ValueSaver<DComplex *> m_saved_float_param{g_float_param};
    ValueSaver<DComplex> m_saved_param_z1{g_param_z1, DComplex{}};
    ValueSaver<DComplex> m_saved_param_z2{g_param_z2, DComplex{}};
    ValueSaver<DComplex> m_saved_power_z{g_power_z};
    ValueSaver<SymmetryType> m_saved_symmetry{g_symmetry};
    ValueSaver<double> m_saved_param_0{g_params[0], 0.0};
    ValueSaver<double> m_saved_param_1{g_params[1], 0.0};
    ValueSaver<double> m_saved_param_2{g_params[2], 0.0};
    ValueSaver<double> m_saved_param_3{g_params[3], 0.0};
    ValueSaver<double> m_saved_param_4{g_params[4], 0.0};
    ValueSaver<double> m_saved_param_5{g_params[5], 0.0};
    ValueSaver<TrigFn> m_saved_trig_0{g_trig_index[0], TrigFn::SIN};
    ValueSaver<TrigFn> m_saved_trig_1{g_trig_index[1], TrigFn::COS};
    ValueSaver<TrigFn> m_saved_trig_2{g_trig_index[2], TrigFn::SIN};
    ValueSaver<TrigFn> m_saved_trig_3{g_trig_index[3], TrigFn::COS};
};

class ParserResetter
{
public:
    ~ParserResetter()
    {
        parser_reset();
    }
};

TEST(TestFractalSpecific, perturbationFlagRequiresPerturbationFunctions)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        if (!bit_set(g_fractal_specific[i].flags, FractalFlags::PERTURB))
        {
            continue;
        }

        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_ref_bf)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation bigfloat reference function";
        EXPECT_NE(nullptr, g_fractal_specific[i].pert_pt)
            << "Fractal type " << g_fractal_specific[i].name << " (" << i
            << ") has no perturbation point function";
    }
}

TEST(TestFractalSpecific, typeMatchesGet)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        EXPECT_EQ(&from, get_fractal_specific(from.type)) << "index " << i << ", " << g_fractal_specific[i].name;
    }
}

TEST(TestFractalDispatch, seededDispatchMatchesDefaults)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        const FractalDispatch dispatch{make_fractal_dispatch(from)};

        EXPECT_EQ(&from, dispatch.specific) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.orbit_calc, dispatch.orbit_calc) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.per_pixel, dispatch.per_pixel) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.per_image, dispatch.per_image) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.calc_type, dispatch.calc_type) << "index " << i << ", " << from.name;
        EXPECT_EQ(from.symmetry, dispatch.symmetry) << "index " << i << ", " << from.name;
    }
}

TEST(TestFractalDispatch, setFractalTypeSeedsCurrentDispatch)
{
    ValueSaver saved_type{g_fractal_type};
    ValueSaver saved_specific{g_cur_fractal_specific};
    ValueSaver saved_dispatch{g_fractal_dispatch};

    set_fractal_type(FractalType::MANDEL);

    const FractalSpecific &from{*get_fractal_specific(FractalType::MANDEL)};
    EXPECT_EQ(&from, g_fractal_dispatch.specific);
    EXPECT_EQ(from.orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(from.per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(from.per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(from.calc_type, g_fractal_dispatch.calc_type);
    EXPECT_EQ(from.symmetry, g_fractal_dispatch.symmetry);
}

TEST(TestFractalDispatch, accessorsUseCurrentDispatch)
{
    FractalDispatch dispatch{};
    dispatch.orbit_calc = test_orbit_calc;
    dispatch.per_pixel = test_per_pixel;
    dispatch.per_image = test_per_image;
    dispatch.calc_type = test_calc_type;

    ValueSaver saved_specific{g_cur_fractal_specific, nullptr};
    ValueSaver saved_dispatch{g_fractal_dispatch, dispatch};

    EXPECT_TRUE(per_image());
    EXPECT_EQ(23, per_pixel());
    EXPECT_EQ(17, orbit_calc());
    EXPECT_EQ(test_calc_type, current_calc_type());
    EXPECT_EQ(31, current_calc_type()());
}

TEST(TestFractalDispatch, settersOnlyUpdateDispatch)
{
    FractalSpecific specific{};
    ValueSaver saved_specific{g_cur_fractal_specific, &specific};
    ValueSaver saved_dispatch{g_fractal_dispatch};

    set_current_orbit_calc(test_orbit_calc);
    set_current_per_pixel(test_per_pixel);
    set_current_per_image(test_per_image);

    EXPECT_EQ(test_orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(test_per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(test_per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(nullptr, specific.orbit_calc);
    EXPECT_EQ(nullptr, specific.per_pixel);
    EXPECT_EQ(nullptr, specific.per_image);
}

TEST(TestFractalDispatch, alternateMathOnlyUpdatesDispatch)
{
    FractalSpecific specific{};
    FractalDispatch dispatch{};
    AlternateMath alternate{
        FractalType::MANDEL, math::BFMathType::BIG_NUM, test_orbit_calc, test_per_pixel, test_per_image};

    ValueSaver saved_specific{g_cur_fractal_specific, &specific};
    ValueSaver saved_dispatch{g_fractal_dispatch, dispatch};

    set_current_alternate_math(alternate);

    EXPECT_EQ(test_orbit_calc, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(test_per_pixel, g_fractal_dispatch.per_pixel);
    EXPECT_EQ(test_per_image, g_fractal_dispatch.per_image);
    EXPECT_EQ(nullptr, specific.orbit_calc);
    EXPECT_EQ(nullptr, specific.per_pixel);
    EXPECT_EQ(nullptr, specific.per_image);
}

TEST(TestFractalDispatch, formulaParseFailureOnlyUpdatesDispatch)
{
    ParserResetter reset_parser;
    DispatchSelectionState state{FractalType::FORMULA};
    const TableFunctions table{table_functions(FractalType::FORMULA)};
    ValueSaver saved_formula_name{g_formula_name, std::string{}};

    fs::path path{"unused.frm"};

    EXPECT_TRUE(parse_formula(path, g_formula_name, true));
    EXPECT_EQ(bad_formula, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(bad_formula, g_fractal_dispatch.per_pixel);
    expect_table_functions(FractalType::FORMULA, table);
}

TEST(TestFractalDispatch, formulaParseSuccessOnlyUpdatesDispatch)
{
    ParserResetter reset_parser;
    DispatchSelectionState state{FractalType::FORMULA};
    const TableFunctions table{table_functions(FractalType::FORMULA)};
    ValueSaver saved_formula_name{g_formula_name, std::string{"Fractint"}};

    fs::path path{fs::path{ID_TEST_FRM_DIR} / ID_TEST_FRM_FILE};

    EXPECT_FALSE(parse_formula(path, g_formula_name, true));
    EXPECT_EQ(formula_orbit, g_fractal_dispatch.orbit_calc);
    EXPECT_EQ(formula_per_pixel, g_fractal_dispatch.per_pixel);
    expect_table_functions(FractalType::FORMULA, table);
}

TEST(TestFractalDispatch, perImageSelectorsOnlyUpdateDispatch)
{
    {
        DispatchSelectionState state{FractalType::MANDEL_Z_POWER};
        const TableFunctions table{table_functions(FractalType::MANDEL_Z_POWER)};
        g_params[2] = 3.0;
        g_params[3] = 1.0;

        EXPECT_TRUE(mandel_per_image());

        EXPECT_EQ(mandel_z_power_cmplx_orbit, current_orbit_calc());
        expect_table_functions(FractalType::MANDEL_Z_POWER, table);
    }

    {
        DispatchSelectionState state{FractalType::MANDEL_FN};
        const TableFunctions table{table_functions(FractalType::MANDEL_FN)};
        g_trig_index[0] = TrigFn::LOG;

        EXPECT_TRUE(mandel_trig_per_image());

        EXPECT_EQ(lambda_trig_orbit, current_orbit_calc());
        expect_table_functions(FractalType::MANDEL_FN, table);
    }

    {
        DispatchSelectionState state{FractalType::PHOENIX};
        const TableFunctions table{table_functions(FractalType::PHOENIX)};
        g_param_z2.x = 2.0;

        EXPECT_TRUE(phoenix_per_image());

        EXPECT_EQ(phoenix_plus_fractal, current_orbit_calc());
        expect_table_functions(FractalType::PHOENIX, table);
    }

    {
        DispatchSelectionState state{FractalType::FN_PLUS_FN};
        const TableFunctions table{table_functions(FractalType::FN_PLUS_FN)};
        g_trig_index[0] = TrigFn::SIN;
        g_trig_index[1] = TrigFn::TAN;
        g_param_z1 = DComplex{2.0, 0.0};
        g_param_z2 = DComplex{2.0, 0.0};

        EXPECT_TRUE(trig_plus_trig_per_image());

        EXPECT_EQ(trig_plus_trig_orbit, current_orbit_calc());
        EXPECT_EQ(other_julia_per_pixel, current_per_pixel());
        expect_table_functions(FractalType::FN_PLUS_FN, table);
    }

    {
        DispatchSelectionState state{FractalType::POPCORN_JUL};
        const TableFunctions table{table_functions(FractalType::POPCORN_JUL)};
        g_trig_index[0] = TrigFn::SIN;
        g_trig_index[1] = TrigFn::TAN;
        g_trig_index[2] = TrigFn::SIN;
        g_trig_index[3] = TrigFn::TAN;
        g_param_z1 = DComplex{0.0, 0.0};
        g_param_z2 = DComplex{3.0, 0.0};
        g_debug_flag = DebugFlags::FORCE_REAL_POPCORN;

        EXPECT_TRUE(julia_per_image());

        EXPECT_EQ(popcorn_fractal, current_orbit_calc());
        expect_table_functions(FractalType::POPCORN_JUL, table);
    }
}

TEST(TestFractalSpecific, toJuliaExists)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        if (const FractalType julia_type = from.to_julia; julia_type != FractalType::NO_FRACTAL)
        {
            EXPECT_NO_THROW(get_fractal_specific(julia_type)) << "index " << i << " (" << from.name << ")";
            EXPECT_EQ(from.type, get_fractal_specific(julia_type)->to_mandel)
                << "index " << i << " (" << from.name << ") mismatched Julia/Mandelbrot toggle";
        }
    }
}

TEST(TestFractalSpecific, toMandelbrotExists)
{
    for (int i = 0; i < g_num_fractal_types; ++i)
    {
        const FractalSpecific &from{g_fractal_specific[i]};
        if (const FractalType mandel_type = from.to_mandel; mandel_type != FractalType::NO_FRACTAL)
        {
            EXPECT_NO_THROW(get_fractal_specific(mandel_type)) << "index " << i << " (" << from.name << ")";
            // type=inverse_julia is a special case;
            // it has a link to Mandelbrot, but doesn't have a link back.
            if (from.type != FractalType::INVERSE_JULIA)
            {
                EXPECT_EQ(from.type, get_fractal_specific(mandel_type)->to_julia)
                    << "index " << i << " (" << from.name << ") mismatched Julia/Mandelbrot toggle";
            }
        }
    }
}

std::ostream &operator<<(std::ostream &str, const FractalSpecific &value)
{
    return str << "type: " << +value.type << " '" << value.name << "'";
}

TEST(TestFractalSpecific, fractalSpecificEntriesAreSortedByType)
{
    EXPECT_TRUE(std::is_sorted(&g_fractal_specific[0], &g_fractal_specific[g_num_fractal_types],
        [](const FractalSpecific &lhs, const FractalSpecific &rhs) { return lhs.type < rhs.type; }));
}

TEST(TestGetFractalSpecific, resultMatchesType)
{
    const FractalSpecific *entry = get_fractal_specific(FractalType::CELLULAR);

    ASSERT_NE(nullptr, entry);
    EXPECT_EQ(FractalType::CELLULAR, entry->type);
}

TEST(TestGetFractalSpecific, unknownTypeThrowsRuntimeError)
{
    EXPECT_THROW(get_fractal_specific(static_cast<FractalType>(-100)), std::runtime_error);
}

} // namespace id::test
