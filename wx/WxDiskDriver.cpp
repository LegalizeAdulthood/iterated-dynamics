// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "WxDiskDriver.h"

#include "gui/App.h"
#include "gui/Colormap.h"
#include "gui/Frame.h"

#include <engine/calcfrac.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <geometry/plot3d.h>
#include <io/save_timer.h>
#include <ui/diskvid.h>
#include <ui/id_keys.h>
#include <ui/read_ticker.h>
#include <ui/slideshw.h>
#include <ui/stop_msg.h>
#include <ui/text_screen.h>
#include <ui/video.h>
#include <ui/zoom.h>

#include <config/cmd_shell.h>

#include <wx/log.h>

#include <cassert>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef WIN32
#include <crtdbg.h>
#endif

using namespace id::config;
using namespace id::engine;
using namespace id::ui;

namespace id::misc
{

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

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
    return wxGetApp().resize(g_video_table[g_adapter].x_dots, g_video_table[g_adapter].y_dots);
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
