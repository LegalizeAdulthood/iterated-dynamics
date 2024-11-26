// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// reads the current colormap into dacbox.
int read_video_palette();

// writes the current colormap from dacbox.
int write_video_palette();

// initializes the graphics window, colormap, etc.
void init_unix_window();

// Returns bitmap of an 8x8 font.
unsigned char *x_get_font();

// Forces all window events to be processed.
void x_sync();

// Checks if the window has been resized, and handles the resize.
int resize_window();

/* Used with schedulealarm.  X11 has a delayed write mode,
 * where the screen is updated only every few seconds.
 */
void redraw_screen();
