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
#include <string>

#undef ID_DEBUG_KEYSTROKES
#ifdef ID_DEBUG_KEYSTROKES
#include <misc/Driver.h>

static void debug_key_strokes(const std::string &text)
{
    driver_debug_line(text);
}
#else
static void debug_key_strokes(const std::string &text)
{
}
#endif

namespace id::gui
{

const wxSize ARBITRARY_DEFAULT_PLOT_SIZE{640, 480};

IdFrame::IdFrame() :
    wxFrame(nullptr, wxID_ANY, wxT("Iterated Dynamics")),
    m_plot(new Plot(this, wxID_ANY, wxDefaultPosition, ARBITRARY_DEFAULT_PLOT_SIZE)),
    m_text_screen(new TextScreen(this))
{
    Bind(wxEVT_KEY_DOWN, &IdFrame::on_key_down, this, wxID_ANY);
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
    int wx_key;
    int id_key;
};

} // namespace

// clang-format off
static WxKeyToIdKey s_key_map[]{
    {WXK_NUMPAD_ENTER,  ui::ID_KEY_CTL_ENTER_2},
    {WXK_HOME,          ui::ID_KEY_HOME},
    {WXK_UP,            ui::ID_KEY_UP_ARROW},
    {WXK_PAGEUP,        ui::ID_KEY_PAGE_UP},
    {WXK_LEFT,          ui::ID_KEY_LEFT_ARROW},
    {WXK_NUMPAD5,       ui::ID_KEY_KEYPAD_5},
    {WXK_RIGHT,         ui::ID_KEY_RIGHT_ARROW},
    {WXK_END,           ui::ID_KEY_END},
    {WXK_DOWN,          ui::ID_KEY_DOWN_ARROW},
    {WXK_PAGEDOWN,      ui::ID_KEY_PAGE_DOWN},
    {WXK_INSERT,        ui::ID_KEY_INSERT},
    {WXK_DELETE,        ui::ID_KEY_DELETE},
};

static WxKeyToIdKey s_shift_key_map[]{
    {'\t',              ui::ID_KEY_SHF_TAB},
    {'A',               ui::ID_KEY_ALT_A},
    {'S',               ui::ID_KEY_ALT_S},
};

static WxKeyToIdKey s_control_key_map[]{
    {WXK_LEFT,          ui::ID_KEY_CTL_LEFT_ARROW},
    {WXK_RIGHT,         ui::ID_KEY_CTL_RIGHT_ARROW},
    {WXK_END,           ui::ID_KEY_CTL_END},
    {WXK_PAGEDOWN,      ui::ID_KEY_CTL_PAGE_DOWN},
    {WXK_HOME,          ui::ID_KEY_CTL_HOME},
    {WXK_PAGEUP,        ui::ID_KEY_CTL_PAGE_UP},
    {WXK_UP,            ui::ID_KEY_CTL_UP_ARROW},
    {'-',               ui::ID_KEY_CTL_MINUS},
    {WXK_NUMPAD5,       ui::ID_KEY_CTL_KEYPAD_5},
    {'+',               ui::ID_KEY_CTL_PLUS},
    {WXK_DOWN,          ui::ID_KEY_CTL_DOWN_ARROW},
    {WXK_INSERT,        ui::ID_KEY_CTL_INSERT},
    {WXK_DELETE,        ui::ID_KEY_CTL_DEL},
    {WXK_TAB,           ui::ID_KEY_CTL_TAB},
};

static WxKeyToIdKey s_alt_key_map[]{
    {'A',               ui::ID_KEY_ALT_A},
    {'S',               ui::ID_KEY_ALT_S},
    {'1',               ui::ID_KEY_ALT_1},
    {'2',               ui::ID_KEY_ALT_2},
    {'3',               ui::ID_KEY_ALT_3},
    {'4',               ui::ID_KEY_ALT_4},
    {'5',               ui::ID_KEY_ALT_5},
    {'6',               ui::ID_KEY_ALT_6},
    {'7',               ui::ID_KEY_ALT_7},
};
// clang-format on

static bool has_modifier(const wxKeyEvent &event, const int modifier)
{
    return (event.GetModifiers() & modifier) != 0;
}

void IdFrame::on_key_down(wxKeyEvent &event)
{
    const bool shift = has_modifier(event, wxMOD_SHIFT);
    const bool alt = has_modifier(event, wxMOD_ALT);
    const bool ctrl = has_modifier(event, wxMOD_CONTROL);
    const int key = event.GetKeyCode();
    const auto find_key = [key](const WxKeyToIdKey *begin, const WxKeyToIdKey *end)
    { return std::find_if(begin, end, [key](const WxKeyToIdKey &entry) { return entry.wx_key == key; }); };
    if ((key & 0x7F) == key)
    {
    }
    else if (key >= WXK_F1 && key <= WXK_F10)
    {
        const int offset = key - WXK_F1;
        if (shift)
        {
            add_key_press(ui::ID_KEY_SHF_F1 + offset);
        }
        else if (ctrl)
        {
            add_key_press(ui::ID_KEY_CTL_F1 + offset);
        }
        else if (alt)
        {
            add_key_press(ui::ID_KEY_ALT_F1 + offset);
        }
        else
        {
            add_key_press(ui::ID_KEY_F1 + offset);
        }
    }
    else if (!(shift || alt || ctrl))
    {
        if (const auto it = find_key(std::begin(s_key_map), std::end(s_key_map)); it != std::end(s_key_map))
        {
            add_key_press(it->id_key);
        }
    }
    else if (shift)
    {
        if (const auto it = find_key(std::begin(s_shift_key_map), std::end(s_shift_key_map));
            it != std::end(s_shift_key_map))
        {
            add_key_press(it->id_key);
        }
    }
    else if (ctrl)
    {
        if (const auto it = find_key(std::begin(s_control_key_map), std::end(s_control_key_map));
            it != std::end(s_control_key_map))
        {
            add_key_press(it->id_key);
        }
    }
    else if (alt)
    {
        if (const auto it = find_key(std::begin(s_alt_key_map), std::end(s_alt_key_map));
            it != std::end(s_alt_key_map))
        {
            add_key_press(it->id_key);
        }
    }
}

void IdFrame::on_char(wxKeyEvent &event)
{
    int key = event.GetKeyCode();
    assert((key & 0x7F) == key);
    // an ASCII key has been pressed
    if (key == '\t' && has_modifier(event, wxMOD_SHIFT))
    {
        key = ui::ID_KEY_SHF_TAB;
    }
    add_key_press(key);
    debug_key_strokes("OnChar " + std::to_string(key));
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
