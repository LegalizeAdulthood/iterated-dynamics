// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11BaseDriver.h"

#include <engine/VideoInfo.h>
#include <geometry/plot3d.h>
#include <io/save_timer.h>
#include <ui/id_keys.h>
#include <ui/slideshw.h>
#include <ui/text_screen.h>
#include <ui/zoom.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>

using namespace id::engine;
using namespace id::ui;

namespace id::misc
{

X11BaseDriver::X11BaseDriver(const char *name, const char *description) :
    m_name(name),
    m_description(description)
{
}

bool X11BaseDriver::init(int * /*argc*/, char ** /*argv*/)
{
    if (!m_frame.init("Iterated Dynamics"))
    {
        return false;
    }

    m_frame.set_event_handler(
        [this](const XEvent &event)
        {
            m_text.handle_event(event);
            m_plot.handle_event(event);
        });
    return m_text.init(m_frame.connection()) && m_plot.init(m_frame.connection());
}

bool X11BaseDriver::validate_mode(const VideoInfo & /*mode*/)
{
    return false;
}

void X11BaseDriver::get_max_screen(int &width, int &height)
{
    m_frame.get_max_screen(width, height);
}

void X11BaseDriver::terminate()
{
    m_saved_screens.clear();
    m_saved_cursor.clear();
    m_plot.destroy();
    m_text.destroy();
    m_frame.set_event_handler({});
    m_frame.terminate();
}

void X11BaseDriver::pause()
{
    m_frame.pause();
}

void X11BaseDriver::resume()
{
    m_frame.resume();
}

void X11BaseDriver::schedule_alarm(int /*secs*/)
{
}

void X11BaseDriver::create_window()
{
    m_frame.create_window(m_text.width(), m_text.height());
    if (m_text.create(m_frame.window(), 0, 0))
    {
        m_frame.add_input_window(m_text.window());
        m_text.show();
    }
    if (m_plot.create(m_frame.window(), m_text.width(), m_text.height()))
    {
        m_frame.add_input_window(m_plot.window());
        m_plot.hide();
    }
}

bool X11BaseDriver::resize()
{
    const bool resized{m_frame.resize(m_text.width(), m_text.height())};
    m_text.set_position(0, 0);
    return resized;
}

void X11BaseDriver::read_palette()
{
}

void X11BaseDriver::write_palette()
{
}

int X11BaseDriver::read_pixel(const int x, const int y)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return 0;
    }
    return m_plot.read_pixel(x, y);
}

void X11BaseDriver::write_pixel(const int x, const int y, const int color)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return;
    }
    m_plot.resize(g_screen_x_dots, g_screen_y_dots);
    m_plot.write_pixel(x, y, color);
}

void X11BaseDriver::draw_line(const int x1, const int y1, const int x2, const int y2, const int color)
{
    id::geometry::draw_line(x1, y1, x2, y2, color);
}

void X11BaseDriver::display_string(int /*x*/, int /*y*/, int /*fg*/, int /*bg*/, const char * /*text*/)
{
}

void X11BaseDriver::save_graphics()
{
    m_plot.save_graphics();
}

void X11BaseDriver::restore_graphics()
{
    m_plot.restore_graphics();
}

int X11BaseDriver::get_key()
{
    int ch{};
    do
    {
        if (m_key_buffer)
        {
            ch = m_key_buffer;
            m_key_buffer = 0;
        }
        else
        {
            ch = handle_special_keys(m_frame.get_key_press(true));
        }
    } while (ch == 0);

    return ch;
}

int X11BaseDriver::key_cursor(const int row, const int col)
{
    int result{};
    if (key_pressed())
    {
        result = get_key();
    }
    else
    {
        move_cursor(row, col);
        result = get_key();
        hide_text_cursor();
    }

    return result;
}

int X11BaseDriver::key_pressed()
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

    const int ch{handle_special_keys(m_frame.get_key_press(false))};
    if (m_key_buffer)
    {
        return m_key_buffer;
    }
    m_key_buffer = ch;
    return ch;
}

int X11BaseDriver::wait_key_pressed(const bool timeout)
{
    int count{10};
    while (!key_pressed())
    {
        delay(25);
        if (timeout)
        {
            if (count == 0 || g_zoom_box_width != 0.0)
            {
                break;
            }
            --count;
        }
    }

    return key_pressed();
}

void X11BaseDriver::unget_key(const int key)
{
    assert(m_key_buffer == 0);
    m_key_buffer = key;
}

void X11BaseDriver::shell()
{
}

void X11BaseDriver::set_video_mode(const VideoInfo & /*mode*/)
{
}

