// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "gui/Colormap.h"
#include "gui/Screen.h"
#include "ui/video_mode.h"

#include <wx/app.h>

namespace id::gui
{

class Frame;

class App : public wxApp
{
public:
    ~App() override = default;

    bool OnInit() override;
    int OnRun() override;

    void create_window(int width, int height);
    void pump_messages(bool wait_flag);
    bool is_text() const;
    void set_for_text();
    void set_for_graphics();
    void clear();
    void put_string(int col, int row, int attr, const char *msg, int &end_row, int &end_col);
    void set_attr(int row, int col, int attr, int count);
    void hide_text_cursor();
    void scroll_up(int top, int bot);
    void move_cursor(int row, int col);
    int get_key_press(bool wait_for_key);
    void flush();
    Screen get_screen() const;
    void set_screen(const Screen &screen);
    int get_char_attr(int row, int col);
    void put_char_attr(int row, int col, int char_attr);
    void set_keyboard_timeout(int ms);
    void get_cursor_pos(int &x, int &y) const;
    void pause();
    void resume();
    Colormap read_palette();
    void write_palette(const Colormap& map);
    bool resize(int width, int height);
    void write_pixel(int x, int y, int color);
    int read_pixel(int x, int y);
    void display_string(int x, int y, int fg, int bg, const char *text);
    void save_graphics();
    void restore_graphics();

private:
    Frame *m_frame{};
};

} // namespace id::gui

id::gui::App &wxGetApp();
