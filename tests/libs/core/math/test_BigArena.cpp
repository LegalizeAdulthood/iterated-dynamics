// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigArena.h>

#include <math/big_init.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory_resource>
#include <new>
#include <stdexcept>
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
    bool fail_next{};
    std::function<void()> on_deallocate;

private:
    struct Block
    {
        void *pointer;
        std::size_t size;
        std::size_t alignment;
    };

    void *do_allocate(std::size_t size, std::size_t align) override;
    void do_deallocate(void *pointer, std::size_t size, std::size_t align) override;
    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override;

    std::vector<Block> m_blocks;
};

void *CountingResource::do_allocate(const std::size_t size, const std::size_t align)
{
    if (std::exchange(fail_next, false))
    {
        throw std::bad_alloc{};
    }
    void *pointer = std::pmr::new_delete_resource()->allocate(size, align);
    m_blocks.push_back({pointer, size, align});
    ++allocations;
    bytes = size;
    alignment = align;
    return pointer;
}

void CountingResource::do_deallocate(void *pointer, const std::size_t size, const std::size_t align)
{
    const auto block = std::find_if(
        m_blocks.begin(), m_blocks.end(), [pointer](const Block &entry) { return entry.pointer == pointer; });
    ASSERT_NE(m_blocks.end(), block);
    EXPECT_EQ(block->size, size);
    EXPECT_EQ(block->alignment, align);
    m_blocks.erase(block);
    if (on_deallocate)
    {
        on_deallocate();
    }
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

TEST(TestBigArena, cloningASingleAllocationRetainsItsArenaAndOwnsAnIndependentBlock)
{
    BigArena arena{17, 2, keep_first_block};
    auto original = arena.allocate_single();
    std::fill_n(original.data(), arena.block_size(), Byte{0x5a});

    auto copy = std::as_const(original).clone();

    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_NE(original.index(), copy.index());
    EXPECT_NE(original.data(), copy.data());
    EXPECT_TRUE(std::equal(original.data(), original.data() + arena.block_size(), copy.data()));
    copy.data()[0] = 0x33;
    EXPECT_EQ(0x5a, original.data()[0]);
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
    const auto copy_index = copy.index();
    copy = {};
    const auto reused = arena.allocate_single();
    EXPECT_EQ(copy_index, reused.index());
    EXPECT_EQ(0x5a, original.data()[0]);
}

TEST(TestBigArena, cloningAWideAllocationCopiesBothBlocksAndReleasesBoth)
{
    BigArena arena{65, 4, keep_first_block};
    auto original = arena.allocate_double();
    const auto size = 2 * arena.block_size();
    std::fill_n(original.data(), arena.block_size(), Byte{0x11});
    std::fill_n(original.data() + arena.block_size(), arena.block_size(), Byte{0x5a});

    auto copy = original.clone();

    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_EQ(2, copy.index());
    EXPECT_TRUE(std::equal(original.data(), original.data() + size, copy.data()));
    copy.data()[size - 1] = 0x33;
    EXPECT_EQ(0x5a, original.data()[size - 1]);
    EXPECT_THROW(arena.allocate_single(), PrecisionError);
    copy = {};
    const auto reused = arena.allocate_double();
    EXPECT_EQ(2, reused.index());
    EXPECT_EQ(0x5a, original.data()[size - 1]);
}

class TestBigArenaClone : public testing::TestWithParam<bool>
{
};

TEST_P(TestBigArenaClone, cloningIntoASelectedArenaBindsAndReleasesThroughThatArena)
{
    BigArena source{65, 2, keep_first_block};
    BigArena destination{65, 2, keep_first_block};
    auto original = GetParam() ? source.allocate_double() : source.allocate_single();
    const auto size = (GetParam() ? 2 : 1) * source.block_size();
    std::fill_n(original.data(), size, Byte{0x5a});

    auto copy = original.clone(destination);

    EXPECT_EQ(&source, &original.arena());
    EXPECT_EQ(&destination, &copy.arena());
    EXPECT_NE(original.data(), copy.data());
    EXPECT_TRUE(std::equal(original.data(), original.data() + size, copy.data()));
    copy.data()[size - 1] = 0x33;
    EXPECT_EQ(0x5a, original.data()[size - 1]);
    copy = {};
    const auto reused = destination.allocate_double();
    EXPECT_EQ(0, reused.index());
    EXPECT_THROW(source.allocate_double(), PrecisionError);
    EXPECT_EQ(0x5a, original.data()[size - 1]);
}

INSTANTIATE_TEST_SUITE_P(Widths, TestBigArenaClone, testing::Bool());

TEST(TestBigArena, cloningAReducedAllocationUsesItsCurrentWidth)
{
    BigArena source{17, 2, reduce_from_second_block};
    BigArena destination{17, 2, keep_first_block};
    auto original = source.allocate_double();
    std::fill_n(original.data(), 128, Byte{0x5a});
    original.reduce();
    auto source_half = source.allocate_single();
    std::fill_n(source_half.data(), 64, Byte{0x33});

    const auto copy = original.clone(destination);

    EXPECT_TRUE(std::equal(original.data(), original.data() + 64, copy.data()));
    const auto destination_half = destination.allocate_single();
    EXPECT_EQ(1, destination_half.index());
    EXPECT_EQ(0x33, source_half.data()[0]);
}

TEST(TestBigArena, clonedWideAllocationsUseTheSelectedArenasReducer)
{
    BigArena source{17, 2, reduce_from_second_block};
    BigArena destination{17, 2, reduce_from_end};
    auto original = source.allocate_double();
    std::fill_n(original.data(), 128, Byte{});
    original.data()[64] = 23;
    original.data()[127] = 29;
    auto copy = original.clone(destination);
    const auto index = copy.index();

    copy.reduce();

    EXPECT_EQ(&destination, &copy.arena());
    EXPECT_EQ(index, copy.index());
    EXPECT_EQ(29, copy.data()[0]);
    EXPECT_EQ(0, original.data()[0]);
    EXPECT_EQ(23, original.data()[64]);
    const auto released_half = destination.allocate_single();
    EXPECT_EQ(index + 1, released_half.index());
    EXPECT_THROW(source.allocate_single(), PrecisionError);
}

TEST(TestBigArena, failedCloningPreservesTheSourceAndDestinationFreeHalf)
{
    BigArena source{17, 2, keep_first_block};
    BigArena destination{17, 2, keep_first_block};
    auto original = source.allocate_double();
    std::fill_n(original.data(), 128, Byte{0x5a});
    auto occupied = destination.allocate_single();
    occupied.data()[0] = 0x33;

    EXPECT_THROW(original.clone(), PrecisionError);
    EXPECT_THROW(original.clone(destination), PrecisionError);

    EXPECT_TRUE(std::all_of(original.data(), original.data() + 128, [](Byte value) { return value == 0x5a; }));
    EXPECT_EQ(0x33, occupied.data()[0]);
    const auto remaining = destination.allocate_single();
    EXPECT_EQ(1, remaining.index());
    EXPECT_EQ(2, destination.high_water_mark());
}

TEST(TestBigArena, repeatedCloningReusesBlocksWithoutBackingAllocations)
{
    CountingResource source_resource;
    CountingResource destination_resource;
    BigArena source{17, 4, keep_first_block, source_resource};
    BigArena destination{17, 4, keep_first_block, destination_resource};
    for (int i = 0; i < 1000; ++i)
    {
        auto original = i % 2 == 0 ? source.allocate_single() : source.allocate_double();
        const auto size = (i % 2 == 0 ? 1 : 2) * source.block_size();
        std::fill_n(original.data(), size, Byte{0x5a});
        const auto local = original.clone();
        const auto selected = original.clone(destination);
        EXPECT_EQ(0x5a, local.data()[size - 1]);
        EXPECT_EQ(0x5a, selected.data()[size - 1]);
    }

    EXPECT_EQ(4, source.high_water_mark());
    EXPECT_EQ(2, destination.high_water_mark());
    EXPECT_EQ(1, source_resource.allocations);
    EXPECT_EQ(1, destination_resource.allocations);
}

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

TEST(TestBigArena, relocationPreservesIndicesWidthsHolesAndTheHighWaterMark)
{
    CountingResource resource;
    BigArena arena{17, 10, reduce_from_end, resource};
    auto first = arena.allocate_single();
    auto second = arena.allocate_single();
    auto third = arena.allocate_single();
    auto fourth = arena.allocate_single();
    auto wide = arena.allocate_double();
    auto released_pair = arena.allocate_double();
    second = {};
    third = {};
    released_pair = {};
    std::fill_n(first.data(), 64, Byte{0x11});
    std::fill_n(fourth.data(), 64, Byte{0x33});
    std::fill_n(wide.data(), 128, Byte{0x55});
    wide.data()[127] = 0x7b;
    const Byte *old_first = first.data();
    const Byte *old_fourth = fourth.data();
    const Byte *old_wide = wide.data();
    std::vector<std::size_t> widths;
    std::vector<const Byte *> sources;
    resource.on_deallocate = [&]
    {
        EXPECT_NE(old_first, first.data());
        EXPECT_EQ(128, arena.block_size());
        EXPECT_EQ(0x11, first.data()[0]);
        EXPECT_EQ(0x7b, wide.data()[127]);
    };

    arena.relocate(65,
        [&](Byte *destination, const Byte *source, std::size_t width)
        {
            EXPECT_EQ(old_first, first.data());
            EXPECT_EQ(0, resource.deallocations);
            widths.push_back(width);
            sources.push_back(source);
            std::fill_n(destination, width * 128, Byte{});
            std::copy_n(source, width * 64, destination);
        });

    resource.on_deallocate = {};
    EXPECT_EQ((std::vector<std::size_t>{1, 1, 2}), widths);
    EXPECT_EQ((std::vector<const Byte *>{old_first, old_fourth, old_wide}), sources);
    EXPECT_EQ(10, arena.capacity());
    EXPECT_EQ(8, arena.high_water_mark());
    EXPECT_EQ(2, resource.allocations);
    EXPECT_EQ(1, resource.deallocations);
    EXPECT_EQ(0, resource.bytes % 64);
    EXPECT_EQ(64, resource.alignment);
    EXPECT_EQ(&arena, &first.arena());
    EXPECT_EQ(0, first.index());
    EXPECT_EQ(3, fourth.index());
    EXPECT_EQ(4, wide.index());
    EXPECT_EQ(0x33, fourth.data()[0]);
    EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(fourth.data()) % 64);
    EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(wide.data() + 128) % 64);

    auto half_a = arena.allocate_single();
    auto half_b = arena.allocate_single();
    EXPECT_EQ(1, std::min(half_a.index(), half_b.index()));
    EXPECT_EQ(2, std::max(half_a.index(), half_b.index()));
    const auto pair = arena.allocate_double();
    EXPECT_EQ(6, pair.index());
    EXPECT_EQ(8, arena.high_water_mark());
    const auto fresh = arena.allocate_double();
    EXPECT_EQ(8, fresh.index());
    EXPECT_EQ(10, arena.high_water_mark());
    EXPECT_THROW(arena.allocate_single(), PrecisionError);

    first = {};
    half_a = {};
    half_b = {};
    wide.reduce();
    EXPECT_EQ(4, wide.index());
    EXPECT_EQ(0x7b, wide.data()[0]);
    auto second_half = arena.allocate_single();
    EXPECT_EQ(5, second_half.index());
    const auto copy = wide.clone();
    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_EQ(0x7b, copy.data()[0]);
    EXPECT_EQ(0x55, copy.data()[126]);
    wide = {};
    second_half = {};
    const auto coalesced = arena.allocate_double();
    EXPECT_EQ(4, coalesced.index());
}

