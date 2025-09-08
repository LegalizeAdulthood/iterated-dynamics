// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/init_failure.h"

#include "win_defines.h"
#include <Windows.h>

namespace id::ui
{

void init_failure(const char *message)
{
    MessageBoxA(nullptr, message, "Id: Fatal Error", MB_OK);
}

} // namespace id::ui
