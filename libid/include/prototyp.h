#pragma once

#include "id.h"

#ifdef XFRACT
#include "unixprot.h"
#else
#ifdef _WIN32
#include "winprot.h"
#endif
#endif
void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
void get_line(int row, int startcol, int stopcol, BYTE *pixels);
long readticker();
int get_sound_params();
void setnullvideo();
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
uclock_t usec_clock();
void restart_uclock();
long stackavail();
int getcolor(int, int);
int out_line(BYTE *, int);
void putcolor_a(int, int, int);
