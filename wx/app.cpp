// SPDX-License-Identifier: GPL-3.0-only
//
#include "app.h"

#include "ui/id_main.h"

#include "frame.h"
#include "wx_special_dirs.h"

#include <wx/wx.h>
#include <wx/evtloop.h>

#include <array>
#include <cassert>

wxIMPLEMENT_APP(IdApp);

bool IdApp::OnInit()
{
    g_special_dirs = std::make_shared<id::WxSpecialDirectories>();
    g_save_dir = g_special_dirs->documents_dir();

    // start a thread to call id_main here?
    id_main(argc, argv);

    return true;
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
