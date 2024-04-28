/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "port.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "id_data.h"
#include "os.h"
#include "rotate.h"
#include "spindac.h"
#include "video_mode.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>

#include <array>
#include <cassert>

#include "d_win32.h"
#include "frame.h"
#include "instance.h"
#include "ods.h"
#include "plot.h"
#include "WinText.h"

#include <array>

#define DRAW_INTERVAL 6
#define TIMER_ID 1

class GDIDriver : public Win32BaseDriver
{
public:
    GDIDriver() :
        Win32BaseDriver("gdi", "Windows GDI")
    {
    }
    ~GDIDriver() override = default;
    void terminate() override;
    void get_max_screen(int &xmax, int &ymax) override;
    bool init(int *argc, char **argv) override;
    bool resize() override;
    int read_palette() override;
    int write_palette() override;
    void schedule_alarm(int secs) override;
    void write_pixel(int x, int y, int color) override;
    int read_pixel(int x, int y) override;
    void write_span(int y, int x, int lastx, BYTE *pixels) override;
    void read_span(int y, int x, int lastx, BYTE *pixels) override;
    void set_line_mode(int mode) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void redraw() override;
    void window() override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    void set_clear() override;
    void set_video_mode(VIDEOINFO *mode) override;
    bool validate_mode(VIDEOINFO *mode) override;
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
static VIDEOINFO modes[] =
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
    *width = wintext.max_width;
    *height = wintext.max_height;
    if (g_video_table[g_adapter].xdots > *width)
    {
        *width = g_video_table[g_adapter].xdots;
        *center_x = false;
    }
    if (g_video_table[g_adapter].ydots > *height)
    {
        *height = g_video_table[g_adapter].ydots;
        *center_y = false;
    }
}

