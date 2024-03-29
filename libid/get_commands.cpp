#include "get_commands.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "file_item.h"
#include "helpdefs.h"
#include "id_data.h"

#include <cstdio>

// execute commands from file
int get_commands()
{
    int ret;
    std::FILE *parmfile;
    ret = 0;
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPPARMFILE;
    long point = get_file_entry(gfe_type::PARM, "Parameter Set", "*.par", g_command_file, g_command_name);
    if (point >= 0 && (parmfile = std::fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        std::fseek(parmfile, point, SEEK_SET);
        ret = load_commands(parmfile);
    }
    g_help_mode = old_help_mode;
    return ret;
}
