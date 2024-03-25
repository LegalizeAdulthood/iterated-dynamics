#include "do_pause.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "get_key_no_help.h"
#include "goodbye.h"

// defer pause until after parsing so we know if in batch mode
void dopause(int action)
{
    static unsigned char needpause = 0;
    switch (action)
    {
    case 0:
        if (g_init_batch == batch_modes::NONE)
        {
            if (needpause == 1)
            {
                driver_get_key();
            }
            else if (needpause == 2)
            {
                if (getakeynohelp() == FIK_ESC)
                {
                    goodbye();
                }
            }
        }
        needpause = 0;
        break;
    case 1:
    case 2:
        needpause = (char)action;
        break;
    default:
        break;
    }
}
