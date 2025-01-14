// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/full_screen_prompt.h"

#include "engine/id_data.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "io/load_entry_text.h"
#include "math/round_float_double.h"
#include "misc/drivers.h"
#include "misc/ValueSaver.h"
#include "ui/cmdfiles.h"
#include "ui/double_to_string.h"
#include "ui/file_item.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/input_field.h"
#include "ui/mouse.h"
#include "ui/put_string_center.h"
#include "ui/text_screen.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

static int prompt_check_key(int key);
static int prompt_check_key_scroll(int key);
static  int input_field_list(int attr, char *field, int field_len, char const **list, int list_len,
                             int row, int col, int (*check_key)(int key));
static int prompt_value_string(char *buf, FullScreenValues const *val);

static int s_fn_key_mask{};

// These need to be global because F6 exits fullscreen_prompt()
static int s_scroll_row_status{};    // will be set to first line of extra info to be displayed (0 = top line)
static int s_scroll_column_status{}; // will be set to first column of extra info to be displayed (0 = leftmost column)

void full_screen_reset_scrolling()
{
    s_scroll_row_status = 0; // make sure we start at beginning of entry
    s_scroll_column_status = 0;
}

namespace
{

struct Prompt
{
    Prompt(char const *hdg,       // heading, lines separated by \n
        int num_prompts,          // there are this many prompts (max)
        char const **prompts,     // array of prompting pointers
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
    
    char const *hdg;             // heading, lines separated by \n
    int num_prompts;             // there are this many prompts (max)
    char const **prompts;        // array of prompting pointers
    FullScreenValues *values;    // array of values
    char *extra_info;            // extra info box to display, \n separated
    std::FILE *scroll_file{};    // file with extra_info entry to scroll
    long scroll_file_start{};    // where entry starts in scroll_file
    bool in_scrolling_mode{};    // will be true if need to scroll extra_info
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

Prompt::Prompt(char const *hdg, int num_prompts, char const **prompts, FullScreenValues *values,
    int fn_key_mask, char *extra_info) :
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
        if (g_fractal_type == FractalType::FORMULA || g_fractal_type == FractalType::FORMULA_FP)
        {
            find_file_item(g_formula_filename, g_formula_name.c_str(), &scroll_file, ItemType::FORMULA);
            in_scrolling_mode = true;
            scroll_file_start = std::ftell(scroll_file);
        }
        else if (g_fractal_type == FractalType::L_SYSTEM)
        {
            find_file_item(g_l_system_filename, g_l_system_name.c_str(), &scroll_file, ItemType::L_SYSTEM);
            in_scrolling_mode = true;
            scroll_file_start = std::ftell(scroll_file);
        }
        else if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
        {
            find_file_item(g_ifs_filename, g_ifs_name.c_str(), &scroll_file, ItemType::IFS);
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
        while ((c = fgetc(scroll_file)) != EOF)
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
        if (*(scan++) == '\n')
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
                if (*(scan++) == '\n')
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
        && s_scroll_row_status == 0          //
        && lines_in_entry == extra_lines - 2 //
        && s_scroll_column_status == 0       //
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
        int i = num_prompts + title_lines + extra_lines + 3; // total rows required
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
            static char const *noyes[2] = {"no", "yes"};
            values[i].type = 'l';
            values[i].uval.ch.vlen = 3;
            values[i].uval.ch.list = noyes;
            values[i].uval.ch.list_len = 2;
        }
        int j = (int) std::strlen(prompts[i]);
        if (values[i].type == '*')
        {
            max_comment = std::max(j, max_comment);
        }
        else
        {
            any_input = true;
            max_prompt_width = std::max(j, max_prompt_width);
            char buf[81];
            j = prompt_value_string(buf, &values[i]);
            max_field_width = std::max(j, max_field_width);
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
        int i = (90 - box_width) / 20;
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

    {
        char buffer[256];
        char *hdg_line = buffer;
        // center each line of heading independently
        int i;
        std::strcpy(hdg_line, hdg);
        for (i = 0; i < title_lines - 1; i++)
        {
            char *next = std::strchr(hdg_line, '\n');
            if (next == nullptr)
            {
                break; // shouldn't happen
            }
            *next = '\0';
            title_width = (int) std::strlen(hdg_line);
            g_text_col_base = box_col + (box_width - title_width) / 2;
            driver_put_string(title_row + i, 0, C_PROMPT_HI, hdg_line);
            *next = '\n';
            hdg_line = next + 1;
        }
        // add scrolling key message, if applicable
        if (in_scrolling_mode)
        {
            *(hdg_line + 31) = (char) 0; // replace the ')'
            std::strcat(hdg_line, ". Ctrl+<arrow key> to scroll text.)");
        }

        title_width = (int) std::strlen(hdg_line);
        g_text_col_base = box_col + (box_width - title_width) / 2;
        driver_put_string(title_row + i, 0, C_PROMPT_HI, hdg_line);
    }
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
    g_text_col_base = box_col + 1;
    driver_put_string(extra_row, 0, C_PROMPT_BKGRD, buf);
    driver_put_string(extra_row + extra_lines - 1, 0, C_PROMPT_BKGRD, buf);
    --g_text_col_base;
    driver_put_string(extra_row, 0, C_PROMPT_BKGRD, UPPER_LEFT);
    driver_put_string(extra_row + extra_lines - 1, 0, C_PROMPT_BKGRD, LOWER_LEFT);
    g_text_col_base += box_width - 1;
    driver_put_string(extra_row, 0, C_PROMPT_BKGRD, UPPER_RIGHT);
    driver_put_string(extra_row + extra_lines - 1, 0, C_PROMPT_BKGRD, LOWER_RIGHT);

    g_text_col_base = box_col;

    for (int i = 1; i < extra_lines - 1; ++i)
    {
        driver_put_string(extra_row + i, 0, C_PROMPT_BKGRD, VERT_LINE);
        driver_put_string(extra_row + i, box_width - 1, C_PROMPT_BKGRD, VERT_LINE);
    }
    g_text_col_base += (box_width - extra_width) / 2;
    const std::string extra_text{extra_info};
    std::string::size_type line_begin{};
    for (int i = 1; i < extra_lines - 1; ++i)
    {
        const std::string::size_type line_end = extra_text.find('\n', line_begin);
        const std::string line{extra_text.substr(line_begin, line_end - line_begin)};
        driver_put_string(extra_row + i, 0, C_PROMPT_TEXT, line.c_str());
        line_begin = line_end + 1;
    }
}

void Prompt::display_empty_box()
{
    g_text_col_base = 0;

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
        char buf[81];
        prompt_value_string(buf, &values[i]);
        driver_put_string(prompt_row + i, value_col, C_PROMPT_LO, buf);
    }
}

int Prompt::prompt_no_params()
{
    put_string_center(instr_row++, 0, 80, C_PROMPT_BKGRD, "No changeable parameters;");
    put_string_center(instr_row, 0, 80, C_PROMPT_BKGRD,
        (g_help_mode > HelpLabels::HELP_INDEX) ? "Press ENTER to exit, ESC to back out, F1 for help"
                                               : "Press ENTER to exit");
    driver_hide_text_cursor();
    g_text_col_base = 2;
    while (true)
    {
        if (rewrite_extra_info)
        {
            rewrite_extra_info = false;
            std::fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(
                scroll_file, extra_info, extra_lines - 2, s_scroll_row_status, s_scroll_column_status);
            for (int i = 1; i <= extra_lines - 2; i++)
            {
                driver_put_string(extra_row + i, 0, C_PROMPT_TEXT, blanks);
            }
            driver_put_string(extra_row + 1, 0, C_PROMPT_TEXT, extra_info);
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
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_UP_ARROW: // scrolling key - up one row
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_LEFT_ARROW: // scrolling key - left one column
            if (in_scrolling_mode && s_scroll_column_status > 0)
            {
                s_scroll_column_status--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_RIGHT_ARROW: // scrolling key - right one column
            if (in_scrolling_mode && std::strchr(extra_info, SCROLL_MARKER) != nullptr)
            {
                s_scroll_column_status++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_DOWN: // scrolling key - down one screen
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status += extra_lines - 2;
                s_scroll_row_status = std::min(s_scroll_row_status, vertical_scroll_limit);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_UP: // scrolling key - up one screen
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status -= extra_lines - 2;
                s_scroll_row_status = std::max(s_scroll_row_status, 0);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_END: // scrolling key - to end of entry
            if (in_scrolling_mode)
            {
                s_scroll_row_status = vertical_scroll_limit;
                s_scroll_column_status = 0;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_HOME: // scrolling key - to beginning of entry
            if (in_scrolling_mode)
            {
                s_scroll_column_status = 0;
                s_scroll_row_status = 0;
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
            if (s_fn_key_mask & (1 << (done + 1 - ID_KEY_F1)))
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
        (g_help_mode > HelpLabels::HELP_INDEX)
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
            int j = g_text_col_base;
            g_text_col_base = 2;
            std::fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(
                scroll_file, extra_info, extra_lines - 2, s_scroll_row_status, s_scroll_column_status);
            for (int i = 1; i <= extra_lines - 2; i++)
            {
                driver_put_string(extra_row + i, 0, C_PROMPT_TEXT, blanks);
            }
            driver_put_string(extra_row + 1, 0, C_PROMPT_TEXT, extra_info);
            g_text_col_base = j;
        }

        const int cur_type = values[cur_choice].type;
        char buf[81];
        const int cur_len = prompt_value_string(buf, &values[cur_choice]);
        if (!rewrite_extra_info)
        {
            put_string_center(instr_row, 0, 80, C_PROMPT_BKGRD,
                (cur_type == 'l') ? "Use <Left> or <Right> to change value of selected field"
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
            i = input_field(j, C_PROMPT_INPUT, buf, cur_len, prompt_row + cur_choice, value_col,
                in_scrolling_mode ? prompt_check_key_scroll : prompt_check_key);
            switch (values[cur_choice].type)
            {
            case 'd':
            case 'D':
                values[cur_choice].uval.dval = std::atof(buf);
                break;
            case 'f':
                values[cur_choice].uval.dval = std::atof(buf);
                round_float_double(&values[cur_choice].uval.dval);
                break;
            case 'i':
                values[cur_choice].uval.ival = std::atoi(buf);
                break;
            case 'L':
                values[cur_choice].uval.Lval = std::atol(buf);
                break;
            case 's':
                std::strncpy(values[cur_choice].uval.sval, buf, 16);
                break;
            default: // assume 0x100+n
                std::strcpy(values[cur_choice].uval.sbuf, buf);
            }
        }

        driver_put_string(prompt_row + cur_choice, prompt_col, C_PROMPT_LO, prompts[cur_choice]);
        {
            int j = (int) std::strlen(buf);
            std::memset(&buf[j], ' ', 80 - j);
        }
        buf[cur_len] = 0;
        driver_put_string(prompt_row + cur_choice, value_col, C_PROMPT_LO, buf);

        switch (i)
        {
        case 0: // enter
            done = 13;
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
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_UP_ARROW: // scrolling key - up one row
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_LEFT_ARROW: // scrolling key - left one column
            if (in_scrolling_mode && s_scroll_column_status > 0)
            {
                s_scroll_column_status--;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_RIGHT_ARROW: // scrolling key - right one column
            if (in_scrolling_mode && std::strchr(extra_info, SCROLL_MARKER) != nullptr)
            {
                s_scroll_column_status++;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_DOWN: // scrolling key - down on screen
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status += extra_lines - 2;
                s_scroll_row_status = std::min(s_scroll_row_status, vertical_scroll_limit);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_PAGE_UP: // scrolling key - up one screen
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status -= extra_lines - 2;
                s_scroll_row_status = std::max(s_scroll_row_status, 0);
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_END: // scrolling key - go to end of entry
            if (in_scrolling_mode)
            {
                s_scroll_row_status = vertical_scroll_limit;
                s_scroll_column_status = 0;
                rewrite_extra_info = true;
            }
            break;
        case ID_KEY_CTL_HOME: // scrolling key - go to beginning of entry
            if (in_scrolling_mode)
            {
                s_scroll_column_status = 0;
                s_scroll_row_status = 0;
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
};

} // namespace

int full_screen_prompt(       // full-screen prompting routine
    char const *hdg,          // heading, lines separated by \n
    int num_prompts,          // there are this many prompts (max)
    char const **prompts,     // array of prompting pointers
    FullScreenValues *values, // array of values
    int fn_key_mask,          // bit n on if Fn to cause return
    char *extra_info          // extra info box to display, \n separated
)
{
    ValueSaver saved_look_at_mouse{g_look_at_mouse, +MouseLook::IGNORE_MOUSE};
    return Prompt{hdg, num_prompts, prompts, values, fn_key_mask, extra_info}.prompt();
}

// format value into buf, return field width
static int prompt_value_string(char *buf, FullScreenValues const *val)
{
    int ret;
    switch (val->type)
    {
    case 'd':
        ret = 20;
        double_to_string(buf, val->uval.dval);
        break;
    case 'D':
        std::sprintf(buf, "%ld", std::lround(val->uval.dval));
        ret = 20;
        break;
    case 'f':
        std::sprintf(buf, "%.7g", val->uval.dval);
        ret = 14;
        break;
    case 'i':
        std::sprintf(buf, "%d", val->uval.ival);
        ret = 6;
        break;
    case 'L':
        std::sprintf(buf, "%ld", val->uval.Lval);
        ret = 10;
        break;
    case '*':
        ret = 0;
        *buf = (char) ret;
        break;
    case 's':
        std::strncpy(buf, val->uval.sval, 16);
        buf[15] = 0;
        ret = 15;
        break;
    case 'l':
        std::strcpy(buf, val->uval.ch.list[val->uval.ch.val]);
        ret = val->uval.ch.vlen;
        break;
    default: // assume 0x100+n
        std::strcpy(buf, val->uval.sbuf);
        ret = val->type & 0xff;
    }
    return ret;
}

static int fn_key_mask_selected(int key)
{
    return s_fn_key_mask & (1 << (key - ID_KEY_F1 + 1));
}

static int prompt_check_key(int key)
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

static int prompt_check_key_scroll(int key)
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

static int input_field_list(int attr, // display attribute
    char *field,                      // display form field value
    int field_len,                    // field length
    char const **list,                // list of values
    int list_len,                     // number of entries in list
    int row,                          // display row
    int col,                          // display column
    int (*check_key)(int key)         // routine to check non data keys, or nullptr
)
{
    int init_val;
    char buf[81];
    ValueSaver save_look_at_mouse{g_look_at_mouse, +MouseLook::IGNORE_MOUSE};
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
        std::strcpy(buf, list[cur_val]);
        {
            int i = (int) std::strlen(buf);
            while (i < field_len)
            {
                buf[i++] = ' ';
            }
        }
        buf[field_len] = 0;
        driver_put_string(row, col, attr, buf);
        switch (int key = driver_key_cursor(row, col); key)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = 0;
            goto inpfldl_end;
        case ID_KEY_ESC:
            goto inpfldl_end;
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
                    goto inpfldl_end;
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
inpfldl_end:
    std::strcpy(field, list[cur_val]);
    return ret;
}
