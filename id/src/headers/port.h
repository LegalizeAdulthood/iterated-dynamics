/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
**
** XFRACT file "SHARED.H" merged into PORT.H on 3/14/92 by --CWM--
** TW also merged in Wes Loewer's BIGPORT.H.
*/

#ifndef PORT_H          // If this is defined, this file has been
#define PORT_H          // included already in this module.

#include <cstdlib>
#if defined(_WIN32)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define _CRT_SECURE_NO_DEPRECATE
// disable unsafe CRT warnings
#pragma warning(disable: 4996)

#endif

#include <cmath>
#include <cfloat>

// If endian.h is not present, it can be handled in the code below,
// but if you have this file, it can make it more fool proof.
#if (defined(XFRACT) && !defined(__sun))
#  if defined(sgi)
#    include <sys/endian.h>
#  else
#    include <endian.h>
#  endif
#endif
#ifndef BIG_ENDIAN
#  define BIG_ENDIAN    4321  // to show byte order (taken from gcc)
#endif
#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
#endif

#if defined(_WIN32)
// _WIN32 uses a flat model
#      ifdef far
#        undef far
#      endif
#      define far
#      ifdef _far
#        undef _far
#      endif
#      define _far
#      ifdef __far
#        undef __far
#      endif
#      define __far
#else
#  ifdef XFRACT
#    ifndef unix
#      define unix
#    endif
#  endif  // XFRACT
#endif // _WIN32

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

#		define LOBYTEFIRST    1
#		define SLASHC         '\\'
#		define SLASH          "\\"
#		define SLASHSLASH     "\\\\"
#		define SLASHDOT       "\\."
#		define DOTSLASH       ".\\"
#		define DOTDOTSLASH    "..\\"
#		define READMODE        "rb"    // Correct DOS text-mode
#		define WRITEMODE       "wb"    // file open "feature".

#		define rand15() rand()

#		ifndef BYTE_ORDER
		// change for little endians that don't have this defined elsewhere (endian.h)
#		ifdef LINUX
#		define BYTE_ORDER LITTLE_ENDIAN
#		else
#		define BYTE_ORDER BIG_ENDIAN // the usual case
#		endif
#		endif

		// TODO: we should refactor this into something better instead of using unix.h
#		include "unix.h"

		/*================================== Win32 definitions */

#else
#    ifdef unix                     // Unix machine
        typedef unsigned char  U8;
        typedef signed char    S8;
        typedef unsigned short U16;
        typedef short          S16;
        typedef unsigned long  U32;
        typedef long           S32;
        typedef unsigned char  BYTE;
        typedef char           CHAR;

#		ifndef __cdecl
#		define __cdecl
#		endif

#		ifdef __SVR4
		typedef void          *VOIDPTR;
#		else
#		 ifdef BADVOID
		typedef char          *VOIDPTR;
#		 else
		typedef void          *VOIDPTR;
#		 endif
#		endif

#		ifdef __SVR4
#		 include <fcntl.h>
		typedef void sigfunc(int);
#		else
		typedef int sigfunc(int);
#		endif

#		ifndef BYTE_ORDER
		// change for little endians that don't have this defined elsewhere (endian.h)
#		ifdef LINUX
#		define BYTE_ORDER LITTLE_ENDIAN
#		else
#		define BYTE_ORDER BIG_ENDIAN // the usual case
#		endif
#		endif

#       define SLASHC         '/'
#       define SLASH          "/"
#       define SLASHSLASH     "//"
#       define SLASHDOT       "/."
#       define DOTSLASH       "./"
#       define DOTDOTSLASH    "../"
#       define READMODE       "r"
#       define WRITEMODE        "w"

#       define rand15() (rand()&0x7FFF)

#       include "unix.h"


#    endif // unix
#endif // _WIN32

// Uses big_access32(), big_set32(),... functions instead of macros.
// Some little endian machines may require this as well.
#if BYTE_ORDER == BIG_ENDIAN
#define ACCESS_BY_BYTE
#endif

#ifdef LOBYTEFIRST
inline int Get16(const BYTE *data)
{
	return *reinterpret_cast<const U16 *>(data);
}
#else
inline int Get16(const BYTE *data)
{
	return *data + (*(data + 1) << 8);
}
#endif

// Some compiler libraries don't correctly handle long double.
// If you want to force the use of doubles, or
// if the compiler supports long doubles, but does not allow
// scanf("%Lf", &longdoublevar);
// to read in a long double, then uncomment this next line
// #define DO_NOT_USE_LONG_DOUBLE

// HP-UX support long doubles and allows them to be read in with
// scanf(), but does not support the functions sinl, cosl, fabsl, etc.
// CAE added this 26Jan95 so it would compile (altered by Wes to new macro)
#ifdef _HPUX_SOURCE
#define DO_NOT_USE_LONG_DOUBLE
#endif

/* Solaris itself does not provide long double arithmetics like sinl.
 * However, the "sunmath" library that comes bundled with Sun C does
 * provide them. */
#ifdef sun
#ifdef USE_SUNMATH
#include <sunmath.h>
#else
#define DO_NOT_USE_LONG_DOUBLE
#endif
#endif

// This should not be neccessary, but below appears to not work
#ifdef CYGWIN
#define DO_NOT_USE_LONG_DOUBLE
#endif

#ifndef DO_NOT_USE_LONG_DOUBLE
#ifdef LDBL_DIG
// this is what we're hoping for
#define USE_LONG_DOUBLE
typedef long double LDBL;
#else
#define DO_NOT_USE_LONG_DOUBLE
#endif // #ifdef LDBL_DIG
#endif // #ifndef DO_NOT_USE_LONG_DOUBLE


#ifdef DO_NOT_USE_LONG_DOUBLE

#ifdef USE_LONG_DOUBLE
#undef USE_LONG_DOUBLE
#endif

// long double isn't supported
// impliment LDBL as double
        typedef double          LDBL;

#if !defined(LDBL_DIG)
#define LDBL_DIG        DBL_DIG        // # of decimal digits of precision
#endif
#if !defined(LDBL_EPSILON)
#define LDBL_EPSILON    DBL_EPSILON    // smallest such that 1.0+LDBL_EPSILON != 1.0
#endif
#if !defined(LDBL_MANT_DIG)
#define LDBL_MANT_DIG   DBL_MANT_DIG   // # of bits in mantissa
#endif
#if !defined(LDBL_MAX)
#define LDBL_MAX        DBL_MAX        // max value
#endif
#if !defined(LDBL_MAX_10_EXP)
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP // max decimal exponent
#endif
#if !defined(LDBL_MAX_EXP)
#define LDBL_MAX_EXP    DBL_MAX_EXP    // max binary exponent
#endif
#if !defined(LDBL_MIN)
#define LDBL_MIN        DBL_MIN        // min positive value
#endif
#if !defined(LDBL_MIN_10_EXP)
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP // min decimal exponent
#endif
#if !defined(LDBL_MIN_EXP)
#define LDBL_MIN_EXP    DBL_MIN_EXP    // min binary exponent
#endif
#if !defined(LDBL_RADIX)
#define LDBL_RADIX      DBL_RADIX      // exponent radix
#endif
#if !defined(LDBL_ROUNDS)
#define LDBL_ROUNDS     DBL_ROUNDS     // addition rounding: near
#endif

#define sqrtl           sqrt
#define logl            log
#define log10l          log10
#define atanl           atan
#define fabsl           fabs
#define sinl            sin
#define cosl            cos
#endif

// PORT_H
#endif
