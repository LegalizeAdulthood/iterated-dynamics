// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11BaseDriver.h"

#include "engine/calcfrac.h"
#include "engine/spindac.h"
#include "engine/VideoInfo.h"
#include "geometry/plot3d.h"
#include "ui/diskvid.h"
#include "ui/video.h"
#include "ui/zoom.h"

#include <cassert>

using namespace id::engine;
using namespace id::geometry;
using namespace id::ui;

namespace id::engine
{
extern int g_and_color;
}

namespace id::misc
{

namespace
{

class X11DiskDriver : public X11BaseDriver
{
public:
    X11DiskDriver() :
        X11BaseDriver("disk", "Linux Disk")
    {
    }

    bool init(int *argc, char **argv) override;
    bool resize() override;
    void read_palette() override;
    void write_palette() override;
    void write_pixel(int x, int y, int color) override;
    int read_pixel(int x, int y) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void draw_xor_line(int x1, int y1, int x2, int y2) override;
    void clear_xor_lines() override;
    void create_window() override;
    void set_video_mode(const VideoInfo &mode) override;
    void set_clear() override;
    void display_string(int x, int y, int fg, int bg, const char *text) override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    bool is_disk() const override;
    bool validate_mode(const VideoInfo &mode) override;
    void save_graphics() override;
    void restore_graphics() override;
    void get_max_screen(int &width, int &height) override;
    void flush() override;

private:
    int m_width{};
    int m_height{};
    unsigned char m_clut[256][3]{};
};

#define DRIVER_MODE(width_, height_, comment_) {0, width_, height_, 256, nullptr, comment_}
VideoInfo s_modes[]{
    // clang-format off
    DRIVER_MODE(640, 480,   "VGA                     "),
    DRIVER_MODE(800, 600,   "SVGA                    "),
    DRIVER_MODE(1024, 768,  "XGA                     "),
    DRIVER_MODE(1280, 768,  "WXGA                    "),
    DRIVER_MODE(1280, 800,  "WXGA                    "),
    DRIVER_MODE(1280, 960,  "                        "),
    DRIVER_MODE(1280, 1024, "SXGA                    "),
    DRIVER_MODE(1400, 1050, "SXGA+                   "),
    DRIVER_MODE(1920, 1080, "HD 1080                 "),
    DRIVER_MODE(2048, 1080, "2K                      "),
    DRIVER_MODE(1500, 1125, "                        "),
    DRIVER_MODE(1600, 1200, "UXGA                    "),
    DRIVER_MODE(1920, 1200, "WUXGA                   "),
    DRIVER_MODE(2048, 1536, "QXGA                    "),
    DRIVER_MODE(2560, 1600, "WQXGA                   "),
    DRIVER_MODE(2560, 2048, "QSXGA                   "),
    // clang-format on
};
#undef DRIVER_MODE

void init_dac_box()
{
    for (int i = 0; i < 256; ++i)
    {
        g_dac_box[i][0] = (i >> 5) * 8 + 7;
        g_dac_box[i][1] = ((i + 16 & 28) >> 2) * 8 + 7;
        g_dac_box[i][2] = (i + 2 & 3) * 16 + 15;
    }
    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    g_dac_box[1][0] = 255;
    g_dac_box[1][1] = 255;
    g_dac_box[1][2] = 255;
    g_dac_box[2][0] = 190;
    g_dac_box[2][1] = 255;
    g_dac_box[2][2] = 255;
}

X11DiskDriver s_disk_driver;

} // namespace

bool X11DiskDriver::init(int *argc, char **argv)
{
    if (!X11BaseDriver::init(argc, argv))
    {
        return false;
    }

    init_dac_box();
    write_palette();

    for (VideoInfo &mode : s_modes)
    {
        add_video_mode(this, &mode);
    }
    return true;
}

bool X11DiskDriver::resize()
{
    m_frame.resize(m_text.width(), m_text.height());
    if (g_video_table[g_adapter].x_dots == m_width && g_video_table[g_adapter].y_dots == m_height)
    {
        return false;
    }

    if (g_disk_flag)
    {
        end_disk();
    }
    start_disk();
    m_width = g_video_table[g_adapter].x_dots;
    m_height = g_video_table[g_adapter].y_dots;
    return true;
}

void X11DiskDriver::read_palette()
{
    if (!g_got_real_dac)
    {
        return;
    }
    for (int i = 0; i < 256; ++i)
    {
        g_dac_box[i][0] = m_clut[i][0];
        g_dac_box[i][1] = m_clut[i][1];
        g_dac_box[i][2] = m_clut[i][2];
    }
}

void X11DiskDriver::write_palette()
{
    for (int i = 0; i < 256; ++i)
    {
        m_clut[i][0] = g_dac_box[i][0];
        m_clut[i][1] = g_dac_box[i][1];
        m_clut[i][2] = g_dac_box[i][2];
    }
}

void X11DiskDriver::write_pixel(const int x, const int y, const int color)
{
    put_color_a(x, y, color);
}

int X11DiskDriver::read_pixel(const int x, const int y)
{
    return get_color(x, y);
}

void X11DiskDriver::draw_line(const int x1, const int y1, const int x2, const int y2, const int color)
{
    geometry::draw_line(x1, y1, x2, y2, color);
}

void X11DiskDriver::draw_xor_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/)
{
}

void X11DiskDriver::clear_xor_lines()
{
}

void X11DiskDriver::create_window()
{
    m_frame.create_window(m_text.width(), m_text.height());
    if (m_text.create(m_frame.window(), 0, 0))
    {
        m_frame.add_input_window(m_text.window());
    }
    m_text_not_graphics = true;
    set_for_text();
}

void X11DiskDriver::set_video_mode(const VideoInfo &mode)
{
    assert(g_video_table[g_adapter].x_dots == mode.x_dots);
    assert(g_video_table[g_adapter].y_dots == mode.y_dots);
    assert(g_video_table[g_adapter].colors == mode.colors);
    assert(g_video_table[g_adapter].driver == this);

    g_is_true_color = false;
    g_vesa_x_res = 0;
    g_vesa_y_res = 0;
    g_good_mode = true;
    g_and_color = g_colors - 1;
    g_box_count = 0;
    g_dac_count = g_cycle_limit;
    g_got_real_dac = true;
    read_palette();

    resize();
    set_disk_dot();
    set_normal_span();
}

void X11DiskDriver::set_clear()
{
    m_text.clear();
}

void X11DiskDriver::display_string(int /*x*/, int /*y*/, int /*fg*/, int /*bg*/, const char * /*text*/)
{
}

bool X11DiskDriver::is_text()
{
    return true;
}

void X11DiskDriver::set_for_text()
{
    m_text_not_graphics = true;
    m_text.show();
}

void X11DiskDriver::set_for_graphics()
{
    hide_text_cursor();
}

bool X11DiskDriver::is_disk() const
{
    return true;
}

bool X11DiskDriver::validate_mode(const VideoInfo &mode)
{
    return mode.colors == 256;
}

void X11DiskDriver::save_graphics()
{
}

void X11DiskDriver::restore_graphics()
{
}

void X11DiskDriver::get_max_screen(int &width, int &height)
{
    width = -1;
    height = -1;
}

void X11DiskDriver::flush()
{
    m_frame.pump_messages(false);
}

Driver *get_disk_driver()
{
    return &s_disk_driver;
}

} // namespace id::misc
