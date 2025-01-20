// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

extern unsigned int          g_height;
extern unsigned              g_num_colors;

int get_byte();
int get_bytes(Byte *where, int how_many);
int gif_view();
int pot_line(Byte *pixels, int line_len);
int sound_line(Byte *pixels, int line_len);
