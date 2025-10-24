// SPDX-License-Identifier: GPL-3.0-only
//
#include "wx_id.h"

#include <misc/stack_avail.h>
#include <ui/id_main.h>

#ifdef WIN32
#include "../win32/instance.h"
#include "../win32/create_minidump.h"

#include <wx/msw/private.h>

#include <crtdbg.h>
#endif

using namespace id::misc;
using namespace id::ui;

namespace id::wx
{

void init()
{
#ifdef WIN32
    g_instance = wxGetInstance();
#endif
}

int main(int argc, char *argv[])
{
    int result = 0;
    g_top_of_stack = reinterpret_cast<char *>(&result);
#ifdef WIN32
    __try // NOLINT(clang-diagnostic-language-extension-token)
    {
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        result = id_main(argc, argv);
    }
    __except (create_minidump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        result = -1;
    }
#else
    result = id_main(argc, argv);
#endif
    return result;
}

} // namespace id::wx
