// SPDX-License-Identifier: GPL-3.0-only
//
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
    driver_set_clear(); // clear the screen
    std::snprintf(msg, sizeof(msg), ID_PROGRAM_NAME " Version %d.%01d (" ID_GIT_HASH ")", //
        g_release / 100, (g_release % 100) / 10);
    if (g_release % 10)
    {
        char buf[MSG_LEN];
        std::snprintf(buf, sizeof(buf), "%01d", g_release % 10);
        std::strcat(msg, buf);
    }
    if (g_patch_level)
    {
        char buf[MSG_LEN];
        std::snprintf(buf, sizeof(buf), ".%d", g_patch_level);
        std::strcat(msg, buf);
    }
    putstringcenter(0, 0, 80, C_TITLE, msg);
}
