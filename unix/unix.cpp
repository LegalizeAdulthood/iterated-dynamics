// SPDX-License-Identifier: GPL-3.0-only
//
/* Unix.c
 * This file contains compatibility routines.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include <config/port.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ID_FILE_MAX_DIR   256       // max length of directory name
#define ID_FILE_MAX_FNAME  9       // max length of filename
#define ID_FILE_MAX_EXT    5       // max length of extension

/*
 *----------------------------------------------------------------------
 *
 * strlwr --
 *
 *      Convert string to lower case.
 *
 * Results:
 *      The string.
 *
 * Side effects:
 *      Modifies the string.
 *
 *----------------------------------------------------------------------
 */
char *
strlwr(char *s)
{
    for (char *sptr = s; *sptr; ++sptr)
    {
        *sptr = std::tolower(*sptr);
    }
    return s;
}

// tenths of millisecond timer routine
static struct timeval tv_start;

void restart_uclock()
{
    gettimeofday(&tv_start, nullptr);
}

typedef unsigned long uclock_t;
uclock_t usec_clock()
{
    uclock_t result;

    struct timeval tv, elapsed;
    gettimeofday(&tv, nullptr);

    elapsed.tv_usec  = tv.tv_usec -  tv_start.tv_sec;
    elapsed.tv_sec   = tv.tv_sec -   tv_start.tv_sec;

    if (elapsed.tv_usec < 0)
    {
        // "borrow
        elapsed.tv_usec += 1000000;
        elapsed.tv_sec--;
    }
    result  = (unsigned long)(elapsed.tv_sec*10000 +  elapsed.tv_usec/100);
    return result;
}
