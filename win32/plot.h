#pragma once

#include <vector>

struct Plot
{
    HINSTANCE instance;
    char title[120];
    HWND parent;

    HWND window;
    HDC memory_dc;
    HBITMAP rendering;
    HBITMAP backup;
    HFONT font;

    bool dirty;
    RECT dirty_region;
    BITMAPINFO bmi;                     // contains first clut entry too
    RGBQUAD bmiColors[255];             // color look up table

    std::vector<BYTE> pixels;
    std::vector<BYTE> saved_pixels;
    size_t pixels_len;
    size_t row_len;
    int width;
    int height;
    unsigned char clut[256][3];
};

int plot_init(Plot *p, HINSTANCE instance, LPCSTR title);
void plot_terminate(Plot *p);
void plot_window(Plot *p, HWND parent);
void plot_write_pixel(Plot *p, int x, int y, int color);
int plot_read_pixel(Plot *p, int x, int y);
void plot_write_span(Plot *p, int x, int y, int lastx, const BYTE *pixels);
void plot_read_span(Plot *p, int x, int y, int lastx, BYTE *pixels);
void plot_set_line_mode(Plot *p, int mode);
void plot_draw_line(Plot *p, int x1, int y1, int x2, int y2, int color);
int plot_resize(Plot *p);
int plot_read_palette(Plot *p);
int plot_write_palette(Plot *p);
void plot_flush(Plot *p);
void plot_schedule_alarm(Plot *p, int secs);
void plot_clear(Plot *p);
void plot_redraw(Plot *p);
void plot_display_string(Plot *p, int x, int y, int fg, int bg, char const *text);
void plot_save_graphics(Plot *p);
void plot_restore_graphics(Plot *p);
