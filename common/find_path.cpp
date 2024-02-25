#include "find_path.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "fractint.h"
#include "id_data.h"
#include "make_path.h"
#include "prompts2.h"
#include "search_path.h"

#ifdef WIN32
#include <corecrt_io.h>
#else
#include <unistd.h>
#endif

#include <filesystem>

namespace fs = std::filesystem;

/*
 *----------------------------------------------------------------------
 *
 * findpath --
 *
 *      Find where a file is.
 *  We return filename if it is an absolute path.
 *  Otherwise, we first try FRACTDIR/filename, SRCDIR/filename,
 *      and then ./filename.
 *
 * Results:
 *      Returns full pathname in fullpathname.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
std::string find_path(const char *filename,
    const std::function<const char *(const char *)> &get_env) // return full pathnames
{
    char fullpathname[FILE_MAX_PATH];
    fullpathname[0] = 0;                         // indicate none found

    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char temp_path[FILE_MAX_PATH];

    // check current directory if curdir= parameter set
    split_fname_ext(filename, fname, ext);
    make_path(temp_path, "", "", fname, ext);
    if (g_check_cur_dir && fs::exists(temp_path))   // file exists
    {
        return (fs::current_path() / filename).make_preferred().string();
    }

    // check for absolute path
    //if (fs::path{filename}.is_absolute() && )
    //{
    //}
    std::strcpy(temp_path, filename);   // avoid side effect changes to filename
    if (temp_path[0] == SLASHC || (temp_path[0] && temp_path[1] == ':'))
    {
        if (access(temp_path, 0) == 0)   // file exists
        {
            std::strcpy(fullpathname, temp_path);
            return fullpathname;
        }

        split_fname_ext(temp_path, fname, ext);
        make_path(temp_path, "", "", fname, ext);
    }

    // check FRACTDIR
    make_path(temp_path, "", g_fractal_search_dir1.c_str(), fname, ext);
    if (access(temp_path, 0) == 0)
    {
        std::strcpy(fullpathname, temp_path);
        return fullpathname;
    }

    // check SRCDIR
    make_path(temp_path, "", g_fractal_search_dir2.c_str(), fname, ext);
    if (access(temp_path, 0) == 0)
    {
        std::strcpy(fullpathname, temp_path);
        return fullpathname;
    }

    // check PATH
    return search_path(temp_path, "PATH", get_env);
}
