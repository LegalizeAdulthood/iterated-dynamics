#pragma once

extern unsigned int          g_height;
extern unsigned              g_num_colors;

int get_byte();
int get_bytes(BYTE *, int);
int gifview();
int pot_line(BYTE *pixels, int linelen);
int sound_line(BYTE *pixels, int linelen);
