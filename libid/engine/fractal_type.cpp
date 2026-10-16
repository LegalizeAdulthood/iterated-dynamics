// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include "engine/fractal_type.h"

#include "fractals/fractalp.h"
#include "misc/version.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::engine
{

void complete_fractal_type_selection(const FractalType previous, const bool accepted)
{
    if (accepted)
    {
        reset_version_to_current();
    }
    else
    {
        set_fractal_type(previous);
    }
}

} // namespace id::engine
