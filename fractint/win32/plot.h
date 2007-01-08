#if !defined(PLOT_H)
#define PLOT_H

typedef struct tagPlot Plot;
struct tagPlot
{
	HINSTANCE instance;
	char title[120];
	HWND parent;

	HWND window;

	BOOL dirty;
	RECT dirty_region;

	BYTE *pixels;
	size_t pixels_len;
	int width;
	int height;
	unsigned char clut[256][3];				/* color look up table */
};

extern int plot_init(Plot *p, HINSTANCE instance, LPCSTR title);
extern void plot_terminate(Plot *p);
extern void plot_window(Plot *p, HWND parent);
extern void plot_write_pixel(Plot *p, int x, int y, int color);
extern int plot_read_pixel(Plot *p, int x, int y);
extern void plot_write_span(Plot *p, int x, int y, int lastx, const BYTE *pixels);
extern void plot_read_span(Plot *p, int x, int y, int lastx, BYTE *pixels);
extern void plot_set_line_mode(Plot *p, int mode);
extern void plot_draw_line(Plot *p, int x1, int y1, int x2, int y2, int color);
extern int plot_resize(Plot *p);
extern int plot_read_palette(Plot *p);
extern int plot_write_palette(Plot *p);
extern void plot_flush(Plot *p);

#endif
