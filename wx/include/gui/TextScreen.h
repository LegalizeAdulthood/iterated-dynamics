// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <wx/stc/stc.h>
#include <wx/wx.h>

#include <array>

namespace id::gui
{

// CGA color constants
enum class CGAColor : unsigned char
{
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    BROWN = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};

// CGA character cell structure
struct CGACell
{
    char character = ' ';
    unsigned char attribute = 0x07; // White on black (standard CGA attribute)

    CGACell() = default;

    CGACell(char ch, unsigned char attr) :
        character(ch),
        attribute(attr)
    {
    }

    CGAColor GetForegroundColor() const
    {
        return static_cast<CGAColor>(attribute & 0x0F);
    }

    CGAColor GetBackgroundColor() const
    {
        return static_cast<CGAColor>(attribute >> 4 & 0x0F);
    }

    bool IsBlinking() const
    {
        return (attribute & 0x80) != 0;
    }

    bool IsIntense() const
    {
        return (attribute & 0x08) != 0;
    }
};

class TextScreen : public wxStyledTextCtrl
{
public:
    static constexpr int SCREEN_WIDTH = 80;
    static constexpr int SCREEN_HEIGHT = 25;
    static constexpr int TOTAL_CELLS = SCREEN_WIDTH * SCREEN_HEIGHT;

    TextScreen(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition,
        const wxSize &size = wxDefaultSize, long style = 0);

    ~TextScreen() override = default;

    // CGA screen manipulation methods
    void put_string(int x_pos, int y_pos, int attr, const char *text, int &end_row, int &end_col);
    void scroll_up(int top, int bot);

    void put_char(int row, int col, char ch, unsigned char attr);
    void put_string(int row, int col, const char *str, unsigned char attr);
    void set_attribute(int row, int col, unsigned char attr, int count);
    void clear(unsigned char attr = 0x07);
    void scroll_up(int top_row, int bottom_row, int lines , unsigned char fill_attr);
    void scroll_down(int top_row, int bottom_row, int lines, unsigned char fill_attr);

    // Cursor operations
    void set_cursor_position(int row, int col);
    void get_cursor_position(int &row, int &col) const;
    void show_cursor(bool show = true);
    void set_cursor_type(int type); // 0 = none, 1 = underline, 2 = block

    // Screen buffer access
    CGACell get_cell(int row, int col) const;
    void set_cell(int row, int col, const CGACell &cell);

    // Utility methods
    void refresh_display();
    bool is_valid_position(int row, int col) const;

protected:
    // Override size-related methods to enforce fixed size
    void DoSetSize(int x, int y, int width, int height, int size_flags = wxSIZE_AUTO) override;
    wxSize DoGetBestSize() const override;
    wxSize GetMinSize() const override;
    wxSize GetMaxSize() const override;

private:
    void invalidate(int left, int bot, int right, int top);
    void initialize_styles();
    void update_cell_display(int row, int col);
    void update_region_display(int start_row, int start_col, int end_row, int end_col);
    int get_style_number(unsigned char attr) const;
    int position_from_row_col(int row, int col) const;
    void row_col_from_position(int pos, int &row, int &col) const;
    wxSize calculate_fixed_size();

    wxFont m_font{};
    wxSize m_char_size{};

    // Screen buffer - 80x25 character cells
    std::array<std::array<CGACell, SCREEN_WIDTH>, SCREEN_HEIGHT> m_screen;

    // Cursor state
    int m_cursor_row{0};
    int m_cursor_col{0};
    bool m_cursor_visible{true};
    int m_cursor_type{1}; // 1 = underline

    // Style mappings for different CGA attribute combinations
    static constexpr int MAX_STYLES = 256; // All possible CGA attribute combinations
    bool m_styles_initialized{false};

    // Fixed size for 80x25 display
    mutable wxSize m_fixed_size{};
};

} // namespace id::gui
