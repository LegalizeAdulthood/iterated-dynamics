// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "X11Connection.h"

#include <array>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace id::misc
{

class X11Frame
{
public:
    using EventHandler = std::function<void(const XEvent &)>;

    bool init(const char *title);
    void terminate();
    void create_window(int width, int height);
    bool resize(int width, int height);
    void pause();
    void resume();
    void pump_messages(bool wait);
    int get_key_press(bool wait);
    void set_keyboard_timeout(int ms);
    void get_max_screen(int &width, int &height) const;
    X11Connection &connection();
    const X11Connection &connection() const;
    Window window() const;
    void set_event_handler(EventHandler handler);
    void add_input_window(Window window);
    void remove_input_window(Window window);
    void get_cursor_pos(int &x, int &y) const;

private:
    enum
    {
        KEY_BUF_MAX = 80,
    };

    bool key_buffer_full() const
    {
        return m_key_press_count >= KEY_BUF_MAX;
    }

    void add_key_press(unsigned int key);
    void destroy_window();
    void handle_event(const XEvent &event);
    void handle_key_press(XKeyEvent event);
    void handle_button_press(XButtonEvent event);
    void handle_button_release(XButtonEvent event);
    void handle_mouse_move(XMotionEvent event);
    void update_cursor_pos(int x, int y);
    bool is_double_click(const XButtonEvent &event) const;
    void grab_pointer(Time time);
    void ungrab_pointer(Time time);
    bool is_input_window(Window window) const;
    void set_fixed_size(int width, int height);
    void save_window_position() const;

    X11Connection m_connection;
    Window m_window{};
    std::string m_title;
    int m_width{};
    int m_height{};
    bool m_mapped{};
    EventHandler m_event_handler;
    std::vector<Window> m_input_windows;
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
    int m_key_press_count{};
    int m_key_press_head{};
    int m_key_press_tail{};
    bool m_keyboard_timeout_active{};
    bool m_timed_out{};
    std::chrono::milliseconds m_keyboard_timeout_interval{};
    std::chrono::steady_clock::time_point m_keyboard_deadline{};
    int m_cursor_x{};
    int m_cursor_y{};
    Window m_last_button_window{};
    unsigned int m_last_button{};
    Time m_last_button_time{};
    int m_last_button_x{};
    int m_last_button_y{};
    bool m_pointer_grabbed{};
};

} // namespace id::misc
