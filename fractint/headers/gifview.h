#if !defined(GIF_VIEW_H)
#define GIF_VIEW_H

extern int get_byte();
extern int get_bytes(BYTE *, int);
extern int gifview();
extern int out_line_sound(BYTE *pixels, int line_length);
extern int out_line_potential(BYTE *pixels, int line_length);

#endif
