/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
*/

#ifndef PORT_H
#define PORT_H

#if defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define _CRT_SECURE_NO_DEPRECATE
/* disable unsafe CRT warnings */
#pragma warning(disable: 4996)
#endif

#if !defined(XFRACT) && !defined(_WIN32)
#  include <dos.h>
#else
#  if !defined(_WIN32)
#    include <unistd.h>
#  endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#if defined(_WIN32)
extern long stackavail();
#endif

/* If endian.h is not present, it can be handled in the code below, */
/* but if you have this file, it can make it more fool proof. */
#if (defined(XFRACT) && !defined(__sun))
#  if defined(sgi)
#    include <sys/endian.h>
#  else
#    include <endian.h>
#  endif
#endif
#ifndef BIG_ENDIAN
#  define BIG_ENDIAN    4321  /* to show byte order (taken from gcc) */
#endif
#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
#endif

#if defined(XFRACT)
#ifndef unix
#define unix
#endif
#endif  /* XFRACT  */

#if defined(_WIN32)
/*================================== Win32 definitions */
typedef unsigned char  U8;
typedef signed char    S8;
typedef unsigned short U16;
typedef signed short   S16;
typedef unsigned long  U32;
typedef signed long    S32;
typedef unsigned char  BYTE;
typedef void          *VOIDPTR;
typedef const void    *VOIDCONSTPTR;

#define CONST          const
#define PRINTER        "PRT:"
#define LOBYTEFIRST    1
#define SLASHC         '\\'
#define SLASH          "\\"
#define SLASHSLASH     "\\\\"
#define SLASHDOT       "\\."
#define DOTSLASH       ".\\"
#define DOTDOTSLASH    "..\\"
#define READMODE        "rb"    /* Correct DOS text-mode        */
#define WRITEMODE       "wb"    /* file open "feature".         */

#define write1(ptr,len,n,stream) fwrite(ptr,len,n,stream)
#define write2(ptr,len,n,stream) fwrite(ptr,len,n,stream)
#define rand15() rand()

#ifndef BYTE_ORDER
/* change for little endians that don't have this defined elsewhere (endian.h) */
#ifdef LINUX
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN /* the usual case */
#endif
#endif

/* TODO: we should refactor this into something better instead of using unix.h */
#include "unix.h"

/*================================== Win32 definitions */

#else
#    ifdef unix                     /* Unix machine */
typedef unsigned char  U8;
typedef signed char    S8;
typedef unsigned short U16;
typedef short          S16;
typedef unsigned long  U32;
typedef long           S32;
typedef unsigned char  BYTE;
typedef char           CHAR;

#ifdef __SVR4
typedef void          *VOIDPTR;
typedef const void    *VOIDCONSTPTR;
#else
typedef void          *VOIDPTR;
typedef const void    *VOIDCONSTPTR;
#endif

#ifdef __SVR4
# include <fcntl.h>
typedef void sigfunc(int);
#else
typedef int sigfunc(int);
#endif

#ifndef BYTE_ORDER
/* change for little endians that don't have this defined elsewhere (endian.h) */
#ifdef LINUX
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN /* the usual case */
#endif
#endif

#       define CONST          const
#       define PRINTER        "/dev/lp"
#       define SLASHC         '/'
#       define SLASH          "/"
#       define SLASHSLASH     "//"
#       define SLASHDOT       "/."
#       define DOTSLASH       "./"
#       define DOTDOTSLASH    "../"
#       define READMODE       "r"
#       define WRITEMODE        "w"

#       define write1(ptr,len,n,stream) (fputc(*(ptr),stream),1)
#       define write2(ptr,len,n,stream) (fputc((*(ptr))&255,stream),fputc((*(ptr))>>8,stream),1)
#       define rand15() (rand()&0x7FFF)

#       include "unix.h"


#    endif /* unix */
#endif /* _WIN32 */

/* Uses big_access32(), big_set32(),... functions instead of macros. */
/* Some little endian machines may require this as well. */
#if BYTE_ORDER == BIG_ENDIAN
#define ACCESS_BY_BYTE
#endif

#ifdef LOBYTEFIRST
#define GET16(c,i)              (i) = *((U16*)(&(c)))
#else
#define GET16(c,i)              (i) = (*(unsigned char *)&(c))+\
                                ((*((unsigned char*)&(c)+1))<<8)
#endif

typedef long double LDBL;

#if !defined(_WIN32)
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _alloca alloca
#endif

#endif  /* PORT_H */
