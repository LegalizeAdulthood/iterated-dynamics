#pragma once

#include "id.h"

#ifdef XFRACT
#include "unixprot.h"
#else
#endif
void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
void get_line(int row, int startcol, int stopcol, BYTE *pixels);
long readticker();
void setnullvideo();
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
uclock_t usec_clock();
void restart_uclock();
int getcolor(int, int);
int out_line(BYTE *, int);
void putcolor_a(int, int, int);
