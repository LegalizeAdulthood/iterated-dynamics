// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#pragma once

#include "fractals/fractype.h"

namespace id::engine
{

// Complete a selection after the candidate type and parameters are applied.
void complete_fractal_type_selection(fractals::FractalType previous, bool accepted);

} // namespace id::engine
