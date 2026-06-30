// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/full_screen_prompt.h"

#include "engine/text_color.h"
#include "fractals/formula.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/lsystem.h"
#include "helpdefs.h"
#include "io/file_item.h"
#include "io/load_entry_text.h"
#include "math/round_float_double.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/double_to_string.h"
#include "ui/help.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/input_field.h"
#include "ui/mouse.h"
#include "ui/put_string_center.h"
#include "ui/text_screen.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

static int prompt_check_key(int key);
static int prompt_check_key_scroll(int key);
static  int input_field_list(int attr, char *field, int field_len, const char **list, int list_len,
                             int row, int col, int (*check_key)(int key));
static int prompt_display_len(const FullScreenValues *val);
static int prompt_field_len(const FullScreenValues *val);
static std::string prompt_value_string(const FullScreenValues *val);

static int s_fn_key_mask{};

// These need to be global because F6 exits fullscreen_prompt()
static int s_scroll_row{};    // will be set to first line of extra info to be displayed (0 = top line)
static int s_scroll_column{}; // will be set to first column of extra info to be displayed (0 = leftmost column)

void full_screen_reset_scrolling()
{
    s_scroll_row = 0; // make sure we start at beginning of entry
    s_scroll_column = 0;
}

namespace
{

void copy_string(char *dest, const int max_chars, const std::string &source)
{
    const std::size_t copy_len{std::min(source.size(), static_cast<std::size_t>(std::max(max_chars, 0)))};
    std::copy_n(source.data(), copy_len, dest);
    dest[copy_len] = 0;
}

struct Prompt
{
    Prompt(const char *hdg,       // heading, lines separated by \n
        int num_prompts,          // there are this many prompts (max)
        const char **prompts,     // array of prompting pointers
        FullScreenValues *values, // array of values
        int fn_key_mask,          // bit n on if Fn to cause return
        char *extra_info);        // extra info box to display, \n separated
    int prompt();

private:
    void init_scroll_file();
    void count_lines_in_entry();
    void count_title_lines();
    void count_extra_lines();
    bool entry_fits();
    void scroll_off();
    void set_scroll_limit();
    void set_vertical_positions();
    void set_horizontal_positions();
    void display_box_heading();
    void display_extra_info();
    void display_empty_box();
    void display_initial_values();
    int prompt_no_params();
    void display_footing();
    int prompt_params();
    void display_title();
    int full_screen_exit();

