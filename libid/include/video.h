#pragma once

#include "port.h"

int getcolor(int x, int y);
void putcolor_a(int xdot, int ydot, int color);
int out_line(BYTE *pixels, int linelen);
void write_span(int row, int startcol, int stopcol, BYTE const *pixels);
void read_span(int row, int startcol, int stopcol, BYTE *pixels);
void set_disk_dot();
void set_normal_dot();
void set_normal_span();
void set_null_video();
