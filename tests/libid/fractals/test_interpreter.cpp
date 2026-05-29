// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/interpreter.h"

#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

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

} // namespace id::test
