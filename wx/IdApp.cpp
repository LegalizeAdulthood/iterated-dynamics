// SPDX-License-Identifier: GPL-3.0-only
//
#include "IdApp.h"

#include "ui/id_main.h"

#include "IdFrame.h"
#include "../win32/instance.h"

#include <wx/wx.h>
#include <wx/evtloop.h>
#ifdef WIN32
#include <wx/msw/private.h>
#endif

#include <array>
#include <cassert>

wxIMPLEMENT_APP(IdApp);

bool IdApp::OnInit()
{
#ifdef WIN32
    g_instance = wxGetInstance();
#endif

    return true;
}

int IdApp::OnRun()
{
    return id::ui::id_main(argc, argv);
}

void IdApp::create_window()
{
    IdFrame *frame = new IdFrame();
    frame->Show(true);
}

void IdApp::pump_messages(bool wait_flag)
{
    wxEventLoopBase *loop = GetMainLoop();
    while (loop->Pending())
    {
        if (!loop->Dispatch())
        {
            return;
        }
    }
}
