#pragma once

#include "win_defines.h"
#include <Windows.h>

#include <array>
#include <string>

#define KEYBUFMAX 80

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

private:
    enum
    {
        FRAME_TIMER_ID = 2
    };

    void adjust_size(int width, int height);
    void add_key_press(unsigned int key);

    HINSTANCE m_instance{};
    HWND m_window{};
    std::string m_title;
    int m_width{};
    int m_height{};
    int m_nc_width{};
    int m_nc_height{};
    bool m_has_focus{};
    bool m_timed_out{};

    // the keypress buffer
    unsigned int m_key_press_count{};
    unsigned int m_key_press_head{};
    unsigned int m_key_press_tail{};
    std::array<unsigned int, KEYBUFMAX> m_key_press_buffer{};
};

extern Frame g_frame;
