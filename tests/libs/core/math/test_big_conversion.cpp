// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigArena.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
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

enum class BigFloatByteProcedure
{
    COPY,
    CLEAR,
    MAXIMUM
};

class TestBigFloatByteProcedures : public TestBigNumConversionBase,
                                   public testing::WithParamInterface<std::tuple<int, BigFloatByteProcedure>>
{
};

TEST_P(TestBigFloatByteProcedures, preservesExactExtentSourceReturnAddressAndGuards)
{
    ValueSaver<int> bf_length{g_bf_length, std::get<0>(GetParam())};
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    const std::vector<Byte> source(size, 0x35);
    std::vector<Byte> destination(size + 16, 0xa5);
    std::vector<Byte> expected(size + 16, 0xa5);
    Byte *const output = destination.data() + 8;

    switch (std::get<1>(GetParam()))
    {
    case BigFloatByteProcedure::COPY:
        std::copy(source.begin(), source.end(), expected.begin() + 8);
        EXPECT_EQ(output, copy_bf(output, source.data()));
        break;
    case BigFloatByteProcedure::CLEAR:
        std::fill_n(expected.begin() + 8, size, Byte{0});
        EXPECT_EQ(output, clear_bf(output));
        break;
    case BigFloatByteProcedure::MAXIMUM:
    {
        constexpr int exponent = std::numeric_limits<long double>::max_exponent / 8;
        std::fill_n(expected.begin() + 8, size, Byte{0});
        expected[8 + size - 4] = 1;
        expected[8 + size - 2] = static_cast<Byte>(exponent % 256);
        expected[8 + size - 1] = static_cast<Byte>(exponent / 256);
        EXPECT_EQ(output, max_bf(output));
        break;
    }
    }

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(std::vector<Byte>(size, 0x35), source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(LengthsAndOperations, TestBigFloatByteProcedures,
    testing::Combine(testing::Values(8, 12, 64, 712),
        testing::Values(BigFloatByteProcedure::COPY, BigFloatByteProcedure::CLEAR, BigFloatByteProcedure::MAXIMUM)));

struct BigFloatByteSignInput
{
    Byte first;
    Byte last;
    int sign;
};

class TestBigFloatByteSign : public testing::TestWithParam<BigFloatByteSignInput>
{
};

TEST_P(TestBigFloatByteSign, observesOnlyTheBorrowedMantissa)
{
    constexpr int length = 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const BigFloatByteSignInput &input = GetParam();
    const std::array<Byte, length + 4> encoded{0x80, input.first, 0, 0, 0, 0, 0, 0, input.last, 0x7f, 0xff, 0xa5};
    const Byte *const mantissa = encoded.data() + 1;

    EXPECT_EQ(input.sign, sign_bf(mantissa));
    EXPECT_EQ(input.sign == 0, is_bf_zero(mantissa));
    EXPECT_EQ(input.sign < 0, is_bf_neg(mantissa));
    EXPECT_EQ(input.sign != 0, is_bf_not_zero(mantissa));
}

INSTANTIATE_TEST_SUITE_P(Mantissas, TestBigFloatByteSign,
    testing::Values(BigFloatByteSignInput{0, 0, 0}, BigFloatByteSignInput{0x80, 0, 1},
        BigFloatByteSignInput{0, 0x01, 1}, BigFloatByteSignInput{0, 0x80, -1}));

struct BigFloatByteComparisonInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    int expected;
};

class TestBigFloatByteComparison : public testing::TestWithParam<BigFloatByteComparisonInput>
{
};

TEST_P(TestBigFloatByteComparison, returnsTheSignedMismatchPosition)
{
    constexpr int length = 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const BigFloatByteComparisonInput &input = GetParam();
    alignas(U16) std::array<Byte, length + 2> left{};
    alignas(U16) std::array<Byte, length + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.end() - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.end() - 5);
    const std::array<Byte, length + 2> original_left = left;
    const std::array<Byte, length + 2> original_right = right;

    EXPECT_EQ(input.expected, cmp_bf(left.data(), right.data()));
    EXPECT_EQ(-input.expected, cmp_bf(right.data(), left.data()));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteComparison,
    testing::Values(BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}, 0},
        BigFloatByteComparisonInput{{0, 0, 0, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}, 8},
        BigFloatByteComparisonInput{{0, 0x80, 0, 0xff, 0xff}, {0x40, 1, 0, 0, 0}, -8},
        BigFloatByteComparisonInput{{0, 0xfe, 0xff, 1, 0}, {0xc0, 0xfe, 0xff, 0, 0}, -8},
        BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 2, 0, 0, 0}, -7},
        BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x80, 1, 0, 0, 0}, -6}));

struct BigFloatByteNegationInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteNegation : public testing::TestWithParam<BigFloatByteNegationInput>
{
};

