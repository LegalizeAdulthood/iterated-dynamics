// UNIX.H - unix port declarations
#pragma once

#include <unistd.h>

typedef float FLOAT4;

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
char *strlwr(char *s);
char *strupr(char *s);
