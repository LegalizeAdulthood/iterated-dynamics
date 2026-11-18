// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/complex_fn.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
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

template <typename Value>
BigArena &value_arena(const BigLifetime lifetime)
{
    if constexpr (std::is_same_v<Value, BigNum>)
    {
        return bn_arena(lifetime);
    }
    else
    {
        return bf_arena(lifetime);
    }
}

template <typename Value>
std::vector<Byte> encoded(const Value &value)
{
    std::vector<Byte> result(value.arena().value_size());
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

void unused_reducer(Byte *)
{
}

} // namespace

template <typename Value>
class TestBigValueStorage : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test, Bailout::MOD};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<FractalType> m_type{
        g_fractal_type, std::is_same_v<Value, BigNum> ? FractalType::MANDEL : FractalType::JULIA_Z_POWER};
};

template <typename Value>
void TestBigValueStorage<Value>::SetUp()
{
    init_bf_dec(40);
    init_big_pi();
}

template <typename Value>
void TestBigValueStorage<Value>::TearDown()
{
    free_bf_vars();
}

using ValueTypes = testing::Types<BigNum, BigFloat>;
TYPED_TEST_SUITE(TestBigValueStorage, ValueTypes);

template <typename Value>
class TestBigNullValue : public testing::Test
{
};

TYPED_TEST_SUITE(TestBigNullValue, ValueTypes);

TYPED_TEST(TestBigNullValue, defaultConstructionAndNullLifetimeOperationsNeedNoArena)
{
    EXPECT_TRUE(std::is_nothrow_default_constructible_v<TypeParam>);
    TypeParam value;
    EXPECT_FALSE(value);
    const TypeParam copy{value};
    EXPECT_FALSE(copy);
    TypeParam moved{std::move(value)};
    EXPECT_FALSE(value);
    EXPECT_FALSE(moved);
    value = copy;
    EXPECT_FALSE(value);
    value = std::move(moved);
    EXPECT_FALSE(value);
    EXPECT_FALSE(moved);
    const Complex<TypeParam> complex;
    EXPECT_FALSE(complex.x);
    EXPECT_FALSE(complex.y);
}

TYPED_TEST(TestBigValueStorage, assignmentInitializesNullValuesInTheWorkingAllocator)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    TypeParam source{value_arena<TypeParam>(BigLifetime::PERSISTENT), -1.25L};
    TypeParam working_source{2.5L};
    TypeParam copied;
    TypeParam moved;
    TypeParam moved_from_working;
    TypeParam scalar;

    copied = source;
    moved = std::move(source);
    moved_from_working = std::move(working_source);
    scalar = 0;

    EXPECT_TRUE(copied);
    EXPECT_TRUE(moved);
    EXPECT_TRUE(moved_from_working);
    EXPECT_FALSE(working_source);
    EXPECT_TRUE(scalar);
    EXPECT_EQ(&working, &copied.arena());
    EXPECT_EQ(&working, &moved.arena());
    EXPECT_EQ(&working, &moved_from_working.arena());
    EXPECT_EQ(&working, &scalar.arena());
    EXPECT_EQ(TypeParam{-1.25L}, copied);
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    EXPECT_EQ(TypeParam{2.5L}, moved_from_working);
    EXPECT_EQ(TypeParam{0}, scalar);
}

TYPED_TEST(TestBigValueStorage, copyingNullWithASelectedAllocatorDoesNotAllocate)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    const TypeParam null;

    const TypeParam selected{arena, null};
    const TypeParam copy{selected};

    EXPECT_FALSE(selected);
    EXPECT_FALSE(copy);
    EXPECT_EQ(&arena, &selected.arena());
    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_EQ(0, arena.high_water_mark());
}

TYPED_TEST(TestBigValueStorage, assigningNullReleasesStorageAndRetainsTheDestinationAllocator)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam value{arena, 1.25L};
    const TypeParam occupied{arena, 2.5L};
    TypeParam null;

    value = null;
    EXPECT_FALSE(value);
    EXPECT_EQ(&arena, &value.arena());
    value = -1.25L;
    EXPECT_EQ(TypeParam{-1.25L}, value);
    value = std::move(null);
    EXPECT_FALSE(value);
    EXPECT_FALSE(null);
    EXPECT_EQ(&arena, &value.arena());
    value = 0;
    EXPECT_TRUE(value);
    EXPECT_EQ(TypeParam{0}, value);
    EXPECT_EQ(TypeParam{2.5L}, occupied);
}