TEST(TestBigArena, failedBackingAllocationLeavesTheOldLayoutAndFreeListsUsable)
{
    CountingResource resource;
    BigArena arena{17, 4, keep_first_block, resource};
    auto first = arena.allocate_single();
    auto wide = arena.allocate_double();
    first.data()[0] = 23;
    wide.data()[127] = 29;
    const Byte *old_address = first.data();
    resource.fail_next = true;
    int conversions = 0;

    EXPECT_THROW(arena.relocate(65, [&](Byte *, const Byte *, std::size_t) { ++conversions; }), std::bad_alloc);

    EXPECT_EQ(0, conversions);
    EXPECT_EQ(1, resource.allocations);
    EXPECT_EQ(0, resource.deallocations);
    EXPECT_EQ(old_address, first.data());
    EXPECT_EQ(64, arena.block_size());
    EXPECT_EQ(4, arena.high_water_mark());
    EXPECT_EQ(23, first.data()[0]);
    EXPECT_EQ(29, wide.data()[127]);
    const auto half = arena.allocate_single();
    EXPECT_EQ(1, half.index());
    wide = {};
    const auto reused = arena.allocate_double();
    EXPECT_EQ(2, reused.index());
}

TEST(TestBigArena, failedConversionDiscardsTheReplacementAndAllowsARetry)
{
    CountingResource resource;
    BigArena arena{17, 6, keep_first_block, resource};
    auto first = arena.allocate_single();
    auto wide = arena.allocate_double();
    auto released_pair = arena.allocate_double();
    released_pair = {};
    std::fill_n(first.data(), 64, Byte{0x11});
    std::fill_n(wide.data(), 128, Byte{0x55});
    const Byte *old_address = first.data();
    int conversions = 0;
    const auto migrate = [&](Byte *destination, const Byte *source, std::size_t width)
    {
        ++conversions;
        std::fill_n(destination, width * 128, Byte{});
        std::copy_n(source, width * 64, destination);
        if (conversions == 2)
        {
            throw std::runtime_error("conversion failed");
        }
    };

    EXPECT_THROW(arena.relocate(65, migrate), std::runtime_error);

    EXPECT_EQ(2, conversions);
    EXPECT_EQ(2, resource.allocations);
    EXPECT_EQ(1, resource.deallocations);
    EXPECT_EQ(old_address, first.data());
    EXPECT_EQ(64, arena.block_size());
    EXPECT_EQ(6, arena.high_water_mark());
    EXPECT_EQ(0x11, first.data()[63]);
    EXPECT_EQ(0x55, wide.data()[127]);

    arena.relocate(65, migrate);

    EXPECT_EQ(4, conversions);
    EXPECT_EQ(3, resource.allocations);
    EXPECT_EQ(2, resource.deallocations);
    EXPECT_EQ(0x11, first.data()[63]);
    EXPECT_EQ(0x55, wide.data()[127]);
    const auto half = arena.allocate_single();
    const auto pair = arena.allocate_double();
    EXPECT_EQ(1, half.index());
    EXPECT_EQ(4, pair.index());
    EXPECT_EQ(6, arena.high_water_mark());
}

