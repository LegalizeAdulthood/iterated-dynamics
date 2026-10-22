// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "geometry/stereo_params.h"

namespace id::geometry
{

std::string g_stereo_texture_filename;
int g_auto_stereo_depth{100};
double g_auto_stereo_width{10};
bool g_gray_flag{};
CalibrationBars g_calibrate{CalibrationBars::MIDDLE};
bool g_use_stereo_texture{};

} // namespace id::geometry
