// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::io
{

extern bool                  g_dither_flag; // true if we want to dither GIFs
extern unsigned int          g_height;
extern unsigned              g_num_colors;

int out_line_dither(Byte *pixels, int line_len);
int out_line_migs(Byte *pixels, int line_len, int left, int top);
int pot_line(Byte *pixels, int line_len);

} // namespace id::io
