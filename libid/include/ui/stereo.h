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

// clang-format off
extern int                   g_auto_stereo_depth;
extern bool                  g_auto_stereo_batch;
extern double                g_auto_stereo_width;
extern bool                  g_save_rds_params;         // save RDS params in PAR output
extern CalibrationBars       g_calibrate;               // add calibration bars to image
extern bool                  g_gray_flag;               // flag to use gray value rather than color number
extern bool                  g_use_stereo_texture;      //
extern std::string           g_stereo_texture_filename; //
extern bool                  g_stereo_texture_reuse;    // reuse current texture map filename
// clang-format on

bool auto_stereo_convert();
bool auto_stereo_batch_convert(); // convert, save, restore without prompting
int out_line_stereo(Byte *pixels, int line_len);
void random_dot_line(Byte *pixels, int line_len);
void lorenz_stereo_photographer_prompt();

} // namespace id::ui