TEST_P(TestBigFloatByteNegation, preservesExponentNormalizationAliasesAndPadding)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteNegationInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source[size] = 0xa5;
    source[size + 1] = 0x5a;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected[size] = 0xa5;
    expected[size + 1] = 0x5a;
    alignas(U16) std::array<Byte, size + 2> result;
    result.fill(0x3c);
    result[size] = 0xa5;
    result[size + 1] = 0x5a;

    EXPECT_EQ(result.data(), neg_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), neg_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), neg_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteNegation,
    testing::Values(BigFloatByteNegationInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteNegationInput{{0x40, 1, 0, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}},
        BigFloatByteNegationInput{{0, 1, 0, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteNegationInput{{0, 0, 0xff, 0xff, 0xff}, {0, 1, 0, 0, 0}}));

struct BigFloatByteAbsInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteAbs : public testing::TestWithParam<BigFloatByteAbsInput>
{
};

TEST_P(TestBigFloatByteAbs, preservesDestinationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteAbsInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 1> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.back() = 0xa5;
    const std::array<Byte, size + 1> original = source;
    std::array<Byte, size + 1> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 1> result;
    result.fill(0xa5);

    EXPECT_EQ(result.data(), abs_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), abs_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteAbs,
    testing::Values(BigFloatByteAbsInput{{0, 0, 0, 0x12, 0}, {0, 0, 0, 0x12, 0}},
        BigFloatByteAbsInput{{0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}},
        BigFloatByteAbsInput{{0, 0, 0xff, 0xff, 0xff}, {0, 1, 0, 0, 0}}));

struct BigFloatByteAdditionInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    std::array<Byte, 5> aligned_right_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteAddition : public testing::TestWithParam<BigFloatByteAdditionInput>
{
};

TEST_P(TestBigFloatByteAddition, inPlaceKernelAlignsMantissasAndPreservesPadding)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteAdditionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> left{};
    alignas(U16) std::array<Byte, size + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.begin() + size - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.begin() + size - 5);
    left.back() = 0x5a;
    right.back() = 0xa5;
    std::array<Byte, size + 2> expected_left{};
    std::array<Byte, size + 2> expected_right{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected_left.begin() + size - 5);
    std::copy(input.aligned_right_tail.begin(), input.aligned_right_tail.end(), expected_right.begin() + size - 5);
    expected_left.back() = 0x5a;
    expected_right.back() = 0xa5;

    EXPECT_EQ(left.data(), unsafe_add_a_bf(left.data(), right.data()));

    EXPECT_EQ(expected_left, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Alignment, TestBigFloatByteAddition,
    testing::Values(BigFloatByteAdditionInput{{0x40, 1, 0, 0, 0}, {0, 0x80, 0xff, 0xff, 0xff}, {0x80, 0xff, 0xff, 0, 0},
                        {0, 0xc0, 0, 0xff, 0xff}},
        BigFloatByteAdditionInput{{0, 0x80, 0, 0xff, 0xff}, {0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}, {0xc0, 1, 0, 0, 0}},
        BigFloatByteAdditionInput{{0x40, 1, 0, 0, 0}, {0, 1, 0, 0xf8, 0xff}, {}, {0x40, 1, 0, 0, 0}}));

struct BigFloatByteSubtractionInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    std::array<Byte, 5> aligned_right_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteSubtraction : public testing::TestWithParam<BigFloatByteSubtractionInput>
{
};

