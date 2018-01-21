// win32.h - Win32 port declarations
#ifndef WIN32_H
#define WIN32_H

#define remove(x) unlink(x)
#if !defined(_MAX_FNAME)
#define _MAX_FNAME 20
#endif
#if !defined(_MAX_EXT)
#define _MAX_EXT 4
#endif

// ftime replacement
#include <sys/types.h>
struct timebx
{
    time_t  time;
    unsigned short millitm;
    int   timezone;
    int   dstflag;
};
#endif