    const char *hdg;             // heading, lines separated by \n
    int num_prompts;             // there are this many prompts (max)
    const char **prompts;        // array of prompting pointers
    FullScreenValues *values;    // array of values
    char *extra_info;            // extra info box to display, \n separated
    std::FILE *scroll_file{};    // file with extra_info entry to scroll
    long scroll_file_start{};    // where entry starts in scroll_file
    bool in_scrolling_mode{};    // will be true if we need to scroll extra_info
    int lines_in_entry{};        // total lines in entry to be scrolled
    int title_width{};           // count title lines, find widest
    int title_lines{1};          //
    int extra_width{};           //
    int extra_lines{};           //
    int vertical_scroll_limit{}; // don't scroll down if this is top line
    int title_row{};             //
    int box_lines{};             //
    int box_row{};               //
    int prompt_row{};            //
    int instr_row{};             //
    int extra_row{};             //
    int max_comment{};           //
    int max_prompt_width{};      //
    int max_field_width{};       //
    bool any_input{};            //
    int box_width{};             //
    int box_col{};               //
    int prompt_col{};            //
    int value_col{};             //
    bool rewrite_extra_info{};   // if true: rewrite extra_info to text box
    char blanks[78]{};           // used to clear text box
    int done{};                  //
};

Prompt::Prompt(const char *hdg, const int num_prompts, const char **prompts, FullScreenValues *values,
    const int fn_key_mask, char *extra_info) :
    hdg(hdg),
    num_prompts(num_prompts),
    prompts(prompts),
    values(values),
    extra_info(extra_info)
{
    s_fn_key_mask = fn_key_mask;

    std::memset(blanks, ' ', 77);    // initialize string of blanks
    blanks[77] = 0;
}

void Prompt::init_scroll_file()
{
    /* If applicable, open file for scrolling extra_info. The function
           find_file_item() opens the file and sets the file pointer to the
           beginning of the entry.
        */
    if (extra_info && *extra_info)
    {
        if (g_fractal_type == FractalType::FORMULA)
        {
            find_file_item(g_formula_filename, g_formula_name, &scroll_file, ItemType::FORMULA);
            in_scrolling_mode = true;
            scroll_file_start = std::ftell(scroll_file);
        }
        else if (g_fractal_type == FractalType::L_SYSTEM)
        {
            find_file_item(g_l_system_filename, g_l_system_name, &scroll_file, ItemType::L_SYSTEM);
            in_scrolling_mode = true;
            scroll_file_start = std::ftell(scroll_file);
        }
        else if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
        {
            find_file_item(g_ifs_filename, g_ifs_name, &scroll_file, ItemType::IFS);
            in_scrolling_mode = true;
            scroll_file_start = std::ftell(scroll_file);
        }
    }
}

void Prompt::count_lines_in_entry()
{
    // initialize widest_entry_line and lines_in_entry
    if (in_scrolling_mode && scroll_file != nullptr)
    {
        int widest_entry_line = 0;
        bool comment = false;
        int c = 0;
        int line_width = -1;
        while ((c = std::fgetc(scroll_file)) != EOF)
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n')
            {
                comment = false;
                lines_in_entry++;
                line_width = -1;
            }
            else if (c == '\t')
            {
                line_width += 7 - line_width % 8;
            }
            else if (c == '\r')
            {
                continue;
            }
            if (++line_width > widest_entry_line)
            {
                widest_entry_line = line_width;
            }
            if (c == '}' && !comment)
            {
                lines_in_entry++;
                break;
            }
        }
        if (c == EOF)
        {
            // should never happen
            std::fclose(scroll_file);
            in_scrolling_mode = false;
        }
    }
}

void Prompt::display_title()
{
    help_title();                                   // clear screen, display title line
    driver_set_attr(1, 0, C_PROMPT_BKGRD, 24 * 80); // init rest of screen to background
}

void Prompt::count_title_lines()
{
    int i = 0;
    const char *scan = hdg;
    while (*scan)
    {
        if (*scan++ == '\n')
        {
            ++title_lines;
            i = -1;
        }
        if (++i > title_width)
        {
            title_width = i;
        }
    }
}

void Prompt::count_extra_lines()
{
    const char *scan = extra_info;
    if (scan != nullptr)
    {
        if (*scan == 0)
        {
            extra_info = nullptr;
        }
        else
        {
            // count extra lines, find widest
            extra_lines = 3;
            int i = 0;
            while (*scan)
            {
                if (*scan++ == '\n')
                {
                    if (extra_lines + num_prompts + title_lines >= 20)
                    {
                        break;
                    }
                    ++extra_lines;
                    i = -1;
                }
                if (++i > extra_width)
                {
                    extra_width = i;
                }
            }
        }
    }
}

bool Prompt::entry_fits()
{
    return in_scrolling_mode                 //
        && s_scroll_row == 0          //
        && lines_in_entry == extra_lines - 2 //
        && s_scroll_column == 0       //
        && std::strchr(extra_info, SCROLL_MARKER) == nullptr;
}

void Prompt::scroll_off()
{
    in_scrolling_mode = false;
    std::fclose(scroll_file);
    scroll_file = nullptr;
}

// initialize vertical scroll limit. When the top line of the text
// box is the vertical scroll limit, the bottom line is the end of the
// entry, and no further down scrolling is necessary.
void Prompt::set_scroll_limit()
{
    if (in_scrolling_mode)
    {
        vertical_scroll_limit = lines_in_entry - (extra_lines - 2);
    }
}

