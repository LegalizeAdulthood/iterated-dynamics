// SPDX-License-Identifier: GPL-3.0-only
//
#include "IdFrame.h"

#include "IdApp.h"

#include <wx/app.h>
#include <wx/wx.h>

#include <array>
#include <cassert>

IdFrame::IdFrame() :
    wxFrame(nullptr, wxID_ANY, wxT("Iterated Dynamics"))
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
}

int IdFrame::get_key_press(bool wait_for_key)
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

void IdFrame::add_key_press(unsigned int key)
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
