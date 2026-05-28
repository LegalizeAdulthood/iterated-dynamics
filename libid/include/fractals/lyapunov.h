// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>

namespace id::fractals
{

struct LyapunovSequence
{
    int length{};
    std::array<int, 34> rxy{};
};

LyapunovSequence build_lyapunov_sequence(long order);
bool lyapunov_per_image();
int lyapunov_type();
int lyapunov_orbit();

} // namespace id::fractals
