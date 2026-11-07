// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

namespace id::ui
{

// clang-format off
extern bool                  g_auto_stereo_batch;
extern bool                  g_save_rds_params;         // save RDS params in PAR output
extern bool                  g_stereo_texture_reuse;    // reuse current texture map filename
// clang-format on

bool auto_stereo_convert();
bool auto_stereo_batch_convert(); // convert, save, restore without prompting
int out_line_stereo(Byte *pixels, int line_len);
void random_dot_line(Byte *pixels, int line_len);
void lorenz_stereo_photographer_prompt();
void setup_stereo_palette();

} // namespace id::ui
