#pragma once

#include "gui/Screen.h"

#include <wx/frame.h>

#include <array>

namespace id::gui
{

class Plot;
class TextScreen;

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
    void set_plot_size(int width, int height);
    void set_for_text();
    void set_for_graphics();
    void clear();
    void put_string(int row, int col, int attr, const char *msg, int &end_row, int &end_col);
    void set_attr(int row, int col, int attr, int count);
    void hide_text_cursor();
    void scroll_up(int top, int bot);
    void move_cursor(int row, int col);
    void flush();
    Screen get_screen() const;
    void set_screen(const Screen & screen);

    bool is_text() const
    {
        return m_text_not_graphics;
    }

protected:
    // Override DoGetBestSize to return the maximum size needed for either control
    wxSize DoGetBestSize() const override;

private:
    enum
    {
        KEY_BUF_MAX = 80,
    };

    void on_key_down(wxKeyEvent &event);
    void on_char(wxKeyEvent &event);
    void add_key_press(unsigned int key);
    bool key_buffer_full() const
    {
        return m_key_press_count >= KEY_BUF_MAX;
    }
    wxSize get_client_size() const;

    bool m_timed_out{};

    /* key_buffer
    *
    * When we peeked ahead and saw a keypress, stash it here for later
    * feeding to our caller.
    */
    mutable int m_key_buffer{};

    // the keypress buffer
    unsigned int m_key_press_count{};
    unsigned int m_key_press_head{};
    unsigned int m_key_press_tail{};
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
    Plot *m_plot{};
    TextScreen *m_text_screen{};
    bool m_text_not_graphics{true};
};

} // namespace id::gui
