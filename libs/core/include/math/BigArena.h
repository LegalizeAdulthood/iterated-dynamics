// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <cstddef>
#include <functional>
#include <memory_resource>

namespace id::math
{

class BigArena
{
public:
    using Index = std::size_t;
    using Reduce = void (*)(Byte *);
    using Migrate = std::function<void(Byte *, const Byte *, std::size_t)>;
    static constexpr std::size_t ALIGNMENT = 64;

    class Allocation
    {
    public:
        Allocation();
        ~Allocation();
        Allocation(const Allocation &) = delete;
        Allocation(Allocation &&other) noexcept;
        Allocation &operator=(const Allocation &) = delete;
        Allocation &operator=(Allocation &&other) noexcept;

        explicit operator bool() const noexcept;
        // Accessors and cloning require a live allocation.
        BigArena &arena() const noexcept;
        Index index() const noexcept;
        Byte *data() noexcept;
        const Byte *data() const noexcept;
        // Copy all allocation bytes, preserving its width and allocator.
        Allocation clone() const;
        // The selected arena must have the same block size and numeric layout.
        Allocation clone(BigArena &arena) const;
        // Requires a live allocation obtained through allocate_double().
        void reduce();

    private:
        friend class BigArena;
        Allocation(BigArena &arena, Index index);
        void reset() noexcept;

        BigArena *m_arena{};
        Index m_index{};
    };

    // Allocation requests carry no byte count.
    // The arena and memory resource must outlive their allocations.
    BigArena(std::size_t block_size, std::size_t block_count, Reduce reduce,
        std::pmr::memory_resource &resource = *std::pmr::new_delete_resource());
    ~BigArena();
    BigArena(const BigArena &) = delete;
    BigArena(BigArena &&) = delete;
    BigArena &operator=(const BigArena &) = delete;
    BigArena &operator=(BigArena &&) = delete;

    // Payload bytes are uninitialized; numeric constructors initialize values.
    Allocation allocate_single();
    Allocation allocate_double();

    // Preserve indices, widths, capacity, and high-water mark in a new layout.
    // The callback receives the allocation width (1 or 2) and must initialize
    // its destination without modifying the source or this arena. Failure
    // leaves the old storage and handles usable. Borrowed addresses expire
    // only after successful relocation, including when the stride is unchanged.
    void relocate(std::size_t block_size, const Migrate &migrate);

    std::size_t block_size() const noexcept;
    std::size_t capacity() const noexcept;
    std::size_t high_water_mark() const noexcept;

private:
    static constexpr Index NO_INDEX = static_cast<Index>(-1);

    struct FreeLink
    {
        Index previous;
        Index next;
    };

    Byte *data(Index index) noexcept;
    const Byte *data(Index index) const noexcept;
    FreeLink links(Index index) const noexcept;
    void set_links(Index index, FreeLink value) noexcept;
    void push(Index &head, Index index) noexcept;
    void remove(Index &head, Index index) noexcept;
    Index take_pair();
    void release(Index index) noexcept;
    void reduce(Index index);

    std::size_t m_block_size;
    std::size_t m_capacity;
    std::size_t m_storage_size;
    std::pmr::memory_resource &m_resource;
    Reduce m_reduce;
    Byte *m_storage{};
    // A wide allocation has width 2 at its start and 0 at its second block.
    Byte *m_widths{};
    Index m_free_pairs{NO_INDEX};
    Index m_free_halves{NO_INDEX};
    Index m_high_water{};
};

} // namespace id::math
