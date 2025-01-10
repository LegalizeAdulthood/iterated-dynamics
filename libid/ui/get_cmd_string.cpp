// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_cmd_string.h"

#include "debug_flags.h"
#include "engine/id_data.h"
#include "helpdefs.h"
#include "io/loadfile.h"
#include "ui/cmdfiles.h"
#include "ui/field_prompt.h"
#include "ValueSaver.h"

/*
    get_cmd_string() is called whenever the 'g' key is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  parameter changed, no need to regenerate
        >0  parameter changed, regenerate
*/

int get_cmd_string()
{
    static char cmd_buf[61];

    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_COMMANDS};
    int i = field_prompt("Enter command string to use.", nullptr, cmd_buf, 60, nullptr);
    if (i >= 0 && cmd_buf[0] != 0)
    {
        i = +cmd_arg(cmd_buf, CmdFile::AT_AFTER_STARTUP);
        if (g_debug_flag == DebugFlags::WRITE_FORMULA_DEBUG_INFORMATION)
        {
            backwards_v18();
            backwards_v19();
            backwards_v20();
        }
    }

    return i;
}
