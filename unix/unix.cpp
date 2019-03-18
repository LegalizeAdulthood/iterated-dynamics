/* Unix.c
 * This file contains compatibility routines.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include "port.h"

#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

#define FILE_MAX_DIR   256       // max length of directory name
#define FILE_MAX_FNAME  9       // max length of filename
#define FILE_MAX_EXT    5       // max length of extension

// stub
void
intdos()
{
}

/*
 *----------------------------------------------------------------------
 *
 * stackavail --
 *
 *      Returns amout of stack available.
 *
 * Results:
 *      Available stack.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
long stackavail()
{
    return 8192;
}

#ifndef HAVESTRI
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
    int c1, c2;

    while (1)
    {
        c1 = *s1++;
        c2 = *s2++;
        if (isupper(c1))
            c1 = tolower(c1);
        if (isupper(c2))
            c2 = tolower(c2);
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
    char c1, c2;

    for (; numChars > 0; --numChars)
    {
        c1 = *s1++;
        c2 = *s2++;
        if (isupper(c1))
            c1 = tolower(c1);
        if (isupper(c2))
            c2 = tolower(c2);
        if (c1 != c2)
        {
            return c1 - c2;
        }
        if (c1 == '\0')
        {
            return 0;
        }
    }
    return 0;
}
#endif

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
    char *sptr = s;
    while (*sptr != '\0')
    {
        if (isupper(*sptr))
        {
            *sptr = tolower(*sptr);
        }
        sptr++;
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
    char *sptr = s;
    while (*sptr != '\0')
    {
        if (islower(*sptr))
        {
            *sptr = toupper(*sptr);
        }
        sptr++;
    }
    return s;
}

/*
 *----------------------------------------------------------------------
 *
 * ltoa --
 *
 *      Convert long to string.
 *
 * Results:
 *      0.
 *
 * Side effects:
 *      Prints number into the string.
 *
 *----------------------------------------------------------------------
 */
int ltoa(long num, char *str, int len)
{
    sprintf(str, "%10d", (int)num);
    return 0;
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

/*
 *----------------------------------------------------------------------
 *
 * splitpath --
 *
 *      This is the splitpath code from prompts.c
 *
 * Results:
 *      Returns drive, dir, base, and extension.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    int length;
    int len;
    int offset;
    char const *tmp;

    if (drive)
        drive[0] = 0;
    if (dir)
        dir[0]   = 0;
    if (fname)
        fname[0] = 0;
    if (ext)
        ext[0]   = 0;

    length = std::strlen(file_template);
    if (length == 0)
        return 0;
    offset = 0;

    // get drive
    if (length >= 2)
        if (file_template[1] == ':')
        {
            if (drive)
            {
                drive[0] = file_template[offset++];
                drive[1] = file_template[offset++];
                drive[2] = 0;
            }
            else
            {
                offset++;
                offset++;
            }
        }

    // get dir
    if (offset < length)
    {
        tmp = std::strrchr(file_template, SLASHC);
        if (tmp)
        {
            tmp++;  // first character after slash
            len = tmp - &file_template[offset];
            if (len >=0 && len < FILE_MAX_DIR && dir)
                        std::strncpy(dir, &file_template[offset], std::min(len, FILE_MAX_DIR));
            if (len < FILE_MAX_DIR && dir)
                        dir[len] = 0;
            offset += len;
        }
    }
    else
        return 0;

    // get fname
    if (offset < length)
    {
        tmp = std::strrchr(file_template, '.');
        if (tmp < std::strrchr(file_template, SLASHC) || tmp < std::strrchr(file_template, ':'))
        {
            tmp = nullptr; // in this case the '.' must be a directory
        }
        if (tmp)
        {
            // tmp++; */ /* first character past "."
            len = tmp - &file_template[offset];
            if ((len > 0) && (offset+len < length) && fname)
            {
                std::strncpy(fname, &file_template[offset], std::min(len, FILE_MAX_FNAME));
                if (len < FILE_MAX_FNAME)
                            fname[len] = 0;
                else
                    fname[FILE_MAX_FNAME-1] = 0;
            }
            offset += len;
            if ((offset < length) && ext)
            {
                std::strncpy(ext, &file_template[offset], FILE_MAX_EXT);
                ext[FILE_MAX_EXT-1] = 0;
            }
        }
        else if ((offset < length) && fname)
        {
            std::strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
            fname[FILE_MAX_FNAME-1] = 0;
        }
    }
    return 0;
}

int
_splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    return splitpath(file_template, drive, dir, fname, ext);
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
