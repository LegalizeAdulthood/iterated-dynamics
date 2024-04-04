#include "get_a_filename.h"

#include "port.h"
#include "prototyp.h"

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

#include <cstring>
#include <string>

// speed key state values
enum
{
    MATCHING = 0,   // string matches list - speed key mode
    TEMPLATE = -2,  // wild cards present - buiding template
    SEARCHPATH = -3 // no match - building path search name
};

enum
{
    MAXNUMFILES = 2977L
};

struct CHOICE
{
    char name[13];
    char full_name[FILE_MAX_PATH];
    char type;
};

static  int check_f6_key(int curkey, int choice);
static  int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match);

static int speedstate{};
static char const *masks[] = {"*.pot", "*.gif"};


bool getafilename(char const *hdg, char const *file_template, char *flname)
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
    CHOICE storage[MAXNUMFILES];
    CHOICE *choices[MAXNUMFILES];
    int attributes[MAXNUMFILES];
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
    for (int i = 0; i < MAXNUMFILES; i++)
    {
        attributes[i] = 1;
        choices[i] = &storage[i];
    }
    // save filename
    std::strcpy(old_flname, flname);

restart:  // return here if template or directory changes
    tmpmask[0] = 0;
    if (flname[0] == 0)
    {
        std::strcpy(flname, DOTSLASH);
    }
    splitpath(flname , drive, dir, fname, ext);
    make_fname_ext(filename, fname, ext);
    retried = 0;

