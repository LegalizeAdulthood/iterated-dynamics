// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

extern int                   g_row_count;       // row-counter for decoder and out_line
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern int                   g_video_start_x;
extern int                   g_video_start_y;

int get_color(int x, int y);
void put_color_a(int xdot, int ydot, int color);
int out_line(Byte *pixels, int linelen);
void write_span(int row, int startcol, int stopcol, Byte const *pixels);
void read_span(int row, int startcol, int stopcol, Byte *pixels);
void set_disk_dot();
void set_normal_dot();
void set_normal_span();
void set_null_video();
