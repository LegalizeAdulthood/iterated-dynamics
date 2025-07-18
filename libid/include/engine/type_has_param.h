// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/fractype.h"

bool type_has_param(FractalType type, int param, const char **prompt);
inline bool type_has_param(FractalType type, int param)
{
    return type_has_param(type, param, nullptr);
}
