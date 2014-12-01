/* UNIX.H - unix port declarations */


#ifndef _UNIX_H
#define _UNIX_H

#ifndef RAND_MAX
#define RAND_MAX 0x7fffffff
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

#ifdef CLK_TCK
#undef CLK_TCK
#endif
#define CLK_TCK 1000
typedef float FLOAT4;
typedef short INT2;
typedef unsigned short UINT2;
typedef int INT4;
typedef unsigned int UINT4;
#define remove(x) unlink(x)
#if !defined(_MAX_FNAME)
#define _MAX_FNAME 20
#endif
#if !defined(_MAX_EXT)
#define _MAX_EXT 4
#endif
#if !defined(_WIN32)
#define chsize(fd,len) ftruncate(fd,len)
#endif

#define inp(x) 0
#define outp(x,y)

inline long labs(long x)
{
    return x >0 ? x : -x;
}

/* We get a problem with connect, since it is used by X */
#define connect connect1
/* dysize may conflict with time.h */
#define dysize dysize1
/* inline is a reserved word, so fixed lsys.c */
/* #define inline inline1 */

typedef void (*SignalHandler)(int);

/* Some stdio.h's don't have this */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

extern int iocount;

#if defined(_WIN32)
#define bcopy(src,dst,n) memcpy(dst,src,n)
#define bzero(buf,siz) memset(buf,0,siz)
#define bcmp(buf1,buf2,len) memcmp(buf1,buf2,len)
#else
char *strlwr(char *s);
char *strupr(char *s);
#endif

/*
 * These defines are so movedata, etc. will work properly, without worrying
 * about the silly segment stuff.
 */
#define movedata(s_seg,s_off,d_seg,d_off,len) bcopy(s_off,d_off,len)
struct SREGS {
    int ds;
};
#define FP_SEG(x) 0
#define FP_OFF(x) ((char *)(x))
#define segread(x)

/* ftime replacement */
#include <sys/types.h>
typedef struct  timebx
{
    time_t  time;
    unsigned short millitm;
    int   timezone;
    int   dstflag;
} timebx;


#endif
