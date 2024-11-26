// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once
#include "main_state.h"

struct Coord
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
void draw_lines(Coord fr, Coord to, int dx, int dy);
void add_box(Coord point);
void clear_box();
void display_box();
void clear_zoom_box();
main_state move_zoom_box(int &key_num, bool &from_mandel, bool &kbd_more, bool &stacked);
void reset_zoom_corners();

main_state request_zoom_in(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state request_zoom_out(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state skew_zoom_left(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state skew_zoom_right(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state decrease_zoom_aspect(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state increase_zoom_aspect(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state zoom_box_in(int &key, bool &from_mande, bool &kbd_more, bool &stacked);
main_state zoom_box_out(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state zoom_box_increase_rotation(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state zoom_box_decrease_rotation(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state zoom_box_increase_color(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state zoom_box_decrease_color(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
