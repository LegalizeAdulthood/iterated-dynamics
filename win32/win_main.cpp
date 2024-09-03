#include "id_main.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>
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
