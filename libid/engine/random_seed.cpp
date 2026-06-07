// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/random_seed.h"

#include <cstdint>

namespace id::engine
{
namespace
{

constexpr std::uint32_t RANDOM_MULTIPLIER{214013U};
constexpr std::uint32_t RANDOM_INCREMENT{2531011U};
std::uint32_t s_random_state{};

} // namespace

bool g_random_seed_flag{}; //
int g_random_seed{};       // Random number seeding flag and value

void set_random_seed(const int seed)
{
    s_random_state = static_cast<std::uint32_t>(seed);
}

void set_random_seed()
{
    set_random_seed(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }
}

int random15()
{
    return random15(s_random_state);
}

int random15(std::uint32_t &state)
{
    state = state * RANDOM_MULTIPLIER + RANDOM_INCREMENT;
    return static_cast<int>((state >> 16U) & RANDOM_MAX);
}

int random_int(const int limit)
{
    return limit <= 1 ? 0 : random15() % limit;
}

double random_unit()
{
    return static_cast<double>(random15()) / RANDOM_MAX;
}

} // namespace id::engine
