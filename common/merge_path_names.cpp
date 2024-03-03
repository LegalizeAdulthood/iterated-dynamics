/*
        Various routines that prompt for things.
*/
#include "merge_path_names.h"

#include "port.h"
#include "prototyp.h"

#include "expand_dirname.h"
#include "find_file.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "is_directory.h"
#include "make_path.h"
#include "split_path.h"

#ifdef WIN32
#include <direct.h>
#include <corecrt_io.h>
#else
#include <unistd.h>
#endif

#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// merge existing full path with new one
// attempt to detect if file or directory

// copies the proposed new filename to the fullpath variable
// does not copy directories for PAR files
// (modes AT_AFTER_STARTUP and AT_CMD_LINE_SET_NAME)
// attempts to extract directory and test for existence
// (modes AT_CMD_LINE and SSTOOLS_INI)
int merge_pathnames(char *oldfullpath, char const *new_filename, cmd_file mode)
{
    char newfilename[FILE_MAX_PATH];
    std::strcpy(newfilename, new_filename);
    bool isadir_error = false;
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char temp_path[FILE_MAX_PATH];

    char drive1[FILE_MAX_DRIVE];
    char dir1[FILE_MAX_DIR];
    char fname1[FILE_MAX_FNAME];
    char ext1[FILE_MAX_EXT];

    // no dot or slash so assume a file
    bool isafile = std::strchr(newfilename, '.') == nullptr
        && std::strchr(newfilename, SLASHC) == nullptr;
    bool isadir = isadirectory(newfilename);
    if (isadir)
    {
        fix_dirname(newfilename);
    }
#ifndef XFRACT
    // if drive, colon, slash, is a directory
    if ((int) std::strlen(newfilename) == 3 && newfilename[1] == ':' && newfilename[2] == SLASHC)
    {
        isadir = true;
    }
    // if drive, colon, with no slash, is a directory
    if ((int) std::strlen(newfilename) == 2 && newfilename[1] == ':')
    {
        newfilename[2] = SLASHC;
        newfilename[3] = 0;
        isadir = true;
    }
    // if dot, slash, '0', its the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASHC && newfilename[2] == 0)
    {
        temp_path[0] = (char)('a' + _getdrive() - 1);
        temp_path[1] = ':';
        temp_path[2] = 0;
        expand_dirname(newfilename, temp_path);
        std::strcat(temp_path, newfilename);
        std::strcpy(newfilename, temp_path);
        isadir = true;
    }
    // if dot, slash, its relative to the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASHC)
    {
        bool test_dir = false;
        temp_path[0] = (char)('a' + _getdrive() - 1);
        temp_path[1] = ':';
        temp_path[2] = 0;
        if (std::strrchr(newfilename, '.') == newfilename)
        {
            test_dir = true;    // only one '.' assume its a directory
        }
        expand_dirname(newfilename, temp_path);
        std::strcat(temp_path, newfilename);
        std::strcpy(newfilename, temp_path);
        if (!test_dir)
        {
            int len = (int) std::strlen(newfilename);
            newfilename[len-1] = 0; // get rid of slash added by expand_dirname
        }
    }
#else
    std::strcpy(newfilename, find_path(newfilename).c_str());
#endif
    // check existence
    if (!isadir || isafile)
    {
        if (fr_findfirst(newfilename) == 0)
        {
            if (DTA.attribute & SUBDIR) // exists and is dir
            {
                fix_dirname(newfilename);  // add trailing slash
                isadir = true;
                isafile = false;
            }
            else
            {
                isafile = true;
            }
        }
    }

    splitpath(newfilename, drive, dir, fname, ext);
    splitpath(oldfullpath, drive1, dir1, fname1, ext1);
    bool const get_path = (mode == cmd_file::AT_CMD_LINE) || (mode == cmd_file::SSTOOLS_INI);
    if ((int) std::strlen(drive) != 0 && get_path)
    {
        std::strcpy(drive1, drive);
    }
    if ((int) std::strlen(dir) != 0 && get_path)
    {
        std::strcpy(dir1, dir);
    }
    if ((int) std::strlen(fname) != 0)
    {
        std::strcpy(fname1, fname);
    }
    if ((int) std::strlen(ext) != 0)
    {
        std::strcpy(ext1, ext);
    }
    if (!isadir && !isafile && get_path)
    {
        make_path(oldfullpath, drive1, dir1, nullptr, nullptr);
        int len = (int) std::strlen(oldfullpath);
        if (len > 0)
        {
            char save;
            // strip trailing slash
            save = oldfullpath[len-1];
            if (save == SLASHC)
            {
                oldfullpath[len-1] = 0;
            }
            if (access(oldfullpath, 0))
            {
                isadir_error = true;
            }
            oldfullpath[len-1] = save;
        }
    }
    make_path(oldfullpath, drive1, dir1, fname1, ext1);
    return isadir_error ? -1 : (isadir ? 1 : 0);
}

int merge_pathnames(std::string &oldfullpath, char const *new_filename, cmd_file mode)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, oldfullpath.c_str());
    int const result = merge_pathnames(buff, new_filename, mode);
    oldfullpath = buff;
    return result;
}
