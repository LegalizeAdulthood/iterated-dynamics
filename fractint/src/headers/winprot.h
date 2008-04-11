#ifndef WINPROT_H
#define WINPROT_H

// This file contains prototypes for win specific functions.

extern long calculate_mandelbrot_fp_p5_asm();
extern void calculate_mandelbrot_fp_p5_asm_start();
extern int get_color(int, int);
extern int out_line(BYTE const *pixels, int line_length);
extern void putcolor_a (int, int, int);

#endif