TYPED_TEST(TestBigValueStorage, constructionAndCopiesUseTheSelectedAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam source{persistent, -1.25L};
    const TypeParam text{persistent, "-2.5"};
    const TypeParam copy{source};
    const TypeParam selected_copy{temporary, source};
    const TypeParam zero{temporary};

    source = 3.75L;

    EXPECT_EQ(&persistent, &source.arena());
    EXPECT_EQ(&persistent, &copy.arena());
    EXPECT_EQ(&temporary, &selected_copy.arena());
    EXPECT_EQ(&temporary, &zero.arena());
    EXPECT_EQ("-2.5", text.to_string());
    EXPECT_EQ("-1.25", copy.to_string());
    EXPECT_EQ("-1.25", selected_copy.to_string());
    EXPECT_EQ(TypeParam{0}, zero);
}

TYPED_TEST(TestBigValueStorage, conversionsAndImportsUseDestinationRepresentationAndAllocator)
{
    using Other = std::conditional_t<std::is_same_v<TypeParam, BigNum>, BigFloat, BigNum>;
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const Other source{-1.25L};
    const TypeParam converted{persistent, source};
    const auto bytes = encoded(converted);
    const auto imported = TypeParam::from_legacy(persistent, bytes.data(), bytes.size());

    EXPECT_EQ(&persistent, &converted.arena());
    EXPECT_EQ(&persistent, &imported.arena());
    EXPECT_EQ(TypeParam{-1.25L}, converted);
    EXPECT_EQ(bytes, encoded(imported));
}

TYPED_TEST(TestBigValueStorage, assignmentRetainsTheDestinationAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam destination{persistent, 2.5L};
    TypeParam source{temporary, -1.25L};

    destination = source;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{-1.25L}, destination);
    source = 3.75L;
    destination = std::move(source);
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, destination);
    EXPECT_EQ(&temporary, &source.arena());
    source = -2.5L;
    EXPECT_EQ(TypeParam{3.75L}, destination);
}

TYPED_TEST(TestBigValueStorage, movedFromValuesRetainTheirAllocatorForReassignment)
{
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam source{temporary, -1.25L};
    TypeParam moved{std::move(source)};

    EXPECT_EQ(&temporary, &source.arena());
    EXPECT_EQ(&temporary, &moved.arena());
    source = 2.5L;
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    source = moved;
    EXPECT_EQ(TypeParam{-1.25L}, source);
}

TYPED_TEST(TestBigValueStorage, aFullArenaPermitsMovesAndExistingDestinationAssignment)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam first{arena, -1.25L};
    TypeParam second{arena, 2.5L};
    const TypeParam outside{3.75L};

    EXPECT_THROW((TypeParam{first}), PrecisionError);
    TypeParam moved{std::move(first)};
    first = std::move(second);
    moved = outside;
    first = outside;
    EXPECT_EQ(&arena, &moved.arena());
    EXPECT_EQ(&arena, &first.arena());
    EXPECT_EQ(outside, first);
    EXPECT_EQ(outside, moved);
}

TYPED_TEST(TestBigValueStorage, failedCrossArenaAssignmentToAnEmptyValuePreservesTheSource)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam destination{arena, -1.25L};
    const TypeParam occupied{arena, 2.5L};
    const TypeParam moved{std::move(destination)};
    TypeParam outside{3.75L};

    EXPECT_THROW(destination = std::move(outside), PrecisionError);
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, outside);
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    EXPECT_EQ(TypeParam{2.5L}, occupied);
}

