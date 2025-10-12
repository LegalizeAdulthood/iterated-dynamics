// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/IdFrame.h>

#include <gui/IdApp.h>
#include <gui/Plot.h>
#include <gui/TextScreen.h>

#include <wx/app.h>
#include <wx/wx.h>

#include <algorithm>
#include <array>
#include <cassert>

namespace id::gui
{

const wxSize ARBITRARY_DEFAULT_PLOT_SIZE{640, 480};

IdFrame::IdFrame() :
    wxFrame(nullptr, wxID_ANY, wxT("Iterated Dynamics")),
    m_plot(new Plot(this, wxID_ANY, wxDefaultPosition, ARBITRARY_DEFAULT_PLOT_SIZE)),
    m_text_screen(new TextScreen(this))
{
    wxMenu *file = new wxMenu;
    file->Append(wxID_EXIT);
    wxMenu *help = new wxMenu;
    help->Append(wxID_ABOUT);
    wxMenuBar *bar = new wxMenuBar;
    bar->Append(file, "&File");
    bar->Append(help, "&Help");
    wxFrameBase::SetMenuBar(bar);
    wxFrameBase::CreateStatusBar();

    Bind(wxEVT_MENU, &IdFrame::on_about, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &IdFrame::on_exit, this, wxID_EXIT);
    Bind(wxEVT_CHAR, &IdFrame::on_char, this, wxID_ANY);

    SetClientSize(get_client_size());
    m_plot->Hide();
    m_text_screen->Hide();
}

wxSize IdFrame::get_client_size() const
{
    const wxSize plot_size = m_plot->GetBestSize();
    const wxSize text_size = m_text_screen->GetBestSize();
    return {std::max(plot_size.GetWidth(), text_size.GetWidth()),
        std::max(plot_size.GetHeight(), text_size.GetHeight())};
}

wxSize IdFrame::DoGetBestSize() const
{
    return get_client_size();
}

int IdFrame::get_key_press(const bool wait_for_key)
{
    wxGetApp().pump_messages(wait_for_key);
    if (wait_for_key && m_timed_out)
    {
        return 0;
    }

    if (m_key_press_count == 0)
    {
        assert(!wait_for_key);
        return 0;
    }

    const int i = m_key_press_buffer[m_key_press_tail];

    if (++m_key_press_tail >= KEY_BUF_MAX)
    {
        m_key_press_tail = 0;
    }
    m_key_press_count--;

    return i;
}

void IdFrame::set_plot_size(int width, int height)
{
    m_plot->SetSize(width, height);
    SetClientSize(get_client_size());
}

void IdFrame::set_for_text()
{
    m_plot->Show(false);
    m_text_screen->Show(true);
}

void IdFrame::set_for_graphics()
{
    m_plot->Show(true);
    m_text_screen->Show(false);
}

void IdFrame::clear()
{
    if (m_text_not_graphics)
    {
        m_text_screen->clear();
    }
    else
    {
        m_plot->clear();
    }
}

void IdFrame::put_string(int row, int col, int attr, const char *msg, int &end_row, int &end_col)
{
    m_text_screen->put_string(col, row, attr, msg, end_row, end_col);
}

void IdFrame::set_attr(int row, int col, int attr, int count)
{
    m_text_screen->set_attribute(row, col, attr, count);
}

void IdFrame::hide_text_cursor()
{
    m_text_screen->show_cursor(false);
}

int IdFrame::key_pressed() const
{
    return 0;
    throw std::runtime_error("not implemented");

    // if (m_key_buffer)
    //{
    //     return m_key_buffer;
    // }
    // flush_output();
    // const int ch = handle_special_keys(g_frame.get_key_press(false));
    // if (m_key_buffer)
    //{
    //     return m_key_buffer;
    // }
    // m_key_buffer = ch;

    // return ch;
}

void IdFrame::scroll_up(int top, int bot)
{
    m_text_screen->scroll_up(top, bot);
}

void IdFrame::on_exit(wxCommandEvent &event)
{
    Close(true);
}

void IdFrame::on_about(wxCommandEvent &event)
{
    wxMessageBox("Iterated Dynamics 2.0", "About Iterated Dynamics", wxOK | wxICON_INFORMATION);
}

void IdFrame::on_char(wxKeyEvent &event)
{
    int key = event.GetKeyCode();
}

void IdFrame::add_key_press(const unsigned int key)
{
    if (key_buffer_full())
    {
        assert(m_key_press_count < KEY_BUF_MAX);
        // no room
        return;
    }

    m_key_press_buffer[m_key_press_head] = key;
    if (++m_key_press_head >= KEY_BUF_MAX)
    {
        m_key_press_head = 0;
    }
    m_key_press_count++;
}

} // namespace id::gui
