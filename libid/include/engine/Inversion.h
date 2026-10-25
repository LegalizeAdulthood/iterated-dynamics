// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/big.h"
#include "math/cmplx.h"

#include <array>

namespace id::engine
{

using InversionParams = std::array<double, 3>; // radius, x center, y center

struct Inversion
{
    double apply(math::DComplex &point) const;
    void apply(math::BFComplex point) const;

    int invert{};
    double radius{};
    math::DComplex center{};
    InversionParams params{};
};

extern Inversion g_inversion;

} // namespace id::engine
