/* Unix.c
 * This file contains compatibility routines.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include "port.h"

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define FILE_MAX_DIR   256       // max length of directory name
#define FILE_MAX_FNAME  9       // max length of filename
#define FILE_MAX_EXT    5       // max length of extension

/*
 *----------------------------------------------------------------------
 *
 * stricmp --
 *
 *      Compare strings, ignoring case.
 *
 * Results:
 *      -1,0,1.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int stricmp(char const *s1, char const *s2)
{
    while (1)
    {
        const int c1 = std::tolower(*s1++);
        const int c2 = std::tolower(*s2++);
        if (c1 != c2)
        {
            return c1 - c2;
        }
        if (c1 == 0)
        {
            return 0;
        }
    }
}
/*
 *----------------------------------------------------------------------
 *
 * strnicmp --
 *
 *      Compare strings, ignoring case.  Maximum length is specified.
 *
 * Results:
 *      -1,0,1.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int strnicmp(char const *s1, char const *s2, int numChars)
{
    for (; numChars > 0; --numChars)
    {
        const int c1 = std::tolower(*s1++);
        const int c2 = std::tolower(*s2++);
        if (c1 != c2)
        {
            return c1 - c2;
        }
        if (c1 == 0)
        {
            return 0;
        }
    }
    return 0;
}

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
/*
 *----------------------------------------------------------------------
 *
 * strupr --
 *
 *      Convert string to upper case.
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
strupr(char *s)
{
    for (char *sptr = s; *sptr; ++sptr)
    {
        *sptr = std::toupper(*sptr);
    }
    return s;
}

/*
 *----------------------------------------------------------------------
 *
 * filelength --
 *
 *      Find length of a file.
 *
 * Results:
 *      Length.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int filelength(int fd)
{
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

// sound.c file prototypes
int get_sound_params()
{
    return 0;
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