TEST_P(TestBigFloatByteSubtraction, preservesExtentAndAlignsScratchOperands)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteSubtractionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> left{};
    alignas(U16) std::array<Byte, size + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.begin() + size - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.begin() + size - 5);
    left.back() = 0x5a;
    right.back() = 0xa5;
    const std::array<Byte, size + 2> original_left = left;
    const std::array<Byte, size + 2> original_right = right;
    std::array<Byte, size + 2> expected{};
    std::array<Byte, size + 2> expected_right{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    std::copy(input.aligned_right_tail.begin(), input.aligned_right_tail.end(), expected_right.begin() + size - 5);
    expected.back() = 0x5a;
    expected_right.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0x5a;

    EXPECT_EQ(result.data(), unsafe_sub_bf(result.data(), left.data(), right.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);

    right = original_right;
    EXPECT_EQ(left.data(), unsafe_sub_a_bf(left.data(), right.data()));
    EXPECT_EQ(expected, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Operations, TestBigFloatByteSubtraction,
    testing::Values(BigFloatByteSubtractionInput{{0x40, 1, 0, 0, 0}, {0, 0x80, 0xff, 0xff, 0xff},
                        {0x80, 0xff, 0xff, 0, 0}, {0xc0, 1, 0, 0, 0}},
        BigFloatByteSubtractionInput{{}, {0, 1, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteSubtractionInput{{0x40, 1, 0, 0, 0}, {}, {}, {0x40, 1, 0, 0, 0}}));

struct BigFloatByteMultiplyInput
{
    int left;
    int right;
    int left_after;
    int right_after;
    int product;
    int product_exponent;
};

class TestBigFloatByteMultiply : public TestBigNumConversionBase,
                                 public testing::WithParamInterface<BigFloatByteMultiplyInput>
{
};

TEST_P(TestBigFloatByteMultiply, writesCompleteWideProductAndRestoresLengths)
{
    const BigFloatByteMultiplyInput &input = GetParam();
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    const auto encode = [](int length, int mantissa, int exponent)
    {
        std::vector<Byte> bytes(length + 2);
        bytes[length - 2] = static_cast<Byte>(mantissa);
        bytes[length - 1] = mantissa < 0 ? 0xff : 0;
        bytes[length] = static_cast<Byte>(exponent);
        bytes[length + 1] = static_cast<Byte>(static_cast<unsigned>(exponent) >> 8);
        return bytes;
    };
    std::vector<Byte> left = encode(g_bf_length, input.left, 2);
    std::vector<Byte> right = encode(g_bf_length, input.right, -1);
    const std::vector<Byte> product = encode(g_r_bf_length, input.product, input.product_exponent);
    std::vector<Byte> guarded(product.size() + 4, 0xa5);
    std::vector<Byte> expected = guarded;
    std::copy(product.begin(), product.end(), expected.begin() + 2);
    Byte *const result = guarded.data() + 2;

    EXPECT_EQ(result, multiply_wide(result, left.data(), right.data()));

    EXPECT_EQ(expected, guarded);
    EXPECT_EQ(encode(g_bf_length, input.left_after, 2), left);
    EXPECT_EQ(encode(g_bf_length, input.right_after, -1), right);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(SignsAndZero, TestBigFloatByteMultiply,
    testing::Values(BigFloatByteMultiplyInput{2, 3, 2, 3, 6, 1}, BigFloatByteMultiplyInput{-2, 3, 2, 3, -6, 1},
        BigFloatByteMultiplyInput{2, -3, 2, 3, -6, 1}, BigFloatByteMultiplyInput{-2, -3, 2, 3, 6, 1},
        BigFloatByteMultiplyInput{0, -3, 0, -3, 0, 0}));

struct BigFloatByteSqrInput
{
    int source;
    int source_after;
    int exponent;
    int squared;
    int squared_exponent;
};

class TestBigFloatByteSqr : public TestBigNumConversionBase, public testing::WithParamInterface<BigFloatByteSqrInput>
{
};

TEST_P(TestBigFloatByteSqr, writesCompleteWideProductAndRestoresLengths)
{
    const BigFloatByteSqrInput &input = GetParam();
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(g_bf_length + 2);
    source[g_bf_length - 2] = static_cast<Byte>(input.source);
    source[g_bf_length - 1] = input.source < 0 ? 0xff : 0;
    source[g_bf_length] = static_cast<Byte>(input.exponent);
    source[g_bf_length + 1] = static_cast<Byte>(static_cast<unsigned>(input.exponent) >> 8);
    std::vector<Byte> expected_source = source;
    expected_source[g_bf_length - 2] = static_cast<Byte>(input.source_after);
    expected_source[g_bf_length - 1] = 0;
    std::vector<Byte> guarded(g_r_bf_length + 6, 0xa5);
    std::vector<Byte> expected = guarded;
    std::fill_n(expected.begin() + 2, g_r_bf_length + 2, Byte{0});
    expected[g_r_bf_length] = static_cast<Byte>(input.squared);
    expected[g_r_bf_length + 2] = static_cast<Byte>(input.squared_exponent);
    expected[g_r_bf_length + 3] = static_cast<Byte>(static_cast<unsigned>(input.squared_exponent) >> 8);
    Byte *const result = guarded.data() + 2;

    EXPECT_EQ(result, square_wide(result, source.data()));

    EXPECT_EQ(expected, guarded);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(SignsAndExponents, TestBigFloatByteSqr,
    testing::Values(BigFloatByteSqrInput{3, 3, -700, 9, -1400}, BigFloatByteSqrInput{-2, 2, 500, 4, 1000},
        BigFloatByteSqrInput{0, 0, -1, 0, 0}));

struct BigFloatByteScalarMultiplyInput
{
    bool negative;
    U16 factor;
    std::array<Byte, 6> result_tail;
};

class TestBigFloatByteScalarMultiply : public testing::TestWithParam<BigFloatByteScalarMultiplyInput>
{
};

TEST_P(TestBigFloatByteScalarMultiply, preservesExtentAndLegacyNormalization)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteScalarMultiplyInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    source[length - 2] = input.negative ? 0xfe : 2;
    source[length - 1] = input.negative ? 0xff : 0;
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected_source = original;
    if (input.factor > 255)
    {
        expected_source[length - 3] = original[length - 2];
        expected_source[length - 2] = original[length - 1];
    }
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 6);
    expected.back() = 0xa5;

    EXPECT_EQ(result.data(), unsafe_mult_bf_int(result.data(), source.data(), input.factor));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), mult_a_bf_int(source.data(), input.factor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Factors, TestBigFloatByteScalarMultiply,
    testing::Values(BigFloatByteScalarMultiplyInput{false, 0, {}},
        BigFloatByteScalarMultiplyInput{true, 0, {0, 0, 0, 0xff, 0, 0}},
        BigFloatByteScalarMultiplyInput{false, 255, {0, 0xfe, 1, 0, 1, 0}},
        BigFloatByteScalarMultiplyInput{true, 256, {0, 0, 0xfe, 0xff, 1, 0}},
        BigFloatByteScalarMultiplyInput{false, 65535, {0xfe, 0xff, 1, 0, 2, 0}}));

struct BigFloatByteDoublingInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteDoubling : public testing::TestWithParam<BigFloatByteDoublingInput>
{
};

TEST_P(TestBigFloatByteDoubling, preservesExponentNormalizationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteDoublingInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;

    EXPECT_EQ(result.data(), double_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), double_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), double_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteDoubling,
    testing::Values(BigFloatByteDoublingInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteDoublingInput{{0x40, 1, 0, 0, 0}, {0x80, 2, 0, 0, 0}},
        BigFloatByteDoublingInput{{0, 0x80, 0, 0, 0}, {0, 1, 0, 1, 0}},
        BigFloatByteDoublingInput{{0, 0x7f, 0xff, 0, 0}, {0xfe, 0xfe, 0xff, 1, 0}}));

struct BigFloatByteHalvingInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
    Byte source_low{};
    Byte result_low{};
};

class TestBigFloatByteHalving : public testing::TestWithParam<BigFloatByteHalvingInput>
{
};

TEST_P(TestBigFloatByteHalving, preservesRoundingNormalizationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteHalvingInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.front() = input.source_low;
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.front() = input.result_low;
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;

    EXPECT_EQ(result.data(), half_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), half_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), half_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteHalving,
    testing::Values(BigFloatByteHalvingInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteHalvingInput{{0, 1, 0, 0, 0}, {0, 0x80, 0, 0xff, 0xff}},
        BigFloatByteHalvingInput{{0, 0xfe, 0xff, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteHalvingInput{{0, 0xfd, 0xff, 0, 0}, {0x80, 0xfe, 0xff, 0, 0}, 3, 1}));

class TestBigFloatByteReciprocal : public TestBigNumConversionBase, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatByteReciprocal, restoresPrecisionAndKeepsDestructiveWindowEffects)
{
    init_bf_length(GetParam());
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    constexpr std::array<Byte, 4> source_tail{0xfe, 0xff, 1, 0}; // -2 * 256
    constexpr std::array<Byte, 4> result_tail{0x80, 0xff, 0xfe, 0xff};
    std::vector<Byte> source(size + 2);
    std::copy(source_tail.begin(), source_tail.end(), source.begin() + size - 4);
    source.back() = 0xa5;
    std::vector<Byte> result(size + 2);
    std::fill(result.begin() + size, result.end(), Byte{0xa5});

    std::vector<Byte> tmp1(size + 2, 0xa5);
    std::vector<Byte> tmp2(size + 2, 0xa5);

    EXPECT_EQ(result.data(), unsafe_inv_bf(result.data(), source.data(), tmp1.data(), tmp2.data()));

    std::vector<Byte> expected_source(size + 2);
    expected_source[size - 4] = 2;
    expected_source.back() = 0xa5;
    std::vector<Byte> expected_result(size + 2);
    std::copy(result_tail.begin(), result_tail.end(), expected_result.begin() + size - 4);
    std::fill(expected_result.begin() + size, expected_result.end(), Byte{0xa5});
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(expected_result, result);
    std::vector<Byte> expected_tmp(size + 2);
    expected_tmp[size - 4] = 1;
    std::fill(expected_tmp.begin() + size, expected_tmp.end(), Byte{0xa5});
    EXPECT_EQ(expected_tmp, tmp1);
    EXPECT_EQ(expected_tmp, tmp2);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteReciprocal, testing::Values(4, 64));

class TestBigFloatByteDivision : public TestBigNumConversionBase, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatByteDivision, reappliesExponentsAndKeepsDestructiveOperandEffects)
{
    init_bf_length(GetParam());
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> numerator(size + 2);
    std::vector<Byte> denominator(size + 2);
    std::vector<Byte> result(size + 2);
    constexpr std::array<Byte, 6> numerator_tail{0xf4, 0xff, 2, 0, 0xa5, 0xa5};   // -12 * 256^2
    constexpr std::array<Byte, 6> denominator_tail{0xfc, 0xff, 1, 0, 0xa5, 0xa5}; // -4 * 256
    std::copy(numerator_tail.begin(), numerator_tail.end(), numerator.end() - 6);
    std::copy(denominator_tail.begin(), denominator_tail.end(), denominator.end() - 6);
    std::fill(result.begin() + size, result.end(), Byte{0xa5});

    std::vector<Byte> tmp1(size + 2, 0xa5);
    std::vector<Byte> tmp2(size + 2, 0xa5);

    EXPECT_EQ(
        result.data(), unsafe_div_bf(result.data(), numerator.data(), denominator.data(), tmp1.data(), tmp2.data()));

    std::vector<Byte> expected(size + 2);
    constexpr std::array<Byte, 6> result_tail{3, 0, 1, 0, 0xa5, 0xa5};
    std::copy(result_tail.begin(), result_tail.end(), expected.end() - 6);
    EXPECT_EQ(expected, result);
    expected[size - 2] = 0;
    EXPECT_EQ(expected, tmp1);
    expected[size - 4] = 1;
    EXPECT_EQ(expected, tmp2);
    expected[size - 4] = 12;
    EXPECT_EQ(expected, numerator);
    expected[size - 4] = 4;
    EXPECT_EQ(expected, denominator);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteDivision, testing::Values(4, 64));

struct BigFloatByteScalarDivisionInput
{
    std::array<Byte, 4> source_tail;
    U16 divisor;
    std::array<Byte, 4> result_tail;
};

class TestBigFloatByteScalarDivision : public testing::TestWithParam<BigFloatByteScalarDivisionInput>
{
};

TEST_P(TestBigFloatByteScalarDivision, preservesShortenedExtentAndMakesTheSourcePositive)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteScalarDivisionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + length - 2);
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected_source = original;
    expected_source[length - 2] = 6;
    expected_source[length - 1] = 0;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + length - 2);
    expected.back() = 0xa5;

    EXPECT_EQ(result.data(), unsafe_div_bf_int(result.data(), source.data(), input.divisor));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), unsafe_div_bf_int(source.data(), source.data(), input.divisor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), div_a_bf_int(source.data(), input.divisor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Divisors, TestBigFloatByteScalarDivision,
    testing::Values(BigFloatByteScalarDivisionInput{{0xfa, 0xff, 2, 0}, 2, {0xfd, 0xff, 2, 0}},
        BigFloatByteScalarDivisionInput{{6, 0, 2, 0}, 32768, {12, 0, 0, 0}}));

