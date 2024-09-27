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
extern bool                  g_video_scroll;

void drawbox(bool draw_it);
void moveboxf(double, double);
void resizebox(int);
void chgboxi(int, int);
void zoomout();
void aspectratio_crop(float, float);
int init_pan_or_recalc(bool);
void drawlines(coords, coords, int, int);
void addbox(coords);
void clearbox();
void dispbox();
void scroll_relative(int bycol, int byrow);
