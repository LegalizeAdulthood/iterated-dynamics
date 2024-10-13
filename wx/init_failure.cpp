// SPDX-License-Identifier: GPL-3.0-only
//
#include "init_failure.h"

#include <wx/wx.h>

void init_failure(const char *message)
{
    wxMessageBox(message, "Initialization Failure");
}
