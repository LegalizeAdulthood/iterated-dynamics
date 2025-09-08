// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/testpt.h"

#include "engine/resume.h"
#include "fractals/TestPoint.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

// standalone engine for "test"
int test_type()
{
    TestPoint test;

    if (g_resuming)
    {
        test.resume();
    }
    if (test.start())   // assume it was stand-alone, doesn't want passes logic
    {
        return 0;
    }
    while (!test.done())
    {
        if (driver_key_pressed())
        {
            test.suspend();
            return -1;
        }
        test.iterate();
    }
    test.finish();
    return 0;
}

} // namespace id::ui
