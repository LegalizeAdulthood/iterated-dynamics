// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigFloat.h>
#include <math/BigNum.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

static_assert(!std::is_same_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigNum, BigNumPtr>);
static_assert(!std::is_convertible_v<BigNumPtr, BigNum>);
static_assert(!std::is_convertible_v<BigNum, BigFloat>);
static_assert(!std::is_convertible_v<BigFloat, BigNum>);
static_assert(!std::is_convertible_v<LDouble, BigNum>);

std::vector<Byte> encoded_value(const BigNum &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bn_length));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

std::vector<Byte> encoded_value(const BigFloat &value)
{
    std::vector<Byte> result(static_cast<std::size_t>(g_bf_length + 2));
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

} // namespace

class TestBigNumValue : public testing::Test
{
protected:
    TestBigNumValue();
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

TestBigNumValue::TestBigNumValue() :
    m_saved_bailout_value(g_user.bailout_value, 0),
    m_saved_bailout_test(g_bailout_test, Bailout::MOD),
    m_saved_bf_digits(g_bf_digits, 0),
    m_saved_fractal_type(g_fractal_type, FractalType::MANDEL)
{
}

void TestBigNumValue::SetUp()
{
    init_bf_dec(40);
}

void TestBigNumValue::TearDown()
{
    free_bf_vars();
}

class TestBigNumLayout : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigNumLayout::SetUp()
{
    if (GetParam() == 2)
    {
        g_bailout_test = Bailout::REAL;
    }
    else if (GetParam() == 4)
    {
        g_user.bailout_value = 100;
    }
    init_bf_length(32);
    ASSERT_EQ(GetParam(), g_int_length);
}

TEST_P(TestBigNumLayout, legacyConversionsPreserveEncodedValuesAndInputs)
{
    std::vector<Byte> fixed(static_cast<std::size_t>(g_bn_length));
    str_to_bn(fixed.data(), "-1.23456789012345678901234567890123456789");
    const auto original_fixed = fixed;
    std::vector<Byte> floating(static_cast<std::size_t>(g_bf_length + 2));
    std::vector<Byte> restored(fixed.size());

    bn_to_bf(floating.data(), fixed.data());
    const auto original_float = floating;
    bf_to_bn(restored.data(), floating.data());

    EXPECT_EQ(original_fixed, fixed);
    EXPECT_EQ(original_float, floating);
    EXPECT_EQ(original_fixed, restored);
}

TEST_P(TestBigNumLayout, legacyFormattingPreservesInputBytes)
{
    std::vector<Byte> value(static_cast<std::size_t>(g_bn_length));
    str_to_bn(value.data(), "-12.5");
    const auto original = value;

    EXPECT_EQ("-12.5", bn_to_string(value.data(), 8));
    EXPECT_EQ("-12.5", bn_to_string(value.data(), 0));
    EXPECT_EQ(original, value);
}

TEST_P(TestBigNumLayout, legacyFloatConversionRetainsOverflowAndUnderflowRules)
{
    for (const char *text : {"1e400", "-1e400", "1e-400", "-1e-400"})
    {
        SCOPED_TRACE(text);
        std::vector<Byte> floating(static_cast<std::size_t>(g_bf_length + 2));
        str_to_bf(floating.data(), text);
        const auto original = floating;
        std::vector<Byte> result(static_cast<std::size_t>(g_bn_length));
        std::vector<Byte> expected(result.size());
        if (std::string_view{text}.find("e-") == std::string_view::npos)
        {
            max_bn(expected.data());
            if (text[0] == '-')
            {
                neg_a_bn(expected.data());
            }
        }

        bf_to_bn(result.data(), floating.data());

        EXPECT_EQ(expected, result);
        EXPECT_EQ(original, floating);
    }
}

TEST_F(TestBigNumValue, zeroConstructionDoesNotAllocateArenaStorage)
{
    const auto marker = save_stack();

    const BigNum value;

    EXPECT_EQ(marker, save_stack());
    EXPECT_EQ("0.", value.to_string());
}

TEST_F(TestBigNumValue, parsesTheSpecifiedView)
{
    const std::string text{"-1.25x"};

    const BigNum value{std::string_view{text.data(), 5}};

    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_EQ("0.", BigNum{std::string_view{}}.to_string());
}

TEST_F(TestBigNumValue, assignsNativeScalarsWithoutSharingCopies)
{
    BigNum value{1.25L};
    const BigNum copy = value;

    value = -7;

    EXPECT_EQ("1.25", copy.to_string());
    EXPECT_EQ("-7.", value.to_string());
}

TEST_F(TestBigNumValue, supportsSmallestAndLargestLegacyLayouts)
{
    for (const int length : {4, 672})
    {
        SCOPED_TRACE(length);
        init_bf_length(length);

        const BigNum value{"-1.25"};

        EXPECT_EQ("-1.25", value.to_string(8));
        EXPECT_EQ(static_cast<std::size_t>(length), encoded_value(value).size());
        EXPECT_EQ(encoded_value(value), encoded_value(BigNum{BigFloat{value}}));
    }
}

TEST_F(TestBigNumValue, importedBytesSurviveArenaRewindAndReuse)
{
    const auto marker = save_stack();
    const std::size_t size = static_cast<std::size_t>(g_bn_length);
    BigNumPtr source = bn_stack().allocate_single();
    str_to_bn(source, "-1.23456789012345678901234567890123456789");
    const std::vector<Byte> expected(source, source + size);
    const BigNumConstPtr input = source;

    const BigNum value = BigNum::from_legacy(input, size);
    restore_stack(marker);
    BigNumPtr reused = bn_stack().allocate_single();
    ASSERT_EQ(source, reused);
    std::fill_n(reused, size, Byte{0xff});

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_F(TestBigNumValue, exportedBytesAreIndependentOfTheValue)
{
    const BigNum value{"-1.25"};
    auto exported = encoded_value(value);

    std::fill(exported.begin(), exported.end(), Byte{0});

    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigNumValue, copyingMovingAndExportDoNotRequireAnActiveArena)
{
    BigNum original{"-1.23456789012345678901234567890123456789"};
    const auto expected = encoded_value(original);
    free_bf_vars();

    const BigNum copy{original};
    BigNum moved{std::move(original)};
    original = copy;
    moved = std::move(original);
    std::vector<Byte> exported(expected.size());
    moved.copy_to_legacy(exported.data(), exported.size());

    EXPECT_EQ(expected, exported);
    init_bf_dec(40);
    EXPECT_EQ(expected, encoded_value(copy));
    EXPECT_EQ(expected, encoded_value(moved));
}

TEST_F(TestBigNumValue, copyAssignmentAndDestructionUseTheOwnedByteCount)
{
    const BigNum original{"-1.25"};
    const auto expected = encoded_value(original);
    g_bailout_test = Bailout::REAL;
    init_bf_dec(80);

    {
        BigNum assigned{2.5L};
        assigned = original;
        std::vector<Byte> exported(expected.size());
        assigned.copy_to_legacy(exported.data(), exported.size());

        EXPECT_EQ(expected, exported);
        free_bf_vars();
    }

    g_bailout_test = Bailout::MOD;
    init_bf_dec(40);
    EXPECT_EQ("-1.25", original.to_string());
}

TEST_F(TestBigNumValue, movedFromValueCanBeReassigned)
{
    BigNum original{"-1.25"};
    const BigNum moved{std::move(original)};

    original = 2.5L;

    EXPECT_EQ("-1.25", moved.to_string());
    EXPECT_EQ("2.5", original.to_string());
}

TEST_F(TestBigNumValue, adaptersRejectNullAndUndersizedBuffers)
{
    const BigNum value{"-1.25"};
    std::vector<Byte> buffer(static_cast<std::size_t>(g_bn_length), Byte{0x5a});
    const auto unchanged = buffer;

    EXPECT_THROW(BigNum::from_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(BigNum::from_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(nullptr, buffer.size()), std::invalid_argument);
    EXPECT_THROW(value.copy_to_legacy(buffer.data(), buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(unchanged, buffer);
}

TEST_F(TestBigNumValue, rejectsNonfiniteScalarInputWithoutChangingTheValue)
{
    BigNum value{"-1.25"};

    EXPECT_THROW((BigNum{std::numeric_limits<LDouble>::infinity()}), std::invalid_argument);
    EXPECT_THROW(value = std::numeric_limits<LDouble>::quiet_NaN(), std::invalid_argument);
    EXPECT_EQ("-1.25", value.to_string());
}

TEST_F(TestBigNumValue, formattingPreservesInputAndRejectsNegativeDigitCounts)
{
    const BigNum value{"-1.25"};
    const auto original = encoded_value(value);

    EXPECT_EQ("-1.25", value.to_string(8));
    EXPECT_EQ("-1.25", value.to_string());
    EXPECT_THROW(value.to_string(-1), std::invalid_argument);
    EXPECT_EQ(original, encoded_value(value));
}

TEST_P(TestBigNumLayout, constructorsUseTheActiveFixedPointLayout)
{
    const std::size_t ones_byte = static_cast<std::size_t>(g_bn_length - g_int_length);
    std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
    expected[ones_byte] = 1;
    expected[ones_byte - 1] = 0x40;

    EXPECT_EQ(expected, encoded_value(BigNum{1.25L}));
    EXPECT_EQ(expected, encoded_value(BigNum{"1.25"}));
}

TEST_P(TestBigNumLayout, explicitConversionsPreserveBitsBeyondMachinePrecision)
{
    std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
    expected.front() = 1;
    expected[static_cast<std::size_t>(g_bn_length - g_int_length)] = 1;
    const BigNum fixed = BigNum::from_legacy(expected.data(), expected.size());

    const BigFloat floating{fixed};
    const auto original_float = encoded_value(floating);
    const BigNum restored{floating};

    EXPECT_EQ(expected, encoded_value(fixed));
    EXPECT_EQ(expected, encoded_value(restored));
    EXPECT_EQ(original_float, encoded_value(floating));
}

TEST_P(TestBigNumLayout, owningConversionsKeepLegacyOverflowAndUnderflowResults)
{
    for (const char *text : {"1e400", "-1e400", "1e-400", "-1e-400"})
    {
        SCOPED_TRACE(text);
        const BigFloat floating{text};
        const auto original = encoded_value(floating);
        auto raw = original;
        std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
        bf_to_bn(expected.data(), raw.data());

        const BigNum fixed{floating};

        EXPECT_EQ(expected, encoded_value(fixed));
        EXPECT_EQ(original, encoded_value(floating));
    }
}

TEST_F(TestBigNumValue, conversionAdaptersValidateDestinationBuffers)
{
    const BigNum fixed{"-1.25"};
    const BigFloat floating{fixed};
    std::vector<Byte> bn_buffer(static_cast<std::size_t>(g_bn_length), Byte{0x5a});
    std::vector<Byte> bf_buffer(static_cast<std::size_t>(g_bf_length + 2), Byte{0x5a});
    const auto original_bn = bn_buffer;
    const auto original_bf = bf_buffer;

    EXPECT_THROW(fixed.copy_to_legacy_float(nullptr, bf_buffer.size()), std::invalid_argument);
    EXPECT_THROW(fixed.copy_to_legacy_float(bf_buffer.data(), bf_buffer.size() - 1), std::invalid_argument);
    EXPECT_THROW(floating.copy_to_legacy_num(nullptr, bn_buffer.size()), std::invalid_argument);
    EXPECT_THROW(floating.copy_to_legacy_num(bn_buffer.data(), bn_buffer.size() - 1), std::invalid_argument);
    EXPECT_EQ(original_bn, bn_buffer);
    EXPECT_EQ(original_bf, bf_buffer);
}

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumLayout, testing::Values(1, 2, 4));

TEST_P(TestBigNumLayout, legacyArithmeticSamplesPreserveDistinctOperands)
{
    auto left = encoded_value(BigNum{"-7.5"});
    auto right = encoded_value(BigNum{"2.5"});
    const auto original_left = left;
    const auto original_right = right;
    std::vector<Byte> result(static_cast<std::size_t>(g_r_length));

    add_bn(result.data(), left.data(), right.data());
    EXPECT_EQ("-5.", bn_to_string(result.data(), 8));
    sub_bn(result.data(), left.data(), right.data());
    EXPECT_EQ("-10.", bn_to_string(result.data(), 8));
    mult_bn(result.data(), left.data(), right.data());
    EXPECT_EQ("-18.75", bn_to_string(result.data() + g_shift_factor, 8));
    auto half = encoded_value(BigNum{"0.5"});
    div_bn(result.data(), left.data(), half.data());
    EXPECT_EQ("-15.", bn_to_string(result.data(), 8));

    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

TEST_P(TestBigNumLayout, legacyDivisionKeepsSaturationAndZeroRules)
{
    std::vector<Byte> maximum(static_cast<std::size_t>(g_bn_length));
    max_bn(maximum.data());
    auto numerator = maximum;
    auto denominator = encoded_value(BigNum{"0.25"});
    std::vector<Byte> result(maximum.size());

    div_bn(result.data(), numerator.data(), denominator.data());
    EXPECT_EQ(maximum, result);
    neg_a_bn(numerator.data());
    neg_a_bn(maximum.data());
    div_bn(result.data(), numerator.data(), denominator.data());
    EXPECT_EQ(maximum, result);
    std::fill(denominator.begin(), denominator.end(), Byte{0});
    div_bn(result.data(), numerator.data(), denominator.data());
    max_bn(maximum.data());
    EXPECT_EQ(maximum, result);
    std::fill(numerator.begin(), numerator.end(), Byte{0});
    div_bn(result.data(), numerator.data(), denominator.data());
    EXPECT_EQ(numerator, result);
}

class TestBigNumProducts : public TestBigNumValue, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestBigNumProducts::SetUp()
{
    init_bf_length(GetParam());
}

TEST_P(TestBigNumProducts, legacyProductsUseTheirDocumentedOffsets)
{
    auto value = encoded_value(BigNum{"-7.5"});
    auto other = value;
    const auto original = value;
    std::vector<Byte> partial(static_cast<std::size_t>(g_r_length));
    std::vector<Byte> full(static_cast<std::size_t>(2 * g_bn_length));

    mult_bn(partial.data(), value.data(), other.data());
    EXPECT_EQ("56.25", bn_to_string(partial.data() + g_shift_factor, 8));
    square_bn(partial.data(), value.data());
    EXPECT_EQ("56.25", bn_to_string(partial.data() + g_shift_factor, 8));
    full_mult_bn(full.data(), value.data(), other.data());
    EXPECT_EQ("56.25", bn_to_string(full.data() + g_bn_length - g_int_length, 8));
    full_square_bn(full.data(), value.data());
    EXPECT_EQ("56.25", bn_to_string(full.data() + g_bn_length - g_int_length, 8));
    EXPECT_EQ(original, value);
    EXPECT_EQ(original, other);
}

TEST_P(TestBigNumProducts, legacyProductTruncationKeepsTheSignedLowBit)
{
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length));
    quantum.front() = 1;
    auto half = encoded_value(BigNum{"0.5"});
    std::vector<Byte> product(static_cast<std::size_t>(g_r_length));
    const std::vector<Byte> zero(quantum.size());

    mult_bn(product.data(), quantum.data(), half.data());
    EXPECT_TRUE(std::equal(zero.begin(), zero.end(), product.data() + g_shift_factor));
    neg_a_bn(quantum.data());
    const auto negative_quantum = quantum;
    mult_bn(product.data(), quantum.data(), half.data());
    EXPECT_TRUE(std::equal(negative_quantum.begin(), negative_quantum.end(), product.data() + g_shift_factor));
    EXPECT_EQ(negative_quantum, quantum);
}

TEST_P(TestBigNumLayout, arithmeticPreservesOperandsAndUsesCanonicalResults)
{
    const BigNum left{"-7.5"};
    const BigNum right{"2.5"};
    const auto original_left = encoded_value(left);
    const auto original_right = encoded_value(right);

    EXPECT_EQ("-5.", (left + right).to_string(8));
    EXPECT_EQ("-10.", (left - right).to_string(8));
    EXPECT_EQ("-18.75", (left * right).to_string(8));
    EXPECT_EQ("-15.", (left / BigNum{"0.5"}).to_string(8));
    EXPECT_EQ("7.5", (-left).to_string(8));
    EXPECT_EQ("56.25", left.sqr().to_string(8));
    EXPECT_EQ(original_left, encoded_value(left));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigNumLayout, compoundAssignmentChangesOnlyItsDestination)
{
    BigNum value{"1.5"};
    const BigNum right{2};
    const BigNum copy = value;
    const auto original_right = encoded_value(right);

    EXPECT_EQ(&value, &(value += right));
    EXPECT_EQ("3.5", value.to_string(8));
    EXPECT_EQ(&value, &(value -= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ(&value, &(value *= right));
    EXPECT_EQ("3.", value.to_string(8));
    EXPECT_EQ(&value, &(value /= right));
    EXPECT_EQ("1.5", value.to_string(8));
    EXPECT_EQ("1.5", copy.to_string(8));
    EXPECT_EQ(original_right, encoded_value(right));
}

TEST_P(TestBigNumLayout, repeatedNegativeOperandsAndSelfAssignmentAreSafe)
{
    BigNum value{-2};
    const auto original = encoded_value(value);

    EXPECT_EQ("4.", (value * value).to_string(8));
    EXPECT_EQ("4.", value.sqr().to_string(8));
    EXPECT_EQ(original, encoded_value(value));
    value *= value;
    EXPECT_EQ("4.", value.to_string(8));
    value /= value;
    EXPECT_EQ("1.", value.to_string(8));
    value += value;
    EXPECT_EQ("2.", value.to_string(8));
    value -= value;
    EXPECT_EQ("0.", value.to_string(8));
}

TEST_P(TestBigNumLayout, assignmentCanAlsoSupplyTheRightOperand)
{
    const BigNum left{8};
    BigNum value{2};

    value = left + value;
    EXPECT_EQ("10.", value.to_string(8));
    value = left - value;
    EXPECT_EQ("-2.", value.to_string(8));
    value = left * value;
    EXPECT_EQ("-16.", value.to_string(8));
    value = left / value;
    EXPECT_EQ("-0.5", value.to_string(8));
}

TEST_P(TestBigNumLayout, expressionsMatchLegacySequencesAndOutliveTheirTemporaries)
{
    const BigNum left{"-1.234567890123456789"};
    const BigNum right{"2.345678901234567890"};
    const BigNum divisor{"-0.5"};
    auto raw_left = encoded_value(left);
    auto raw_right = encoded_value(right);
    auto raw_divisor = encoded_value(divisor);
    std::vector<Byte> expected(static_cast<std::size_t>(g_bn_length));
    std::vector<Byte> quotient(expected.size());
    std::vector<Byte> product(static_cast<std::size_t>(g_r_length));
    // Value expressions use fresh results. Reusing a legacy division
    // destination can retain low-bit differences from its previous contents.
    add_bn(expected.data(), raw_left.data(), raw_right.data());
    div_bn(quotient.data(), expected.data(), raw_divisor.data());
    mult_bn(product.data(), quotient.data(), raw_left.data());
    sub_bn(expected.data(), product.data() + g_shift_factor, raw_right.data());
    const auto marker = save_stack();

    const BigNum result = ((left + right) / divisor) * left - right;
    const BigNum unrelated = BigNum{-8} * BigNum{-9};

    EXPECT_EQ(marker, save_stack());
    bn_stack().allocate_single();
    restore_stack(marker);
    EXPECT_EQ(expected, encoded_value(result));
    EXPECT_EQ("72.", unrelated.to_string(8));
    EXPECT_EQ(raw_left, encoded_value(left));
    EXPECT_EQ(raw_right, encoded_value(right));
    EXPECT_EQ(raw_divisor, encoded_value(divisor));
}

TEST_P(TestBigNumLayout, divisionAssignmentMatchesAFreshResult)
{
    BigNum value = BigNum{"-1.234567890123456789"} + BigNum{"2.345678901234567890"};
    const BigNum divisor{"-0.5"};
    const auto expected = encoded_value(value / divisor);

    value /= divisor;

    EXPECT_EQ(expected, encoded_value(value));
}

TEST_P(TestBigNumLayout, divisionKeepsLegacySaturationAndZeroResults)
{
    std::vector<Byte> maximum(static_cast<std::size_t>(g_bn_length));
    max_bn(maximum.data());
    const BigNum positive = BigNum::from_legacy(maximum.data(), maximum.size());
    const BigNum quarter{"0.25"};
    const BigNum zero;

    EXPECT_EQ(maximum, encoded_value(positive / quarter));
    EXPECT_EQ(maximum, encoded_value((-positive) / zero));
    neg_a_bn(maximum.data());
    EXPECT_EQ(maximum, encoded_value((-positive) / quarter));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / zero));
    EXPECT_EQ(encoded_value(zero), encoded_value(zero / quarter));
}

TEST_P(TestBigNumProducts, productsAndSquaresReturnOnlyCanonicalBytes)
{
    const BigNum value{"-7.5"};
    const auto original = encoded_value(value);
    auto raw = original;
    std::vector<Byte> product(static_cast<std::size_t>(g_r_length));
    square_bn(product.data(), raw.data());
    const std::vector<Byte> expected(product.data() + g_shift_factor, product.data() + g_shift_factor + g_bn_length);

    EXPECT_EQ(expected, encoded_value(value * value));
    EXPECT_EQ(expected, encoded_value(value.sqr()));
    EXPECT_EQ(original, encoded_value(value));
}

TEST_P(TestBigNumProducts, normalizationPreservesSignedLowBitTruncation)
{
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length));
    quantum.front() = 1;
    const BigNum positive = BigNum::from_legacy(quantum.data(), quantum.size());
    const BigNum half{"0.5"};
    const auto zero = encoded_value(BigNum{});

    EXPECT_EQ(zero, encoded_value(positive * half));
    EXPECT_EQ(zero, encoded_value(positive.sqr()));
    neg_a_bn(quantum.data());
    const BigNum negative = BigNum::from_legacy(quantum.data(), quantum.size());
    EXPECT_EQ(quantum, encoded_value(negative * half));
    EXPECT_EQ(quantum, encoded_value(negative));
}

TEST_F(TestBigNumValue, arithmeticKeepsFixedWidthOverflow)
{
    std::vector<Byte> maximum(static_cast<std::size_t>(g_bn_length));
    max_bn(maximum.data());
    std::vector<Byte> minimum(maximum.size());
    minimum.back() = 0x80;
    std::vector<Byte> quantum(maximum.size());
    quantum.front() = 1;
    const BigNum high = BigNum::from_legacy(maximum.data(), maximum.size());
    const BigNum low = BigNum::from_legacy(minimum.data(), minimum.size());
    const BigNum unit = BigNum::from_legacy(quantum.data(), quantum.size());
    std::vector<Byte> negative_twice_quantum(maximum.size(), Byte{0xff});
    negative_twice_quantum.front() = 0xfe;

    EXPECT_EQ(minimum, encoded_value(high + unit));
    EXPECT_EQ(maximum, encoded_value(low - unit));
    EXPECT_EQ(minimum, encoded_value(-low));
    EXPECT_EQ(negative_twice_quantum, encoded_value(high * BigNum{2}));
}

TEST_F(TestBigNumValue, comparisonsReadNumericalValuesWithoutChangingOperands)
{
    std::vector<Byte> quantum(static_cast<std::size_t>(g_bn_length));
    quantum.front() = 1;
    const BigNum tiny = BigNum::from_legacy(quantum.data(), quantum.size());
    auto above_one = encoded_value(BigNum{1});
    above_one.front() = 1;
    const BigNum values[]{BigNum{-7.5L}, BigNum{-1}, -tiny, BigNum{}, tiny, BigNum{1},
        BigNum::from_legacy(above_one.data(), above_one.size()), BigNum{7.5L}};

    for (std::size_t i = 0; i < std::size(values); ++i)
    {
        for (std::size_t j = 0; j < std::size(values); ++j)
        {
            const auto left = encoded_value(values[i]);
            const auto right = encoded_value(values[j]);
            const BigNum copy = values[j];

            EXPECT_EQ(i == j, values[i] == copy);
            EXPECT_EQ(i != j, values[i] != copy);
            EXPECT_EQ(i < j, values[i] < copy);
            EXPECT_EQ(i <= j, values[i] <= copy);
            EXPECT_EQ(i > j, values[i] > copy);
            EXPECT_EQ(i >= j, values[i] >= copy);
            EXPECT_EQ(left, encoded_value(values[i]));
            EXPECT_EQ(right, encoded_value(copy));
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumProducts, testing::Values(4, 8, 12, 672));

} // namespace id::test
