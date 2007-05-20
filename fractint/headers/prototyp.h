#ifndef PROTOTYP_H
#define PROTOTYP_H

/* includes needed to define the prototypes */

#include "mpmath.h"
#include "big.h"
#include "fractint.h"
#include "helpcom.h"
#include "externs.h"

extern int get_corners();
extern int getakeynohelp();
extern void set_null_video();
extern void spindac(int, int);
extern void initasmvars();

/* maintain the common prototypes in this file
 * split the dos/win/unix prototypes into separate files.
 */

#ifdef XFRACT
#include "unixprot.h"
#endif

#ifdef _WIN32
#include "winprot.h"
#endif

extern long cdecl divide(long, long, int);
extern long cdecl multiply(long, long, int);
extern void put_line(int, int, int, BYTE *);
extern void get_line(int, int, int, BYTE *);
extern void find_special_colors();
extern long read_ticker();

/*  fractint -- C file prototypes */

extern int application_main(int argc, char **argv);
extern int elapsed_time(int);

/*  memory -- C file prototypes */
/* TODO: Get rid of this and use regular memory routines;
** see about creating standard disk memory routines for disk video
*/
extern int MemoryType(U16 handle);
extern void InitMemory();
extern void ExitCheck();
extern U16 MemoryAlloc(U16 size, long count, int stored_at);
extern void MemoryRelease(U16 handle);
extern bool MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
extern bool SetMemory(int value, U16 size, long count, long offset, U16 handle);

/*
 *  uclock -- C file prototypes 
 *  The  uclock_t typedef placed here because uclock.h
 *  prototype is for DOS version only.
 */
typedef unsigned long uclock_t;

extern uclock_t usec_clock();
extern void restart_uclock();
extern void wait_until(int index, uclock_t wait_time);

extern void init_failure(const char *message);
extern int expand_dirname(char *dirname, char *drive);
extern int abort_message(char *file, unsigned int line, int flags, char *msg);
#define ABORT(flags_, msg_) abort_message(__FILE__, __LINE__, flags_, msg_)

#ifndef DEBUG
/*#define DEBUG */
#endif

#endif