// work out vertical positioning
void Prompt::set_vertical_positions()
{
    {
        const int i = num_prompts + title_lines + extra_lines + 3; // total rows required
        int j = (25 - i) / 2;                                // top row of it all when centered
        j -= j / 4;                                          // higher is better if lots extra
        box_lines = num_prompts;
        title_row = 1 + j;
    }
    box_row = title_row + title_lines;
    prompt_row = title_row + title_lines;
    if (title_row > 2)
    {
        // room for blank between title & box?
        --title_row;
        --box_row;
        ++box_lines;
    }
    instr_row = box_row + box_lines;
    if (instr_row + 3 + extra_lines < 25)
    {
        ++box_lines; // blank at bottom of box
        ++instr_row;
        if (instr_row + 3 + extra_lines < 25)
        {
            ++instr_row; // blank before instructions
        }
    }
    extra_row = instr_row + 2;
    if (num_prompts > 1) // 3 instructions lines
    {
        ++extra_row;
    }
    if (extra_row + extra_lines < 25)
    {
        ++extra_row;
    }
}

// work out horizontal positioning
void Prompt::set_horizontal_positions()
{
    if (in_scrolling_mode) // set box to max width if in scrolling mode
    {
        extra_width = 76;
    }

    for (int i = 0; i < num_prompts; i++)
    {
        if (values[i].type == 'y')
        {
            static const char *noyes[2] = {"no", "yes"};
            values[i].type = 'l';
            values[i].uval.ch.vlen = 3;
            values[i].uval.ch.list = noyes;
            values[i].uval.ch.list_len = 2;
        }
        const std::string_view prompt{prompts[i]};
        int j = static_cast<int>(prompt.size());
        if (values[i].type == '*')
        {
            max_comment = std::max(j, max_comment);
        }
        else
        {
            any_input = true;
            max_prompt_width = std::max(j, max_prompt_width);
            max_field_width = std::max(prompt_display_len(&values[i]), max_field_width);
        }
    }
    box_width = max_prompt_width + max_field_width + 2;
    box_width = std::max(max_comment, box_width);
    if ((box_width += 4) > 80)
    {
        box_width = 80;
    }
    box_col = (80 - box_width) / 2; // center the box
    prompt_col = box_col + 2;
    value_col = box_col + box_width - max_field_width - 2;
    if (box_width <= 76)
    {
        // make margin a bit wider if we can
        box_width += 2;
        --box_col;
    }

    {
        int j = title_width;
        j = std::max(j, extra_width);
        int i = j + 4 - box_width;
        if (i > 0)
        {
            // expand box for title/extra
            if (box_width + i > 80)
            {
                i = 80 - box_width;
            }
            box_width += i;
            box_col -= i / 2;
        }
    }
    {
        const int i = (90 - box_width) / 20;
        box_col -= i;
        prompt_col -= i;
        value_col -= i;
    }
}

void Prompt::display_box_heading()
{
    for (int i = title_row; i < box_row; ++i)
    {
        driver_set_attr(i, box_col, C_PROMPT_HI, box_width);
    }

    // center each line of heading independently
    std::string_view hdg_line{hdg};
    int i = 0;
    for (; i < title_lines - 1; i++)
    {
        const std::size_t next = hdg_line.find('\n');
        if (next == std::string_view::npos)
        {
            break; // shouldn't happen
        }
        const std::string_view line{hdg_line.substr(0, next)};
        title_width = static_cast<int>(line.size());
        driver_put_string(title_row + i, box_col + (box_width - title_width) / 2, C_PROMPT_HI, std::string{line});
        hdg_line.remove_prefix(next + 1);
    }
    // add scrolling key message, if applicable
    std::string final_line{hdg_line};
    if (in_scrolling_mode)
    {
        constexpr std::size_t scroll_heading_prefix_len{31};
        if (final_line.size() > scroll_heading_prefix_len)
        {
            final_line.resize(scroll_heading_prefix_len);
        }
        final_line += ". Ctrl+<arrow key> to scroll text.)";
    }

    title_width = static_cast<int>(final_line.size());
    driver_put_string(title_row + i, box_col + (box_width - title_width) / 2, C_PROMPT_HI, final_line);
}

