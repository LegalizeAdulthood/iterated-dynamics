// SPDX-License-Identifier: GPL-3.0-only
//
#include "goodbye.h"

#include <wx/wx.h>
#include <wx/evtloop.h>

#include <array>
#include <cassert>

class IdApp : public wxApp
{
public:
    ~IdApp() override = default;

    bool OnInit() override;
    void pump_messages(bool wait_flag);
};

class IdFrame : public wxFrame
{
public:
    IdFrame();
    IdFrame(const IdFrame &rhs) = delete;
    IdFrame(IdFrame &&rhs) = delete;
    ~IdFrame() override = default;
    IdFrame &operator=(const IdFrame &rhs) = delete;
    IdFrame &operator=(IdFrame &&rhs) = delete;

    int get_key_press(bool wait_for_key);

private:
    enum
    {
        KEY_BUF_MAX = 80,
    };

    void on_exit(wxCommandEvent &event);
    void on_about(wxCommandEvent &event);
    void on_char(wxKeyEvent &event);
    void add_key_press(unsigned int key);
    bool key_buffer_full() const
    {
        return m_key_press_count >= KEY_BUF_MAX;
    }

    bool m_timed_out{};

    // the keypress buffer
    unsigned int m_key_press_count{};
    unsigned int m_key_press_head{};
    unsigned int m_key_press_tail{};
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
};

wxIMPLEMENT_APP(IdApp);

bool IdApp::OnInit()
{
    IdFrame *frame = new IdFrame();
    frame->Show(true);

    // start a thread to call id_main here?
    
    return true;
}

void IdApp::pump_messages(bool wait_flag)
{
    wxEventLoopBase *loop = GetMainLoop();
    while (loop->Pending())
    {
        if (!loop->Dispatch())
        {
            return;
        }
    }
}

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
        _ASSERTE(!wait_for_key);
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
