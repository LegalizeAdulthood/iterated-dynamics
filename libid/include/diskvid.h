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
int targa_startdisk(std::FILE *, int);
void enddisk();
int readdisk(int, int);
void writedisk(int, int, int);
void targa_readdisk(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
void targa_writedisk(unsigned int, unsigned int, BYTE, BYTE, BYTE);
void dvid_status(int line, char const *msg);
inline void dvid_status(int line, const std::string &msg)
{
    dvid_status(line, msg.c_str());
}
int  common_startdisk(long, long, int);
int FromMemDisk(long, int, void *);
bool ToMemDisk(long, int, void *);
