// SPDX-License-Identifier: GPL-3.0-only
//
#include "merge_path_names.h"

#include "cmdfiles.h"
#include "expand_dirname.h"
#include "find_file.h"
#include "fix_dirname.h"
#include "id.h"
#include "is_directory.h"
#include "make_path.h"
#include "port.h"
#include "split_path.h"

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
int merge_path_names(char *oldfullpath, char const *new_filename, CmdFile mode)
{
    char newfilename[FILE_MAX_PATH];
    std::strcpy(newfilename, fs::path(new_filename).make_preferred().string().c_str());

    // no dot or slash so assume a file
    bool isafile = std::strchr(newfilename, '.') == nullptr
        && std::strchr(newfilename, SLASH_CH) == nullptr;
    bool isadir = is_a_directory(newfilename);
    if (isadir)
    {
        fix_dir_name(newfilename);
    }

    // if dot, slash, NUL, it's the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASH_CH && newfilename[2] == 0)
    {
        char temp_drive[FILE_MAX_PATH];
        expand_dir_name(newfilename, temp_drive);
        std::strcat(temp_drive, newfilename);
        std::strcpy(newfilename, temp_drive);
        isadir = true;
    }

    // if dot, slash, its relative to the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASH_CH)
    {
        bool test_dir = false;
        char temp_drive[FILE_MAX_PATH];
        if (std::strrchr(newfilename, '.') == newfilename)
        {
            test_dir = true;    // only one '.' assume it's a directory
        }
        expand_dir_name(newfilename, temp_drive);
        std::strcat(temp_drive, newfilename);
        std::strcpy(newfilename, temp_drive);
        if (!test_dir)
        {
            int len = (int) std::strlen(newfilename);
            newfilename[len-1] = 0; // get rid of slash added by expand_dirname
        }
    }

    // check existence
    if (!isadir || isafile)
    {
        if (fr_find_first(newfilename) == 0)
        {
            if (g_dta.attribute & SUB_DIR) // exists and is dir
            {
                fix_dir_name(newfilename);  // add trailing slash
                isadir = true;
                isafile = false;
            }
            else
            {
                isafile = true;
            }
        }
    }

    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    split_path(newfilename, drive, dir, fname, ext);

    char drive1[FILE_MAX_DRIVE];
    char dir1[FILE_MAX_DIR];
    char fname1[FILE_MAX_FNAME];
    char ext1[FILE_MAX_EXT];
    split_path(oldfullpath, drive1, dir1, fname1, ext1);

    bool const get_path = (mode == CmdFile::AT_CMD_LINE) || (mode == CmdFile::SSTOOLS_INI);
    if (get_path)
    {
        if ((int) std::strlen(drive) != 0)
        {
            std::strcpy(drive1, drive);
        }
        if ((int) std::strlen(dir) != 0)
        {
            std::strcpy(dir1, dir);
        }
    }
    if ((int) std::strlen(fname) != 0)
    {
        std::strcpy(fname1, fname);
    }
    if ((int) std::strlen(ext) != 0)
    {
        std::strcpy(ext1, ext);
    }
    bool isadir_error = false;
    if (!isadir && !isafile && get_path)
    {
        make_drive_dir(oldfullpath, drive1, dir1);
        int len = (int) std::strlen(oldfullpath);
        if (len > 0)
        {
            char save;
            // strip trailing slash
            save = oldfullpath[len-1];
            if (save == SLASH_CH)
            {
                oldfullpath[len-1] = 0;
            }
            if (!fs::exists(oldfullpath))
            {
                isadir_error = true;
            }
            oldfullpath[len-1] = save;
        }
    }
    make_path(oldfullpath, drive1, dir1, fname1, ext1);
    return isadir_error ? -1 : (isadir ? 1 : 0);
}

int merge_path_names(std::string &oldfullpath, char const *new_filename, CmdFile mode)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, oldfullpath.c_str());
    int const result = merge_path_names(buff, new_filename, mode);
    oldfullpath = buff;
    return result;
}
