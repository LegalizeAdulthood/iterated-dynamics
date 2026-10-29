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

struct BigFloatParseInput
{
    const char *name;
    std::string storage;
    std::size_t length;
    LDouble expected;
};

class TestBigFloatParse : public TestBigFloat, public testing::WithParamInterface<BigFloatParseInput>
{
};

TEST_P(TestBigFloatParse, parsesTheSpecifiedText)
{
    const std::string_view text{GetParam().storage.data(), GetParam().length};
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, text);

    EXPECT_EQ(GetParam().expected, bf_to_float(value));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatParse,
    testing::Values(BigFloatParseInput{"PlainDecimal", "1.25", 4, 1.25L},
        BigFloatParseInput{"UnterminatedView", "1.25x", 4, 1.25L}, BigFloatParseInput{"Exponent", "1.25e2", 6, 125.0L}),
    [](const testing::TestParamInfo<BigFloatParseInput> &info) { return info.param.name; });

class TestBigFloatFormat : public TestBigFloat
{
protected:
    void prepare_value(const char *text);

    BigFloat m_value{};
};

void TestBigFloatFormat::prepare_value(const char *text)
{
    m_value = alloc_stack(g_bf_length + 2);
    if (text)
    {
        str_to_bf(m_value, text);
    }
    else
    {
        clear_bf(m_value);
    }
}

TEST_F(TestBigFloatFormat, generalFormatHandlesZero)
{
    prepare_value(nullptr);

    EXPECT_EQ("0.0", bf_to_string(m_value, 5));
}

struct BigFloatFormatInput
{
    const char *name;
    const char *input;
    int decimals;
    const char *expected;
};

class TestBigFloatFixedFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
protected:
    void SetUp() override;
};

void TestBigFloatFixedFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatFixedFormat, formatsFixedDecimal)
{
    EXPECT_EQ(GetParam().expected, bf_to_string_f(m_value, GetParam().decimals));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatFixedFormat,
    testing::Values(
        BigFloatFormatInput{"Zero", nullptr, 5, "0.0"}, BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

class TestBigFloatScientificFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
protected:
    void SetUp() override;
};

void TestBigFloatScientificFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatScientificFormat, formatsScientificDecimal)
{
    EXPECT_EQ(GetParam().expected, bf_to_string_e(m_value, GetParam().decimals));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatScientificFormat,
    testing::Values(
        BigFloatFormatInput{"Zero", nullptr, 5, "0.0"}, BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25e0"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

class TestActivePrecisionArena : public TestBigFloat
{
protected:
    void prepare_active_arena(FractalType type);
    void expect_preserved_arena();

    int m_length{};
    int m_decimals{};
    int m_stack{};
    BigFloat m_corner{};
};

void TestActivePrecisionArena::prepare_active_arena(const FractalType type)
{
    g_fractal_type = type;
    init_bf_dec(20);
    float_to_bf(g_bf_x_min, 1.25);
    m_length = g_bf_length;
    m_decimals = g_decimals;
    m_stack = save_stack();
    m_corner = g_bf_x_min;
}

void TestActivePrecisionArena::expect_preserved_arena()
{
    EXPECT_EQ(m_length, g_bf_length);
    EXPECT_EQ(m_decimals, g_decimals);
    EXPECT_EQ(m_stack, save_stack());
    EXPECT_EQ(m_corner, g_bf_x_min);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

struct RejectedDecimalPrecisionInput
{
    const char *name;
    FractalType type;
    BFMathType math;
    int request;
};

class TestRejectedDecimalPrecision : public TestActivePrecisionArena,
                                     public testing::WithParamInterface<RejectedDecimalPrecisionInput>
{
protected:
    void SetUp() override;
};

void TestRejectedDecimalPrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_arena(GetParam().type);
    ASSERT_EQ(GetParam().math, g_bf_math);
}

TEST_P(TestRejectedDecimalPrecision, preservesActiveArenaAndAllowsCorrection)
{
    const int integer_length{g_bn_length};

    EXPECT_THROW(init_bf_dec(GetParam().request), PrecisionError);

    expect_preserved_arena();
    EXPECT_EQ(integer_length, g_bn_length);
    EXPECT_NO_THROW(init_bf_dec(40));
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

INSTANTIATE_TEST_SUITE_P(Requests, TestRejectedDecimalPrecision,
    testing::Values(RejectedDecimalPrecisionInput{"BigNumUnsupported", FractalType::MANDEL, BFMathType::BIG_NUM, 2000},
        RejectedDecimalPrecisionInput{
            "BigNumMaximumInteger", FractalType::MANDEL, BFMathType::BIG_NUM, std::numeric_limits<int>::max()},
        RejectedDecimalPrecisionInput{"BigFloatUnsupported", FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, 2000}),
    [](const testing::TestParamInfo<RejectedDecimalPrecisionInput> &info) { return info.param.name; });

class TestRejectedBytePrecision : public TestActivePrecisionArena, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestRejectedBytePrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_arena(FractalType::MANDEL);
}

TEST_P(TestRejectedBytePrecision, preservesActiveArenaAndAllowsCorrection)
{
    EXPECT_THROW(init_bf_length(GetParam()), PrecisionError);

    expect_preserved_arena();
    init_bf_dec(40);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

INSTANTIATE_TEST_SUITE_P(Requests, TestRejectedBytePrecision,
    testing::Values(4096, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()));

TEST_F(TestBigFloat, rejectedForcedPrecisionAllowsCorrectedRequest)
{
    g_bf_digits = 2000;

    EXPECT_THROW(init_bf_dec(20), PrecisionError);

    g_bf_digits = 40;
    EXPECT_NO_THROW(init_bf_dec(20));
    EXPECT_EQ(40, g_decimals);
}

struct ArenaPrecisionLimit
{
    const char *name;
    FractalType type;
    BFMathType math;
    int length;
};

class TestArenaPrecisionLimit : public TestBigFloat, public testing::WithParamInterface<ArenaPrecisionLimit>
{
protected:
    void SetUp() override;
};

void TestArenaPrecisionLimit::SetUp()
{
    TestBigFloat::SetUp();
    g_fractal_type = GetParam().type;
    init_bf_length(GetParam().length);
    ASSERT_EQ(GetParam().math, g_bf_math);
    ASSERT_EQ(GetParam().length, g_bn_length);
    float_to_bf(g_bf_x_min, 1.25);
}

TEST_P(TestArenaPrecisionLimit, rejectsRequestsPastTheBackendLimit)
{
    const int stack{save_stack()};

    EXPECT_THROW(init_bf_length(GetParam().length + 1), PrecisionError);

    EXPECT_EQ(GetParam().length, g_bn_length);
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

INSTANTIATE_TEST_SUITE_P(Backends, TestArenaPrecisionLimit,
    testing::Values(ArenaPrecisionLimit{"BigNum", FractalType::MANDEL, BFMathType::BIG_NUM, 672},
        ArenaPrecisionLimit{"BigFloat", FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, 708}),
    [](const testing::TestParamInfo<ArenaPrecisionLimit> &info) { return info.param.name; });

TEST_F(TestBigFloat, stackStorageCrossesSixtyFourKilobytes)
{
    constexpr size_t OLD_ARENA_SIZE{64 * 1024};
    const int stack{save_stack()};
    float_to_bf(g_bf_x_min, 1.25);
    {
        BigStackSaver saved;
        alloc_stack(OLD_ARENA_SIZE);
        const BigFloat value{alloc_stack(g_bf_length + 2)};
        EXPECT_GT(save_stack(), OLD_ARENA_SIZE);
        float_to_bf(value, 2.5);
        add_bf(value, value, g_bf_x_min);

        EXPECT_EQ(3.75L, bf_to_float(value));
        EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    }
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

class TestArenaStack : public TestBigFloat
{
protected:
    void SetUp() override;

    int m_stack{};
    long m_high_water{};
    std::size_t m_available{};
};

void TestArenaStack::SetUp()
{
    TestBigFloat::SetUp();
    m_stack = save_stack();
    m_high_water = g_bignum_max_stack_addr;
    m_available = static_cast<std::size_t>(g_max_stack - m_stack);
}

class TestOversizedArenaStack : public TestArenaStack, public testing::WithParamInterface<bool>
{
};

TEST_P(TestOversizedArenaStack, rejectionPreservesAllocationState)
{
    const std::size_t request{GetParam() ? std::numeric_limits<std::size_t>::max() : m_available + 1};

    EXPECT_THROW(alloc_stack(request), PrecisionError);

    EXPECT_EQ(m_stack, save_stack());
    EXPECT_EQ(m_high_water, g_bignum_max_stack_addr);
}

INSTANTIATE_TEST_SUITE_P(Requests, TestOversizedArenaStack, testing::Bool(),
    [](const testing::TestParamInfo<bool> &info) { return info.param ? "MaximumSize" : "AvailablePlusOne"; });

TEST_F(TestArenaStack, exactExhaustionAllocatesTheRemainingStorage)
{
    EXPECT_NE(nullptr, alloc_stack(m_available));
    EXPECT_EQ(g_max_stack, save_stack());
}

class TestExhaustedArenaStack : public TestArenaStack
{
protected:
    void SetUp() override;
};

void TestExhaustedArenaStack::SetUp()
{
    TestArenaStack::SetUp();
    ASSERT_NE(nullptr, alloc_stack(m_available));
    ASSERT_EQ(g_max_stack, save_stack());
}

TEST_F(TestExhaustedArenaStack, rejectsAllocationAfterExhaustion)
{
    EXPECT_THROW(alloc_stack(1), PrecisionError);
}

TEST_F(TestExhaustedArenaStack, restoringStackAfterRejectionAllowsReuse)
{
    EXPECT_THROW(alloc_stack(1), PrecisionError);

    restore_stack(m_stack);

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

class TestFreedArena : public TestBigFloat
{
protected:
    void SetUp() override;
};

void TestFreedArena::SetUp()
{
    TestBigFloat::SetUp();
    float_to_bf(g_bf_x_min, 1.25);
    alloc_stack(g_bf_length + 2);
    free_bf_vars();
}

TEST_F(TestFreedArena, releasesActiveArenaState)
{
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(0, save_stack());
    EXPECT_EQ(0, g_start_stack);
    EXPECT_EQ(0, g_max_stack);
    EXPECT_EQ(0, g_bignum_max_stack_addr);
}

TEST_F(TestFreedArena, rejectsAllocationWithoutAnArena)
{
    EXPECT_THROW(alloc_stack(1), std::logic_error);
}

TEST_F(TestFreedArena, invalidInitializationLeavesNoActiveArena)
{
    EXPECT_THROW(init_bf_dec(2000), PrecisionError);

    EXPECT_EQ(BFMathType::NONE, g_bf_math);
}

TEST_F(TestFreedArena, initializesFreshStorageAfterRejectedOperations)
{
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
