// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/full_screen_choice.h"

#include "io/ends_with_slash.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "engine/cmdfiles.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/put_string_center.h"
#include "ui/text_screen.h"

#include <config/string_case_compare.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>

const std::string g_speed_prompt{"Speed key string"};

/* For file list purposes only, it's a directory name if first
   char is a dot or last char is a slash */
static int is_a_dir_name(const char *name)
{
    return *name == '.' || ends_with_slash(name) ? 1 : 0;
}

static void footer_msg(int *i, ChoiceFlags flags, const char *speed_string)
{
    put_string_center((*i)++, 0, 80, C_PROMPT_BKGRD,
        speed_string ? "Use the cursor keys or type a value to make a selection"
                    : "Use the cursor keys to highlight your selection");
    put_string_center(*(i++), 0, 80, C_PROMPT_BKGRD,
        bit_set(flags, ChoiceFlags::MENU)
            ? "Press ENTER for highlighted choice, or F1 for help"
            : (bit_set(flags, ChoiceFlags::HELP)
                      ? "Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help"
                      : "Press ENTER for highlighted choice, or ESCAPE to back out"));
}

static void show_speed_string(
    int speed_row, const char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, const char *speed_string, int speed_match))
{
    char buf[81];
    std::memset(buf, ' ', 80);
    buf[80] = 0;
    driver_put_string(speed_row, 0, C_PROMPT_BKGRD, buf);
    if (*speed_string)
    {
        // got a speedstring on the go
        driver_put_string(speed_row, 15, C_CHOICE_SP_INSTR, " ");
        int j;
        if (speed_prompt)
        {
            int speed_match = 0;
            j = speed_prompt(speed_row, 16, C_CHOICE_SP_INSTR, speed_string, speed_match);
        }
        else
        {
            driver_put_string(speed_row, 16, C_CHOICE_SP_INSTR, g_speed_prompt);
            j = static_cast<int>(g_speed_prompt.length());
        }
        std::strcpy(buf, speed_string);
        int i = (int) std::strlen(buf);
        while (i < 30)
        {
            buf[i++] = ' ';
        }
        buf[i] = 0;
        driver_put_string(speed_row, 16+j, C_CHOICE_SP_INSTR, " ");
        driver_put_string(speed_row, 17+j, C_CHOICE_SP_KEYIN, buf);
        driver_move_cursor(speed_row, 17+j+(int) std::strlen(speed_string));
    }
    else
    {
        driver_hide_text_cursor();
    }
}

static void process_speed_string(char *speed_string, //
    const char **choices,                            // array of choice strings
    int key,                                         //
    int *current,                                    //
    int num_choices,                                 //
    bool is_unsorted)
{
    int i = (int) std::strlen(speed_string);
    if (key == 8 && i > 0)   // backspace
    {
        speed_string[--i] = 0;
    }
    if (33 <= key && key <= 126 && i < 30)
    {
        key = std::tolower(key);
        speed_string[i] = (char)key;
        speed_string[++i] = 0;
    }
    if (i > 0)
    {
        // locate matching type
        *current = 0;
        int comp_result;
        while (*current < num_choices
            && (comp_result = string_case_compare(speed_string, choices[*current], i)) != 0)
        {
            if (comp_result < 0 && !is_unsorted)
            {
                *current -= *current ? 1 : 0;
                break;
            }
            ++*current;
        }
        if (*current >= num_choices)   // bumped end of list
        {
            *current = num_choices - 1;
            /*if the list is unsorted, and the entry found is not the exact
              entry, then go looking for the exact entry.
            */
        }
        else if (is_unsorted && choices[*current][i])
        {
            int temp = *current;
            while (++temp < num_choices)
            {
                if (!choices[temp][i] && string_case_equal(speed_string, choices[temp], i))
                {
                    *current = temp;
                    break;
                }
            }
        }
    }
}

