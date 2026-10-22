// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string>

namespace id::geometry
{

enum class CalibrationBars
{
    NONE = 0,
    MIDDLE = 1,
    TOP = 2,
};

inline int operator+(const CalibrationBars value)
{
    return static_cast<int>(value);
}

extern int g_auto_stereo_depth;
extern double g_auto_stereo_width;
extern CalibrationBars g_calibrate; // add calibration bars to image
extern bool g_gray_flag;            // use gray value rather than color number
extern bool g_use_stereo_texture;
extern std::string g_stereo_texture_filename;

} // namespace id::geometry
