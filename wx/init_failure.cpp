// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/init_failure.h"

#include <wx/wx.h>

namespace id::ui
{

void init_failure(const char *message)
{
    wxMessageBox(message, "Initialization Failure");
}

} // namespace id::ui
