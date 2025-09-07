// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/find_path.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "io/search_path.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace id::io
{

// find_path
//
//      Find where a file is.
//
std::string find_path(const char *filename,
    const std::function<const char *(const char *)> &get_env) // return full pathnames
{
    const fs::path file_path{fs::path{filename}.make_preferred()};

    // check current directory if curdir= parameter set
    if (g_check_cur_dir && fs::exists(file_path.filename()))   // file exists
    {
        return (fs::current_path() / file_path.filename()).make_preferred().string();
    }

    // check for absolute path
    if (file_path.is_absolute() && fs::exists(file_path)) // file exists
    {
        return file_path.string();
    }

    const auto check_dir = [&](const fs::path &dir)
    {
        fs::path check_path{dir / file_path};
        return fs::exists(check_path) ? check_path.make_preferred().string() : std::string{};
    };

    // check FRACTDIR
    std::string dir1_path{check_dir(g_fractal_search_dir1)};
    if (!dir1_path.empty())
    {
        return dir1_path;
    }

    // check SRCDIR
    std::string dir2_path{check_dir(g_fractal_search_dir2)};
    if (!dir2_path.empty())
    {
        return dir2_path;
    }

    // check PATH
    return search_path(file_path.filename().string().c_str(), "PATH", get_env);
}

} // namespace id::io