void GDIDriver::center_windows(bool center_x, bool center_y)
{
    POINT text_pos = { 0 }, plot_pos = { 0 };

    if (center_x)
    {
        plot_pos.x = (g_frame.m_width - plot.width)/2;
    }
    else
    {
        text_pos.x = (g_frame.m_width - wintext.max_width)/2;
    }

    if (center_y)
    {
        plot_pos.y = (g_frame.m_height - plot.height)/2;
    }
    else
    {
        text_pos.y = (g_frame.m_height - wintext.max_height)/2;
    }

    bool status = SetWindowPos(plot.window, nullptr,
        plot_pos.x, plot_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE) == TRUE;
    _ASSERTE(status);
    status = SetWindowPos(wintext.hWndCopy, nullptr,
        text_pos.x, text_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE) == TRUE;
    _ASSERTE(status);
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

void GDIDriver::terminate()
{
    ODS("gdi_terminate");

    plot_terminate(&plot);
    Win32BaseDriver::terminate();
}

void GDIDriver::get_max_screen(int &xmax, int &ymax)
{
    RECT desktop;
    GetClientRect(GetDesktopWindow(), &desktop);
    desktop.right -= GetSystemMetrics(SM_CXFRAME) * 2;
    desktop.bottom -= GetSystemMetrics(SM_CYFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION) - 1;

    xmax = desktop.right;
    ymax = desktop.bottom;
}

bool GDIDriver::init(int *argc, char **argv)
{
    if (!Win32BaseDriver::init(argc, argv))
    {
        return false;
    };

    plot_init(&plot, g_instance, "Plot");

    // add default list of video modes
    {
        int width, height;
        get_max_screen(width, height);

        for (int m = 0; m < std::size(modes); m++)
        {
            if ((modes[m].xdots <= width)
                && (modes[m].ydots <= height))
            {
                add_video_mode(this, &modes[m]);
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
    if ((g_video_table[g_adapter].xdots == plot.width)
        && (g_video_table[g_adapter].ydots == plot.height)
        && (width == g_frame.m_width)
        && (height == g_frame.m_height))
    {
        return false;
    }

    frame_resize(width, height);
    plot_resize(&plot);
    center_windows(center_graphics_x, center_graphics_y);
    return true;
}


// Reads the current video palette into g_dac_box.
int GDIDriver::read_palette()
{
    return plot_read_palette(&plot);
}

// Writes g_dac_box into the video palette.
int GDIDriver::write_palette()
{
    return plot_write_palette(&plot);
}

void GDIDriver::schedule_alarm(int secs)
{
    secs = (secs ? 1 : DRAW_INTERVAL)*1000;
    if (text_not_graphics)
    {
        wintext_schedule_alarm(&wintext, secs);
    }
    else
    {
        plot_schedule_alarm(&plot, secs);
    }
}

void GDIDriver::write_pixel(int x, int y, int color)
{
    plot_write_pixel(&plot, x, y, color);
}

int GDIDriver::read_pixel(int x, int y)
{
    return plot_read_pixel(&plot, x, y);
}

void GDIDriver::write_span(int y, int x, int lastx, BYTE *pixels)
{
    plot_write_span(&plot, x, y, lastx, pixels);
}

void GDIDriver::read_span(int y, int x, int lastx, BYTE *pixels)
{
    plot_read_span(&plot, y, x, lastx, pixels);
}

void GDIDriver::set_line_mode(int mode)
{
    plot_set_line_mode(&plot, mode);
}

void GDIDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    plot_draw_line(&plot, x1, y1, x2, y2, color);
}

void GDIDriver::redraw()
{
    ODS("gdi_redraw");
    if (text_not_graphics)
    {
        wintext_paintscreen(&wintext, 0, 80, 0, 25);
    }
    else
    {
        plot_redraw(&plot);
    }
    g_frame.pump_messages(false);
}

void GDIDriver::window()
{
    int width;
    int height;
    bool center_x = true, center_y = true;

    get_max_size(&width, &height, &center_x, &center_y);
    g_frame.window(width, height);
    wintext.hWndParent = g_frame.m_window;
    wintext_texton(&wintext);
    plot_window(&plot, g_frame.m_window);
    center_windows(center_x, center_y);
}

bool GDIDriver::is_text()
{
    return text_not_graphics;
}

void GDIDriver::set_for_text()
{
    text_not_graphics = true;
    show_hide_windows(wintext.hWndCopy, plot.window);
}

void GDIDriver::set_for_graphics()
{
    text_not_graphics = false;
    show_hide_windows(plot.window, wintext.hWndCopy);
    hide_text_cursor();
}

void GDIDriver::set_clear()
{
    if (text_not_graphics)
    {
        wintext_clear(&wintext);
    }
    else
    {
        plot_clear(&plot);
    }
}

extern void set_normal_dot();
extern void set_normal_line();
void GDIDriver::set_video_mode(VIDEOINFO *mode)
{
    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
    g_good_mode = true;
    if (g_dot_mode != 0)
    {
        g_and_color = g_colors-1;
        g_box_count = 0;
        g_dac_learn = true;
        g_dac_count = g_cycle_limit;
        g_got_real_dac = true;          // we are "VGA"

        read_palette();
    }

    resize();
    plot_clear(&plot);

    if (g_disk_flag)
    {
        enddisk();
    }

    set_normal_dot();
    set_normal_line();

    set_for_graphics();
    set_clear();
}

bool GDIDriver::validate_mode(VIDEOINFO *mode)
{
    int width, height;
    get_max_screen(width, height);

    // allow modes <= size of screen with 256 colors
    return (mode->xdots <= width)
        && (mode->ydots <= height)
        && (mode->colors == 256);
}

void GDIDriver::pause()
{
    if (wintext.hWndCopy)
    {
        ShowWindow(wintext.hWndCopy, SW_HIDE);
    }
    if (plot.window)
    {
        ShowWindow(plot.window, SW_HIDE);
    }
}

void GDIDriver::resume()
{
    if (!wintext.hWndCopy)
    {
        window();
    }

    ShowWindow(wintext.hWndCopy, SW_NORMAL);
    wintext_resume(&wintext);
}

void GDIDriver::display_string(int x, int y, int fg, int bg, char const *text)
{
    plot_display_string(&plot, x, y, fg, bg, text);
}

void GDIDriver::save_graphics()
{
    plot_save_graphics(&plot);
}

void GDIDriver::restore_graphics()
{
    plot_restore_graphics(&plot);
}

void GDIDriver::flush()
{
    plot_flush(&plot);
}

GDIDriver gdi_driver_info{};
Driver *gdi_driver = &gdi_driver_info;
