// SPDX-License-Identifier: GPL-3.0-only
//
#include "TextScreen.h"

#include <algorithm>
#include <cassert>

namespace ui
{

static wxColour cga_color_to_wx_color(CGAColor color, bool intense)
{
    // Standard CGA color palette
    static const wxColour cga_colors[] = {
        wxColour(0, 0, 0),       // BLACK
        wxColour(0, 0, 170),     // BLUE
        wxColour(0, 170, 0),     // GREEN
        wxColour(0, 170, 170),   // CYAN
        wxColour(170, 0, 0),     // RED
        wxColour(170, 0, 170),   // MAGENTA
        wxColour(170, 85, 0),    // BROWN
        wxColour(170, 170, 170), // LIGHT_GRAY
        wxColour(85, 85, 85),    // DARK_GRAY
        wxColour(85, 85, 255),   // LIGHT_BLUE
        wxColour(85, 255, 85),   // LIGHT_GREEN
        wxColour(85, 255, 255),  // LIGHT_CYAN
        wxColour(255, 85, 85),   // LIGHT_RED
        wxColour(255, 85, 255),  // LIGHT_MAGENTA
        wxColour(255, 255, 85),  // YELLOW
        wxColour(255, 255, 255)  // WHITE
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

TextScreen::TextScreen(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style) :
    wxStyledTextCtrl(parent, id, pos, wxDefaultSize, style), // Always use calculated size
    m_font(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE).FaceName("Consolas"))
{
    InvalidateBestSize();

    // Initialize the screen buffer
    clear();

    // Configure the styled text control for fixed-width display
    SetUseHorizontalScrollBar(false);
    SetUseVerticalScrollBar(false);
    SetMarginWidth(0, 0); // Hide line number margin
    SetMarginWidth(1, 0); // Hide symbol margin
    SetMarginWidth(2, 0); // Hide fold margin

    // Set up for 80x25 display
    SetZoom(0);
    SetTabWidth(1);
    SetIndent(0);
    SetUseTabs(false);
    SetViewEOL(false);
    SetViewWhiteSpace(wxSTC_WS_INVISIBLE);
    SetOvertype(true); // Overtype mode for CGA simulation

    // Disable features not needed for CGA simulation
    SetReadOnly(false);
    SetCaretLineVisible(false);
    SetSelectionMode(wxSTC_SEL_RECTANGLE);

    // Set monospace font similar to CGA
    if (!m_font.IsOk())
    {
        m_font = wxFont(wxFontInfo(12).Family(wxFONTFAMILY_MODERN));
    }
    StyleSetFont(wxSTC_STYLE_DEFAULT, m_font);

    // Initialize CGA color styles
    initialize_styles();

    // Set initial content to 80x25 spaces
    std::string initial_content;
    for (int row = 0; row < SCREEN_HEIGHT; ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            initial_content += ' ';
        }
        if (row < SCREEN_HEIGHT - 1)
        {
            initial_content += '\n';
        }
    }
    SetText(initial_content);

    // Position cursor at top-left
    set_cursor_position(0, 0);

    // Prevent user from changing the size
    wxStyledTextCtrl::SetMaxLength(TOTAL_CELLS + SCREEN_HEIGHT - 1); // Include newlines

    // Calculate and set the fixed size
    m_fixed_size = calculate_fixed_size();
    wxStyledTextCtrl::DoSetSize(pos.x, pos.y, m_fixed_size.x, m_fixed_size.y, wxSIZE_USE_EXISTING);
}

wxSize TextScreen::calculate_fixed_size() const
{
    // Get the character dimensions from the current font
    wxClientDC dc(const_cast<TextScreen *>(this));
    dc.SetFont(m_font);

    wxSize char_size = dc.GetTextExtent("M"); // Use 'M' for consistent character width

    // Calculate the exact size needed for 80x25 characters
    int width = char_size.x * SCREEN_WIDTH;
    int height = char_size.y * SCREEN_HEIGHT;

    // Add small margins to account for control borders
    const int margin = 4;
    width += margin;
    height += margin;

    return wxSize(width, height);
}

void TextScreen::DoSetSize(int x, int y, int width, int height, int size_flags)
{
    // Ignore any size changes and use our fixed size
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_fixed_size();
    }

    // Only update position, keep our fixed size
    wxStyledTextCtrl::DoSetSize(x, y, m_fixed_size.x, m_fixed_size.y, size_flags | wxSIZE_FORCE);
}

wxSize TextScreen::DoGetBestSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_fixed_size();
    }

    return m_fixed_size;
}

wxSize TextScreen::GetMinSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_fixed_size();
    }

    return m_fixed_size;
}

wxSize TextScreen::GetMaxSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_fixed_size();
    }

    return m_fixed_size;
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
        CGAColor fg_color = static_cast<CGAColor>(attr & 0x0F);
        CGAColor bg_color = static_cast<CGAColor>((attr >> 4) & 0x0F);
        bool intense = (attr & 0x08) != 0;
        bool blinking = (attr & 0x80) != 0;

        int style_num = attr;

        // Set foreground and background colors
        StyleSetForeground(style_num, cga_color_to_wx_color(fg_color, intense));
        StyleSetBackground(style_num, cga_color_to_wx_color(bg_color, false));

        // Handle blinking (simulate with bold for now)
        StyleSetBold(style_num, blinking);

        // Set font
        StyleSetFont(style_num, StyleGetFont(wxSTC_STYLE_DEFAULT));
    }

    m_styles_initialized = true;
}