/*
    flags:          MENU use menu coloring scheme
                    HELP include F1 for help in instructions
                    INSTRUCTIONS add caller's instr after normal set
                    CRUNCH menu items up one line
    hdg:            heading info, \n delimited
    hdg2:           column heading or nullptr
    instr:          instructions, \n delimited, or nullptr
    num_choices:    How many choices in list
    choices:        array of choice strings
    attributes:     &3: 0 normal color, 1,3 highlight
                    &256 marks a dummy entry
    box_width:      box width, 0 for calc (in items)
    box_depth:      box depth, 0 for calc, 99 for max
    col_width:      data width of a column, 0 for calc
    current:        start with this item
    format_item:    routine to display an item or nullptr
    speed_string:   returned speed key value, or nullptr
    speed_prompt:   routine to display prompt or nullptr
    check_key:      routine to check keystroke or nullptr

    return is: n >= 0 for choice n selected,
              -1 for escape
              k for check_key routine return value k (if not 0 nor -1)
              speed_string[0] != 0 on return if string is present
*/
int full_screen_choice(ChoiceFlags flags, const char *hdg, const char *hdg2, const char *instr,
    int num_choices, const char **choices, int *attributes, int box_width, int box_depth, int col_width,
    int current, void (*format_item)(int choice, char *buf), char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, const char *speed_string, int speed_match),
    int (*check_key)(int key, int choice))
{
    const int scrunch = bit_set(flags, ChoiceFlags::CRUNCH) ? 1 : 0; // scrunch up a line
    ValueSaver saved_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    int ret = -1;
    // preset current to passed string
    const int speed_len = (speed_string == nullptr) ? 0 : (int) std::strlen(speed_string);
    if (speed_len > 0)
    {
        current = 0;
        if (bit_set(flags, ChoiceFlags::NOT_SORTED))
        {
            int k;
            while (current < num_choices && (k = string_case_compare(speed_string, choices[current], speed_len)) != 0)
            {
                ++current;
            }
            if (k != 0)
            {
                current = 0;
            }
        }
        else
        {
            int k;
            while (current < num_choices && (k = string_case_compare(speed_string, choices[current], speed_len)) > 0)
            {
                ++current;
            }
            if (k < 0 && current > 0)  // oops - overshot
            {
                --current;
            }
        }
        if (current >= num_choices) // bumped end of list
        {
            current = num_choices - 1;
        }
    }

    while (true)
    {
        if (current >= num_choices)  // no real choice in the list?
        {
            return ret;
        }
        if ((attributes[current] & 256) == 0)
        {
            break;
        }
        ++current;                  // scan for a real choice
    }

    int title_width = 0;
    int title_lines = 0;
    if (hdg)
    {
        const char *char_ptr = hdg;              // count title lines, find widest
        int i = 0;
        title_lines = 1;
        while (*char_ptr)
        {
            if (*(char_ptr++) == '\n')
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

    if (col_width == 0)             // find the widest column
    {
        for (int i = 0; i < num_choices; ++i)
        {
            const int len = (int) std::strlen(choices[i]);
            col_width = std::max(len, col_width);
        }
    }
    // title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?)
    int reqd_rows = 3 - scrunch;                // calc rows available
    if (hdg)
    {
        reqd_rows += title_lines + 1;
    }
    if (instr)                   // count instructions lines
    {
        const char *char_ptr = instr;
        ++reqd_rows;
        while (*char_ptr)
        {
            if (*(char_ptr++) == '\n')
            {
                ++reqd_rows;
            }
        }
        if (bit_set(flags , ChoiceFlags::INSTRUCTIONS))          // show std instr too
        {
            reqd_rows += 2;
        }
    }
    else
    {
        reqd_rows += 2;              // standard instructions
    }
    if (speed_string)
    {
        ++reqd_rows;   // a row for speedkey prompt
    }
    {
        const int max_depth = 25 - reqd_rows;
        box_depth = std::min(box_depth, max_depth); // limit the depth to max
        if (box_width == 0)           // pick box width and depth
        {
            if (num_choices <= max_depth - 2)  // single column is 1st choice if we can
            {
                box_depth = num_choices;
                box_width = 1;
            }
            else
            {
                // sort-of-wide is 2nd choice
                box_width = 60 / (col_width + 1);
                if (box_width == 0
                    || (box_depth = (num_choices + box_width - 1)/box_width) > max_depth - 2)
                {
                    box_width = 80 / (col_width + 1); // last gasp, full width
                    box_depth = (num_choices + box_width - 1)/box_width;
                    box_depth = std::min(box_depth, max_depth);
                }
            }
        }
    }
    int top_left_row;
    int top_left_col;
    {
        int i = (80 / box_width - col_width) / 2 - 1;
        if (i == 0) // to allow wider prompts
        {
            i = 1;
        }
        i = std::max(i, 0);
        i = std::min(i, 3);
        col_width += i;
        int j = box_width*col_width + i;     // overall width of box
        j = std::max(j, title_width + 2);
        j = std::min(j, 80);
        if (j <= 70 && box_width == 2)         // special case makes menus nicer
        {
            ++j;
            ++col_width;
        }
        int k = (80 - j) / 2;                       // center the box
        k -= (90 - j) / 20;
        top_left_col = k + i;                     // column of topleft choice
        i = (25 - reqd_rows - box_depth) / 2;
        i -= i / 4;                             // higher is better if lots extra
        top_left_row = 3 + title_lines + i;        // row of topleft choice

        // now set up the overall display
        assert(g_text_col_base == 0);
        assert(g_text_row_base == 0);
        help_title();                                   // clear, display title line
        driver_set_attr(1, 0, C_PROMPT_BKGRD, 24 * 80); // init rest to background
        for (i = top_left_row - 1 - title_lines; i < top_left_row + box_depth + 1; ++i)
        {
            driver_set_attr(i, k, C_PROMPT_LO, j);          // draw empty box
        }
    }
    if (hdg)
    {
        g_text_col_base = (80 - title_width) / 2;   // set left margin for putstring
        g_text_col_base -= (90 - title_width) / 20; // put heading into box
        driver_put_string(top_left_row - title_lines - 1, 0, C_PROMPT_HI, hdg);
        g_text_col_base = 0;
    }
    if (hdg2)                               // display 2nd heading
    {
        driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_MED, hdg2);
    }
    int speed_row = 0;  // speed key prompt
    {
        int i = top_left_row + box_depth + 1;
        if (instr == nullptr || bit_set(flags, ChoiceFlags::INSTRUCTIONS)) // display default instructions
        {
            if (i < 20)
            {
                ++i;
            }
            if (speed_string)
            {
                speed_row = i;
                *speed_string = 0;
                if (++i < 22)
                {
                    ++i;
                }
            }
            i -= scrunch;
            footer_msg(&i, flags, speed_string);
        }
        if (instr)                            // display caller's instructions
        {
            const char *char_ptr = instr;
            int j = -1;
            char buf[81];
            while ((buf[++j] = *(char_ptr++)) != 0)
            {
                if (buf[j] == '\n')
                {
                    buf[j] = 0;
                    put_string_center(i++, 0, 80, C_PROMPT_BKGRD, buf);
                    j = -1;
                }
            }
            put_string_center(i, 0, 80, C_PROMPT_BKGRD, buf);
        }
    }

    const int box_items = box_width * box_depth;
    int top_left_choice = 0;                      // pick topleft for init display
    while (current - top_left_choice >= box_items
        || (current - top_left_choice > box_items/2
            && top_left_choice + box_items < num_choices))
    {
        top_left_choice += box_width;
    }
    bool redisplay = true;
    top_left_row -= scrunch;
    while (true) // main loop
    {
        if (redisplay)                       // display the current choices
        {
            char buf[81];
            std::memset(buf, ' ', 80);
            buf[box_width*col_width] = 0;
            for (int i = (hdg2) ? 0 : -1; i <= box_depth; ++i)  // blank the box
            {
                driver_put_string(top_left_row + i, top_left_col, C_PROMPT_LO, buf);
            }
            for (int i = 0; i + top_left_choice < num_choices && i < box_items; ++i)
            {
                // display the choices
                int j = i + top_left_choice;
                int k = attributes[j] & 3;
                if (k == 1)
                {
                    k = C_PROMPT_LO;
                }
                else if (k == 3)
                {
                    k = C_PROMPT_HI;
                }
                else
                {
                    k = C_PROMPT_MED;
                }
                const char *char_ptr;
                if (format_item)
                {
                    (*format_item)(j, buf);
                    char_ptr = buf;
                }
                else
                {
                    char_ptr = choices[j];
                }
                driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*col_width,
                                  k, char_ptr);
            }
            /***
            ... format differs for summary/detail, whups, force box width to
            ...  be 72 when detail toggle available?  (2 grey margin each
            ...  side, 1 blue margin each side)
            ***/
            if (top_left_choice > 0 && hdg2 == nullptr)
            {
                driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_LO, "(more)");
            }
            if (top_left_choice + box_items < num_choices)
            {
                driver_put_string(top_left_row + box_depth, top_left_col, C_PROMPT_LO, "(more)");
            }
            redisplay = false;
        }

        const char *item_ptr;
        // ReSharper disable once CppTooWideScope
        char cur_item[81];
        {
            int i = current - top_left_choice;           // highlight the current choice
            if (format_item)
            {
                (*format_item)(current, cur_item);
                item_ptr = cur_item;
            }
            else
            {
                item_ptr = choices[current];
            }
            driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*col_width,
                              C_CHOICE_CURRENT, item_ptr);
        }

        if (speed_string)                     // show speedstring if any
        {
            show_speed_string(speed_row, speed_string, speed_prompt);
        }
        else
        {
            driver_hide_text_cursor();
        }

        driver_wait_key_pressed(false); // enables help
        int key = driver_get_key();
        {
            int i = current - top_left_choice;           // unhighlight current choice
            int k = attributes[current] & 3;
            if (k == 1)
            {
                k = C_PROMPT_LO;
            }
            else if (k == 3)
            {
                k = C_PROMPT_HI;
            }
            else
            {
                k = C_PROMPT_MED;
            }
            driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*col_width,
                              k, item_ptr);
        }

        int increment = 0;
        int rev_increment = 0;
        // deal with input key
        switch (key)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = current;
            return ret;
        case ID_KEY_ESC:
            return ret;
        case ID_KEY_DOWN_ARROW:
            increment = box_width;
            rev_increment = 0 - increment;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
            increment = box_width;
            rev_increment = 0 - increment;
            {
                int new_current = current;
                while ((new_current += box_width) != current)
                {
                    if (new_current >= num_choices)
                    {
                        new_current = (new_current % box_width) - box_width;
                    }
                    else if (!is_a_dir_name(choices[new_current]))
                    {
                        if (current != new_current)
                        {
                            current = new_current - box_width;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case ID_KEY_UP_ARROW:
            rev_increment = box_width;
            increment = 0 - rev_increment;
            break;
        case ID_KEY_CTL_UP_ARROW:
            rev_increment = box_width;
            increment = 0 - rev_increment;
            {
                int new_current = current;
                while ((new_current -= box_width) != current)
                {
                    if (new_current < 0)
                    {
                        new_current = (num_choices - current) % box_width;
                        new_current =  num_choices + (new_current ? box_width - new_current: 0);
                    }
                    else if (!is_a_dir_name(choices[new_current]))
                    {
                        if (current != new_current)
                        {
                            current = new_current + box_width;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case ID_KEY_RIGHT_ARROW:
            increment = 1;
            rev_increment = -1;
            break;
        case ID_KEY_CTL_RIGHT_ARROW:  /* move to next file; if at last file, go to
                                 first file */
            increment = 1;
            rev_increment = -1;
            {
                int new_current = current;
                while (++new_current != current)
                {
                    if (new_current >= num_choices)
                    {
                        new_current = -1;
                    }
                    else if (!is_a_dir_name(choices[new_current]))
                    {
                        if (current != new_current)
                        {
                            current = new_current - 1;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case ID_KEY_LEFT_ARROW:
            increment = -1;
            rev_increment = 1;
            break;
        case ID_KEY_CTL_LEFT_ARROW: /* move to previous file; if at first file, go to
                               last file */
            increment = -1;
            rev_increment = 1;
            {
                int new_current = current;
                while (--new_current != current)
                {
                    if (new_current < 0)
                    {
                        new_current = num_choices;
                    }
                    else if (!is_a_dir_name(choices[new_current]))
                    {
                        if (current != new_current)
                        {
                            current = new_current + 1;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case ID_KEY_PAGE_UP:
            if (num_choices > box_items)
            {
                top_left_choice -= box_items;
                increment = -box_items;
                rev_increment = box_width;
                redisplay = true;
            }
            break;
        case ID_KEY_PAGE_DOWN:
            if (num_choices > box_items)
            {
                top_left_choice += box_items;
                increment = box_items;
                rev_increment = -box_width;
                redisplay = true;
            }
            break;
        case ID_KEY_HOME:
            current = -1;
            rev_increment = 1;
            increment = 1;
            break;
        case ID_KEY_CTL_HOME:
            current = -1;
            rev_increment = 1;
            increment = 1;
            for (int new_current = 0; new_current < num_choices; ++new_current)
            {
                if (!is_a_dir_name(choices[new_current]))
                {
                    current = new_current - 1;
                    break;  // breaks the for loop
                }
            }
            break;
        case ID_KEY_END:
            current = num_choices;
            rev_increment = -1;
            increment = rev_increment;
            break;
        case ID_KEY_CTL_END:
            current = num_choices;
            rev_increment = -1;
            increment = rev_increment;
            for (int new_current = num_choices - 1; new_current >= 0; --new_current)
            {
                if (!is_a_dir_name(choices[new_current]))
                {
                    current = new_current + 1;
                    break;  // breaks the for loop
                }
            }
            break;
        default:
            if (check_key)
            {
                ret = (*check_key)(key, current);
                if (ret < -1 || ret > 0)
                {
                    return ret;
                }
                if (ret == -1)
                {
                    redisplay = true;
                }
            }
            ret = -1;
            if (speed_string)
            {
                process_speed_string(speed_string, choices, key, &current, num_choices,
                    bit_set(flags, ChoiceFlags::NOT_SORTED));
            }
            break;
        }
        if (increment)                  // apply cursor movement
        {
            current += increment;
            if (speed_string)               // zap speedstring
            {
                speed_string[0] = 0;
            }
        }
        while (true)
        {
            // adjust to a non-comment choice
            if (current < 0 || current >= num_choices)
            {
                increment = rev_increment;
            }
            else if ((attributes[current] & 256) == 0)
            {
                break;
            }
            current += increment;
        }
        if (top_left_choice > num_choices - box_items)
        {
            top_left_choice = ((num_choices + box_width - 1)/box_width)*box_width - box_items;
        }
        top_left_choice = std::max(top_left_choice, 0);
        while (current < top_left_choice)
        {
            top_left_choice -= box_width;
            redisplay = true;
        }
        while (current >= top_left_choice + box_items)
        {
            top_left_choice += box_width;
            redisplay = true;
        }
    }
}
