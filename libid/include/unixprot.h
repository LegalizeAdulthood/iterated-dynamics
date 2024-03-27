#pragma once

// This file contains prototypes for unix/linux specific functions.

struct EVOLUTION_INFO;
struct FRACTAL_INFO;
struct ORBITS_INFO;

void decode_evolver_info(EVOLUTION_INFO *, int);
void decode_fractal_info(FRACTAL_INFO *, int);
void decode_orbits_info(ORBITS_INFO *, int);

int stricmp(char const *, char const *);
int strnicmp(char const *, char const *, int);

// initializes curses text window and the signal handlers.
void UnixInit();

// initializes the graphics window, colormap, etc.
void initUnixWindow();

// cleans up X window and curses.
void UnixDone();

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

void putprompt();
void loaddac();
