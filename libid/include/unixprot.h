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

// Checks if the window has been resized, and handles the resize.
int resizeWindow();

// Forces all window events to be processed.
void xsync();

/* Used with schedulealarm.  Xfractint has a delayed write mode,
 * where the screen is updated only every few seconds.
 */
void redrawscreen();

void loaddac();
