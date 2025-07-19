// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/file_item.h"

#include "engine/cmdfiles.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/lsys_fns.h"
#include "fractals/parser.h"
#include "io/find_file.h"
#include "io/load_entry_text.h"
#include "io/make_path.h"
#include "io/split_path.h"
#include "io/trim_filename.h"
#include "misc/Driver.h"
#include "ui/full_screen_choice.h"
#include "ui/get_key_no_help.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/set_default_params.h"
#include "ui/shell_sort.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/text_screen.h"

#include <config/path_limits.h>
#include <config/string_case_compare.h>

#include <fmt/format.h>
#include <fmt/std.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

constexpr int MAX_ENTRIES = 2000;

struct FileEntry
{
    char name[ITEM_NAME_LEN+2];
    long point; // points to the ( or the { following the name
};

static std::FILE *s_gfe_file{};
static FileEntry **s_gfe_choices{}; // for format_parmfile_line
static const char *s_gfe_title{};

bool find_file_item(
    std::string &filename, const std::string &item_name, std::FILE **file_ptr, ItemType item_type)
{
    std::FILE *infile = nullptr;
    bool found = false;
    char drive[ID_FILE_MAX_DRIVE];
    char dir[ID_FILE_MAX_DIR];
    char fname[ID_FILE_MAX_FNAME];
    char ext[ID_FILE_MAX_EXT];
    char full_path[ID_FILE_MAX_PATH];
    std::string default_extension;

    split_path(filename, drive, dir, fname, ext);
    make_fname_ext(full_path, fname, ext);
    if (!string_case_equal(filename.c_str(), g_command_file.c_str()))
    {
        infile = std::fopen(filename.c_str(), "rb");
        if (infile != nullptr)
        {
            if (search_for_entry(infile, item_name.c_str()))
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
            make_path(full_path, "", DOT_SLASH, fname, ext);
            infile = std::fopen(full_path, "rb");
            if (infile != nullptr)
            {
                if (search_for_entry(infile, item_name.c_str()))
                {
                    filename = full_path;
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

    std::string par_search_name;
    switch (item_type)
    {
    case ItemType::FORMULA:
        par_search_name = "frm:";
        par_search_name += item_name;
        default_extension = ".frm";
        split_drive_dir(g_search_for.frm, drive, dir);
        break;
    case ItemType::L_SYSTEM:
        par_search_name = "lsys:";
        par_search_name += item_name;
        default_extension = ".l";
        split_drive_dir(g_search_for.lsys, drive, dir);
        break;
    case ItemType::IFS:
        par_search_name = "ifs:";
        par_search_name += item_name;
        default_extension = ".ifs";
        split_drive_dir(g_search_for.ifs, drive, dir);
        break;
    case ItemType::PAR_SET:
    default:
        par_search_name = item_name;
        default_extension = ".par";
        split_drive_dir(g_search_for.par, drive, dir);
        break;
    }

    if (!found)
    {
        infile = std::fopen(g_command_file.c_str(), "rb");
        if (infile != nullptr)
        {
            if (search_for_entry(infile, par_search_name.c_str()))
            {
                filename = g_command_file;
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
        make_path(full_path, drive, dir, fname, ext);
        infile = std::fopen(full_path, "rb");
        if (infile != nullptr)
        {
            if (search_for_entry(infile, item_name.c_str()))
            {
                filename = full_path;
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
        make_path(full_path, drive, dir, "*", default_extension.c_str());
        for (bool more = fr_find_first(full_path); more; more = fr_find_next())
        {
            show_temp_msg(fmt::format("Searching {:13s} for {:s}      ", g_dta.filename, item_name));
            if (!(g_dta.attribute & SUB_DIR)
                && g_dta.filename != "."
                && g_dta.filename != "..")
            {
                split_fname_ext(g_dta.filename, fname, ext);
                make_path(full_path, drive, dir, fname, ext);
                infile = std::fopen(full_path, "rb");
                if (infile != nullptr)
                {
                    if (search_for_entry(infile, item_name.c_str()))
                    {
                        filename = full_path;
                        found = true;
                        break;
                    }
                    std::fclose(infile);
                    infile = nullptr;
                }
            }
        }
        clear_temp_msg();
    }

    if (!found && g_organize_formulas_search && item_type == ItemType::L_SYSTEM)
    {
        split_drive_dir(g_organize_formulas_dir.string(), drive, dir);
        fname[0] = '_';
        fname[1] = (char) 0;
        if (std::isalpha(item_name[0]))
        {
            if (!string_case_equal(item_name.c_str(), "carr", 4))
            {
                fname[1] = item_name[0];
                fname[2] = (char) 0;
            }
            else if (std::isdigit(item_name[4]))
            {
                std::strcat(fname, "rc");
                fname[3] = item_name[4];
                fname[4] = (char) 0;
            }
            else
            {
                std::strcat(fname, "rc");
            }
        }
        else if (std::isdigit(item_name[0]))
        {
            std::strcat(fname, "num");
        }
        else
        {
            std::strcat(fname, "chr");
        }
        make_path(full_path, drive, dir, fname, default_extension.c_str());
        infile = std::fopen(full_path, "rb");
        if (infile != nullptr)
        {
            if (search_for_entry(infile, item_name.c_str()))
            {
                filename = full_path;
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
        stop_msg(fmt::format("'{:s}' file entry item not found", item_name));
        return true;
    }
    // found file
    if (file_ptr != nullptr)
    {
        *file_ptr = infile;
    }
    else if (infile != nullptr)
    {
        std::fclose(infile);
    }
    return false;
}

// skip to next non-white space character and return it
static int skip_white_space(std::FILE *infile, long *file_offset)
{
    int c;
    do
    {
        c = std::getc(infile);
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
        c = std::getc(infile);
        (*file_offset)++;
    }
    while (c != '\n' && c != '\r' && c != EOF);
    return c;
}

static int scan_entries(std::FILE *infile, FileEntry *choices, const char *item_name)
{
    // returns the number of entries found; if a
    // specific entry is being looked for, returns -1 if
    // the entry is found, 0 otherwise.
    char buf[101];
    int exclude_entry;
    long file_offset = -1;
    int num_entries = 0;

    while (true)
    {
        // scan the file for entry names
top:
        int c = skip_white_space(infile, &file_offset);
        if (c == ';')
        {
            c = skip_comment(infile, &file_offset);
            if (c == EOF)
            {
                break;
            }
            continue;
        }
        long temp_offset = file_offset;
        long name_offset = file_offset;
        // next equiv roughly to fscanf(..,"%40[^* \n\r\t({\032]",buf)
        int len = 0;
        // allow spaces in entry names in next
        while (c != ' ' && c != '\t' && c != '(' && c != ';'
            && c != '{' && c != '\n' && c != '\r' && c != EOF)
        {
            if (len < 40)
            {
                buf[len++] = (char) c;
            }
            c = std::getc(infile);
            ++file_offset;
            if (c == '\n' || c == '\r')
            {
                goto top;
            }
        }
        buf[len] = 0;
        while (c != '{' &&  c != EOF)
        {
            if (c == ';')
            {
                c = skip_comment(infile, &file_offset);
            }
            else
            {
                c = std::getc(infile);
                ++file_offset;
                if (c == '\n' || c == '\r')
                {
                    goto top;
                }
            }
        }
        if (c == '{')
        {
            while (c != '}' && c != EOF)
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
                    c = std::getc(infile);
                    ++file_offset;
                }
                if (c == '{') //second '{' found
                {
                    if (temp_offset == name_offset) // if on same line, skip line
                    {
                        skip_comment(infile, &file_offset);
                        goto top;
                    }
                    std::fseek(infile, temp_offset, SEEK_SET); // else, go back to
                    file_offset = temp_offset - 1;             // beginning of line
                    goto top;
                }
            }
            if (c != '}')     // i.e. is EOF
            {
                break;
            }

            if (string_case_equal(buf, "frm:", 4)    //
                || string_case_equal(buf, "ifs:", 4) //
                || string_case_equal(buf, "par:", 4))
            {
                exclude_entry = 4;
            }
            else if (string_case_equal(buf, "lsys:", 5))
            {
                exclude_entry = 5;
            }
            else
            {
                exclude_entry = 0;
            }

            buf[ITEM_NAME_LEN + exclude_entry] = 0;
            if (item_name != nullptr)  // looking for one entry
            {
                if (string_case_equal(buf, item_name))
                {
                    std::fseek(infile, name_offset + (long) exclude_entry, SEEK_SET);
                    return -1;
                }
            }
            else // make a whole list of entries
            {
                if (buf[0] != 0 && !string_case_equal(buf, "comment") && !exclude_entry)
                {
                    std::strcpy(choices[num_entries].name, buf);
                    choices[num_entries].point = name_offset;
                    if (++num_entries >= MAX_ENTRIES)
                    {
                        stop_msg(fmt::format("Too many entries in file, first {:d} used", MAX_ENTRIES));
                        break;
                    }
                }
            }
        }
        else if (c == EOF)
        {
            break;
        }
    }
    return num_entries;
}

bool search_for_entry(std::FILE *infile, const char *item_name)
{
    return scan_entries(infile, nullptr, item_name) == -1;
}

static void format_param_file_line(int choice, char *buf)
{
    int c;
    char line[80];
    std::fseek(s_gfe_file, s_gfe_choices[choice]->point, SEEK_SET);
    while (std::getc(s_gfe_file) != '{')
    {
    }
    do
    {
        c = std::getc(s_gfe_file);
    }
    while (c == ' ' || c == '\t' || c == ';');
    int i = 0;
    while (i < 56 && c != '\n' && c != '\r' && c != EOF)
    {
        line[i++] = (char)((c == '\t') ? ' ' : c);
        c = std::getc(s_gfe_file);
    }
    line[i] = 0;
    *fmt::format_to(buf, "{:<20s}{:<56s}", s_gfe_choices[choice]->name, line) = '\0';
}

static int check_gfe_key(int key, int choice)
{
    char blanks[79];         // used to clear the entry portion of screen
    std::memset(blanks, ' ', 78);
    blanks[78] = (char) 0;

    if (key == ID_KEY_F6)
    {
        return 0-ID_KEY_F6;
    }
    if (key == ID_KEY_F4)
    {
        return 0-ID_KEY_F4;
    }
    if (key == ID_KEY_F2)
    {
        char inf_hdg[60];
        char inf_buf[25 * 80];
        int widest_entry_line = 0;
        int lines_in_entry = 0;
        bool comment = false;
        int c = 0;
        int width_ct = 0;
        std::fseek(s_gfe_file, s_gfe_choices[choice]->point, SEEK_SET);
        while ((c = std::fgetc(s_gfe_file)) != EOF)
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n')
            {
                comment = false;
                lines_in_entry++;
                width_ct =  -1;
            }
            else if (c == '\t')
            {
                width_ct += 7 - width_ct % 8;
            }
            else if (c == '\r')
            {
                continue;
            }
            if (++width_ct > widest_entry_line)
            {
                widest_entry_line = width_ct;
            }
            if (c == '}' && !comment)
            {
                lines_in_entry++;
                break;
            }
        }
        bool in_scrolling_mode = false; // true if entry doesn't fit available space
        if (c == EOF)
        {
            // should never happen
            std::fseek(s_gfe_file, s_gfe_choices[choice]->point, SEEK_SET);
            in_scrolling_mode = false;
        }
        std::fseek(s_gfe_file, s_gfe_choices[choice]->point, SEEK_SET);
        load_entry_text(s_gfe_file, inf_buf, 17, 0, 0);
        if (lines_in_entry > 17 || widest_entry_line > 74)
        {
            in_scrolling_mode = true;
        }
        std::strcpy(inf_hdg, s_gfe_title);
        std::strcat(inf_hdg, " file entry:\n\n");
        // ... instead, call help with buffer?  heading added
        driver_stack_screen();
        help_title();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);

        g_text_col_base = 0;
        driver_put_string(2, 1, C_GENERAL_HI, inf_hdg);
        g_text_col_base = 2; // left margin is 2
        driver_put_string(4, 0, C_GENERAL_MED, inf_buf);
        driver_put_string(-1, 0, C_GENERAL_LO,
            "\n"
            "\n"
            " Use arrow keys, <PageUp>, <PageDown>, <Home>, and <End> to scroll text\n"
            "Any other key to return to selection list");

        int top_line = 0;
        int left_column = 0;
        bool done = false;
        bool rewrite_inf_buf = false;  // if true: rewrite the entry portion of screen
        while (!done)
        {
            if (rewrite_inf_buf)
            {
                rewrite_inf_buf = false;
                std::fseek(s_gfe_file, s_gfe_choices[choice]->point, SEEK_SET);
                load_entry_text(s_gfe_file, inf_buf, 17, top_line, left_column);
                for (int i = 4; i < (lines_in_entry < 17 ? lines_in_entry + 4 : 21); i++)
                {
                    driver_put_string(i, 0, C_GENERAL_MED, blanks);
                }
                driver_put_string(4, 0, C_GENERAL_MED, inf_buf);
            }
            int i = get_a_key_no_help();
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
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_UP_ARROW:
                case ID_KEY_CTL_UP_ARROW:  // up one line
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line--;
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_LEFT_ARROW:
                case ID_KEY_CTL_LEFT_ARROW:  // left one column
                    if (in_scrolling_mode && left_column > 0)
                    {
                        left_column--;
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_RIGHT_ARROW:
                case ID_KEY_CTL_RIGHT_ARROW: // right one column
                    if (in_scrolling_mode && std::strchr(inf_buf, SCROLL_MARKER) != nullptr)
                    {
                        left_column++;
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_PAGE_DOWN:
                case ID_KEY_CTL_PAGE_DOWN: // down 17 lines
                    if (in_scrolling_mode && top_line < lines_in_entry - 17)
                    {
                        top_line += 17;
                        top_line = std::min(top_line, lines_in_entry - 17);
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_PAGE_UP:
                case ID_KEY_CTL_PAGE_UP: // up 17 lines
                    if (in_scrolling_mode && top_line > 0)
                    {
                        top_line -= 17;
                        top_line = std::max(top_line, 0);
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_END:
                case ID_KEY_CTL_END:       // to end of entry
                    if (in_scrolling_mode)
                    {
                        top_line = lines_in_entry - 17;
                        left_column = 0;
                        rewrite_inf_buf = true;
                    }
                    break;
                case ID_KEY_HOME:
                case ID_KEY_CTL_HOME:     // to beginning of entry
                    if (in_scrolling_mode)
                    {
                        left_column = 0;
                        top_line = 0;
                        rewrite_inf_buf = true;
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
        g_text_col_base = 0;
        driver_hide_text_cursor();
        driver_unstack_screen();
    }
    return 0;
}

static long gfe_choose_entry(ItemType type, const char *title, const std::string &filename, std::string &entry_name)
{
    const char *o_instr = "Press F6 to select different file, F2 for details, F4 to toggle sort ";
    char buf[101];
    FileEntry storage[MAX_ENTRIES + 1]{};
    FileEntry *choices[MAX_ENTRIES + 1] = { nullptr };
    int attributes[MAX_ENTRIES + 1]{};
    char instr[80];

    static bool do_sort = true;

    s_gfe_choices = &choices[0];
    s_gfe_title = title;

retry:
    for (int i = 0; i < MAX_ENTRIES+1; i++)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }

    help_title(); // to display a clue when file big and next is slow

    int num_entries = scan_entries(s_gfe_file, &storage[0], nullptr);
    if (num_entries == 0)
    {
        stop_msg("File doesn't contain any valid entries");
        std::fclose(s_gfe_file);
        return -2; // back to file list
    }
    std::strcpy(instr, o_instr);
    if (do_sort)
    {
        std::strcat(instr, "off");
        shell_sort(&choices, num_entries, sizeof(FileEntry *));
    }
    else
    {
        std::strcat(instr, "on");
    }

    std::strcpy(buf, entry_name.c_str()); // preset to last choice made
    const std::string heading{std::string{title} + " Selection\n"
        + "File: " + trim_filename(filename, 68)};
    void (*format_item)(int, char *) = nullptr;
    int box_depth = 0;
    int col_width = box_depth;
    int box_width = col_width;
    if (type == ItemType::PAR_SET)
    {
        format_item = format_param_file_line;
        box_width = 1;
        box_depth = 16;
        col_width = 76;
    }

    const int i =
        full_screen_choice(ChoiceFlags::INSTRUCTIONS | (do_sort ? ChoiceFlags::NONE : ChoiceFlags::NOT_SORTED),
            heading.c_str(), nullptr, instr, num_entries, (const char **) choices, attributes, box_width,
            box_depth, col_width, 0, format_item, buf, nullptr, check_gfe_key);
    if (i == -ID_KEY_F4)
    {
        std::fseek(s_gfe_file, 0, SEEK_SET);
        do_sort = !do_sort;
        goto retry;
    }
    std::fclose(s_gfe_file);
    if (i < 0)
    {
        // go back to file list or cancel
        return (i == -ID_KEY_F6) ? -2 : -1;
    }
    entry_name = choices[i]->name;
    return choices[i]->point;
}

// Formula, LSystem, etc. type structure, select from file
// containing definitions in the form    name { ... }
static long get_file_entry(
    ItemType type, const char *type_desc, const char *type_wildcard, std::string &filename, std::string &entry_name)
{
    std::string hdg{fmt::format("Select {:s} File", type_desc)};
    while (true)
    {
        // binary mode used here - it is more work, but much faster,
        //     especially when ftell or fgetpos is used
        s_gfe_file = std::fopen(filename.c_str(), "rb");
        while (s_gfe_file == nullptr)
        {
            stop_msg("Couldn't open " + filename);
            if (driver_get_filename(hdg.c_str(), type_desc, type_wildcard, filename))
            {
                return -1;
            }
            s_gfe_file = std::fopen(filename.c_str(), "rb");
        }
        long entry_pointer = gfe_choose_entry(type, type_desc, filename, entry_name);
        if (entry_pointer == -2)
        {
            std::fclose(s_gfe_file);
            s_gfe_file = nullptr;
            // either they cancel browsing for a new file, in which case
            // filename is unchanged, or they browsed for a new file that's
            // been stored in filename.  Either way, we need to open the
            // file and show the entries again.
            driver_get_filename(hdg.c_str(), type_desc, type_wildcard, filename);
            continue;
        }
        return entry_pointer;
    }
}

long get_file_entry(ItemType type, std::string &filename, std::string &entry_name)
{
    switch (type)
    {
    case ItemType::PAR_SET:
        return get_file_entry(type, "Parameter Set", "*.par", filename, entry_name);

    case ItemType::FORMULA:
    {
        const long entry_pointer{get_file_entry(type, "Formula", "*.frm", filename, entry_name)};
        return entry_pointer >= 0 ? (run_formula(entry_name, true) ? entry_pointer : 0) : entry_pointer;
    }

    case ItemType::L_SYSTEM:
    {
        const long entry_pointer{get_file_entry(type, "L-System", "*.l", filename, entry_name)};
        return entry_pointer >= 0 ? (lsystem_load() == 0 ? 0 : entry_pointer) : entry_pointer;
    }

    case ItemType::IFS:
    {
        const long entry_pointer{get_file_entry(type, "IFS", "*.ifs", filename, entry_name)};
        if (entry_pointer >= 0 && ifs_load() == 0)
        {
            set_fractal_type(!g_ifs_type ? FractalType::IFS : FractalType::IFS_3D);
            set_default_params(); // to correct them if 3d
            return 0;
        }
        return entry_pointer;
    }
    }
    throw std::runtime_error("Unknown item type " + std::to_string(static_cast<int>(type)));
}
