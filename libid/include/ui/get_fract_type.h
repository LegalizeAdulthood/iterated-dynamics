// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "fractals/fractype.h"

namespace id::ui
{

int get_fract_type();
int get_fract_params(bool prompt_for_type_params);
bool select_fractal_file_entry();

} // namespace id::ui
