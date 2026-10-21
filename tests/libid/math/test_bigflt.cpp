// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

class TestBigFloat : public testing::Test
{
protected:
    TestBigFloat() :
        m_saved_bailout_value(g_user.bailout_value, 0),
        m_saved_bailout_test(g_bailout_test, Bailout::MOD),
        m_saved_bf_digits(g_bf_digits, 0),
        m_saved_fractal_type(g_fractal_type, FractalType::MANDEL)
    {
    }

    void SetUp() override
    {
        init_bf_dec(20);
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

TEST_F(TestBigFloat, parsesPlainDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, "1.25");

    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, parsesStringViewWithoutTerminator)
{
    const std::string storage{"1.25x"};
    const std::string_view text{storage.data(), 4};
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, text);

    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, parsesExponent)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, "1.25e2");

    EXPECT_EQ(125.0L, bf_to_float(value));
}

TEST_F(TestBigFloat, formatsZero)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    clear_bf(value);

    EXPECT_EQ("0.0", bf_to_string(value, 5));
    EXPECT_EQ("0.0", bf_to_string_e(value, 5));
    EXPECT_EQ("0.0", bf_to_string_f(value, 5));
}

TEST_F(TestBigFloat, formatsFixedDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");

    EXPECT_EQ("1.25", bf_to_string_f(value, 5));
}

TEST_F(TestBigFloat, formatsScientificDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");

    EXPECT_EQ("1.25e0", bf_to_string_e(value, 5));
}

TEST_F(TestBigFloat, rejectedPrecisionPreservesActiveArena)
{
    float_to_bf(g_bf_x_min, 1.25);
    const int length{g_bf_length};
    const int decimals{g_decimals};
    const int stack{save_stack()};
    const BigFloat corner{g_bf_x_min};

    EXPECT_THROW(init_bf_dec(2000), PrecisionError);
    EXPECT_THROW(init_bf_dec(std::numeric_limits<int>::max()), PrecisionError);
    EXPECT_THROW(init_bf_length(4096), PrecisionError);
    EXPECT_THROW(init_bf_length(std::numeric_limits<int>::max()), PrecisionError);
    EXPECT_THROW(init_bf_length(std::numeric_limits<int>::min()), PrecisionError);

    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(decimals, g_decimals);
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(corner, g_bf_x_min);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    init_bf_dec(40);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

TEST_F(TestBigFloat, rejectedForcedPrecisionAllowsCorrectedRequest)
{
    g_bf_digits = 2000;

    EXPECT_THROW(init_bf_dec(20), PrecisionError);

    g_bf_digits = 40;
    EXPECT_NO_THROW(init_bf_dec(20));
    EXPECT_EQ(40, g_decimals);
}

TEST_F(TestBigFloat, floatingPointArenaRejectsUnsupportedPrecision)
{
    g_fractal_type = FractalType::DIVIDE_BROT5;
    init_bf_dec(20);
    ASSERT_EQ(BFMathType::BIG_FLT, g_bf_math);
    const int length{g_bn_length};

    EXPECT_THROW(init_bf_dec(2000), PrecisionError);

    EXPECT_EQ(length, g_bn_length);
    EXPECT_NO_THROW(init_bf_dec(40));
}

TEST_F(TestBigFloat, stackLimitFailureLeavesAllocationStateUnchanged)
{
    const int stack{save_stack()};
    const long high_water{g_bignum_max_stack_addr};
    const size_t available{static_cast<size_t>(g_max_stack - stack)};

    EXPECT_THROW(alloc_stack(available + 1), PrecisionError);
    EXPECT_THROW(alloc_stack(std::numeric_limits<size_t>::max()), PrecisionError);

    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(high_water, g_bignum_max_stack_addr);
    EXPECT_NE(nullptr, alloc_stack(available));
    EXPECT_EQ(g_max_stack, save_stack());
    EXPECT_THROW(alloc_stack(1), PrecisionError);
    restore_stack(stack);
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");
    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, stackSaverUnwindsAfterExhaustion)
{
    const int stack{save_stack()};

    EXPECT_THROW(
        {
            BigStackSaver saved;
            alloc_stack(g_bf_length + 2);
            alloc_stack(static_cast<size_t>(g_max_stack));
        },
        PrecisionError);

    EXPECT_EQ(stack, save_stack());
    EXPECT_NO_THROW(alloc_stack(g_bf_length + 2));
}

TEST_F(TestBigFloat, freeingArenaAllowsFreshInitialization)
{
    float_to_bf(g_bf_x_min, 1.25);
    alloc_stack(g_bf_length + 2);

    free_bf_vars();

    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(0, save_stack());
    EXPECT_EQ(0, g_start_stack);
    EXPECT_EQ(0, g_max_stack);
    EXPECT_EQ(0, g_bignum_max_stack_addr);
    EXPECT_THROW(alloc_stack(1), std::logic_error);
    EXPECT_THROW(init_bf_dec(2000), PrecisionError);
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    init_bf_dec(20);
    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");
    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, loweringPrecisionPreservesCornersWithoutOverrunningArena)
{
    init_bf_dec(1200);
    float_to_bf(g_bf_x_min, 1.25);

    init_bf_dec(20);

    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    EXPECT_EQ(g_start_stack, save_stack());
    EXPECT_NO_THROW(alloc_stack(g_bf_length + 2));
}

} // namespace id::test
