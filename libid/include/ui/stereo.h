// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <string>

namespace id::ui
{

extern int                   g_auto_stereo_depth;
extern double                g_auto_stereo_width;
extern char                  g_calibrate;
extern bool                  g_gray_flag;
extern bool                  g_image_map;
extern std::string           g_stereo_map_filename;

bool auto_stereo_convert();
int out_line_stereo(Byte *pixels, int line_len);

} // namespace id::ui
