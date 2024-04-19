#include "file_item.h"

#include "port.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "find_file.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "get_a_filename.h"
#include "get_key_no_help.h"
#include "help_title.h"
#include "id.h"
#include "id_keys.h"
#include "ifs.h"
#include "load_entry_text.h"
#include "lsys_fns.h"
#include "make_path.h"
#include "parser.h"
#include "prototyp.h" // for stricmp
#include "set_default_parms.h"
#include "shell_sort.h"
#include "split_path.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "text_screen.h"
#include "trim_filename.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

#define MAXENTRIES 2000L

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, gfe_type itemtype)
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
            if (search_for_entry(infile, itemname))
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
                if (search_for_entry(infile, itemname))
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
    case gfe_type::FORMULA:
        std::strcpy(parsearchname, "frm:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".frm");
        split_drive_dir(g_search_for.frm, drive, dir);
        break;
    case gfe_type::L_SYSTEM:
        std::strcpy(parsearchname, "lsys:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".l");
        split_drive_dir(g_search_for.lsys, drive, dir);
        break;
    case gfe_type::IFS:
        std::strcpy(parsearchname, "ifs:");
        std::strcat(parsearchname, itemname);
        parsearchname[ITEM_NAME_LEN + 5] = (char) 0; //safety
        std::strcpy(defaultextension, ".ifs");
        split_drive_dir(g_search_for.ifs, drive, dir);
        break;
    case gfe_type::PARM:
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
            if (search_for_entry(infile, parsearchname))
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
            if (search_for_entry(infile, itemname))
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
                    if (search_for_entry(infile, itemname))
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

    if (!found && g_organize_formulas_search && itemtype == gfe_type::L_SYSTEM)
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
            if (search_for_entry(infile, itemname))
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

bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, gfe_type itemtype)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    bool const result = find_file_item(buf, itemname, fileptr, itemtype);
    filename = buf;
    return result;
}

struct entryinfo
{
    char name[ITEM_NAME_LEN+2];
    long point; // points to the ( or the { following the name
};

static char tstack[4096] = { 0 };
static std::FILE *gfe_file;
static entryinfo **gfe_choices; // for format_getparm_line
static char const *gfe_title;

// skip to next non-white space character and return it
static int skip_white_space(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = getc(infile);
        (*file_offset)++;
    }
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return c;
}

// skip to end of line
static int skip_comment(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = getc(infile);
        (*file_offset)++;
    }
    while (c != '\n' && c != '\r' && c != EOF && c != '\032');
    return c;
}

