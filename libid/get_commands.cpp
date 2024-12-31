// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_commands.h"

#include "file_item.h"
#include "helpdefs.h"
#include "id_data.h"
#include "ValueSaver.h"

#include <cstdio>

// execute commands from file
CmdArgFlags get_commands()
{
    std::FILE *parmfile;
    CmdArgFlags ret{CmdArgFlags::NONE};
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_PARAM_FILE};
    long point = get_file_entry(ItemType::PARM, "Parameter Set", "*.par", g_command_file, g_command_name);
    if (point >= 0 && (parmfile = std::fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        std::fseek(parmfile, point, SEEK_SET);
        ret = load_commands(parmfile);
    }
    return ret;
}
