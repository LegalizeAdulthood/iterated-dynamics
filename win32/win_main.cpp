// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/id_main.h"

#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

#include "win_defines.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <crtdbg.h>

HINSTANCE g_instance{};

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show)
{
    int result = 0;

    __try  // NOLINT(clang-diagnostic-language-extension-token)
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
