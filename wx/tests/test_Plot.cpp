#include <gui/Plot.h>

#include <wx/stc/stc.h>
#include <wx/wx.h>

namespace id::wx
{

class TestPlotApp : public wxApp
{
public:
    bool OnInit() override;
};

class TestPlotFrame : public wxFrame
{
public:
    TestPlotFrame(const wxString &title);

protected:
    // Override size-related methods to make frame non-resizable
    void DoSetSize(int x, int y, int width, int height, int size_flags = wxSIZE_AUTO) override;
    wxSize DoGetBestSize() const override;
    wxSize GetMinSize() const override;
    wxSize GetMaxSize() const override;

private:
    void on_action_display_string(wxCommandEvent &event);
    void on_exit(wxCommandEvent &event);
    wxSize calculate_frame_size() const;

    Plot *m_plot{};
    mutable wxSize m_fixed_size{};
};

wxIMPLEMENT_APP(TestPlotApp);

bool TestPlotApp::OnInit()
{
    TestPlotFrame *frame = new TestPlotFrame("Plot Control Test");
    frame->Show(true);
    return true;
}

TestPlotFrame::TestPlotFrame(const wxString &title) :
    wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)),
    m_plot(new Plot(this, wxID_ANY, wxDefaultPosition, wxSize{640, 480})),
    m_fixed_size(calculate_frame_size())
{
    wxMenuBar *menu_bar = new wxMenuBar;
    wxMenu *file = new wxMenu;
    file->Append(wxID_EXIT, "&Quit\tAlt-F4", "Quit");
    menu_bar->Append(file, "&File");
    wxMenu *action = new wxMenu;
    wxMenuItem *display_string = action->Append(wxID_ANY, "&Display String");
    Bind(wxEVT_MENU, &TestPlotFrame::on_action_display_string, this, display_string->GetId());
    menu_bar->Append(action, "&Action");
    wxFrameBase::SetMenuBar(menu_bar);
    Bind(wxEVT_MENU, &TestPlotFrame::on_exit, this, wxID_EXIT);

    SetSize(m_fixed_size);
    Center();
}

wxSize TestPlotFrame::calculate_frame_size() const
{
    // Get the Plot's fixed size
    wxSize text_screen_size = m_plot->GetBestSize();

    // Account for frame decorations (menu bar, borders, etc.)
    wxSize frame_size = text_screen_size;

    // Add space for menu bar
    if (GetMenuBar())
    {
        frame_size.y += GetMenuBar()->GetSize().GetHeight();
    }

    // Add frame border space (estimate based on typical frame decorations)
    frame_size.x += 16; // Left and right borders
    frame_size.y += 40; // Title bar and bottom border

    return frame_size;
}

void TestPlotFrame::DoSetSize(int x, int y, int width, int height, int size_flags)
{
    // Ignore any size changes and use our fixed size
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_frame_size();
    }

    // Only update position, keep our fixed size
    wxFrame::DoSetSize(x, y, m_fixed_size.x, m_fixed_size.y, size_flags | wxSIZE_FORCE);
}

wxSize TestPlotFrame::DoGetBestSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_frame_size();
    }

    return m_fixed_size;
}

wxSize TestPlotFrame::GetMinSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_frame_size();
    }

    return m_fixed_size;
}

wxSize TestPlotFrame::GetMaxSize() const
{
    if (m_fixed_size.x == 0 || m_fixed_size.y == 0)
    {
        m_fixed_size = calculate_frame_size();
    }

    return m_fixed_size;
}

void TestPlotFrame::on_action_display_string(wxCommandEvent & /*event*/)
{
    m_plot->display_string(10, 10, 255, 0, "Hello, Plot!");
}

void TestPlotFrame::on_exit(wxCommandEvent & /*event*/)
{
    Close(true);
}

} // namespace id::wx
