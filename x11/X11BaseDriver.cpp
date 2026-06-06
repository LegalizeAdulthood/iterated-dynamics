// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11BaseDriver.h"

#include <engine/VideoInfo.h>
#include <io/save_timer.h>
#include <ui/id_keys.h>
#include <ui/slideshw.h>
#include <ui/zoom.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>

using namespace id::engine;
using namespace id::ui;

namespace id::misc
{

namespace
{

enum
{
    DEFAULT_WINDOW_WIDTH = 640,
    DEFAULT_WINDOW_HEIGHT = 480,
};

} // namespace

X11BaseDriver::X11BaseDriver(const char *name, const char *description) :
    m_name(name),
    m_description(description)
{
}

bool X11BaseDriver::init(int * /*argc*/, char ** /*argv*/)
{
    return m_frame.init("Iterated Dynamics");
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
    m_frame.create_window(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

bool X11BaseDriver::resize()
{
    return m_frame.resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

void X11BaseDriver::read_palette()
{
}

void X11BaseDriver::write_palette()
{
}

int X11BaseDriver::read_pixel(int /*x*/, int /*y*/)
{
    return 0;
}

void X11BaseDriver::write_pixel(int /*x*/, int /*y*/, int /*color*/)
{
}

void X11BaseDriver::draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, int /*color*/)
{
}

void X11BaseDriver::display_string(int /*x*/, int /*y*/, int /*fg*/, int /*bg*/, const char * /*text*/)
{
}

void X11BaseDriver::save_graphics()
{
}

void X11BaseDriver::restore_graphics()
{
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

void X11BaseDriver::put_string(int /*row*/, int /*col*/, int /*attr*/, const char * /*msg*/)
{
}

bool X11BaseDriver::is_text()
{
    return true;
}

void X11BaseDriver::set_for_text()
{
}

void X11BaseDriver::set_for_graphics()
{
}

void X11BaseDriver::set_clear()
{
}

void X11BaseDriver::move_cursor(int /*row*/, int /*col*/)
{
}

void X11BaseDriver::hide_text_cursor()
{
}

void X11BaseDriver::set_attr(int /*row*/, int /*col*/, int /*attr*/, int /*count*/)
{
}

void X11BaseDriver::scroll_up(int /*top*/, int /*bot*/)
{
}

void X11BaseDriver::stack_screen()
{
}

void X11BaseDriver::unstack_screen()
{
}

void X11BaseDriver::discard_screen()
{
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
    return 0;
}

void X11BaseDriver::put_char_attr(int /*char_attr*/)
{
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
