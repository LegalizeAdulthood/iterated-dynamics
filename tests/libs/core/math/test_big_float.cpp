// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
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
static_assert(std::is_same_v<decltype(g_bf_pi), BigFloat>);

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

class TestBigFloatPiStorage : public TestBigFloat, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
    static void expect_pi_value();
};

void TestBigFloatPiStorage::SetUp()
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(GetParam());
    ASSERT_TRUE(g_bf_pi);
    EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &g_bf_pi.arena());
}

void TestBigFloatPiStorage::expect_pi_value()
{
    EXPECT_NEAR(3.141592653589793238L, g_bf_pi.to_float(), 0.00002L);
}

TEST_P(TestBigFloatPiStorage, initializationUsesTheExistingWorkingSlotAndLeavesPaddingUntouched)
{
    BigArena &working = bf_arena(BigLifetime::WORKING);
    Byte *const original = g_bf_pi.data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark stack = save_stack();

    init_big_pi();

    expect_pi_value();
    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(std::vector<Byte>(working.block_size() - g_bf_length - 2, Byte{0xa5}),
        (std::vector<Byte>{g_bf_pi.data() + g_bf_length + 2, g_bf_pi.data() + working.block_size()}));
}

TEST_P(TestBigFloatPiStorage, repeatedBindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    init_big_pi();
    BigArena &working = bf_arena(BigLifetime::WORKING);
    Byte *const original = g_bf_pi.data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const std::size_t high_water = working.high_water_mark();

    init_bf_length(GetParam());

    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + working.block_size()}));
    init_big_pi();
    expect_pi_value();
}

TEST_P(TestBigFloatPiStorage, precisionChangesClearPiAndTableCopiesUseTheRelocatedSlot)
{
    init_big_pi();
    std::fill_n(g_bf_pi.data(), bf_arena(BigLifetime::WORKING).block_size(), Byte{0xa5});

    init_bf_length(GetParam() == 708 ? 4 : 708);

    BigArena &working = bf_arena(BigLifetime::WORKING);
    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + working.block_size()}));
    init_big_pi();
    expect_pi_value();
    std::fill_n(g_bf_pi.data(), working.block_size(), Byte{0x5a});

    init_bf_length(GetParam());

    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + working.block_size()}));
    init_big_pi();
    expect_pi_value();
}

TEST_P(TestBigFloatPiStorage, temporaryRewindsPreservePiStorage)
{
    init_big_pi();
    Byte *const original = g_bf_pi.data();
    const BigStackMark stack = save_stack();
    const std::vector<Byte> before(original, original + g_bf_length + 2);

    bf_stack().allocate_double();
    restore_stack(stack);

    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(before, (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + g_bf_length + 2}));
}

TEST_P(TestBigFloatPiStorage, repeatedCleanupUnbindsPiAndReinitializationRestoresItsBytes)
{
    init_big_pi();

    free_bf_vars();

    EXPECT_FALSE(g_bf_pi);

    free_bf_vars();

    EXPECT_FALSE(g_bf_pi);

    init_bf_length(GetParam());
    init_big_pi();

    expect_pi_value();
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatPiStorage, testing::Values(4, 12, 56, 64, 708));

struct BigFloatRampInput
{
    const char *name;
    bool logarithm;
    const char *input;
    const char *expected;
};

class TestBigFloatWorkspaceRamp : public TestBigFloat,
                                  public testing::WithParamInterface<std::tuple<int, BigFloatRampInput>>
{
};

