#pragma once
#if !defined(ZOOM_H)
#define ZOOM_H

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

extern void drawbox(bool draw_it);
extern void moveboxf(double, double);
extern void resizebox(int);
extern void chgboxi(int, int);
extern void zoomout();
extern void aspectratio_crop(float, float);
extern int init_pan_or_recalc(bool);
extern void drawlines(coords, coords, int, int);
extern void addbox(coords);
extern void clearbox();
extern void dispbox();

#endif
