#include "full_screen_choice.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "ends_with_slash.h"
#include "help_title.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "os.h"
#include "put_string_center.h"
#include "string_case_compare.h"
#include "text_screen.h"
#include "value_saver.h"

#include <cctype>
#include <cstring>
#include <string>

std::string const g_speed_prompt{"Speed key string"};

/* For file list purposes only, it's a directory name if first
   char is a dot or last char is a slash */
inline int isadirname(char const *name)
{
    return *name == '.' || endswithslash(name) ? 1 : 0;
}

static void footer_msg(int *i, int options, char const *speedstring)
{
    putstringcenter((*i)++, 0, 80, C_PROMPT_BKGRD,
                    (speedstring) ? "Use the cursor keys or type a value to make a selection"
                    : "Use the cursor keys to highlight your selection");
    putstringcenter(*(i++), 0, 80, C_PROMPT_BKGRD,
                    (options & CHOICE_MENU) ? "Press ENTER for highlighted choice, or F1 for help"
                    : ((options & CHOICE_HELP) ? "Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help"
                       : "Press ENTER for highlighted choice, or ESCAPE to back out"));
}

static void show_speedstring(
    int speedrow,
    char const *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match))
{
    char buf[81];
    std::memset(buf, ' ', 80);
    buf[80] = 0;
    driver_put_string(speedrow, 0, C_PROMPT_BKGRD, buf);
    if (*speedstring)
    {
        // got a speedstring on the go
        driver_put_string(speedrow, 15, C_CHOICE_SP_INSTR, " ");
        int j;
        if (speedprompt)
        {
            int speed_match = 0;
            j = speedprompt(speedrow, 16, C_CHOICE_SP_INSTR, speedstring, speed_match);
        }
        else
        {
            driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, g_speed_prompt);
            j = static_cast<int>(g_speed_prompt.length());
        }
        std::strcpy(buf, speedstring);
        int i = (int) std::strlen(buf);
        while (i < 30)
        {
            buf[i++] = ' ';

        }
        buf[i] = 0;
        driver_put_string(speedrow, 16+j, C_CHOICE_SP_INSTR, " ");
        driver_put_string(speedrow, 17+j, C_CHOICE_SP_KEYIN, buf);
        driver_move_cursor(speedrow, 17+j+(int) std::strlen(speedstring));
    }
    else
    {
        driver_hide_text_cursor();
    }
}

static void process_speedstring(char *speedstring, //
    char const **choices,                          // array of choice strings
    int curkey,                                    //
    int *pcurrent,                                 //
    int numchoices,                                //
    int is_unsorted)
{
    int i = (int) std::strlen(speedstring);
    if (curkey == 8 && i > 0)   // backspace
    {
        speedstring[--i] = 0;
    }
    if (33 <= curkey && curkey <= 126 && i < 30)
    {
        curkey = std::tolower(curkey);
        speedstring[i] = (char)curkey;
        speedstring[++i] = 0;
    }
    if (i > 0)
    {
        // locate matching type
        *pcurrent = 0;
        int comp_result;
        while (*pcurrent < numchoices
            && (comp_result = strncasecmp(speedstring, choices[*pcurrent], i)) != 0)
        {
            if (comp_result < 0 && !is_unsorted)
            {
                *pcurrent -= *pcurrent ? 1 : 0;
                break;
            }
            else
            {
                ++*pcurrent;
            }
        }
        if (*pcurrent >= numchoices)   // bumped end of list
        {
            *pcurrent = numchoices - 1;
            /*if the list is unsorted, and the entry found is not the exact
              entry, then go looking for the exact entry.
            */
        }
        else if (is_unsorted && choices[*pcurrent][i])
        {
            int temp = *pcurrent;
            while (++temp < numchoices)
            {
                if (!choices[temp][i] && !strncasecmp(speedstring, choices[temp], i))
                {
                    *pcurrent = temp;
                    break;
                }
            }
        }
    }
}