TEST_P(TestBigFloatWorkspaceRamp, preservesOperandsAndRestoresBorrowedWorkspaceWindows)
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    init_big_pi();
    const auto &test = std::get<1>(GetParam());
    BigFloat input{test.input};
    BigFloat result{0};
    BigFloat one{1};
    const std::vector<Byte> original(input.data(), input.data() + g_bf_length + 2);
    auto *const tmp3 = g_bf_tmp3.data();
    auto *const tmp5 = g_bf_tmp5.data();
    auto *const pi = g_bf_pi.data();
    const std::vector<Byte> original_pi(pi, pi + g_bf_length + 2);
    const int length = g_bf_length;
    const int bn_length = g_bn_length;
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const auto stack = save_stack();

    if (test.logarithm)
    {
        ln_bf(result, input);
    }
    else
    {
        atan2_bf(result, input, one);
    }

    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(tmp3, g_bf_tmp3.data());
    EXPECT_EQ(tmp5, g_bf_tmp5.data());
    EXPECT_EQ(pi, g_bf_pi.data());
    EXPECT_EQ(original_pi, (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + g_bf_length + 2}));
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(original, (std::vector<Byte>{input.data(), input.data() + g_bf_length + 2}));
    EXPECT_EQ(test.expected, bf_to_string(result, 12));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndOperations, TestBigFloatWorkspaceRamp,
    testing::Combine(testing::Values(32, 64, 708),
        testing::Values(BigFloatRampInput{"LogTwo", true, "2", "0.69314718056"},
            BigFloatRampInput{"AtanHalf", false, "0.5", "0.463647609001"},
            BigFloatRampInput{"AtanTwo", false, "2", "1.10714871779"})),
    [](const testing::TestParamInfo<TestBigFloatWorkspaceRamp::ParamType> &info)
    { return std::string{std::get<1>(info.param).name} + '_' + std::to_string(std::get<0>(info.param)); });

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
    const BigStackMark marker = save_stack();
    {
        BigFloat value{bf_arena(BigLifetime::TEMPORARY)};

        str_to_bf(value, text);

        EXPECT_EQ(GetParam().expected, bf_to_float(value));
    }
    EXPECT_EQ(marker, save_stack());
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatParse,
    testing::Values(BigFloatParseInput{"PlainDecimal", "1.25", 4, 1.25L},
        BigFloatParseInput{"UnterminatedView", "1.25x", 4, 1.25L}, BigFloatParseInput{"Exponent", "1.25e2", 6, 125.0L}),
    [](const testing::TestParamInfo<BigFloatParseInput> &info) { return info.param.name; });

class TestBigFloatFormat : public TestBigFloat
{
protected:
    void TearDown() override;
    void prepare_value(const char *text);

    BigFloat m_value{};
};

void TestBigFloatFormat::prepare_value(const char *text)
{
    BigFloat value{bf_arena(BigLifetime::TEMPORARY)};
    std::destroy_at(&m_value);
    ::new (&m_value) BigFloat{std::move(value)};
    if (text)
    {
        str_to_bf(m_value, text);
    }
    else
    {
        clear_bf(m_value);
    }
}

void TestBigFloatFormat::TearDown()
{
    m_value = BigFloat{};
    TestBigFloat::TearDown();
}

TEST_F(TestBigFloatFormat, generalFormatHandlesZero)
{
    prepare_value(nullptr);

    EXPECT_EQ("0.0", bf_to_string(m_value, 5));
}

TEST_F(TestBigFloatFormat, resetReleasesItsTemporarySlot)
{
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    BigArena::Allocation probe = temporary.allocate_single();
    const BigArena::Index index = probe.index();
    probe.reset();
    const BigStackMark marker = save_stack();

    prepare_value("1.25");
    EXPECT_EQ(&temporary, &m_value.arena());
    EXPECT_EQ("1.25", bf_to_string(m_value, 5));
    EXPECT_EQ(marker, save_stack());

    m_value = BigFloat{};
    ASSERT_FALSE(m_value);

    BigArena::Allocation reused = temporary.allocate_single();
    EXPECT_EQ(index, reused.index());
    EXPECT_EQ(marker, save_stack());
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
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string(input, GetParam().decimals));
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
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string_f(input, GetParam().decimals));
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
    const BigFloat &input = m_value;
    EXPECT_EQ(GetParam().expected, bf_to_string_e(input, GetParam().decimals));
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

enum class BigFloatFormatMode
{
    GENERAL,
    SCIENTIFIC,
    FIXED
};

class TestBigFloatFormatterStorage : public TestBigFloatFormat,
                                     public testing::WithParamInterface<std::tuple<int, BigFloatFormatMode, bool>>
{
};

