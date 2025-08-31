// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <wx/wx.h>

#include <array>
#include <string>
#include <vector>

namespace ui
{

using Colormap = std::array<std::array<Byte, 3>, 256>;

class Plot : public wxControl
{
public:
    Plot();
    Plot(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition,
        const wxSize &size = wxDefaultSize, long style = 0);

    ~Plot() override = default;

    void on_paint(wxPaintEvent &event);

    void write_pixel(int x, int y, int color);
    int read_pixel(int x, int y);
    void write_span(int y, int x, int last_x, const Byte *pixels);
    void flush();
    void read_span(int y, int x, int last_x, Byte *pixels);
    void set_line_mode(int mode);
    void draw_line(int x1, int y1, int x2, int y2, int color);
    Colormap get_colormap() const;
    void set_colormap(const Colormap &value);
    void schedule_alarm(int secs);
    void clear();
    void redraw();
    void display_string(int x, int y, int fg, int bg, const char *text);
    void save_graphics();
    void restore_graphics();

protected:
    // Override size-related methods to enforce fixed size
    void DoSetSize(int x, int y, int width, int height, int sizeFlags) override;
    wxSize DoGetBestSize() const override;
    wxSize GetMinSize() const override;
    wxSize GetMaxSize() const override;

private:
    void init();
    void set_dirty_region(const wxRect &rect);
    void init_pixels();
    void create_backing_store();

    std::string m_title;
    wxImage m_rendering;
    wxImage m_backup;
    wxFont m_font;
    bool m_dirty{};
    wxRect m_dirty_region{};
    std::vector<Byte> m_pixels;
    std::vector<Byte> m_saved_pixels;
    size_t m_pixels_len{};
    size_t m_row_len{};
    int m_width{};
    int m_height{};
    Colormap m_clut{};

    wxDECLARE_DYNAMIC_CLASS(Plot);
};

} // namespace ui
