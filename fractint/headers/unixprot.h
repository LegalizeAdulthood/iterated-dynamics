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
extern void initasmvars(void);
extern long multiply(long, long, int);
extern long divide(long, long, int);
extern int  keypressed(void);
extern int  waitkeypressed(int);
extern int  getakeynohelp(void);
extern int  getakey(void);
extern int  getkeynowait(void);
extern int  getkeyint(int);
extern void buzzer(int);
extern void delay(int);
extern void tone(int, int);
extern void snd(int);
extern void nosnd(void);
extern long readticker(void);
extern void * farmemalloc(long);
extern void farmemfree(void *);
extern void erasesegment(int, int);
extern int  farread(int, void *, unsigned int);
extern int  farwrite(int, void *, unsigned int);
extern long normalize(char *);
extern int  far_strlen (char *);
extern void far_strcpy (char *, char *);
extern int  far_strcmp (char *, char *);
extern int  far_strnicmp (char *, char *, int);
extern void far_strcat (char *, char *);
extern void far_memset(VOIDFARPTR, int, unsigned int);
extern void far_memcpy(VOIDFARPTR, VOIDFARPTR, int);
extern int  far_memcmp(VOIDFARPTR, VOIDFARPTR, int);
extern int  far_memicmp(VOIDFARPTR, VOIDFARPTR, int);
extern void decode_evolver_info(struct evolution_info *, int);
extern void fix_ranges(int *, int, int);
extern void decode_fractal_info(struct fractal_info *, int);
extern void decode_orbits_info(struct orbits_info *, int);

/*
 *   unix.c -- C file prototypes
 */
extern long clock_ticks(void);
#ifndef HAVESTRI
extern int stricmp(char *, char *);
extern int strnicmp(char *, char *, int);
#endif
extern int memicmp(char *, char *, int);
extern unsigned short _rotl(unsigned short, short);
extern int ltoa(long, char *, int);
extern void ftimex(struct timebx *);
extern long stackavail(void);
extern int kbhit(void);
extern void mute(void);
extern int soundon(int);
extern void soundoff(void);
extern int get_sound_params(void);

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
extern void setnullvideo (void);
extern void movecursor(int, int);
extern int  keycursor(int, int);
extern void setattr(int, int, int, int);
extern void home (void);
extern void scrollup(int, int);
extern void spindac(int, int);
extern void setclear (void);
extern BYTE *findfont (int);
extern void adapter_detect(void);
extern void find_special_colors(void);
extern char get_a_char (void);
extern void put_a_char (int);
extern void get_line(int, int, int, BYTE *);
extern void put_line(int, int, int, BYTE *);
extern int  out_line(BYTE *, int);
extern void setvideotext (void);
extern void loaddac(void);
extern void putcolor_a(int, int, int);
extern int  out_line(BYTE *, int);
extern int  getcolor(int, int);
extern void setvideomode(int, int, int, int);
extern void putstring(int,int,int,char far *);

#endif

