// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_a_filename.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "expand_dirname.h"
#include "find_file.h"
#include "find_path.h"
#include "fix_dirname.h"
#include "full_screen_choice.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "is_directory.h"
#include "loadfile.h"
#include "make_path.h"
#include "put_string_center.h"
#include "shell_sort.h"
#include "split_path.h"
#include "stereo.h"
#include "trim_filename.h"

#include <cstring>

// speed key state values
enum
{
    MATCHING = 0,   // string matches list - speed key mode
    TEMPLATE = -2,  // wild cards present - building template
    SEARCH_PATH = -3 // no match - building path search name
};

enum
{
    MAX_NUM_FILES = 2977L
};

namespace
{

struct Choice
{
    char full_name[FILE_MAX_PATH];
    bool subdir;
};

} // namespace

static int check_f6_key(int curkey, int choice);
static int filename_speed_str(int row, int col, int vid, char const *speedstring, int speed_match);

static int s_speed_state{};
static char const *s_masks[]{"*.pot", "*.gif"};

bool get_a_file_name(char const *hdg, char const *file_template, std::string &flname)
{
    char user_file_template[FILE_MAX_PATH] = { 0 };
    int rds;  // if getting an RDS image map
    char instr[80];
    int masklen;
    char filename[FILE_MAX_PATH];
    char speedstr[81];
    char tmpmask[FILE_MAX_PATH];   // used to locate next file in list
    char old_flname[FILE_MAX_PATH];
    int out;
    int retried;
    // Only the first 13 characters of file names are displayed...
    Choice storage[MAX_NUM_FILES];
    Choice *choices[MAX_NUM_FILES];
    int attributes[MAX_NUM_FILES];
    int filecount;   // how many files
    int dircount;    // how many directories
    bool notroot;     // not the root directory
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];

    static int numtemplates = 1;
    static bool dosort = true;

    rds = (g_stereo_map_filename == flname) ? 1 : 0;
    for (int i = 0; i < MAX_NUM_FILES; i++)
    {
        attributes[i] = 1;
        choices[i] = &storage[i];
    }
    // save filename
    std::strcpy(old_flname, flname.c_str());

restart:  // return here if template or directory changes
    tmpmask[0] = 0;
    if (flname[0] == 0)
    {
        flname = DOT_SLASH;
    }
    split_path(flname , drive, dir, fname, ext);
    make_fname_ext(filename, fname, ext);
    retried = 0;

