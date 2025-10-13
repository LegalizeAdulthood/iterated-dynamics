// SPDX-License-Identifier: GPL-3.0-only
//
#include <gui/TextScreen.h>

#include <algorithm>
#include <cassert>

namespace id::gui
{

static wxColour cga_color_to_wx_color(CGAColor color, const bool intense)
{
    // Standard CGA color palette
    static const wxColour cga_colors[] = {
        wxColour(0, 0, 0),
        wxColour(0, 0, 128),
        wxColour(0, 128, 0),
        wxColour(0, 128, 128),
        wxColour(128, 0, 0),
        wxColour(128, 0, 128),
        wxColour(128, 128, 0),
        wxColour(192, 192, 192),
        //  wxColour(128, 128, 128),  This looks lousy - make it black
        wxColour(0, 0, 0),
        wxColour(0, 0, 255),
        wxColour(0, 255, 0),
        wxColour(0, 255, 255),
        wxColour(255, 0, 0),
        wxColour(255, 0, 255),
        wxColour(255, 255, 0),
        wxColour(255, 255, 255)
    };

    int color_index = static_cast<int>(color);
    if (color_index < 0 || color_index > 15)
    {
        color_index = 7; // Default to light gray
    }

    wxColour result = cga_colors[color_index];

    // For foreground colors, apply intensity
    if (intense && color_index < 8)
    {
        // Intensify the color
        result = cga_colors[color_index + 8];
    }

    return result;
}

enum class MarginIndex
{
    LINE_NUMBER = 0,
    SYMBOLS = 1,
    FOLDING = 2,
};

inline int operator+(MarginIndex value)
{
    return static_cast<int>(value);
}

TextScreen::TextScreen(wxWindow *parent, const wxWindowID id, const wxPoint &pos, const wxSize &size, const long style) :
    wxStyledTextCtrl(parent, id, pos, wxDefaultSize, style), // Always use calculated size
    m_font(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE).FaceName("Consolas"))
{
    InvalidateBestSize();
    clear();
    SetUseHorizontalScrollBar(false);
    SetUseVerticalScrollBar(false);
    SetMarginWidth(+MarginIndex::LINE_NUMBER, 0); // Hide margins
    SetMarginWidth(+MarginIndex::SYMBOLS, 0);
    SetMarginWidth(+MarginIndex::FOLDING, 0);
    SetZoom(0);
    SetTabWidth(1);
    SetIndent(0);
    SetUseTabs(false);
    SetViewEOL(false);
    SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    SetOvertype(true); // Overtype mode for CGA simulation
    SetReadOnly(false);
    SetCaretLineVisible(false);
    SetSelectionMode(wxSTC_SEL_RECTANGLE);
    HideSelection(true);
    if (!m_font.IsOk())
    {
        m_font = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    }
    StyleSetFont(wxSTC_STYLE_DEFAULT, m_font);
    initialize_styles();
    set_cursor_position(0, 0);
    SetCaretStyle(+CursorType::UNDERLINE);
    wxStyledTextCtrl::SetMaxLength(TOTAL_CELLS + SCREEN_HEIGHT - 1); // Include newlines
    m_fixed_size = calculate_fixed_size();
    wxStyledTextCtrl::DoSetSize(pos.x, pos.y, m_fixed_size.x, m_fixed_size.y, wxSIZE_USE_EXISTING);
    wxWindowBase::Enable(false);
}

wxSize TextScreen::calculate_fixed_size()
{
    // Get the character dimensions from the current font
    wxClientDC dc(this);
    dc.SetFont(m_font);

    m_char_size = dc.GetTextExtent("M"); // Use 'M' for consistent character width

    // Calculate the exact size needed for 80x25 characters
    int width = m_char_size.x * SCREEN_WIDTH;
    int height = m_char_size.y * (SCREEN_HEIGHT + 1); // TODO: if SCREEN_HEIGHT is 25, why do we need +1?

    // Add small margins to account for control borders
    constexpr int margin = 4;
    width += margin;
    height += margin;

    return wxSize(width, height);
}

void TextScreen::DoSetSize(const int x, const int y, int width, int height, const int size_flags)
{
    assert(!(m_fixed_size.x == 0 || m_fixed_size.y == 0));
    // Only update position, keep our fixed size
    wxStyledTextCtrl::DoSetSize(x, y, m_fixed_size.x, m_fixed_size.y, size_flags | wxSIZE_FORCE);
}

wxSize TextScreen::DoGetBestSize() const
{
    assert(!(m_fixed_size.x == 0 || m_fixed_size.y == 0));
    return m_fixed_size;
}

