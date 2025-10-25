// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "WxDiskDriver.h"

#include <engine/calcfrac.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <geometry/plot3d.h>
#include <gui/App.h>
#include <ui/diskvid.h>
#include <ui/video.h>
#include <ui/zoom.h>

using namespace id::engine;
using namespace id::ui;

namespace id::misc
{

#define DRIVER_MODE(width_, height_, comment_) \
    { 0, width_, height_, 256, nullptr, comment_ }
static VideoInfo s_modes[] = {
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
        g_dac_box[i][1] = ((i + 16 & 28) >> 2)*8+7;
        g_dac_box[i][2] = (i + 2 & 3)*16+15;
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

bool WxDiskDriver::init(int *argc, char **argv)
{
    if (!WxBaseDriver::init(argc, argv))
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
void WxDiskDriver::set_video_mode(const VideoInfo &mode)
{
    // This should already be the case, but let's confirm assumptions.
    assert(g_video_table[g_adapter].x_dots == mode.x_dots);
    assert(g_video_table[g_adapter].y_dots == mode.y_dots);
    assert(g_video_table[g_adapter].colors == mode.colors);
    assert(g_video_table[g_adapter].driver == this);

    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
    g_good_mode = true;
    g_and_color = g_colors - 1;
    g_box_count = 0;
    g_dac_count = g_cycle_limit;
    g_got_real_dac = true; // we are "VGA"
    read_palette();

    resize();
    wxGetApp().clear();
    if (g_disk_flag)
    {
        end_disk();
    }
    set_normal_dot();
    set_normal_span();
    set_for_graphics();
    set_clear();
}

void WxDiskDriver::discard_screen()
{
    if (!m_saved_screens.empty())
    {
        // unstack
        m_saved_screens.pop_back();
        m_saved_cursor.pop_back();
    }
    // discarding last text screen reverts to showing graphics
    if (m_saved_screens.empty() && !is_disk())
    {
        set_for_graphics();
    }
}

bool WxDiskDriver::is_disk() const
{
    return true;
}

bool WxDiskDriver::validate_mode(const VideoInfo &mode)
{
    int width;
    int height;
    get_max_screen(width, height);

    // allow modes <= size of screen with 256 colors
    return mode.x_dots <= width
        && mode.y_dots <= height
        && mode.colors == 256;
}

void WxDiskDriver::create_window()
{
    wxGetApp().create_window(g_video_table[g_adapter].x_dots, g_video_table[g_adapter].y_dots);
}

bool WxDiskDriver::resize()
{
    wxGetApp().fit_to_text();
    if (g_video_table[g_adapter].x_dots == m_width
        && g_video_table[g_adapter].y_dots == m_height)
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

void WxDiskDriver::read_palette()
{
    if (!g_got_real_dac)
    {
        return;
    }
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = m_clut[i][0];
        g_dac_box[i][1] = m_clut[i][1];
        g_dac_box[i][2] = m_clut[i][2];
    }
}

void WxDiskDriver::write_palette()
{
    for (int i = 0; i < 256; i++)
    {
        m_clut[i][0] = g_dac_box[i][0];
        m_clut[i][1] = g_dac_box[i][1];
        m_clut[i][2] = g_dac_box[i][2];
    }
}

int WxDiskDriver::read_pixel(int x, int y)
{
    return wxGetApp().read_pixel(x, y);
}

void WxDiskDriver::write_pixel(int x, int y, int color)
{
    wxGetApp().write_pixel(x, y, color);
}

void WxDiskDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    geometry::draw_line(x1, y1, x2, y2, color);
}

void WxDiskDriver::display_string(int x, int y, int fg, int bg, const char *text)
{
    wxGetApp().display_string(x, y, fg, bg, text);
}

void WxDiskDriver::save_graphics()
{
    wxGetApp().save_graphics();
}

void WxDiskDriver::restore_graphics()
{
    wxGetApp().restore_graphics();
}

static WxDiskDriver s_wx_disk_driver{};

Driver *get_wx_disk_driver()
{
    return &s_wx_disk_driver;
}

} // namespace id::misc
