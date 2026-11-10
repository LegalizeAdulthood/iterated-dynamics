// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

static_assert(std::is_same_v<decltype(g_bf_x_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_x_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_x_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_params), BigFloat[10]>);

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

TEST_F(TestBigFloat, conversionRelocatesTheExponentAndTruncatesLowMantissaBytes)
{
    const int active_length = g_bf_length;
    std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12};
    const auto original = source;
    const std::vector<Byte> cases[]{
        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
        {0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
        original,
    };
    for (const auto &expected : cases)
    {
        SCOPED_TRACE(expected.size());
        std::vector<Byte> destination(expected.size() + 2, 0xcc);

        EXPECT_EQ(0, convert_bf(destination.data() + 1, source.data(), static_cast<int>(expected.size()) - 2, 8));

        EXPECT_EQ(expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
        EXPECT_EQ(0xcc, destination.front());
        EXPECT_EQ(0xcc, destination.back());
        EXPECT_EQ(original, source);
        EXPECT_EQ(active_length, g_bf_length);
    }
}

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
    BigFloatPtr value{bf_stack().allocate_single()};

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

    BigFloatPtr m_value{};
};

void TestBigFloatFormat::prepare_value(const char *text)
{
    m_value = bf_stack().allocate_single();
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

class TestBigFloatGeneralFormat : public TestBigFloatFormat, public testing::WithParamInterface<BigFloatFormatInput>
{
protected:
    void SetUp() override;
};

void TestBigFloatGeneralFormat::SetUp()
{
    TestBigFloat::SetUp();
    prepare_value(GetParam().input);
}

TEST_P(TestBigFloatGeneralFormat, selectsNotationAndFormatsDecimal)
{
    EXPECT_EQ(GetParam().expected, bf_to_string(m_value, GetParam().decimals));
}

// Preserve the legacy carry truncation while refactoring decimal storage.
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatGeneralFormat,
    testing::Values(BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234"}, BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999"},
        BigFloatFormatInput{"SmallFixed", "0.00125", 8, "0.00125"},
        BigFloatFormatInput{"SmallScientific", "0.000125", 8, "1.25e-4"},
        BigFloatFormatInput{"LargeFixed", "125000", 8, "125000.0"},
        BigFloatFormatInput{"FewDigitsLargeFixed", "125000", 3, "125000.0"},
        BigFloatFormatInput{"LargeScientific", "1250000", 8, "1.25e6"},
        BigFloatFormatInput{"DefaultPrecision", "1.0000123456789012345", 0, "1.0000123456789012345"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

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
    testing::Values(BigFloatFormatInput{"Zero", nullptr, 5, "0.0"},
        BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25"}, BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234"}, BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999"},
        BigFloatFormatInput{"SmallExponent", "0.000125", 8, "0.000125"},
        BigFloatFormatInput{"LargeExponent", "1250000", 8, "1250000.0"},
        BigFloatFormatInput{"FewDigitsLargeExponent", "1250000", 3, "1250000.0"},
        BigFloatFormatInput{"SingleDigit", "1.25", 1, "1.0"},
        BigFloatFormatInput{"DefaultPrecision", "1.234567890123456789", 0, "1.234567890123456789"}),
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
    testing::Values(BigFloatFormatInput{"Zero", nullptr, 5, "0.0"},
        BigFloatFormatInput{"PositiveDecimal", "1.25", 5, "1.25e0"},
        BigFloatFormatInput{"Negative", "-1.25", 5, "-1.25e0"},
        BigFloatFormatInput{"RoundDown", "1.2344", 4, "1.234e0"},
        BigFloatFormatInput{"RoundUp", "1.2346", 4, "1.235e0"},
        BigFloatFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999e0"},
        BigFloatFormatInput{"SmallExponent", "0.000125", 8, "1.25e-4"},
        BigFloatFormatInput{"LargeExponent", "1250000", 8, "1.25e6"},
        BigFloatFormatInput{"SingleDigit", "1.25", 1, "1.0e0"},
        BigFloatFormatInput{"DefaultPrecision", "1.234567890123456789", 0, "1.234567890123456789e0"}),
    [](const testing::TestParamInfo<BigFloatFormatInput> &info) { return info.param.name; });