wxSize TextScreen::GetMinSize() const
{
    assert(!(m_fixed_size.x == 0 || m_fixed_size.y == 0));
    return m_fixed_size;
}

wxSize TextScreen::GetMaxSize() const
{
    assert(!(m_fixed_size.x == 0 || m_fixed_size.y == 0));
    return m_fixed_size;
}

void TextScreen::invalidate(const int left, const int bot, const int right, const int top)
{
    refresh_display();
    const wxRect exposed{                                             //
        left * m_char_size.GetWidth(), top * m_char_size.GetHeight(), //
        (right + 1) * m_char_size.GetWidth(), (bot + 1) * m_char_size.GetHeight()};
    RefreshRect(exposed);
}

void TextScreen::initialize_styles()
{
    if (m_styles_initialized)
    {
        return;
    }

    // Initialize all 256 possible CGA attribute combinations
    for (int attr = 0; attr < MAX_STYLES; ++attr)
    {
        const CGAColor fg_color = static_cast<CGAColor>(attr & 0x0F);
        const CGAColor bg_color = static_cast<CGAColor>(attr >> 4 & 0x0F);
        const bool intense = (attr & 0x08) != 0;
        const bool blinking = (attr & 0x80) != 0;

        const int style_num = attr;

        // Set foreground and background colors
        StyleSetForeground(style_num, cga_color_to_wx_color(fg_color, intense));
        StyleSetBackground(style_num, cga_color_to_wx_color(bg_color, false));

        // Handle blinking (simulate with bold for now)
        StyleSetBold(style_num, blinking);

        // Set font
        StyleSetFont(style_num, m_font);
    }

    m_styles_initialized = true;
}

void TextScreen::put_string(
    const int x_pos, const int y_pos, const int attr, const char *text, int &end_row, int &end_col)
{
    const char xa = attr & 0x0ff;
    int max_row = y_pos;
    int row = y_pos;
    int max_col = x_pos - 1;
    int col = x_pos - 1;

    int i;
    char xc;
    for (i = 0; (xc = text[i]) != 0; i++)
    {
        if (xc == '\r' || xc == '\n')
        {
            if (row < SCREEN_HEIGHT - 1)
            {
                row++;
            }
            col = x_pos - 1;
        }
        else
        {
            if (++col >= SCREEN_WIDTH)
            {
                if (row < SCREEN_HEIGHT - 1)
                {
                    row++;
                }
                col = x_pos;
            }
            max_row = std::max(max_row, row);
            max_col = std::max(max_col, col);
            m_screen[row][col] = CGACell(xc, xa);
        }
    }
    if (i > 0)
    {
        invalidate(x_pos, y_pos, max_col, max_row);
        end_row = row;
        end_col = col + 1;
    }
}

void TextScreen::scroll_up(const int top, const int bot)
{
    for (int row = top; row < bot; row++)
    {
        m_screen[row] = m_screen[row + 1];
    }
    auto &last{m_screen[bot]};
    std::fill(last.begin(), last.end(), CGACell(' ', 0));
    invalidate(0, bot, SCREEN_WIDTH, top);
}

void TextScreen::put_char(const int row, const int col, const char ch, const unsigned char attr)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    // Update the screen buffer
    m_screen[row][col] = CGACell(ch, attr);

    // Update the display
    update_cell_display(row, col);
}

void TextScreen::put_string(const int row, const int col, const char *str, const unsigned char attr)
{
    if (!str || !is_valid_position(row, col))
    {
        return;
    }

    int current_col = col;
    while (*str && current_col < SCREEN_WIDTH)
    {
        put_char(row, current_col, *str, attr);
        ++str;
        ++current_col;
    }
}

void TextScreen::set_attribute(const int row, const int col, const unsigned char attr, const int count)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    for (int i = 0; i < count && col + i < SCREEN_WIDTH; ++i)
    {
        m_screen[row][col + i].attribute = attr;
    }

    update_region_display(row, col, row, std::min(col + count - 1, SCREEN_WIDTH - 1));
}

void TextScreen::clear(const unsigned char attr)
{
    // Clear the screen buffer
    for (auto &row : m_screen)
    {
        for (auto &cell : row)
        {
            cell = CGACell(' ', attr);
        }
    }

    // Update the entire display
    refresh_display();

    // Reset cursor to top-left
    set_cursor_position(0, 0);
}

