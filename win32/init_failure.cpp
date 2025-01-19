// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/init_failure.h"

#include "win_defines.h"
#include <Windows.h>

void init_failure(char const *message)
{
    MessageBoxA(nullptr, message, "Id: Fatal Error", MB_OK);
}