TEST(TestBigFloatByteScalarDivisionZero, distinguishesAliasedAndInPlaceSaturation)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int max_exp = std::numeric_limits<LDouble>::max_exponent / 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    alignas(U16) std::array<Byte, size + 2> source{0, 0, 0, 0, 0, 0, 0xfa, 0xff, 2, 0, 0, 0xa5};
    const std::array<Byte, size + 2> original = source;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    const std::array<Byte, size + 2> positive{
        0, 0, 0, 0, 0, 0, 1, 0, static_cast<Byte>(max_exp), static_cast<Byte>(max_exp >> 8), 0, 0xa5};
    const std::array<Byte, size + 2> negative{
        0, 0, 0, 0, 0, 0, 0, 0xff, static_cast<Byte>(max_exp - 1), static_cast<Byte>((max_exp - 1) >> 8), 0, 0xa5};

    EXPECT_EQ(result.data(), unsafe_div_bf_int(result.data(), source.data(), 0));
    EXPECT_EQ(negative, result);
    EXPECT_EQ(original, source);

    EXPECT_EQ(source.data(), unsafe_div_bf_int(source.data(), source.data(), 0));
    EXPECT_EQ(positive, source);

    source = original;
    EXPECT_EQ(source.data(), div_a_bf_int(source.data(), 0));
    EXPECT_EQ(negative, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

class TestBigFloatByteExponential : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteExponential, truncatesTheFirstTinyTermWithinTheBorrowedWindows)
{
    init_bf_length(4);
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int offset = 4;
    ASSERT_EQ(length, g_bf_length);
    alignas(U16) std::array<Byte, offset + size + 2> source{};
    std::fill_n(source.begin(), offset, Byte{0xa5});
    source[offset + length - 2] = 1;
    source[offset + length] = 0xf8; // exponent -8
    source[offset + length + 1] = 0xff;
    source.back() = 0xa5;
    const std::array<Byte, offset + size + 2> original = source;
    alignas(U16) std::array<Byte, offset + size + 2> result{};
    result.fill(0xa5);
    Byte *const output = result.data() + offset;
    alignas(U16) std::array<Byte, size + 2> tmp1{};
    alignas(U16) std::array<Byte, size + 2> tmp2{};
    alignas(U16) std::array<Byte, size + 2> tmp3{};
    tmp1.fill(0xa5);
    tmp2.fill(0xa5);
    tmp3.fill(0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output, exp_bf(output, source.data() + offset, tmp1.data(), tmp2.data(), tmp3.data()));

