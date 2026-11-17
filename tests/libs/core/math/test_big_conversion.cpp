// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big.h>

#include <math/big_init.h>
#include <math/BigArena.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

void keep_first_block(Byte *)
{
}

} // namespace

class TestBigNumConversionBase : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<int> m_digits{g_bf_digits, 0};
};

void TestBigNumConversionBase::SetUp()
{
    init_bf_dec(20);
}

void TestBigNumConversionBase::TearDown()
{
    free_bf_vars();
}

struct BigNumConversionInput
{
    const char *name;
    int length;
    int integer_length;
    std::vector<Byte> expected;
};

class TestBigNumByteConversion : public TestBigNumConversionBase,
                                 public testing::WithParamInterface<BigNumConversionInput>
{
};

TEST_P(TestBigNumByteConversion, conversionPreservesFixedPointBytePositionsAndTruncation)
{
    const int active_length = g_bn_length;
    const int active_integer_length = g_int_length;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe};
    const std::vector<Byte> original = source;
    const BigNumConversionInput &input = GetParam();
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    EXPECT_EQ(0, convert_bn(destination.data() + 1, source.data(), input.length, input.integer_length, 8, 2));

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
    EXPECT_EQ(active_length, g_bn_length);
    EXPECT_EQ(active_integer_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumByteConversion,
    testing::Values(
        BigNumConversionInput{"WidenFraction", 12, 2, {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"NarrowFraction", 4, 2, {0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"SameLayout", 8, 2, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"WidenInteger", 12, 4, {0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0, 0}},
        BigNumConversionInput{"NarrowInteger", 8, 1, {0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}}),
    [](const testing::TestParamInfo<BigNumConversionInput> &info) { return info.param.name; });

class TestBigNumSignedConversion : public TestBigNumConversionBase, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigNumSignedConversion, integerWideningPreservesTheSignWithoutChangingTheSourceOrGuards)
{
    const bool negative = GetParam();
    const Byte high_byte = negative ? 0xfe : 0x7e;
    const Byte extension = negative ? 0xff : 0;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, high_byte};
    const std::vector<Byte> original = source;
    const std::vector<Byte> expected{0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, high_byte, extension, extension};
    std::vector<Byte> destination(14, 0xcc);

    convert_bn_signed(destination.data() + 1, source.data(), 12, 4, 8, 2);

    EXPECT_EQ(expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Signs, TestBigNumSignedConversion, testing::Bool());

class TestBigArenaNumericMigration : public testing::TestWithParam<bool>
{
};

TEST_P(TestBigArenaNumericMigration, repeatedGrowthAndReductionPreserveEncodedValues)
{
    const int exponent_bytes = GetParam() ? 2 : 0;
    int old_length = 28;
    BigArena arena{static_cast<std::size_t>(old_length + exponent_bytes), 8, keep_first_block};
    std::vector<BigArena::Allocation> values;
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_double());
    const std::vector<BigArena::Index> indices{0, 1, 2, 4};
    const std::vector<int> widths{1, 1, 1, 2};
    std::vector<std::vector<Byte>> expected;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        const int length = widths[i] * old_length;
        std::vector<Byte> encoded(length + exponent_bytes);
        if (i != 2)
        {
            encoded[0] = 0x35;
            encoded[1] = 0x7b;
            encoded[length - 2] = i == 0 ? 0x7f : 0x80;
            encoded[length - 1] = i == 0 ? 0 : 0xff;
            if (GetParam())
            {
                encoded[length] = 0x19;
                encoded[length + 1] = i == 0 ? 0 : 0xff;
            }
        }
        std::copy(encoded.begin(), encoded.end(), values[i].data());
        expected.push_back(std::move(encoded));
    }

    // The first growth keeps the same aligned stride; later changes cross it.
    for (const int new_length : {60, 124, 20, 28})
    {
        SCOPED_TRACE(new_length);
        arena.relocate(static_cast<std::size_t>(new_length + exponent_bytes),
            [&](Byte *destination, const Byte *source, std::size_t width)
            {
                const int blocks = static_cast<int>(width);
                if (GetParam())
                {
                    convert_bf(destination, source, blocks * new_length, blocks * old_length);
                }
                else
                {
                    convert_bn(destination, source, blocks * new_length, blocks * 2, blocks * old_length, blocks * 2);
                }
            });

        for (std::size_t i = 0; i < values.size(); ++i)
        {
            const int old_mantissa = widths[i] * old_length;
            const int new_mantissa = widths[i] * new_length;
            const int retained = std::min(old_mantissa, new_mantissa);
            std::vector<Byte> converted(new_mantissa + exponent_bytes);
            std::copy_n(expected[i].data() + old_mantissa - retained, retained + exponent_bytes,
                converted.data() + new_mantissa - retained);
            expected[i] = std::move(converted);
            EXPECT_EQ(expected[i], (std::vector<Byte>{values[i].data(), values[i].data() + expected[i].size()}));
            EXPECT_EQ(indices[i], values[i].index());
            EXPECT_EQ(&arena, &values[i].arena());
            EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values[i].data()) % 64);
        }
        EXPECT_EQ(6, arena.high_water_mark());
        EXPECT_EQ(8, arena.capacity());
        EXPECT_EQ(0, arena.block_size() % 64);
        old_length = new_length;
    }
    values.clear();
    const BigArena::Allocation reused = arena.allocate_double();
    EXPECT_EQ(6, arena.high_water_mark());
    EXPECT_EQ(0, reused.index() % 2);
}

INSTANTIATE_TEST_SUITE_P(Representations, TestBigArenaNumericMigration, testing::Bool());

} // namespace id::test
