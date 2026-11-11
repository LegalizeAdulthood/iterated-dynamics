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
#include <iterator>
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

namespace
{

// Encoded Pi bytes captured before the storage ownership conversion.
constexpr Byte PI_BYTES[] = {
    0x44, 0xD5, 0xDB, 0x69, 0x17, 0xDF, 0x2E, 0x56, 0x87, 0x1A, 0xA0, 0x8C, 0x6F, 0xCA, 0xBB, 0x57, 0x5C, 0x9E, 0x82,
    0xDF, 0x00, 0x3E, 0x48, 0x7B, 0x31, 0x53, 0x60, 0x87, 0x23, 0xFD, 0xFA, 0xB5, 0x3D, 0x32, 0xAB, 0x52, 0x05, 0xAD,
    0xC8, 0x1E, 0x50, 0x2F, 0x15, 0x6B, 0x61, 0xFD, 0xDF, 0x16, 0x75, 0x3C, 0xF8, 0x22, 0x32, 0xDB, 0xF8, 0xE9, 0xA5,
    0x8E, 0xCC, 0xA3, 0x1F, 0xFB, 0xFE, 0x25, 0x9F, 0x67, 0x79, 0x72, 0x2C, 0x40, 0xC6, 0x00, 0xA1, 0xD6, 0x0A, 0x32,
    0x60, 0x1A, 0xBD, 0xC0, 0x79, 0x55, 0xDB, 0xFB, 0xD3, 0xB9, 0x39, 0x5F, 0x0B, 0xD2, 0x0F, 0x74, 0xC8, 0x45, 0x57,
    0xA8, 0xCB, 0xC0, 0xB3, 0x4B, 0x2E, 0x19, 0x07, 0x28, 0x0F, 0x66, 0xFD, 0x4A, 0x33, 0xDE, 0x04, 0xD0, 0xE3, 0xBE,
    0x09, 0xBD, 0x5E, 0xAF, 0x44, 0x45, 0x81, 0xCC, 0x2C, 0x95, 0x30, 0x9B, 0x1F, 0x51, 0xFC, 0x6D, 0x6F, 0xEC, 0x52,
    0x3B, 0xEB, 0xB2, 0x39, 0x13, 0xB5, 0x53, 0x6C, 0x3E, 0xAF, 0x6F, 0xFB, 0x68, 0x63, 0x24, 0x6A, 0x19, 0xC2, 0x9E,
    0x5C, 0x5E, 0xC4, 0x60, 0x9F, 0x40, 0xB6, 0x4F, 0xA9, 0xC1, 0xBA, 0x06, 0xC0, 0x04, 0xBD, 0xE0, 0x6C, 0x97, 0x3B,
    0x4C, 0x79, 0xB6, 0x1A, 0x50, 0xFE, 0xE3, 0xF7, 0xDE, 0xE8, 0xF6, 0xD8, 0x79, 0xD4, 0x25, 0x7B, 0x1B, 0x99, 0x80,
    0xC9, 0x72, 0x53, 0x07, 0x9B, 0xC0, 0xF1, 0x49, 0xD3, 0xEA, 0x0F, 0xDB, 0x48, 0x12, 0x0A, 0xD0, 0x24, 0xD7, 0xD0,
    0x37, 0x3D, 0x02, 0x9B, 0x42, 0x72, 0xDF, 0xFE, 0x1B, 0x06, 0x77, 0x3F, 0x36, 0x62, 0xAA, 0xD3, 0x4E, 0xA6, 0x6A,
    0xC1, 0x56, 0x9F, 0x44, 0x1A, 0x40, 0x73, 0x20, 0xC1, 0x85, 0xD8, 0x75, 0x6F, 0xE0, 0xBE, 0x5E, 0x8B, 0x3B, 0xC3,
    0xA5, 0x84, 0x7D, 0xB4, 0x9F, 0x6F, 0x45, 0x19, 0x86, 0xEE, 0x8C, 0x88, 0x0E, 0x43, 0x82, 0x3E, 0x59, 0xCA, 0x66,
    0x76, 0x01, 0xAF, 0x39, 0x1D, 0x65, 0xF1, 0xA1, 0x98, 0x2A, 0xFB, 0x7E, 0x50, 0xF0, 0x3B, 0xBA, 0xE4, 0x3B, 0x7A,
    0x13, 0x6C, 0x0B, 0xEF, 0x6E, 0xA3, 0x33, 0x51, 0xAB, 0x28, 0xA7, 0x0F, 0x96, 0x68, 0x2F, 0x54, 0xD8, 0xD2, 0xA0,
    0x51, 0x6A, 0xF0, 0x88, 0xD3, 0xAB, 0x61, 0x9C, 0x0C, 0x67, 0x9A, 0x6C, 0xE9, 0xF6, 0x42, 0x68, 0xC6, 0x21, 0x5E,
    0x9B, 0x1F, 0x9E, 0x4A, 0xF0, 0xC8, 0x69, 0x04, 0x20, 0x84, 0xA4, 0x82, 0x44, 0x0B, 0x2E, 0x39, 0x42, 0xF4, 0x83,
    0xF3, 0x6F, 0x6D, 0x0F, 0xC5, 0xAC, 0x96, 0xD3, 0x81, 0x3E, 0x89, 0x23, 0x88, 0x1B, 0x65, 0xEB, 0x02, 0x23, 0x26,
    0xDC, 0xB1, 0x75, 0x85, 0xE9, 0x5D, 0x5D, 0x84, 0xEF, 0x32, 0x80, 0xEC, 0x5D, 0x60, 0xAC, 0x7C, 0x48, 0x91, 0xA9,
    0x21, 0xFB, 0xCC, 0x09, 0xD8, 0x61, 0x93, 0x21, 0x28, 0x66, 0x1B, 0xE8, 0xBF, 0xC4, 0xAF, 0xB9, 0x4B, 0x6B, 0x98,
    0x48, 0x8F, 0x3B, 0x77, 0x86, 0x95, 0x28, 0x81, 0x53, 0x32, 0x7A, 0x5C, 0xCF, 0x24, 0x6C, 0x33, 0xBA, 0xD6, 0xAF,
    0x1E, 0x93, 0x87, 0x9B, 0x16, 0x3E, 0x5C, 0xCE, 0xF6, 0x31, 0x18, 0x74, 0x5D, 0xC5, 0xA9, 0x2B, 0x2A, 0xBC, 0x6F,
    0x63, 0x11, 0x14, 0xEE, 0xB3, 0x93, 0xE9, 0x72, 0x7C, 0xAF, 0x86, 0x54, 0xA1, 0xCE, 0xE8, 0x41, 0x11, 0x34, 0x5C,
    0xCC, 0xB4, 0xB6, 0x10, 0xAB, 0x2A, 0x6A, 0x39, 0xCA, 0x55, 0x40, 0x14, 0xE8, 0x63, 0x62, 0x98, 0x48, 0x57, 0x94,
    0xAB, 0x55, 0xAA, 0xF3, 0x25, 0x55, 0xE6, 0x60, 0x5C, 0x60, 0x55, 0xDA, 0x2F, 0xAF, 0x78, 0x27, 0x4B, 0x31, 0xBD,
    0xC1, 0x77, 0x15, 0xD7, 0x3E, 0x8A, 0x1E, 0xB0, 0x8B, 0x0E, 0x9E, 0x6C, 0x0E, 0x18, 0x3A, 0x60, 0xB0, 0xDC, 0x79,
    0x8E, 0xEF, 0x38, 0xDB, 0xB8, 0x18, 0x79, 0x41, 0xCA, 0xF0, 0x85, 0x60, 0x28, 0x23, 0xB0, 0xD1, 0xC5, 0x13, 0x60,
    0xF2, 0x2A, 0x39, 0xD5, 0x30, 0x9C, 0xB5, 0x59, 0x5A, 0xC2, 0x1D, 0xA4, 0x54, 0x7B, 0xEE, 0x4A, 0x15, 0x82, 0x58,
    0xCD, 0x8B, 0x71, 0x58, 0xB6, 0x8E, 0x72, 0x8F, 0x74, 0x95, 0x0D, 0x7E, 0x3D, 0x93, 0xF4, 0xA3, 0xFE, 0x58, 0xA4,
    0x69, 0x4E, 0x57, 0x71, 0xD8, 0x20, 0x69, 0x63, 0x16, 0xFC, 0x8E, 0x85, 0xE2, 0xF2, 0x01, 0x08, 0xF7, 0x6C, 0x91,
    0xB3, 0x47, 0x99, 0xA1, 0x24, 0x99, 0x7F, 0x2C, 0xF1, 0x45, 0x90, 0x7C, 0xBA, 0x96, 0x7E, 0x26, 0x6A, 0xED, 0xAF,
    0xE1, 0xB8, 0xB7, 0xDF, 0x1A, 0xD0, 0xDB, 0x72, 0xFD, 0x2F, 0xAC, 0xB5, 0xDF, 0x98, 0xA6, 0x0B, 0x31, 0xD1, 0x1B,
    0xFB, 0x79, 0x89, 0xD9, 0xD5, 0x16, 0x92, 0x17, 0x09, 0x47, 0xB5, 0xB5, 0xD5, 0x84, 0x3F, 0xDD, 0x50, 0x7C, 0xC9,
    0xB7, 0x29, 0xAC, 0xC0, 0x6C, 0x0C, 0xE9, 0x34, 0xCF, 0x66, 0x54, 0xBE, 0x77, 0x13, 0xD0, 0x38, 0xE6, 0x21, 0x28,
    0x45, 0x89, 0x6C, 0x4E, 0xEC, 0x98, 0xFA, 0x2E, 0x08, 0xD0, 0x31, 0x9F, 0x29, 0x22, 0x38, 0x09, 0xA4, 0x44, 0x73,
    0x70, 0x03, 0x2E, 0x8A, 0x19, 0x13, 0xD3, 0x08, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24,
    /* . */ 0x03, 0x00, 0x00, 0x00
    //  <- up to g_int_length 4 ->
    // or BigFloat int length of 2 + 2 byte exp
};

} // namespace

