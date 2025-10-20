// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <string>

namespace id::ui
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

extern int                   g_auto_stereo_depth;
extern double                g_auto_stereo_width;
extern CalibrationBars       g_calibrate;           // add calibration bars to image
extern bool                  g_gray_flag;           // flag to use gray value rather than color number
extern bool                  g_image_map;
extern std::string           g_stereo_map_filename;

bool auto_stereo_convert();
int out_line_stereo(Byte *pixels, int line_len);

} // namespace id::ui
