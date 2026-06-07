// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/interpreter.h"

#include "engine/random_seed.h"
#include "fractals/parser.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

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

} // namespace id::test
