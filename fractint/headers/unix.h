/* UNIX.H - unix port declarations */


#ifndef _UNIX_H
#define _UNIX_H

#define far
#define cdecl
#define huge
#define near
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
#if !defined(max)
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#if !defined(min)
#define min(a,b) ((a)<(b)?(a):(b))
#endif
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

#ifndef labs
#define labs(x) ((x)>0?(x):-(x))
#endif

/* We get a problem with connect, since it is used by X */
#define connect connect1
/* dysize may conflict with time.h */
#define dysize dysize1
/* inline is a reserved word, so fixed lsys.c */
/* #define inline inline1 */

typedef void (*SignalHandler)(int);

#ifdef NOSIGHAND
typedef void (*SignalHandler)(int);
#endif

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

char *strlwr(char *s);
char *strupr(char *s);

#if defined(_WIN32)
#define bcopy(src,dst,n) memcpy(dst,src,n)
#define bzero(buf,siz) memset(buf,0,siz)
#define bcmp(buf1,buf2,len) memcmp(buf1,buf2,len)
#else
#ifndef LINUX
#ifndef __SVR4
/* bcopy is probably faster than memmove, memcpy */
# ifdef memcpy   
#  undef memcpy   
# endif          
# ifdef memmove  
#  undef memmove  
# endif 

# define memcpy(dst,src,n) bcopy(src,dst,n)
# define memmove(dst,src,n) bcopy(src,dst,n)
#else
# define bcopy(src,dst,n) memcpy(dst,src,n)
# define bzero(buf,siz) memset(buf,0,siz)
# define bcmp(buf1,buf2,len) memcmp(buf1,buf2,len)
#endif
#endif
#endif

/* For Unix, all memory is FARMEM */
#define EXPANDED FARMEM
#define EXTENDED FARMEM

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
