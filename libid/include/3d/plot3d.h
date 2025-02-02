// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class StereoImage
{
    NONE,
    RED,
    BLUE
};

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
extern StereoImage           g_which_image;
extern int                   g_x_shift1;
extern int                   g_xx_adjust1;
extern int                   g_y_shift1;
extern int                   g_yy_adjust1;

void draw_line(int x1, int y1, int x2, int y2, int color);
void plot_setup();
