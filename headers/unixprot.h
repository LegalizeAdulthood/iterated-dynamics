#ifndef UNIXPROT_H
#define UNIXPROT_H
// This file contains prototypes for unix/linux specific functions.

struct EVOLUTION_INFO;
struct FRACTAL_INFO;
struct ORBITS_INFO;

extern int getakey();
extern int waitkeypressed(int);
extern void fix_ranges(int *, int, int);
extern void decode_evolver_info(EVOLUTION_INFO *, int);
extern void decode_fractal_info(FRACTAL_INFO *, int);
extern void decode_orbits_info(ORBITS_INFO *, int);

#ifndef HAVESTRI
extern int stricmp(char const *, char const *);
extern int strnicmp(char const *, char const *, int);
#endif
extern int ltoa(long, char *, int);

// initializes curses text window and the signal handlers.
void UnixInit();

// initializes the graphics window, colormap, etc.
void initUnixWindow();

// cleans up X window and curses.
void UnixDone();

// clears the graphics window
int startvideo();

// just a stub.
int endvideo();

// reads a pixel from the screen.
int readvideo(int x, int y);

// reads a line of pixels from the screen.
void readvideoline(int y, int x, int lastx, BYTE *pixels);

// writes a pixel to the screen.
void writevideo(int x, int y, int color);

// writes a line of pixels to the screen.
void writevideoline(int y, int x, int lastx, BYTE const *pixels);

// reads the current colormap into dacbox.
int readvideopalette();

// writes the current colormap from dacbox.
int writevideopalette();

// Checks if the window has been resized, and handles the resize.
int resizeWindow();

// Checks if a key has been pressed.
int xgetkey(int block);

// Returns bitmap of an 8x8 font.
unsigned char *xgetfont();

// Draws a line from (x1,y1) to (x2,y2).
void drawline(int x1, int y1, int x2, int y2);

// Sets line mode to draw or xor.
void setlinemode(int mode);

// Calls a Unix subshell.
void shell_to_dos();

// Forces all window events to be processed.
void xsync();

/* Used with schedulealarm.  Xfractint has a delayed write mode,
 * where the screen is updated only every few seconds.
 */
void redrawscreen();

// Schedules the next delayed update.
void schedulealarm(int soon);

extern void putprompt();
extern void loaddac();
#endif
