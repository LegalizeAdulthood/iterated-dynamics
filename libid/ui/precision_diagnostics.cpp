// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/precision_diagnostics.h"

#include "engine/fractalb.h"
#include "ui/stop_msg.h"

#include <string>
#include <vector>

namespace id::ui
{

bool show_precision_diagnostics()
{
    std::vector<std::string> diagnostics;
    diagnostics.swap(engine::g_precision_diagnostics);
    for (const std::string &diagnostic : diagnostics)
    {
        if (stop_msg(diagnostic))
        {
            return true;
        }
    }
    return false;
}

} // namespace id::ui
