// SPDX-License-Identifier: GPL-3.0-only
//
#include "fix_dirname.h"

#include "port.h"

#include "id.h"

#include <cstring>

// fix up directory names
void fix_dir_name(char *dirname)
{
    int length = (int) std::strlen(dirname); // index of last character

    // make sure dirname ends with a slash
    if (length > 0)
    {
        if (dirname[length-1] == SLASHC)
        {
            return;
        }
    }
    std::strcat(dirname, SLASH);
}

void fix_dir_name(std::string &dirname)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, dirname.c_str());
    fix_dir_name(buff);
    dirname = buff;
}
