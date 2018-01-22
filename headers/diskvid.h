#pragma once
#if !defined(DISKVID_H)
#define DISKVID_H

#include <stdio.h>

extern bool                  g_disk_16_bit;
extern bool                  g_disk_flag;       // disk video active flag
extern bool                  g_disk_targa;
extern bool                  g_good_mode;       // video mode ok?

extern int startdisk();
extern int pot_startdisk();
extern int targa_startdisk(FILE *, int);
extern void enddisk();
extern int readdisk(int, int);
extern void writedisk(int, int, int);
extern void targa_readdisk(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
extern void targa_writedisk(unsigned int, unsigned int, BYTE, BYTE, BYTE);
extern void dvid_status(int line, char const *msg);
extern int  common_startdisk(long, long, int);
extern int FromMemDisk(long, int, void *);
extern bool ToMemDisk(long, int, void *);

#endif
