// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/check_write_file.h"

#include "io/update_save_name.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace id::io
{

bool g_overwrite_file{}; // true if file overwrite allowed

fs::path get_checked_save_path(const WriteFile kind, const fs::path &name)
{
    fs::path path{get_save_path(kind, name.string())};
    while (!g_overwrite_file && fs::exists(path))
    {
        path = next_save_name(path.string());
    }
    return path;
}

fs::path get_append_save_path(const WriteFile kind, const fs::path &name)
{
    return get_save_path(kind, name.string());
}

void check_write_file(std::string &name, const char *ext)
{
    do
    {
        fs::path open_file{name};
        if (!open_file.has_extension())
        {
            open_file.replace_extension(ext);
        }
        if (!fs::exists(open_file))
        {
            name = open_file.string();
            return;
        }
        // file already exists
        if (!g_overwrite_file)
        {
            update_save_name(name);
        }
    } while (!g_overwrite_file);
}

} // namespace id::io