TEST_F(TestBigFloatFormat, formatsAtMaximumSupportedPrecision)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(708);
    prepare_value("-1.25");

    EXPECT_EQ("-1.25", bf_to_string(m_value, g_bf_decimals));
    EXPECT_EQ("-1.25", bf_to_string_f(m_value, g_bf_decimals));
    EXPECT_EQ("-1.25e0", bf_to_string_e(m_value, g_bf_decimals));
}

TEST_F(TestBigFloatFormat, repeatedFormattingPreservesTheValue)
{
    prepare_value("-1.234567890123456789");
    const std::string original{reinterpret_cast<const char *>(m_value), static_cast<std::size_t>(g_bf_length + 2)};
    BigFloatConstPtr input = m_value;

    EXPECT_EQ("-1.23", bf_to_string(input, 3));
    EXPECT_EQ("-1.234567890123456789e0", bf_to_string_e(input, 0));
    EXPECT_EQ("-1.2346", bf_to_string_f(input, 5));
    EXPECT_EQ(original, (std::string{reinterpret_cast<const char *>(m_value), original.size()}));
}

class TestActivePrecisionArena : public TestBigFloat
{
protected:
    void prepare_active_arena(FractalType type);
    void expect_preserved_arena();

    int m_length{};
    int m_decimals{};
    BigStackMark m_stack{};
    BigFloatPtr m_corner{};
};

void TestActivePrecisionArena::prepare_active_arena(const FractalType type)
{
    g_fractal_type = type;
    init_bf_dec(20);
    float_to_bf(g_bf_x_min.data(), 1.25);
    m_length = g_bf_length;
    m_decimals = g_decimals;
    m_stack = save_stack();
    m_corner = g_bf_x_min.data();
}

void TestActivePrecisionArena::expect_preserved_arena()
{
    EXPECT_EQ(m_length, g_bf_length);
    EXPECT_EQ(m_decimals, g_decimals);
    EXPECT_EQ(m_stack, save_stack());
    EXPECT_EQ(m_corner, g_bf_x_min.data());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
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
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
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
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
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
    float_to_bf(g_bf_x_min.data(), 1.25);
}

TEST_P(TestArenaPrecisionLimit, rejectsRequestsPastTheBackendLimit)
{
    const auto stack{save_stack()};

    EXPECT_THROW(init_bf_length(GetParam().length + 1), PrecisionError);

    EXPECT_EQ(GetParam().length, g_bn_length);
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
}

INSTANTIATE_TEST_SUITE_P(Backends, TestArenaPrecisionLimit,
    testing::Values(ArenaPrecisionLimit{"BigNum", FractalType::MANDEL, BFMathType::BIG_NUM, 672},
        ArenaPrecisionLimit{"BigFloat", FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, 708}),
    [](const testing::TestParamInfo<ArenaPrecisionLimit> &info) { return info.param.name; });

TEST_F(TestBigFloat, stackStorageCrossesSixtyFourKilobytes)
{
    constexpr std::size_t OLD_ARENA_SIZE{64 * 1024};
    const auto stack = save_stack();
    float_to_bf(g_bf_x_min.data(), 1.25);
    {
        BigStackSaver saved;
        const std::size_t stride = bf_arena(BigLifetime::TEMPORARY).block_size();
        for (std::size_t bytes = 0; bytes < OLD_ARENA_SIZE; bytes += stride)
        {
            bf_stack().allocate_single();
        }
        BigFloatPtr value = bf_stack().allocate_single();
        float_to_bf(value, 2.5);
        add_bf(value, value, g_bf_x_min.data());
        EXPECT_GT(big_memory_stats().temporaries, OLD_ARENA_SIZE);
        EXPECT_EQ(3.75L, bf_to_float(value));
        EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
    }
    EXPECT_EQ(stack, save_stack());
}

class TestArenaStack : public TestBigFloat
{
protected:
    void SetUp() override;
    void fill_stack();

    BigStackMark m_stack{};
    std::size_t m_available{};
};

void TestArenaStack::SetUp()
{
    TestBigFloat::SetUp();
    m_stack = save_stack();
    m_available = bf_arena(BigLifetime::TEMPORARY).capacity();
}

void TestArenaStack::fill_stack()
{
    for (std::size_t i = 0; i < m_available; ++i)
    {
        bf_stack().allocate_single();
    }
}

