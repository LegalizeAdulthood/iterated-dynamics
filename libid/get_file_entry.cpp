#include "get_file_entry.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "get_a_filename.h"
#include "get_key_no_help.h"
#include "help_title.h"
#include "ifs.h"
#include "load_entry_text.h"
#include "lsys_fns.h"
#include "os.h"
#include "parser.h"
#include "prompts1.h"
#include "prompts2.h"
#include "set_default_parms.h"
#include "shell_sort.h"
#include "stop_msg.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#define MAXENTRIES 2000L

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

    if (curkey == FIK_F6)
    {
        return 0-FIK_F6;
    }
    if (curkey == FIK_F4)
    {
        return 0-FIK_F4;
    }
    if (curkey == FIK_F2)
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
            if (i == FIK_DOWN_ARROW        || i == FIK_CTL_DOWN_ARROW
                || i == FIK_UP_ARROW       || i == FIK_CTL_UP_ARROW
                || i == FIK_LEFT_ARROW     || i == FIK_CTL_LEFT_ARROW
                || i == FIK_RIGHT_ARROW    || i == FIK_CTL_RIGHT_ARROW
                || i == FIK_HOME           || i == FIK_CTL_HOME
                || i == FIK_END            || i == FIK_CTL_END
                || i == FIK_PAGE_UP        || i == FIK_CTL_PAGE_UP
                || i == FIK_PAGE_DOWN      || i == FIK_CTL_PAGE_DOWN)
            {
                switch (i)
                {
                case FIK_DOWN_ARROW:
                case FIK_CTL_DOWN_ARROW: // down one line
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line++;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_UP_ARROW:
                case FIK_CTL_UP_ARROW:  // up one line
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line--;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_LEFT_ARROW:
                case FIK_CTL_LEFT_ARROW:  // left one column
                    if (in_scrolling_mode && left_column > 0)
                    {
                        left_column--;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_RIGHT_ARROW:
                case FIK_CTL_RIGHT_ARROW: // right one column
                    if (in_scrolling_mode && std::strchr(infbuf, '\021') != nullptr)
                    {
                        left_column++;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_PAGE_DOWN:
                case FIK_CTL_PAGE_DOWN: // down 17 lines
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
                case FIK_PAGE_UP:
                case FIK_CTL_PAGE_UP: // up 17 lines
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
                case FIK_END:
                case FIK_CTL_END:       // to end of entry
                    if (in_scrolling_mode)
                    {
                        top_line = lines_in_entry - 17;
                        left_column = 0;
                        rewrite_infbuf = true;
                    }
                    break;
                case FIK_HOME:
                case FIK_CTL_HOME:     // to beginning of entry
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
static long gfe_choose_entry(int type, char const *title, char const *filename, char *entryname)
{
#ifdef XFRACT
    char const *o_instr = "Press " FK_F6 " to select file, " FK_F2 " for details, " FK_F4 " to toggle sort ";
    // keep the above line length < 80 characters
#else
    char const *o_instr = "Press " FK_F6 " to select different file, " FK_F2 " for details, " FK_F4 " to toggle sort ";
#endif
    int numentries;
    char buf[101];
    entryinfo storage[MAXENTRIES + 1];
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
        + "File: " + filename};
    formatitem = nullptr;
    boxdepth = 0;
    colwidth = boxdepth;
    boxwidth = colwidth;
    if (type == GETPARM)
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
    if (i == -FIK_F4)
    {
        rewind(gfe_file);
        dosort = !dosort;
        goto retry;
    }
    std::fclose(gfe_file);
    if (i < 0)
    {
        // go back to file list or cancel
        return (i == -FIK_F6) ? -2 : -1;
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
        case GETFORMULA:
            if (!RunForm(entryname, true))
            {
                return 0;
            }
            break;
        case GETLSYS:
            if (LLoad() == 0)
            {
                return 0;
            }
            break;
        case GETIFS:
            if (ifsload() == 0)
            {
                g_fractal_type = !g_ifs_type ? fractal_type::IFS : fractal_type::IFS3D;
                g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
                set_default_parms(); // to correct them if 3d
                return 0;
            }
            break;
        case GETPARM:
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