void X11BaseDriver::put_string(const int row, const int col, const int attr, const char *msg)
{
    if (row != -1)
    {
        g_text_row = row;
    }
    if (col != -1)
    {
        g_text_col = col;
    }

    const int abs_row{g_text_row_base + g_text_row};
    const int abs_col{g_text_col_base + g_text_col};
    assert(abs_row >= 0 && abs_row < X11_TEXT_MAX_ROW);
    assert(abs_col >= 0 && abs_col < X11_TEXT_MAX_COL);
    m_text.put_string(abs_col, abs_row, attr, msg, g_text_row, g_text_col);
}

bool X11BaseDriver::is_text()
{
    return m_text_not_graphics;
}

void X11BaseDriver::set_for_text()
{
    m_text_not_graphics = true;
    m_text.show();
}

void X11BaseDriver::set_for_graphics()
{
    m_text_not_graphics = false;
    m_text.show();
    hide_text_cursor();
}

void X11BaseDriver::set_clear()
{
    if (m_text_not_graphics)
    {
        m_text.clear();
    }
    else
    {
        if (g_screen_x_dots > 0 && g_screen_y_dots > 0)
        {
            m_plot.resize(g_screen_x_dots, g_screen_y_dots);
            m_plot.clear();
        }
    }
}

void X11BaseDriver::move_cursor(const int row, const int col)
{
    if (row != -1)
    {
        m_cursor.row = row;
        g_text_row = row;
    }
    if (col != -1)
    {
        m_cursor.col = col;
        g_text_col = col;
    }
    m_cursor_shown = true;
    m_text.move_cursor(g_text_row_base + m_cursor.row, g_text_col_base + m_cursor.col);
}

void X11BaseDriver::hide_text_cursor()
{
    m_cursor_shown = false;
    m_text.hide_cursor();
}

void X11BaseDriver::set_attr(const int row, const int col, const int attr, const int count)
{
    if (row != -1)
    {
        g_text_row = row;
    }
    if (col != -1)
    {
        g_text_col = col;
    }
    m_text.set_attr(g_text_row_base + g_text_row, g_text_col_base + g_text_col, attr, count);
}

void X11BaseDriver::scroll_up(const int top, const int bot)
{
    m_text.scroll_up(top, bot);
}

void X11BaseDriver::stack_screen()
{
    if (m_saved_screens.empty())
    {
        set_for_text();
    }
    m_saved_cursor.push_back(TextLocation{g_text_row, g_text_col});
    m_saved_screens.push_back(m_text.get_screen());
    set_clear();
}

void X11BaseDriver::unstack_screen()
{
    assert(!m_saved_cursor.empty());
    const TextLocation packed{m_saved_cursor.back()};
    m_saved_cursor.pop_back();
    g_text_row = packed.row;
    g_text_col = packed.col;
    if (!m_saved_screens.empty())
    {
        m_text.set_screen(m_saved_screens.back());
        m_saved_screens.pop_back();
        move_cursor(-1, -1);
    }
    if (m_saved_screens.empty())
    {
        set_for_graphics();
    }
}

void X11BaseDriver::discard_screen()
{
    if (!m_saved_screens.empty())
    {
        m_saved_screens.pop_back();
        m_saved_cursor.pop_back();
    }
    if (m_saved_screens.empty())
    {
        set_for_graphics();
    }
}

int X11BaseDriver::init_fm()
{
    return 0;
}

void X11BaseDriver::buzzer(Buzzer /*kind*/)
{
}

bool X11BaseDriver::sound_on(int /*frequency*/)
{
    return false;
}

void X11BaseDriver::sound_off()
{
}

void X11BaseDriver::mute()
{
}

bool X11BaseDriver::is_disk() const
{
    return false;
}

int X11BaseDriver::get_char_attr()
{
    return m_text.get_char_attr(g_text_row, g_text_col);
}

void X11BaseDriver::put_char_attr(const int char_attr)
{
    m_text.put_char_attr(g_text_row, g_text_col, char_attr);
}

void X11BaseDriver::delay(const int ms)
{
    m_frame.pump_messages(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(std::max(ms, 0)));
}

void X11BaseDriver::set_keyboard_timeout(const int ms)
{
    m_frame.set_keyboard_timeout(ms);
}

void X11BaseDriver::flush()
{
    m_plot.flush();
    m_frame.pump_messages(false);
}

void X11BaseDriver::debug_text(const char * /*text*/)
{
}

void X11BaseDriver::get_cursor_pos(int &x, int &y) const
{
    x = 0;
    y = 0;
}

void X11BaseDriver::check_memory()
{
}

bool X11BaseDriver::get_filename(
    const char * /*hdg*/, const char * /*type_desc*/, const char * /*type_wildcard*/, std::string & /*result_filename*/)
{
    return false;
}

} // namespace id::misc
