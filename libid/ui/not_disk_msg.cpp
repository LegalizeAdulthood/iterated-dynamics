// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/not_disk_msg.h"

#include "ui/stop_msg.h"

void not_disk_msg()
{
    stop_msg("This type may be slow using a real-disk based 'video' mode, but may not \n"
            "be too bad if you have enough expanded or extended memory. Press <Esc> to \n"
            "abort if it appears that your disk drive is working too hard.");
}
