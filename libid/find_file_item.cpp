#include "find_file_item.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "find_file.h"
#include "id.h"
#include "make_path.h"
#include "prompts1.h"
#include "split_path.h"
#include "stop_msg.h"
#include "temp_msg.h"

#include <cassert>

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, int itemtype)
{
    std::FILE *infile = nullptr;
    bool found = false;
    char parsearchname[ITEM_NAME_LEN + 6];
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char fullpath[FILE_MAX_PATH];
    char defaultextension[5];


    splitpath(filename, drive, dir, fname, ext);
    make_fname_ext(fullpath, fname, ext);
    if (stricmp(filename, g_command_file.c_str()))
    {
        infile = std::fopen(filename, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }

        if (!found && g_check_cur_dir)
        {
            make_path(fullpath, "", DOTSLASH, fname, ext);
            infile = std::fopen(fullpath, "rb");
            if (infile != nullptr)
            {
                if (scan_entries(infile, nullptr, itemname) == -1)
                {
                    std::strcpy(filename, fullpath);
                    found = true;
                }
                else
                {
                    std::fclose(infile);
                    infile = nullptr;
                }
            }
        }
    }

    switch (itemtype)
    {
    case 1:
        std::strcpy(parsearchname, "frm:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".frm");
        split_drive_dir(g_search_for.frm, drive, dir);
        break;
    case 2:
        std::strcpy(parsearchname, "lsys:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".l");
        split_drive_dir(g_search_for.lsys, drive, dir);
        break;
    case 3:
        std::strcpy(parsearchname, "ifs:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".ifs");
        split_drive_dir(g_search_for.ifs, drive, dir);
        break;
    default:
        std::strcpy(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".par");
        split_drive_dir(g_search_for.par, drive, dir);
        break;
    }

    if (!found)
    {
        infile = std::fopen(g_command_file.c_str(), "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, parsearchname) == -1)
            {
                std::strcpy(filename, g_command_file.c_str());
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        make_path(fullpath, drive, dir, fname, ext);
        infile = std::fopen(fullpath, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                std::strcpy(filename, fullpath);
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        // search for file
        int out;
        make_path(fullpath, drive, dir, "*", defaultextension);
        out = fr_findfirst(fullpath);
        while (out == 0)
        {
            char msg[200];
            std::sprintf(msg, "Searching %13s for %s      ", DTA.filename.c_str(), itemname);
            showtempmsg(msg);
            if (!(DTA.attribute & SUBDIR)
                && DTA.filename != "."
                && DTA.filename != "..")
            {
                split_fname_ext(DTA.filename, fname, ext);
                make_path(fullpath, drive, dir, fname, ext);
                infile = std::fopen(fullpath, "rb");
                if (infile != nullptr)
                {
                    if (scan_entries(infile, nullptr, itemname) == -1)
                    {
                        std::strcpy(filename, fullpath);
                        found = true;
                        break;
                    }
                    else
                    {
                        std::fclose(infile);
                        infile = nullptr;
                    }
                }
            }
            out = fr_findnext();
        }
        cleartempmsg();
    }

    if (!found && g_organize_formulas_search && itemtype == 1)
    {
        split_drive_dir(g_organize_formulas_dir, drive, dir);
        fname[0] = '_';
        fname[1] = (char) 0;
        if (std::isalpha(itemname[0]))
        {
            if (strnicmp(itemname, "carr", 4))
            {
                fname[1] = itemname[0];
                fname[2] = (char) 0;
            }
            else if (std::isdigit(itemname[4]))
            {
                std::strcat(fname, "rc");
                fname[3] = itemname[4];
                fname[4] = (char) 0;
            }
            else
            {
                std::strcat(fname, "rc");
            }
        }
        else if (std::isdigit(itemname[0]))
        {
            std::strcat(fname, "num");
        }
        else
        {
            std::strcat(fname, "chr");
        }
        make_path(fullpath, drive, dir, fname, defaultextension);
        infile = std::fopen(fullpath, "rb");
        if (infile != nullptr)
        {
            if (scan_entries(infile, nullptr, itemname) == -1)
            {
                std::strcpy(filename, fullpath);
                found = true;
            }
            else
            {
                std::fclose(infile);
                infile = nullptr;
            }
        }
    }

    if (!found)
    {
        std::sprintf(fullpath, "'%s' file entry item not found", itemname);
        stopmsg(STOPMSG_NONE, fullpath);
        return true;
    }
    // found file
    if (fileptr != nullptr)
    {
        *fileptr = infile;
    }
    else if (infile != nullptr)
    {
        std::fclose(infile);
    }
    return false;
}

bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, int itemtype)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    bool const result = find_file_item(buf, itemname, fileptr, itemtype);
    filename = buf;
    return result;
}
