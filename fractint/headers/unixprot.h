#ifndef UNIXPROT_H
#define UNIXPROT_H

/* This file contains prototypes for unix/linux specific functions. */


/*  calmanp5 -- assembler file prototypes */

extern long  cdecl calcmandfpasm_c(void);
extern long  cdecl calcmandfpasm_p5(void);
extern void cdecl calcmandfpasmstart_p5(void);


/*
 *   general.c -- C file prototypes
 */
extern int waitkeypressed(int);
extern void fix_ranges(int *, int, int);
extern void decode_evolver_info(struct evolution_info *, int);
extern void decode_fractal_info(struct fractal_info *, int);
extern void decode_orbits_info(struct orbits_info *, int);

/*
 *   unix.c -- C file prototypes
 */
extern long clock_ticks(void);
#ifndef HAVESTRI
extern int stricmp(const char *, const char *);
extern int strnicmp(const char *, const char *, int);
#endif
#if !defined(_WIN32)
extern int memicmp(char *, char *, int);
extern unsigned short _rotl(unsigned short, short);
extern int ltoa(long, char *, int);
#endif
extern void ftimex(struct timebx *);
extern long stackavail(void);
extern int kbhit(void);

/*   unixscr.c -- C file prototypes */

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

/*
 *   video.c -- C file prototypes
 */
extern void put_prompt(void);
extern void load_dac(void);
extern void putcolor_a(int, int, int);
extern int  out_line(BYTE *, int);
extern int  getcolor(int, int);
extern void setvideomode(int, int, int, int);
extern void putstring(int,int,int,char far *);
extern BYTE *findfont (int);
 
#endif

