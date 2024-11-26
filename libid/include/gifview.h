// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

extern unsigned int          g_height;
extern unsigned              g_num_colors;

int get_byte();
int get_bytes(BYTE *, int);
int gif_view();
int pot_line(BYTE *pixels, int linelen);
int sound_line(BYTE *pixels, int linelen);
