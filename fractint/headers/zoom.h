#if !defined(ZOOM_H)
#define ZOOM_H

extern void zoom_box_draw(int);
extern void zoom_box_move(double, double);
extern void zoom_box_resize(int);
extern void zoom_box_change_i(int, int);
extern void zoom_box_out();
extern void aspect_ratio_crop(float, float);
extern int init_pan_or_recalc(int);
extern void _fastcall draw_lines(Coordinate, Coordinate, int, int);
extern void _fastcall add_box(Coordinate);
extern void clear_box();
extern void display_box();

#endif