TEST_F(TestArenaStack, exactExhaustionAllocatesEveryRemainingBlock)
{
    EXPECT_NO_THROW(fill_stack());
    EXPECT_EQ(m_available, save_stack().big_float);
}

class TestExhaustedArenaStack : public TestArenaStack
{
protected:
    void SetUp() override;
};

void TestExhaustedArenaStack::SetUp()
{
    TestArenaStack::SetUp();
    fill_stack();
}

TEST_F(TestExhaustedArenaStack, rejectionPreservesAllocationStateForBothWidths)
{
    const auto before = save_stack();
    const auto high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();

    EXPECT_THROW(bf_stack().allocate_single(), PrecisionError);
    EXPECT_THROW(bf_stack().allocate_double(), PrecisionError);

    EXPECT_EQ(before, save_stack());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_NO_THROW(bn_stack().allocate_double());
}

TEST_F(TestExhaustedArenaStack, restoringStackAfterRejectionAllowsReuse)
{
    EXPECT_THROW(bf_stack().allocate_single(), PrecisionError);
    restore_stack(m_stack);
    BigFloatPtr value = bf_stack().allocate_single();
    str_to_bf(value, "1.25");
    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, stackSaverUnwindsAfterExhaustion)
{
    const auto stack = save_stack();
    EXPECT_THROW(
        {
            BigStackSaver saved;
            for (std::size_t i = 0; i < bf_arena(BigLifetime::TEMPORARY).capacity(); ++i)
            {
                bf_stack().allocate_single();
            }
            bf_stack().allocate_double();
        },
        PrecisionError);
    EXPECT_EQ(stack, save_stack());
    EXPECT_NO_THROW(bf_stack().allocate_single());
}

class TestFreedArena : public TestBigFloat
{
protected:
    void SetUp() override;
};

void TestFreedArena::SetUp()
{
    TestBigFloat::SetUp();
    float_to_bf(g_bf_x_min.data(), 1.25);
    bf_stack().allocate_single();
    free_bf_vars();
}

TEST_F(TestFreedArena, releasesActiveArenaBindings)
{
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_FALSE(g_bf_x_min);
    EXPECT_FALSE(g_bn_tmp1);
    EXPECT_EQ(nullptr, g_bf_tmp1);
}

TEST_F(TestFreedArena, changingMathFlagDoesNotReactivateFreedBindings)
{
    g_bf_math = BFMathType::BIG_FLT;

    EXPECT_THROW(bf_stack().allocate_single(), std::logic_error);
    EXPECT_THROW(bn_stack().allocate_double(), std::logic_error);
    EXPECT_EQ(BigStackMark{}, save_stack());
}

TEST_F(TestFreedArena, initializesFreshStorageAfterRejectedOperations)
{
    EXPECT_THROW(bf_stack().allocate_single(), std::logic_error);
    EXPECT_THROW(bn_stack().allocate_double(), std::logic_error);
    EXPECT_THROW(init_bf_dec(2000), PrecisionError);
    EXPECT_EQ(BFMathType::NONE, g_bf_math);

    init_bf_dec(20);

    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min.data()));
    BigFloatPtr value = bf_stack().allocate_single();
    str_to_bf(value, "1.25");
    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, loweringPrecisionPreservesCornersWithoutOverrunningArena)
{
    init_bf_dec(1200);
    float_to_bf(g_bf_x_min.data(), 1.25);
    init_bf_dec(20);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min.data()));
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_NO_THROW(bf_stack().allocate_single());
}

