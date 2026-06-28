// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/lsystem.h"

#include "fractals/lsystem.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int lsystem_type()
{
    if (!lsystem_loaded() && lsystem_load())
    {
        return -1;
    }

    LSystem l_system;
    l_system.start();
    unsigned char poll_counter{};
    while (!l_system.done() && !l_system.interrupted())
    {
        if (poll_counter++ == 0 && driver_key_pressed())
        {
            l_system.suspend();
            return 0;
        }
        l_system.iterate();
    }
    return 0;
}

} // namespace id::ui
