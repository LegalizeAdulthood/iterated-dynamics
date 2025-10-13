// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/IdApp.h>

#include "ui/id_main.h"

#include <gui/IdFrame.h>
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

wxIMPLEMENT_APP(id::gui::IdApp);

namespace id::gui
{

bool IdApp::OnInit()
{
#ifdef WIN32
    g_instance = wxGetInstance();
#endif

    return true;
}

int IdApp::OnRun()
{
    m_mainLoop = CreateMainLoop();
    return id_main(argc, argv);
}

void IdApp::create_window(const int width, const int height)
{
    m_frame = new IdFrame();
    m_frame->set_plot_size(width, height);
    m_frame->Show(true);
}

void IdApp::pump_messages(bool wait_flag)
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

bool IdApp::is_text() const
{
    return m_frame->is_text();
}

void IdApp::set_for_text()
{
    m_frame->set_for_text();
}

void IdApp::set_for_graphics()
{
    m_frame->set_for_graphics();
}

void IdApp::clear()
{
    m_frame->clear();
}

void IdApp::put_string(int col, int row, int attr, const char *msg, int &end_row, int &end_col)
{
    m_frame->put_string(row, col, attr, msg, end_row, end_col);
}

void IdApp::set_attr(int row, int col, int attr, int count)
{
    m_frame->set_attr(row, col, attr, count);
}

void IdApp::hide_text_cursor()
{
    m_frame->hide_text_cursor();
}

void IdApp::scroll_up(int top, int bot)
{
    m_frame->scroll_up(top, bot);
}

void IdApp::move_cursor(int row, int col)
{
    m_frame->move_cursor(row, col);
}

int IdApp::get_key_press(bool wait_for_key)
{
    return m_frame->get_key_press(wait_for_key);
}

void IdApp::flush()
{
    m_frame->flush();
}

Screen IdApp::get_screen() const
{
    return m_frame->get_screen();
}

void IdApp::set_screen(const Screen &screen)
{
    m_frame->set_screen(screen);
}

int IdApp::get_char_attr(int row, int col)
{
    return m_frame->get_char_attr(row, col);
}

void IdApp::put_char_attr(int row, int col, int char_attr)
{
    m_frame->put_char_attr(row, col, char_attr);
}

void IdApp::set_keyboard_timeout(int ms)
{
    m_frame->set_keyboard_timeout(ms);
}

void IdApp::get_cursor_pos(int &x, int &y) const
{
    m_frame->get_cursor_pos(x, y);
}

void IdApp::pause()
{
    m_frame->Hide();
}

void IdApp::resume()
{
    m_frame->Show();
}

} // namespace id::gui
