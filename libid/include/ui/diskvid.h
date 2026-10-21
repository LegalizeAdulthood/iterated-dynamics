// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace id::ui
{

extern bool g_good_mode;

int start_disk();
// TODO: removed later
// Slice 41 moves potential-storage setup to its UI controller.
int pot_start_disk();
// TODO: removed later
// Slice 41 moves Targa-storage setup to its UI controller.
int targa_start_disk(std::FILE *file, int overhead);
// TODO: removed later
// Slice 41 moves storage setup to its UI controller.
int common_start_disk(std::int64_t row_size, std::int64_t col_size, int colors);
// TODO: removed later
// Slice 41 replaces per-access bridges with controller-owned progress.
int disk_read_pixel(int col, int row);
// TODO: removed later
// Slice 41 replaces per-access bridges with controller-owned progress.
void disk_write_pixel(int col, int row, int color);
// TODO: removed later
// Slice 41 replaces per-access bridges with controller-owned progress.
void targa_read_disk(unsigned int col, unsigned int row, Byte *red, Byte *green, Byte *blue);
// TODO: removed later
// Slice 41 replaces per-access bridges with controller-owned progress.
void targa_write_disk(unsigned int col, unsigned int row, Byte red, Byte green, Byte blue);
void dvid_status(int line, const char *msg);
inline void dvid_status(const int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}

} // namespace id::ui
