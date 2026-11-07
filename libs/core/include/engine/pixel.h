// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

namespace id::engine
{

using WritePixel = void (*)(int x, int y, int color);
using ReadPixel = int (*)(int x, int y);

void set_pixel_functions(WritePixel write_pixel, ReadPixel read_pixel);
bool disk_pixel_output();
int get_color(int x, int y);
void put_color_a(int x, int y, int color);
void write_span(int row, int start_col, int stop_col, const Byte *pixels);
void read_span(int row, int start_col, int stop_col, Byte *pixels);

} // namespace id::engine
