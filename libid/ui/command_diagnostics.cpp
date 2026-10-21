// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/command_diagnostics.h"

#include "engine/cmdfiles.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/get_key_no_help.h"
#include "ui/id_keys.h"
#include "ui/stop_msg.h"

#include <algorithm>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

void show_command_diagnostics(const CommandDiagnostics &diagnostics)
{
    ValueSaver first_init{g_first_init, false};
    for (const std::string &warning : diagnostics.warnings)
    {
        ValueSaver batch{g_init_batch};
        stop_msg(warning);
    }
    for (const std::string &error : diagnostics.errors)
    {
        stop_msg(error);
    }
}

bool show_startup_command_diagnostics(const CommandDiagnostics &diagnostics)
{
    if (diagnostics.errors.empty() && diagnostics.warnings.empty())
    {
        return true;
    }
    if (g_init_batch != BatchMode::NONE)
    {
        show_command_diagnostics(diagnostics);
        return diagnostics.errors.empty();
    }

    driver_set_for_text();
    driver_put_string(0, 0, 15, "Id found the following problems when parsing commands: ");
    int row{1};
    for (const std::vector<std::string> *messages : {&diagnostics.errors, &diagnostics.warnings})
    {
        for (const std::string &message : *messages)
        {
            driver_put_string(row, 0, 7, message);
            row += 1 + static_cast<int>(std::count(message.begin(), message.end(), '\n'));
        }
    }
    const bool failed{!diagnostics.errors.empty()};
    if (failed)
    {
        driver_buzzer(Buzzer::PROBLEM);
    }
    driver_put_string(
        ++row, 0, 15, failed ? "Press any key to exit" : "Press Escape to abort, any other key to continue");
    driver_move_cursor(row + 1, 0);
    driver_wait_key_pressed(false);
    const int key{get_a_key_no_help()};
    return !failed && key != ID_KEY_ESC;
}

} // namespace id::ui
