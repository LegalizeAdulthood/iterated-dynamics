// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class StereoImage
{
    NONE,
    RED,
    BLUE
};

enum class GlassesType
{
    NONE = 0,
    ALTERNATING = 1, // something conflicts with ALTERNATE
    SUPERIMPOSE = 2,
    PHOTO = 3,
    STEREO_PAIR = 4,
};

inline bool alternating_or_superimpose(GlassesType value)
{
    return value == GlassesType::ALTERNATING || value == GlassesType::SUPERIMPOSE;
}

extern int                   g_adjust_3d_x;
extern int                   g_adjust_3d_y;
extern int                   g_blue_bright;
extern int                   g_blue_crop_left;
extern int                   g_blue_crop_right;
extern int                   g_eye_separation;
extern GlassesType           g_glasses_type;
extern int                   g_red_bright;
extern int                   g_red_crop_left;
extern int                   g_red_crop_right;
extern StereoImage           g_which_image;
extern int                   g_x_shift1;
extern int                   g_xx_adjust1;
extern int                   g_y_shift1;
extern int                   g_yy_adjust1;

inline bool glasses_alternating_or_superimpose()
{
    return alternating_or_superimpose(g_glasses_type);
}

void draw_line(int x1, int y1, int x2, int y2, int color);
void plot_setup();