TYPED_TEST(TestBigValueStorage, eagerScalarAndComplexResultsUseTheWorkingAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    const TypeParam left{persistent, 1.25L};
    const TypeParam right{persistent, 0.5L};
    for (const auto &result : {left + right, left - right, left * right, left / right, -left, left.sqr()})
    {
        EXPECT_EQ(&working, &result.arena());
    }
    const Complex<TypeParam> value{left, right};
    for (const auto &result : {+value, -value, value + value, value - value, value * value, value / value, value * 0.5,
             0.5 * value, power(value, 1), power(value, 3), cube(value)})
    {
        EXPECT_EQ(&working, &result.x.arena());
        EXPECT_EQ(&working, &result.y.arena());
    }
    if constexpr (std::is_same_v<TypeParam, BigFloat>)
    {
        EXPECT_EQ(&working, &left.half().arena());
        EXPECT_EQ(&working, &left.log().arena());
        EXPECT_EQ(&working, &right.exp().arena());
        EXPECT_EQ(&working, &left.atan2(right).arena());
        const auto trigonometric = right.sin_cos();
        EXPECT_EQ(&working, &trigonometric.first.arena());
        EXPECT_EQ(&working, &trigonometric.second.arena());
    }
    EXPECT_EQ(&persistent, &left.arena());
    EXPECT_EQ(&persistent, &right.arena());
}

TYPED_TEST(TestBigValueStorage, copiesMovesAndExportsRemainValidAfterContextReset)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const TypeParam source{persistent, -1.25L};
    TypeParam assigned{2.5L};
    const auto expected = encoded(source);
    free_bf_vars();

    const TypeParam copy{source};
    TypeParam moving{source};
    const TypeParam moved{std::move(moving)};
    assigned = copy;

    EXPECT_EQ(expected, encoded(source));
    EXPECT_EQ(expected, encoded(copy));
    EXPECT_EQ(expected, encoded(moved));
    EXPECT_EQ(expected, encoded(assigned));
}

TYPED_TEST(TestBigValueStorage, precisionChangesMigrateEveryAttachedLifetime)
{
    std::vector<TypeParam> values;
    for (BigLifetime lifetime : {BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY})
    {
        values.emplace_back(value_arena<TypeParam>(lifetime), "-1.23456789012345678901234567890123456789");
    }
    for (int digits : {80, 20, 40})
    {
        const std::vector<Byte> old_bytes = encoded(values.front());
        const int old_integer = g_int_length;
        init_bf_dec(digits);
        std::vector<Byte> expected(values.front().arena().value_size());
        if constexpr (std::is_same_v<TypeParam, BigNum>)
        {
            ASSERT_EQ(old_integer, g_int_length);
        }
        // The most significant mantissa bytes and any exponent stay at the
        // end of the encoding. Growth adds zeros at the fractional end.
        const std::size_t retained = std::min(old_bytes.size(), expected.size());
        std::copy(old_bytes.end() - retained, old_bytes.end(), expected.end() - retained);
        for (const auto &value : values)
        {
            EXPECT_EQ(expected, encoded(value));
        }
    }
}

TYPED_TEST(TestBigValueStorage, temporaryRewindsLeavePersistentAndWorkingValuesIndependent)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    TypeParam destination{persistent, 1.25L};
    const TypeParam working{2.5L};
    const auto before = save_stack();
    {
        BigStackSaver scope;
        TypeParam temporary{value_arena<TypeParam>(BigLifetime::TEMPORARY), 1.25L};
        bf_stack().allocate_double();
        bn_stack().allocate_single();
        destination = temporary + working;
    }
    EXPECT_EQ(before, save_stack());
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, destination);
    EXPECT_EQ(TypeParam{2.5L}, working);
}

TYPED_TEST(TestBigValueStorage, compoundAssignmentsRetainTheDestinationAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const TypeParam right{value_arena<TypeParam>(BigLifetime::TEMPORARY), 0.5L};
    TypeParam destination{persistent, 1.25L};
    destination += right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.75L}, destination);
    destination -= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.25L}, destination);
    destination *= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{0.625L}, destination);
    destination /= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.25L}, destination);
}

TYPED_TEST(TestBigValueStorage, constructionInitializesRecycledStorage)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    auto previous = arena.allocate_single();
    std::fill_n(previous.data(), arena.block_size(), Byte{0xff});
    previous = {};

    const TypeParam zero{arena};

    EXPECT_EQ(std::vector<Byte>(arena.value_size()), encoded(zero));
}

} // namespace id::test
