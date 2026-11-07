// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/interpreter.h>

#include <engine/random_seed.h>
#include <fractals/parser.h>
#include <math/arg.h>
#include <math/fixed_pt.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

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

struct TestFormulaRandom : TestInterpreter
{
    void SetUp() override;
    DComplex rand_from_seed(int seed);
    DComplex rand_from_srand(int image_seed);

    ValueSaver<CompiledFormula> saved_formula{g_formula};
    ValueSaver<int> saved_random_seed{g_random_seed, 1234};
    ValueSaver<bool> saved_random_seed_flag{g_random_seed_flag, true};
    ValueSaver<RuntimeState> saved_runtime{g_runtime, RuntimeState{}};
};

void TestFormulaRandom::SetUp()
{
    g_formula.vars.resize(8);
}

DComplex TestFormulaRandom::rand_from_seed(const int seed)
{
    g_random_seed = seed;
    g_runtime = RuntimeState{};
    random_seed();
    d_random();
    return g_formula.vars[7].a.d;
}

DComplex TestFormulaRandom::rand_from_srand(const int image_seed)
{
    g_random_seed = image_seed;
    g_runtime = RuntimeState{};
    Arg stack{};
    ValueSaver<Arg *> saved_stack_arg{g_arg1, &stack};
    stack.d = DComplex{0.25, 0.75};
    d_stk_srand();
    return g_formula.vars[7].a.d;
}

struct ZeroBasePowerInput
{
    const char *name;
    int version;
    DComplex exponent;
    DComplex result;
    bool overflow;
};

class TestZeroBasePower : public TestInterpreter, public testing::WithParamInterface<ZeroBasePowerInput>
{
protected:
    void SetUp() override;
};

void TestZeroBasePower::SetUp()
{
    g_version = parse_legacy_version(GetParam().version);
    g_overflow = false;
}

TEST_P(TestZeroBasePower, retainsVersionedResultAndOverflow)
{
    const DComplex result{stack_power({0.0, 0.0}, GetParam().exponent)};

    EXPECT_EQ(GetParam().result, result);
    EXPECT_EQ(GetParam().overflow, g_overflow);
}

INSTANTIATE_TEST_SUITE_P(Compatibility, TestZeroBasePower,
    testing::Values(ZeroBasePowerInput{"Pre1900PositiveExponent", 1899, {7.0, 0.0}, {0.0, 0.0}, false},
        ZeroBasePowerInput{"Reset1900PositiveExponent", 1900, {7.0, 0.0}, {0.0, 0.0}, true},
        ZeroBasePowerInput{"Pre1900ZeroExponent", 1899, {0.0, 0.0}, {1.0, 0.0}, false},
        ZeroBasePowerInput{"Reset1900ZeroExponent", 1900, {0.0, 0.0}, {0.0, 0.0}, true}),
    [](const testing::TestParamInfo<ZeroBasePowerInput> &info) { return info.param.name; });

struct FormulaSeedPair
{
    const char *name;
    int first;
    int second;
    bool equal;
};

class TestFormulaRandomSeeds : public TestFormulaRandom, public testing::WithParamInterface<FormulaSeedPair>
{
};

TEST_P(TestFormulaRandomSeeds, fixedSeedControlsRandomValue)
{
    const DComplex first{rand_from_seed(GetParam().first)};
    const DComplex second{rand_from_seed(GetParam().second)};

    EXPECT_EQ(GetParam().equal, first == second);
}

INSTANTIATE_TEST_SUITE_P(SeedPairs, TestFormulaRandomSeeds,
    testing::Values(FormulaSeedPair{"Repeated", 1234, 1234, true}, FormulaSeedPair{"Changed", 1234, 5678, false}),
    [](const testing::TestParamInfo<FormulaSeedPair> &info) { return info.param.name; });

TEST_F(TestFormulaRandom, formulaRandWithoutFixedSeedAdvancesImageSeed)
{
    g_random_seed_flag = false;

    random_seed();

    EXPECT_EQ(1235, g_random_seed);
}

TEST_F(TestFormulaRandom, formulaSrandIgnoresImageRandomSeed)
{
    EXPECT_EQ(rand_from_srand(1234), rand_from_srand(5678));
}

} // namespace id::test
