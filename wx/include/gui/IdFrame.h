#pragma once

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

protected:
    // Override DoGetBestSize to return the maximum size needed for either control
    wxSize DoGetBestSize() const override;

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
    wxSize get_client_size() const;

    bool m_timed_out{};

    // the keypress buffer
    unsigned int m_key_press_count{};
    unsigned int m_key_press_head{};
    unsigned int m_key_press_tail{};
    std::array<int, KEY_BUF_MAX> m_key_press_buffer{};
    Plot *m_plot{};
    TextScreen *m_text_screen{};
};

} // namespace id::gui