static int scan_entries(std::FILE *infile, entryinfo *choices, char const *itemname)
{
    /*
    function returns the number of entries found; if a
    specific entry is being looked for, returns -1 if
    the entry is found, 0 otherwise.
    */
    char buf[101];
    int exclude_entry;
    long name_offset, temp_offset;
    long file_offset = -1;
    int numentries = 0;

    while (true)
    {
        // scan the file for entry names
        int c, len;
top:
        c = skip_white_space(infile, &file_offset);
        if (c == ';')
        {
            c = skip_comment(infile, &file_offset);
            if (c == EOF || c == '\032')
            {
                break;
            }
            continue;
        }
        temp_offset = file_offset;
        name_offset = temp_offset;
        // next equiv roughly to fscanf(..,"%40[^* \n\r\t({\032]",buf)
        len = 0;
        // allow spaces in entry names in next
        while (c != ' ' && c != '\t' && c != '(' && c != ';'
            && c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032')
        {
            if (len < 40)
            {
                buf[len++] = (char) c;
            }
            c = getc(infile);
            ++file_offset;
            if (c == '\n' || c == '\r')
            {
                goto top;
            }
        }
        buf[len] = 0;
        while (c != '{' &&  c != EOF && c != '\032')
        {
            if (c == ';')
            {
                c = skip_comment(infile, &file_offset);
            }
            else
            {
                c = getc(infile);
                ++file_offset;
                if (c == '\n' || c == '\r')
                {
                    goto top;
                }
            }
        }
        if (c == '{')
        {
            while (c != '}' && c != EOF && c != '\032')
            {
                if (c == ';')
                {
                    c = skip_comment(infile, &file_offset);
                }
                else
                {
                    if (c == '\n' || c == '\r')       // reset temp_offset to
                    {
                        temp_offset = file_offset;  // beginning of new line
                    }
                    c = getc(infile);
                    ++file_offset;
                }
                if (c == '{') //second '{' found
                {
                    if (temp_offset == name_offset) //if on same line, skip line
                    {
                        skip_comment(infile, &file_offset);
                        goto top;
                    }
                    else
                    {
                        std::fseek(infile, temp_offset, SEEK_SET); //else, go back to
                        file_offset = temp_offset - 1;        //beginning of line
                        goto top;
                    }
                }
            }
            if (c != '}')     // i.e. is EOF or '\032'
            {
                break;
            }

            if (strnicmp(buf, "frm:", 4) == 0 ||
                    strnicmp(buf, "ifs:", 4) == 0 ||
                    strnicmp(buf, "par:", 4) == 0)
            {
                exclude_entry = 4;
            }
            else if (strnicmp(buf, "lsys:", 5) == 0)
            {
                exclude_entry = 5;
            }
            else
            {
                exclude_entry = 0;
            }

            buf[ITEM_NAME_LEN + exclude_entry] = 0;
            if (itemname != nullptr)  // looking for one entry
            {
                if (stricmp(buf, itemname) == 0)
                {
                    std::fseek(infile, name_offset + (long) exclude_entry, SEEK_SET);
                    return -1;
                }
            }
            else // make a whole list of entries
            {
                if (buf[0] != 0 && stricmp(buf, "comment") != 0 && !exclude_entry)
                {
                    std::strcpy(choices[numentries].name, buf);
                    choices[numentries].point = name_offset;
                    if (++numentries >= MAXENTRIES)
                    {
                        std::sprintf(buf, "Too many entries in file, first %ld used", MAXENTRIES);
                        stopmsg(STOPMSG_NONE, buf);
                        break;
                    }
                }
            }
        }
        else if (c == EOF || c == '\032')
        {
            break;
        }
    }
    return numentries;
}

bool search_for_entry(std::FILE *infile, char const *itemname)
{
    return scan_entries(infile, nullptr, itemname) == -1;
}

static void format_parmfile_line(int choice, char *buf)
{
    int c, i;
    char line[80];
    std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
    while (getc(gfe_file) != '{')
    {
    }
    do
    {
        c = getc(gfe_file);
    }
    while (c == ' ' || c == '\t' || c == ';');
    i = 0;
    while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032')
    {
        line[i++] = (char)((c == '\t') ? ' ' : c);
        c = getc(gfe_file);
    }
    line[i] = 0;
    std::sprintf(buf, "%-20s%-56s", gfe_choices[choice]->name, line);
}

static int check_gfe_key(int curkey, int choice)
{
    char infhdg[60];
    char infbuf[25*80];
    char blanks[79];         // used to clear the entry portion of screen
    std::memset(blanks, ' ', 78);
    blanks[78] = (char) 0;

    if (curkey == ID_KEY_F6)
    {
        return 0-ID_KEY_F6;
    }
    if (curkey == ID_KEY_F4)
    {
        return 0-ID_KEY_F4;
    }
    if (curkey == ID_KEY_F2)
    {
        int widest_entry_line = 0;
        int lines_in_entry = 0;
        bool comment = false;
        int c = 0;
        int widthct = 0;
        std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
        while ((c = fgetc(gfe_file)) != EOF && c != '\032')
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n')
            {
                comment = false;
                lines_in_entry++;
                widthct =  -1;
            }
            else if (c == '\t')
            {
                widthct += 7 - widthct % 8;
            }
            else if (c == '\r')
            {
                continue;
            }
            if (++widthct > widest_entry_line)
            {
                widest_entry_line = widthct;
            }
            if (c == '}' && !comment)
            {
                lines_in_entry++;
                break;
            }
        }
        bool in_scrolling_mode = false; // true if entry doesn't fit available space
        if (c == EOF || c == '\032')
        {
            // should never happen
            std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
            in_scrolling_mode = false;
        }
        std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
        load_entry_text(gfe_file, infbuf, 17, 0, 0);
        if (lines_in_entry > 17 || widest_entry_line > 74)
        {
            in_scrolling_mode = true;
        }
        std::strcpy(infhdg, gfe_title);
        std::strcat(infhdg, " file entry:\n\n");
        // ... instead, call help with buffer?  heading added
        driver_stack_screen();
        helptitle();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);

        g_text_cbase = 0;
        driver_put_string(2, 1, C_GENERAL_HI, infhdg);
        g_text_cbase = 2; // left margin is 2
        driver_put_string(4, 0, C_GENERAL_MED, infbuf);
        driver_put_string(-1, 0, C_GENERAL_LO,
            "\n"
            "\n"
            " Use " UPARR1 ", " DNARR1 ", " RTARR1 ", " LTARR1
                ", PgUp, PgDown, Home, and End to scroll text\n"
            "Any other key to return to selection list");

        int top_line = 0;
        int left_column = 0;
        bool done = false;
        bool rewrite_infbuf = false;  // if true: rewrite the entry portion of screen
        while (!done)
        {
            if (rewrite_infbuf)
            {
                rewrite_infbuf = false;
                std::fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
                load_entry_text(gfe_file, infbuf, 17, top_line, left_column);
                for (int i = 4; i < (lines_in_entry < 17 ? lines_in_entry + 4 : 21); i++)
                {
                    driver_put_string(i, 0, C_GENERAL_MED, blanks);
                }
                driver_put_string(4, 0, C_GENERAL_MED, infbuf);
            }
            int i = getakeynohelp();
            if (i == ID_KEY_DOWN_ARROW        || i == ID_KEY_CTL_DOWN_ARROW
                || i == ID_KEY_UP_ARROW       || i == ID_KEY_CTL_UP_ARROW
                || i == ID_KEY_LEFT_ARROW     || i == ID_KEY_CTL_LEFT_ARROW
                || i == ID_KEY_RIGHT_ARROW    || i == ID_KEY_CTL_RIGHT_ARROW
                || i == ID_KEY_HOME           || i == ID_KEY_CTL_HOME
                || i == ID_KEY_END            || i == ID_KEY_CTL_END
                || i == ID_KEY_PAGE_UP        || i == ID_KEY_CTL_PAGE_UP
                || i == ID_KEY_PAGE_DOWN      || i == ID_KEY_CTL_PAGE_DOWN)
            {
                switch (i)
                {
                case ID_KEY_DOWN_ARROW:
                case ID_KEY_CTL_DOWN_ARROW: // down one line
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line++;
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_UP_ARROW:
                case ID_KEY_CTL_UP_ARROW:  // up one line
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line--;
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_LEFT_ARROW:
                case ID_KEY_CTL_LEFT_ARROW:  // left one column
                    if (in_scrolling_mode && left_column > 0)
                    {
                        left_column--;
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_RIGHT_ARROW:
                case ID_KEY_CTL_RIGHT_ARROW: // right one column
                    if (in_scrolling_mode && std::strchr(infbuf, '\021') != nullptr)
                    {
                        left_column++;
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_PAGE_DOWN:
                case ID_KEY_CTL_PAGE_DOWN: // down 17 lines
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line += 17;
                        if (top_line > lines_in_entry - 17)
                        {
                            top_line = lines_in_entry - 17;
                        }
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_PAGE_UP:
                case ID_KEY_CTL_PAGE_UP: // up 17 lines
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line -= 17;
                        if (top_line < 0)
                        {
                            top_line = 0;
                        }
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_END:
                case ID_KEY_CTL_END:       // to end of entry
                    if (in_scrolling_mode)
                    {
                        top_line = lines_in_entry - 17;
                        left_column = 0;
                        rewrite_infbuf = true;
                    }
                    break;
                case ID_KEY_HOME:
                case ID_KEY_CTL_HOME:     // to beginning of entry
                    if (in_scrolling_mode)
                    {
                        left_column = 0;
                        top_line = left_column;
                        rewrite_infbuf = true;
                    }
                    break;
                default:
                    break;
                }
            }
            else
            {
                done = true;  // a key other than scrolling key was pressed
            }
        }
        g_text_cbase = 0;
        driver_hide_text_cursor();
        driver_unstack_screen();
    }
    return 0;
}

// subrtn of get_file_entry, separated so that storage gets freed up
static long gfe_choose_entry(gfe_type type, char const *title, char const *filename, char *entryname)
{
    char const *o_instr = "Press F6 to select different file, F2 for details, F4 to toggle sort ";
    int numentries;
    char buf[101];
    entryinfo storage[MAXENTRIES + 1]{};
    entryinfo *choices[MAXENTRIES + 1] = { nullptr };
    int attributes[MAXENTRIES + 1] = { 0 };
    void (*formatitem)(int, char *);
    int boxwidth, boxdepth, colwidth;
    char instr[80];

    static bool dosort = true;

    gfe_choices = &choices[0];
    gfe_title = title;

retry:
    for (int i = 0; i < MAXENTRIES+1; i++)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }

    helptitle(); // to display a clue when file big and next is slow

    numentries = scan_entries(gfe_file, &storage[0], nullptr);
    if (numentries == 0)
    {
        stopmsg(STOPMSG_NONE, "File doesn't contain any valid entries");
        std::fclose(gfe_file);
        return -2; // back to file list
    }
    std::strcpy(instr, o_instr);
    if (dosort)
    {
        std::strcat(instr, "off");
        shell_sort((char *) &choices, numentries, sizeof(entryinfo *));
    }
    else
    {
        std::strcat(instr, "on");
    }

    std::strcpy(buf, entryname); // preset to last choice made
    std::string const heading{std::string{title} + " Selection\n"
        + "File: " + trim_filename(filename, 68)};
    formatitem = nullptr;
    boxdepth = 0;
    colwidth = boxdepth;
    boxwidth = colwidth;
    if (type == gfe_type::PARM)
    {
        formatitem = format_parmfile_line;
        boxwidth = 1;
        boxdepth = 16;
        colwidth = 76;
    }

    int i = fullscreen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
        heading.c_str(), nullptr, instr, numentries, (char const **) choices,
        attributes, boxwidth, boxdepth, colwidth, 0,
        formatitem, buf, nullptr, check_gfe_key);
    if (i == -ID_KEY_F4)
    {
        rewind(gfe_file);
        dosort = !dosort;
        goto retry;
    }
    std::fclose(gfe_file);
    if (i < 0)
    {
        // go back to file list or cancel
        return (i == -ID_KEY_F6) ? -2 : -1;
    }
    std::strcpy(entryname, choices[i]->name);
    return choices[i]->point;
}

