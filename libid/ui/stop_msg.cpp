// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/stop_msg.h"

#include "engine/cmdfiles.h"
#include "engine/wait_until.h"
#include "io/dir_file.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/get_key_no_help.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/text_screen.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

static void blank_rows(int row, int count, int attr)
{
    char buf[81];
    std::memset(buf, ' ', 80);
    buf[80] = 0;
    while (--count >= 0)
    {
        driver_put_string(row++, 0, attr, buf);
    }
}

/* int stopmsg(flags,message) displays message and waits for a key:
     message should be a max of 9 lines with \n's separating them;
       no leading or trailing \n's in message;
       no line longer than 76 chars for best appearance;
     flag options:
       &1 if already in text display mode, stackscreen is not called
          and message is displayed at (12,0) instead of (4,0)
       &2 if continue/cancel indication is to be returned;
          when not set, "Any key to continue..." is displayed
          when set, "Escape to cancel, any other key to continue..."
          true is returned for cancel, false for continue
       &4 set to suppress buzzer
       &8 for parser - use a fixed pitch font
      &16 for info only message (green box instead of red in DOS vsn)
   */
bool stop_msg(StopMsgFlags flags, const std::string &msg)
{
    int top_row;
    static bool batch_mode = false;
    if (g_debug_flag != DebugFlags::NONE || g_init_batch >= BatchMode::NORMAL)
    {
        if (std::FILE *fp = dir_fopen(g_working_dir, "stopmsg.txt", g_init_batch == BatchMode::NONE ? "w" : "a"))
        {
            std::fprintf(fp, "%s\n", msg.c_str());
            std::fclose(fp);
        }
    }
    if (g_first_init)
    {
        // & cmdfiles hasn't finished 1st try
        driver_set_for_text();
        driver_buzzer(Buzzer::PROBLEM);
        driver_put_string(0, 0, 15, "*** Error during startup:");
        driver_put_string(2, 0, 15, msg);
        driver_move_cursor(8, 0);
        sleep_ms(10000);
        close_drivers();
        std::exit(1);
    }
    if (g_init_batch >= BatchMode::NORMAL || batch_mode)
    {
        // in batch mode
        g_init_batch = BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE; // used to set errorlevel
        batch_mode = true; // fixes *second* stopmsg in batch mode bug
        return true;
    }
    ValueSaver saved_look_at_mouse{g_look_at_mouse, mouse_look_key(ID_KEY_ENTER)};
    if (bit_set(flags, StopMsgFlags::NO_STACK))
    {
        blank_rows(top_row = 12, 10, 7);
    }
    else
    {
        driver_stack_screen();
        top_row = 4;
        driver_move_cursor(4, 0);
    }
    g_text_col_base = 2; // left margin is 2
    driver_put_string(top_row, 0, 7, msg);
    if (bit_set(flags, StopMsgFlags::CANCEL))
    {
        driver_put_string(g_text_row+2, 0, 7, "Escape to cancel, any other key to continue...");
    }
    else
    {
        driver_put_string(g_text_row+2, 0, 7, "Any key to continue...");
    }
    g_text_col_base = 0; // back to full line
    const int color = bit_set(flags, StopMsgFlags::INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
    driver_set_attr(top_row, 0, color, (g_text_row+1-top_row)*80);
    driver_hide_text_cursor();   // cursor off
    if (!bit_set(flags, StopMsgFlags::NO_BUZZER))
    {
        driver_buzzer(bit_set(flags, StopMsgFlags::INFO_ONLY) ? Buzzer::COMPLETE
                                                                       : Buzzer::PROBLEM);
    }
    while (driver_key_pressed())   // flush any keyahead
    {
        driver_get_key();
    }
    bool ret = false;
    if (g_debug_flag != DebugFlags::SHOW_FORMULA_INFO_AFTER_COMPILE)
    {
        if (get_a_key_no_help() == ID_KEY_ESC)
        {
            ret = true;
        }
    }
    if (bit_set(flags, StopMsgFlags::NO_STACK))
    {
        blank_rows(top_row, 10, 7);
    }
    else
    {
        driver_unstack_screen();
    }
    return ret;
}

} // namespace id::ui