TEST(TestBigArena, relocatingAnEmptyArenaDoesNotConvertOrReserveBlocks)
{
    BigArena arena{17, 4, keep_first_block};
    int conversions = 0;

    arena.relocate(129, [&](Byte *, const Byte *, std::size_t) { ++conversions; });

    EXPECT_EQ(0, conversions);
    EXPECT_EQ(192, arena.block_size());
    EXPECT_EQ(0, arena.high_water_mark());
    const auto wide = arena.allocate_double();
    EXPECT_EQ(0, wide.index());
}

TEST(TestBigArena, rejectedRelocationGeometryLeavesLiveHandlesUntouched)
{
    CountingResource resource;
    BigArena arena{17, 2, keep_first_block, resource};
    auto value = arena.allocate_single();
    value.data()[0] = 42;
    const Byte *address = value.data();
    const auto migrate = [](Byte *, const Byte *, std::size_t) { ADD_FAILURE(); };

    EXPECT_THROW(arena.relocate(0, migrate), std::invalid_argument);
    EXPECT_THROW(arena.relocate(std::numeric_limits<std::size_t>::max(), migrate), std::length_error);

    EXPECT_EQ(1, resource.allocations);
    EXPECT_EQ(0, resource.deallocations);
    EXPECT_EQ(address, value.data());
    EXPECT_EQ(42, value.data()[0]);
    EXPECT_EQ(64, arena.block_size());
    const auto half = arena.allocate_single();
    EXPECT_EQ(1, half.index());
}

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
    const auto reused = arena.allocate_double();
    EXPECT_EQ(6, arena.high_water_mark());
    EXPECT_EQ(0, reused.index() % 2);
}

