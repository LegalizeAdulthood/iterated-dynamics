// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "X11Connection.h"

#include <array>
#include <chrono>
#include <string>

namespace id::misc
{

class X11Frame
{
public:
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
    void set_fixed_size(int width, int height);

    X11Connection m_connection;
    Window m_window{};
    std::string m_title;
    int m_width{};
    int m_height{};
    bool m_mapped{};
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
    int m_key_press_count{};
    int m_key_press_head{};
    int m_key_press_tail{};
    bool m_keyboard_timeout_active{};
    bool m_timed_out{};
    std::chrono::milliseconds m_keyboard_timeout_interval{};
    std::chrono::steady_clock::time_point m_keyboard_deadline{};
};

} // namespace id::misc
