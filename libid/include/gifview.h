// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

extern unsigned int          g_height;
extern unsigned              g_num_colors;

int get_byte();
int get_bytes(Byte *, int);
int gif_view();
int pot_line(Byte *pixels, int linelen);
int sound_line(Byte *pixels, int linelen);
