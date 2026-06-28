// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/lsystem.h"

#include "fractals/lsystem.h"

using namespace id::fractals;

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
    while (!l_system.done() && !l_system.interrupted())
    {
        l_system.iterate();
    }
    return 0;
}

} // namespace id::ui