TEST_F(TestBigFloat, precisionChangesPreserveAllCornersSavedCornersAndParameters)
{
    const auto parameters = []
    {
        return std::array<BigFloatPtr, 22>{g_bf_x_min.data(), g_bf_x_max.data(), g_bf_y_min.data(), g_bf_y_max.data(),
            g_bf_x_3rd.data(), g_bf_y_3rd.data(), g_bf_params[0].data(), g_bf_params[1].data(), g_bf_params[2].data(),
            g_bf_params[3].data(), g_bf_params[4].data(), g_bf_params[5].data(), g_bf_params[6].data(),
            g_bf_params[7].data(), g_bf_params[8].data(), g_bf_params[9].data(), g_bf_save_x_min.data(),
            g_bf_save_x_max.data(), g_bf_save_y_min.data(), g_bf_save_y_max.data(), g_bf_save_x_3rd.data(),
            g_bf_save_y_3rd.data()};
    };
    std::vector<std::vector<Byte>> expected;
    for (BigFloatPtr value : parameters())
    {
        str_to_bf(value, "-1.2345678901234567890123456789e-200");
        value[0] = static_cast<Byte>(expected.size() + 1);
        expected.emplace_back(value, value + g_bf_length + 2);
    }
    for (int length : {64, 12, 32})
    {
        const int old_length = g_bf_length;
        init_bf_length(length);
        const auto values = parameters();
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            std::vector<Byte> converted(g_bf_length + 2);
            convert_bf(converted.data(), expected[i].data(), g_bf_length, old_length);
            EXPECT_EQ(converted, (std::vector<Byte>{values[i], values[i] + g_bf_length + 2}));
            expected[i] = std::move(converted);
        }
    }
}

TEST_F(TestBigFloat, nestedStackRewindsPreserveOuterValuesAndPersistentCorners)
{
    const auto start = save_stack();
    float_to_bf(g_bf_x_min.data(), -1.25);
    {
        BigStackSaver outer;
        BigNumPtr fixed = bn_stack().allocate_single();
        BigFloatPtr floating = bf_stack().allocate_single();
        float_to_bn(fixed, 2.5);
        float_to_bf(floating, -3.75);
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            float_to_bn(bn_stack().allocate_double(), -7.5);
            float_to_bf(bf_stack().allocate_double(), 8.25);
            EXPECT_GT(save_stack(), middle);
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_EQ(2.5L, bn_to_float(fixed));
        EXPECT_EQ(-3.75L, bf_to_float(floating));
        EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min.data()));
    }
    EXPECT_EQ(start, save_stack());
    EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min.data()));
}

TEST_F(TestBigFloat, eachRepresentationAndLifetimeHasItsOwnAlignedArena)
{
    std::vector<BigArena::Allocation> values;
    std::vector<const BigArena *> arenas;
    for (BigLifetime lifetime : {BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY})
    {
        for (BigArena *arena : {&bn_arena(lifetime), &bf_arena(lifetime)})
        {
            EXPECT_EQ(arenas.end(), std::find(arenas.begin(), arenas.end(), arena));
            arenas.push_back(arena);
            values.push_back(arena->allocate_single());
            EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values.back().data()) % 64);
            EXPECT_EQ(0, arena->block_size() % 64);
        }
    }
}

TEST_F(TestBigFloat, bothStackAdaptersRewindWhileOwnedPersistentAndWorkingValuesSurvive)
{
    auto persistent = bf_arena(BigLifetime::PERSISTENT).allocate_single();
    auto working = bn_arena(BigLifetime::WORKING).allocate_single();
    float_to_bf(persistent.data(), -2.5);
    float_to_bn(working.data(), 3.75);
    const auto before = save_stack();
    {
        BigStackSaver scope;
        float_to_bn(bn_stack().allocate_single(), -5.5);
        float_to_bf(bf_stack().allocate_double(), 6.25);
        const auto middle = save_stack();
        EXPECT_GT(middle.big_num, before.big_num);
        EXPECT_GT(middle.big_float, before.big_float);
        {
            BigStackSaver inner;
            bn_stack().allocate_double();
            bf_stack().allocate_single();
        }
        EXPECT_EQ(middle, save_stack());
    }
    EXPECT_EQ(before, save_stack());
    EXPECT_EQ(-2.5L, bf_to_float(persistent.data()));
    EXPECT_EQ(3.75L, bn_to_float(working.data()));
}

TEST_F(TestBigFloat, stackRewindCoalescesAReleasedNarrowedAllocationWithItsBorrowedHalf)
{
    const auto before = save_stack();
    auto &arena = bf_arena(BigLifetime::TEMPORARY);
    BigArena::Index index{};
    {
        BigStackSaver scope;
        auto wide = arena.allocate_double();
        index = wide.index();
        float_to_bf(wide.data() + g_padding, -2.5);
        wide.reduce();
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            BigFloatPtr half = bf_stack().allocate_single();
            EXPECT_EQ(wide.data() + arena.block_size(), half);
            float_to_bf(half, 3.75);
            bn_stack().allocate_double();
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_EQ(-2.5L, bf_to_float(wide.data()));
        bf_stack().allocate_single();
        // The owning first half is released before the scope rewinds the second.
    }
    EXPECT_EQ(before, save_stack());
    const auto reused = arena.allocate_double();
    EXPECT_EQ(index, reused.index());
}

