// SPDX-License-Identifier: GPL-3.0-only
//
#include "id_main.h"

#if 0
#include "win_defines.h"
#include <DbgHelp.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <crtdbg.h>

#include <cctype>
#include <cstdarg>

#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdLine, int show)
{
    int result = 0;

    __try
    {
        g_top_of_stack = (char *) &result;
        g_instance = instance;
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        result = id_main(__argc, __argv);
    }
    __except (create_minidump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        result = -1;
    }

    return result;
}
#endif

#include <wx/wx.h>

class IdApp : public wxApp
{
public:
    ~IdApp() override = default;

    bool OnInit() override;
};

class IdFrame : public wxFrame
{
public:
    IdFrame();
    ~IdFrame() override = default;

private:
    void OnHello(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
};

enum
{
    ID_Hello = 1,
};

wxIMPLEMENT_APP(IdApp);

bool IdApp::OnInit()
{
    IdFrame *frame = new IdFrame();
    frame->Show(true);
    return true;
}

IdFrame::IdFrame() :
    wxFrame(nullptr, wxID_ANY, wxT("Iterated Dynamics"))
{
    wxMenu *file = new wxMenu;
    file->Append(ID_Hello, "&Hello...\tCtrl+H", "Hep shown in status bar for this menu item");
    file->AppendSeparator();
    file->Append(wxID_EXIT);

    wxMenu *help = new wxMenu;
    help->Append(wxID_ABOUT);

    wxMenuBar *bar = new wxMenuBar;
    bar->Append(file, "&File");
    bar->Append(help, "&Help");

    wxFrameBase::SetMenuBar(bar);

    wxFrameBase::CreateStatusBar();
    wxFrameBase::SetStatusText("Welcome to Iterated Dynamics!");

    Bind(wxEVT_MENU, &IdFrame::OnHello, this, ID_Hello);
    Bind(wxEVT_MENU, &IdFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &IdFrame::OnExit, this, wxID_EXIT);
}

void IdFrame::OnHello(wxCommandEvent &event)
{
    wxLogMessage("Hello from Iterated Dynamics!");
}

void IdFrame::OnExit(wxCommandEvent &event)
{
    Close(true);
}

void IdFrame::OnAbout(wxCommandEvent &event)
{
    wxMessageBox(
        "Iterated Dynamics is a fractal renderer", "About Iterated Dynamics", wxOK | wxICON_INFORMATION);
}
