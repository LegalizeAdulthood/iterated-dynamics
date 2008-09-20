/*
**  UCLOCK.H
**
**  Original Copyright 1988-1991 by Bob Stout as part of
**  the MicroFirm Function Library (MFL)
**
**  This subset version is functionally identical to the
**  version originally published by the author in Tech Specialist
**  magazine and is hereby donated to the public domain.
*/

#ifndef UCLOCK_H_INCLUDED
#define UCLOCK_H_INCLUDED

#include <dos.h>
#include <time.h>

#if defined(__ZTC__)
 #include <int.h>
 #undef int_on
 #undef int_off
#elif defined(__TURBOC__)
 #define int_on         enable
 #define int_off        disable
 #ifndef inp
  #define inp           inportb
 #endif
 #ifndef outp
  #define outp          outportb
 #endif
#else /* assume MSC/QC */
 #include <conio.h>
 #define int_on         _enable
 #define int_off        _disable
 #ifndef MK_FP
  #define MK_FP(seg,offset) \
        ((void far *)(((unsigned long)(seg)<<16) | (unsigned)(offset)))
 #endif
#endif

/* ANSI-equivalent declarations and prototypes */

typedef unsigned long uclock_t;

#define UCLK_TCK 1000000L /* Usec per second - replaces CLK_TCK         */

#if __cplusplus
 extern "C" {
#endif

uclock_t usec_clock(void);
void     restart_uclock(void);

#if __cplusplus
 }
#endif
#endif