retry_dir:
    if (dir[0] == 0)
    {
        std::strcpy(dir, ".");
    }
    expand_dirname(dir, drive);
    make_drive_dir(tmpmask, drive, dir);
    fix_dirname(tmpmask);
    if (retried == 0 && std::strcmp(dir, SLASH) && std::strcmp(dir, DOTSLASH))
    {
        int j = (int) std::strlen(tmpmask) - 1;
        tmpmask[j] = 0; // strip trailing backslash
        if (std::strchr(tmpmask, '*') || std::strchr(tmpmask, '?')
            || fr_findfirst(tmpmask) != 0
            || (DTA.attribute & SUBDIR) == 0)
        {
            std::strcpy(dir, DOTSLASH);
            ++retried;
            goto retry_dir;
        }
        tmpmask[j] = SLASHC;
    }
    if (file_template[0])
    {
        numtemplates = 1;
        split_fname_ext(file_template, fname, ext);
    }
    else
    {
        numtemplates = sizeof(masks)/sizeof(masks[0]);
    }
    filecount = -1;
    dircount  = 0;
    notroot   = false;
    masklen = (int) std::strlen(tmpmask);
    std::strcat(tmpmask, "*.*");
    out = fr_findfirst(tmpmask);
    while (out == 0 && filecount < MAXNUMFILES)
    {
        if ((DTA.attribute & SUBDIR) && DTA.filename != ".")
        {
            if (DTA.filename != "..")
            {
                DTA.filename += SLASH;
            }
            std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
            choices[filecount]->name[12] = 0;
            choices[filecount]->type = 1;
            std::strcpy(choices[filecount]->full_name, DTA.filename.c_str());
            dircount++;
            if (DTA.filename == "..")
            {
                notroot = true;
            }
        }
        out = fr_findnext();
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
            std::strcpy(&(tmpmask[masklen]), masks[j]);
        }
        out = fr_findfirst(tmpmask);
        while (out == 0 && filecount < MAXNUMFILES)
        {
            if (!(DTA.attribute & SUBDIR))
            {
                if (rds)
                {
                    putstringcenter(2, 0, 80, C_GENERAL_INPUT, DTA.filename.c_str());

                    split_fname_ext(DTA.filename, fname, ext);
                    // just using speedstr as a handy buffer
                    make_path(speedstr, drive, dir, fname, ext);
                    std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
                    choices[filecount]->name[12] = 0;
                    choices[filecount]->type = 0;
                }
                else
                {
                    std::strncpy(choices[++filecount]->name, DTA.filename.c_str(), 13);
                    choices[filecount]->name[12] = 0;
                    choices[filecount]->type = 0;
                    std::strcpy(choices[filecount]->full_name, DTA.filename.c_str());
                }
            }
            out = fr_findnext();
        }
    }
    while (++j < numtemplates);
    if (++filecount == 0)
    {
        std::strcpy(choices[filecount]->name, "*nofiles*");
        choices[filecount]->type = 0;
        ++filecount;
    }

    std::strcpy(instr, "Press " FK_F6 " for default directory, " FK_F4 " to toggle sort ");
    if (dosort)
    {
        std::strcat(instr, "off");
        shell_sort(&choices, filecount, sizeof(CHOICE *)); // sort file list
    }
    else
    {
        std::strcat(instr, "on");
    }
    if (!notroot && dir[0] && dir[0] != SLASHC) // must be in root directory
    {
        splitpath(tmpmask, drive, dir, fname, ext);
        std::strcpy(dir, SLASH);
        make_path(tmpmask, drive, dir, fname, ext);
    }
    if (numtemplates > 1)
    {
        std::strcat(tmpmask, " ");
        std::strcat(tmpmask, masks[0]);
    }

    std::string const heading{std::string{hdg} + "\n"
        + "Template: " + tmpmask};
    std::strcpy(speedstr, filename);
    int i = 0;
    if (speedstr[0] == 0)
    {
        for (i = 0; i < filecount; i++) // find first file
        {
            if (choices[i]->type == 0)
            {
                break;
            }
        }
        if (i >= filecount)
        {
            i = 0;
        }
    }

    i = fullscreen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
        heading.c_str(), nullptr, instr, filecount, (char const **) choices,
        attributes, 0, 99, 0, i, nullptr, speedstr, filename_speedstr, check_f6_key);
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
        fix_dirname(dir);
        make_drive_dir(flname, drive, dir);
        lastdir = 1 - lastdir;
        goto restart;
    }
    if (i < 0)
    {
        // restore filename
        std::strcpy(flname, old_flname);
        return true;
    }
    if (speedstr[0] == 0 || speedstate == MATCHING)
    {
        if (choices[i]->type)
        {
            if (std::strcmp(choices[i]->name, "..") == 0) // go up a directory
            {
                if (std::strcmp(dir, DOTSLASH) == 0)
                {
                    std::strcpy(dir, DOTDOTSLASH);
                }
                else
                {
                    char *s = std::strrchr(dir, SLASHC);
                    if (s != nullptr) // trailing slash
                    {
                        *s = 0;
                        s = std::strrchr(dir, SLASHC);
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
            fix_dirname(dir);
            make_drive_dir(flname, drive, dir);
            goto restart;
        }
        split_fname_ext(choices[i]->full_name, fname, ext);
        make_path(flname, drive, dir, fname, ext);
    }
    else
    {
        if (speedstate == SEARCHPATH
            && std::strchr(speedstr, '*') == nullptr && std::strchr(speedstr, '?') == nullptr
            && ((fr_findfirst(speedstr) == 0 && (DTA.attribute & SUBDIR))
                || std::strcmp(speedstr, SLASH) == 0)) // it is a directory
        {
            speedstate = TEMPLATE;
        }

        if (speedstate == TEMPLATE)
        {
            /* extract from tempstr the pathname and template information,
                being careful not to overwrite drive and directory if not
                newly specified */
            char drive1[FILE_MAX_DRIVE];
            char dir1[FILE_MAX_DIR];
            char fname1[FILE_MAX_FNAME];
            char ext1[FILE_MAX_EXT];
            splitpath(speedstr, drive1, dir1, fname1, ext1);
            if (drive1[0])
            {
                std::strcpy(drive, drive1);
            }
            if (dir1[0])
            {
                std::strcpy(dir, dir1);
            }
            make_path(flname, drive, dir, fname1, ext1);
            if (std::strchr(fname1, '*') || std::strchr(fname1, '?') ||
                    std::strchr(ext1,   '*') || std::strchr(ext1,   '?'))
            {
                make_fname_ext(user_file_template, fname1, ext1);
                // cppcheck-suppress uselessAssignmentPtrArg
                file_template = user_file_template;
            }
            else if (isadirectory(flname))
            {
                fix_dirname(flname);
            }
            goto restart;
        }
        else // speedstate == SEARCHPATH
        {
            const std::string full_path{find_path(speedstr)};
            if (!full_path.empty())
            {
                std::strcpy(flname, full_path.c_str());
            }
            else
            {
                // failed, make diagnostic useful:
                std::strcpy(flname, speedstr);
                if (std::strchr(speedstr, SLASHC) == nullptr)
                {
                    split_fname_ext(speedstr, fname, ext);
                    make_path(flname, drive, dir, fname, ext);
                }
            }
        }
    }
    g_browse_name = std::string{fname} + ext;
    return false;
}

bool getafilename(char const *hdg, char const *file_template, std::string &flname)
{
    char buff[FILE_MAX_PATH];
    std::strncpy(buff, flname.c_str(), FILE_MAX_PATH);
    bool const result = getafilename(hdg, file_template, buff);
    flname = buff;
    return result;
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

static int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match)
{
    char const *prompt;
    if (std::strchr(speedstring, ':')
        || std::strchr(speedstring, '*') || std::strchr(speedstring, '*')
        || std::strchr(speedstring, '?'))
    {
        speedstate = TEMPLATE;  // template
        prompt = "File Template";
    }
    else if (speed_match)
    {
        speedstate = SEARCHPATH; // does not match list
        prompt = "Search Path for";
    }
    else
    {
        speedstate = MATCHING;
        prompt = g_speed_prompt.c_str();
    }
    driver_put_string(row, col, vid, prompt);
    return (int) std::strlen(prompt);
}
