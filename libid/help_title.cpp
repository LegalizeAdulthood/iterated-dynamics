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

void help_title()
{
    driver_set_clear();
    char msg[MSG_LEN];
    std::snprintf(msg, sizeof(msg), ID_PROGRAM_NAME " Version %d.%d", ID_VERSION_MAJOR, ID_VERSION_MINOR);
    if (ID_VERSION_PATCH)
    {
        char buf[MSG_LEN];
        std::snprintf(buf, sizeof(buf), ".%d", ID_VERSION_PATCH);
        std::strcat(msg, buf);
    }
    if (ID_VERSION_TWEAK)
    {
        char buf[MSG_LEN];
        std::snprintf(buf, sizeof(buf), ".%d", ID_VERSION_TWEAK);
        std::strcat(msg, buf);
    }
    strcat(msg, " (" ID_GIT_HASH ")");
    put_string_center(0, 0, 80, C_TITLE, msg);
}