class TestBigFloatPiStorage : public TestBigFloat, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
    static BigFloatPtr pi_data();
    static void expect_pi_bytes();
};

void TestBigFloatPiStorage::SetUp()
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(GetParam());
    ASSERT_TRUE(g_bf_pi);
    EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &g_bf_pi.arena());
}

BigFloatPtr TestBigFloatPiStorage::pi_data()
{
    return g_bf_pi.data();
}

void TestBigFloatPiStorage::expect_pi_bytes()
{
    const std::vector<Byte> expected(std::end(PI_BYTES) - g_bf_length - 2, std::end(PI_BYTES));
    EXPECT_EQ(expected, (std::vector<Byte>{pi_data(), pi_data() + g_bf_length + 2}));
}

TEST_P(TestBigFloatPiStorage, tableCopyUsesTheExistingWorkingSlotAndLeavesPaddingUntouched)
{
    auto &working = bf_arena(BigLifetime::WORKING);
    auto *const original = pi_data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const auto high_water = working.high_water_mark();
    const auto stack = save_stack();

    init_big_pi();

    expect_pi_bytes();
    EXPECT_EQ(original, pi_data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(std::vector<Byte>(working.block_size() - g_bf_length - 2, Byte{0xa5}),
        (std::vector<Byte>{pi_data() + g_bf_length + 2, pi_data() + working.block_size()}));
}

TEST_P(TestBigFloatPiStorage, repeatedBindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    init_big_pi();
    auto &working = bf_arena(BigLifetime::WORKING);
    auto *const original = pi_data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const auto high_water = working.high_water_mark();

    init_bf_length(GetParam());

    EXPECT_EQ(original, pi_data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{pi_data(), pi_data() + working.block_size()}));
    init_big_pi();
    expect_pi_bytes();
}

