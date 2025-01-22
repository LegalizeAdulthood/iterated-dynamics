// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/field_prompt.h"

#include "misc/Driver.h"
#include "ui/cmdfiles.h"
#include "ui/help_title.h"
#include "ui/input_field.h"
#include "ui/put_string_center.h"
#include "ui/text_screen.h"

#include <algorithm>

int field_prompt(char const *hdg, // heading, \n delimited lines
    char const *instr,            // additional instructions or nullptr
    char *fld,                    // the field itself
    int len,                      // field length (declare as 1 larger for \0)
    int (*check_key)(int key)     // routine to check non data keys, or nullptr
)
{
    char buf[81]{};
    help_title();                                   // clear screen, display title
    driver_set_attr(1, 0, C_PROMPT_BKGRD, 24 * 80); // init rest to background
    char const *char_ptr = hdg;                                  // count title lines, find widest
    int box_width = 0;
    int i = box_width;
    int title_lines = 1;
    while (*char_ptr)
    {
        if (*(char_ptr++) == '\n')
        {
            ++title_lines;
            i = -1;
        }
        if (++i > box_width)
        {
            box_width = i;
        }
    }
    box_width = std::max(len, box_width);
    i = title_lines + 4;                    // total rows in box
    int title_row = (25 - i) / 2;               // top row of it all when centered
    title_row -= title_row / 4;              // higher is better if lots extra
    int title_col = (80 - box_width) / 2;        // center the box
    title_col -= (90 - box_width) / 20;
    int prompt_col = title_col - (box_width - len) / 2;
    int j = title_col;                          // add margin at each side of box
    i = (82-box_width)/4;
    i = std::min(i, 3);
    j -= i;
    box_width += i * 2;
    for (int k = -1; k < title_lines + 3; ++k) // draw empty box
    {
        driver_set_attr(title_row + k, j, C_PROMPT_LO, box_width);
    }
    g_text_col_base = title_col;                  // set left margin for putstring
    driver_put_string(title_row, 0, C_PROMPT_HI, hdg); // display heading
    g_text_col_base = 0;
    i = title_row + title_lines + 4;
    if (instr)
    {
        // display caller's instructions
        char_ptr = instr;
        j = -1;
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
    else                                     // default instructions
    {
        put_string_center(i, 0, 80, C_PROMPT_BKGRD, "Press ENTER when finished (or ESCAPE to back out)");
    }
    return input_field(InputFieldFlags::NONE, C_PROMPT_INPUT, fld, len, title_row + title_lines + 1, prompt_col, check_key);
}
