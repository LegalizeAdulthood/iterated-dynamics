#include "TextScreen.h"

#include <wx/stc/stc.h>
#include <wx/wx.h>

namespace formula
{

enum class Syntax
{
    NONE,
    COMMENT,
    KEYWORD,
    WHITESPACE,
    FUNCTION,
    IDENTIFIER
};

inline int operator+(Syntax value)
{
    return static_cast<int>(value);
}

} // namespace formula

enum class MarginIndex
{
    LINE_NUMBER = 0,
    FOLDING = 2,
};

inline int operator+(MarginIndex value)
{
    return static_cast<int>(value);
}

class TestTextScreenApp : public wxApp
{
public:
    bool OnInit() override;
};

class TextScreenFrame : public wxFrame
{
public:
    TextScreenFrame(const wxString &title);

private:
    void set_style_font_color(formula::Syntax style, const wxFont &font, const char *color_name);
    void init_coloring();
    void init_line_numbers();
    void show_hide_line_numbers();
    void on_view_line_numbers(wxCommandEvent &event);
    void on_action_load(wxCommandEvent &event);
    void on_margin_click(wxStyledTextEvent &event);
    void on_exit(wxCommandEvent &event);

    wxMenuItem *m_view_lines{};
    ui::TextScreen *m_text_screen{};
    int m_line_margin_width{};
    bool m_show_lines{true};
};

wxIMPLEMENT_APP(TestTextScreenApp);

bool TestTextScreenApp::OnInit()
{
    TextScreenFrame *frame = new TextScreenFrame("Scintilla Editing Example");
    frame->Show(true);
    return true;
}

TextScreenFrame::TextScreenFrame(const wxString &title) :
    wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
    m_text_screen(new ui::TextScreen(this, wxID_ANY))
{
    wxMenuBar *menu_bar = new wxMenuBar;
    wxMenu *file = new wxMenu;
    file->Append(wxID_EXIT, "&Quit\tAlt-F4", "Quit");
    menu_bar->Append(file, "&File");
    wxMenu *view = new wxMenu;
    m_view_lines = view->Append(wxID_ANY, "&Line Numbers", "Line Numbers", wxITEM_CHECK);
    Bind(wxEVT_MENU, &TextScreenFrame::on_view_line_numbers, this, m_view_lines->GetId());
    menu_bar->Append(view, "&View");
    wxMenu *action = new wxMenu;
    wxMenuItem *load = action->Append(wxID_ANY, "&Load Screen");
    Bind(wxEVT_MENU, &TextScreenFrame::on_action_load, this, load->GetId());
    menu_bar->Append(action, "&Action");
    wxFrameBase::SetMenuBar(menu_bar);
    Bind(wxEVT_MENU, &TextScreenFrame::on_exit, this, wxID_EXIT);

    init_coloring();
    init_line_numbers();
}

void TextScreenFrame::set_style_font_color(formula::Syntax style, const wxFont &font, const char *color_name)
{
    m_text_screen->StyleSetFont(+style, font);
    wxColour color;
    wxASSERT(wxFromString(color_name, &color));
    m_text_screen->StyleSetForeground(+style, color);
}

void TextScreenFrame::init_coloring()
{
    wxFont typewriter;
    typewriter.SetFamily(wxFONTFAMILY_TELETYPE);
    typewriter.SetPointSize(12);
    set_style_font_color(formula::Syntax::NONE, typewriter, "black");
    set_style_font_color(formula::Syntax::COMMENT, typewriter, "forest green");
    set_style_font_color(formula::Syntax::KEYWORD, typewriter, "blue");
    set_style_font_color(formula::Syntax::WHITESPACE, typewriter, "black");
    set_style_font_color(formula::Syntax::FUNCTION, typewriter, "red");
    set_style_font_color(formula::Syntax::IDENTIFIER, typewriter, "purple");
    m_text_screen->Colourise(0, -1);
}

void TextScreenFrame::init_line_numbers()
{
    m_text_screen->SetMarginType(+MarginIndex::LINE_NUMBER, wxSTC_MARGIN_NUMBER);
    m_line_margin_width = m_text_screen->TextWidth(wxSTC_STYLE_LINENUMBER, "_99999");
    show_hide_line_numbers();
}

void TextScreenFrame::show_hide_line_numbers()
{
    m_text_screen->SetMarginWidth(+MarginIndex::LINE_NUMBER, m_show_lines ? m_line_margin_width : 0);
    m_view_lines->Check(m_show_lines);
}

void TextScreenFrame::on_view_line_numbers(wxCommandEvent &/*event*/)
{
    m_show_lines = !m_show_lines;
    show_hide_line_numbers();
}

void TextScreenFrame::on_action_load(wxCommandEvent &event)
{
}

void TextScreenFrame::on_margin_click(wxStyledTextEvent &event)
{
    m_text_screen->ToggleFold(m_text_screen->LineFromPosition(event.GetPosition()));
}

void TextScreenFrame::on_exit(wxCommandEvent & /*event*/)
{
    Close(true);
}
