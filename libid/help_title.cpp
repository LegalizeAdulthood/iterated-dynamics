#include "help_title.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "id.h"
#include "drivers.h"
#include "put_string_center.h"
#include "version.h"

#include <cstdio>
#include <cstring>

void helptitle()
{
    char msg[MSG_LEN];
    char buf[MSG_LEN];
    driver_set_clear(); // clear the screen
    std::sprintf(msg, ID_PROGRAM_NAME " Version %d.%01d", g_release/100, (g_release%100)/10);
    if (g_release%10)
    {
        std::sprintf(buf, "%01d", g_release%10);
        std::strcat(msg, buf);
    }
    if (g_patch_level)
    {
        std::sprintf(buf, ".%d", g_patch_level);
        std::strcat(msg, buf);
    }
    putstringcenter(0, 0, 80, C_TITLE, msg);
}
