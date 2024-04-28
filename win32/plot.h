#pragma once

#include <vector>

struct Plot
{
    HINSTANCE m_instance;
    char m_title[120];
    HWND m_parent;

    HWND m_window;
    HDC m_memory_dc;
    HBITMAP m_rendering;
    HBITMAP m_backup;
    HFONT m_font;

    bool m_dirty;
    RECT m_dirty_region;
    BITMAPINFO m_bmi;                     // contains first clut entry too
    RGBQUAD m_bmi_colors[255];             // color look up table

    std::vector<BYTE> m_pixels;
    std::vector<BYTE> m_saved_pixels;
    size_t m_pixels_len;
    size_t m_row_len;
    int m_width;
    int m_height;
    unsigned char m_clut[256][3];
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
