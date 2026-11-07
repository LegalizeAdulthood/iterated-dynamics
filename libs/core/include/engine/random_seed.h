// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdint>

namespace id::engine
{

constexpr int ID_RANDOM_MAX{0x7FFF};

extern int g_random_seed;
extern bool g_random_seed_flag;

// Image generation random draws go through this API. Formula srand() uses
// its runtime state; UI-only behavior may use local std::rand() calls.
void set_random_seed(int seed);
void set_random_seed();
int random15(std::uint32_t &state);
int random15();
int random_int(int limit);
double random_unit();

} // namespace id::engine