TEST_F(TestBigFloat, ownedArenaValuesSurviveContextResetAndLaterPrecisionGrowth)
{
    auto floating = bf_arena(BigLifetime::WORKING).allocate_single();
    auto fixed = bn_arena(BigLifetime::PERSISTENT).allocate_single();
    float_to_bf(floating.data(), -2.5);
    float_to_bn(fixed.data(), 3.75);
    const auto floating_index = floating.index();
    const auto fixed_index = fixed.index();

    free_bf_vars();
    EXPECT_NE(nullptr, floating.data());
    EXPECT_NE(nullptr, fixed.data());
    init_bf_length(64);

    EXPECT_EQ(floating_index, floating.index());
    EXPECT_EQ(fixed_index, fixed.index());
    EXPECT_EQ(-2.5L, bf_to_float(floating.data()));
    EXPECT_EQ(3.75L, bn_to_float(fixed.data()));
    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min.data()));
}

TEST_F(TestBigFloat, eachRepresentationReducesProductsThroughItsConfiguredFunction)
{
    auto fixed = bn_arena(BigLifetime::TEMPORARY).allocate_double();
    auto floating = bf_arena(BigLifetime::TEMPORARY).allocate_double();
    std::vector<Byte> expected_num(g_bn_length);
    std::vector<Byte> expected_float(g_bf_length + 2);
    str_to_bn(expected_num.data(), "-1.234567890123456789");
    str_to_bf(expected_float.data(), "-1.234567890123456789e-200");
    std::copy(expected_num.begin(), expected_num.end(), fixed.data() + g_shift_factor);
    std::copy(expected_float.begin(), expected_float.end(), floating.data() + g_padding);
    const auto fixed_index = fixed.index();
    const auto floating_index = floating.index();

    fixed.reduce();
    floating.reduce();

    EXPECT_EQ(expected_num, (std::vector<Byte>{fixed.data(), fixed.data() + expected_num.size()}));
    EXPECT_EQ(expected_float, (std::vector<Byte>{floating.data(), floating.data() + expected_float.size()}));
    EXPECT_EQ(fixed_index, fixed.index());
    EXPECT_EQ(floating_index, floating.index());
    const auto fixed_half = bn_arena(BigLifetime::TEMPORARY).allocate_single();
    const auto floating_half = bf_arena(BigLifetime::TEMPORARY).allocate_single();
    EXPECT_EQ(fixed_index + 1, fixed_half.index());
    EXPECT_EQ(floating_index + 1, floating_half.index());
}

TEST_F(TestBigFloat, parameterPackingPreservesOpaqueBytesAndRejectsIncompletePayloads)
{
    std::vector<char> expected(22 * (g_bf_length + 2));
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        expected[i] = static_cast<char>(i * 53 + 157);
    }
    unpack_bf_parameters(expected);

    EXPECT_EQ(expected, pack_bf_parameters());
    const auto first = reinterpret_cast<const char *>(g_bf_x_min.data());
    const auto second = reinterpret_cast<const char *>(g_bf_x_max.data());
    EXPECT_TRUE(std::equal(expected.begin(), expected.begin() + g_bf_length + 2, first));
    EXPECT_TRUE(std::equal(expected.begin() + g_bf_length + 2, expected.begin() + 2 * (g_bf_length + 2), second));
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
}

class TestPersistentBigFloatParameters : public TestBigFloat, public testing::WithParamInterface<int>
{
};

