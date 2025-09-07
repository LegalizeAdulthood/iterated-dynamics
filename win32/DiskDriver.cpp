// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for id.
 */
#include "Frame.h"
#include "ods.h"
#include "Win32BaseDriver.h"
#include "WinText.h"

#include <config/driver_types.h>

#include "3d/plot3d.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "ui/diskvid.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/video.h"
#include "ui/video_mode.h"
#include "ui/zoom.h"

enum
{
    DRAW_INTERVAL = 6,
    TIMER_ID = 1
};

class DiskDriver : public Win32BaseDriver
{
public:
    DiskDriver() :
        Win32BaseDriver("disk", "Windows Disk")
    {
    }

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
    void set_video_mode(VideoInfo *mode) override;
    void set_clear() override;
    void display_string(int x, int y, int fg, int bg, const char *text) override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    bool is_disk() const override;
    bool validate_mode(VideoInfo *mode) override;
    void pause() override;
    void resume() override;
    void save_graphics() override;
    void restore_graphics() override;
    void get_max_screen(int &x_max, int &y_max) override;
    void flush() override;

private:
    int m_width{};
    int m_height{};
    unsigned char m_clut[256][3]{};
};

#define DRIVER_MODE(width_, height_ ) \
    { 0, width_, height_, 256, nullptr, "                        " }
static VideoInfo s_modes[] =
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

/*----------------------------------------------------------------------
*
* initdacbox --
*
* Put something nice in the dac.
*
* The conditions are:
*   Colors 1 and 2 should be bright so ifs fractals show up.
*   Color 15 should be bright for lsystem.
*   Color 1 should be bright for bifurcation.
*   Colors 1, 2, 3 should be distinct for periodicity.
*   The color map should look good for mandelbrot.
*
* Results:
*   None.
*
* Side effects:
*   Loads the dac.
*
*----------------------------------------------------------------------
*/
static void init_dac_box()
{
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = (i >> 5)*8+7;
        g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
        g_dac_box[i][2] = (((i+2) & 3))*16+15;
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

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

bool DiskDriver::init(int *argc, char **argv)
{
    const bool base_init = Win32BaseDriver::init(argc, argv);
    if (!base_init)
    {
        return false;
    }

    init_dac_box();

    // add default list of video modes
    for (VideoInfo &mode : s_modes)
    {
        add_video_mode(this, &mode);
    }

    return true;
}

/* resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
bool DiskDriver::resize()
{
    g_frame.resize(m_win_text.get_max_width(), m_win_text.get_max_height());
    if ((g_video_table[g_adapter].x_dots == m_width)
        && (g_video_table[g_adapter].y_dots == m_height))
    {
        return false;
    }

    if (g_disk_flag)
    {
        end_disk();
    }
    start_disk();

    return true;
}

/*----------------------------------------------------------------------
* read_palette
*
*   Reads the current video palette into g_dac_box.
*
*
* Results:
*   None.
*
* Side effects:
*   Fills in g_dac_box.
*
*----------------------------------------------------------------------
*/
int DiskDriver::read_palette()
{
    ODS("DiskDriver::read_palette");
    if (!g_got_real_dac)
    {
        return -1;
    }
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = m_clut[i][0];
        g_dac_box[i][1] = m_clut[i][1];
        g_dac_box[i][2] = m_clut[i][2];
    }
    return 0;
}

/*
*----------------------------------------------------------------------
*
* write_palette --
*   Writes g_dac_box into the video palette.
*
*
* Results:
*   None.
*
* Side effects:
*   Changes the displayed colors.
*
*----------------------------------------------------------------------
*/
int DiskDriver::write_palette()
{
    ODS("DiskDriver::write_palette");
    for (int i = 0; i < 256; i++)
    {
        m_clut[i][0] = g_dac_box[i][0];
        m_clut[i][1] = g_dac_box[i][1];
        m_clut[i][2] = g_dac_box[i][2];
    }

    return 0;
}

/*
*----------------------------------------------------------------------
*
* schedule_alarm --
*
*   Start the refresh alarm
*
* Results:
*   None.
*
* Side effects:
*   Starts the alarm.
*
*----------------------------------------------------------------------
*/
void DiskDriver::schedule_alarm(int secs)
{
    m_win_text.schedule_alarm((secs ? 1 : DRAW_INTERVAL) * 1000);
}

