// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/popcorn.h"

#include "engine/NonStandardFractal.h"
#include "ui/diskvid.h"
#include "ui/KeyboardHandler.h"

#include <memory>

using namespace id::engine;

namespace id::ui
{

int popcorn_type(NonStandardFractal &calculation)
{
    reset_calc_interrupted();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};

    while (!calculation.done())
    {
        calculation.iterate();
        display_disk_storage_progress();
        if (!calculation.done())
        {
            if (calc_interrupted())
            {
                calculation.suspend();
                return -1;
            }
        }
    }
    return 0;
}

} // namespace id::ui
