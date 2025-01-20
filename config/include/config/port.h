// SPDX-License-Identifier: GPL-3.0-only
//
/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
*/

#pragma once

#include "config/port_config.h"

using Byte = unsigned char;

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

#if defined(_WIN32)
// ================================== Win32 definitions
#define LOW_BYTE_FIRST    1     // NOLINT(modernize-macro-to-enum)
#define SLASH_CH         '\\'   // NOLINT(modernize-macro-to-enum)
#define SLASH            "\\"
#define SLASH_SLASH      "\\\\"
#define SLASH_DOT        "\\."
#define DOT_SLASH        ".\\"
#define DOT_DOT_SLASH    "..\\"
// ================================== Win32 definitions
#else
// ================================== linux definitions

#ifndef unix
#define unix
#endif

typedef int sigfunc(int);

#define SLASH_CH      '/'
#define SLASH         "/"
#define SLASH_SLASH   "//"
#define SLASH_DOT     "/."
#define DOT_SLASH     "./"
#define DOT_DOT_SLASH "../"

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

// ================================== linux definitions
#endif
// Uses BIG_ACCESS32(), BIG_SET32(), ... functions instead of macros.
// Some little endian machines may require this as well.
#if BYTE_ORDER == BIG_ENDIAN
#define ACCESS_BY_BYTE
#endif

// MSVC on x64 doesn't have a distinct type for long double vs. double
using LDouble = long double;