TEST_P(TestBigFloatFormatterStorage, preservesInputScratchBoundsSharedLengthsAndExistingReservations)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatFormatMode mode = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    prepare_value(negative ? "-1.25" : "1.25");
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::size_t stride = m_value.arena().block_size();
    const std::size_t copy_size = 2 * bf_arena(BigLifetime::WORKING).block_size();
    std::fill(m_value.data() + size, m_value.data() + stride, Byte{0xa5});
    const std::vector<Byte> original(m_value.data(), m_value.data() + stride);
    std::fill_n(g_bf_tmp_copy1.data(), copy_size, Byte{0x5a});
    std::fill_n(g_bf_tmp_copy2.data(), copy_size, Byte{0x3c});
    const Byte *const input = m_value.data();
    const Byte *const scratch = g_bf_tmp_copy1.data();
    const std::size_t working_high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const std::size_t temporary_high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();
    const BigStackMark mark = save_stack();
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;
    const int int_length = g_int_length;
    std::string expected{negative ? "-1.25" : "1.25"};
    std::string actual;

    switch (mode)
    {
    case BigFloatFormatMode::GENERAL:
        actual = bf_to_string(m_value, 12);
        break;
    case BigFloatFormatMode::SCIENTIFIC:
        expected += "e0";
        actual = bf_to_string_e(m_value, 12);
        break;
    case BigFloatFormatMode::FIXED:
        actual = bf_to_string_f(m_value, 12);
        break;
    }

    EXPECT_EQ(expected, actual);
    EXPECT_EQ(original, std::vector<Byte>(m_value.data(), m_value.data() + stride));
    EXPECT_EQ(std::vector<Byte>(size, 0), std::vector<Byte>(scratch, scratch + size));
    EXPECT_EQ(std::vector<Byte>(copy_size - size, 0x5a), std::vector<Byte>(scratch + size, scratch + copy_size));
    EXPECT_EQ(std::vector<Byte>(copy_size, 0x3c),
        std::vector<Byte>(g_bf_tmp_copy2.data(), g_bf_tmp_copy2.data() + copy_size));
    EXPECT_EQ(input, m_value.data());
    EXPECT_EQ(scratch, g_bf_tmp_copy1.data());
    EXPECT_EQ(working_high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(temporary_high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsModesAndSigns, TestBigFloatFormatterStorage,
    testing::Combine(testing::Values(4, 12, 64, 708),
        testing::Values(BigFloatFormatMode::GENERAL, BigFloatFormatMode::SCIENTIFIC, BigFloatFormatMode::FIXED),
        testing::Bool()));

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
    const std::string original{
        reinterpret_cast<const char *>(m_value.data()), static_cast<std::size_t>(g_bf_length + 2)};
    const BigFloat &input = m_value;

    EXPECT_EQ("-1.23", bf_to_string(input, 3));
    EXPECT_EQ("-1.234567890123456789e0", bf_to_string_e(input, 0));
    EXPECT_EQ("-1.2346", bf_to_string_f(input, 5));
    EXPECT_EQ(original, (std::string{reinterpret_cast<const char *>(m_value.data()), original.size()}));
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
    float_to_bf(g_bf_x_min, 1.25);
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
    const auto stack{save_stack()};

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
    constexpr std::size_t OLD_ARENA_SIZE{64 * 1024};
    const auto stack = save_stack();
    float_to_bf(g_bf_x_min, 1.25);
    {
        BigStackSaver saved;
        const std::size_t stride = bf_arena(BigLifetime::TEMPORARY).block_size();
        for (std::size_t bytes = 0; bytes < OLD_ARENA_SIZE; bytes += stride)
        {
            bf_stack().allocate_single();
        }
        Byte *const value = bf_stack().allocate_single();
        const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
        const std::vector<Byte> bytes(size, 0x5a);
        std::copy(bytes.begin(), bytes.end(), value);
        EXPECT_GT(big_memory_stats().temporaries, OLD_ARENA_SIZE);
        EXPECT_EQ(bytes, (std::vector<Byte>{value, value + size}));
        EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
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
    BigFloat{"1.25"}.copy_to_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    BigFloat observed_float = BigFloat::from_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(1.25L, bf_to_float(observed_float));
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
    float_to_bf(g_bf_x_min, 1.25);
    bf_stack().allocate_single();
    free_bf_vars();
}

TEST_F(TestFreedArena, releasesActiveArenaBindings)
{
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_FALSE(g_bf_x_min);
    EXPECT_FALSE(g_bn_tmp2);
    EXPECT_FALSE(g_bf_tmp1);
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

    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
    BigFloatPtr value = bf_stack().allocate_single();
    BigFloat{"1.25"}.copy_to_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    BigFloat observed_float = BigFloat::from_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(1.25L, bf_to_float(observed_float));
}

TEST_F(TestBigFloat, loweringPrecisionPreservesCornersWithoutOverrunningArena)
{
    init_bf_dec(1200);
    float_to_bf(g_bf_x_min, 1.25);
    init_bf_dec(20);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_NO_THROW(bf_stack().allocate_single());
}

TEST_F(TestBigFloat, precisionChangesPreserveAllCornersSavedCornersAndParameters)
{
    const auto parameters = []
    {
        return std::array<BigFloat *, 22>{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd,
            &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5],
            &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max,
            &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    };
    std::vector<std::vector<Byte>> expected;
    for (BigFloat *value : parameters())
    {
        str_to_bf(*value, "-1.2345678901234567890123456789e-200");
        value->data()[0] = static_cast<Byte>(expected.size() + 1);
        expected.emplace_back(value->data(), value->data() + g_bf_length + 2);
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
            EXPECT_EQ(converted, (std::vector<Byte>{values[i]->data(), values[i]->data() + g_bf_length + 2}));
            expected[i] = std::move(converted);
        }
    }
}