/*
*----------------------------------------------------------------------
*
* write_pixel --
*
*   Write a point to the screen
*
* Results:
*   None.
*
* Side effects:
*   Draws point.
*
*----------------------------------------------------------------------
*/
void DiskDriver::write_pixel(int x, int y, int color)
{
    put_color_a(x, y, color);
}

/*
*----------------------------------------------------------------------
*
* read_pixel --
*
*   Read a point from the screen
*
* Results:
*   Value of point.
*
* Side effects:
*   None.
*
*----------------------------------------------------------------------
*/
int DiskDriver::read_pixel(int x, int y)
{
    return get_color(x, y);
}

/*
*----------------------------------------------------------------------
*
* write_span --
*
*   Write a line of pixels to the screen.
*
* Results:
*   None.
*
* Side effects:
*   Draws pixels.
*
*----------------------------------------------------------------------
*/
void DiskDriver::write_span(int y, int x, int last_x, Byte *pixels)
{
    int width = last_x-x+1;
    ODS3("DiskDriver::write_span (%d,%d,%d)", y, x, lastx);

    for (int i = 0; i < width; i++)
    {
        write_pixel(x+i, y, pixels[i]);
    }
}

/*
*----------------------------------------------------------------------
*
* read_span --
*
*   Reads a line of pixels from the screen.
*
* Results:
*   None.
*
* Side effects:
*   Gets pixels
*
*----------------------------------------------------------------------
*/
void DiskDriver::read_span(int y, int x, int last_x, Byte *pixels)
{
    ODS3("DiskDriver::read_span (%d,%d,%d)", y, x, lastx);
    int width = last_x-x+1;
    for (int i = 0; i < width; i++)
    {
        pixels[i] = read_pixel(x+i, y);
    }
}

void DiskDriver::set_line_mode(int mode)
{
    ODS1("DiskDriver::set_line_mode %d", mode);
}

void DiskDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    ODS5("DiskDriver::draw_line (%d,%d) (%d,%d) %d", x1, y1, x2, y2, color);
    ::id::draw_line(x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* redraw --
*
*   Refresh the screen.
*
* Results:
*   None.
*
* Side effects:
*   Redraws the screen.
*
*----------------------------------------------------------------------
*/
void DiskDriver::redraw()
{
    ODS("DiskDriver::redraw");
    m_win_text.paint_screen(0, 80, 0, 25);
}

void DiskDriver::create_window()
{
    g_frame.create_window(m_win_text.get_max_width(), m_win_text.get_max_height());
    m_win_text.set_parent(g_frame.get_window());
    m_win_text.text_on();
}

void DiskDriver::set_video_mode(VideoInfo *mode)
{
    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
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

void DiskDriver::set_clear()
{
    m_win_text.clear();
}

void DiskDriver::display_string(int x, int y, int fg, int bg, const char *text)
{
}

bool DiskDriver::is_text()
{
    return true;
}

void DiskDriver::set_for_text()
{
}

void DiskDriver::set_for_graphics()
{
    hide_text_cursor();
}

bool DiskDriver::is_disk() const
{
    return true;
}

bool DiskDriver::validate_mode(VideoInfo *mode)
{
    /* allow modes of any size with 256 colors */
    return mode->colors == 256;
}

void DiskDriver::pause()
{
    if (m_win_text.get_window())
    {
        ShowWindow(m_win_text.get_window(), SW_HIDE);
    }
}

void DiskDriver::resume()
{
    if (!m_win_text.get_window())
    {
        create_window();
    }

    if (m_win_text.get_window())
    {
        ShowWindow(m_win_text.get_window(), SW_NORMAL);
    }
    m_win_text.resume();
}

void DiskDriver::save_graphics()
{
}

void DiskDriver::restore_graphics()
{
}

void DiskDriver::get_max_screen(int &x_max, int &y_max)
{
    x_max = -1;
    y_max = -1;
}

void DiskDriver::flush()
{
}

static DiskDriver s_disk_driver{};

namespace id
{

Driver *get_disk_driver()
{
    return &s_disk_driver;
}

} // namespace id
