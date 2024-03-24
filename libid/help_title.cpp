#include "help_title.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "id.h"
#include "drivers.h"
#include "realdos.h"

#include <cstdio>
#include <cstring>

int g_release{2099};        // this has 2 implied decimals; increment it every synch
const int g_patch_level{8}; // patchlevel for DOS version

void helptitle()
{
    char msg[MSG_LEN], buf[MSG_LEN];
    driver_set_clear(); // clear the screen
    std::sprintf(msg, "Iterated Dynamics Version %d.%01d", g_release/100, (g_release%100)/10);
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
