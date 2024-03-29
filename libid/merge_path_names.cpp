#include "merge_path_names.h"

#include "port.h"
#include "prototyp.h"

#include "expand_dirname.h"
#include "find_file.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "id.h"
#include "is_directory.h"
#include "make_path.h"
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
int merge_pathnames(char *oldfullpath, char const *new_filename, cmd_file mode)
{
    char newfilename[FILE_MAX_PATH];
    std::strcpy(newfilename, fs::path(new_filename).make_preferred().string().c_str());

    // no dot or slash so assume a file
    bool isafile = std::strchr(newfilename, '.') == nullptr
        && std::strchr(newfilename, SLASHC) == nullptr;
    bool isadir = isadirectory(newfilename);
    if (isadir)
    {
        fix_dirname(newfilename);
    }

    // if dot, slash, NUL, it's the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASHC && newfilename[2] == 0)
    {
        char temp_drive[FILE_MAX_PATH];
        expand_dirname(newfilename, temp_drive);
        std::strcat(temp_drive, newfilename);
        std::strcpy(newfilename, temp_drive);
        isadir = true;
    }

    // if dot, slash, its relative to the current directory, set up full path
    if (newfilename[0] == '.' && newfilename[1] == SLASHC)
    {
        bool test_dir = false;
        char temp_drive[FILE_MAX_PATH];
        if (std::strrchr(newfilename, '.') == newfilename)
        {
            test_dir = true;    // only one '.' assume it's a directory
        }
        expand_dirname(newfilename, temp_drive);
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

    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    splitpath(newfilename, drive, dir, fname, ext);

    char drive1[FILE_MAX_DRIVE];
    char dir1[FILE_MAX_DIR];
    char fname1[FILE_MAX_FNAME];
    char ext1[FILE_MAX_EXT];
    splitpath(oldfullpath, drive1, dir1, fname1, ext1);

    bool const get_path = (mode == cmd_file::AT_CMD_LINE) || (mode == cmd_file::SSTOOLS_INI);
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
            if (save == SLASHC)
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

int merge_pathnames(std::string &oldfullpath, char const *new_filename, cmd_file mode)
{
    char buff[FILE_MAX_PATH];
    std::strcpy(buff, oldfullpath.c_str());
    int const result = merge_pathnames(buff, new_filename, mode);
    oldfullpath = buff;
    return result;
}
