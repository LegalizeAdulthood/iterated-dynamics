// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/big.h>
#include <math/BigArena.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace id::fractals
{
enum class FractalType;
}

namespace id::math
{

class PrecisionError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

extern int g_bf_digits;                 // digits to use (force) for g_bf_math

enum class BigLifetime
{
    PERSISTENT,
    WORKING,
    TEMPORARY
};

// Arenas have stable addresses and outlive active legacy bindings.
// Call init_bf_dec or init_bf_length before obtaining an arena.
BigArena &bn_arena(BigLifetime lifetime);
BigArena &bf_arena(BigLifetime lifetime);

void init_bn_product_scratch(BigArena &working);
bool is_bn_product_scratch(const Byte *source);
void free_bn_product_scratch();

class BigStack
{
public:
    explicit BigStack(BigArena &arena);
    BigStack(const BigStack &) = delete;
    BigStack &operator=(const BigStack &) = delete;
    // Retain each allocation's handle until the matching stack rewind.
    Byte *allocate_single();
    Byte *allocate_double();
    std::size_t mark() const;
    void restore(std::size_t mark);
    bool contains(const Byte *address) const;

private:
    BigArena &m_arena;
    std::vector<BigArena::Allocation> m_values;
};

BigStack &bn_stack();
BigStack &bf_stack();

struct BigStackMark
{
    std::size_t big_num{};
    std::size_t big_float{};
    bool operator==(const BigStackMark &right) const;
    bool operator!=(const BigStackMark &right) const;
    bool operator>(const BigStackMark &right) const;
};

struct BigMemoryStats
{
    std::size_t globals{};
    std::size_t temporaries{};
    std::size_t capacity{};
};

BigMemoryStats big_memory_stats();
std::vector<Byte> pack_bf_parameters();
void unpack_bf_parameters(const std::vector<Byte> &parameters);

void free_bf_vars();
BigStackMark save_stack();
void restore_stack(BigStackMark old_mark);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
// Validate stored precision and return the corner/parameter payload size.
size_t bf_parameter_bytes(int bn_length, fractals::FractalType type);
void init_big_pi();

class BigStackSaver
{
public:
    BigStackSaver();
    BigStackSaver(const BigStackSaver &rhs) = delete;
    BigStackSaver(BigStackSaver &&rhs) = delete;
    ~BigStackSaver();
    BigStackSaver &operator=(const BigStackSaver &rhs) = delete;
    BigStackSaver &operator=(BigStackSaver &&rhs) = delete;

private:
    BigStackMark m_mark;
};

} // namespace id::math