    std::array<Byte, offset + size + 2> expected{};
    expected.fill(0xa5);
    std::fill_n(expected.begin() + offset, size, Byte{0});
    expected[offset + length - 2] = 1;
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    std::array<Byte, size + 2> term{};
    term.fill(0xa5);
    std::copy_n(original.begin() + offset, size, term.begin());
    EXPECT_EQ(term, tmp1);
    EXPECT_EQ(term, tmp2);
    EXPECT_EQ(term, tmp3);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteLogarithm : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteLogarithm, confinesThePrecisionRampToTheBorrowedWindows)
{
    init_bf_length(64);
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(size + 4, 0xa5);
    std::fill_n(source.begin() + 2, size, Byte{0});
    source[size - 2] = 1;
    const std::vector<Byte> original = source;
    std::vector<Byte> result(size + 4, 0xa5);
    std::fill_n(result.begin() + 2, size, Byte{0});
    const std::vector<Byte> expected = result;
    Byte *const output = result.data() + 2;
    std::vector<Byte> expected_scratch(size + 2);
    expected_scratch[size] = 0xa5;
    expected_scratch[size + 1] = 0xa5;
    std::vector<Byte> tmp5 = expected_scratch;
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        unsafe_ln_bf(output, source.data() + 2, g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(), g_bf_tmp4.data(),
            tmp5.data(), g_bf_tmp6.data()));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(expected_scratch, tmp5);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteSineCosine : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteSineCosine, reducesTheBorrowedHalfPiWindowAndPreservesGuards)
{
    init_bf_length(4);
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int offset = 4;
    ASSERT_EQ(length, g_bf_length);
    // Use an exact pi substitute of 3.125 so the half-pi branch is exact.
    alignas(U16) std::array<Byte, offset + size + 2> pi{
        0xa5, 0xa5, 0xa5, 0xa5, 0, 0, 0, 0, 0, 0x20, 3, 0, 0, 0, 0xa5, 0xa5};
    const std::array<Byte, offset + size + 2> original_pi = pi;
    alignas(U16) std::array<Byte, offset + size + 2> input{
        0xa5, 0xa5, 0xa5, 0xa5, 0, 0, 0, 0, 0, 0x70, 0xfe, 0xff, 0, 0, 0xa5, 0xa5};
    alignas(U16) std::array<Byte, size + 2> sine{};
    alignas(U16) std::array<Byte, size + 2> cosine{};
    sine.fill(0xa5);
    cosine.fill(0xa5);
    int_to_bf(g_bf_pi.data(), 4);
    alignas(U16) std::array<Byte, size + 2> tmp1{};
    alignas(U16) std::array<Byte, size + 2> tmp2{};
    tmp1.fill(0xa5);
    tmp2.fill(0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(sine.data(),
        unsafe_sin_cos_bf(
            sine.data(), cosine.data(), input.data() + offset, pi.data() + offset, tmp1.data(), tmp2.data()));

    std::array<Byte, offset + size + 2> reduced{};
    reduced.fill(0xa5);
    std::fill_n(reduced.begin() + offset, size, Byte{0});
    constexpr std::array<Byte, size + 2> zero{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa5, 0xa5};
    constexpr std::array<Byte, size + 2> one{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0xa5, 0xa5};
    constexpr std::array<Byte, size + 2> half_pi{0, 0, 0, 0, 0, 0x90, 1, 0, 0, 0, 0xa5, 0xa5};
    EXPECT_EQ(reduced, input);
    EXPECT_EQ(zero, sine);
    EXPECT_EQ(one, cosine);
    EXPECT_EQ(original_pi, pi);
    EXPECT_EQ(half_pi, tmp1);
    EXPECT_EQ(half_pi, tmp2);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteArctangent : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteArctangent, restoresPrecisionAndKeepsLargeArgumentScratchEffects)
{
    init_bf_length(64);
    init_big_pi();
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(size + 4, 0xa5);
    Byte *const input = source.data() + 2;
    std::fill_n(input, size, Byte{0});
    input[size - 4] = 2;
    std::vector<Byte> result(size + 4, 0xa5);
    Byte *const output = result.data() + 2;
    std::fill_n(output, size, Byte{0});
    std::vector<Byte> tmp3(size + 2);
    std::fill(tmp3.begin() + size, tmp3.end(), Byte{0xa5});
    std::vector<Byte> copy1(size + 2, 0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        arctangent(output, input, g_bf_pi.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), tmp3.data(), g_bf_tmp4.data(),
            g_bf_tmp5.data(), copy1.data()));

    std::vector<Byte> reciprocal(size + 4, 0xa5);
    std::fill_n(reciprocal.begin() + 2, size, Byte{0});
    reciprocal[size - 2] = 0x80;
    reciprocal[size] = 0xff;
    reciprocal[size + 1] = 0xff;
    EXPECT_EQ(reciprocal, source);
    EXPECT_EQ(std::vector<Byte>(output, output + size + 2), tmp3);
    EXPECT_EQ(0, BIG_ACCESS16(copy1.data() + g_bf_length));
    EXPECT_EQ(0xa5, copy1[size]);
    EXPECT_EQ(0xa5, copy1[size + 1]);
    EXPECT_EQ(0xa5, result.front());
    EXPECT_EQ(0xa5, result.back());
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
    EXPECT_NEAR(1.1071487177940905L, bf_to_float(output), 1e-11L);
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

struct BigFloatToBigNumInput
{
    const char *name;
    int integer_length;
    std::vector<Byte> source;
    std::array<Byte, 4> expected;
};

class TestBigFloatToBigNumBytes : public TestBigNumConversionBase,
                                  public testing::WithParamInterface<BigFloatToBigNumInput>
{
};

TEST_P(TestBigFloatToBigNumBytes, preservesScalingSaturationSourceReturnIdentityAndDestinationBounds)
{
    const BigFloatToBigNumInput &input = GetParam();
    ValueSaver<int> bn_length{g_bn_length, 4};
    ValueSaver<int> bf_length{g_bf_length, static_cast<int>(input.source.size()) - 2};
    ValueSaver<int> integer_length{g_int_length, input.integer_length};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source{0xaa};
    source.insert(source.end(), input.source.begin(), input.source.end());
    source.push_back(0xbb);
    const std::vector<Byte> original = source;
    std::array<Byte, 8> destination;
    destination.fill(0xcc);
    std::array<Byte, 8> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    EXPECT_EQ(destination.data() + 1, bf_to_bn(destination.data() + 1, source.data() + 1));

    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatToBigNumBytes,
    testing::Values(BigFloatToBigNumInput{"Zero", 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {}},
        BigFloatToBigNumInput{
            "Positive", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0, 0}, {0x44, 0x55, 0x66, 0x77}},
        BigFloatToBigNumInput{
            "Negative", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xfe, 0xff, 0, 0}, {0x44, 0x55, 0x66, 0xfe}},
        BigFloatToBigNumInput{
            "TwoIntegerBytes", 2, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0, 0}, {0x55, 0x66, 0x77, 0}},
        BigFloatToBigNumInput{"NegativeIntegerTruncation", 4, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xfe, 0xff, 0, 0},
            {0xfe, 0xff, 0xff, 0xff}},
        BigFloatToBigNumInput{
            "NegativeExponent", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0xff, 0xff}, {0x55, 0x66, 0x77, 0}},
        BigFloatToBigNumInput{"SmallestFraction", 1, {0, 0, 0, 0, 0, 0, 1, 0, 0xfd, 0xff}, {1, 0, 0, 0}},
        BigFloatToBigNumInput{"PositiveUnderflow", 1, {0, 0, 0, 0, 0, 0, 1, 0, 0xfc, 0xff}, {}},
        BigFloatToBigNumInput{"NegativeUnderflow", 1, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 0xfc, 0xff}, {}},
        BigFloatToBigNumInput{"PositiveOverflow", 1, {0, 0, 0, 0, 0, 0, 1, 0, 1, 0}, {0xff, 0xff, 0xff, 0x7f}},
        BigFloatToBigNumInput{"NegativeOverflow", 1, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 1, 0}, {1, 0, 0, 0x80}},
        BigFloatToBigNumInput{
            "ShortenedMantissa", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0, 0, 0}, {0x22, 0x33, 0x44, 0x55}}),
    [](const testing::TestParamInfo<BigFloatToBigNumInput> &info) { return info.param.name; });

