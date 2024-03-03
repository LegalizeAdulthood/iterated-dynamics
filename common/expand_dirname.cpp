#include "expand_dirname.h"

#include "port.h"
#include "fractint.h"

#include "fix_dirname.h"

#include <Shlwapi.h>
#include <crtdbg.h>

#include <cstring>

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
    if (PathIsRelative(dirname))
    {
        char relative[MAX_PATH];
        _ASSERTE(std::strlen(drive) < NUM_OF(relative));
        std::strcpy(relative, drive);
        _ASSERTE(std::strlen(relative) + std::strlen(dirname) < NUM_OF(relative));
        std::strcat(relative, dirname);
        char absolute[MAX_PATH];
        BOOL status = PathSearchAndQualify(relative, absolute, NUM_OF(absolute));
        _ASSERTE(status);
        if (':' == absolute[1])
        {
            drive[0] = absolute[0];
            drive[1] = absolute[1];
            drive[2] = 0;
            std::strcpy(dirname, &absolute[2]);
        }
        else
        {
            std::strcpy(dirname, absolute);
        }
    }
    fix_dirname(dirname);

    return 0;
}
