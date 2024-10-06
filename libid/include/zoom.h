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

void draw_box(bool draw_it);
void move_box(double dx, double dy);
void resize_box(int steps);
void change_box(int dw, int dd);
void zoom_out();
void aspect_ratio_crop(float old_aspect, float new_aspect);
int init_pan_or_recalc(bool do_zoom_out);
void draw_lines(coords fr, coords to, int dx, int dy);
void add_box(coords point);
void clear_box();
void display_box();
void clear_zoom_box();
void move_zoom_box(int key_num);
void reset_zoom_corners();
