// SPDX-License-Identifier: GPL-3.0-only
//
#include "stop_msg.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "debug_flags.h"
#include "dir_file.h"
#include "do_pause.h"
#include "drivers.h"
#include "get_key_no_help.h"
#include "id_data.h"
#include "id_keys.h"
#include "mouse.h"
#include "os.h"
#include "text_screen.h"
#include "value_saver.h"

#include <cstdio>
#include <cstring>
#ifdef XFRACT
#include <unistd.h>
#endif

static void blankrows(int row, int count, int attr)
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
bool stop_msg(stopmsg_flags flags, const std::string &msg)
{
    int toprow;
    int color;
    static bool batchmode = false;
    if (g_debug_flag != debug_flags::none || g_init_batch >= batch_modes::NORMAL)
    {
        if (std::FILE *fp = dir_fopen(g_working_dir.c_str(), "stopmsg.txt", g_init_batch == batch_modes::NONE ? "w" : "a"))
        {
            std::fprintf(fp, "%s\n", msg.c_str());
            std::fclose(fp);
        }
    }
    if (g_first_init)
    {
        // & cmdfiles hasn't finished 1st try
#ifdef XFRACT
        driver_set_for_text();
        driver_buzzer(buzzer_codes::PROBLEM);
        driver_put_string(0, 0, 15, "*** Error during startup:");
        driver_put_string(2, 0, 15, msg);
        driver_move_cursor(8, 0);
        sleep(1);
        close_drivers();
        exit(1);
#else
        std::printf("%s\n", msg.c_str());
        do_pause(1); // pause deferred until after cmdfiles
        return false;
#endif
    }
    if (g_init_batch >= batch_modes::NORMAL || batchmode)
    {
        // in batch mode
        g_init_batch = batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE; // used to set errorlevel
        batchmode = true; // fixes *second* stopmsg in batch mode bug
        return true;
    }
    ValueSaver saved_look_at_mouse{g_look_at_mouse, -ID_KEY_ENTER};
    if (bit_set(flags, stopmsg_flags::NO_STACK))
    {
        blankrows(toprow = 12, 10, 7);
    }
    else
    {
        driver_stack_screen();
        toprow = 4;
        driver_move_cursor(4, 0);
    }
    g_text_cbase = 2; // left margin is 2
    driver_put_string(toprow, 0, 7, msg);
    if (bit_set(flags, stopmsg_flags::CANCEL))
    {
        driver_put_string(g_text_row+2, 0, 7, "Escape to cancel, any other key to continue...");
    }
    else
    {
        driver_put_string(g_text_row+2, 0, 7, "Any key to continue...");
    }
    g_text_cbase = 0; // back to full line
    color = bit_set(flags, stopmsg_flags::INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
    driver_set_attr(toprow, 0, color, (g_text_row+1-toprow)*80);
    driver_hide_text_cursor();   // cursor off
    if (!bit_set(flags, stopmsg_flags::NO_BUZZER))
    {
        driver_buzzer(bit_set(flags, stopmsg_flags::INFO_ONLY) ? buzzer_codes::COMPLETE
                                                                       : buzzer_codes::PROBLEM);
    }
    while (driver_key_pressed())   // flush any keyahead
    {
        driver_get_key();
    }
    bool ret = false;
    if (g_debug_flag != debug_flags::show_formula_info_after_compile)
    {
        if (get_a_key_no_help() == ID_KEY_ESC)
        {
            ret = true;
        }
    }
    if (bit_set(flags, stopmsg_flags::NO_STACK))
    {
        blankrows(toprow, 10, 7);
    }
    else
    {
        driver_unstack_screen();
    }
    return ret;
}
