#pragma once

#include "id.h"

#ifdef XFRACT
#include "unixprot.h"
#else
#ifdef _WIN32
#include "winprot.h"
#endif
#endif
extern void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
extern void get_line(int row, int startcol, int stopcol, BYTE *pixels);
extern long readticker();
extern int get_sound_params();
extern void setnullvideo();
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
extern uclock_t usec_clock();
extern void restart_uclock();
extern void init_failure(char const *message);
extern long stackavail();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void putcolor_a(int, int, int);