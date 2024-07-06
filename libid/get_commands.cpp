#include "get_commands.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "file_item.h"
#include "helpdefs.h"
#include "id_data.h"
#include "value_saver.h"

#include <cstdio>

// execute commands from file
cmdarg_flags get_commands()
{
    std::FILE *parmfile;
    cmdarg_flags ret{cmdarg_flags::NONE};
    ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_PARMFILE};
    long point = get_file_entry(gfe_type::PARM, "Parameter Set", "*.par", g_command_file, g_command_name);
    if (point >= 0 && (parmfile = std::fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        std::fseek(parmfile, point, SEEK_SET);
        ret = load_commands(parmfile);
    }
    return ret;
}
