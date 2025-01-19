// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "Frame.h"
#include "instance.h"
#include "ods.h"
#include "Plot.h"
#include "Win32BaseDriver.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/video.h"
#include "ui/video_mode.h"

#include <config/driver_types.h>

#include <array> // for std::size

enum
{
    DRAW_INTERVAL = 6
};

class GDIDriver : public Win32BaseDriver
{
public:
    GDIDriver() :
        Win32BaseDriver("gdi", "Windows GDI")
    {
    }
    ~GDIDriver() override = default;
    void terminate() override;
    void get_max_screen(int &x_max, int &y_max) override;
    bool init(int *argc, char **argv) override;
    bool resize() override;
    int read_palette() override;
    int write_palette() override;
    void schedule_alarm(int secs) override;
    void write_pixel(int x, int y, int color) override;
    int read_pixel(int x, int y) override;
    void write_span(int y, int x, int last_x, Byte *pixels) override;
    void read_span(int y, int x, int last_x, Byte *pixels) override;
    void set_line_mode(int mode) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void redraw() override;
    void create_window() override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    void set_clear() override;
    void set_video_mode(VideoInfo *mode) override;
    bool validate_mode(VideoInfo *mode) override;
    void pause() override;
    void resume() override;
    void display_string(int x, int y, int fg, int bg, char const *text) override;
    void save_graphics() override;
    void restore_graphics() override;
    void flush() override;

private:
    void get_max_size(int *width, int *height, bool *center_x, bool *center_y) const;
    void center_windows(bool center_x, bool center_y);

    Plot plot{};
    bool text_not_graphics{true};
};

#define DRIVER_MODE(width_, height_) \
    { 0, width_, height_, 256, nullptr, "                        " }
static VideoInfo modes[] =
{
    DRIVER_MODE( 800,  600),
    DRIVER_MODE(1024,  768),
    DRIVER_MODE(1200,  900),
    DRIVER_MODE(1280,  960),
    DRIVER_MODE(1400, 1050),
    DRIVER_MODE(1500, 1125),
    DRIVER_MODE(1600, 1200)
};
#undef DRIVER_MODE

static void show_hide_windows(HWND show, HWND hide)
{
    ShowWindow(show, SW_NORMAL);
    ShowWindow(hide, SW_HIDE);
}

void GDIDriver::get_max_size(int *width, int *height, bool *center_x, bool *center_y) const
{
    *width = m_win_text.get_max_width();
    *height = m_win_text.get_max_height();
    if (g_video_table[g_adapter].x_dots > *width)
    {
        *width = g_video_table[g_adapter].x_dots;
        *center_x = false;
    }
    if (g_video_table[g_adapter].y_dots > *height)
    {
        *height = g_video_table[g_adapter].y_dots;
        *center_y = false;
    }
}

