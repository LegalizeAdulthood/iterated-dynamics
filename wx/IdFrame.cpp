// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/IdFrame.h>

#include <gui/IdApp.h>
#include <gui/Plot.h>
#include <gui/TextScreen.h>

#include <ui/id_keys.h>

#include <wx/app.h>
#include <wx/wx.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>

namespace id::gui
{

const wxSize ARBITRARY_DEFAULT_PLOT_SIZE{640, 480};

IdFrame::IdFrame() :
    wxFrame(nullptr, wxID_ANY, wxT("Iterated Dynamics")),
    m_plot(new Plot(this, wxID_ANY, wxDefaultPosition, ARBITRARY_DEFAULT_PLOT_SIZE)),
    m_text_screen(new TextScreen(this))
{
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

void IdFrame::scroll_up(int top, int bot)
{
    m_text_screen->scroll_up(top, bot);
}

void IdFrame::move_cursor(int row, int col, int cursor_type)
{
    m_text_screen->set_cursor_position(row, col);
    m_text_screen->set_cursor_type(cursor_type);
}

void IdFrame::flush()
{
    m_plot->flush();
}

namespace
{

struct WxKeyToIdKey
{
    wxKeyCode wx_key;
    int id_key;
};

}

static WxKeyToIdKey s_key_map[]{
    {WXK_NUMPAD_ENTER, ui::ID_KEY_CTL_ENTER_2} //
};

void IdFrame::on_char(wxKeyEvent &event)
{
    int key = event.GetKeyCode();
    if ((key & 0x7F) == key)
    {
        add_key_press(key);
    }
    else if (const auto it = std::find_if(std::begin(s_key_map), std::end(s_key_map),
                 [key](const WxKeyToIdKey &entry) { return entry.wx_key == key; });
        it != std::end(s_key_map))
    {
        add_key_press(it->id_key);
    }
    else
    {
        assert("Unhandled key press" == nullptr);
    }
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
