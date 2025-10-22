// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::io
{

extern bool                  g_dither_flag; // true if we want to dither GIFs
extern unsigned int          g_height;
extern unsigned              g_num_colors;

int get_byte();
int get_bytes(Byte *where, int how_many);
int gif_view();
int pot_line(Byte *pixels, int line_len);
int sound_line(Byte *pixels, int line_len);

} // namespace id::io
