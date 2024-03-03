#include "split_path.h"

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

void splitpath(const std::string &file_template, char *drive, char *dir, char *fname, char *ext)
{
    fs::path path{file_template};
    path.make_preferred();
    if (drive != nullptr)
    {
        std::strcpy(drive, path.root_name().string().c_str());
    }
    const std::string dir_name{path.parent_path().string().substr(path.root_name().string().length())};
    if (dir != nullptr)
    {
        std::strcpy(dir, dir_name.empty() ? "" : (dir_name + static_cast<char>(fs::path::preferred_separator)).c_str());
    }
    if (fname != nullptr)
    {
        std::strcpy(fname, path.stem().string().c_str());
    }
    if (ext != nullptr)
    {
        std::strcpy(ext, path.extension().string().c_str());
    }
}
