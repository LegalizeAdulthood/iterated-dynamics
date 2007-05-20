#ifndef WINPROT_H
#define WINPROT_H

/* This file contains prototypes for win specific functions. */

extern long cdecl calculate_mandelbrot_fp_p5_asm();
extern void cdecl calculate_mandelbrot_fp_p5_asm_start();
extern int getcolor(int, int);
extern int out_line(BYTE *, int);
extern void putcolor_a (int, int, int);

#endif