TEST_P(TestPersistentBigFloatParameters, preservesEverySerializedFieldAndRejectsIncompletePayload)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::size_t width = static_cast<std::size_t>(g_bf_length + 2);
    const std::array<BigFloatPtr, 22> values{g_bf_x_min.data(), g_bf_x_max.data(), g_bf_y_min.data(), g_bf_y_max.data(),
        g_bf_x_3rd.data(), g_bf_y_3rd.data(), g_bf_params[0].data(), g_bf_params[1].data(), g_bf_params[2].data(),
        g_bf_params[3].data(), g_bf_params[4].data(), g_bf_params[5].data(), g_bf_params[6].data(),
        g_bf_params[7].data(), g_bf_params[8].data(), g_bf_params[9].data(), g_bf_save_x_min.data(),
        g_bf_save_x_max.data(), g_bf_save_y_min.data(), g_bf_save_y_max.data(), g_bf_save_x_3rd.data(),
        g_bf_save_y_3rd.data()};
    std::vector<char> expected(values.size() * width);
    for (std::size_t field = 0; field < values.size(); ++field)
    {
        for (std::size_t byte = 0; byte < width; ++byte)
        {
            const Byte value = static_cast<Byte>(field * 17 + byte * 53);
            values[field][byte] = value;
            expected[field * width + byte] = static_cast<char>(value);
        }
    }

    EXPECT_EQ(expected, pack_bf_parameters());
    const std::vector<char> zeros(expected.size());
    unpack_bf_parameters(zeros);
    EXPECT_EQ(zeros, pack_bf_parameters());
    unpack_bf_parameters(expected);
    EXPECT_EQ(expected, pack_bf_parameters());
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
}

TEST_P(TestPersistentBigFloatParameters, preservesValuesAcrossRelocationAndClearsThemOnReinitialization)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    std::array<BigFloatPtr, 22> values{g_bf_x_min.data(), g_bf_x_max.data(), g_bf_y_min.data(), g_bf_y_max.data(),
        g_bf_x_3rd.data(), g_bf_y_3rd.data(), g_bf_params[0].data(), g_bf_params[1].data(), g_bf_params[2].data(),
        g_bf_params[3].data(), g_bf_params[4].data(), g_bf_params[5].data(), g_bf_params[6].data(),
        g_bf_params[7].data(), g_bf_params[8].data(), g_bf_params[9].data(), g_bf_save_x_min.data(),
        g_bf_save_x_max.data(), g_bf_save_y_min.data(), g_bf_save_y_max.data(), g_bf_save_x_3rd.data(),
        g_bf_save_y_3rd.data()};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        float_to_bf(values[i], -static_cast<LDouble>(i + 1));
    }
    const int next_length = GetParam() == 4 ? 32 : 4;
    init_bf_length(next_length);
    values = {g_bf_x_min.data(), g_bf_x_max.data(), g_bf_y_min.data(), g_bf_y_max.data(), g_bf_x_3rd.data(),
        g_bf_y_3rd.data(), g_bf_params[0].data(), g_bf_params[1].data(), g_bf_params[2].data(), g_bf_params[3].data(),
        g_bf_params[4].data(), g_bf_params[5].data(), g_bf_params[6].data(), g_bf_params[7].data(),
        g_bf_params[8].data(), g_bf_params[9].data(), g_bf_save_x_min.data(), g_bf_save_x_max.data(),
        g_bf_save_y_min.data(), g_bf_save_y_max.data(), g_bf_save_x_3rd.data(), g_bf_save_y_3rd.data()};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(-static_cast<LDouble>(i + 1), bf_to_float(values[i]));
    }
    const auto high_water = bf_arena(BigLifetime::PERSISTENT).high_water_mark();
    free_bf_vars();
    init_bf_length(GetParam());
    EXPECT_EQ(std::vector<char>(22 * (g_bf_length + 2)), pack_bf_parameters());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::PERSISTENT).high_water_mark());
}

TEST_P(TestPersistentBigFloatParameters, ownsPersistentSlotsAndBecomesNullAtCleanup)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4],
        &g_bf_params[5], &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min,
        &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (const BigFloat *value : values)
    {
        EXPECT_TRUE(*value);
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &value->arena());
    }

    free_bf_vars();

    for (const BigFloat *value : values)
    {
        EXPECT_FALSE(*value);
    }
    init_bf_length(GetParam());
    for (const BigFloat *value : values)
    {
        EXPECT_TRUE(*value);
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &value->arena());
    }
}

INSTANTIATE_TEST_SUITE_P(Precision, TestPersistentBigFloatParameters, testing::Values(4, 708),
    [](const testing::TestParamInfo<int> &info) { return info.param == 4 ? "Minimum" : "Maximum"; });

} // namespace id::test
