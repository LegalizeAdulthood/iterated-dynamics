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
#define O_BINARY 0
#ifdef CLK_TCK
#undef CLK_TCK
#endif
#define CLK_TCK 1000
typedef float FLOAT4;
typedef short INT2;
typedef unsigned short UINT2;
typedef int INT4;
typedef unsigned int UINT4;
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))
#define remove(x) unlink(x)
#define _MAX_FNAME 20
#define _MAX_EXT 4
#define chsize(fd,len) ftruncate(fd,len)

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

/* External functions in unixscr.c */

int unixarg(int argc, char **argv, int *i);
	/* Parses xfractint-specific command line arguments */
void UnixInit(void);
	/* initializes curses text window and the signal handlers. */
void initUnixWindow(void);
	/* initializes the graphics window, colormap, etc. */
void UnixDone(void);
	/* cleans up X window and curses. */
int startvideo(void);
	/* clears the graphics window */
int endvideo(void);
	/* just a stub. */
int readvideo(int x, int y);
	/* reads a pixel from the screen. */
void readvideoline(int y, int x, int lastx, BYTE *pixels);
	/* reads a line of pixels from the screen. */
void writevideo(int x, int y, int color);
	/* writes a pixel to the screen. */
void writevideoline(int y, int x, int lastx, BYTE *pixels);
	/* writes a line of pixels to the screen. */
int readvideopalette(void);
	/* reads the current colormap into dacbox. */
int writevideopalette(void);
	/* writes the current colormap from dacbox. */
int resizeWindow(void);
	/* Checks if the window has been resized, and handles the resize.  */
int xgetkey(int block);
	/* Checks if a key has been pressed. */
unsigned char * xgetfont(void);
	/* Returns bitmap of an 8x8 font. */
void drawline(int x1, int y1, int x2, int y2);
	/* Draws a line from (x1,y1) to (x2,y2). */
void setlinemode(int mode);
	/* Sets line mode to draw or xor. */
void shell_to_dos(void);
	/* Calls a Unix subshell. */
void xsync(void);
	/* Forces all window events to be processed. */
void redrawscreen(void);
	/* Used with schedulealarm.  Xfractint has a delayed write mode,
	 * where the screen is updated only every few seconds.
	 */
void schedulealarm(int soon);
	/* Schedules the next delayed update. */

#endif
