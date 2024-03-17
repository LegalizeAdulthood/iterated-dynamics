#include "expand_dirname.h"

#include "fix_dirname.h"

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
    fs::path path{dirname};
    if (path.has_relative_path())
    {
        path = absolute(path).make_preferred();
        std::strcpy(drive, path.root_name().string().c_str());
        std::strcpy(dirname, path.string().substr(std::strlen(drive)).c_str());
    }
    fix_dirname(dirname);

    return 0;
}