INSTANTIATE_TEST_SUITE_P(Representations, TestBigArenaNumericMigration, testing::Bool());

TEST(TestBigArena, preparedRelocationPublishesOnlyWhenCommitted)
{
    CountingResource resource;
    BigArena arena{17, 2, keep_first_block, resource};
    auto value = arena.allocate_single();
    value.data()[0] = 42;
    const Byte *old_address = value.data();
    auto pending = arena.prepare_relocation(
        65, [](Byte *destination, const Byte *source, std::size_t) { destination[0] = source[0]; });

    EXPECT_EQ(old_address, value.data());
    EXPECT_EQ(64, arena.block_size());
    EXPECT_EQ(2, resource.allocations);
    EXPECT_EQ(0, resource.deallocations);

    pending.commit();

    EXPECT_NE(old_address, value.data());
    EXPECT_EQ(128, arena.block_size());
    EXPECT_EQ(42, value.data()[0]);
    EXPECT_EQ(1, resource.deallocations);
}

TEST(TestBigArena, failurePreparingAnotherArenaDiscardsUncommittedRelocations)
{
    CountingResource first_resource;
    CountingResource second_resource;
    BigArena first{17, 2, keep_first_block, first_resource};
    BigArena second{17, 2, keep_first_block, second_resource};
    auto value = first.allocate_single();
    value.data()[0] = 42;
    const Byte *old_address = value.data();
    second_resource.fail_next = true;
    const auto migrate = [](Byte *destination, const Byte *source, std::size_t) { destination[0] = source[0]; };

    EXPECT_THROW(
        {
            auto first_pending = first.prepare_relocation(65, migrate);
            auto second_pending = second.prepare_relocation(65, migrate);
        },
        std::bad_alloc);

    EXPECT_EQ(old_address, value.data());
    EXPECT_EQ(42, value.data()[0]);
    EXPECT_EQ(64, first.block_size());
    EXPECT_EQ(64, second.block_size());
    EXPECT_EQ(1, first_resource.deallocations);
    EXPECT_EQ(0, second_resource.deallocations);
    const auto half = first.allocate_single();
    EXPECT_EQ(1, half.index());
}

} // namespace id::test
