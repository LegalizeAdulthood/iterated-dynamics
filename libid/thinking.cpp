// SPDX-License-Identifier: GPL-3.0-only
//
#include "thinking.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "help_title.h"
#include "os.h"
#include "text_screen.h"

#include <cstring>

/* thinking(1,message):
      if thinking message not yet on display, it is displayed;
      otherwise the wheel is updated
      returns 0 to keep going, -1 if keystroke pending
   thinking(0,nullptr):
      call this when thinking phase is done
   */
static int s_think_state{-1};
static char const *const s_wheel[]{"-", "\\", "|", "/"};
static int s_think_col{};
static int s_think_count{};

bool thinking(int options, char const *msg)
{
    if (options == 0)
    {
        if (s_think_state >= 0)
        {
            s_think_state = -1;
            driver_unstack_screen();
        }
        return false;
    }

    if (s_think_state < 0)
    {
        driver_stack_screen();
        s_think_state = 0;
        helptitle();
        char buf[81];
        std::strcpy(buf, "  ");
        std::strcat(buf, msg);
        std::strcat(buf, "    ");
        driver_put_string(4, 10, C_GENERAL_HI, buf);
        s_think_col = g_text_col - 3;
        s_think_count = 0;
    }
    if ((s_think_count++) < 100)
    {
        return false;
    }
    s_think_count = 0;
    driver_put_string(4, s_think_col, C_GENERAL_HI, s_wheel[s_think_state]);
    driver_hide_text_cursor(); // turn off cursor
    s_think_state = (s_think_state + 1) & 3;
    return driver_key_pressed() != 0;
}
