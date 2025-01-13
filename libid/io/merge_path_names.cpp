// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/merge_path_names.h"

#include "io/expand_dirname.h"
#include "io/find_file.h"
#include "io/fix_dirname.h"
#include "io/is_directory.h"
#include "io/make_path.h"
#include "io/split_path.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"

#include <config/port.h>

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
int merge_path_names(char *old_full_path, char const *new_filename, CmdFile mode)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, fs::path(new_filename).make_preferred().string().c_str());

    // no dot or slash so assume a file
    bool is_a_file = std::strchr(buff, '.') == nullptr
        && std::strchr(buff, SLASH_CH) == nullptr;
    bool is_a_dir = is_a_directory(buff);
    if (is_a_dir)
    {
        fix_dir_name(buff);
    }

    // if dot, slash, NUL, it's the current directory, set up full path
    if (buff[0] == '.' && buff[1] == SLASH_CH && buff[2] == 0)
    {
        char temp_drive[FILE_MAX_PATH];
        expand_dir_name(buff, temp_drive);
        std::strcat(temp_drive, buff);
        std::strcpy(buff, temp_drive);
        is_a_dir = true;
    }

    // if dot, slash, its relative to the current directory, set up full path
    if (buff[0] == '.' && buff[1] == SLASH_CH)
    {
        bool test_dir = false;
        char temp_drive[FILE_MAX_PATH];
        if (std::strrchr(buff, '.') == buff)
        {
            test_dir = true;    // only one '.' assume it's a directory
        }
        expand_dir_name(buff, temp_drive);
        std::strcat(temp_drive, buff);
        std::strcpy(buff, temp_drive);
        if (!test_dir)
        {
            int len = (int) std::strlen(buff);
            buff[len-1] = 0; // get rid of slash added by expand_dirname
        }
    }

    // check existence
    if (!is_a_dir || is_a_file)
    {
        if (fr_find_first(buff) == 0)
        {
            if (g_dta.attribute & SUB_DIR) // exists and is dir
            {
                fix_dir_name(buff);  // add trailing slash
                is_a_dir = true;
                is_a_file = false;
            }
            else
            {
                is_a_file = true;
            }
        }
    }

    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    split_path(buff, drive, dir, fname, ext);

    char drive1[FILE_MAX_DRIVE];
    char dir1[FILE_MAX_DIR];
    char fname1[FILE_MAX_FNAME];
    char ext1[FILE_MAX_EXT];
    split_path(old_full_path, drive1, dir1, fname1, ext1);

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
    bool is_a_dir_error = false;
    if (!is_a_dir && !is_a_file && get_path)
    {
        make_drive_dir(old_full_path, drive1, dir1);
        int len = (int) std::strlen(old_full_path);
        if (len > 0)
        {
            // strip trailing slash
            char save = old_full_path[len - 1];
            if (save == SLASH_CH)
            {
                old_full_path[len-1] = 0;
            }
            if (!fs::exists(old_full_path))
            {
                is_a_dir_error = true;
            }
            old_full_path[len-1] = save;
        }
    }
    make_path(old_full_path, drive1, dir1, fname1, ext1);
    return is_a_dir_error ? -1 : (is_a_dir ? 1 : 0);
}

int merge_path_names(std::string &old_full_path, char const *new_filename, CmdFile mode)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, old_full_path.c_str());
    int const result = merge_path_names(buff, new_filename, mode);
    old_full_path = buff;
    return result;
}
