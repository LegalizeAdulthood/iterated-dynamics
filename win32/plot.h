#pragma once

#include <vector>

struct Plot
{
    int init(HINSTANCE instance, LPCSTR title);
    void terminate();
    void create_window(HWND parent);
    void write_pixel(int x, int y, int color);
    int read_pixel(int x, int y);
    void write_span(int y, int x, int lastx, const BYTE *pixels);
    void flush();
    void read_span(int y, int x, int lastx, BYTE *pixels);
    void set_line_mode(int mode);
    void draw_line(int x1, int y1, int x2, int y2, int color);
    int resize();
    int read_palette();
    int write_palette();
    void schedule_alarm(int secs);
    void clear();
    void redraw();
    void display_string(int x, int y, int fg, int bg, char const *text);
    void save_graphics();
    void restore_graphics();
    HWND get_window() const
    {
        return m_window;
    }
    int get_width() const
    {
        return m_width;
    }
    int get_height() const
    {
        return m_height;
    }

    // message handlers
    void on_paint(HWND window);

private:
    void set_dirty_region(int xmin, int ymin, int xmax, int ymax);
    void init_pixels();
    void create_backing_store();

    HINSTANCE m_instance{};
    std::string m_title;
    HWND m_parent{};
    HWND m_window{};
    HDC m_memory_dc{};
    HBITMAP m_rendering{};
    HBITMAP m_backup{};
    HFONT m_font{};
    bool m_dirty{};
    RECT m_dirty_region{};
    BITMAPINFO m_bmi{};                     // contains first clut entry too
    RGBQUAD m_bmi_colors[255]{};             // color look up table
    std::vector<BYTE> m_pixels;
    std::vector<BYTE> m_saved_pixels;
    size_t m_pixels_len{};
    size_t m_row_len{};
    int m_width{};
    int m_height{};
    unsigned char m_clut[256][3]{};
};
