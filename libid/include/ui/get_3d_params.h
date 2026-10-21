// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "ui/ThreeDSettings.h"

#include <string>

namespace id::ui
{

extern const std::string GLASSES1_MAP_NAME;

int get_3d_params(ThreeDSettings &settings);
int get_fract3d_params();

} // namespace id::ui
