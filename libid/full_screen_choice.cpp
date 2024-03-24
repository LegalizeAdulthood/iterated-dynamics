#include "full_screen_choice.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "id.h"
#include "id_data.h"
#include "os.h"
#include "realdos.h"
#include "string_case_compare.h"

#include <cstring>

/* For file list purposes only, it's a directory name if first
   char is a dot or last char is a slash */
inline int isadirname(char const *name)
{
    return *name == '.' || endswithslash(name) ? 1 : 0;
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
    int numchoices,
    char const **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*checkkey)(int, int)
)
{
    int titlelines, titlewidth;
    int reqdrows;
    int topleftrow, topleftcol;
    int topleftchoice;
    int speedrow = 0;  // speed key prompt
    int boxitems;      // boxwidth*boxdepth
    int curkey, increment, rev_increment = 0;
    bool redisplay;
    char const *charptr;
    char buf[81];
    char curitem[81];
    char const *itemptr;
    int ret, old_look_at_mouse;
    int scrunch;  // scrunch up a line

    scrunch = (options & CHOICE_CRUNCH) ? 1 : 0;
    old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    ret = -1;
    // preset current to passed string
    int const speed_len = (speedstring == nullptr) ? 0 : (int) std::strlen(speedstring);
    if (speed_len > 0)
    {
        current = 0;
        if (options & CHOICE_NOT_SORTED)
        {
            int k;
            while (current < numchoices && (k = strncasecmp(speedstring, choices[current], speed_len)) != 0)
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
            while (current < numchoices && (k = strncasecmp(speedstring, choices[current], speed_len)) > 0)
            {
                ++current;
            }
            if (k < 0 && current > 0)  // oops - overshot
            {
                --current;
            }
        }
        if (current >= numchoices) // bumped end of list
        {
            current = numchoices - 1;
        }
    }

    while (true)
    {
        if (current >= numchoices)  // no real choice in the list?
        {
            goto fs_choice_end;
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

    if (colwidth == 0)             // find widest column
    {
        for (int i = 0; i < numchoices; ++i)
        {
            const int len = (int) std::strlen(choices[i]);
            if (len > colwidth)
            {
                colwidth = len;
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
    if (speedstring)
    {
        ++reqdrows;   // a row for speedkey prompt
    }
    {
        int const max_depth = 25 - reqdrows;
        if (boxdepth > max_depth) // limit the depth to max
        {
            boxdepth = max_depth;
        }
        if (boxwidth == 0)           // pick box width and depth
        {
            if (numchoices <= max_depth - 2)  // single column is 1st choice if we can
            {
                boxdepth = numchoices;
                boxwidth = 1;
            }
            else
            {
                // sort-of-wide is 2nd choice
                boxwidth = 60 / (colwidth + 1);
                if (boxwidth == 0
                    || (boxdepth = (numchoices + boxwidth - 1)/boxwidth) > max_depth - 2)
                {
                    boxwidth = 80 / (colwidth + 1); // last gasp, full width
                    boxdepth = (numchoices + boxwidth - 1)/boxwidth;
                    if (boxdepth > max_depth)
                    {
                        boxdepth = max_depth;
                    }
                }
            }
        }
    }
    {
        int i = (80 / boxwidth - colwidth) / 2 - 1;
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
        colwidth += i;
        int j = boxwidth*colwidth + i;     // overall width of box
        if (j < titlewidth+2)
        {
            j = titlewidth + 2;
        }
        if (j > 80)
        {
            j = 80;
        }
        if (j <= 70 && boxwidth == 2)         // special case makes menus nicer
        {
            ++j;
            ++colwidth;
        }
        int k = (80 - j) / 2;                       // center the box
        k -= (90 - j) / 20;
        topleftcol = k + i;                     // column of topleft choice
        i = (25 - reqdrows - boxdepth) / 2;
        i -= i / 4;                             // higher is better if lots extra
        topleftrow = 3 + titlelines + i;        // row of topleft choice

        // now set up the overall display
        helptitle();                            // clear, display title line
        driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      // init rest to background
        for (i = topleftrow - 1 - titlelines; i < topleftrow + boxdepth + 1; ++i)
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
        int i = topleftrow + boxdepth + 1;
        if (instr == nullptr || (options & CHOICE_INSTRUCTIONS))   // display default instructions
        {
            if (i < 20)
            {
                ++i;
            }
            if (speedstring)
            {
                speedrow = i;
                *speedstring = 0;
                if (++i < 22)
                {
                    ++i;
                }
            }
            i -= scrunch;
            footer_msg(&i, options, speedstring);
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

    boxitems = boxwidth * boxdepth;
    topleftchoice = 0;                      // pick topleft for init display
    while (current - topleftchoice >= boxitems
        || (current - topleftchoice > boxitems/2
            && topleftchoice + boxitems < numchoices))
    {
        topleftchoice += boxwidth;
    }
    redisplay = true;
    topleftrow -= scrunch;
    while (true) // main loop
    {
        if (redisplay)                       // display the current choices
        {
            std::memset(buf, ' ', 80);
            buf[boxwidth*colwidth] = 0;
            for (int i = (hdg2) ? 0 : -1; i <= boxdepth; ++i)  // blank the box
            {
                driver_put_string(topleftrow + i, topleftcol, C_PROMPT_LO, buf);
            }
            for (int i = 0; i + topleftchoice < numchoices && i < boxitems; ++i)
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
                if (formatitem)
                {
                    (*formatitem)(j, buf);
                    charptr = buf;
                }
                else
                {
                    charptr = choices[j];
                }
                driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
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
            if (topleftchoice + boxitems < numchoices)
            {
                driver_put_string(topleftrow + boxdepth, topleftcol, C_PROMPT_LO, "(more)");
            }
            redisplay = false;
        }

        {
            int i = current - topleftchoice;           // highlight the current choice
            if (formatitem)
            {
                (*formatitem)(current, curitem);
                itemptr = curitem;
            }
            else
            {
                itemptr = choices[current];
            }
            driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
                              C_CHOICE_CURRENT, itemptr);
        }

        if (speedstring)                     // show speedstring if any
        {
            show_speedstring(speedrow, speedstring, speedprompt);
        }
        else
        {
            driver_hide_text_cursor();
        }

        driver_wait_key_pressed(0); // enables help
        curkey = driver_get_key();
#ifdef XFRACT
        if (curkey == FIK_F10)
        {
            curkey = ')';
        }
        if (curkey == FIK_F9)
        {
            curkey = '(';
        }
        if (curkey == FIK_F8)
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
            driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
                              k, itemptr);
        }

        increment = 0;
        // deal with input key
        switch (curkey)
        {
        case FIK_ENTER:
        case FIK_ENTER_2:
            ret = current;
            goto fs_choice_end;
        case FIK_ESC:
            goto fs_choice_end;
        case FIK_DOWN_ARROW:
            increment = boxwidth;
            rev_increment = 0 - increment;
            break;
        case FIK_CTL_DOWN_ARROW:
            increment = boxwidth;
            rev_increment = 0 - increment;
            {
                int newcurrent = current;
                while ((newcurrent += boxwidth) != current)
                {
                    if (newcurrent >= numchoices)
                    {
                        newcurrent = (newcurrent % boxwidth) - boxwidth;
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent - boxwidth;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case FIK_UP_ARROW:
            rev_increment = boxwidth;
            increment = 0 - rev_increment;
            break;
        case FIK_CTL_UP_ARROW:
            rev_increment = boxwidth;
            increment = 0 - rev_increment;
            {
                int newcurrent = current;
                while ((newcurrent -= boxwidth) != current)
                {
                    if (newcurrent < 0)
                    {
                        newcurrent = (numchoices - current) % boxwidth;
                        newcurrent =  numchoices + (newcurrent ? boxwidth - newcurrent: 0);
                    }
                    else if (!isadirname(choices[newcurrent]))
                    {
                        if (current != newcurrent)
                        {
                            current = newcurrent + boxwidth;
                        }
                        break;  // breaks the while loop
                    }
                }
            }
            break;
        case FIK_RIGHT_ARROW:
            increment = 1;
            rev_increment = -1;
            break;
        case FIK_CTL_RIGHT_ARROW:  /* move to next file; if at last file, go to
                                 first file */
            increment = 1;
            rev_increment = -1;
            {
                int newcurrent = current;
                while (++newcurrent != current)
                {
                    if (newcurrent >= numchoices)
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
        case FIK_LEFT_ARROW:
            increment = -1;
            rev_increment = 1;
            break;
        case FIK_CTL_LEFT_ARROW: /* move to previous file; if at first file, go to
                               last file */
            increment = -1;
            rev_increment = 1;
            {
                int newcurrent = current;
                while (--newcurrent != current)
                {
                    if (newcurrent < 0)
                    {
                        newcurrent = numchoices;
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
        case FIK_PAGE_UP:
            if (numchoices > boxitems)
            {
                topleftchoice -= boxitems;
                increment = -boxitems;
                rev_increment = boxwidth;
                redisplay = true;
            }
            break;
        case FIK_PAGE_DOWN:
            if (numchoices > boxitems)
            {
                topleftchoice += boxitems;
                increment = boxitems;
                rev_increment = -boxwidth;
                redisplay = true;
            }
            break;
        case FIK_HOME:
            current = -1;
            rev_increment = 1;
            increment = rev_increment;
            break;
        case FIK_CTL_HOME:
            current = -1;
            rev_increment = 1;
            increment = rev_increment;
            for (int newcurrent = 0; newcurrent < numchoices; ++newcurrent)
            {
                if (!isadirname(choices[newcurrent]))
                {
                    current = newcurrent - 1;
                    break;  // breaks the for loop
                }
            }
            break;
        case FIK_END:
            current = numchoices;
            rev_increment = -1;
            increment = rev_increment;
            break;
        case FIK_CTL_END:
            current = numchoices;
            rev_increment = -1;
            increment = rev_increment;
            for (int newcurrent = numchoices - 1; newcurrent >= 0; --newcurrent)
            {
                if (!isadirname(choices[newcurrent]))
                {
                    current = newcurrent + 1;
                    break;  // breaks the for loop
                }
            }
            break;
        default:
            if (checkkey)
            {
                ret = (*checkkey)(curkey, current);
                if (ret < -1 || ret > 0)
                {
                    goto fs_choice_end;
                }
                if (ret == -1)
                {
                    redisplay = true;
                }
            }
            ret = -1;
            if (speedstring)
            {
                process_speedstring(speedstring, choices, curkey, &current,
                                    numchoices, options & CHOICE_NOT_SORTED);
            }
            break;
        }
        if (increment)                  // apply cursor movement
        {
            current += increment;
            if (speedstring)               // zap speedstring
            {
                speedstring[0] = 0;
            }
        }
        while (true)
        {
            // adjust to a non-comment choice
            if (current < 0 || current >= numchoices)
            {
                increment = rev_increment;
            }
            else if ((attributes[current] & 256) == 0)
            {
                break;
            }
            current += increment;
        }
        if (topleftchoice > numchoices - boxitems)
        {
            topleftchoice = ((numchoices + boxwidth - 1)/boxwidth)*boxwidth - boxitems;
        }
        if (topleftchoice < 0)
        {
            topleftchoice = 0;
        }
        while (current < topleftchoice)
        {
            topleftchoice -= boxwidth;
            redisplay = true;
        }
        while (current >= topleftchoice + boxitems)
        {
            topleftchoice += boxwidth;
            redisplay = true;
        }
    }

fs_choice_end:
    g_look_at_mouse = old_look_at_mouse;
    return ret;
}
