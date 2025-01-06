// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include <cstdio>
#include <string>

extern bool                  g_disk_16_bit;
extern bool                  g_disk_flag;       // disk video active flag
extern bool                  g_disk_targa;
extern bool                  g_good_mode;       // video mode ok?

int start_disk();
int pot_start_disk();
int targa_start_disk(std::FILE *targa_fp, int overhead);
void end_disk();
int disk_read_pixel(int col, int row);
void disk_write_pixel(int col, int row, int color);
void targa_read_disk(unsigned int col, unsigned int row, Byte *red, Byte *green, Byte *blue);
void targa_write_disk(unsigned int col, unsigned int row, Byte red, Byte green, Byte blue);
void dvid_status(int line, char const *msg);
inline void dvid_status(int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}
int common_start_disk(long new_row_size, long new_col_size, int colors);
bool from_mem_disk(long offset, int size, void *dest);
bool to_mem_disk(long offset, int size, void *src);
