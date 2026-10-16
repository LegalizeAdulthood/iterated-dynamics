// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include "fractals/interpreter.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/random_seed.h"
#include "fractals/formula.h"
#include "fractals/parser.h"
#include "io/gif_extensions.h"
#include "io/loadfile.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <ostream>
#include <tuple>

using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::engine;

namespace id::test
{

struct TestInterpreter : testing::Test
{
    ValueSaver<Arg *> saved_arg1{g_arg1};
    ValueSaver<Arg *> saved_arg2{g_arg2};
    ValueSaver<bool> saved_overflow{g_overflow};
    ValueSaver<Version> saved_version{g_version};
};

static DComplex formula_rand_from_seed(const int seed)
{
    ValueSaver saved_formula{g_formula};
    ValueSaver saved_random_seed{g_random_seed, seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};
    ValueSaver saved_runtime{g_runtime, RuntimeState{}};

    g_formula.vars.resize(8);
    random_seed();
    d_random();

    return g_formula.vars[7].a.d;
}

static DComplex formula_rand_from_srand(const int image_seed)
{
    ValueSaver saved_formula{g_formula};
    ValueSaver saved_random_seed{g_random_seed, image_seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};
    ValueSaver saved_runtime{g_runtime, RuntimeState{}};
    Arg stack{};
    ValueSaver saved_arg1{g_arg1, &stack};

    g_formula.vars.resize(8);
    stack.d = DComplex{0.25, 0.75};
    d_stk_srand();

    return g_formula.vars[7].a.d;
}

struct FormulaPowerCase
{
    DComplex base;
    DComplex exponent;
    DComplex expected_old;
    DComplex expected_new;
};

void PrintTo(const FormulaPowerCase &value, std::ostream *str)
{
    *str << '(' << value.base.x << ", " << value.base.y << ")^(" << value.exponent.x << ", " << value.exponent.y << ')';
}

// DOS FPUcplxlog(0) returns zero, so the old power path returns exp(0).
static const FormulaPowerCase FORMULA_POWERS[]{
    {{0.0, 0.0}, {7.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{0.0, 0.0}, {-2.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{0.0, 0.0}, {2.0, 3.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{0.0, 0.0}, {0.0, 3.0}, {1.0, 0.0}, {0.0, 0.0}},
    {{2.0, 0.0}, {3.0, 0.0}, {8.0, 0.0}, {8.0, 0.0}},
    {{3.0, 4.0}, {0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}},
    {{3.0, 4.0}, {1.0, 0.0}, {3.0, 4.0}, {3.0, 4.0}},
    {{3.0, 4.0}, {2.0, 0.0}, {-7.0, 24.0}, {-7.0, 24.0}},
};

class TestInterpreterPower : public TestInterpreter,
                             public testing::WithParamInterface<std::tuple<Version, FormulaPowerCase, bool>>
{
};

TEST_P(TestInterpreterPower, preservesCompatibleResultStackAndOverflow)
{
    const auto &[version, power, overflow] = GetParam();
    g_version = version;
    g_overflow = overflow;
    const DComplex expected = version < 1900 ? power.expected_old : power.expected_new;
    Arg stack[3]{{{17.0, 19.0}}, {power.base}, {power.exponent}};
    g_arg2 = &stack[1];
    g_arg1 = &stack[2];

    d_stk_pwr();

    EXPECT_NEAR(expected.x, stack[1].d.x, 1e-13);
    EXPECT_NEAR(expected.y, stack[1].d.y, 1e-13);
    EXPECT_EQ((DComplex{17.0, 19.0}), stack[0].d);
    EXPECT_EQ(&stack[1], g_arg1);
    EXPECT_EQ(&stack[0], g_arg2);
    EXPECT_EQ(overflow, g_overflow);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestInterpreterPower,
    testing::Combine(testing::Values(parse_legacy_version(1899), parse_legacy_version(1900), current_id_version()),
        testing::ValuesIn(FORMULA_POWERS), testing::Bool()));

enum class FormulaPowerVersionSource
{
    PAR_RESET,
    GIF_METADATA
};

void PrintTo(const FormulaPowerVersionSource value, std::ostream *str)
{
    *str << (value == FormulaPowerVersionSource::PAR_RESET ? "PAR reset" : "GIF metadata");
}

class TestFormulaPowerOrbit
    : public TestInterpreter,
      public testing::WithParamInterface<std::tuple<Version, FormulaPowerCase, FormulaPowerVersionSource>>
{
};

TEST_P(TestFormulaPowerOrbit, loadedVersionAllowsSubsequentIterations)
{
    const auto &[version, power, source] = GetParam();
    ValueSaver saved_file_version{g_file_version};
    ValueSaver saved_formula{g_formula, CompiledFormula{}};
    ValueSaver saved_runtime{g_runtime, RuntimeState{}};
    ValueSaver saved_name{g_formula_name, std::string{"PowerCompatibility"}};
    ValueSaver saved_old_z{g_old_z};
    ValueSaver saved_new_z{g_new_z};
    if (source == FormulaPowerVersionSource::PAR_RESET)
    {
        ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET,
            cmd_arg("reset=" + to_par_string(version), CmdFile::AT_AFTER_STARTUP));
    }
    else
    {
        FractalInfo info{};
        info.info_version = FRACTAL_INFO_VERSION;
        if (version.legacy)
        {
            info.release = static_cast<std::int16_t>(100 * version.major + version.minor);
        }
        else
        {
            info.version_major = version.major;
            info.version_minor = version.minor;
            info.version_patch = version.patch;
            info.version_tweak = version.tweak;
        }
        g_file_version = fractal_info_version(info);
        g_version = g_file_version;
    }
    ASSERT_EQ(version, g_version);
    g_overflow = false;
    g_formula_name = "PowerCompatibility";
    Arg base{power.base};
    Arg exponent{power.exponent};
    Arg increment{{1.0, 0.0}};
    Arg keep_iterating{{1.0, 0.0}};
    g_formula.vars.resize(5);
    g_formula.store = {&g_formula.vars[3].a};
    g_formula.load = {&g_formula.vars[3].a, &base, &exponent, &increment, &keep_iterating};
    g_formula.fns = {stk_lod, stk_lod, stk_lod, d_stk_pwr, d_stk_add, stk_lod, d_stk_add, stk_sto, stk_clr, stk_lod};
    g_formula.op_count = static_cast<int>(g_formula.fns.size());
    const DComplex expected = version < 1900 ? power.expected_old : power.expected_new;

    ASSERT_EQ(0, formula_orbit());

    EXPECT_NEAR(expected.x + 1.0, g_new_z.x, 1e-13);
    EXPECT_NEAR(expected.y, g_new_z.y, 1e-13);
    EXPECT_FALSE(g_overflow);
    ASSERT_EQ(0, formula_orbit());
    EXPECT_NEAR(2.0 * (expected.x + 1.0), g_new_z.x, 2e-13);
    EXPECT_NEAR(2.0 * expected.y, g_new_z.y, 2e-13);
    EXPECT_FALSE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestFormulaPowerOrbit,
    testing::Combine(testing::Values(parse_legacy_version(1899), parse_legacy_version(1900), current_id_version()),
        testing::ValuesIn(FORMULA_POWERS),
        testing::Values(FormulaPowerVersionSource::PAR_RESET, FormulaPowerVersionSource::GIF_METADATA)));

TEST_F(TestInterpreter, formulaRandRepeatsWithFixedRandomSeed)
{
    EXPECT_EQ(formula_rand_from_seed(1234), formula_rand_from_seed(1234));
}

TEST_F(TestInterpreter, formulaRandChangesWithFixedRandomSeed)
{
    const DComplex first{formula_rand_from_seed(1234)};
    const DComplex second{formula_rand_from_seed(5678)};

    EXPECT_TRUE(first.x != second.x || first.y != second.y);
}

TEST_F(TestInterpreter, formulaRandWithoutFixedSeedAdvancesImageSeed)
{
    ValueSaver saved_formula{g_formula};
    ValueSaver saved_random_seed{g_random_seed, 1234};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, false};
    ValueSaver saved_runtime{g_runtime, RuntimeState{}};

    g_formula.vars.resize(8);
    random_seed();

    EXPECT_EQ(1235, g_random_seed);
}

TEST_F(TestInterpreter, formulaSrandIgnoresImageRandomSeed)
{
    EXPECT_EQ(formula_rand_from_srand(1234), formula_rand_from_srand(5678));
}

struct FormulaDenominatorVersion
{
    Version version;
    bool traps_errors;
};

void PrintTo(const FormulaDenominatorVersion &value, std::ostream *str)
{
    *str << to_par_string(value.version) << ", traps errors: " << value.traps_errors;
}

struct FormulaDenominatorOperation
{
    const char *name;
    FunctionPtr function;
    bool binary;
    DComplex operand;
    DComplex expected;
};

void PrintTo(const FormulaDenominatorOperation &value, std::ostream *str)
{
    *str << value.name << '(' << value.operand.x << ", " << value.operand.y << ')';
}

static const double DENOMINATOR_ROOT{std::sqrt(DBL_MIN)};
static const double FORMULA_PI{std::acos(-1.0)};

// The C unary functions return before replacing the operand. Trig
// functions have already doubled it; reciprocal leaves it unchanged.
static const FormulaDenominatorOperation ERROR_DENOMINATORS[]{
    {"tan", d_stk_tan, false, {FORMULA_PI / 2, 0.0}, {FORMULA_PI, 0.0}},
    {"tan", d_stk_tan, false, {FORMULA_PI / 2, DBL_MIN / 2}, {FORMULA_PI, DBL_MIN}},
    {"tanh", d_stk_tanh, false, {0.0, FORMULA_PI / 2}, {0.0, FORMULA_PI}},
    {"tanh", d_stk_tanh, false, {DBL_MIN / 2, FORMULA_PI / 2}, {DBL_MIN, FORMULA_PI}},
    {"cotan", d_stk_cotan, false, {0.0, 0.0}, {0.0, 0.0}},
    {"cotan", d_stk_cotan, false, {DENOMINATOR_ROOT / 2, DENOMINATOR_ROOT / 4},
        {DENOMINATOR_ROOT, DENOMINATOR_ROOT / 2}},
    {"cotanh", d_stk_cotanh, false, {0.0, 0.0}, {0.0, 0.0}},
    {"cotanh", d_stk_cotanh, false, {DENOMINATOR_ROOT / 4, DENOMINATOR_ROOT / 2},
        {DENOMINATOR_ROOT / 2, DENOMINATOR_ROOT}},
    {"recip", d_stk_recip, false, {0.0, 0.0}, {0.0, 0.0}},
    {"recip", d_stk_recip, false, {DENOMINATOR_ROOT / 2, 0.0}, {DENOMINATOR_ROOT / 2, 0.0}},
    {"recip", d_stk_recip, false, {0.0, DENOMINATOR_ROOT / 2}, {0.0, DENOMINATOR_ROOT / 2}},
    {"recip", d_stk_recip, false, {DENOMINATOR_ROOT, 0.0}, {DENOMINATOR_ROOT, 0.0}},
    {"div", d_stk_div, true, {0.0, 0.0}, {ID_INFINITY, ID_INFINITY}},
    {"div", d_stk_div, true, {DENOMINATOR_ROOT / 2, 0.0}, {ID_INFINITY, ID_INFINITY}},
    {"div", d_stk_div, true, {0.0, DENOMINATOR_ROOT / 2}, {ID_INFINITY, ID_INFINITY}},
    {"div", d_stk_div, true, {DENOMINATOR_ROOT, 0.0}, {ID_INFINITY, ID_INFINITY}},
};

static const FormulaDenominatorOperation FINITE_DENOMINATORS[]{
    {"tan", d_stk_tan, false, {0.0, 0.0}, {0.0, 0.0}},
    {"tanh", d_stk_tanh, false, {0.0, 0.0}, {0.0, 0.0}},
    {"cotan", d_stk_cotan, false, {FORMULA_PI / 4, 0.0}, {1.0, 0.0}},
    {"cotanh", d_stk_cotanh, false, {std::log(3.0) / 2, 0.0}, {2.0, 0.0}},
    {"recip", d_stk_recip, false, {3.0, 4.0}, {0.12, -0.16}},
    {"recip", d_stk_recip, false, {2 * DENOMINATOR_ROOT, 0.0}, {0.5 / DENOMINATOR_ROOT, 0.0}},
    {"recip", d_stk_recip, false, {0.0, 2 * DENOMINATOR_ROOT}, {0.0, -0.5 / DENOMINATOR_ROOT}},
    {"div", d_stk_div, true, {3.0, 4.0}, {2.0, 0.0}},
    {"div", d_stk_div, true, {2 * DENOMINATOR_ROOT, 0.0}, {3 / DENOMINATOR_ROOT, 4 / DENOMINATOR_ROOT}},
    {"div", d_stk_div, true, {0.0, 2 * DENOMINATOR_ROOT}, {4 / DENOMINATOR_ROOT, -3 / DENOMINATOR_ROOT}},
};

static void run_denominator_operation(Arg (&stack)[3], const FormulaDenominatorOperation &operation)
{
    stack[0].d = {17.0, 19.0};
    stack[1].d = {6.0, 8.0};
    stack[2].d = operation.operand;
    g_arg1 = &stack[2];
    g_arg2 = &stack[1];
    operation.function();
}

class TestInterpreterDenominatorError
    : public TestInterpreter,
      public testing::WithParamInterface<std::tuple<FormulaDenominatorVersion, FormulaDenominatorOperation, bool>>
{
};

TEST_P(TestInterpreterDenominatorError, preservesOperandsStackAndCompatibleOverflow)
{
    const auto &[version, operation, initial_overflow] = GetParam();
    g_version = version.version;
    g_overflow = initial_overflow;
    Arg stack[3]{};

    run_denominator_operation(stack, operation);

    const int result_index = operation.binary ? 1 : 2;
    EXPECT_EQ(operation.expected, stack[result_index].d);
    EXPECT_EQ(&stack[result_index], g_arg1);
    EXPECT_EQ(&stack[result_index - 1], g_arg2);
    EXPECT_EQ((DComplex{17.0, 19.0}), stack[0].d);
    EXPECT_EQ(initial_overflow || version.traps_errors, g_overflow);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestInterpreterDenominatorError,
    testing::Combine(testing::Values(FormulaDenominatorVersion{parse_legacy_version(1920), false},
                         FormulaDenominatorVersion{parse_legacy_version(1921), true},
                         FormulaDenominatorVersion{current_id_version(), true}),
        testing::ValuesIn(ERROR_DENOMINATORS), testing::Bool()));

class TestInterpreterFiniteDenominator
    : public TestInterpreter,
      public testing::WithParamInterface<std::tuple<FormulaDenominatorVersion, FormulaDenominatorOperation>>
{
};

TEST_P(TestInterpreterFiniteDenominator, validOperandsDoNotSetOverflow)
{
    const auto &[version, operation] = GetParam();
    g_version = version.version;
    g_overflow = false;
    Arg stack[3]{};

    run_denominator_operation(stack, operation);

    const DComplex result = stack[operation.binary ? 1 : 2].d;
    EXPECT_NEAR(operation.expected.x, result.x, std::abs(operation.expected.x) * 1e-14 + 1e-14);
    EXPECT_NEAR(operation.expected.y, result.y, std::abs(operation.expected.y) * 1e-14 + 1e-14);
    EXPECT_FALSE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestInterpreterFiniteDenominator,
    testing::Combine(testing::Values(FormulaDenominatorVersion{parse_legacy_version(1920), false},
                         FormulaDenominatorVersion{parse_legacy_version(1921), true},
                         FormulaDenominatorVersion{current_id_version(), true}),
        testing::ValuesIn(FINITE_DENOMINATORS)));

class TestFormulaDenominatorOrbit
    : public TestInterpreter,
      public testing::WithParamInterface<std::tuple<FormulaDenominatorVersion, FormulaDenominatorOperation>>
{
};

TEST_P(TestFormulaDenominatorOrbit, continuesOrTerminatesAtCompatibleRelease)
{
    const auto &[version, operation] = GetParam();
    g_version = version.version;
    g_overflow = false;
    ValueSaver saved_formula{g_formula, CompiledFormula{}};
    ValueSaver saved_runtime{g_runtime, RuntimeState{}};
    ValueSaver saved_name{g_formula_name, std::string{"DenominatorCompatibility"}};
    ValueSaver saved_old_z{g_old_z};
    ValueSaver saved_new_z{g_new_z};
    Arg numerator{{6.0, 8.0}};
    Arg operand{operation.operand};
    Arg keep_iterating{{1.0, 0.0}};
    g_formula.vars.resize(5);
    g_formula.store = {&g_formula.vars[3].a};
    if (operation.binary)
    {
        g_formula.fns = {stk_lod, stk_lod, operation.function, stk_sto, stk_clr, stk_lod};
        g_formula.load = {&numerator, &operand, &keep_iterating};
    }
    else
    {
        g_formula.fns = {stk_lod, operation.function, stk_sto, stk_clr, stk_lod};
        g_formula.load = {&operand, &keep_iterating};
    }
    g_formula.op_count = static_cast<int>(g_formula.fns.size());

    ASSERT_EQ(0, formula_orbit());

    EXPECT_EQ(operation.expected, g_new_z);
    EXPECT_EQ(version.traps_errors, g_overflow);
    EXPECT_EQ(version.traps_errors ? 1 : 0, formula_orbit());
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestFormulaDenominatorOrbit,
    testing::Combine(testing::Values(FormulaDenominatorVersion{parse_legacy_version(1920), false},
                         FormulaDenominatorVersion{parse_legacy_version(1921), true},
                         FormulaDenominatorVersion{current_id_version(), true}),
        testing::ValuesIn(ERROR_DENOMINATORS)));

class TestInterpreterGenericDivision : public TestInterpreter, public testing::WithParamInterface<Version>
{
};

TEST_P(TestInterpreterGenericDivision, genericDivisionStillSetsOverflowForAllReleases)
{
    g_version = GetParam();
    g_overflow = false;
    DComplex result{};

    fpu_cmplx_div(DComplex{6.0, 8.0}, DComplex{}, result);

    EXPECT_EQ((DComplex{ID_INFINITY, ID_INFINITY}), result);
    EXPECT_TRUE(g_overflow);
}

INSTANTIATE_TEST_SUITE_P(ResetVersions, TestInterpreterGenericDivision,
    testing::Values(parse_legacy_version(1920), parse_legacy_version(1921), current_id_version()));

} // namespace id::test
