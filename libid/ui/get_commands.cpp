// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_commands.h"

#include "helpdefs.h"
#include "misc/ValueSaver.h"
#include "ui/file_item.h"
#include "ui/help.h"

#include <cstdio>

using namespace id::engine;
using namespace id::help;
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
        ret = load_commands(param_file);
    }
    return ret;
}

} // namespace id::ui
