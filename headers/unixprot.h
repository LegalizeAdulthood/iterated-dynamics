#ifndef UNIXPROT_H
#define UNIXPROT_H
// This file contains prototypes for unix/linux specific functions.
void Cursor_StartMouseTracking();
void Cursor_EndMouseTracking();
/*
 *   general.c -- C file prototypes
 */
extern int getakey();
extern int waitkeypressed(int);
extern void fix_ranges(int *, int, int);
extern void decode_evolver_info(struct evolution_info *, int);
extern void decode_fractal_info(struct fractal_info *, int);
extern void decode_orbits_info(struct orbits_info *, int);
/*
 *   unix.c -- C file prototypes
 */
extern long clock_ticks();
#ifndef HAVESTRI
extern int stricmp(const char *, const char *);
extern int strnicmp(const char *, const char *, int);
#endif
extern int ltoa(long, char *, int);
extern void ftimex(struct timebx *);
// unixscr.c -- C file prototypes
void UnixInit();
// initializes curses text window and the signal handlers.
void initUnixWindow();
// initializes the graphics window, colormap, etc.
void UnixDone();
// cleans up X window and curses.
int startvideo();
// clears the graphics window
int endvideo();
// just a stub.
int readvideo(int x, int y);
// reads a pixel from the screen.
void readvideoline(int y, int x, int lastx, BYTE *pixels);
// reads a line of pixels from the screen.
void writevideo(int x, int y, int color);
// writes a pixel to the screen.
void writevideoline(int y, int x, int lastx, BYTE *pixels);
// writes a line of pixels to the screen.
int readvideopalette();
// reads the current colormap into dacbox.
int writevideopalette();
// writes the current colormap from dacbox.
int resizeWindow();
// Checks if the window has been resized, and handles the resize.
int xgetkey(int block);
// Checks if a key has been pressed.
unsigned char * xgetfont();
// Returns bitmap of an 8x8 font.
void drawline(int x1, int y1, int x2, int y2);
// Draws a line from (x1,y1) to (x2,y2).
void setlinemode(int mode);
// Sets line mode to draw or xor.
void shell_to_dos();
// Calls a Unix subshell.
void xsync();
// Forces all window events to be processed.
void redrawscreen();
/* Used with schedulealarm.  Xfractint has a delayed write mode,
 * where the screen is updated only every few seconds.
 */
void schedulealarm(int soon);
// Schedules the next delayed update.
/*
 *   video.c -- C file prototypes
 */
extern void putprompt();
extern void loaddac();
#endif