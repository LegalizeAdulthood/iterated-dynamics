// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/help_title.h"

#include "engine/cmdfiles.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/put_string_center.h"

#include <config/port_config.h>

#include <fmt/format.h>

#include <string>

void help_title()
{
    driver_set_clear();
    std::string msg{fmt::format(ID_PROGRAM_NAME " Version {:d}.{:d}", id::ID_VERSION_MAJOR, id::ID_VERSION_MINOR)};
    if (id::ID_VERSION_PATCH)
    {
        msg += fmt::format(".{:d}", id::ID_VERSION_PATCH);
    }
    if (id::ID_VERSION_TWEAK)
    {
        msg += fmt::format(".{:d}", id::ID_VERSION_TWEAK);
    }
    msg += " (" ID_GIT_HASH ")";
    put_string_center(0, 0, 80, C_TITLE, msg);
}
