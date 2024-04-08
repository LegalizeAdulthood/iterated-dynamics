#include "id_main.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>
#include <crtdbg.h>

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdLine, int show)
{
    int result = 0;

    __try
    {
        s_tos = (char *) &result;
        g_instance = instance;
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        result = id_main(__argc, __argv);
    }
    __except (CreateMiniDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        result = -1;
    }

    return result;
}
