// UNIX.H - unix port declarations
#ifndef UNIX_H
#define UNIX_H
#if !defined(O_BINARY)
#define O_BINARY 0
#endif
typedef float FLOAT4;
#define remove(x) unlink(x)
#if !defined(_MAX_FNAME)
#define _MAX_FNAME 20
#endif
#if !defined(_MAX_EXT)
#define _MAX_EXT 4
#endif
#define chsize(fd, en) ftruncate(fd, en)
// We get a problem with connect, since it is used by X
#define connect connect1
typedef void (*SignalHandler)(int);
extern int iocount;
char *strlwr(char *s);
char *strupr(char *s);
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
