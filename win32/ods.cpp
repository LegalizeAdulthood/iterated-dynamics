// SPDX-License-Identifier: GPL-3.0-only
//
#include "ods.h"

#include "win_defines.h"
#include <Windows.h>

#include <cstdarg>
#include <cstdio>

/* ods
 *
 * varargs version of OutputDebugString with file and line markers.
 */
void ods(char const *file, unsigned int line, char const *format, ...)
{
    char full_msg[MAX_PATH+1];
    char app_msg[MAX_PATH+1];
    std::va_list args;

    va_start(args, format);
    std::vsnprintf(app_msg, MAX_PATH, format, args);
    std::snprintf(full_msg, MAX_PATH, "%s(%u): %s\n", file, line, app_msg);
    va_end(args);

    OutputDebugStringA(full_msg);
}