TEST_F(TestBigFloat, nestedStackRewindsPreserveOuterValuesAndPersistentCorners)
{
    const auto start = save_stack();
    float_to_bf(g_bf_x_min, -1.25);
    {
        BigStackSaver outer;
        BigNumPtr fixed = bn_stack().allocate_single();
        BigFloatPtr floating = bf_stack().allocate_single();
        BigNum{2.5L}.copy_to_legacy(fixed, g_bn_length);
        BigFloat{-3.75}.copy_to_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            BigNum{-7.5L}.copy_to_legacy(bn_stack().allocate_double(), g_bn_length);
            BigFloat{8.25}.copy_to_legacy(bf_stack().allocate_double(), static_cast<std::size_t>(g_bf_length + 2));
            EXPECT_GT(save_stack(), middle);
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_EQ(2.5L, BigNum::from_legacy(fixed, g_bn_length).to_float());
        BigFloat observed_float = BigFloat::from_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(-3.75L, bf_to_float(observed_float));
        EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min));
    }
    EXPECT_EQ(start, save_stack());
    EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min));
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
    BigFloat{-2.5}.copy_to_legacy(persistent.data(), static_cast<std::size_t>(g_bf_length + 2));
    BigNum{3.75L}.copy_to_legacy(working.data(), g_bn_length);
    const auto before = save_stack();
    {
        BigStackSaver scope;
        BigNum{-5.5L}.copy_to_legacy(bn_stack().allocate_single(), g_bn_length);
        BigFloat{6.25}.copy_to_legacy(bf_stack().allocate_double(), static_cast<std::size_t>(g_bf_length + 2));
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
    BigFloat observed_float = BigFloat::from_legacy(persistent.data(), static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(-2.5L, bf_to_float(observed_float));
    EXPECT_EQ(3.75L, BigNum::from_legacy(working.data(), g_bn_length).to_float());
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
        BigFloat{-2.5}.copy_to_legacy(wide.data() + g_padding, static_cast<std::size_t>(g_bf_length + 2));
        wide.reduce();
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            BigFloatPtr half = bf_stack().allocate_single();
            EXPECT_EQ(wide.data() + arena.block_size(), half);
            BigFloat{3.75}.copy_to_legacy(half, static_cast<std::size_t>(g_bf_length + 2));
            bn_stack().allocate_double();
        }
        EXPECT_EQ(middle, save_stack());
        BigFloat observed_float = BigFloat::from_legacy(wide.data(), static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(-2.5L, bf_to_float(observed_float));
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
    BigFloat{-2.5}.copy_to_legacy(floating.data(), static_cast<std::size_t>(g_bf_length + 2));
    BigNum{3.75L}.copy_to_legacy(fixed.data(), g_bn_length);
    const auto floating_index = floating.index();
    const auto fixed_index = fixed.index();

    free_bf_vars();
    EXPECT_NE(nullptr, floating.data());
    EXPECT_NE(nullptr, fixed.data());
    init_bf_length(64);

    EXPECT_EQ(floating_index, floating.index());
    EXPECT_EQ(fixed_index, fixed.index());
    BigFloat observed_float = BigFloat::from_legacy(floating.data(), static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(-2.5L, bf_to_float(observed_float));
    EXPECT_EQ(3.75L, BigNum::from_legacy(fixed.data(), g_bn_length).to_float());
    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
}

TEST_F(TestBigFloat, eachRepresentationReducesProductsThroughItsConfiguredFunction)
{
    BigArena::Allocation fixed = bn_arena(BigLifetime::TEMPORARY).allocate_double();
    BigArena::Allocation floating = bf_arena(BigLifetime::TEMPORARY).allocate_double();
    std::vector<Byte> expected_num(g_bn_length);
    std::vector<Byte> expected_float(g_bf_length + 2);
    expected_num.front() = 0x35;
    expected_num.back() = 0x7f;
    expected_float.front() = 0x59;
    expected_float[expected_float.size() - 2] = 0x34;
    expected_float.back() = 0xfe;
    std::copy(expected_num.begin(), expected_num.end(), fixed.data() + g_shift_factor);
    std::copy(expected_float.begin(), expected_float.end(), floating.data() + g_padding);
    const BigArena::Index fixed_index = fixed.index();
    const BigArena::Index floating_index = floating.index();

    fixed.reduce();
    floating.reduce();

    EXPECT_EQ(expected_num, (std::vector<Byte>{fixed.data(), fixed.data() + expected_num.size()}));
    EXPECT_EQ(expected_float, (std::vector<Byte>{floating.data(), floating.data() + expected_float.size()}));
    EXPECT_EQ(fixed_index, fixed.index());
    EXPECT_EQ(floating_index, floating.index());
    const BigArena::Allocation fixed_half = bn_arena(BigLifetime::TEMPORARY).allocate_single();
    const BigArena::Allocation floating_half = bf_arena(BigLifetime::TEMPORARY).allocate_single();
    EXPECT_EQ(fixed_index + 1, fixed_half.index());
    EXPECT_EQ(floating_index + 1, floating_half.index());
}

TEST_F(TestBigFloat, parameterPackingPreservesOpaqueBytesAndRejectsIncompletePayloads)
{
    std::vector<Byte> expected(22 * (g_bf_length + 2));
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        expected[i] = static_cast<Byte>(i * 53 + 157);
    }
    unpack_bf_parameters(expected);

    EXPECT_EQ(expected, pack_bf_parameters());
    const Byte *const first = g_bf_x_min.data();
    const Byte *const second = g_bf_x_max.data();
    EXPECT_TRUE(std::equal(expected.begin(), expected.begin() + g_bf_length + 2, first));
    EXPECT_TRUE(std::equal(expected.begin() + g_bf_length + 2, expected.begin() + 2 * (g_bf_length + 2), second));
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
}

class TestBigFloatParameterPrecision : public TestBigFloat, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigFloatParameterPrecision, derivesTheSavedPrecisionBeforeUnpacking)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    const int stored_bn_length = g_bn_length;
    init_bf_length(stored_bn_length);
    const std::array<int, 10> expected_lengths{g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length,
        g_r_bf_length, g_shift_factor, g_bn_step, g_decimals, g_bf_decimals};
    g_bf_x_min = 1.25L;
    g_bf_params[9] = -2.5L;
    g_bf_save_y_3rd = 3.75L;
    std::vector<Byte> payload = pack_bf_parameters();
    const std::vector<Byte> expected = payload;
    init_bf_length(std::get<1>(GetParam()));

    ASSERT_NO_THROW(unpack_bf_parameters(payload));

    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(1.25L, g_bf_x_min.to_float());
    EXPECT_EQ(-2.5L, g_bf_params[9].to_float());
    EXPECT_EQ(3.75L, g_bf_save_y_3rd.to_float());
    EXPECT_EQ(expected_lengths,
        (std::array<int, 10>{g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length,
            g_shift_factor, g_bn_step, g_decimals, g_bf_decimals}));
    EXPECT_EQ(expected, payload);
}

