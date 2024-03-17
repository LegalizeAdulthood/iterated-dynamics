/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
*/

#pragma once

#include "port_config.h"

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

#ifndef BIG_ENDIAN
#  define BIG_ENDIAN    4321  // to show byte order (taken from gcc)
#endif
#ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER
#if ID_BIG_ENDIAN
#define BYTE_ORDER BIG_ENDIAN
#else
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#endif

#define write1(ptr, len, n, stream) std::fwrite(ptr, len, n, stream)
#define write2(ptr, len, n, stream) std::fwrite(ptr, len, n, stream)
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
// ================================== Win32 definitions
#else
// ================================== linux definitions

#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _alloca alloca
#ifndef unix
#define unix
#endif

typedef int sigfunc(int);

#       define SLASHC         '/'
#       define SLASH          "/"
#       define SLASHSLASH     "//"
#       define SLASHDOT       "/."
#       define DOTSLASH       "./"
#       define DOTDOTSLASH    "../"

#if !defined(_MAX_FNAME)
#define _MAX_FNAME 20
#endif
#if !defined(_MAX_EXT)
#define _MAX_EXT 4
#endif

typedef float FLOAT4;

#define chsize(fd, en) ftruncate(fd, en)
// We get a problem with connect, since it is used by X
#define connect connect1
typedef void (*SignalHandler)(int);
char *strlwr(char *s);
char *strupr(char *s);

// ================================== linux definitions
#endif
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
