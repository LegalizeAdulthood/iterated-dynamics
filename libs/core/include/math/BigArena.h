// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <utility>

namespace id::math
{

class PrecisionError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

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
        Allocation() noexcept;
        ~Allocation();
        Allocation(const Allocation &) = delete;
        Allocation(Allocation &&other) noexcept;
        Allocation &operator=(const Allocation &) = delete;
        Allocation &operator=(Allocation &&other) noexcept;

        explicit operator bool() const noexcept
        {
            return m_index != NO_INDEX;
        }

        // Arena binding survives a move; data requires a live block.
        BigArena *arena_ptr() const noexcept
        {
            return m_arena;
        }

        BigArena &arena() const noexcept
        {
            return *m_arena;
        }

        Index index() const noexcept
        {
            return m_index;
        }

        std::size_t blocks() const noexcept
        {
            return m_arena->m_widths[m_index];
        }

        Byte *data() noexcept
        {
            return m_arena->data(m_index);
        }

        const Byte *data() const noexcept
        {
            return std::as_const(*m_arena).data(m_index);
        }
        // Copy all allocation bytes, preserving its width and allocator.
        Allocation clone() const;
        // The selected arena must have the same block size and numeric layout.
        Allocation clone(BigArena &arena) const;
        // Release the block, retaining the arena binding for reassignment.
        void reset() noexcept;
        // Requires a live allocation obtained through allocate_double().
        void reduce()
        {
            m_arena->reduce(m_index);
        }

    private:
        friend class BigArena;
        Allocation(BigArena &arena, Index index);

        BigArena *m_arena{};
        Index m_index{NO_INDEX};
    };

    class Relocation
    {
    public:
        ~Relocation();
        Relocation(Relocation &&) noexcept;
        Relocation &operator=(Relocation &&) noexcept;
        Relocation(const Relocation &) = delete;
        Relocation &operator=(const Relocation &) = delete;
        // Commit once, after all related arenas have prepared successfully.
        void commit() noexcept;

    private:
        friend class BigArena;
        Relocation(BigArena &arena, std::unique_ptr<BigArena> replacement);
        BigArena *m_arena;
        std::unique_ptr<BigArena> m_replacement;
    };

    // Allocation requests carry no byte count.
    // The arena and memory resource must outlive their allocations.
    BigArena(std::size_t block_size, std::size_t block_count, Reduce reduce,
        std::pmr::memory_resource &resource = *std::pmr::new_delete_resource());
    BigArena(std::size_t value_size, std::size_t block_size, std::size_t block_count, Reduce reduce,
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
    void relocate(std::size_t value_size, std::size_t block_size, const Migrate &migrate);
    Relocation prepare_relocation(std::size_t block_size, const Migrate &migrate);
    Relocation prepare_relocation(std::size_t value_size, std::size_t block_size, const Migrate &migrate);

    // Canonical encoded extent, independent of aligned capacity and width.
    std::size_t value_size() const noexcept
    {
        return m_value_size;
    }

    std::size_t block_size() const noexcept
    {
        return m_block_size;
    }

    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    std::size_t high_water_mark() const noexcept
    {
        return m_high_water;
    }

private:
    static constexpr Index NO_INDEX = static_cast<Index>(-1);

    struct FreeLink
    {
        Index previous;
        Index next;
    };

    Byte *data(Index index) noexcept
    {
        return m_storage + index * m_block_size;
    }

    const Byte *data(Index index) const noexcept
    {
        return m_storage + index * m_block_size;
    }
    FreeLink links(Index index) const noexcept;
    void set_links(Index index, FreeLink value) noexcept;
    void push(Index &head, Index index) noexcept;
    void remove(Index &head, Index index) noexcept;
    Index take_pair();
    void release(Index index) noexcept;
    void reduce(Index index);

    std::size_t m_value_size;
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
