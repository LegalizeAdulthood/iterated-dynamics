// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>

namespace id::engine
{

struct Potential
{
    std::array<double, 3> params{}; // three potential parameters
    bool store_16bit{};             // store 16 bit continuous potential values
    bool flag{};                    // continuous potential enabled?
};

extern Potential g_potential;

} // namespace id::engine
