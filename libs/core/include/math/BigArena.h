// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <cstddef>
#include <memory_resource>

namespace id::math
{

class BigArena
{
public:
    using Index = std::size_t;
    using Reduce = void (*)(Byte *);
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
        Index index() const noexcept;
        Byte *data() noexcept;
        const Byte *data() const noexcept;
        // Requires a live allocation obtained through allocate_double().
        void reduce();

    private:
        friend class BigArena;
        Allocation(BigArena &arena, Index index);
        void reset() noexcept;

        BigArena *m_arena{};
        Index m_index{};
    };

    // Geometry is configured once; allocation requests carry no byte count.
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
