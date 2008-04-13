/* UNIX.H - unix port declarations */
#if !defined(UNIX_H)
#define UNIX_H

#ifndef RAND_MAX
#define RAND_MAX 0x7fffffff
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

#if !defined(_WIN32)
#define chsize(fd,len) ftruncate(fd,len)
#endif

typedef void (*SignalHandler)(int);

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

/* ftime replacement */
#include <sys/types.h>
struct timebx
{
	time_t time;
	unsigned short millitm;
	int timezone;
	int dstflag;
};

#endif