struct BigNumToBigFloatInput
{
    const char *name;
    int integer_length;
    int float_length;
    std::array<Byte, 4> source;
    std::vector<Byte> expected;
};

class TestBigNumToBigFloatBytes : public TestBigNumConversionBase,
                                  public testing::WithParamInterface<BigNumToBigFloatInput>
{
};

TEST_P(TestBigNumToBigFloatBytes, preservesScalingExponentSourceReturnIdentityAndDestinationBounds)
{
    const BigNumToBigFloatInput &input = GetParam();
    ValueSaver<int> bn_length{g_bn_length, 4};
    ValueSaver<int> bf_length{g_bf_length, input.float_length};
    ValueSaver<int> integer_length{g_int_length, input.integer_length};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::array<Byte, 6> source{0xaa, 0, 0, 0, 0, 0xbb};
    std::copy(input.source.begin(), input.source.end(), source.begin() + 1);
    const std::array<Byte, 6> original = source;
    std::vector<Byte> destination(input.expected.size() + 4, 0xcc);
    std::vector<Byte> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    EXPECT_EQ(destination.data() + 1, bn_to_bf(destination.data() + 1, source.data() + 1));

    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumToBigFloatBytes,
    testing::Values(BigNumToBigFloatInput{"Zero", 1, 8, {}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        BigNumToBigFloatInput{"Positive", 1, 8, {0, 0, 0x40, 1}, {0, 0, 0, 0, 0, 0x40, 1, 0, 0, 0}},
        BigNumToBigFloatInput{"Negative", 1, 8, {0, 0, 0xc0, 0xfe}, {0, 0, 0, 0, 0, 0xc0, 0xfe, 0xff, 0, 0}},
        BigNumToBigFloatInput{"TwoIntegerBytes", 2, 8, {0, 0, 0x40, 1}, {0, 0, 0, 0, 0, 0x40, 1, 0, 1, 0}},
        BigNumToBigFloatInput{"FourIntegerBytes", 4, 8, {0, 0, 0xc0, 0xfe}, {0, 0, 0, 0, 0, 0xc0, 0xfe, 0xff, 3, 0}},
        BigNumToBigFloatInput{"Half", 1, 8, {0, 0, 0x80, 0}, {0, 0, 0, 0, 0, 0, 0x80, 0, 0xff, 0xff}},
        BigNumToBigFloatInput{"LowestFractionBit", 1, 8, {1, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 0, 0xfd, 0xff}},
        BigNumToBigFloatInput{"MostNegativeInteger", 4, 8, {0, 0, 0, 0x80}, {0, 0, 0, 0, 0, 0, 0x80, 0xff, 3, 0}},
        BigNumToBigFloatInput{"ShortenedMantissa", 1, 6, {0, 0, 0x40, 1}, {0, 0, 0, 0x40, 1, 0, 0, 0}}),
    [](const testing::TestParamInfo<BigNumToBigFloatInput> &info) { return info.param.name; });

struct BigFloatConversionInput
{
    const char *name;
    std::vector<Byte> expected;
};

class TestBigFloatByteConversion : public testing::TestWithParam<BigFloatConversionInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_saved_bailout_value{engine::g_user.bailout_value, 0};
    ValueSaver<engine::Bailout> m_saved_bailout_test{engine::g_bailout_test, engine::Bailout::MOD};
    ValueSaver<int> m_saved_bf_digits{g_bf_digits, 0};
    ValueSaver<fractals::FractalType> m_saved_fractal_type{fractals::g_fractal_type, fractals::FractalType::MANDEL};
};

