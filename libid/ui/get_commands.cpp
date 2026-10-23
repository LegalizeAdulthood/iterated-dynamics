// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/get_commands.h"

#include "engine/CommandProcessor.h"
#include "helpdefs.h"
#include "io/file_item.h"
#include "io/loadfile.h"
#include "misc/ValueSaver.h"
#include "ui/command_diagnostics.h"
#include "ui/command_effects.h"
#include "ui/CommandRouter.h"
#include "ui/get_file_entry.h"
#include "ui/goodbye.h"
#include "ui/help.h"

#include <cstdio>
#include <vector>

using namespace id::engine;
using namespace id::help;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

// execute commands from file
CmdArgFlags get_commands()
{
    std::FILE *param_file;
    CmdArgFlags ret{CmdArgFlags::NONE};
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_PARAM_FILE};
    long point = get_file_entry(ItemType::PAR_SET, g_parameter_file, g_parameter_set_name);
    if (point >= 0 && (param_file = std::fopen(g_parameter_file.string().c_str(), "rb")) != nullptr)
    {
        std::fseek(param_file, point, SEEK_SET);
        CommandProcessor processor{param_file, CmdFile::AT_AFTER_STARTUP};
        const CommandRouter router;
        CommandParseResult result{router.process(processor)};
        // PAR reads a file and sets color, don't read colors from GIF.
        g_read_color = !g_colors_preloaded || g_show_file != ShowFile::LOAD_IMAGE;
        const bool exit_requested{apply_command_effects(result)};
        ret = result.flags;
        show_command_diagnostics(result.diagnostics);
        if (exit_requested)
        {
            if (!result.diagnostics.errors.empty())
            {
                g_init_batch = BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE;
            }
            goodbye();
        }
    }
    return ret;
}

} // namespace id::ui
