#if !defined(ZOOM_H)
#define ZOOM_H

extern void zoom_box_draw(bool drawit);
extern void zoom_box_move(double, double);
extern void zoom_box_resize(int);
extern void zoom_box_change_i(int, int);
extern void zoom_box_out();
extern void aspect_ratio_crop(float, float);
extern void init_pan_or_recalc(bool do_zoomout);
extern void draw_lines(Coordinate, Coordinate, int, int);
extern void add_box(Coordinate);

#endif