void GDIDriver::center_windows(bool center_x, bool center_y)
{
    POINT text_pos{};
    POINT plot_pos{};

    if (center_x)
    {
        plot_pos.x = (g_frame.get_width() - plot.get_width())/2;
    }
    else
    {
        text_pos.x = (g_frame.get_width() - m_win_text.get_max_width())/2;
    }

    if (center_y)
    {
        plot_pos.y = (g_frame.get_height() - plot.get_height())/2;
    }
    else
    {
        text_pos.y = (g_frame.get_height() - m_win_text.get_max_height())/2;
    }

    bool status = SetWindowPos(plot.get_window(), nullptr,
        plot_pos.x, plot_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE) == TRUE;
    _ASSERTE(status);
    status = SetWindowPos(m_win_text.get_window(), nullptr,
        text_pos.x, text_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE) == TRUE;
    _ASSERTE(status);
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

void GDIDriver::terminate()
{
    ODS("GDIDriver::terminate");

    plot.terminate();
    Win32BaseDriver::terminate();
}

void GDIDriver::get_max_screen(int &x_max, int &y_max)
{
    RECT desktop;
    GetClientRect(GetDesktopWindow(), &desktop);
    desktop.right -= GetSystemMetrics(SM_CXFRAME) * 2;
    desktop.bottom -= GetSystemMetrics(SM_CYFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) - 1;

    x_max = desktop.right;
    y_max = desktop.bottom;
}

bool GDIDriver::init(int *argc, char **argv)
{
    if (!Win32BaseDriver::init(argc, argv))
    {
        return false;
    };

    plot.init(g_instance, "Plot");

    // add default list of video modes
    {
        int width, height;
        get_max_screen(width, height);

        for (VideoInfo &mode : modes)
        {
            if (mode.x_dots <= width && mode.y_dots <= height)
            {
                add_video_mode(this, &mode);
            }
        }
    }

    return true;
}

/* resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
bool GDIDriver::resize()
{
    int width, height;
    bool center_graphics_x = true, center_graphics_y = true;

    get_max_size(&width, &height, &center_graphics_x, &center_graphics_y);
    if (g_video_table[g_adapter].x_dots == plot.get_width()     //
        && g_video_table[g_adapter].y_dots == plot.get_height() //
        && width == g_frame.get_width()                        //
        && height == g_frame.get_height())
    {
        return false;
    }

    g_frame.resize(width, height);
    plot.resize();
    center_windows(center_graphics_x, center_graphics_y);
    return true;
}

// Reads the current video palette into g_dac_box.
int GDIDriver::read_palette()
{
    return plot.read_palette();
}

// Writes g_dac_box into the video palette.
int GDIDriver::write_palette()
{
    return plot.write_palette();
}

void GDIDriver::schedule_alarm(int secs)
{
    secs = (secs ? 1 : DRAW_INTERVAL)*1000;
    if (text_not_graphics)
    {
        m_win_text.schedule_alarm(secs);
    }
    else
    {
        plot.schedule_alarm(secs);
    }
}

void GDIDriver::write_pixel(int x, int y, int color)
{
    plot.write_pixel(x, y, color);
}

int GDIDriver::read_pixel(int x, int y)
{
    return plot.read_pixel(x,y);
}

void GDIDriver::write_span(int y, int x, int last_x, Byte *pixels)
{
    plot.write_span(y, x, last_x, pixels);
}

void GDIDriver::read_span(int y, int x, int last_x, Byte *pixels)
{
    plot.read_span(y, x, last_x, pixels);
}

void GDIDriver::set_line_mode(int mode)
{
    plot.set_line_mode(mode);
}

void GDIDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    plot.draw_line(x1, y1, x2, y2, color);
}

void GDIDriver::redraw()
{
    ODS("GDIDriver::redraw");
    if (text_not_graphics)
    {
        m_win_text.paint_screen(0, 80, 0, 25);
    }
    else
    {
        plot.redraw();
    }
    g_frame.pump_messages(false);
}

void GDIDriver::create_window()
{
    int width;
    int height;
    bool center_x = true, center_y = true;

    get_max_size(&width, &height, &center_x, &center_y);
    g_frame.create_window(width, height);
    m_win_text.set_parent(g_frame.get_window());
    m_win_text.text_on();
    plot.create_window(g_frame.get_window());
    center_windows(center_x, center_y);
}

bool GDIDriver::is_text()
{
    return text_not_graphics;
}

void GDIDriver::set_for_text()
{
    text_not_graphics = true;
    show_hide_windows(m_win_text.get_window(), plot.get_window());
}

void GDIDriver::set_for_graphics()
{
    text_not_graphics = false;
    show_hide_windows(plot.get_window(), m_win_text.get_window());
    hide_text_cursor();
}

void GDIDriver::set_clear()
{
    if (text_not_graphics)
    {
        m_win_text.clear();
    }
    else
    {
        plot.clear();
    }
}

void GDIDriver::set_video_mode(VideoInfo *mode)
{
    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
    g_good_mode = true;
    g_and_color = g_colors - 1;
    g_box_count = 0;
    g_dac_learn = true;
    g_dac_count = g_cycle_limit;
    g_got_real_dac = true; // we are "VGA"
    read_palette();

    resize();
    plot.clear();
    if (g_disk_flag)
    {
        end_disk();
    }
    set_normal_dot();
    set_normal_span();
    set_for_graphics();
    set_clear();
}

bool GDIDriver::validate_mode(VideoInfo *mode)
{
    int width, height;
    get_max_screen(width, height);

    // allow modes <= size of screen with 256 colors
    return (mode->x_dots <= width)
        && (mode->y_dots <= height)
        && (mode->colors == 256);
}

void GDIDriver::pause()
{
    if (m_win_text.get_window())
    {
        ShowWindow(m_win_text.get_window(), SW_HIDE);
    }
    if (plot.get_window())
    {
        ShowWindow(plot.get_window(), SW_HIDE);
    }
}

void GDIDriver::resume()
{
    if (!m_win_text.get_window())
    {
        create_window();
    }

    ShowWindow(m_win_text.get_window(), SW_NORMAL);
    m_win_text.resume();
}

void GDIDriver::display_string(int x, int y, int fg, int bg, char const *text)
{
    plot.display_string(x, y, fg, bg, text);
}

void GDIDriver::save_graphics()
{
    plot.save_graphics();
}

void GDIDriver::restore_graphics()
{
    plot.restore_graphics();
}

void GDIDriver::flush()
{
    plot.flush();
}

static GDIDriver s_gdi_driver{};
Driver *g_gdi_driver = &s_gdi_driver;