/*
    options:        &2 use menu coloring scheme
                    &4 include F1 for help in instructions
                    &8 add caller's instr after normal set
                    &16 menu items up one line
    hdg:            heading info, \n delimited
    hdg2:           column heading or nullptr
    instr:          instructions, \n delimited, or nullptr
    numchoices:     How many choices in list
    choices:        array of choice strings
    attributes:     &3: 0 normal color, 1,3 highlight
                    &256 marks a dummy entry
    boxwidth:       box width, 0 for calc (in items)
    boxdepth:       box depth, 0 for calc, 99 for max
    colwidth:       data width of a column, 0 for calc
    current:        start with this item
    formatitem:     routine to display an item or nullptr
    speedstring:    returned speed key value, or nullptr
    speedprompt:    routine to display prompt or nullptr
    checkkey:       routine to check keystroke or nullptr

    return is: n>=0 for choice n selected,
              -1 for escape
              k for checkkey routine return value k (if not 0 nor -1)
              speedstring[0] != 0 on return if string is present
*/
int fullscreen_choice(
    int options,
    char const *hdg,
    char const *hdg2,
    char const *instr,
    int num_choices,
    char const **choices,
    int *attributes,
    int box_width,
    int box_depth,
    int col_width,
    int current,
    void (*format_item)(int choice, char *buf),
    char *speed_string,
    int (*speed_prompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*check_key)(int curkey, int choice)
    )
{
    int titlelines;
    int titlewidth;
    int reqdrows;
    int topleftrow;
    int topleftcol;
    int topleftchoice;
    int speedrow = 0;  // speed key prompt
    int boxitems;      // boxwidth*boxdepth
    int curkey;
    int increment;
    int rev_increment = 0;
    bool redisplay;
    char const *charptr;
    char buf[81];
    char curitem[81];
    char const *itemptr;
    int ret;
    int scrunch;  // scrunch up a line

    scrunch = (options & CHOICE_CRUNCH) ? 1 : 0;
    ValueSaver saved_look_at_mouse(g_look_at_mouse, 0);
    ret = -1;
    // preset current to passed string
    int const speed_len = (speed_string == nullptr) ? 0 : (int) std::strlen(speed_string);
    if (speed_len > 0)
    {
        current = 0;
        if (options & CHOICE_NOT_SORTED)
        {
            int k;
            while (current < num_choices && (k = strncasecmp(speed_string, choices[current], speed_len)) != 0)
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
            while (current < num_choices && (k = strncasecmp(speed_string, choices[current], speed_len)) > 0)
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

    titlewidth = 0;
    titlelines = titlewidth;
    if (hdg)
    {
        charptr = hdg;              // count title lines, find widest
        int i = 0;
        titlelines = 1;
        while (*charptr)
        {
            if (*(charptr++) == '\n')
            {
                ++titlelines;
                i = -1;
            }
            if (++i > titlewidth)
            {
                titlewidth = i;
            }
        }
    }

    if (col_width == 0)             // find widest column
    {
        for (int i = 0; i < num_choices; ++i)
        {
            const int len = (int) std::strlen(choices[i]);
            if (len > col_width)
            {
                col_width = len;
            }
        }
    }
    // title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?)
    reqdrows = 3 - scrunch;                // calc rows available
    if (hdg)
    {
        reqdrows += titlelines + 1;
    }
    if (instr)                   // count instructions lines
    {
        charptr = instr;
        ++reqdrows;
        while (*charptr)
        {
            if (*(charptr++) == '\n')
            {
                ++reqdrows;
            }
        }
        if ((options & CHOICE_INSTRUCTIONS))          // show std instr too
        {
            reqdrows += 2;
        }
    }
    else
    {
        reqdrows += 2;              // standard instructions
    }
    if (speed_string)
    {
        ++reqdrows;   // a row for speedkey prompt
    }
    {
        int const max_depth = 25 - reqdrows;
        if (box_depth > max_depth) // limit the depth to max
        {
            box_depth = max_depth;
        }
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
                    if (box_depth > max_depth)
                    {
                        box_depth = max_depth;
                    }
                }
            }
        }
    }
    {
        int i = (80 / box_width - col_width) / 2 - 1;
        if (i == 0) // to allow wider prompts
        {
            i = 1;
        }
        if (i < 0)
        {
            i = 0;
        }
        if (i > 3)
        {
            i = 3;
        }
        col_width += i;
        int j = box_width*col_width + i;     // overall width of box
        if (j < titlewidth+2)
        {
            j = titlewidth + 2;
        }
        if (j > 80)
        {
            j = 80;
        }
        if (j <= 70 && box_width == 2)         // special case makes menus nicer
        {
            ++j;
            ++col_width;
        }
        int k = (80 - j) / 2;                       // center the box
        k -= (90 - j) / 20;
        topleftcol = k + i;                     // column of topleft choice
        i = (25 - reqdrows - box_depth) / 2;
        i -= i / 4;                             // higher is better if lots extra
        topleftrow = 3 + titlelines + i;        // row of topleft choice

        // now set up the overall display
        helptitle();                            // clear, display title line
        driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      // init rest to background
        for (i = topleftrow - 1 - titlelines; i < topleftrow + box_depth + 1; ++i)
        {
            driver_set_attr(i, k, C_PROMPT_LO, j);          // draw empty box
        }
    }
    if (hdg)
    {
        g_text_cbase = (80 - titlewidth) / 2;   // set left margin for putstring
        g_text_cbase -= (90 - titlewidth) / 20; // put heading into box
        driver_put_string(topleftrow - titlelines - 1, 0, C_PROMPT_HI, hdg);
        g_text_cbase = 0;
    }
    if (hdg2)                               // display 2nd heading
    {
        driver_put_string(topleftrow - 1, topleftcol, C_PROMPT_MED, hdg2);
    }
    {
        int i = topleftrow + box_depth + 1;
        if (instr == nullptr || (options & CHOICE_INSTRUCTIONS))   // display default instructions
        {
            if (i < 20)
            {
                ++i;
            }
            if (speed_string)
            {
                speedrow = i;
                *speed_string = 0;
                if (++i < 22)
                {
                    ++i;
                }
            }
            i -= scrunch;
            footer_msg(&i, options, speed_string);
        }
        if (instr)                            // display caller's instructions
        {
            charptr = instr;
            int j = -1;
            while ((buf[++j] = *(charptr++)) != 0)
            {
                if (buf[j] == '\n')
                {
                    buf[j] = 0;
                    putstringcenter(i++, 0, 80, C_PROMPT_BKGRD, buf);
                    j = -1;
                }
            }
            putstringcenter(i, 0, 80, C_PROMPT_BKGRD, buf);
        }
    }

    boxitems = box_width * box_depth;
    topleftchoice = 0;                      // pick topleft for init display
    while (current - topleftchoice >= boxitems
        || (current - topleftchoice > boxitems/2
            && topleftchoice + boxitems < num_choices))
    {
        topleftchoice += box_width;
    }
    redisplay = true;
    topleftrow -= scrunch;
    while (true) // main loop
    {
        if (redisplay)                       // display the current choices
        {
            std::memset(buf, ' ', 80);
            buf[box_width*col_width] = 0;
            for (int i = (hdg2) ? 0 : -1; i <= box_depth; ++i)  // blank the box
            {
                driver_put_string(topleftrow + i, topleftcol, C_PROMPT_LO, buf);
            }
            for (int i = 0; i + topleftchoice < num_choices && i < boxitems; ++i)
            {
                // display the choices
                int j = i + topleftchoice;
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
                if (format_item)
                {
                    (*format_item)(j, buf);
                    charptr = buf;
                }
                else
                {
                    charptr = choices[j];
                }
                driver_put_string(topleftrow + i/box_width, topleftcol + (i % box_width)*col_width,
                                  k, charptr);
            }
            /***
            ... format differs for summary/detail, whups, force box width to
            ...  be 72 when detail toggle available?  (2 grey margin each
            ...  side, 1 blue margin each side)
            ***/
            if (topleftchoice > 0 && hdg2 == nullptr)
            {
                driver_put_string(topleftrow - 1, topleftcol, C_PROMPT_LO, "(more)");
            }
            if (topleftchoice + boxitems < num_choices)
            {
                driver_put_string(topleftrow + box_depth, topleftcol, C_PROMPT_LO, "(more)");
            }
            redisplay = false;
        }

        {
            int i = current - topleftchoice;           // highlight the current choice
            if (format_item)
            {
                (*format_item)(current, curitem);
                itemptr = curitem;
            }
            else
            {
                itemptr = choices[current];
            }
            driver_put_string(topleftrow + i/box_width, topleftcol + (i % box_width)*col_width,
                              C_CHOICE_CURRENT, itemptr);
        }

        if (speed_string)                     // show speedstring if any
        {
            show_speedstring(speedrow, speed_string, speed_prompt);
        }
        else
        {
            driver_hide_text_cursor();
        }

        driver_wait_key_pressed(0); // enables help
        curkey = driver_get_key();
#ifdef XFRACT
        if (curkey == ID_KEY_F10)
        {
            curkey = ')';
        }
        if (curkey == ID_KEY_F9)
        {
            curkey = '(';
        }
        if (curkey == ID_KEY_F8)
        {
            curkey = '*';
        }
#endif

        {
            int i = current - topleftchoice;           // unhighlight current choice
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
            driver_put_string(topleftrow + i/box_width, topleftcol + (i % box_width)*col_width,
                              k, itemptr);
        }

        increment = 0;
        // deal with input key
        switch (curkey)
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
                int newcurrent = current;
                while ((newcurrent += box_width) != current)
                {
                    if (newcurrent >= num_choices)
                    {
                        newcurrent = (newcurrent % box_width) - box_width;
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent - box_width;
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
                int newcurrent = current;
                while ((newcurrent -= box_width) != current)
                {
                    if (newcurrent < 0)
                    {
                        newcurrent = (num_choices - current) % box_width;
                        newcurrent =  num_choices + (newcurrent ? box_width - newcurrent: 0);
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent + box_width;
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
                int newcurrent = current;
                while (++newcurrent != current)
                {
                    if (newcurrent >= num_choices)
                    {
                        newcurrent = -1;
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent - 1;
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
                int newcurrent = current;
                while (--newcurrent != current)
                {
                    if (newcurrent < 0)
                    {
                        newcurrent = num_choices;
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent + 1;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case ID_KEY_PAGE_UP:
            if (num_choices > boxitems)
            {
                topleftchoice -= boxitems;
                increment = -boxitems;
                rev_increment = box_width;
                redisplay = true;
            }
            break;
        case ID_KEY_PAGE_DOWN:
            if (num_choices > boxitems)
            {
                topleftchoice += boxitems;
                increment = boxitems;
                rev_increment = -box_width;
                redisplay = true;
            }
            break;
        case ID_KEY_HOME:
            current = -1;
            rev_increment = 1;
            increment = rev_increment;
            break;
        case ID_KEY_CTL_HOME:
            current = -1;
            rev_increment = 1;
            increment = rev_increment;
            for (int newcurrent = 0; newcurrent < num_choices; ++newcurrent)
            {
                if (!isadirname(choices[newcurrent]))
                {
                    current = newcurrent - 1;
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
            for (int newcurrent = num_choices - 1; newcurrent >= 0; --newcurrent)
            {
                if (!isadirname(choices[newcurrent]))
                {
                    current = newcurrent + 1;
                    break;  // breaks the for loop
                }
            }
            break;
        default:
            if (check_key)
            {
                ret = (*check_key)(curkey, current);
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
                process_speedstring(speed_string, choices, curkey, &current,
                                    num_choices, options & CHOICE_NOT_SORTED);
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
        if (topleftchoice > num_choices - boxitems)
        {
            topleftchoice = ((num_choices + box_width - 1)/box_width)*box_width - boxitems;
        }
        if (topleftchoice < 0)
        {
            topleftchoice = 0;
        }
        while (current < topleftchoice)
        {
            topleftchoice -= box_width;
            redisplay = true;
        }
        while (current >= topleftchoice + boxitems)
        {
            topleftchoice += box_width;
            redisplay = true;
        }
    }
}
