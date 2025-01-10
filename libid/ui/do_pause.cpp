// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/do_pause.h"

#include "drivers.h"
#include "ui/cmdfiles.h"
#include "ui/get_key_no_help.h"
#include "ui/goodbye.h"
#include "ui/id_keys.h"

// defer pause until after parsing so we know if in batch mode
void do_pause(int action)
{
    static int need_pause{};
    switch (action)
    {
    case 0:
        if (g_init_batch == BatchMode::NONE)
        {
            if (need_pause == 1)
            {
                driver_get_key();
            }
            else if (need_pause == 2)
            {
                if (get_a_key_no_help() == ID_KEY_ESC)
                {
                    goodbye();
                }
            }
        }
        need_pause = 0;
        break;
    case 1:
    case 2:
        need_pause = action;
        break;
    default:
        break;
    }
}
