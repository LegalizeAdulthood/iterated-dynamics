#include "save_file.h"

#include "special_dirs.h"

#include <filesystem>

namespace fs = std::filesystem;

std::FILE *open_save_file(const std::string &name, const std::string &mode)
{
    fs::path path{name};
    if (path.is_relative())
    {
        path = fs::path{g_save_dir} / path;
    }
    return std::fopen(path.string().c_str(), mode.c_str());
}
