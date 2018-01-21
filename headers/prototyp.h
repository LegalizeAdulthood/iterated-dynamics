#ifndef PROTOTYP_H
#define PROTOTYP_H
// includes needed to define the prototypes
#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "externs.h"
// maintain the common prototypes in this file

#ifdef XFRACT
#include "unixprot.h"
#else
#ifdef _WIN32
#include "winprot.h"
#endif
#endif
extern long multiply(long x, long y, int n);
extern long divide(long x, long y, int n);
extern void spindac(int dir, int inc);
extern void put_line(int row, int startcol, int stopcol, BYTE const *pixels);
extern void get_line(int row, int startcol, int stopcol, BYTE *pixels);
extern void find_special_colors();
extern int getakeynohelp();
extern long readticker();
extern int get_sound_params();
extern void setnullvideo();
// memory -- C file prototypes
// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
extern void DisplayHandle(U16 handle);
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern bool CopyFromMemoryToHandle(BYTE const *buffer, U16 size, long count, long offset, U16 handle);
extern bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool SetMemory(int value, U16 size, long count, long offset, U16 handle);
// soi -- C file prototypes
extern void soi();
/*
 *  uclock -- C file prototypes
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;
extern uclock_t usec_clock();
extern void restart_uclock();
extern void wait_until(int index, uclock_t wait_time);
extern void init_failure(char const *message);
extern int expand_dirname(char *dirname, char *drive);
extern bool abortmsg(char const *file, unsigned int line, int flags, char const *msg);
#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
extern long stackavail();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void setvideomode(int, int, int, int);
extern int pot_startdisk();
extern void putcolor_a(int, int, int);
extern int startdisk();
#endif
