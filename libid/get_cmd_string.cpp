#include "get_cmd_string.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "debug_flags.h"
#include "field_prompt.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"
#include "loadfile.h"

/*
    get_cmd_string() is called whenever the 'g' key is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  parameter changed, no need to regenerate
        >0  parameter changed, regenerate
*/

int get_cmd_string()
{
    static char cmdbuf[61];

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP_COMMANDS;
    int i = field_prompt("Enter command string to use.", nullptr, cmdbuf, 60, nullptr);
    g_help_mode = old_help_mode;
    if (i >= 0 && cmdbuf[0] != 0)
    {
        i = +cmdarg(cmdbuf, cmd_file::AT_AFTER_STARTUP);
        if (g_debug_flag == debug_flags::write_formula_debug_information)
        {
            backwards_v18();
            backwards_v19();
            backwards_v20();
        }
    }

    return i;
}
