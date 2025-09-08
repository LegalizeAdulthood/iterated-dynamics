// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/fractype.h"

namespace id::ui
{

int get_fract_type();
int get_fract_params(bool prompt_for_type_params);
void set_fractal_default_functions(fractals::FractalType previous);

} // namespace id::ui