TEST_P(TestBigFloatPiStorage, precisionChangesClearPiAndTableCopiesUseTheRelocatedSlot)
{
    init_big_pi();
    std::fill_n(pi_data(), bf_arena(BigLifetime::WORKING).block_size(), Byte{0xa5});

    init_bf_length(GetParam() == 708 ? 4 : 708);

    auto &working = bf_arena(BigLifetime::WORKING);
    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{pi_data(), pi_data() + working.block_size()}));
    init_big_pi();
    expect_pi_bytes();
    std::fill_n(pi_data(), working.block_size(), Byte{0x5a});

    init_bf_length(GetParam());

    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{pi_data(), pi_data() + working.block_size()}));
    init_big_pi();
    expect_pi_bytes();
}

TEST_P(TestBigFloatPiStorage, temporaryRewindsPreservePiStorage)
{
    init_big_pi();
    auto *const original = pi_data();
    const auto stack = save_stack();

    bf_stack().allocate_double();
    restore_stack(stack);

    EXPECT_EQ(original, pi_data());
    expect_pi_bytes();
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

    expect_pi_bytes();
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatPiStorage, testing::Values(4, 12, 64, 708));

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
        ln_bf(result.data(), input.data());
    }
    else
    {
        atan2_bf(result.data(), input.data(), one.data());
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
    EXPECT_EQ(test.expected, bf_to_string(result.data(), 12));
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

        str_to_bf(value.data(), text);

        EXPECT_EQ(GetParam().expected, bf_to_float(value.data()));
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
        str_to_bf(m_value.data(), text);
    }
    else
    {
        clear_bf(m_value.data());
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

    EXPECT_EQ("0.0", bf_to_string(m_value.data(), 5));
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
    EXPECT_EQ("1.25", bf_to_string(m_value.data(), 5));
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
    EXPECT_EQ(GetParam().expected, bf_to_string(m_value.data(), GetParam().decimals));
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
    EXPECT_EQ(GetParam().expected, bf_to_string_f(m_value.data(), GetParam().decimals));
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
    EXPECT_EQ(GetParam().expected, bf_to_string_e(m_value.data(), GetParam().decimals));
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

    EXPECT_EQ("-1.25", bf_to_string(m_value.data(), g_bf_decimals));
    EXPECT_EQ("-1.25", bf_to_string_f(m_value.data(), g_bf_decimals));
    EXPECT_EQ("-1.25e0", bf_to_string_e(m_value.data(), g_bf_decimals));
}

