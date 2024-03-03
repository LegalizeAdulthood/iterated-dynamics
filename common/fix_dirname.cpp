#include "fix_dirname.h"

#include "port.h"
#include "fractint.h"

#include <cstring>

// fix up directory names
void fix_dirname(char *dirname)
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

void fix_dirname(std::string &dirname)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, dirname.c_str());
    fix_dirname(buff);
    dirname = buff;
}
