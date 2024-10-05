// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for id.
 */
#include "port.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "id_data.h"
#include "os.h"
#include "plot3d.h"
#include "rotate.h"
#include "spindac.h"
#include "video.h"
#include "video_mode.h"

#include "win_defines.h"
#include <Windows.h>

#include <cassert>

#include "win_text.h"
#include "frame.h"
#include "d_win32.h"
#include "ods.h"

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
    void write_span(int y, int x, int lastx, BYTE *pixels) override;
    void read_span(int y, int x, int lastx, BYTE *pixels) override;
    void set_line_mode(int mode) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void redraw() override;
    void create_window() override;
    void set_video_mode(VIDEOINFO *mode) override;
    void set_clear() override;
    void display_string(int x, int y, int fg, int bg, char const *text) override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    bool diskp() override;
    bool validate_mode(VIDEOINFO *mode) override;
    void pause() override;
    void resume() override;
    void save_graphics() override;
    void restore_graphics() override;
    void get_max_screen(int &xmax, int &ymax) override;
    void flush() override;

private:
    int width{};
    int height{};
    unsigned char clut[256][3]{};
};

#define DRIVER_MODE(width_, height_ ) \
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
static void initdacbox()
{
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = (i >> 5)*8+7;
        g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
        g_dac_box[i][2] = (((i+2) & 3))*16+15;
    }
    g_dac_box[0][2] = 0;
    g_dac_box[0][1] = g_dac_box[0][2];
    g_dac_box[0][0] = g_dac_box[0][1];
    g_dac_box[1][2] = 255;
    g_dac_box[1][1] = g_dac_box[1][2];
    g_dac_box[1][0] = g_dac_box[1][1];
    g_dac_box[2][0] = 190;
    g_dac_box[2][2] = 255;
    g_dac_box[2][1] = g_dac_box[2][2];
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

    initdacbox();

    // add default list of video modes
    for (VIDEOINFO &mode : modes)
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
    if ((g_video_table[g_adapter].xdots == width)
        && (g_video_table[g_adapter].ydots == height))
    {
        return false;
    }

    if (g_disk_flag)
    {
        enddisk();
    }
    startdisk();

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
        g_dac_box[i][0] = clut[i][0];
        g_dac_box[i][2] = clut[i][2];
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
        clut[i][0] = g_dac_box[i][0];
        clut[i][1] = g_dac_box[i][1];
        clut[i][2] = g_dac_box[i][2];
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
    putcolor_a(x, y, color);
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
    return getcolor(x, y);
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
void DiskDriver::write_span(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx-x+1;
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
void DiskDriver::read_span(int y, int x, int lastx, BYTE *pixels)
{
    ODS3("DiskDriver::read_span (%d,%d,%d)", y, x, lastx);
    int width = lastx-x+1;
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
    ::draw_line(x1, y1, x2, y2, color);
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

void DiskDriver::set_video_mode(VIDEOINFO *mode)
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
        g_got_real_dac = true;
        read_palette();
    }

    resize();
    set_disk_dot();
    set_normal_span();
}

void DiskDriver::set_clear()
{
    m_win_text.clear();
}

void DiskDriver::display_string(int x, int y, int fg, int bg, char const *text)
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

bool DiskDriver::diskp()
{
    return true;
}

bool DiskDriver::validate_mode(VIDEOINFO *mode)
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

void DiskDriver::get_max_screen(int &xmax, int &ymax)
{
    xmax = -1;
    ymax = -1;
}

void DiskDriver::flush()
{
}

static DiskDriver s_disk_driver{};

Driver *g_disk_driver = &s_disk_driver;
