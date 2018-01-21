#pragma once
#if !defined(PLOT3D_H)
#define PLOT3D_H

extern int                   g_adjust_3d_x;
extern int                   g_adjust_3d_y;
extern int                   g_blue_bright;
extern int                   g_blue_crop_left;
extern int                   g_blue_crop_right;
extern int                   g_eye_separation;
extern int                   g_glasses_type;
extern int                   g_red_bright;
extern int                   g_red_crop_left;
extern int                   g_red_crop_right;
extern stereo_images         g_which_image;
extern int                   g_x_shift1;
extern int                   g_xx_adjust1;
extern int                   g_y_shift1;
extern int                   g_yy_adjust1;

extern void draw_line(int, int, int, int, int);
extern void plot3dsuperimpose16(int, int, int);
extern void plot3dsuperimpose256(int, int, int);
extern void plotIFS3dsuperimpose256(int, int, int);
extern void plot3dalternate(int, int, int);
extern void plot_setup();

#endif
