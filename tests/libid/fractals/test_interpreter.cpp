// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/interpreter.h"

#include "engine/calcfrac.h"
#include "engine/random_seed.h"
#include "fractals/formula.h"
#include "fractals/parser.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>
#include <ostream>
#include <tuple>

using namespace id::fractals;
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

static DComplex stack_power(const DComplex &base, const DComplex &exponent)
{
    Arg stack[2]{};
    stack[0].d = base;
    stack[1].d = exponent;
    g_arg2 = &stack[0];
    g_arg1 = &stack[1];

    d_stk_pwr();

    return stack[0].d;
}

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

TEST_F(TestInterpreter, pre1900ZeroBasePowerDoesNotOverflow)
{
    g_version = parse_legacy_version(1899);
    g_overflow = false;

    const DComplex result{stack_power({0.0, 0.0}, {7.0, 0.0})};

    EXPECT_EQ((DComplex{0.0, 0.0}), result);
    EXPECT_FALSE(g_overflow);
}

TEST_F(TestInterpreter, reset1900ZeroBasePowerSetsOverflow)
{
    g_version = parse_legacy_version(1900);
    g_overflow = false;

    const DComplex result{stack_power({0.0, 0.0}, {7.0, 0.0})};

    EXPECT_EQ((DComplex{0.0, 0.0}), result);
    EXPECT_TRUE(g_overflow);
}

TEST_F(TestInterpreter, pre1900ZeroBaseZeroExponentReturnsOne)
{
    g_version = parse_legacy_version(1899);
    g_overflow = false;

    const DComplex result{stack_power({0.0, 0.0}, {0.0, 0.0})};

    EXPECT_EQ((DComplex{1.0, 0.0}), result);
    EXPECT_FALSE(g_overflow);
}

TEST_F(TestInterpreter, reset1900ZeroBaseZeroExponentSetsOverflow)
{
    g_version = parse_legacy_version(1900);
    g_overflow = false;

    const DComplex result{stack_power({0.0, 0.0}, {0.0, 0.0})};

    EXPECT_EQ((DComplex{0.0, 0.0}), result);
    EXPECT_TRUE(g_overflow);
}

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
