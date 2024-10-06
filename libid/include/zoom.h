// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

struct coords
{
    int x;
    int y;
};

extern int                   g_box_color;
extern int                   g_box_values[];
extern int                   g_box_x[];
extern int                   g_box_y[];

void drawbox(bool draw_it);
void moveboxf(double dx, double dy);
void resizebox(int steps);
void chgboxi(int dw, int dd);
void zoomout();
void aspectratio_crop(float oldaspect, float newaspect);
int init_pan_or_recalc(bool do_zoomout);
void drawlines(coords fr, coords to, int dx, int dy);
void addbox(coords point);
void clearbox();
void dispbox();
void clear_zoombox();
void move_zoombox(int keynum);
void reset_zoom_corners();
