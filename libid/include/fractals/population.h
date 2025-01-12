// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cmath>

extern double g_population;
extern double g_rate;

inline bool population_exceeded()
{
    constexpr double LIMIT{100000.0};
    return std::abs(g_population) > LIMIT;
}

inline int population_orbit()
{
    return population_exceeded() ? 1 : 0;
}
