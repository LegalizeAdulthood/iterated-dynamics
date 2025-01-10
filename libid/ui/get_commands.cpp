// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_commands.h"

#include "helpdefs.h"
#include "engine/id_data.h"
#include "ui/file_item.h"
#include "ValueSaver.h"

#include <cstdio>

// execute commands from file
CmdArgFlags get_commands()
{
    std::FILE *param_file;
    CmdArgFlags ret{CmdArgFlags::NONE};
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_PARAM_FILE};
    long point = get_file_entry(ItemType::PAR_SET, "Parameter Set", "*.par", g_command_file, g_command_name);
    if (point >= 0 && (param_file = std::fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        std::fseek(param_file, point, SEEK_SET);
        ret = load_commands(param_file);
    }
    return ret;
}