INSTANTIATE_TEST_SUITE_P(StoredAndActivePrecisions, TestBigFloatParameterPrecision,
    testing::Combine(testing::Values(4, 5, 12, 65, 708), testing::Values(4, 64)));

class TestBigFloatParameterPayload : public TestBigFloat, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatParameterPayload, rejectsMalformedSizesWithoutChangingPrecisionOrParameters)
{
    g_bf_x_min = 1.25L;
    const std::vector<Byte> expected = pack_bf_parameters();
    const std::array<int, 4> lengths{g_bn_length, g_bf_length, g_decimals, g_bf_decimals};
    const std::vector<Byte> malformed(GetParam(), 0);

    EXPECT_THROW(unpack_bf_parameters(malformed), std::invalid_argument);

    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(lengths, (std::array<int, 4>{g_bn_length, g_bf_length, g_decimals, g_bf_decimals}));
}

INSTANTIATE_TEST_SUITE_P(Sizes, TestBigFloatParameterPayload, testing::Values(0, 1, 132, 154, 219, 221, 242, 15730));

class TestPersistentBigFloatParameters : public TestBigFloat, public testing::WithParamInterface<int>
{
};

TEST_P(TestPersistentBigFloatParameters, preservesEverySerializedFieldAndRejectsIncompletePayload)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::size_t width = static_cast<std::size_t>(g_bf_length + 2);
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4],
        &g_bf_params[5], &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min,
        &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    std::vector<Byte> expected(values.size() * width);
    for (std::size_t field = 0; field < values.size(); ++field)
    {
        for (std::size_t byte = 0; byte < width; ++byte)
        {
            const Byte value = static_cast<Byte>(field * 17 + byte * 53);
            values[field]->data()[byte] = value;
            expected[field * width + byte] = value;
        }
    }

    EXPECT_EQ(expected, pack_bf_parameters());
    const std::vector<Byte> zeros(expected.size());
    const Byte *const address = g_bf_x_min.data();
    BigArena &arena = bf_arena(BigLifetime::PERSISTENT);
    const std::size_t high_water = arena.high_water_mark();
    std::fill(g_bf_x_min.data() + width, g_bf_x_min.data() + arena.block_size(), Byte{0xa5});
    unpack_bf_parameters(zeros);
    EXPECT_EQ(zeros, pack_bf_parameters());
    unpack_bf_parameters(expected);
    EXPECT_EQ(expected, pack_bf_parameters());
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(address, g_bf_x_min.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(std::vector<Byte>(arena.block_size() - width, 0xa5),
        std::vector<Byte>(g_bf_x_min.data() + width, g_bf_x_min.data() + arena.block_size()));
}

