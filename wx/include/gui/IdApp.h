// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <wx/app.h>

namespace id::gui
{

class IdFrame;

class IdApp : public wxApp
{
public:
    ~IdApp() override = default;

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
    int key_pressed() const;
    void scroll_up(int top, int bot);

private:
    IdFrame *m_frame{};
};

} // namespace id::gui

id::gui::IdApp &wxGetApp();