void TextScreen::scroll_up(
    const int top_row, const int bottom_row, const int lines, const unsigned char fill_attr)
{
    if (top_row < 0 || bottom_row >= SCREEN_HEIGHT || top_row > bottom_row || lines <= 0)
    {
        return;
    }

    // Move lines up
    for (int src_row = top_row + lines; src_row <= bottom_row; ++src_row)
    {
        int dest_row = src_row - lines;
        if (dest_row >= top_row && dest_row <= bottom_row)
        {
            m_screen[dest_row] = m_screen[src_row];
        }
    }

    // Fill bottom lines with spaces
    for (int row = std::max(bottom_row - lines + 1, top_row); row <= bottom_row; ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            m_screen[row][col] = CGACell(' ', fill_attr);
        }
    }

    update_region_display(top_row, 0, bottom_row, SCREEN_WIDTH - 1);
}

void TextScreen::scroll_down(
    const int top_row, const int bottom_row, const int lines, const unsigned char fill_attr)
{
    if (top_row < 0 || bottom_row >= SCREEN_HEIGHT || top_row > bottom_row || lines <= 0)
    {
        return;
    }

    // Move lines down
    for (int src_row = bottom_row - lines; src_row >= top_row; --src_row)
    {
        int dest_row = src_row + lines;
        if (dest_row >= top_row && dest_row <= bottom_row)
        {
            m_screen[dest_row] = m_screen[src_row];
        }
    }

    // Fill top lines with spaces
    for (int row = top_row; row < std::min(top_row + lines, bottom_row + 1); ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            m_screen[row][col] = CGACell(' ', fill_attr);
        }
    }

    update_region_display(top_row, 0, bottom_row, SCREEN_WIDTH - 1);
}

void TextScreen::set_cursor_position(const int row, const int col)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;

    const int pos = position_from_row_col(row, col);
    SetCurrentPos(pos);
    SetAnchor(pos);
}

void TextScreen::get_cursor_position(int &row, int &col) const
{
    row = m_cursor_row;
    col = m_cursor_col;
}

void TextScreen::show_cursor(const bool show)
{
    m_cursor_visible = show;
    SetCaretStyle(show ? +m_cursor_type : +CursorType::NONE);
}

CGACell TextScreen::get_cell(const int row, const int col) const
{
    if (!is_valid_position(row, col))
    {
        return CGACell();
    }
    return m_screen[row][col];
}

void TextScreen::set_cell(const int row, const int col, const CGACell &cell)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    m_screen[row][col] = cell;
    update_cell_display(row, col);
}

void TextScreen::refresh_display()
{
    std::string content;
    content.reserve(TOTAL_CELLS + SCREEN_HEIGHT - 1);

    for (int row = 0; row < SCREEN_HEIGHT; ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            content += m_screen[row][col].character;
        }
        if (row < SCREEN_HEIGHT - 1)
        {
            content += '\n';
        }
    }

    SetText(content);

    // Apply styling
    for (int row = 0; row < SCREEN_HEIGHT; ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            const int pos = position_from_row_col(row, col);
            const int style = get_style_number(m_screen[row][col].attribute);
            StartStyling(pos);
            SetStyling(1, style);
        }
    }
}

bool TextScreen::is_valid_position(const int row, const int col) const
{
    return row >= 0 && row < SCREEN_HEIGHT && col >= 0 && col < SCREEN_WIDTH;
}

void TextScreen::update_cell_display(const int row, const int col)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    const int pos = position_from_row_col(row, col);
    const char ch = m_screen[row][col].character;

    // Replace the character
    SetTargetStart(pos);
    SetTargetEnd(pos + 1);
    const char text[2]{ch, '\0'};
    ReplaceTarget(wxString(text, wxConvUTF8, 1));

    // Apply styling
    const int style = get_style_number(m_screen[row][col].attribute);
    StartStyling(pos);
    SetStyling(1, style);
}

void TextScreen::update_region_display(
    const int start_row, const int start_col, const int end_row, const int end_col)
{
    for (int row = start_row; row <= end_row; ++row)
    {
        const int col_start = row == start_row ? start_col : 0;
        const int col_end = row == end_row ? end_col : SCREEN_WIDTH - 1;

        for (int col = col_start; col <= col_end; ++col)
        {
            update_cell_display(row, col);
        }
    }
}

int TextScreen::get_style_number(const unsigned char attr) const
{
    return static_cast<int>(attr);
}

int TextScreen::position_from_row_col(const int row, const int col) const
{
    return row * (SCREEN_WIDTH + 1) + col; // +1 for newline characters
}

void TextScreen::row_col_from_position(const int pos, int &row, int &col) const
{
    row = pos / (SCREEN_WIDTH + 1);
    col = pos % (SCREEN_WIDTH + 1);

    // Clamp to valid ranges
    row = std::clamp(row, 0, SCREEN_HEIGHT - 1);
    col = std::clamp(col, 0, SCREEN_WIDTH - 1);
}

} // namespace id::gui
