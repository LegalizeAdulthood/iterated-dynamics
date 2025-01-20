// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_a_filename.h"

#include "engine/id_data.h"
#include "io/expand_dirname.h"
#include "io/find_file.h"
#include "io/find_path.h"
#include "io/fix_dirname.h"
#include "io/is_directory.h"
#include "io/loadfile.h"
#include "io/make_path.h"
#include "io/split_path.h"
#include "io/trim_filename.h"
#include "misc/drivers.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"
#include "ui/full_screen_choice.h"
#include "ui/id_keys.h"
#include "ui/put_string_center.h"
#include "ui/shell_sort.h"
#include "ui/stereo.h"

#include <config/path_limits.h>

#include <cstring>
#include <iterator>

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
    char full_name[ID_FILE_MAX_PATH];
    bool sub_dir;
};

} // namespace

static int check_f6_key(int key, int choice);
static int filename_speed_str(int row, int col, int vid, char const *speed_string, int speed_match);

static int s_speed_state{};
static char const *s_masks[]{"*.pot", "*.gif"};

bool get_a_file_name(char const *hdg, char const *file_template, std::string &result_filename)
{
    char user_file_template[ID_FILE_MAX_PATH]{};
    // if getting an RDS image map
    char instr[80];
    char filename[ID_FILE_MAX_PATH];
    char speed_str[81];
    char tmp_mask[ID_FILE_MAX_PATH];   // used to locate next file in list
    char old_file_name[ID_FILE_MAX_PATH];
    // Only the first 13 characters of file names are displayed...
    Choice storage[MAX_NUM_FILES];
    Choice *choices[MAX_NUM_FILES];
    int attributes[MAX_NUM_FILES];
    // how many files
    // how many directories
    // not the root directory
    char drive[ID_FILE_MAX_DRIVE];
    char dir[ID_FILE_MAX_DIR];
    char fname[ID_FILE_MAX_FNAME];
    char ext[ID_FILE_MAX_EXT];

    static int num_templates = 1;
    static bool do_sort = true;

    bool rds = g_stereo_map_filename == result_filename;
    for (int i = 0; i < MAX_NUM_FILES; i++)
    {
        attributes[i] = 1;
        choices[i] = &storage[i];
    }
    // save filename
    std::strcpy(old_file_name, result_filename.c_str());

restart:  // return here if template or directory changes
    tmp_mask[0] = 0;
    if (result_filename[0] == 0)
    {
        result_filename = DOT_SLASH;
    }
    split_path(result_filename , drive, dir, fname, ext);
    make_fname_ext(filename, fname, ext);
    int retried = 0;

retry_dir:
    if (dir[0] == 0)
    {
        std::strcpy(dir, ".");
    }
    expand_dir_name(dir, drive);
    make_drive_dir(tmp_mask, drive, dir);
    fix_dir_name(tmp_mask);
    if (retried == 0 && std::strcmp(dir, SLASH) != 0 && std::strcmp(dir, DOT_SLASH) != 0)
    {
        int j = (int) std::strlen(tmp_mask) - 1;
        tmp_mask[j] = 0; // strip trailing backslash
        if (std::strchr(tmp_mask, '*') || std::strchr(tmp_mask, '?')
            || fr_find_first(tmp_mask) != 0
            || (g_dta.attribute & SUB_DIR) == 0)
        {
            std::strcpy(dir, DOT_SLASH);
            ++retried;
            goto retry_dir;
        }
        tmp_mask[j] = SLASH_CH;
    }
    if (file_template[0])
    {
        num_templates = 1;
        split_fname_ext(file_template, fname, ext);
    }
    else
    {
        num_templates = std::size(s_masks);
    }
    int file_count = -1;
    int dir_count = 0;
    bool not_root = false;
    int mask_len = (int) std::strlen(tmp_mask);
    std::strcat(tmp_mask, "*.*");
    int out = fr_find_first(tmp_mask);
    while (out == 0 && file_count < MAX_NUM_FILES)
    {
        if ((g_dta.attribute & SUB_DIR) && g_dta.filename != ".")
        {
            if (g_dta.filename != "..")
            {
                g_dta.filename += SLASH;
            }
            std::strcpy(choices[++file_count]->full_name, g_dta.filename.c_str());
            choices[file_count]->sub_dir = true;
            dir_count++;
            if (g_dta.filename == "..")
            {
                not_root = true;
            }
        }
        out = fr_find_next();
    }
    tmp_mask[mask_len] = 0;
    if (file_template[0])
    {
        make_path(tmp_mask, drive, dir, fname, ext);
    }
    int j = 0;
    do
    {
        if (num_templates > 1)
        {
            std::strcpy(&(tmp_mask[mask_len]), s_masks[j]);
        }
        out = fr_find_first(tmp_mask);
        while (out == 0 && file_count < MAX_NUM_FILES)
        {
            if (!(g_dta.attribute & SUB_DIR))
            {
                if (rds)
                {
                    put_string_center(2, 0, 80, C_GENERAL_INPUT, g_dta.filename.c_str());

                    split_fname_ext(g_dta.filename, fname, ext);
                    make_path(choices[++file_count]->full_name, drive, dir, fname, ext);
                    choices[file_count]->sub_dir = false;
                }
                else
                {
                    std::strcpy(choices[++file_count]->full_name, g_dta.filename.c_str());
                    choices[file_count]->sub_dir = false;
                }
            }
            out = fr_find_next();
        }
    }
    while (++j < num_templates);
    if (++file_count == 0)
    {
        std::strcpy(choices[file_count]->full_name, "*nofiles*");
        choices[file_count]->sub_dir = false;
        ++file_count;
    }

    std::strcpy(instr, "Press F6 for default directory, F4 to toggle sort ");
    if (do_sort)
    {
        std::strcat(instr, "off");
        shell_sort(&choices, file_count, sizeof(Choice *)); // sort file list
    }
    else
    {
        std::strcat(instr, "on");
    }
    if (!not_root && dir[0] && dir[0] != SLASH_CH) // must be in root directory
    {
        split_path(tmp_mask, drive, dir, fname, ext);
        std::strcpy(dir, SLASH);
        make_path(tmp_mask, drive, dir, fname, ext);
    }
    if (num_templates > 1)
    {
        std::strcat(tmp_mask, " ");
        std::strcat(tmp_mask, s_masks[0]);
    }

    std::string const heading{std::string{hdg} + "\n"
        + "Template: " + trim_file_name(tmp_mask, 66)};
    std::strcpy(speed_str, filename);
    int i = 0;
    if (speed_str[0] == 0)
    {
        for (i = 0; i < file_count; i++) // find first file
        {
            if (!choices[i]->sub_dir)
            {
                break;
            }
        }
        if (i >= file_count)
        {
            i = 0;
        }
    }

    i = full_screen_choice(ChoiceFlags::INSTRUCTIONS | (do_sort ? ChoiceFlags::NONE : ChoiceFlags::NOT_SORTED),
        heading.c_str(), nullptr, instr, file_count, (char const **) choices, attributes, 0, 99, 0, i, nullptr,
        speed_str, filename_speed_str, check_f6_key);
    if (i == -ID_KEY_F4)
    {
        do_sort = !do_sort;
        goto restart;
    }
    if (i == -ID_KEY_F6)
    {
        static int last_dir = 0;
        if (last_dir == 0)
        {
            std::strcpy(dir, g_fractal_search_dir1.c_str());
        }
        else
        {
            std::strcpy(dir, g_fractal_search_dir2.c_str());
        }
        fix_dir_name(dir);
        result_filename = make_drive_dir(drive, dir);
        last_dir = 1 - last_dir;
        goto restart;
    }
    if (i < 0)
    {
        // restore filename
        result_filename = old_file_name;
        return true;
    }
    if (speed_str[0] == 0 || s_speed_state == MATCHING)
    {
        if (choices[i]->sub_dir)
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
            result_filename = make_drive_dir(drive, dir);
            goto restart;
        }
        split_fname_ext(choices[i]->full_name, fname, ext);
        result_filename = make_path(drive, dir, fname, ext);
    }
    else
    {
        if (s_speed_state == SEARCH_PATH
            && std::strchr(speed_str, '*') == nullptr && std::strchr(speed_str, '?') == nullptr
            && ((fr_find_first(speed_str) == 0 && (g_dta.attribute & SUB_DIR))
                || std::strcmp(speed_str, SLASH) == 0)) // it is a directory
        {
            s_speed_state = TEMPLATE;
        }

        if (s_speed_state == TEMPLATE)
        {
            /* extract from tempstr the pathname and template information,
                being careful not to overwrite drive and directory if not
                newly specified */
            char drive1[ID_FILE_MAX_DRIVE];
            char dir1[ID_FILE_MAX_DIR];
            char fname1[ID_FILE_MAX_FNAME];
            char ext1[ID_FILE_MAX_EXT];
            split_path(speed_str, drive1, dir1, fname1, ext1);
            if (drive1[0])
            {
                std::strcpy(drive, drive1);
            }
            if (dir1[0])
            {
                std::strcpy(dir, dir1);
            }
            result_filename = make_path(drive, dir, fname1, ext1);
            if (std::strchr(fname1, '*') || std::strchr(fname1, '?') ||
                    std::strchr(ext1,   '*') || std::strchr(ext1,   '?'))
            {
                make_fname_ext(user_file_template, fname1, ext1);
                // cppcheck-suppress uselessAssignmentPtrArg
                file_template = user_file_template;
            }
            else if (is_a_directory(result_filename.c_str()))
            {
                fix_dir_name(result_filename);
            }
            goto restart;
        }
        else // speedstate == SEARCHPATH
        {
            const std::string full_path{find_path(speed_str)};
            if (!full_path.empty())
            {
                result_filename = full_path;
            }
            else
            {
                // failed, make diagnostic useful:
                result_filename = speed_str;
                if (std::strchr(speed_str, SLASH_CH) == nullptr)
                {
                    split_fname_ext(speed_str, fname, ext);
                    result_filename = make_path(drive, dir, fname, ext);
                }
            }
        }
    }
    g_browse_name = std::string{fname} + ext;
    return false;
}

// choice is used by other routines called by fullscreen_choice()
static int check_f6_key(int key, int /*choice*/)
{
    if (key == ID_KEY_F6 || key == ID_KEY_F4)
    {
        return 0 - key;
    }
    return 0;
}

static int filename_speed_str(int row, int col, int vid, char const *speed_string, int speed_match)
{
    char const *prompt;
    if (std::strchr(speed_string, ':')
        || std::strchr(speed_string, '*') || std::strchr(speed_string, '*')
        || std::strchr(speed_string, '?'))
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
