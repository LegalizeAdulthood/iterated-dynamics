#include "not_disk_msg.h"

#include "stop_msg.h"

void notdiskmsg()
{
    stopmsg(STOPMSG_NONE,
            "This type may be slow using a real-disk based 'video' mode, but may not \n"
            "be too bad if you have enough expanded or extended memory. Press <Esc> to \n"
            "abort if it appears that your disk drive is working too hard.");
}
