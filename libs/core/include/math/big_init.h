// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/BigArena.h>

#include <cstddef>
#include <vector>

namespace id::fractals
{
enum class FractalType;
}

namespace id::math
{

class MPPrecision;

// values that g_bf_math can hold,
// 0 = g_bf_math is not being used
// 2 = g_bf_math is being used
enum class BFMathType
{
    // cppcheck-suppress variableHidingEnum
    NONE = 0,
    BIG_FLT = 2 // g_bf_math is being used with BigFloat numbers
};

// globals
extern BFMathType g_bf_math;
extern int g_bn_step;
extern int g_int_length;
extern int g_bn_length;
extern int g_r_length;
extern int g_padding;
extern int g_decimals;
extern int g_shift_factor;
extern int g_bf_length;
extern int g_r_bf_length;
extern int g_bf_decimals;
extern int g_bf_digits; // digits to use (force) for g_bf_math

// Request the precision needed to distinguish adjacent pixels at the
// maximum resolution of MAX_PIXELS by MAX_PIXELS or at current resolution
enum class ResolutionFlag
{
    MAX = 0,
    CURRENT = 1,
};

void calc_lengths();

enum class BigLifetime
{
    PERSISTENT,
    WORKING,
    TEMPORARY
};

// Arenas have stable addresses and outlive active legacy bindings.
// Call init_bf_dec or init_bf_length before obtaining an arena.
BigArena &bf_arena(BigLifetime lifetime);
MPPrecision bf_precision();

// Paired operand-preservation workspaces.
extern BigArena::Allocation g_bf_tmp_copy1; // Paired operand-preservation workspace.
extern BigArena::Allocation g_bf_tmp_copy2; // Paired operand-preservation workspace.

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

BigStack &bf_stack();

struct BigStackMark
{
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
