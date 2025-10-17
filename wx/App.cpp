// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/App.h>

#include "io/CurrentPathSaver.h"
#include "ui/id_main.h"

#include <gui/Frame.h>
#include "../win32/instance.h"

#include <fmt/format.h>

#include <wx/wx.h>
#include <wx/evtloop.h>
#ifdef WIN32
#include <wx/msw/private.h>
#endif

#include <array>
#include <cassert>
#include <filesystem>

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
    m_frame->pump_messages(GetMainLoop(), wait_flag);
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

Colormap App::read_palette()
{
    return m_frame->read_palette();
}

void App::write_palette(const Colormap &map)
{
    m_frame->write_palette(map);
}

bool App::resize(int width, int height)
{
    return m_frame->resize(width, height);
}

void App::write_pixel(int x, int y, int color)
{
    m_frame->write_pixel(x, y, color);
}

int App::read_pixel(int x, int y)
{
    return m_frame->read_pixel(x, y);
}

void App::display_string(int x, int y, int fg, int bg, const char *text)
{
    m_frame->display_string(x, y, fg, bg, text);
}

void App::save_graphics()
{
    m_frame->save_graphics();
}

void App::restore_graphics()
{
    m_frame->restore_graphics();
}

bool App::get_filename(
    const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename)
{
    io::CurrentPathSaver saved_current;
    std::filesystem::path path{result_filename};
    if (path.has_filename())
    {
        if (path.is_relative())
        {
            path = std::filesystem::current_path() / path;
        }
    }
    else
    {
        path.clear();
    }
    wxFileDialog dlg(m_frame,                                            // parent
        hdg,                                                             // message
        wxEmptyString,                                                   // defaultDir
        path.string(),                                                   // defaultFile
        fmt::format("{0:s} Files ({1:s})|{1:s}|All files ({2:s})|{2:s}", // wildcard
            type_desc, type_wildcard, wxFileSelectorDefaultWildcardStr),
        wxFD_DEFAULT_STYLE);                                             // style,
    if (dlg.ShowModal() == wxID_OK)
    {
        result_filename = dlg.GetPath().ToStdString();
        return false;
    }
    return true;
}

} // namespace id::gui