TEST_P(TestPersistentBigFloatParameters, preservesValuesAcrossRelocationAndClearsThemOnReinitialization)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd,
        &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5],
        &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max,
        &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        float_to_bf(*values[i], -static_cast<LDouble>(i + 1));
    }
    const int next_length = GetParam() == 4 ? 32 : 4;
    init_bf_length(next_length);
    values = {&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd, &g_bf_params[0],
        &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5], &g_bf_params[6],
        &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min,
        &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(-static_cast<LDouble>(i + 1), bf_to_float(*values[i]));
    }
    const auto high_water = bf_arena(BigLifetime::PERSISTENT).high_water_mark();
    free_bf_vars();
    init_bf_length(GetParam());
    EXPECT_EQ(std::vector<Byte>(22 * (g_bf_length + 2)), pack_bf_parameters());
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

namespace
{

enum class KernelSlot
{
    TMP1,
    TMP2,
    TMP3,
    TMP4,
    TMP5,
    TMP6,
    COPY1,
    COPY2
};

} // namespace

class TestBigFloatKernelStorage : public TestBigFloat, public testing::WithParamInterface<std::tuple<int, KernelSlot>>
{
protected:
    void SetUp() override;
    Byte *buffer() const;
    bool allocated() const;
    std::size_t reservation_size() const;
    void expect_owner() const;
};

void TestBigFloatKernelStorage::SetUp()
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
}

Byte *TestBigFloatKernelStorage::buffer() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        return g_bf_tmp1.data();
    case KernelSlot::TMP2:
        return g_bf_tmp2.data();
    case KernelSlot::TMP3:
        return g_bf_tmp3.data();
    case KernelSlot::TMP4:
        return g_bf_tmp4.data();
    case KernelSlot::TMP5:
        return g_bf_tmp5.data();
    case KernelSlot::TMP6:
        return g_bf_tmp6.data();
    case KernelSlot::COPY1:
        return g_bf_tmp_copy1.data();
    case KernelSlot::COPY2:
        return g_bf_tmp_copy2.data();
    }
    return nullptr;
}

