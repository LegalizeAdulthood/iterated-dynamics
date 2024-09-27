// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// reads the current colormap into dacbox.
int readvideopalette();

// writes the current colormap from dacbox.
int writevideopalette();

// initializes the graphics window, colormap, etc.
void initUnixWindow();

// Returns bitmap of an 8x8 font.
unsigned char *xgetfont();

// Forces all window events to be processed.
void xsync();

// Checks if the window has been resized, and handles the resize.
int resizeWindow();

/* Used with schedulealarm.  X11 has a delayed write mode,
 * where the screen is updated only every few seconds.
 */
void redrawscreen();
