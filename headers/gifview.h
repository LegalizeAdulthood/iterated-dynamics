#pragma once
#if !defined(GIFVIEW_H)
#define GIFVIEW_H

extern unsigned int          g_height;
extern unsigned              g_num_colors;

extern int get_byte();
extern int get_bytes(BYTE *, int);
extern int gifview();

#endif
