// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/App.h>

#include "ui/id_main.h"

#include <gui/Frame.h>
#include "../win32/instance.h"

#include <wx/wx.h>
#include <wx/evtloop.h>
#ifdef WIN32
#include <wx/msw/private.h>
#endif

#include <array>
#include <cassert>

using namespace id::misc;
using namespace id::ui;

wxIMPLEMENT_APP(id::gui::App);

namespace id::gui
{

bool App::OnInit()
{
#ifdef WIN32
    g_instance = wxGetInstance();
#endif

    return true;
}

int App::OnRun()
{
    m_mainLoop = CreateMainLoop();
    return id_main(argc, argv);
}

void App::create_window(const int width, const int height)
{
    m_frame = new Frame();
    m_frame->set_plot_size(width, height);
    m_frame->Show(true);
}

void App::pump_messages(bool wait_flag)
{
    if (wxEventLoopBase *loop = GetMainLoop())
    {
        while (loop->Pending())
        {
            if (!loop->Dispatch())
            {
                return;
            }
        }
    }
}

bool App::is_text() const
{
    return m_frame->is_text();
}

void App::set_for_text()
{
    m_frame->set_for_text();
}

void App::set_for_graphics()
{
    m_frame->set_for_graphics();
}

void App::clear()
{
    m_frame->clear();
}

void App::put_string(int col, int row, int attr, const char *msg, int &end_row, int &end_col)
{
    m_frame->put_string(row, col, attr, msg, end_row, end_col);
}

void App::set_attr(int row, int col, int attr, int count)
{
    m_frame->set_attr(row, col, attr, count);
}

void App::hide_text_cursor()
{
    m_frame->hide_text_cursor();
}

void App::scroll_up(int top, int bot)
{
    m_frame->scroll_up(top, bot);
}

void App::move_cursor(int row, int col)
{
    m_frame->move_cursor(row, col);
}

int App::get_key_press(bool wait_for_key)
{
    return m_frame->get_key_press(wait_for_key);
}

void App::flush()
{
    m_frame->flush();
}

Screen App::get_screen() const
{
    return m_frame->get_screen();
}

void App::set_screen(const Screen &screen)
{
    m_frame->set_screen(screen);
}

int App::get_char_attr(int row, int col)
{
    return m_frame->get_char_attr(row, col);
}

void App::put_char_attr(int row, int col, int char_attr)
{
    m_frame->put_char_attr(row, col, char_attr);
}

void App::set_keyboard_timeout(int ms)
{
    m_frame->set_keyboard_timeout(ms);
}

void App::get_cursor_pos(int &x, int &y) const
{
    m_frame->get_cursor_pos(x, y);
}

void App::pause()
{
    m_frame->Hide();
}

void App::resume()
{
    m_frame->Show();
}

} // namespace id::gui