void TextScreen::put_char(int row, int col, char ch, unsigned char attr)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    // Update the screen buffer
    m_screen_buffer[row][col] = CGACell(ch, attr);

    // Update the display
    update_cell_display(row, col);
}

void TextScreen::put_string(int row, int col, const char *str, unsigned char attr)
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

void TextScreen::set_attribute(int row, int col, unsigned char attr, int count)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    for (int i = 0; i < count && (col + i) < SCREEN_WIDTH; ++i)
    {
        m_screen_buffer[row][col + i].attribute = attr;
    }

    update_region_display(row, col, row, std::min(col + count - 1, SCREEN_WIDTH - 1));
}

void TextScreen::clear(unsigned char attr)
{
    // Clear the screen buffer
    for (auto &row : m_screen_buffer)
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

void TextScreen::scroll_up(int top_row, int bottom_row, int lines, unsigned char fill_attr)
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
            m_screen_buffer[dest_row] = m_screen_buffer[src_row];
        }
    }

    // Fill bottom lines with spaces
    for (int row = std::max(bottom_row - lines + 1, top_row); row <= bottom_row; ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            m_screen_buffer[row][col] = CGACell(' ', fill_attr);
        }
    }

    update_region_display(top_row, 0, bottom_row, SCREEN_WIDTH - 1);
}

void TextScreen::scroll_down(int top_row, int bottom_row, int lines, unsigned char fill_attr)
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
            m_screen_buffer[dest_row] = m_screen_buffer[src_row];
        }
    }

    // Fill top lines with spaces
    for (int row = top_row; row < std::min(top_row + lines, bottom_row + 1); ++row)
    {
        for (int col = 0; col < SCREEN_WIDTH; ++col)
        {
            m_screen_buffer[row][col] = CGACell(' ', fill_attr);
        }
    }

    update_region_display(top_row, 0, bottom_row, SCREEN_WIDTH - 1);
}

void TextScreen::set_cursor_position(int row, int col)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;

    int pos = position_from_row_col(row, col);
    SetCurrentPos(pos);
    SetAnchor(pos);
}

void TextScreen::get_cursor_position(int &row, int &col) const
{
    row = m_cursor_row;
    col = m_cursor_col;
}

void TextScreen::show_cursor(bool show)
{
    m_cursor_visible = show;
    SetCaretWidth(show ? (m_cursor_type == 2 ? 8 : 1) : 0);
}

void TextScreen::set_cursor_type(int type)
{
    m_cursor_type = type;
    if (m_cursor_visible)
    {
        SetCaretWidth(type == 2 ? 8 : (type == 1 ? 1 : 0));
    }
}

CGACell TextScreen::get_cell(int row, int col) const
{
    if (!is_valid_position(row, col))
    {
        return CGACell();
    }
    return m_screen_buffer[row][col];
}

void TextScreen::set_cell(int row, int col, const CGACell &cell)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    m_screen_buffer[row][col] = cell;
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
            content += m_screen_buffer[row][col].character;
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
            int pos = position_from_row_col(row, col);
            int style = get_style_number(m_screen_buffer[row][col].attribute);
            StartStyling(pos);
            SetStyling(1, style);
        }
    }
}

bool TextScreen::is_valid_position(int row, int col) const
{
    return row >= 0 && row < SCREEN_HEIGHT && col >= 0 && col < SCREEN_WIDTH;
}

void TextScreen::update_cell_display(int row, int col)
{
    if (!is_valid_position(row, col))
    {
        return;
    }

    int pos = position_from_row_col(row, col);
    char ch = m_screen_buffer[row][col].character;

    // Replace the character
    SetTargetStart(pos);
    SetTargetEnd(pos + 1);
    const char text[2]{ch, '\0'};
    ReplaceTarget(wxString(text, wxConvUTF8, 1));

    // Apply styling
    int style = get_style_number(m_screen_buffer[row][col].attribute);
    StartStyling(pos);
    SetStyling(1, style);
}

void TextScreen::update_region_display(int start_row, int start_col, int end_row, int end_col)
{
    for (int row = start_row; row <= end_row; ++row)
    {
        int col_start = (row == start_row) ? start_col : 0;
        int col_end = (row == end_row) ? end_col : SCREEN_WIDTH - 1;

        for (int col = col_start; col <= col_end; ++col)
        {
            update_cell_display(row, col);
        }
    }
}

int TextScreen::get_style_number(unsigned char attr) const
{
    return static_cast<int>(attr);
}

int TextScreen::position_from_row_col(int row, int col) const
{
    return row * (SCREEN_WIDTH + 1) + col; // +1 for newline characters
}

void TextScreen::row_col_from_position(int pos, int &row, int &col) const
{
    row = pos / (SCREEN_WIDTH + 1);
    col = pos % (SCREEN_WIDTH + 1);

    // Clamp to valid ranges
    row = std::clamp(row, 0, SCREEN_HEIGHT - 1);
    col = std::clamp(col, 0, SCREEN_WIDTH - 1);
}

} // namespace ui
