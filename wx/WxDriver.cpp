// SPDX-License-Identifier: GPL-3.0-only
//
/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "WxDriver.h"

#include "engine/id_data.h"
#include "gui/IdApp.h"
#include "gui/IdFrame.h"
#include "io/save_timer.h"
#include "io/special_dirs.h"
#include "misc/stack_avail.h"
#include "ui/id_keys.h"
#include "ui/read_ticker.h"
#include "ui/slideshw.h"
#include "ui/text_screen.h"
#include "ui/video_mode.h"
#include "ui/zoom.h"

#include <cassert>
#include <chrono>
#include <ctime>
#include <string>
#include <thread>

#ifdef WIN32
#include <crtdbg.h>
#endif

using namespace id::ui;

namespace id::misc
{

#if 0
long readticker()
{
    // TODO
    return (long) 0;
}

long stackavail()
{
    // TODO
    return 0L;
}

using uclock_t = unsigned long;
uclock_t usec_clock()
{
    uclock_t result{};
    // TODO
    assert(FALSE);

    return result;
}

void restart_uclock()
{
    // TODO
}
#endif

static void flush_output()
{
    static time_t start = 0;
    static long ticks_per_second = 0;
    static long last = 0;
    static long frames_per_second = 10;

    if (!ticks_per_second)
    {
        if (!start)
        {
            std::time(&start);
            last = read_ticker();
        }
        else
        {
            const std::time_t now = std::time(nullptr);
            const long now_ticks = read_ticker();
            if (now > start)
            {
                ticks_per_second = (now_ticks - last)/ static_cast<long>(now - start);
            }
        }
    }
    else
    {
        if (long now = read_ticker(); (now - last)*frames_per_second > ticks_per_second)
        {
            driver_flush();
            wxGetApp().pump_messages(false);
            last = now;
        }
    }
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

void WxDriver::terminate()
{
    m_saved_screens.clear();
    m_saved_cursor.clear();
}

bool WxDriver::init(int *argc, char **argv)
{
    // nothing needs to be done in wxWidgets
    return true;
}

/* key_pressed
 *
 * Return 0 if no key has been pressed, or the FIK value if it has.
 * driver_get_key() must still be called to eat the key; this routine
 * only peeks ahead.
 *
 * When a keystroke has been found by the underlying wintext_xxx
 * message pump, stash it in the one key buffer for later use by
 * get_key.
 */
int WxDriver::key_pressed()
{
    if (m_key_buffer)
    {
        return m_key_buffer;
    }
    if (io::auto_save_needed())
    {
        unget_key(ID_KEY_FAKE_AUTOSAVE);
        assert(m_key_buffer == ID_KEY_FAKE_AUTOSAVE);
        return m_key_buffer;
    }
    flush_output();
    const int ch = handle_special_keys(wxGetApp().get_key_press(false));
    if (m_key_buffer)
    {
        return m_key_buffer;
    }
    m_key_buffer = ch;

    return ch;
}

/* unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void WxDriver::unget_key(const int key)
{
    assert(0 == m_key_buffer);
    m_key_buffer = key;
}

/* get_key
 *
 * Get a keystroke, blocking if necessary.  First, check the key buffer
 * and if that's empty ask the wintext window to pump a keystroke for us.
 * If we get it, pass it off to handle tab and help displays.  If those
 * displays ate the key, then get another one.
 */
int WxDriver::get_key()
{
    throw std::runtime_error("not implemented");
    //int ch;

    //do
    //{
    //    if (m_key_buffer)
    //    {
    //        ch = m_key_buffer;
    //        m_key_buffer = 0;
    //    }
    //    else
    //    {
    //        ch = handle_special_keys(g_frame.get_key_press(true));
    //    }
    //}
    //while (ch == 0);

    //return ch;
}

// Spawn a command prompt.
void  WxDriver::shell()
{
    throw std::runtime_error("not implemented");
    //STARTUPINFO si =
    //{
    //    sizeof(si)
    //};
    //PROCESS_INFORMATION pi = { 0 };
    //const char *comspec = getenv("COMSPEC");
    //if (comspec == nullptr)
    //{
    //    comspec = "cmd.exe";
    //}
    //const std::string command_line(comspec);
    //if (CreateProcessA(
    //        command_line.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
    //{
    //    DWORD status = WaitForSingleObject(pi.hProcess, 100);
    //    while (WAIT_TIMEOUT == status)
    //    {
    //        g_frame.pump_messages(false);
    //        status = WaitForSingleObject(pi.hProcess, 100);
    //    }
    //    CloseHandle(pi.hProcess);
    //}
    //else
    //{
    //    stopmsg("Couldn't run shell '" + command_line + "', error " + std::to_string(GetLastError()));
    //}
}

void WxDriver::hide_text_cursor()
{
    wxGetApp().hide_text_cursor();
}

void WxDriver::set_video_mode(VideoInfo *mode)
{
    throw std::runtime_error("not implemented");
    // initially, set the virtual line to be the scan line length
    //g_is_true_color = false;            // assume not truecolor
    //g_vesa_x_res = 0;                   // reset indicators used for
    //g_vesa_y_res = 0;                   // virtual screen limits estimation
    //g_good_mode = true;
    //if (g_dot_mode != 0)
    //{
    //    g_and_color = g_colors-1;
    //    g_box_count = 0;
    //    g_dac_learn = true;
    //    g_dac_count = g_cycle_limit;
    //    g_got_real_dac = true;          // we are "VGA"
    //    read_palette();
    //}

    //resize();
    //if (g_disk_flag)
    //{
    //    enddisk();
    //}
    //set_normal_dot();
    //set_normal_span();
    //set_for_graphics();
    //set_clear();
}

void WxDriver::put_string(int row, int col, int attr, const char *msg)
{
    if (-1 != row)
    {
        g_text_row = row;
    }
    if (-1 != col)
    {
        g_text_col = col;
    }
    {
        const int abs_row = g_text_row_base + g_text_row;
        const int abs_col = g_text_col_base + g_text_col;
        assert(abs_row >= 0 && abs_row < gui::WINTEXT_MAX_ROW);
        assert(abs_col >= 0 && abs_col < gui::WINTEXT_MAX_COL);
        wxGetApp().put_string(abs_col, abs_row, attr, msg, g_text_row, g_text_col);
    }
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
void WxDriver::scroll_up(int top, int bot)
{
    wxGetApp().scroll_up(top, bot);
}

void WxDriver::move_cursor(int row, int col)
{
    throw std::runtime_error("not implemented");
    //if (row != -1)
    //{
    //    m_cursor_row = row;
    //    g_text_row = row;
    //}
    //if (col != -1)
    //{
    //    m_cursor_col = col;
    //    g_text_col = col;
    //}
    //m_win_text.cursor(g_text_cbase + m_cursor_col, g_text_rbase + m_cursor_row, 1);
    //m_cursor_shown = true;
}

void WxDriver::set_attr(int row, int col, int attr, int count)
{
    if (-1 != row)
    {
        g_text_row = row;
    }
    if (-1 != col)
    {
        g_text_col = col;
    }
    wxGetApp().set_attr(g_text_row_base + g_text_row, g_text_col_base + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
void WxDriver::stack_screen()
{
    throw std::runtime_error("not implemented");
    // set for text mode if this is the first screen stacked
    //if (m_saved_screens.empty())
    //{
    //    driver_set_for_text();
    //}
    //m_saved_cursor.push_back(g_text_row * 80 + g_text_col);
    //m_saved_screens.push_back(m_win_text.get_screen());
    //driver_set_clear();
}

void WxDriver::unstack_screen()
{
    throw std::runtime_error("not implemented");
    //assert(!m_saved_cursor.empty());
    //const int packed{m_saved_cursor.back()};
    //m_saved_cursor.pop_back();
    //g_text_row = packed / 80;
    //g_text_col = packed % 80;
    //if (!m_saved_screens.empty())
    //{
    //    // unstack
    //    m_win_text.set_screen(m_saved_screens.back());
    //    m_saved_screens.pop_back();
    //    move_cursor(-1, -1);
    //}
    //// unstacking the last saved screen reverts to graphics display
    //if (m_saved_screens.empty())
    //{
    //    set_for_graphics();
    //}
}

void WxDriver::discard_screen()
{
    if (!m_saved_screens.empty())
    {
        // unstack
        m_saved_screens.pop_back();
        m_saved_cursor.pop_back();
    }
    // discarding last text screen reverts to showing graphics
    if (m_saved_screens.empty())
    {
        set_for_graphics();
    }
}

int WxDriver::init_fm()
{
    throw std::runtime_error("not implemented");
    return 0;
}

void WxDriver::buzzer(Buzzer kind)
{
    throw std::runtime_error("not implemented");
    //UINT beep{MB_OK};
    //switch (kind)
    //{
    //case buzzer_codes::COMPLETE:
    //default:
    //    break;
    //case buzzer_codes::INTERRUPT:
    //    beep = MB_ICONWARNING;
    //    break;
    //case buzzer_codes::PROBLEM:
    //    beep = MB_ICONERROR;
    //    break;
    //}
    //MessageBeep(beep);
}

bool WxDriver::sound_on(int freq)
{
    throw std::runtime_error("not implemented");
    return false;
}

void WxDriver::sound_off()
{
    throw std::runtime_error("not implemented");
}

void WxDriver::mute()
{
    throw std::runtime_error("not implemented");
}

bool WxDriver::is_disk() const
{
    return false;
}

int WxDriver::key_cursor(int row, int col)
{
    throw std::runtime_error("not implemented");
    //if (-1 != row)
    //{
    //    m_cursor_row = row;
    //    g_text_row = row;
    //}
    //if (-1 != col)
    //{
    //    m_cursor_col = col;
    //    g_text_col = col;
    //}

    //int result;
    //if (key_pressed())
    //{
    //    result = get_key();
    //}
    //else
    //{
    //    m_cursor_shown = true;
    //    m_win_text.cursor(m_cursor_col, m_cursor_row, 1);
    //    result = get_key();
    //    hide_text_cursor();
    //    m_cursor_shown = false;
    //}

    //return result;
}

int WxDriver::wait_key_pressed(const bool timeout)
{
    int count = 10;
    while (!key_pressed())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        if (timeout)
        {
            // timeout early if zooming
            if (count == 0 || g_zoom_box_width != 0.0)
            {
                break;
            }
            --count;
        }
    }

    return key_pressed();
}

int WxDriver::get_char_attr()
{
    throw std::runtime_error("not implemented");
    //return m_win_text.get_char_attr(g_text_row, g_text_col);
}

void WxDriver::put_char_attr(int char_attr)
{
    throw std::runtime_error("not implemented");
    //m_win_text.put_char_attr(g_text_row, g_text_col, char_attr);
}

void WxDriver::delay(int ms)
{
    wxGetApp().pump_messages(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void WxDriver::get_true_color(int x, int y, int *r, int *g, int *b, int *a)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::put_true_color(int x, int y, int r, int g, int b, int a)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::set_keyboard_timeout(int ms)
{
    throw std::runtime_error("not implemented");
    //g_frame.set_keyboard_timeout(ms);
}

void WxDriver::debug_text(const char *text)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::get_cursor_pos(int &x, int &y) const
{
    throw std::runtime_error("not implemented");
    // g_frame.get_cursor_pos(x, y);
}

bool WxDriver::validate_mode(const VideoInfo &mode)
{
    int width;
    int height;
    get_max_screen(width, height);

    // allow modes <= size of screen with 256 colors
    return mode.x_dots <= width
        && mode.y_dots <= height
        && mode.colors == 256;
}

void WxDriver::get_max_screen(int &width, int &height)
{
    wxDisplaySize(&width, &height);
}

void WxDriver::pause()
{
    throw std::runtime_error("not implemented");
}

void WxDriver::resume()
{
    throw std::runtime_error("not implemented");
}

void WxDriver::schedule_alarm(int secs)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::create_window()
{
    wxGetApp().create_window(
        g_video_table[engine::g_adapter].x_dots, g_video_table[engine::g_adapter].y_dots);
}

bool WxDriver::resize()
{
    throw std::runtime_error("not implemented");
    return false;
}

void WxDriver::redraw()
{
    throw std::runtime_error("not implemented");
}

int WxDriver::read_palette()
{
    throw std::runtime_error("not implemented");
    return 0;
}

int WxDriver::write_palette()
{
    throw std::runtime_error("not implemented");
    return 0;
}

int WxDriver::read_pixel(int x, int y)
{
    throw std::runtime_error("not implemented");
    return 0;
}

void WxDriver::write_pixel(int x, int y, int color)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::read_span(int y, int x, int lastx, Byte *pixels)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::write_span(int y, int x, int lastx, Byte *pixels)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::set_line_mode(int mode)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::display_string(int x, int y, int fg, int bg, const char *text)
{
    throw std::runtime_error("not implemented");
}

void WxDriver::save_graphics()
{
    throw std::runtime_error("not implemented");
}

void WxDriver::restore_graphics()
{
    throw std::runtime_error("not implemented");
}

bool WxDriver::is_text()
{
    return wxGetApp().is_text();
}

void WxDriver::set_for_text()
{
    wxGetApp().set_for_text();
}

void WxDriver::set_for_graphics()
{
    wxGetApp().set_for_graphics();
}

void WxDriver::set_clear()
{
    wxGetApp().clear();
}

void WxDriver::flush()
{
    wxGetApp().flush();
}

void WxDriver::check_memory()
{
#ifdef WIN32
    assert(_CrtCheckMemory() == TRUE);
#endif
    // TODO: is there something comparable we can do under gcc/clang?
}

bool WxDriver::get_filename(
    const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename)
{
    throw std::runtime_error("not implemented");
    return false;
}

static WxDriver s_wx_driver{};

Driver *get_wx_driver()
{
    return &s_wx_driver;
}

} // namespace id::misc
