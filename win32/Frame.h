// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "mouse.h"

#include "win_defines.h"
#include <Windows.h>

#include <array>
#include <string>

class Frame
{
public:
    void init(HINSTANCE instance, LPCSTR title);
    void terminate();
    void create_window(int width, int height);
    int get_key_press(bool wait_for_key);
    void pump_messages(bool wait_flag);
    void resize(int width, int height);
    void set_keyboard_timeout(int ms);
    HWND get_window() const
    {
        return m_window;
    }
    int get_width() const
    {
        return m_width;
    }
    int get_height() const
    {
        return m_height;
    }

    // event handlers
    void on_close(HWND window);
    void on_paint(HWND window);
    void on_key_down(HWND window, UINT vk, BOOL down, int repeat_count, UINT flags);
    void on_char(HWND window, TCHAR ch, int num_repeat);
    void on_get_min_max_info(HWND hwnd, LPMINMAXINFO info);
    void on_timer(HWND window, UINT id);
    void on_set_focus(HWND window, HWND old_focus);
    void on_kill_focus(HWND window, HWND old_focus);
    void on_left_button_up(HWND window, int x, int y, UINT flags);
    void on_right_button_up(HWND window, int x, int y, UINT key_flags);
    void on_middle_button_up(HWND window, int x, int y, UINT key_flags);
    void on_mouse_move(HWND window, int x, int y, UINT key_flags);
    void on_left_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags);
    void on_right_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags);
    void on_middle_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags);
    void get_cursor_pos(int &x, int &y) const
    {
        x = static_cast<int>(m_pos.x);
        y = static_cast<int>(m_pos.y);
    }

private:
    enum
    {
        FRAME_TIMER_ID = 2,
        KEY_BUF_MAX = 80,
    };

    void adjust_size(int width, int height);
    void add_key_press(unsigned int key);
    bool key_buffer_full() const
    {
        return m_key_press_count >= KEY_BUF_MAX;
    }

    HINSTANCE m_instance{};
    HWND m_window{};
    std::string m_title;
    int m_width{};
    int m_height{};
    int m_nc_width{};
    int m_nc_height{};
    bool m_has_focus{};
    bool m_timed_out{};
    POINT m_last{-1,-1};
    POINT m_delta{};
    POINT m_pos{};
    int m_look_at_mouse{+MouseLook::IGNORE_MOUSE};
    long m_last_tick{-1};

    // the keypress buffer
    unsigned int m_key_press_count{};
    unsigned int m_key_press_head{};
    unsigned int m_key_press_tail{};
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
};

extern Frame g_frame;