void Prompt::display_extra_info()
{
    if (!extra_info)
    {
        return;
    }

    constexpr char HORIZ_LINE{'\xC4'};
    constexpr const char *LOWER_LEFT{"\xC0"};
    constexpr const char *LOWER_RIGHT{"\xD9"};
    constexpr const char *VERT_LINE{"\xB3"};
    constexpr const char *UPPER_LEFT{"\xDA"};
    constexpr const char *UPPER_RIGHT{"\xBF"};
    char buf[81];
    std::memset(buf, HORIZ_LINE, 80);
    buf[box_width - 2] = 0;

    int col = box_col + 1;
    driver_put_string(extra_row, col, C_PROMPT_BKGRD, buf);
    driver_put_string(extra_row + extra_lines - 1, col, C_PROMPT_BKGRD, buf);

    --col;
    driver_put_string(extra_row, col, C_PROMPT_BKGRD, UPPER_LEFT);
    driver_put_string(extra_row + extra_lines - 1, col, C_PROMPT_BKGRD, LOWER_LEFT);

    col += box_width - 2;
    driver_put_string(extra_row, col, C_PROMPT_BKGRD, UPPER_RIGHT);
    driver_put_string(extra_row + extra_lines - 1, col, C_PROMPT_BKGRD, LOWER_RIGHT);

    col = box_col;
    for (int i = 1; i < extra_lines - 1; ++i)
    {
        driver_put_string(extra_row + i, col, C_PROMPT_BKGRD, VERT_LINE);
        driver_put_string(extra_row + i, col + box_width - 2, C_PROMPT_BKGRD, VERT_LINE);
    }

    col += (box_width - extra_width) / 2 - 1;
    const std::string extra_text{extra_info};
    std::string::size_type line_begin{};
    for (int i = 1; i < extra_lines - 1; ++i)
    {
        const std::string::size_type line_end = extra_text.find('\n', line_begin);
        const std::string line{extra_text.substr(line_begin, line_end - line_begin)};
        driver_put_string(extra_row + i, col, C_PROMPT_TEXT, line.c_str());
        line_begin = line_end + 1;
    }
}

void Prompt::display_empty_box()
{
    for (int i = 0; i < box_lines; ++i)
    {
        driver_set_attr(box_row + i, box_col, C_PROMPT_LO, box_width);
    }
}

void Prompt::display_initial_values()
{
    for (int i = 0; i < num_prompts; i++)
    {
        driver_put_string(prompt_row + i, prompt_col, C_PROMPT_LO, prompts[i]);
        std::string value{prompt_value_string(&values[i])};
        value.resize(prompt_display_len(&values[i]), ' ');
        driver_put_string(prompt_row + i, value_col, C_PROMPT_LO, value.c_str());
    }
}