void TestBigFloatByteConversion::SetUp()
{
    init_bf_dec(20);
}

void TestBigFloatByteConversion::TearDown()
{
    free_bf_vars();
}

TEST_P(TestBigFloatByteConversion, conversionRelocatesTheExponentAndTruncatesLowMantissaBytes)
{
    const int active_length = g_bf_length;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12};
    const std::vector<Byte> original = source;
    const BigFloatConversionInput &input = GetParam();
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    EXPECT_EQ(0, convert_bf(destination.data() + 1, source.data(), static_cast<int>(input.expected.size()) - 2, 8));

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
    EXPECT_EQ(active_length, g_bf_length);
}

TEST_P(TestBigFloatByteConversion, unpackConvertsAtCurrentLengthAndReturnsTheNextStoredCursor)
{
    const BigFloatConversionInput &input = GetParam();
    ValueSaver<int> length{g_bf_length, static_cast<int>(input.expected.size()) - 2};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::array<Byte, 12> source{0xaa, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12, 0xbb};
    const std::array<Byte, 12> original = source;
    std::vector<Byte> destination(input.expected.size() + 4, 0xcc);
    std::vector<Byte> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    const Byte *const cursor = unpack_bf(destination.data() + 1, source.data() + 1, 8);

    EXPECT_EQ(source.data() + 11, cursor);
    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteConversion,
    testing::Values(BigFloatConversionInput{"WidenMantissa",
                        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatConversionInput{"NarrowMantissa", {0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatConversionInput{"SameLength", {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}}),
    [](const testing::TestParamInfo<BigFloatConversionInput> &info) { return info.param.name; });

TEST(TestBigFloatDiagnosticBytes, hexDumpPrintsShiftedShortenedWorkspace)
{
    const int original_length = 28;
    std::vector<Byte> workspace(static_cast<std::size_t>(original_length + 2), 0xcc);
    const ValueSaver<int> active_length{g_bf_length, 8};
    Byte *const shortened = workspace.data() + original_length - g_bf_length;
    const std::array<Byte, 10> bytes{0x00, 0x09, 0x8a, 0xff, 0x12, 0x34, 0x80, 0xfe, 0xfe, 0xff};
    std::copy(bytes.begin(), bytes.end(), shortened);
    const std::vector<Byte> original = workspace;
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    ASSERT_GT(workspace.size(), size);
    BigArena dump_arena{size, 2, [](Byte *) {}};
    const BigFloat snapshot = BigFloat::from_legacy(dump_arena, shortened, size);
    EXPECT_EQ(&dump_arena, &snapshot.arena());
    EXPECT_NE(shortened, snapshot.data());
    std::array<Byte, 10> observed{};
    snapshot.copy_to_legacy(observed.data(), observed.size());
    EXPECT_EQ(bytes, observed);
    testing::internal::CaptureStdout();

    bf_hex_dump(snapshot);

    const std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ("00 09 8A FF 12 34 80 FE  e FFFE\n", output);
    snapshot.copy_to_legacy(observed.data(), observed.size());
    EXPECT_EQ(bytes, observed);
    EXPECT_EQ(original, workspace);
    EXPECT_EQ(8, g_bf_length);
}

struct BigFloatMigrationInput
{
    const char *name;
    std::size_t blocks;
    int old_length;
    int old_padding;
    int new_length;
    int new_padding;
    std::vector<Byte> source;
    std::vector<Byte> expected;
};

class TestBigFloatByteMigration : public testing::TestWithParam<BigFloatMigrationInput>
{
};

TEST_P(TestBigFloatByteMigration, preservesCanonicalOrWideExtentExponentSourceAndGuards)
{
    const BigFloatMigrationInput &input = GetParam();
    const std::vector<Byte> original = input.source;
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    migrate_bf(destination.data() + 1, input.source.data(), input.blocks, input.new_length, input.new_padding,
        input.old_length, input.old_padding);

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, input.source);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteMigration,
    testing::Values(BigFloatMigrationInput{"CanonicalGrowth", 1, 8, 4, 12, 8,
                        {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
                        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatMigrationInput{"CanonicalShrink", 1, 12, 8, 8, 4,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff},
            {0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff}},
        BigFloatMigrationInput{"WideGrowth", 2, 8, 4, 12, 8,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0x78, 0x56},
            {0, 0, 0, 0, 0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0x78,
                0x56}},
        BigFloatMigrationInput{"WideShrink", 2, 12, 8, 8, 4,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08, 0x19, 0x2a,
                0x3b, 0xfe, 0x80, 0xff},
            {0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08, 0x19, 0x2a, 0x3b, 0xfe, 0x80, 0xff}}),
    [](const testing::TestParamInfo<BigFloatMigrationInput> &info) { return info.param.name; });

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
