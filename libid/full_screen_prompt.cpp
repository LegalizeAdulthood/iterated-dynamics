#include "full_screen_prompt.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "double_to_string.h"
#include "drivers.h"
#include "file_item.h"
#include "fractype.h"
#include "helpdefs.h"
#include "help_title.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "input_field.h"
#include "load_entry_text.h"
#include "os.h"
#include "put_string_center.h"
#include "round_float_double.h"
#include "text_screen.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int prompt_checkkey(int curkey);
static int prompt_checkkey_scroll(int curkey);
static  int input_field_list(int attr, char *fld, int vlen, char const **list, int llen,
                             int row, int col, int (*checkkey)(int));
static int prompt_valuestring(char *buf, fullscreenvalues const *val);

static int s_prompt_fn_keys{};

// These need to be global because F6 exits fullscreen_prompt()
static int s_scroll_row_status{};    // will be set to first line of extra info to be displayed (0 = top line)
static int s_scroll_column_status{}; // will be set to first column of extra info to be displayed (0 = leftmost column)

void full_screen_reset_scrolling()
{
    s_scroll_row_status = 0; // make sure we start at beginning of entry
    s_scroll_column_status = 0;
}

int fullscreen_prompt(        // full-screen prompting routine
    char const *hdg,          // heading, lines separated by \n
    int num_prompts,          // there are this many prompts (max)
    char const **prompts,     // array of prompting pointers
    fullscreenvalues *values, // array of values
    int fn_key_mask,          // bit n on if Fn to cause return
    char *extra_info          // extra info box to display, \n separated
)
{
    const int old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    s_prompt_fn_keys = fn_key_mask;

    /* If applicable, open file for scrolling extrainfo. The function
       find_file_item() opens the file and sets the file pointer to the
       beginning of the entry.
    */
    std::FILE *scroll_file = nullptr; // file with extrainfo entry to scroll
    long scroll_file_start = 0;       // where entry starts in scroll_file
    bool in_scrolling_mode = false;   // will be true if need to scroll extrainfo
    if (extra_info && *extra_info)
    {
        if (g_fractal_type == fractal_type::FORMULA || g_fractal_type == fractal_type::FFORMULA)
        {
            find_file_item(g_formula_filename, g_formula_name.c_str(), &scroll_file, gfe_type::FORMULA);
            in_scrolling_mode = true;
            scroll_file_start = ftell(scroll_file);
        }
        else if (g_fractal_type == fractal_type::LSYSTEM)
        {
            find_file_item(g_l_system_filename, g_l_system_name.c_str(), &scroll_file, gfe_type::L_SYSTEM);
            in_scrolling_mode = true;
            scroll_file_start = ftell(scroll_file);
        }
        else if (g_fractal_type == fractal_type::IFS || g_fractal_type == fractal_type::IFS3D)
        {
            find_file_item(g_ifs_filename, g_ifs_name.c_str(), &scroll_file, gfe_type::IFS);
            in_scrolling_mode = true;
            scroll_file_start = ftell(scroll_file);
        }
    }

    // initialize widest_entry_line and lines_in_entry
    int lines_in_entry = 0;    // total lines in entry to be scrolled
    int widest_entry_line = 0; // length of longest line in entry
    if (in_scrolling_mode && scroll_file != nullptr)
    {
        bool comment = false;
        int c = 0;
        int line_width = -1;
        while ((c = fgetc(scroll_file)) != EOF && c != '\032')
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n')
            {
                comment = false;
                lines_in_entry++;
                line_width =  -1;
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
        if (c == EOF || c == '\032')
        {
            // should never happen
            std::fclose(scroll_file);
            in_scrolling_mode = false;
        }
    }

    helptitle();                        // clear screen, display title line
    driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);  // init rest of screen to background

    int title_width = 0;                      // count title lines, find widest
    int title_lines = 1;
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
    int extra_width = 0;
    int extra_lines = 0;
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

    // if entry fits in available space, shut off scrolling
    if (in_scrolling_mode && s_scroll_row_status == 0
        && lines_in_entry == extra_lines - 2
        && s_scroll_column_status == 0
        && std::strchr(extra_info, '\021') == nullptr)
    {
        in_scrolling_mode = false;
        std::fclose(scroll_file);
        scroll_file = nullptr;
    }

    /*initialize vertical scroll limit. When the top line of the text
      box is the vertical scroll limit, the bottom line is the end of the
      entry, and no further down scrolling is necessary.
    */
    int vertical_scroll_limit = 0; // don't scroll down if this is top line
    if (in_scrolling_mode)
    {
        vertical_scroll_limit = lines_in_entry - (extra_lines - 2);
    }

    // work out vertical positioning
    int title_row;
    int box_lines;
    {
        int i = num_prompts + title_lines + extra_lines + 3; // total rows required
        int j = (25 - i) / 2;                   // top row of it all when centered
        j -= j / 4;                         // higher is better if lots extra
        box_lines = num_prompts;
        title_row = 1 + j;
    }
    int box_row = title_row + title_lines;
    int prompt_row = box_row;
    if (title_row > 2)
    {
        // room for blank between title & box?
        --title_row;
        --box_row;
        ++box_lines;
    }
    int instr_row = box_row + box_lines;
    if (instr_row + 3 + extra_lines < 25)
    {
        ++box_lines;    // blank at bottom of box
        ++instr_row;
        if (instr_row + 3 + extra_lines < 25)
        {
            ++instr_row; // blank before instructions
        }
    }
    int extra_row = instr_row + 2;
    if (num_prompts > 1)   // 3 instructions lines
    {
        ++extra_row;
    }
    if (extra_row + extra_lines < 25)
    {
        ++extra_row;
    }

    if (in_scrolling_mode)    // set box to max width if in scrolling mode
    {
        extra_width = 76;
    }

    // work out horizontal positioning
    int max_comment = 0;
    int max_prompt_width = 0;
    int max_field_width = 0;
    bool any_input = false;
    for (int i = 0; i < num_prompts; i++)
    {
        if (values[i].type == 'y')
        {
            static char const *noyes[2] = {"no", "yes"};
            values[i].type = 'l';
            values[i].uval.ch.vlen = 3;
            values[i].uval.ch.list = noyes;
            values[i].uval.ch.llen = 2;
        }
        int j = (int) std::strlen(prompts[i]);
        if (values[i].type == '*')
        {
            if (j > max_comment)
            {
                max_comment = j;
            }
        }
        else
        {
            any_input = true;
            if (j > max_prompt_width)
            {
                max_prompt_width = j;
            }
            char buf[81];
            j = prompt_valuestring(buf, &values[i]);
            if (j > max_field_width)
            {
                max_field_width = j;
            }
        }
    }
    int box_width = max_prompt_width + max_field_width + 2;
    if (max_comment > box_width)
    {
        box_width = max_comment;
    }
    if ((box_width += 4) > 80)
    {
        box_width = 80;
    }
    int box_col = (80 - box_width) / 2;       // center the box
    int prompt_col = box_col + 2;
    int valuecol = box_col + box_width - max_field_width - 2;
    if (box_width <= 76)
    {
        // make margin a bit wider if we can
        box_width += 2;
        --box_col;
    }
    {
        int j = title_width;
        if (j < extra_width)
        {
            j = extra_width;
        }
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
        box_col    -= i;
        prompt_col -= i;
        valuecol  -= i;
    }

    // display box heading
    for (int i = title_row; i < box_row; ++i)
    {
        driver_set_attr(i, box_col, C_PROMPT_HI, box_width);
    }

    {
        char buffer[256];
        char *hdgline = buffer;
        // center each line of heading independently
        int i;
        std::strcpy(hdgline, hdg);
        for (i = 0; i < title_lines-1; i++)
        {
            char *next = std::strchr(hdgline, '\n');
            if (next == nullptr)
            {
                break; // shouldn't happen
            }
            *next = '\0';
            title_width = (int) std::strlen(hdgline);
            g_text_cbase = box_col + (box_width - title_width) / 2;
            driver_put_string(title_row+i, 0, C_PROMPT_HI, hdgline);
            *next = '\n';
            hdgline = next+1;
        }
        // add scrolling key message, if applicable
        if (in_scrolling_mode)
        {
            *(hdgline + 31) = (char) 0;   // replace the ')'
            std::strcat(hdgline, ". CTRL+(direction key) to scroll text.)");
        }

        title_width = (int) std::strlen(hdgline);
        g_text_cbase = box_col + (box_width - title_width) / 2;
        driver_put_string(title_row+i, 0, C_PROMPT_HI, hdgline);
    }

    // display extra info
    if (extra_info)
    {
#ifndef XFRACT
#define S1 '\xC4'
#define S2 "\xC0"
#define S3 "\xD9"
#define S4 "\xB3"
#define S5 "\xDA"
#define S6 "\xBF"
#else
#define S1 '-'
#define S2 "+" // ll corner
#define S3 "+" // lr corner
#define S4 "|"
#define S5 "+" // ul corner
#define S6 "+" // ur corner
#endif
        char buf[81];
        std::memset(buf, S1, 80);
        buf[box_width-2] = 0;
        g_text_cbase = box_col + 1;
        driver_put_string(extra_row, 0, C_PROMPT_BKGRD, buf);
        driver_put_string(extra_row+extra_lines-1, 0, C_PROMPT_BKGRD, buf);
        --g_text_cbase;
        driver_put_string(extra_row, 0, C_PROMPT_BKGRD, S5);
        driver_put_string(extra_row+extra_lines-1, 0, C_PROMPT_BKGRD, S2);
        g_text_cbase += box_width - 1;
        driver_put_string(extra_row, 0, C_PROMPT_BKGRD, S6);
        driver_put_string(extra_row+extra_lines-1, 0, C_PROMPT_BKGRD, S3);

        g_text_cbase = box_col;

        for (int i = 1; i < extra_lines-1; ++i)
        {
            driver_put_string(extra_row+i, 0, C_PROMPT_BKGRD, S4);
            driver_put_string(extra_row+i, box_width-1, C_PROMPT_BKGRD, S4);
        }
        g_text_cbase += (box_width - extra_width) / 2;
        driver_put_string(extra_row+1, 0, C_PROMPT_TEXT, extra_info);
    }

    g_text_cbase = 0;

    // display empty box
    for (int i = 0; i < box_lines; ++i)
    {
        driver_set_attr(box_row+i, box_col, C_PROMPT_LO, box_width);
    }

    // display initial values
    for (int i = 0; i < num_prompts; i++)
    {
        driver_put_string(prompt_row+i, prompt_col, C_PROMPT_LO, prompts[i]);
        char buf[81];
        prompt_valuestring(buf, &values[i]);
        driver_put_string(prompt_row+i, valuecol, C_PROMPT_LO, buf);
    }

    bool rewrite_extrainfo = false; // if true: rewrite extrainfo to text box
    char blanks[78];                // used to clear text box
    std::memset(blanks, ' ', 77);   // initialize string of blanks
    blanks[77] = 0;
    int done{};
    const auto fullscreen_exit = [&]
    {
        driver_hide_text_cursor();
        g_look_at_mouse = old_look_at_mouse;
        if (scroll_file)
        {
            std::fclose(scroll_file);
            scroll_file = nullptr;
        }
        return done;
    };
    if (!any_input)
    {
        putstringcenter(instr_row++, 0, 80, C_PROMPT_BKGRD,
                        "No changeable parameters;");
        putstringcenter(instr_row, 0, 80, C_PROMPT_BKGRD,
                (g_help_mode > help_labels::HELP_INDEX) ?
                "Press ENTER to exit, ESC to back out, F1 for help"
                : "Press ENTER to exit");
        driver_hide_text_cursor();
        g_text_cbase = 2;
        while (true)
        {
            if (rewrite_extrainfo)
            {
                rewrite_extrainfo = false;
                std::fseek(scroll_file, scroll_file_start, SEEK_SET);
                load_entry_text(scroll_file, extra_info, extra_lines - 2,
                                s_scroll_row_status, s_scroll_column_status);
                for (int i = 1; i <= extra_lines-2; i++)
                {
                    driver_put_string(extra_row+i, 0, C_PROMPT_TEXT, blanks);
                }
                driver_put_string(extra_row+1, 0, C_PROMPT_TEXT, extra_info);
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
                return fullscreen_exit();
            case ID_KEY_CTL_DOWN_ARROW:    // scrolling key - down one row
                if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
                {
                    s_scroll_row_status++;
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_UP_ARROW:      // scrolling key - up one row
                if (in_scrolling_mode && s_scroll_row_status > 0)
                {
                    s_scroll_row_status--;
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_LEFT_ARROW:    // scrolling key - left one column
                if (in_scrolling_mode && s_scroll_column_status > 0)
                {
                    s_scroll_column_status--;
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_RIGHT_ARROW:   // scrolling key - right one column
                if (in_scrolling_mode && std::strchr(extra_info, '\021') != nullptr)
                {
                    s_scroll_column_status++;
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_PAGE_DOWN:   // scrolling key - down one screen
                if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
                {
                    s_scroll_row_status += extra_lines - 2;
                    if (s_scroll_row_status > vertical_scroll_limit)
                    {
                        s_scroll_row_status = vertical_scroll_limit;
                    }
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_PAGE_UP:     // scrolling key - up one screen
                if (in_scrolling_mode && s_scroll_row_status > 0)
                {
                    s_scroll_row_status -= extra_lines - 2;
                    if (s_scroll_row_status < 0)
                    {
                        s_scroll_row_status = 0;
                    }
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_END:         // scrolling key - to end of entry
                if (in_scrolling_mode)
                {
                    s_scroll_row_status = vertical_scroll_limit;
                    s_scroll_column_status = 0;
                    rewrite_extrainfo = true;
                }
                break;
            case ID_KEY_CTL_HOME:        // scrolling key - to beginning of entry
                if (in_scrolling_mode)
                {
                    s_scroll_column_status = 0;
                    s_scroll_row_status = s_scroll_column_status;
                    rewrite_extrainfo = true;
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
                if (s_prompt_fn_keys & (1 << (done+1-ID_KEY_F1)))
                {
                    return fullscreen_exit();
                }
            }
        }
    }

    // display footing
    if (num_prompts > 1)
    {
        putstringcenter(instr_row++, 0, 80, C_PROMPT_BKGRD,
                        "Use <Up> and <Down> to select values to change");
    }
    putstringcenter(instr_row+1, 0, 80, C_PROMPT_BKGRD,
            (g_help_mode > help_labels::HELP_INDEX) ?
            "Press ENTER when finished, ESCAPE to back out, or F1 for help"
            : "Press ENTER when finished (or ESCAPE to back out)");

    int curchoice = 0;
    while (values[curchoice].type == '*')
    {
        ++curchoice;
    }

    while (!done)
    {
        if (rewrite_extrainfo)
        {
            int j = g_text_cbase;
            g_text_cbase = 2;
            std::fseek(scroll_file, scroll_file_start, SEEK_SET);
            load_entry_text(scroll_file, extra_info, extra_lines - 2,
                            s_scroll_row_status, s_scroll_column_status);
            for (int i = 1; i <= extra_lines-2; i++)
            {
                driver_put_string(extra_row+i, 0, C_PROMPT_TEXT, blanks);
            }
            driver_put_string(extra_row+1, 0, C_PROMPT_TEXT, extra_info);
            g_text_cbase = j;
        }

        const int curtype = values[curchoice].type;
        char buf[81];
        const int curlen = prompt_valuestring(buf, &values[curchoice]);
        if (!rewrite_extrainfo)
        {
            putstringcenter(instr_row, 0, 80, C_PROMPT_BKGRD,
                (curtype == 'l') ?
                "Use <Left> or <Right> to change value of selected field"
                : "Type in replacement value for selected field");
        }
        else
        {
            rewrite_extrainfo = false;
        }
        driver_put_string(prompt_row+curchoice, prompt_col, C_PROMPT_HI, prompts[curchoice]);

        int i;
        if (curtype == 'l')
        {
            i = input_field_list(
                    C_PROMPT_CHOOSE, buf, curlen,
                    values[curchoice].uval.ch.list, values[curchoice].uval.ch.llen,
                    prompt_row+curchoice, valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
            int j;
            for (j = 0; j < values[curchoice].uval.ch.llen; ++j)
            {
                if (std::strcmp(buf, values[curchoice].uval.ch.list[j]) == 0)
                {
                    break;
                }
            }
            values[curchoice].uval.ch.val = j;
        }
        else
        {
            int j = 0;
            if (curtype == 'i')
            {
                j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
            }
            if (curtype == 'L')
            {
                j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
            }
            if (curtype == 'd')
            {
                j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE;
            }
            if (curtype == 'D')
            {
                j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE | INPUTFIELD_INTEGER;
            }
            if (curtype == 'f')
            {
                j = INPUTFIELD_NUMERIC;
            }
            i = input_field(j, C_PROMPT_INPUT, buf, curlen,
                            prompt_row+curchoice, valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
            switch (values[curchoice].type)
            {
            case 'd':
            case 'D':
                values[curchoice].uval.dval = std::atof(buf);
                break;
            case 'f':
                values[curchoice].uval.dval = std::atof(buf);
                roundfloatd(&values[curchoice].uval.dval);
                break;
            case 'i':
                values[curchoice].uval.ival = std::atoi(buf);
                break;
            case 'L':
                values[curchoice].uval.Lval = std::atol(buf);
                break;
            case 's':
                std::strncpy(values[curchoice].uval.sval, buf, 16);
                break;
            default: // assume 0x100+n
                std::strcpy(values[curchoice].uval.sbuf, buf);
            }
        }

        driver_put_string(prompt_row+curchoice, prompt_col, C_PROMPT_LO, prompts[curchoice]);
        {
            int j = (int) std::strlen(buf);
            std::memset(&buf[j], ' ', 80-j);
        }
        buf[curlen] = 0;
        driver_put_string(prompt_row+curchoice, valuecol, C_PROMPT_LO,  buf);

        switch (i)
        {
        case 0:  // enter
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
            curchoice = -1;
        case ID_KEY_DOWN_ARROW:
            do
            {
                if (++curchoice >= num_prompts)
                {
                    curchoice = 0;
                }
            }
            while (values[curchoice].type == '*');
            break;
        case ID_KEY_PAGE_DOWN:
            curchoice = num_prompts;
        case ID_KEY_UP_ARROW:
            do
            {
                if (--curchoice < 0)
                {
                    curchoice = num_prompts - 1;
                }
            }
            while (values[curchoice].type == '*');
            break;
        case ID_KEY_CTL_DOWN_ARROW:     // scrolling key - down one row
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status++;
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_UP_ARROW:       // scrolling key - up one row
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status--;
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_LEFT_ARROW:     //scrolling key - left one column
            if (in_scrolling_mode && s_scroll_column_status > 0)
            {
                s_scroll_column_status--;
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_RIGHT_ARROW:    // scrolling key - right one column
            if (in_scrolling_mode && std::strchr(extra_info, '\021') != nullptr)
            {
                s_scroll_column_status++;
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_PAGE_DOWN:    // scrolling key - down on screen
            if (in_scrolling_mode && s_scroll_row_status < vertical_scroll_limit)
            {
                s_scroll_row_status += extra_lines - 2;
                if (s_scroll_row_status > vertical_scroll_limit)
                {
                    s_scroll_row_status = vertical_scroll_limit;
                }
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_PAGE_UP:      // scrolling key - up one screen
            if (in_scrolling_mode && s_scroll_row_status > 0)
            {
                s_scroll_row_status -= extra_lines - 2;
                if (s_scroll_row_status < 0)
                {
                    s_scroll_row_status = 0;
                }
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_END:          // scrolling key - go to end of entry
            if (in_scrolling_mode)
            {
                s_scroll_row_status = vertical_scroll_limit;
                s_scroll_column_status = 0;
                rewrite_extrainfo = true;
            }
            break;
        case ID_KEY_CTL_HOME:         // scrolling key - go to beginning of entry
            if (in_scrolling_mode)
            {
                s_scroll_column_status = 0;
                s_scroll_row_status = s_scroll_column_status;
                rewrite_extrainfo = true;
            }
            break;
        }
    }

    return fullscreen_exit();
}

static int prompt_valuestring(char *buf, fullscreenvalues const *val)
{
    // format value into buf, return field width
    int i;
    int ret;
    switch (val->type)
    {
    case 'd':
        ret = 20;
        double_to_string(buf, val->uval.dval);
        break;
    case 'D':
        if (val->uval.dval < 0)
        {
            // We have to round the right way
            std::sprintf(buf, "%ld", (long)(val->uval.dval-.5));
        }
        else
        {
            std::sprintf(buf, "%ld", (long)(val->uval.dval+.5));
        }
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

static int prompt_checkkey(int curkey)
{
    switch (curkey)
    {
    case ID_KEY_PAGE_UP:
    case ID_KEY_DOWN_ARROW:
    case ID_KEY_PAGE_DOWN:
    case ID_KEY_UP_ARROW:
        return curkey;
    case ID_KEY_F2:
    case ID_KEY_F3:
    case ID_KEY_F4:
    case ID_KEY_F5:
    case ID_KEY_F6:
    case ID_KEY_F7:
    case ID_KEY_F8:
    case ID_KEY_F9:
    case ID_KEY_F10:
        if (s_prompt_fn_keys & (1 << (curkey+1-ID_KEY_F1)))
        {
            return curkey;
        }
    }
    return 0;
}

static int prompt_checkkey_scroll(int curkey)
{
    switch (curkey)
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
        return curkey;
    case ID_KEY_F2:
    case ID_KEY_F3:
    case ID_KEY_F4:
    case ID_KEY_F5:
    case ID_KEY_F6:
    case ID_KEY_F7:
    case ID_KEY_F8:
    case ID_KEY_F9:
    case ID_KEY_F10:
        if (s_prompt_fn_keys & (1 << (curkey+1-ID_KEY_F1)))
        {
            return curkey;
        }
    }
    return 0;
}

static int input_field_list(
    int attr,             // display attribute
    char *fld,            // display form field value
    int vlen,             // field length
    char const **list,          // list of values
    int llen,             // number of entries in list
    int row,              // display row
    int col,              // display column
    int (*checkkey)(int)  // routine to check non data keys, or nullptr
)
{
    int initval;
    int curval;
    char buf[81];
    int curkey;
    int ret;
    int old_look_at_mouse;
    old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    for (initval = 0; initval < llen; ++initval)
    {
        if (std::strcmp(fld, list[initval]) == 0)
        {
            break;
        }
    }
    if (initval >= llen)
    {
        initval = 0;
    }
    curval = initval;
    ret = -1;
    while (true)
    {
        std::strcpy(buf, list[curval]);
        {
            int i = (int) std::strlen(buf);
            while (i < vlen)
            {
                buf[i++] = ' ';
            }
        }
        buf[vlen] = 0;
        driver_put_string(row, col, attr, buf);
        curkey = driver_key_cursor(row, col); // get a keystroke
        switch (curkey)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = 0;
            goto inpfldl_end;
        case ID_KEY_ESC:
            goto inpfldl_end;
        case ID_KEY_RIGHT_ARROW:
            if (++curval >= llen)
            {
                curval = 0;
            }
            break;
        case ID_KEY_LEFT_ARROW:
            if (--curval < 0)
            {
                curval = llen - 1;
            }
            break;
        case ID_KEY_F5:
            curval = initval;
            break;
        default:
            if (nonalpha(curkey))
            {
                if (checkkey && (ret = (*checkkey)(curkey)) != 0)
                {
                    goto inpfldl_end;
                }
                break;                                // non alphanum char
            }
            int j = curval;
            for (int i = 0; i < llen; ++i)
            {
                if (++j >= llen)
                {
                    j = 0;
                }
                if ((*list[j] & 0xdf) == (curkey & 0xdf))
                {
                    curval = j;
                    break;
                }
            }
        }
    }
inpfldl_end:
    std::strcpy(fld, list[curval]);
    g_look_at_mouse = old_look_at_mouse;
    return ret;
}