bool TestBigFloatKernelStorage::allocated() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        return static_cast<bool>(g_bf_tmp1);
    case KernelSlot::TMP2:
        return static_cast<bool>(g_bf_tmp2);
    case KernelSlot::TMP3:
        return static_cast<bool>(g_bf_tmp3);
    case KernelSlot::TMP4:
        return static_cast<bool>(g_bf_tmp4);
    case KernelSlot::TMP5:
        return static_cast<bool>(g_bf_tmp5);
    case KernelSlot::TMP6:
        return static_cast<bool>(g_bf_tmp6);
    case KernelSlot::COPY1:
        return static_cast<bool>(g_bf_tmp_copy1);
    case KernelSlot::COPY2:
        return static_cast<bool>(g_bf_tmp_copy2);
    }
    return false;
}

std::size_t TestBigFloatKernelStorage::reservation_size() const
{
    const auto slot = std::get<1>(GetParam());
    const std::size_t width = slot == KernelSlot::COPY1 || slot == KernelSlot::COPY2 ? 2 : 1;
    return width * bf_arena(BigLifetime::WORKING).block_size();
}

void TestBigFloatKernelStorage::expect_owner() const
{
    static_assert(std::is_same_v<decltype(g_bf_tmp1), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp2), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp3), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp4), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp5), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp6), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp_copy1), BigArena::Allocation>);
    static_assert(std::is_same_v<decltype(g_bf_tmp_copy2), BigArena::Allocation>);
    auto &working = bf_arena(BigLifetime::WORKING);
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        EXPECT_EQ(&working, &g_bf_tmp1.arena());
        break;
    case KernelSlot::TMP2:
        EXPECT_EQ(&working, &g_bf_tmp2.arena());
        break;
    case KernelSlot::TMP3:
        EXPECT_EQ(&working, &g_bf_tmp3.arena());
        break;
    case KernelSlot::TMP4:
        EXPECT_EQ(&working, &g_bf_tmp4.arena());
        break;
    case KernelSlot::TMP5:
        EXPECT_EQ(&working, &g_bf_tmp5.arena());
        break;
    case KernelSlot::TMP6:
        EXPECT_EQ(&working, &g_bf_tmp6.arena());
        break;
    case KernelSlot::COPY1:
        EXPECT_EQ(&working, &g_bf_tmp_copy1.arena());
        EXPECT_EQ(2U, g_bf_tmp_copy1.blocks());
        break;
    case KernelSlot::COPY2:
        EXPECT_EQ(&working, &g_bf_tmp_copy2.arena());
        EXPECT_EQ(2U, g_bf_tmp_copy2.blocks());
        break;
    }
}

TEST_P(TestBigFloatKernelStorage, rebindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    auto *const original = buffer();
    std::fill_n(original, reservation_size(), Byte{0xa5});
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(original, buffer());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, precisionChangesDiscardTheWholeScratchReservation)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});
    const int original_length = std::get<0>(GetParam());

    init_bf_length(original_length == 708 ? 4 : 708);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
    std::fill_n(buffer(), reservation_size(), Byte{0x5a});

    init_bf_length(original_length);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, repeatedCleanupUnbindsAndReinitializationClearsScratch)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});

    free_bf_vars();

    EXPECT_FALSE(allocated());

    free_bf_vars();

    EXPECT_FALSE(allocated());

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, temporaryRewindsLeaveWorkingScratchUntouched)
{
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});
    const auto *const original = buffer();
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const auto stack = save_stack();

    bf_stack().allocate_double();
    restore_stack(stack);

    EXPECT_EQ(original, buffer());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(
        std::vector<Byte>(reservation_size(), Byte{0xa5}), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndSlots, TestBigFloatKernelStorage,
    testing::Combine(testing::Values(4, 12, 56, 64, 708),
        testing::Values(KernelSlot::TMP1, KernelSlot::TMP2, KernelSlot::TMP3, KernelSlot::TMP4, KernelSlot::TMP5,
            KernelSlot::TMP6, KernelSlot::COPY1, KernelSlot::COPY2)));

} // namespace id::test
