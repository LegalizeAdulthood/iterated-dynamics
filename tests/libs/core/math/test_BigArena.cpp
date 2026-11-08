// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigArena.h>

#include <math/big_init.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::math;

namespace id::test
{
namespace
{

class CountingResource : public std::pmr::memory_resource
{
public:
    std::size_t allocations{};
    std::size_t deallocations{};
    std::size_t bytes{};
    std::size_t alignment{};

private:
    void *do_allocate(std::size_t size, std::size_t align) override;
    void do_deallocate(void *pointer, std::size_t size, std::size_t align) override;
    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override;
};

void *CountingResource::do_allocate(const std::size_t size, const std::size_t align)
{
    ++allocations;
    bytes = size;
    alignment = align;
    return std::pmr::new_delete_resource()->allocate(size, align);
}

void CountingResource::do_deallocate(void *pointer, const std::size_t size, const std::size_t align)
{
    EXPECT_EQ(bytes, size);
    EXPECT_EQ(alignment, align);
    ++deallocations;
    std::pmr::new_delete_resource()->deallocate(pointer, size, align);
}

bool CountingResource::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return &other == this;
}

void keep_first_block(Byte *)
{
}

void reduce_from_second_block(Byte *value)
{
    value[0] = value[64];
}

void reduce_from_end(Byte *value)
{
    value[0] = value[127];
}

static_assert(!std::is_copy_constructible_v<BigArena>);
static_assert(!std::is_move_constructible_v<BigArena>);
static_assert(!std::is_copy_constructible_v<BigArena::Allocation>);
static_assert(std::is_nothrow_move_constructible_v<BigArena::Allocation>);
static_assert(std::is_nothrow_move_assignable_v<BigArena::Allocation>);

} // namespace

class TestBigArenaAlignment : public testing::TestWithParam<std::size_t>
{
};

TEST_P(TestBigArenaAlignment, alignsEveryBlockAndBothHalvesOfPairs)
{
    CountingResource resource;
    BigArena arena{GetParam(), 7, keep_first_block, resource};
    std::vector<BigArena::Allocation> values;
    values.reserve(arena.capacity());

    for (std::size_t i = 0; i < arena.capacity(); ++i)
    {
        values.push_back(arena.allocate_single());
        EXPECT_EQ(i, values.back().index());
        EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values.back().data()) % 64);
    }

    EXPECT_EQ(8, arena.capacity());
    EXPECT_EQ(0, arena.block_size() % 64);
    EXPECT_GE(arena.block_size(), GetParam());
    EXPECT_LT(arena.block_size() - GetParam(), 64);
    EXPECT_EQ(arena.block_size(), static_cast<std::size_t>(values[1].data() - values[0].data()));
    EXPECT_EQ(1, resource.allocations);
    EXPECT_EQ(64, resource.alignment);
    EXPECT_EQ(0, resource.bytes % 64);
    EXPECT_THROW(arena.allocate_single(), PrecisionError);

    values.clear();
    for (std::size_t i = 0; i < arena.capacity() / 2; ++i)
    {
        values.push_back(arena.allocate_double());
        EXPECT_EQ(0, values.back().index() % 2);
        EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values.back().data()) % 64);
        EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values.back().data() + arena.block_size()) % 64);
    }
    EXPECT_EQ(1, resource.allocations);
}

INSTANTIATE_TEST_SUITE_P(BlockBytes, TestBigArenaAlignment, testing::Values(1U, 63U, 64U, 65U, 129U, 720U));

TEST(TestBigArena, reservesFreshStorageInPairs)
{
    BigArena arena{17, 8, keep_first_block};
    EXPECT_EQ(0, arena.high_water_mark());

    const auto first = arena.allocate_single();
    EXPECT_EQ(0, first.index());
    EXPECT_EQ(2, arena.high_water_mark());
    const auto second = arena.allocate_single();
    EXPECT_EQ(1, second.index());
    EXPECT_EQ(2, arena.high_water_mark());
    const auto wide = arena.allocate_double();
    EXPECT_EQ(2, wide.index());
    EXPECT_EQ(4, arena.high_water_mark());
}

TEST(TestBigArena, prefersAFreeHalfBeforeSplittingAWholePair)
{
    BigArena arena{17, 4, keep_first_block};
    auto first = arena.allocate_single();
    const auto second = arena.allocate_single();
    auto wide = arena.allocate_double();
    const auto wide_index = wide.index();
    wide = {};
    first = {};

    const auto half = arena.allocate_single();
    const auto pair = arena.allocate_double();

    EXPECT_EQ(0, half.index());
    EXPECT_EQ(wide_index, pair.index());
    EXPECT_EQ(4, arena.high_water_mark());
}

