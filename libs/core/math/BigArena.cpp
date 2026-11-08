// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigArena.h>

#include <math/big_init.h>

#include <cassert>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace id::math
{
namespace
{

std::size_t round_up(const std::size_t value, const std::size_t alignment)
{
    if (value > std::numeric_limits<std::size_t>::max() - (alignment - 1))
    {
        throw std::length_error("BigArena size is too large");
    }
    return (value + alignment - 1) / alignment * alignment;
}

std::size_t aligned_block_size(const std::size_t size)
{
    if (size == 0)
    {
        throw std::invalid_argument("BigArena block size must be positive");
    }
    return round_up(size, BigArena::ALIGNMENT);
}

std::size_t paired_capacity(const std::size_t count)
{
    if (count == 0)
    {
        throw std::invalid_argument("BigArena block count must be positive");
    }
    return round_up(count, 2);
}

std::size_t storage_size(const std::size_t block_size, const std::size_t count)
{
    const std::size_t metadata_size = round_up(count, BigArena::ALIGNMENT);
    if (count > (std::numeric_limits<std::size_t>::max() - metadata_size) / block_size)
    {
        throw std::length_error("BigArena size is too large");
    }
    return block_size * count + metadata_size;
}

} // namespace

BigArena::Allocation::Allocation() = default;

BigArena::Allocation::Allocation(BigArena &arena, const Index index) :
    m_arena(&arena),
    m_index(index)
{
}

BigArena::Allocation::~Allocation()
{
    reset();
}

BigArena::Allocation::Allocation(Allocation &&other) noexcept :
    m_arena(std::exchange(other.m_arena, nullptr)),
    m_index(other.m_index)
{
}

BigArena::Allocation &BigArena::Allocation::operator=(Allocation &&other) noexcept
{
    if (this != &other)
    {
        reset();
        m_arena = std::exchange(other.m_arena, nullptr);
        m_index = other.m_index;
    }
    return *this;
}

BigArena::Allocation::operator bool() const noexcept
{
    return m_arena != nullptr;
}

BigArena &BigArena::Allocation::arena() const noexcept
{
    return *m_arena;
}

BigArena::Index BigArena::Allocation::index() const noexcept
{
    return m_index;
}

std::size_t BigArena::Allocation::blocks() const noexcept
{
    return m_arena->m_widths[m_index];
}

Byte *BigArena::Allocation::data() noexcept
{
    return m_arena->data(m_index);
}

const Byte *BigArena::Allocation::data() const noexcept
{
    return std::as_const(*m_arena).data(m_index);
}

BigArena::Allocation BigArena::Allocation::clone() const
{
    return clone(*m_arena);
}

BigArena::Allocation BigArena::Allocation::clone(BigArena &arena) const
{
    assert(arena.m_block_size == m_arena->m_block_size);
    const Byte width = m_arena->m_widths[m_index];
    Allocation result = width == 1 ? arena.allocate_single() : arena.allocate_double();
    std::memcpy(result.data(), data(), width * m_arena->m_block_size);
    return result;
}

void BigArena::Allocation::reduce()
{
    m_arena->reduce(m_index);
}

void BigArena::Allocation::reset() noexcept
{
    if (m_arena != nullptr)
    {
        m_arena->release(m_index);
        m_arena = nullptr;
    }
}

BigArena::Relocation::Relocation(BigArena &arena, std::unique_ptr<BigArena> replacement) :
    m_arena(&arena),
    m_replacement(std::move(replacement))
{
}

BigArena::Relocation::~Relocation() = default;
BigArena::Relocation::Relocation(Relocation &&) noexcept = default;
BigArena::Relocation &BigArena::Relocation::operator=(Relocation &&) noexcept = default;

void BigArena::Relocation::commit() noexcept
{
    std::swap(m_arena->m_block_size, m_replacement->m_block_size);
    std::swap(m_arena->m_storage_size, m_replacement->m_storage_size);
    std::swap(m_arena->m_storage, m_replacement->m_storage);
    std::swap(m_arena->m_widths, m_replacement->m_widths);
    std::swap(m_arena->m_free_pairs, m_replacement->m_free_pairs);
    std::swap(m_arena->m_free_halves, m_replacement->m_free_halves);
    m_replacement.reset();
}

BigArena::BigArena(const std::size_t block_size, const std::size_t block_count, const Reduce reduce,
    std::pmr::memory_resource &resource) :
    m_block_size(aligned_block_size(block_size)),
    m_capacity(paired_capacity(block_count)),
    m_storage_size(storage_size(m_block_size, m_capacity)),
    m_resource(resource),
    m_reduce(reduce)
{
    static_assert(sizeof(FreeLink) <= ALIGNMENT);
    if (reduce == nullptr)
    {
        throw std::invalid_argument("BigArena requires a reduction function");
    }
    // Width metadata shares the allocation, after the aligned payload region.
    m_storage = static_cast<Byte *>(m_resource.allocate(m_storage_size, ALIGNMENT));
    m_widths = m_storage + m_block_size * m_capacity;
    std::memset(m_widths, 0, m_capacity);
}

BigArena::~BigArena()
{
    m_resource.deallocate(m_storage, m_storage_size, ALIGNMENT);
}

BigArena::Allocation BigArena::allocate_single()
{
    Index index;
    if (m_free_halves != NO_INDEX)
    {
        index = m_free_halves;
        remove(m_free_halves, index);
    }
    else
    {
        index = take_pair();
        push(m_free_halves, index + 1);
    }
    m_widths[index] = 1;
    return Allocation{*this, index};
}

BigArena::Allocation BigArena::allocate_double()
{
    const Index index = take_pair();
    m_widths[index] = 2;
    return Allocation{*this, index};
}

void BigArena::relocate(const std::size_t block_size, const Migrate &migrate)
{
    auto pending = prepare_relocation(block_size, migrate);
    pending.commit();
}

BigArena::Relocation BigArena::prepare_relocation(const std::size_t block_size, const Migrate &migrate)
{
    auto storage = std::make_unique<BigArena>(block_size, m_capacity, m_reduce, m_resource);
    BigArena &replacement = *storage;
    replacement.m_high_water = m_high_water;
    for (Index index = 0; index < m_high_water;)
    {
        const Byte width = m_widths[index];
        if (width != 0)
        {
            migrate(replacement.data(index), data(index), width);
            replacement.m_widths[index] = width;
            index += width;
        }
        else
        {
            ++index;
        }
    }

    // Rebuild links in free blocks only; a wide companion is not a free half.
    for (Index index = 0; index < m_high_water; index += 2)
    {
        if (replacement.m_widths[index] == 2)
        {
            continue;
        }
        if (replacement.m_widths[index] == 0 && replacement.m_widths[index + 1] == 0)
        {
            replacement.push(replacement.m_free_pairs, index);
        }
        else
        {
            for (Index half = index; half < index + 2; ++half)
            {
                if (replacement.m_widths[half] == 0)
                {
                    replacement.push(replacement.m_free_halves, half);
                }
            }
        }
    }

    return Relocation{*this, std::move(storage)};
}

std::size_t BigArena::block_size() const noexcept
{
    return m_block_size;
}

std::size_t BigArena::capacity() const noexcept
{
    return m_capacity;
}

std::size_t BigArena::high_water_mark() const noexcept
{
    return m_high_water;
}

Byte *BigArena::data(const Index index) noexcept
{
    return m_storage + index * m_block_size;
}

const Byte *BigArena::data(const Index index) const noexcept
{
    return m_storage + index * m_block_size;
}

BigArena::FreeLink BigArena::links(const Index index) const noexcept
{
    FreeLink result;
    std::memcpy(&result, data(index), sizeof(result));
    return result;
}

void BigArena::set_links(const Index index, const FreeLink value) noexcept
{
    std::memcpy(data(index), &value, sizeof(value));
}

void BigArena::push(Index &head, const Index index) noexcept
{
    if (head != NO_INDEX)
    {
        FreeLink next = links(head);
        next.previous = index;
        set_links(head, next);
    }
    set_links(index, {NO_INDEX, head});
    head = index;
}

void BigArena::remove(Index &head, const Index index) noexcept
{
    const FreeLink entry = links(index);
    if (entry.previous == NO_INDEX)
    {
        head = entry.next;
    }
    else
    {
        FreeLink previous = links(entry.previous);
        previous.next = entry.next;
        set_links(entry.previous, previous);
    }
    if (entry.next != NO_INDEX)
    {
        FreeLink next = links(entry.next);
        next.previous = entry.previous;
        set_links(entry.next, next);
    }
}

BigArena::Index BigArena::take_pair()
{
    if (m_free_pairs != NO_INDEX)
    {
        const Index index = m_free_pairs;
        remove(m_free_pairs, index);
        return index;
    }
    if (m_capacity - m_high_water < 2)
    {
        throw PrecisionError("Out of BigArena space");
    }
    const Index index = m_high_water;
    m_high_water += 2;
    return index;
}

void BigArena::release(const Index index) noexcept
{
    assert(m_widths[index] == 1 || m_widths[index] == 2);
    if (m_widths[index] == 2)
    {
        m_widths[index] = 0;
        push(m_free_pairs, index);
    }
    else
    {
        m_widths[index] = 0;
        const Index other = index ^ 1;
        if (m_widths[other] == 0)
        {
            remove(m_free_halves, other);
            push(m_free_pairs, index & ~Index{1});
        }
        else
        {
            push(m_free_halves, index);
        }
    }
}

void BigArena::reduce(const Index index)
{
    assert(m_widths[index] == 2);
    m_reduce(data(index));
    m_widths[index] = 1;
    push(m_free_halves, index + 1);
}

} // namespace id::math
