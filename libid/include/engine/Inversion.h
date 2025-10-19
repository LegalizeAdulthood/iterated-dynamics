// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

#include <array>

namespace id::engine
{

using InversionParams = std::array<double, 3>; // radius, x center, y center

struct Inversion
{
    int invert{};
    double radius{};
    math::DComplex center{};
    InversionParams params{};
};

extern Inversion g_inversion;

} // namespace id::engine