class TestBigArenaRelease : public testing::TestWithParam<bool>
{
};

TEST_P(TestBigArenaRelease, coalescesAReleasedPairInEitherOrder)
{
    BigArena arena{17, 2, keep_first_block};
    auto first = arena.allocate_single();
    auto second = arena.allocate_single();
    if (GetParam())
    {
        first = {};
        second = {};
    }
    else
    {
        second = {};
        first = {};
    }

    const auto wide = arena.allocate_double();

    EXPECT_EQ(0, wide.index());
    EXPECT_EQ(2, arena.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(Orders, TestBigArenaRelease, testing::Bool());

class TestBigArenaCoalescing : public testing::TestWithParam<std::size_t>
{
};

TEST_P(TestBigArenaCoalescing, coalescingPreservesOtherAvailableHalves)
{
    BigArena arena{17, 6, keep_first_block};
    std::vector<BigArena::Allocation> values;
    for (int i = 0; i < 6; ++i)
    {
        values.push_back(arena.allocate_single());
    }
    values[0] = {};
    values[2] = {};
    values[4] = {};

    // Remove the tail, middle, or head of the free-half list by coalescing.
    values[GetParam() + 1] = {};

    std::vector<BigArena::Index> expected{0, 2, 4};
    expected.erase(std::find(expected.begin(), expected.end(), GetParam()));
    const auto first = arena.allocate_single();
    const auto second = arena.allocate_single();
    const auto wide = arena.allocate_double();
    EXPECT_EQ(expected.front(), std::min(first.index(), second.index()));
    EXPECT_EQ(expected.back(), std::max(first.index(), second.index()));
    EXPECT_EQ(GetParam(), wide.index());
    EXPECT_EQ(6, arena.high_water_mark());
}

INSTANTIATE_TEST_SUITE_P(Pairs, TestBigArenaCoalescing, testing::Values(0U, 2U, 4U));

TEST(TestBigArena, doesNotJoinFreeHalvesAcrossPairBoundaries)
{
    BigArena arena{17, 4, keep_first_block};
    const auto first = arena.allocate_single();
    auto second = arena.allocate_single();
    auto third = arena.allocate_single();
    const auto fourth = arena.allocate_single();
    second = {};
    third = {};

    EXPECT_THROW(arena.allocate_double(), PrecisionError);

    const auto a = arena.allocate_single();
    const auto b = arena.allocate_single();
    EXPECT_EQ(1, std::min(a.index(), b.index()));
    EXPECT_EQ(2, std::max(a.index(), b.index()));
    EXPECT_EQ(4, arena.high_water_mark());
}

TEST(TestBigArena, reservesAnotherPairWhenOnlyIsolatedHalvesAreFree)
{
    BigArena arena{17, 6, keep_first_block};
    const auto single = arena.allocate_single();
    const auto first_wide = arena.allocate_double();

    const auto next_wide = arena.allocate_double();
    const auto half = arena.allocate_single();

    EXPECT_EQ(4, next_wide.index());
    EXPECT_EQ(1, half.index());
    EXPECT_EQ(6, arena.high_water_mark());
}

TEST(TestBigArena, exhaustionPreservesAFreeHalf)
{
    BigArena arena{17, 2, keep_first_block};
    auto first = arena.allocate_single();
    first.data()[0] = 42;

    EXPECT_THROW(arena.allocate_double(), PrecisionError);

    const auto second = arena.allocate_single();
    EXPECT_EQ(1, second.index());
    EXPECT_EQ(42, first.data()[0]);
    EXPECT_EQ(2, arena.high_water_mark());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
    first = {};
    const auto reused = arena.allocate_single();
    EXPECT_EQ(0, reused.index());
}

TEST(TestBigArena, releasesBothBlocksWhenAWideAllocationIsDestroyed)
{
    BigArena arena{17, 2, keep_first_block};
    {
        const auto wide = arena.allocate_double();
        EXPECT_EQ(0, wide.index());
    }

    const auto first = arena.allocate_single();
    const auto second = arena.allocate_single();

    EXPECT_EQ(0, first.index());
    EXPECT_EQ(1, second.index());
}

TEST(TestBigArena, movingTransfersOwnershipWithoutReleasingTheBlock)
{
    BigArena arena{17, 2, keep_first_block};
    auto source = arena.allocate_double();
    Byte *address = source.data();

    auto destination = std::move(source);

    EXPECT_FALSE(source);
    EXPECT_TRUE(destination);
    EXPECT_EQ(address, destination.data());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
    destination = {};
    const auto reused = arena.allocate_double();
    EXPECT_EQ(0, reused.index());
}

TEST(TestBigArena, movingHandlesBetweenArenasReleasesThroughTheOriginalOwner)
{
    BigArena first_arena{17, 2, keep_first_block};
    BigArena second_arena{17, 2, keep_first_block};
    auto first = first_arena.allocate_double();
    auto second = second_arena.allocate_double();
    Byte *address = first.data();

    second = std::move(first);

    EXPECT_EQ(address, second.data());
    const auto reused_second = second_arena.allocate_double();
    EXPECT_THROW(first_arena.allocate_double(), PrecisionError);
    second = {};
    const auto reused_first = first_arena.allocate_double();
    EXPECT_EQ(0, reused_first.index());
    EXPECT_EQ(0, reused_second.index());
}

TEST(TestBigArena, selfMovePreservesOwnership)
{
    BigArena arena{17, 2, keep_first_block};
    auto value = arena.allocate_double();
    Byte *address = value.data();
    auto &same = value;

    value = std::move(same);

    EXPECT_EQ(address, value.data());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
}

TEST(TestBigArena, reductionReadsTheWideValueBeforeReleasingItsSecondHalf)
{
    BigArena arena{17, 2, reduce_from_second_block};
    auto wide = arena.allocate_double();
    Byte *address = wide.data();
    const auto index = wide.index();
    std::fill_n(address, 64, Byte{0x11});
    std::fill_n(address + 64, 64, Byte{0x5a});

    wide.reduce();

    EXPECT_EQ(index, wide.index());
    EXPECT_EQ(address, wide.data());
    EXPECT_EQ(0x5a, wide.data()[0]);
    auto half = arena.allocate_single();
    EXPECT_EQ(index + 1, half.index());
    half.data()[0] = 0x33;
    EXPECT_EQ(0x5a, wide.data()[0]);
    wide = {};
    half = {};
    const auto reused = arena.allocate_double();
    EXPECT_EQ(index, reused.index());
}

TEST(TestBigArena, eachArenaUsesItsOwnReductionFunction)
{
    BigArena first_arena{17, 2, reduce_from_second_block};
    BigArena second_arena{17, 2, reduce_from_end};
    auto first = first_arena.allocate_double();
    auto second = second_arena.allocate_double();
    first.data()[64] = 23;
    first.data()[127] = 29;
    second.data()[64] = 31;
    second.data()[127] = 37;

    first.reduce();
    second.reduce();

    EXPECT_EQ(23, first.data()[0]);
    EXPECT_EQ(37, second.data()[0]);
}

TEST(TestBigArena, repeatedAllocationAndReductionReuseOneBackingAllocation)
{
    CountingResource resource;
    {
        BigArena arena{17, 4, reduce_from_second_block, resource};
        for (int i = 0; i < 1000; ++i)
        {
            const auto first = arena.allocate_single();
            const auto second = arena.allocate_single();
            auto wide = arena.allocate_double();
            wide.data()[64] = 42;
            wide.reduce();
            const auto released_half = arena.allocate_single();
            EXPECT_EQ(42, wide.data()[0]);
            EXPECT_EQ(4, arena.high_water_mark());
        }
        EXPECT_EQ(1, resource.allocations);
        EXPECT_EQ(0, resource.deallocations);
    }
    EXPECT_EQ(1, resource.deallocations);
}

TEST(TestBigArena, rejectsOverflowBeforeAllocatingBackingStorage)
{
    CountingResource resource;
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();

    EXPECT_THROW((BigArena{maximum, 2, keep_first_block, resource}), std::length_error);
    EXPECT_THROW((BigArena{64, maximum, keep_first_block, resource}), std::length_error);
    EXPECT_EQ(0, resource.allocations);
}

TEST(TestBigArena, rejectsEmptyGeometryAndMissingReductionBeforeAllocating)
{
    CountingResource resource;

    EXPECT_THROW((BigArena{0, 2, keep_first_block, resource}), std::invalid_argument);
    EXPECT_THROW((BigArena{17, 0, keep_first_block, resource}), std::invalid_argument);
    EXPECT_THROW((BigArena{17, 2, nullptr, resource}), std::invalid_argument);
    EXPECT_EQ(0, resource.allocations);
}

} // namespace id::test
