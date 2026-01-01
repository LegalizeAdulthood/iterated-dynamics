// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_file_entry.h"

#include "engine/text_color.h"
#include "fractals/fractalp.h"
#include "fractals/ifs.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "io/file_item.h"
#include "io/load_entry_text.h"
#include "io/trim_filename.h"
#include "misc/Driver.h"
#include "ui/full_screen_choice.h"
#include "ui/get_key_no_help.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/set_default_params.h"
#include "ui/shell_sort.h"
#include "ui/stop_msg.h"
#include "ui/text_screen.h"

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;

namespace id::ui
{

constexpr int MAX_ENTRIES = 2000;

struct GetFileEntry
{
    std::FILE *file{};
    FileEntry **choices{}; // for format_param_file_line
    const char *title{};
};

static GetFileEntry s_gfe{};

static bool is_newline(const int c)
{
    return c == '\n' || c == '\r';
}

static void format_param_file_line(const int choice, char *buf)
{
    int c;
    char line[80];
    std::fseek(s_gfe.file, s_gfe.choices[choice]->point, SEEK_SET);
    while (std::getc(s_gfe.file) != '{')
    {
    }
    do
    {
        c = std::getc(s_gfe.file);
    }
    while (c == ' ' || c == '\t' || c == ';');
    int i = 0;
    while (i < 56 && !is_newline(c) && c != EOF)
    {
        line[i++] = c == '\t' ? ' ' : static_cast<char>(c);
        c = std::getc(s_gfe.file);
    }
    line[i] = 0;
    *fmt::format_to(buf, "{:<20s}{:<56s}", s_gfe.choices[choice]->name, line) = '\0';
}

static int check_gfe_key(const int key, const int choice)
{
    char blanks[79];         // used to clear the entry portion of screen
    std::memset(blanks, ' ', 78);
    blanks[78] = static_cast<char>(0);

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
        char inf_buf[25 * 80];
        int widest_entry_line = 0;
        int lines_in_entry = 0;
        bool comment = false;
        int c = 0;
        int width_ct = 0;
        std::fseek(s_gfe.file, s_gfe.choices[choice]->point, SEEK_SET);
        while ((c = std::fgetc(s_gfe.file)) != EOF)
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
            std::fseek(s_gfe.file, s_gfe.choices[choice]->point, SEEK_SET);
            in_scrolling_mode = false;
        }
        std::fseek(s_gfe.file, s_gfe.choices[choice]->point, SEEK_SET);
        load_entry_text(s_gfe.file, inf_buf, 17, 0, 0);
        if (lines_in_entry > 17 || widest_entry_line > 74)
        {
            in_scrolling_mode = true;
        }
        // ... instead, call help with buffer?  heading added
        driver_stack_screen();
        help_title();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);

        g_text_col_base = 0;
        driver_put_string(2, 1, C_GENERAL_HI, s_gfe.title + std::string{" file entry:\n\n"});
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
                std::fseek(s_gfe.file, s_gfe.choices[choice]->point, SEEK_SET);
                load_entry_text(s_gfe.file, inf_buf, 17, top_line, left_column);
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

static long gfe_choose_entry(
    const ItemType type, const char *title, const std::filesystem::path &path, std::string &entry_name)
{
    const char *o_instr = "Press F6 to select different file, F2 for details, F4 to toggle sort ";
    char buf[101];
    FileEntry storage[MAX_ENTRIES + 1]{};
    FileEntry *choices[MAX_ENTRIES + 1] = { nullptr };
    int attributes[MAX_ENTRIES + 1]{};
    char instr[80];

    static bool do_sort = true;

    s_gfe.choices = &choices[0];
    s_gfe.title = title;

retry:
    for (int i = 0; i < MAX_ENTRIES+1; i++)
    {
        choices[i] = &storage[i];
        attributes[i] = 1;
    }

    help_title(); // to display a clue when file big and next is slow

    const int num_entries = scan_entries(s_gfe.file, &storage[0]);
    if (num_entries == 0)
    {
        stop_msg("File doesn't contain any valid entries");
        std::fclose(s_gfe.file);
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
        + "File: " + trim_filename(path, 68)};
    FormatItem *format_item{};
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
        std::fseek(s_gfe.file, 0, SEEK_SET);
        do_sort = !do_sort;
        goto retry;
    }
    std::fclose(s_gfe.file);
    if (i < 0)
    {
        // go back to file list or cancel
        return i == -ID_KEY_F6 ? -2 : -1;
    }
    entry_name = choices[i]->name;
    return choices[i]->point;
}

// Formula, LSystem, etc. type structure, select from file
// containing definitions in the form    name { ... }
static long get_file_entry(const ItemType type, const char *type_desc, const char *type_wildcard,
    std::filesystem::path &path, std::string &entry_name)
{
    const std::string hdg{fmt::format("Select {:s} File", type_desc)};
    while (true)
    {
        // binary mode used here - it is more work, but much faster,
        //     especially when ftell or fgetpos is used
        s_gfe.file = std::fopen(path.string().c_str(), "rb");
        while (s_gfe.file == nullptr)
        {
            stop_msg("Couldn't open " + path.string());
            if (driver_get_filename(hdg.c_str(), type_desc, type_wildcard, path))
            {
                return -1;
            }
            s_gfe.file = std::fopen(path.string().c_str(), "rb");
        }
        const long entry_pointer = gfe_choose_entry(type, type_desc, path, entry_name);
        if (entry_pointer == -2)
        {
            std::fclose(s_gfe.file);
            s_gfe.file = nullptr;
            // either they cancel browsing for a new file, in which case
            // filename is unchanged, or they browsed for a new file that's
            // been stored in filename.  Either way, we need to open the
            // file and show the entries again.
            driver_get_filename(hdg.c_str(), type_desc, type_wildcard, path);
            continue;
        }
        return entry_pointer;
    }
}

long get_file_entry(ItemType type, std::filesystem::path &path, std::string &entry_name)
{
    switch (type)
    {
    case ItemType::PAR_SET:
        return get_file_entry(type, "Parameter Set", "*.par", path, entry_name);

    case ItemType::FORMULA:
    {
        const long entry_pointer{get_file_entry(type, "Formula", "*.frm", path, entry_name)};
        return entry_pointer >= 0 ? (parse_formula(g_formula_filename, entry_name, true) ? entry_pointer : 0) : entry_pointer;
    }

    case ItemType::L_SYSTEM:
    {
        const long entry_pointer{get_file_entry(type, "L-System", "*.l", path, entry_name)};
        return entry_pointer >= 0 ? (lsystem_load() == 0 ? 0 : entry_pointer) : entry_pointer;
    }

    case ItemType::IFS:
    {
        const long entry_pointer{get_file_entry(type, "IFS", "*.ifs", path, entry_name)};
        if (entry_pointer >= 0 && ifs_load() == 0)
        {
            set_fractal_type(g_ifs_dim == IFSDimension::TWO ? FractalType::IFS : FractalType::IFS_3D);
            set_default_params(); // to correct them if 3d
            return 0;
        }
        return entry_pointer;
    }
    }
    throw std::runtime_error("Unknown item type " + std::to_string(static_cast<int>(type)));
}

} // namespace id::ui
