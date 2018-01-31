// win32.h - Win32 port declarations
#ifndef WIN32_H
#define WIN32_H

// required for _unlink
#include <io.h>
#include <stdio.h>

#define id_fs_remove(x) _unlink(x)

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
