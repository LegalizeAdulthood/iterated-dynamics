// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11BaseDriver.h"

#include <config/cmd_shell.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <geometry/plot3d.h>
#include <io/save_timer.h>
#include <ui/diskvid.h>
#include <ui/id_keys.h>
#include <ui/slideshw.h>
#include <ui/stop_msg.h>
#include <ui/text_screen.h>
#include <ui/video.h>
#include <ui/zoom.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

using namespace id::engine;
using namespace id::ui;

namespace id::engine
{
extern int g_and_color;
}

namespace id::misc
{

namespace
{

constexpr std::chrono::milliseconds OUTPUT_FLUSH_INTERVAL{100};

bool has_graphics_mode()
{
    return g_adapter >= 0 && g_adapter < MAX_VIDEO_MODES && g_video_table[g_adapter].x_dots > 0 &&
        g_video_table[g_adapter].y_dots > 0;
}

} // namespace

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

bool X11BaseDriver::validate_mode(const VideoInfo &mode)
{
    int width{};
    int height{};
    get_max_screen(width, height);
    return mode.x_dots <= width && mode.y_dots <= height && mode.colors == 256;
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
    if (m_frame.window() == None)
    {
        create_window();
        return;
    }
    m_frame.resume();
    if (m_text_not_graphics)
    {
        set_for_text();
    }
    else
    {
        set_for_graphics();
    }
}

void X11BaseDriver::schedule_alarm(int /*secs*/)
{
}

void X11BaseDriver::create_window()
{
    const WindowLayout layout{get_window_layout()};
    m_frame.create_window(layout.frame_width, layout.frame_height);
    if (m_text.create(m_frame.window(), layout.text_x, layout.text_y))
    {
        m_frame.add_input_window(m_text.window());
    }
    if (m_plot.create(m_frame.window(), layout.plot_width, layout.plot_height))
    {
        m_frame.add_input_window(m_plot.window());
    }
    center_windows(layout);
    if (m_text_not_graphics)
    {
        set_for_text();
    }
    else
    {
        set_for_graphics();
    }
}

bool X11BaseDriver::resize()
{
    const WindowLayout layout{get_window_layout()};
    const bool frame_resized{m_frame.resize(layout.frame_width, layout.frame_height)};
    const bool plot_resized{m_plot.resize(layout.plot_width, layout.plot_height)};
    center_windows(layout);
    return frame_resized || plot_resized;
}

void X11BaseDriver::read_palette()
{
    m_plot.read_palette();
}

void X11BaseDriver::write_palette()
{
    m_plot.write_palette();
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

void X11BaseDriver::display_string(const int x, const int y, const int fg, const int bg, const char *text)
{
    if (g_screen_x_dots <= 0 || g_screen_y_dots <= 0)
    {
        return;
    }
    m_plot.resize(g_screen_x_dots, g_screen_y_dots);
    m_plot.display_string(x, y, fg, bg, text);
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
    flush_output();
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
    const auto timeout{[] { driver_flush(); }};
    if (config::cmd_shell(timeout))
    {
        return;
    }
    stop_msg("Couldn't run shell '" + config::cmd_shell_command() + "', error " +
        std::to_string(config::get_cmd_shell_error()));
}

void X11BaseDriver::set_video_mode(const VideoInfo &mode)
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
    m_plot.clear();
    if (g_disk_flag)
    {
        end_disk();
    }
    set_normal_dot();
    set_normal_span();
    set_for_graphics();
    set_clear();
    m_flush_started = false;
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
    m_plot.hide();
    m_text.show();
}

void X11BaseDriver::set_for_graphics()
{
    m_text_not_graphics = false;
    hide_text_cursor();
    m_text.hide();
    m_plot.show();
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
    if (!m_frame.connection().is_open())
    {
        return;
    }
    XBell(m_frame.connection().display(), 0);
    XFlush(m_frame.connection().display());
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

void X11BaseDriver::debug_text(const char *text)
{
    if (text == nullptr)
    {
        return;
    }
    std::fputs(text, stderr);
    std::fflush(stderr);
}

void X11BaseDriver::get_cursor_pos(int &x, int &y) const
{
    m_frame.get_cursor_pos(x, y);
}

void X11BaseDriver::check_memory()
{
}

bool X11BaseDriver::get_filename(
    const char * /*hdg*/, const char * /*type_desc*/, const char * /*type_wildcard*/, std::string & /*result_filename*/)
{
    return true;
}

X11BaseDriver::WindowLayout X11BaseDriver::get_window_layout() const
{
    WindowLayout result{};
    const int text_width{m_text.width()};
    const int text_height{m_text.height()};
    result.plot_width = has_graphics_mode() ? g_video_table[g_adapter].x_dots : text_width;
    result.plot_height = has_graphics_mode() ? g_video_table[g_adapter].y_dots : text_height;
    result.frame_width = std::max(text_width, result.plot_width);
    result.frame_height = std::max(text_height, result.plot_height);
    result.text_x = (result.frame_width - text_width) / 2;
    result.text_y = (result.frame_height - text_height) / 2;
    result.plot_x = (result.frame_width - result.plot_width) / 2;
    result.plot_y = (result.frame_height - result.plot_height) / 2;
    return result;
}

void X11BaseDriver::center_windows(const WindowLayout &layout)
{
    m_text.set_position(layout.text_x, layout.text_y);
    m_plot.set_position(layout.plot_x, layout.plot_y);
}

void X11BaseDriver::flush_output()
{
    if (m_text_not_graphics)
    {
        return;
    }

    const auto now{std::chrono::steady_clock::now()};
    if (m_flush_started && now - m_last_flush < OUTPUT_FLUSH_INTERVAL)
    {
        return;
    }

    flush();
    m_last_flush = now;
    m_flush_started = true;
}

} // namespace id::misc
