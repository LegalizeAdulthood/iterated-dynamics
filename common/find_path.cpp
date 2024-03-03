#include "find_path.h"

#include "port.h"
#include "fractint.h"

#include "cmdfiles.h"
#include "id_data.h"
#include "search_path.h"

#ifdef WIN32
#include <corecrt_io.h>
#else
#include <unistd.h>
#endif

#include <filesystem>

namespace fs = std::filesystem;

// find_path
//
//      Find where a file is.
//
std::string find_path(const char *filename,
    const std::function<const char *(const char *)> &get_env) // return full pathnames
{
    const fs::path file_path{fs::path{filename}.make_preferred()};

    // check current directory if curdir= parameter set
    if (g_check_cur_dir && exists(file_path.filename()))   // file exists
    {
        return (fs::current_path() / file_path.filename()).make_preferred().string();
    }

    // check for absolute path
    if (file_path.is_absolute() && exists(file_path)) // file exists
    {
        return file_path.string();
    }

    const auto check_dir = [&](const std::string &dir)
    {
        fs::path check_path{fs::path{dir} / file_path};
        return exists(check_path) ? check_path.make_preferred().string() : std::string{};
    };

    // check FRACTDIR
    const std::string dir1_path{check_dir(g_fractal_search_dir1)};
    if (!dir1_path.empty())
    {
        return dir1_path;
    }

    // check SRCDIR
    const std::string dir2_path{check_dir(g_fractal_search_dir2)};
    if (!dir2_path.empty())
    {
        return dir2_path;
    }

    // check PATH
    return search_path(file_path.filename().string().c_str(), "PATH", get_env);
}