int Prompt::prompt_no_params()
{
    put_string_center(instr_row++, 0, 80, C_PROMPT_BKGRD, "No changeable parameters;");
    put_string_center(instr_row, 0, 80, C_PROMPT_BKGRD,
        g_help_mode > HelpLabels::HELP_INDEX ? "Press ENTER to exit, ESC to back out, F1 for help"
                                               : "Press ENTER to exit");
    driver_hide_text_cursor();
    while (true)
    {
        if (rewrite_extra_info)
        {
            rewrite_extra_info = false;
            std::fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(scroll_file, extra_info, extra_lines - 2, s_scroll_row, s_scroll_column);
            for (int i = 1; i <= extra_lines - 2; i++)
            {
                driver_put_string(extra_row + i, 1, C_PROMPT_TEXT, blanks);
            }
            driver_put_string(extra_row + 1, 1, C_PROMPT_TEXT, extra_info);
        }
        // TODO: rework key interaction to blocking wait
        while (!driver_key_pressed())
        {
        }
        done = driver_get_key();
        switch (done)
        {
        case ID_KEY_ESC:
            done = -1;
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            return full_screen_exit();
        case ID_KEY_CTL_DOWN_ARROW: // scrolling key - down one row
            if (in_scrolling_mode && s_scroll_row < vertical_scroll_limit)
            {
                s_scroll_row++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_UP_ARROW: // scrolling key - up one row
            if (in_scrolling_mode && s_scroll_row > 0)
            {
                s_scroll_row--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_LEFT_ARROW: // scrolling key - left one column
            if (in_scrolling_mode && s_scroll_column > 0)
            {
                s_scroll_column--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_RIGHT_ARROW: // scrolling key - right one column
            if (in_scrolling_mode && std::strchr(extra_info, SCROLL_MARKER) != nullptr)
            {
                s_scroll_column++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_DOWN: // scrolling key - down one screen
            if (in_scrolling_mode && s_scroll_row < vertical_scroll_limit)
            {
                s_scroll_row += extra_lines - 2;
                s_scroll_row = std::min(s_scroll_row, vertical_scroll_limit);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_UP: // scrolling key - up one screen
            if (in_scrolling_mode && s_scroll_row > 0)
            {
                s_scroll_row -= extra_lines - 2;
                s_scroll_row = std::max(s_scroll_row, 0);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_END: // scrolling key - to end of entry
            if (in_scrolling_mode)
            {
                s_scroll_row = vertical_scroll_limit;
                s_scroll_column = 0;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_HOME: // scrolling key - to beginning of entry
            if (in_scrolling_mode)
            {
                s_scroll_column = 0;
                s_scroll_row = 0;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_F2:
        case ID_KEY_F3:
        case ID_KEY_F4:
        case ID_KEY_F5:
        case ID_KEY_F6:
        case ID_KEY_F7:
        case ID_KEY_F8:
        case ID_KEY_F9:
        case ID_KEY_F10:
            if (s_fn_key_mask & 1 << (done + 1 - ID_KEY_F1))
            {
                return full_screen_exit();
            }
        }
    }
}

void Prompt::display_footing()
{
    if (num_prompts > 1)
    {
        put_string_center(
            instr_row++, 0, 80, C_PROMPT_BKGRD, "Use <Up> and <Down> to select values to change");
    }
    put_string_center(instr_row + 1, 0, 80, C_PROMPT_BKGRD,
        g_help_mode > HelpLabels::HELP_INDEX
            ? "Press ENTER when finished, ESCAPE to back out, or F1 for help"
            : "Press ENTER when finished (or ESCAPE to back out)");
}

int Prompt::prompt_params()
{
    int cur_choice = 0;
    while (values[cur_choice].type == '*')
    {
        ++cur_choice;
    }

    while (!done)
    {
        if (rewrite_extra_info)
        {
            ValueSaver saved_text_col_base{g_text_col_base, 1};
            std::fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(
                scroll_file, extra_info, extra_lines - 2, s_scroll_row, s_scroll_column);
            for (int i = 1; i <= extra_lines - 2; i++)
            {
                driver_put_string(extra_row + i, 0, C_PROMPT_TEXT, blanks);
            }
            driver_put_string(extra_row + 1, 0, C_PROMPT_TEXT, extra_info);
        }

        const int cur_type = values[cur_choice].type;
        const int cur_len = prompt_display_len(&values[cur_choice]);
        std::string field{prompt_value_string(&values[cur_choice])};
        if (!rewrite_extra_info)
        {
            put_string_center(instr_row, 0, 80, C_PROMPT_BKGRD,
                cur_type == 'l' ? "Use <Left> or <Right> to change value of selected field"
                                  : "Type in replacement value for selected field");
        }
        else
        {
            rewrite_extra_info = false;
        }
        driver_put_string(prompt_row + cur_choice, prompt_col, C_PROMPT_HI, prompts[cur_choice]);

        int i;
        if (cur_type == 'l')
        {
            char buf[81];
            copy_string(buf, cur_len, field);
            i = input_field_list(C_PROMPT_CHOOSE, buf, cur_len, values[cur_choice].uval.ch.list,
                values[cur_choice].uval.ch.list_len, prompt_row + cur_choice, value_col,
                in_scrolling_mode ? prompt_check_key_scroll : prompt_check_key);
            int j;
            for (j = 0; j < values[cur_choice].uval.ch.list_len; ++j)
            {
                if (std::strcmp(buf, values[cur_choice].uval.ch.list[j]) == 0)
                {
                    break;
                }
            }
            values[cur_choice].uval.ch.val = j;
            field = buf;
        }
        else
        {
            InputFieldFlags j{};
            if (cur_type == 'i')
            {
                j = InputFieldFlags::NUMERIC | InputFieldFlags::INTEGER;
            }
            if (cur_type == 'L')
            {
                j = InputFieldFlags::NUMERIC | InputFieldFlags::INTEGER;
            }
            if (cur_type == 'd')
            {
                j = InputFieldFlags::NUMERIC | InputFieldFlags::DOUBLE;
            }
            if (cur_type == 'D')
            {
                j = InputFieldFlags::NUMERIC | InputFieldFlags::DOUBLE | InputFieldFlags::INTEGER;
            }
            if (cur_type == 'f')
            {
                j = InputFieldFlags::NUMERIC;
            }
            i = input_field(j, C_PROMPT_INPUT, field, prompt_field_len(&values[cur_choice]), cur_len,
                prompt_row + cur_choice, value_col, in_scrolling_mode ? prompt_check_key_scroll : prompt_check_key);
            switch (values[cur_choice].type)
            {
            case 'd':
            case 'D':
                values[cur_choice].uval.dval = std::atof(field.c_str());
                break;
            case 'f':
                values[cur_choice].uval.dval = std::atof(field.c_str());
                round_float_double(&values[cur_choice].uval.dval);
                break;
            case 'i':
                values[cur_choice].uval.ival = std::atoi(field.c_str());
                break;
            case 'L':
                values[cur_choice].uval.Lval = std::atol(field.c_str());
                break;
            case 's':
                copy_string(values[cur_choice].uval.sval, 15, field);
                break;
            default: // assume 0x100+n
                copy_string(values[cur_choice].uval.sbuf, prompt_field_len(&values[cur_choice]), field);
            }
        }

        driver_put_string(prompt_row + cur_choice, prompt_col, C_PROMPT_LO, prompts[cur_choice]);
        field.resize(cur_len, ' ');
        driver_put_string(prompt_row + cur_choice, value_col, C_PROMPT_LO, field.c_str());

        switch (i)
        {
        case 0: // enter
            done = ID_KEY_ENTER;
            break;
        case -1: // escape
        case ID_KEY_F2:
        case ID_KEY_F3:
        case ID_KEY_F4:
        case ID_KEY_F5:
        case ID_KEY_F6:
        case ID_KEY_F7:
        case ID_KEY_F8:
        case ID_KEY_F9:
        case ID_KEY_F10:
            done = i;
            break;
        case ID_KEY_PAGE_UP:
            cur_choice = -1;
        case ID_KEY_DOWN_ARROW:
            do
            {
                if (++cur_choice >= num_prompts)
                {
                    cur_choice = 0;
                }
            } while (values[cur_choice].type == '*');
            break;
        case ID_KEY_PAGE_DOWN:
            cur_choice = num_prompts;
        case ID_KEY_UP_ARROW:
            do
            {
                if (--cur_choice < 0)
                {
                    cur_choice = num_prompts - 1;
                }
            } while (values[cur_choice].type == '*');
            break;
        case ID_KEY_CTL_DOWN_ARROW: // scrolling key - down one row
            if (in_scrolling_mode && s_scroll_row < vertical_scroll_limit)
            {
                s_scroll_row++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_UP_ARROW: // scrolling key - up one row
            if (in_scrolling_mode && s_scroll_row > 0)
            {
                s_scroll_row--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_LEFT_ARROW: // scrolling key - left one column
            if (in_scrolling_mode && s_scroll_column > 0)
            {
                s_scroll_column--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_RIGHT_ARROW: // scrolling key - right one column
            if (in_scrolling_mode && std::strchr(extra_info, SCROLL_MARKER) != nullptr)
            {
                s_scroll_column++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_DOWN: // scrolling key - down on screen
            if (in_scrolling_mode && s_scroll_row < vertical_scroll_limit)
            {
                s_scroll_row += extra_lines - 2;
                s_scroll_row = std::min(s_scroll_row, vertical_scroll_limit);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_UP: // scrolling key - up one screen
            if (in_scrolling_mode && s_scroll_row > 0)
            {
                s_scroll_row -= extra_lines - 2;
                s_scroll_row = std::max(s_scroll_row, 0);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_END: // scrolling key - go to end of entry
            if (in_scrolling_mode)
            {
                s_scroll_row = vertical_scroll_limit;
                s_scroll_column = 0;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_HOME: // scrolling key - go to beginning of entry
            if (in_scrolling_mode)
            {
                s_scroll_column = 0;
                s_scroll_row = 0;
                rewrite_extra_info = true;
            }
            break;
        }
    }
    return full_screen_exit();
}

int Prompt::prompt()
{
    init_scroll_file();
    count_lines_in_entry();
    display_title();
    count_title_lines();
    count_extra_lines();
    if (entry_fits())
    {
        scroll_off();
    }
    set_scroll_limit();
    set_vertical_positions();
    set_horizontal_positions();
    display_box_heading();
    display_extra_info();
    display_empty_box();
    display_initial_values();
    if (!any_input)
    {
        return prompt_no_params();
    }
    display_footing();
    return prompt_params();
}

int Prompt::full_screen_exit()
{
    driver_hide_text_cursor();
    if (scroll_file)
    {
        std::fclose(scroll_file);
        scroll_file = nullptr;
    }
    return done;
}

} // namespace

int full_screen_prompt(       // full-screen prompting routine
    const char *hdg,          // heading, lines separated by \n
    const int num_prompts,          // there are this many prompts (max)
    const char **prompts,     // array of prompting pointers
    FullScreenValues *values, // array of values
    const int fn_key_mask,          // bit n on if Fn to cause return
    char *extra_info          // extra info box to display, \n separated
)
{
    ValueSaver saved_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    return Prompt{hdg, num_prompts, prompts, values, fn_key_mask, extra_info}.prompt();
}

static std::string prompt_value_string(const FullScreenValues *val)
{
    switch (val->type)
    {
    case 'd':
        return double_to_string(val->uval.dval);
    case 'D':
        return fmt::format("{:d}", std::lround(val->uval.dval));
    case 'f':
        return fmt::format("{:.7g}", val->uval.dval);
    case 'i':
        return fmt::format("{:d}", val->uval.ival);
    case 'L':
        return fmt::format("{:d}", val->uval.Lval);
    case '*':
        return {};
    case 's':
        return val->uval.sval;
    case 'l':
        return val->uval.ch.list[val->uval.ch.val];
    default: // assume 0x100+n
        return val->uval.sbuf;
    }
}

static int prompt_display_len(const FullScreenValues *val)
{
    return val->display_len != 0 ? val->display_len : prompt_field_len(val);
}

static int prompt_field_len(const FullScreenValues *val)
{
    switch (val->type)
    {
    case 'd':
    case 'D':
        return 20;
    case 'f':
        return 14;
    case 'i':
        return 6;
    case 'L':
        return 10;
    case 's':
        return 15;
    case 'l':
        return val->uval.ch.vlen;
    case '*':
        return 0;
    default:
        return val->type & 0xff;
    }
}

static int fn_key_mask_selected(const int key)
{
    return s_fn_key_mask & 1 << (key - ID_KEY_F1 + 1);
}

static int prompt_check_key(const int key)
{
    switch (key)
    {
    case ID_KEY_PAGE_UP:
    case ID_KEY_DOWN_ARROW:
    case ID_KEY_PAGE_DOWN:
    case ID_KEY_UP_ARROW:
        return key;

    case ID_KEY_F2:
    case ID_KEY_F3:
    case ID_KEY_F4:
    case ID_KEY_F5:
    case ID_KEY_F6:
    case ID_KEY_F7:
    case ID_KEY_F8:
    case ID_KEY_F9:
    case ID_KEY_F10:
        if (fn_key_mask_selected(key))
        {
            return key;
        }
        break;

    default:
        break;
    }
    return 0;
}

static int prompt_check_key_scroll(const int key)
{
    switch (key)
    {
    case ID_KEY_PAGE_UP:
    case ID_KEY_DOWN_ARROW:
    case ID_KEY_CTL_DOWN_ARROW:
    case ID_KEY_PAGE_DOWN:
    case ID_KEY_UP_ARROW:
    case ID_KEY_CTL_UP_ARROW:
    case ID_KEY_CTL_LEFT_ARROW:
    case ID_KEY_CTL_RIGHT_ARROW:
    case ID_KEY_CTL_PAGE_DOWN:
    case ID_KEY_CTL_PAGE_UP:
    case ID_KEY_CTL_END:
    case ID_KEY_CTL_HOME:
        return key;

    case ID_KEY_F2:
    case ID_KEY_F3:
    case ID_KEY_F4:
    case ID_KEY_F5:
    case ID_KEY_F6:
    case ID_KEY_F7:
    case ID_KEY_F8:
    case ID_KEY_F9:
    case ID_KEY_F10:
        if (fn_key_mask_selected(key))
        {
            return key;
        }
        break;

    default:
        break;
    }
    return 0;
}

static int input_field_list(const int attr, // display attribute
    char *field,                            // display form field value
    const int field_len,                    // field length
    const char **list,                      // list of values
    const int list_len,                     // number of entries in list
    const int row,                          // display row
    const int col,                          // display column
    int (*check_key)(int key)               // routine to check non data keys, or nullptr
)
{
    int init_val;
    ValueSaver save_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    for (init_val = 0; init_val < list_len; ++init_val)
    {
        if (std::strcmp(field, list[init_val]) == 0)
        {
            break;
        }
    }
    if (init_val >= list_len)
    {
        init_val = 0;
    }
    int cur_val = init_val;
    int ret = -1;
    while (true)
    {
        std::string display{list[cur_val]};
        display.resize(static_cast<std::size_t>(std::max(field_len, 0)), ' ');
        driver_put_string(row, col, attr, display);
        switch (const int key = driver_key_cursor(row, col); key)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = 0;
            goto end;
        case ID_KEY_ESC:
            goto end;
        case ID_KEY_RIGHT_ARROW:
            if (++cur_val >= list_len)
            {
                cur_val = 0;
            }
            break;
        case ID_KEY_LEFT_ARROW:
            if (--cur_val < 0)
            {
                cur_val = list_len - 1;
            }
            break;
        case ID_KEY_F5:
            cur_val = init_val;
            break;
        default:
            if (non_alpha(key))
            {
                if (check_key && (ret = (*check_key)(key)) != 0)
                {
                    goto end;
                }
                break;                                // non alphanum char
            }
            int j = cur_val;
            for (int i = 0; i < list_len; ++i)
            {
                if (++j >= list_len)
                {
                    j = 0;
                }
                if ((*list[j] & 0xdf) == (key & 0xdf))
                {
                    cur_val = j;
                    break;
                }
            }
        }
    }
end:
    copy_string(field, field_len, list[cur_val]);
    return ret;
}

} // namespace id::ui
