#pragma once
#if !defined(PLOT3D_H)
#define PLOT3D_H

extern void draw_line(int, int, int, int, int);
extern void plot3dsuperimpose16(int, int, int);
extern void plot3dsuperimpose256(int, int, int);
extern void plotIFS3dsuperimpose256(int, int, int);
extern void plot3dalternate(int, int, int);
extern void plot_setup();

#endif