retry_dir:
    if (dir[0] == 0)
    {
        std::strcpy(dir, ".");
    }
    expand_dir_name(dir, drive);
    make_drive_dir(tmpmask, drive, dir);
    fix_dir_name(tmpmask);
    if (retried == 0 && std::strcmp(dir, SLASH) && std::strcmp(dir, DOT_SLASH))
    {
        int j = (int) std::strlen(tmpmask) - 1;
        tmpmask[j] = 0; // strip trailing backslash
        if (std::strchr(tmpmask, '*') || std::strchr(tmpmask, '?')
            || fr_find_first(tmpmask) != 0
            || (g_dta.attribute & SUB_DIR) == 0)
        {
            std::strcpy(dir, DOT_SLASH);
            ++retried;
            goto retry_dir;
        }
        tmpmask[j] = SLASH_CH;
    }
    if (file_template[0])
    {
        numtemplates = 1;
        split_fname_ext(file_template, fname, ext);
    }
    else
    {
        numtemplates = sizeof(s_masks)/sizeof(s_masks[0]);
    }
    filecount = -1;
    dircount  = 0;
    notroot   = false;
    masklen = (int) std::strlen(tmpmask);
    std::strcat(tmpmask, "*.*");
    out = fr_find_first(tmpmask);
    while (out == 0 && filecount < MAX_NUM_FILES)
    {
        if ((g_dta.attribute & SUB_DIR) && g_dta.filename != ".")
        {
            if (g_dta.filename != "..")
            {
                g_dta.filename += SLASH;
            }
            std::strcpy(choices[++filecount]->full_name, g_dta.filename.c_str());
            choices[filecount]->subdir = true;
            dircount++;
            if (g_dta.filename == "..")
            {
                notroot = true;
            }
        }
        out = fr_find_next();
    }
    tmpmask[masklen] = 0;
    if (file_template[0])
    {
        make_path(tmpmask, drive, dir, fname, ext);
    }
    int j = 0;
    do
    {
        if (numtemplates > 1)
        {
            std::strcpy(&(tmpmask[masklen]), s_masks[j]);
        }
        out = fr_find_first(tmpmask);
        while (out == 0 && filecount < MAX_NUM_FILES)
        {
            if (!(g_dta.attribute & SUB_DIR))
            {
                if (rds)
                {
                    put_string_center(2, 0, 80, C_GENERAL_INPUT, g_dta.filename.c_str());

                    split_fname_ext(g_dta.filename, fname, ext);
                    make_path(choices[++filecount]->full_name, drive, dir, fname, ext);
                    choices[filecount]->subdir = false;
                }
                else
                {
                    std::strcpy(choices[++filecount]->full_name, g_dta.filename.c_str());
                    choices[filecount]->subdir = false;
                }
            }
            out = fr_find_next();
        }
    }
    while (++j < numtemplates);
    if (++filecount == 0)
    {
        std::strcpy(choices[filecount]->full_name, "*nofiles*");
        choices[filecount]->subdir = false;
        ++filecount;
    }

    std::strcpy(instr, "Press F6 for default directory, F4 to toggle sort ");
    if (dosort)
    {
        std::strcat(instr, "off");
        shell_sort(&choices, filecount, sizeof(Choice *)); // sort file list
    }
    else
    {
        std::strcat(instr, "on");
    }
    if (!notroot && dir[0] && dir[0] != SLASH_CH) // must be in root directory
    {
        split_path(tmpmask, drive, dir, fname, ext);
        std::strcpy(dir, SLASH);
        make_path(tmpmask, drive, dir, fname, ext);
    }
    if (numtemplates > 1)
    {
        std::strcat(tmpmask, " ");
        std::strcat(tmpmask, s_masks[0]);
    }

    std::string const heading{std::string{hdg} + "\n"
        + "Template: " + trim_file_name(tmpmask, 66)};
    std::strcpy(speedstr, filename);
    int i = 0;
    if (speedstr[0] == 0)
    {
        for (i = 0; i < filecount; i++) // find first file
        {
            if (!choices[i]->subdir)
            {
                break;
            }
        }
        if (i >= filecount)
        {
            i = 0;
        }
    }

    i = full_screen_choice(ChoiceFlags::INSTRUCTIONS | (dosort ? ChoiceFlags::NONE : ChoiceFlags::NOT_SORTED),
        heading.c_str(), nullptr, instr, filecount, (char const **) choices, attributes, 0, 99, 0, i, nullptr,
        speedstr, filename_speed_str, check_f6_key);
    if (i == -ID_KEY_F4)
    {
        dosort = !dosort;
        goto restart;
    }
    if (i == -ID_KEY_F6)
    {
        static int lastdir = 0;
        if (lastdir == 0)
        {
            std::strcpy(dir, g_fractal_search_dir1.c_str());
        }
        else
        {
            std::strcpy(dir, g_fractal_search_dir2.c_str());
        }
        fix_dir_name(dir);
        flname = make_drive_dir(drive, dir);
        lastdir = 1 - lastdir;
        goto restart;
    }
    if (i < 0)
    {
        // restore filename
        flname = old_flname;
        return true;
    }
    if (speedstr[0] == 0 || s_speed_state == MATCHING)
    {
        if (choices[i]->subdir)
        {
            if (std::strcmp(choices[i]->full_name, "..") == 0) // go up a directory
            {
                if (std::strcmp(dir, DOT_SLASH) == 0)
                {
                    std::strcpy(dir, DOT_DOT_SLASH);
                }
                else
                {
                    char *s = std::strrchr(dir, SLASH_CH);
                    if (s != nullptr) // trailing slash
                    {
                        *s = 0;
                        s = std::strrchr(dir, SLASH_CH);
                        if (s != nullptr)
                        {
                            *(s + 1) = 0;
                        }
                    }
                }
            }
            else  // go down a directory
            {
                std::strcat(dir, choices[i]->full_name);
            }
            fix_dir_name(dir);
            flname = make_drive_dir(drive, dir);
            goto restart;
        }
        split_fname_ext(choices[i]->full_name, fname, ext);
        flname = make_path(drive, dir, fname, ext);
    }
    else
    {
        if (s_speed_state == SEARCH_PATH
            && std::strchr(speedstr, '*') == nullptr && std::strchr(speedstr, '?') == nullptr
            && ((fr_find_first(speedstr) == 0 && (g_dta.attribute & SUB_DIR))
                || std::strcmp(speedstr, SLASH) == 0)) // it is a directory
        {
            s_speed_state = TEMPLATE;
        }

        if (s_speed_state == TEMPLATE)
        {
            /* extract from tempstr the pathname and template information,
                being careful not to overwrite drive and directory if not
                newly specified */
            char drive1[FILE_MAX_DRIVE];
            char dir1[FILE_MAX_DIR];
            char fname1[FILE_MAX_FNAME];
            char ext1[FILE_MAX_EXT];
            split_path(speedstr, drive1, dir1, fname1, ext1);
            if (drive1[0])
            {
                std::strcpy(drive, drive1);
            }
            if (dir1[0])
            {
                std::strcpy(dir, dir1);
            }
            flname = make_path(drive, dir, fname1, ext1);
            if (std::strchr(fname1, '*') || std::strchr(fname1, '?') ||
                    std::strchr(ext1,   '*') || std::strchr(ext1,   '?'))
            {
                make_fname_ext(user_file_template, fname1, ext1);
                // cppcheck-suppress uselessAssignmentPtrArg
                file_template = user_file_template;
            }
            else if (is_a_directory(flname.c_str()))
            {
                fix_dir_name(flname);
            }
            goto restart;
        }
        else // speedstate == SEARCHPATH
        {
            const std::string full_path{find_path(speedstr)};
            if (!full_path.empty())
            {
                flname = full_path;
            }
            else
            {
                // failed, make diagnostic useful:
                flname = speedstr;
                if (std::strchr(speedstr, SLASH_CH) == nullptr)
                {
                    split_fname_ext(speedstr, fname, ext);
                    flname = make_path(drive, dir, fname, ext);
                }
            }
        }
    }
    g_browse_name = std::string{fname} + ext;
    return false;
}

// choice is used by other routines called by fullscreen_choice()
static int check_f6_key(int curkey, int /*choice*/)
{
    if (curkey == ID_KEY_F6)
    {
        return 0-ID_KEY_F6;
    }
    else if (curkey == ID_KEY_F4)
    {
        return 0-ID_KEY_F4;
    }
    return 0;
}

static int filename_speed_str(int row, int col, int vid, char const *speedstring, int speed_match)
{
    char const *prompt;
    if (std::strchr(speedstring, ':')
        || std::strchr(speedstring, '*') || std::strchr(speedstring, '*')
        || std::strchr(speedstring, '?'))
    {
        s_speed_state = TEMPLATE;  // template
        prompt = "File Template";
    }
    else if (speed_match)
    {
        s_speed_state = SEARCH_PATH; // does not match list
        prompt = "Search Path for";
    }
    else
    {
        s_speed_state = MATCHING;
        prompt = g_speed_prompt.c_str();
    }
    driver_put_string(row, col, vid, prompt);
    return (int) std::strlen(prompt);
}
