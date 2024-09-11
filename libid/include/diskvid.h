#pragma once

#include "port.h"

#include <cstdio>
#include <string>

extern bool                  g_disk_16_bit;
extern bool                  g_disk_flag;       // disk video active flag
extern bool                  g_disk_targa;
extern bool                  g_good_mode;       // video mode ok?

int startdisk();
int pot_startdisk();
int targa_startdisk(std::FILE *targafp, int overhead);
void enddisk();
int readdisk(int col, int row);
void writedisk(int col, int row, int color);
void targa_readdisk(unsigned int col, unsigned int row, BYTE *red, BYTE *green, BYTE *blue);
void targa_writedisk(unsigned int col, unsigned int row, BYTE red, BYTE green, BYTE blue);
void dvid_status(int line, char const *msg);
inline void dvid_status(int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}
int common_startdisk(long newrowsize, long newcolsize, int colors);
int FromMemDisk(long offset, int size, void *dest);
bool ToMemDisk(long offset, int size, void *src);