TEST_F(TestBigFloatFormat, repeatedFormattingPreservesTheValue)
{
    prepare_value("-1.234567890123456789");
    const std::string original{
        reinterpret_cast<const char *>(m_value.data()), static_cast<std::size_t>(g_bf_length + 2)};
    const BigFloat &input = m_value;

    EXPECT_EQ("-1.23", bf_to_string(input.data(), 3));
    EXPECT_EQ("-1.234567890123456789e0", bf_to_string_e(input.data(), 0));
    EXPECT_EQ("-1.2346", bf_to_string_f(input.data(), 5));
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
        return std::array<BigFloat *, 22>{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd,
            &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5],
            &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max,
            &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    };
    std::vector<std::vector<Byte>> expected;
    for (BigFloat *value : parameters())
    {
        str_to_bf(value->data(), "-1.2345678901234567890123456789e-200");
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
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4],
        &g_bf_params[5], &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min,
        &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    std::vector<char> expected(values.size() * width);
    for (std::size_t field = 0; field < values.size(); ++field)
    {
        for (std::size_t byte = 0; byte < width; ++byte)
        {
            const Byte value = static_cast<Byte>(field * 17 + byte * 53);
            values[field]->data()[byte] = value;
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
    std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd,
        &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5],
        &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max,
        &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        float_to_bf(values[i]->data(), -static_cast<LDouble>(i + 1));
    }
    const int next_length = GetParam() == 4 ? 32 : 4;
    init_bf_length(next_length);
    values = {&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd, &g_bf_params[0],
        &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5], &g_bf_params[6],
        &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min,
        &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(-static_cast<LDouble>(i + 1), bf_to_float(values[i]->data()));
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
    testing::Combine(testing::Values(4, 12, 64, 708),
        testing::Values(KernelSlot::TMP1, KernelSlot::TMP2, KernelSlot::TMP3, KernelSlot::TMP4, KernelSlot::TMP5,
            KernelSlot::TMP6, KernelSlot::COPY1, KernelSlot::COPY2)));

} // namespace id::test
