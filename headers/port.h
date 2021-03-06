/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
*/
#ifndef PORT_H
#define PORT_H

#include <cstdint>

using U8 = std::uint8_t;
using S8 = std::int8_t;
using U16 = std::uint16_t;
using S16 = std::int16_t;
using U32 = std::uint32_t;
using S32 = std::int32_t;
using BYTE = U8;

#if defined(_WIN32)
#include <crtdbg.h>
// disable deprecated CRT warnings
#pragma warning(disable: 4996)
#endif

// If endian.h is not present, it can be handled in the code below,
// but if you have this file, it can make it more fool proof.
#if defined(XFRACT)
#include <endian.h>
#endif

#ifndef BIG_ENDIAN
#  define BIG_ENDIAN    4321  // to show byte order (taken from gcc)
#endif
#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
#endif

#define write1(ptr, len, n, stream) fwrite(ptr, len, n, stream)
#define write2(ptr, len, n, stream) fwrite(ptr, len, n, stream)
#define rand15() (rand() & 0x7FFF)

#if defined(_WIN32)
// ================================== Win32 definitions
#define LOBYTEFIRST    1
#define SLASHC         '\\'
#define SLASH          "\\"
#define SLASHSLASH     "\\\\"
#define SLASHDOT       "\\."
#define DOTSLASH       ".\\"
#define DOTDOTSLASH    "..\\"
#ifndef BYTE_ORDER
// change for little endians that don't have this defined elsewhere (endian.h)
#define BYTE_ORDER BIG_ENDIAN // the usual case
#endif
#include "win32.h"
// ================================== Win32 definitions
#else
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _alloca alloca
#if defined(XFRACT)
#ifndef unix
#define unix
#endif
#endif

#ifdef unix                     // Unix machine
typedef int sigfunc(int);
#ifndef BYTE_ORDER
// change for little endians that don't have this defined elsewhere (endian.h)
#ifdef LINUX
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN // the usual case
#endif
#endif
#       define SLASHC         '/'
#       define SLASH          "/"
#       define SLASHSLASH     "//"
#       define SLASHDOT       "/."
#       define DOTSLASH       "./"
#       define DOTDOTSLASH    "../"
#       include "unix.h"
#    endif // unix
#endif // _WIN32
// Uses big_access32(), big_set32(), ... functions instead of macros.
// Some little endian machines may require this as well.
#if BYTE_ORDER == BIG_ENDIAN
#define ACCESS_BY_BYTE
#endif
#ifdef LOBYTEFIRST
#define GET16(c, i)              (i) = *((U16*)(&(c)))
#else
#define GET16(c, i)              (i) = (*(unsigned char *)&(c))+\
                                ((*((unsigned char*)&(c)+1)) << 8)
#endif
using LDBL = long double;
#endif  /* PORT_H */
