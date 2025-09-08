// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/main_state.h"

namespace id::ui
{

struct Coord
{
    int x;
    int y;
};

extern int                   g_box_x[];
extern int                   g_box_y[];
extern int                   g_box_values[];
extern int                   g_box_color;
extern int                   g_box_count;
extern double                g_zoom_box_height;
extern int                   g_zoom_box_rotation;
extern double                g_zoom_box_skew;
extern double                g_zoom_box_width;
extern double                g_zoom_box_x;
extern double                g_zoom_box_y;
extern bool                  g_zoom_enabled;

void draw_box(bool draw_it);
void move_box(double dx, double dy);
void resize_box(int steps);
void change_box(int dw, int dh);
void zoom_out();
void aspect_ratio_crop(float old_aspect, float new_aspect);
void init_pan_or_recalc(bool do_zoom_out);
void draw_lines(Coord fr, Coord to, int dx, int dy);
void add_box(Coord point);
void clear_box();
void display_box();
void clear_zoom_box();
MainState move_zoom_box(MainContext &context);
void reset_zoom_corners();

MainState request_zoom_in(MainContext &context);
MainState request_zoom_out(MainContext &context);
MainState skew_zoom_left(MainContext &context);
MainState skew_zoom_right(MainContext &context);
MainState decrease_zoom_aspect(MainContext &context);
MainState increase_zoom_aspect(MainContext &context);
MainState zoom_box_in(MainContext &context);
MainState zoom_box_out(MainContext &context);
MainState zoom_box_increase_rotation(MainContext &context);
MainState zoom_box_decrease_rotation(MainContext &context);
MainState zoom_box_increase_color(MainContext &context);
MainState zoom_box_decrease_color(MainContext &context);

} // namespace id::ui