long get_file_entry(gfe_type type, char const *title, char const *fmask, char *filename, char *entryname)
{
    // Formula, LSystem, etc type structure, select from file
    // containing definitions in the form    name { ... }
    bool firsttry;
    long entry_pointer;
    bool newfile = false;
    while (true)
    {
        firsttry = false;
        // binary mode used here - it is more work, but much faster,
        //     especially when ftell or fgetpos is used
        while (newfile || (gfe_file = std::fopen(filename, "rb")) == nullptr)
        {
            char buf[60];
            newfile = false;
            if (firsttry)
            {
                stopmsg(STOPMSG_NONE, std::string{"Can't find "} + filename);
            }
            std::sprintf(buf, "Select %s File", title);
            if (getafilename(buf, fmask, filename))
            {
                return -1;
            }

            firsttry = true; // if around open loop again it is an error
        }
        setvbuf(gfe_file, tstack, _IOFBF, 4096); // improves speed when file is big
        newfile = false;
        entry_pointer = gfe_choose_entry(type, title, filename, entryname);
        if (entry_pointer == -2)
        {
            newfile = true; // go to file list,
            continue;    // back to getafilename
        }
        if (entry_pointer == -1)
        {
            return -1;
        }
        switch (type)
        {
        case gfe_type::FORMULA:
            if (!RunForm(entryname, true))
            {
                return 0;
            }
            break;
        case gfe_type::L_SYSTEM:
            if (LLoad() == 0)
            {
                return 0;
            }
            break;
        case gfe_type::IFS:
            if (ifsload() == 0)
            {
                g_fractal_type = !g_ifs_type ? fractal_type::IFS : fractal_type::IFS3D;
                g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
                set_default_parms(); // to correct them if 3d
                return 0;
            }
            break;
        case gfe_type::PARM:
            return entry_pointer;
        }
    }
}

long get_file_entry(gfe_type type, char const *title, char const *fmask,
    std::string &filename, char *entryname)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    long const result = get_file_entry(type, title, fmask, buf, entryname);
    filename = buf;
    return result;
}

long get_file_entry(gfe_type type, char const *title, char const *fmask,
    std::string &filename, std::string &entryname)
{
    char buf[FILE_MAX_PATH];
    assert(filename.size() < FILE_MAX_PATH);
    std::strncpy(buf, filename.c_str(), FILE_MAX_PATH);
    buf[FILE_MAX_PATH - 1] = 0;
    char name_buf[ITEM_NAME_LEN];
    std::strncpy(name_buf, entryname.c_str(), ITEM_NAME_LEN);
    name_buf[ITEM_NAME_LEN - 1] = 0;
    long const result = get_file_entry(type, title, fmask, buf, name_buf);
    filename = buf;
    entryname = name_buf;
    return result;
}
