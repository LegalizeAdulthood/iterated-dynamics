// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <wx/app.h>

class IdApp : public wxApp
{
public:
    ~IdApp() override = default;

    bool OnInit() override;
    int OnRun() override;

    void create_window();
    void pump_messages(bool wait_flag);
};

IdApp &wxGetApp();
