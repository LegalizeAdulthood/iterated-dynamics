#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "find_path.h"
#include "fractalp.h"
#include "fractype.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "memory.h"
#include "miscres.h"
#include "os.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "string_case_compare.h"
#include "zoom.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int menu_checkkey(int curkey, int choice);

int g_release = 2099;   // this has 2 implied decimals; increment it every synch
int g_patch_level = 8;  // patchlevel for DOS version

/* int stopmsg(flags,message) displays message and waits for a key:
     message should be a max of 9 lines with \n's separating them;
       no leading or trailing \n's in message;
       no line longer than 76 chars for best appearance;
     flag options:
       &1 if already in text display mode, stackscreen is not called
          and message is displayed at (12,0) instead of (4,0)
       &2 if continue/cancel indication is to be returned;
          when not set, "Any key to continue..." is displayed
          when set, "Escape to cancel, any other key to continue..."
          true is returned for cancel, false for continue
       &4 set to suppress buzzer
       &8 for Fractint for Windows & parser - use a fixed pitch font
      &16 for info only message (green box instead of red in DOS vsn)
   */
bool stopmsg(int flags, char const* msg)
{
    int toprow;
    int color;
    int old_look_at_mouse;
    static bool batchmode = false;
    if (g_debug_flag != debug_flags::none || g_init_batch >= batch_modes::NORMAL)
    {
        if (std::FILE *fp = dir_fopen(g_working_dir.c_str(), "stopmsg.txt", g_init_batch == batch_modes::NONE ? "w" : "a"))
        {
            std::fprintf(fp, "%s\n", msg);
            std::fclose(fp);
        }
    }
    if (g_first_init)
    {
        // & cmdfiles hasn't finished 1st try
#ifdef XFRACT
        driver_set_for_text();
        driver_buzzer(buzzer_codes::PROBLEM);
        driver_put_string(0, 0, 15, "*** Error during startup:");
        driver_put_string(2, 0, 15, msg);
        driver_move_cursor(8, 0);
#if !defined(WIN32)
        sleep(1);
#endif
        close_drivers();
        exit(1);
#else
        std::printf("%s\n", msg);
        dopause(1); // pause deferred until after cmdfiles
        return false;
#endif
    }
    if (g_init_batch >= batch_modes::NORMAL || batchmode)
    {
        // in batch mode
        g_init_batch = batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE; // used to set errorlevel
        batchmode = true; // fixes *second* stopmsg in batch mode bug
        return true;
    }
    old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = -13;
    if ((flags & STOPMSG_NO_STACK))
    {
        blankrows(toprow = 12, 10, 7);
    }
    else
    {
        driver_stack_screen();
        toprow = 4;
        driver_move_cursor(4, 0);
    }
    g_text_cbase = 2; // left margin is 2
    driver_put_string(toprow, 0, 7, msg);
    if (flags & STOPMSG_CANCEL)
    {
        driver_put_string(g_text_row+2, 0, 7, "Escape to cancel, any other key to continue...");
    }
    else
    {
        driver_put_string(g_text_row+2, 0, 7, "Any key to continue...");
    }
    g_text_cbase = 0; // back to full line
    color = (flags & STOPMSG_INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
    driver_set_attr(toprow, 0, color, (g_text_row+1-toprow)*80);
    driver_hide_text_cursor();   // cursor off
    if ((flags & STOPMSG_NO_BUZZER) == 0)
    {
        driver_buzzer((flags & STOPMSG_INFO_ONLY) ? buzzer_codes::COMPLETE : buzzer_codes::PROBLEM);
    }
    while (driver_key_pressed())   // flush any keyahead
    {
        driver_get_key();
    }
    bool ret = false;
    if (g_debug_flag != debug_flags::show_formula_info_after_compile)
    {
        if (getakeynohelp() == FIK_ESC)
        {
            ret = true;
        }
    }
    if ((flags & STOPMSG_NO_STACK))
    {
        blankrows(toprow, 10, 7);
    }
    else
    {
        driver_unstack_screen();
    }
    g_look_at_mouse = old_look_at_mouse;
    return ret;
}


static std::vector<BYTE> temptextsave;
static int textxdots;
static int textydots;

/* texttempmsg(msg) displays a text message of up to 40 characters, waits
      for a key press, restores the prior display, and returns (without
      eating the key).
      It works in almost any video mode - does nothing in some very odd cases
      (HCGA hi-res with old bios), or when there isn't 10k of temp mem free. */
int texttempmsg(char const *msgparm)
{
    if (showtempmsg(msgparm))
    {
        return -1;
    }

    driver_wait_key_pressed(0); // wait for a keystroke but don't eat it
    cleartempmsg();
    return 0;
}

void freetempmsg()
{
    temptextsave.clear();
}

bool showtempmsg(char const *msgparm)
{
    static long size = 0;
    char msg[41];
    int xrepeat = 0;
    int yrepeat = 0;
    int save_sxoffs, save_syoffs;

    std::strncpy(msg, msgparm, 40);
    msg[40] = 0; // ensure max message len of 40 chars
    if (driver_diskp())  // disk video, screen in text mode, easy
    {
        dvid_status(0, msg);
        return false;
    }
    if (g_first_init)      // & cmdfiles hasn't finished 1st try
    {
        std::printf("%s\n", msg);
        return false;
    }

    xrepeat = (g_screen_x_dots >= 640) ? 2 : 1;
    yrepeat = (g_screen_y_dots >= 300) ? 2 : 1;
    textxdots = (int) std::strlen(msg) * xrepeat * 8;
    textydots = yrepeat * 8;

    size = (long) textxdots * (long) textydots;
    if (temptextsave.size() != size)
    {
        temptextsave.clear();
    }
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    if (g_video_scroll)
    {
        g_logical_screen_x_offset = g_video_start_x;
        g_logical_screen_y_offset = g_video_start_y;
    }
    else
    {
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_offset = g_logical_screen_y_offset;
    }
    if (temptextsave.empty()) // only save screen first time called
    {
        temptextsave.resize(textxdots*textydots);
        for (int i = 0; i < textydots; ++i)
        {
            get_line(i, 0, textxdots-1, &temptextsave[textxdots*i]);
        }
    }

    find_special_colors(); // get g_color_dark & g_color_medium set
    driver_display_string(0, 0, g_color_medium, g_color_dark, msg);
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;

    return false;
}

void cleartempmsg()
{
    if (driver_diskp()) // disk video, easy
    {
        dvid_status(0, "");
    }
    else if (!temptextsave.empty())
    {
        int save_sxoffs = g_logical_screen_x_offset;
        int save_syoffs = g_logical_screen_y_offset;
        if (g_video_scroll)
        {
            g_logical_screen_x_offset = g_video_start_x;
            g_logical_screen_y_offset = g_video_start_y;
        }
        else
        {
            g_logical_screen_y_offset = 0;
            g_logical_screen_x_offset = g_logical_screen_y_offset;
        }
        for (int i = 0; i < textydots; ++i)
        {
            put_line(i, 0, textxdots-1, &temptextsave[textxdots*i]);
        }
        if (!g_using_jiim)                // jiim frees memory with freetempmsg()
        {
            temptextsave.clear();
        }
        g_logical_screen_x_offset = save_sxoffs;
        g_logical_screen_y_offset = save_syoffs;
    }
}

void blankrows(int row, int rows, int attr)
{
    char buf[81];
    std::memset(buf, ' ', 80);
    buf[80] = 0;
    while (--rows >= 0)
    {
        driver_put_string(row++, 0, attr, buf);
    }
}

void helptitle()
{
    char msg[MSG_LEN], buf[MSG_LEN];
    driver_set_clear(); // clear the screen
    std::sprintf(msg, "FRACTINT Version %d.%01d", g_release/100, (g_release%100)/10);
    if (g_release%10)
    {
        std::sprintf(buf, "%01d", g_release%10);
        std::strcat(msg, buf);
    }
    if (g_patch_level)
    {
        std::sprintf(buf, ".%d", g_patch_level);
        std::strcat(msg, buf);
    }
    putstringcenter(0, 0, 80, C_TITLE, msg);
}

namespace
{

void footer_msg(int *i, int options, char const *speedstring)
{
    putstringcenter((*i)++, 0, 80, C_PROMPT_BKGRD,
                    (speedstring) ? "Use the cursor keys or type a value to make a selection"
                    : "Use the cursor keys to highlight your selection");
    putstringcenter(*(i++), 0, 80, C_PROMPT_BKGRD,
                    (options & CHOICE_MENU) ? "Press ENTER for highlighted choice, or " FK_F1 " for help"
                    : ((options & CHOICE_HELP) ? "Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help"
                       : "Press ENTER for highlighted choice, or ESCAPE to back out"));
}

} // namespace

int putstringcenter(int row, int col, int width, int attr, char const *msg)
{
    char buf[81];
    int i, j, k;
    i = 0;
#ifdef XFRACT
    if (width >= 80)
    {
        width = 79; // Some systems choke in column 80
    }
#endif
    while (msg[i])
    {
        ++i; // std::strlen for a
    }
    if (i == 0)
    {
        return -1;
    }
    if (i >= width)
    {
        i = width - 1; // sanity check
    }
    j = (width - i) / 2;
    j -= (width + 10 - i) / 20; // when wide a bit left of center looks better
    std::memset(buf, ' ', width);
    buf[width] = 0;
    i = 0;
    k = j;
    while (msg[i])
    {
        buf[k++] = msg[i++]; // std::strcpy for a
    }
    driver_put_string(row, col, attr, buf);
    return j;
}

// ------------------------------------------------------------------------

std::string const g_speed_prompt{"Speed key string"};

/* For file list purposes only, it's a directory name if first
   char is a dot or last char is a slash */
static int isadirname(char const *name)
{
    if (*name == '.' || endswithslash(name))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

namespace
{

void show_speedstring(
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
            driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, g_speed_prompt.c_str());
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

} // namespace

void process_speedstring(char    *speedstring,
                         char const **choices,         // array of choice strings
                         int       curkey,
                         int      *pcurrent,
                         int       numchoices,
                         int       is_unsorted)
{
    int i = (int) std::strlen(speedstring);
    if (curkey == 8 && i > 0)   // backspace
    {
        speedstring[--i] = 0;
    }
    if (33 <= curkey && curkey <= 126 && i < 30)
    {
#ifndef XFRACT
        curkey = std::tolower(curkey);
#endif
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

static int menutype;
#define MENU_HDG 3
#define MENU_ITEM 1

int main_menu(int fullmenu)
{
    char const *choices[44]; // 2 columns * 22 rows
    int attributes[44];
    int choicekey[44];
    int i;
    int nextleft, nextright;
    bool showjuliatoggle;
    bool const old_tab_mode = g_tab_mode;

top:
    menutype = fullmenu;
    g_tab_mode = false;
    showjuliatoggle = false;
    for (int j = 0; j < 44; ++j)
    {
        attributes[j] = 256;
        choices[j] = "";
        choicekey[j] = -1;
    }
    nextleft = -2;
    nextright = -1;

    if (fullmenu)
    {
        nextleft += 2;
        choices[nextleft] = "      CURRENT IMAGE         ";
        attributes[nextleft] = 256+MENU_HDG;

        nextleft += 2;
        choicekey[nextleft] = 13; // enter
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = (g_calc_status == calc_status_value::RESUMABLE) ?
                            "continue calculation        " :
                            "return to image             ";

        nextleft += 2;
        choicekey[nextleft] = 9; // tab
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "info about image      <tab> ";

        nextleft += 2;
        choicekey[nextleft] = 'o';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "orbits window          <o>  ";
        if (!(g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::INVERSEJULIA))
        {
            nextleft += 2;
        }
    }

    nextleft += 2;
    choices[nextleft] = "      NEW IMAGE             ";
    attributes[nextleft] = 256+MENU_HDG;

    nextleft += 2;
    choicekey[nextleft] = FIK_DELETE;
    attributes[nextleft] = MENU_ITEM;
#ifdef XFRACT
    choices[nextleft] = "draw fractal           <D>  ";
#else
    choices[nextleft] = "select video mode...  <del> ";
#endif

    nextleft += 2;
    choicekey[nextleft] = 't';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "select fractal type    <t>  ";

    if (fullmenu)
    {
        if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
            || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
        {
            nextleft += 2;
            choicekey[nextleft] = FIK_SPACE;
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "toggle to/from julia <space>";
            showjuliatoggle = true;
        }
        if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP
            || g_fractal_type == fractal_type::INVERSEJULIA)
        {
            nextleft += 2;
            choicekey[nextleft] = 'j';
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "toggle to/from inverse <j>  ";
            showjuliatoggle = true;
        }

        nextleft += 2;
        choicekey[nextleft] = 'h';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "return to prior image  <h>   ";

        nextleft += 2;
        choicekey[nextleft] = FIK_BACKSPACE;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "reverse thru history <ctl-h> ";
    }
    else
    {
        nextleft += 2;
    }

    nextleft += 2;
    choices[nextleft] = "      OPTIONS                ";
    attributes[nextleft] = 256+MENU_HDG;

    nextleft += 2;
    choicekey[nextleft] = 'x';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "basic options...       <x>  ";

    nextleft += 2;
    choicekey[nextleft] = 'y';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "extended options...    <y>  ";

    nextleft += 2;
    choicekey[nextleft] = 'z';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "type-specific parms... <z>  ";

    nextleft += 2;
    choicekey[nextleft] = 'p';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "passes options...      <p>  ";

    nextleft += 2;
    choicekey[nextleft] = 'v';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "view window options... <v>  ";

    if (!showjuliatoggle)
    {
        nextleft += 2;
        choicekey[nextleft] = 'i';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "fractal 3D parms...    <i>  ";
    }

    nextleft += 2;
    choicekey[nextleft] = FIK_CTL_B;
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "browse parms...      <ctl-b>";

    if (fullmenu)
    {
        nextleft += 2;
        choicekey[nextleft] = FIK_CTL_E;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "evolver parms...     <ctl-e>";

#ifndef XFRACT
        nextleft += 2;
        choicekey[nextleft] = FIK_CTL_F;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "sound parms...       <ctl-f>";
#endif
    }

    nextright += 2;
    attributes[nextright] = 256 + MENU_HDG;
    choices[nextright] = "        std::FILE                  ";

    nextright += 2;
    choicekey[nextright] = '@';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "run saved command set... <@>  ";

    if (fullmenu)
    {
        nextright += 2;
        choicekey[nextright] = 's';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "save image to file       <s>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'r';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "load image from file...  <r>  ";

    nextright += 2;
    choicekey[nextright] = '3';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "3d transform from file...<3>  ";

    if (fullmenu)
    {
        nextright += 2;
        choicekey[nextright] = '#';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "3d overlay from file.....<#>  ";

        nextright += 2;
        choicekey[nextright] = 'b';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "save current parameters..<b>  ";

        nextright += 2;
        choicekey[nextright] = 16;
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "print image          <ctl-p>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'd';
    attributes[nextright] = MENU_ITEM;
#ifdef XFRACT
    choices[nextright] = "shell to Linux/Unix      <d>  ";
#else
    choices[nextright] = "shell to dos             <d>  ";
#endif

    nextright += 2;
    choicekey[nextright] = 'g';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "give command string      <g>  ";

    nextright += 2;
    choicekey[nextright] = FIK_ESC;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "quit " FRACTINT "           <esc> ";

    nextright += 2;
    choicekey[nextright] = FIK_INSERT;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "restart " FRACTINT "        <ins> ";

#ifdef XFRACT
    if (fullmenu && (g_got_real_dac || g_fake_lut) && g_colors >= 16)
#else
    if (fullmenu && g_got_real_dac && g_colors >= 16)
#endif
    {
        nextright += 2;
        choices[nextright] = "       COLORS                 ";
        attributes[nextright] = 256+MENU_HDG;

        nextright += 2;
        choicekey[nextright] = 'c';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "color cycling mode       <c>  ";

        nextright += 2;
        choicekey[nextright] = '+';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "rotate palette      <+>, <->  ";

        if (g_colors > 16)
        {
            nextright += 2;
            choicekey[nextright] = 'e';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "palette editing mode     <e>  ";

            nextright += 2;
            choicekey[nextright] = 'a';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "make starfield           <a>  ";
        }
    }

    nextright += 2;
    choicekey[nextright] = FIK_CTL_A;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "ant automaton          <ctl-a>";

    nextright += 2;
    choicekey[nextright] = FIK_CTL_S;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "stereogram             <ctl-s>";

    i = driver_key_pressed() ? driver_get_key() : 0;
    if (menu_checkkey(i, 0) == 0)
    {
        g_help_mode = help_labels::HELPMAIN;         // switch help modes
        nextleft += 2;
        if (nextleft < nextright)
        {
            nextleft = nextright + 1;
        }
        i = fullscreen_choice(CHOICE_MENU | CHOICE_CRUNCH,
                              "MAIN MENU",
                              nullptr, nullptr, nextleft, (char const **) choices, attributes,
                              2, nextleft/2, 29, 0, nullptr, nullptr, nullptr, menu_checkkey);
        if (i == -1)     // escape
        {
            i = FIK_ESC;
        }
        else if (i < 0)
        {
            i = 0 - i;
        }
        else                      // user selected a choice
        {
            i = choicekey[i];
            if (-10 == i)
            {
                g_help_mode = help_labels::HELPZOOM;
                help(0);
                i = 0;
            }
        }
    }
    if (i == FIK_ESC)             // escape from menu exits Fractint
    {
        helptitle();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
        for (int j = 9; j <= 11; ++j)
        {
            driver_set_attr(j, 18, C_GENERAL_INPUT, 40);
        }
        putstringcenter(10, 18, 40, C_GENERAL_INPUT,
#ifdef XFRACT
                        "Exit from Xfractint (y/n)? y"
#else
                        "Exit from Fractint (y/n)? y"
#endif
                       );
        driver_hide_text_cursor();
        while ((i = driver_get_key()) != 'y' && i != 'Y' && i != FIK_ENTER)
        {
            if (i == 'n' || i == 'N')
            {
                goto top;
            }
        }
        goodbye();
    }
    if (i == FIK_TAB)
    {
        tab_display();
        i = 0;
    }
    if (i == FIK_ENTER || i == FIK_ENTER_2)
    {
        i = 0;                 // don't trigger new calc
    }
    g_tab_mode = old_tab_mode;
    return i;
}

static int menu_checkkey(int curkey, int /*choice*/)
{
    int testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
    // We use F2 for shift-@, annoyingly enough
    if (testkey == FIK_F2)
    {
        return -testkey;
    }
#endif
    if (testkey == '2')
    {
        testkey = '@';

    }
    if (std::strchr("#@2txyzgvir3dj", testkey)
        || testkey == FIK_INSERT || testkey == FIK_CTL_B
        || testkey == FIK_ESC || testkey == FIK_DELETE
        || testkey == FIK_CTL_F)
    {
        return -testkey;
    }
    if (menutype)
    {
        if (std::strchr("\\sobpkrh", testkey)
            || testkey == FIK_TAB || testkey == FIK_CTL_A
            || testkey == FIK_CTL_E || testkey == FIK_BACKSPACE
            || testkey == FIK_CTL_P || testkey == FIK_CTL_S
            || testkey == FIK_CTL_U)   // ctrl-A, E, H, P, S, U
        {
            return -testkey;
        }
        if (testkey == ' ')
        {
            if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
                || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
            {
                return -testkey;
            }
        }
        if (g_got_real_dac && g_colors >= 16)
        {
            if (std::strchr("c+-", testkey))
            {
                return -testkey;
            }
            if (g_colors > 16 && (testkey == 'a' || (testkey == 'e')))
            {
                return -testkey;
            }
        }
        // Alt-A and Alt-S
        if (testkey == FIK_ALT_A || testkey == FIK_ALT_S)
        {
            return -testkey;
        }
    }
    if (check_vidmode_key(0, testkey) >= 0)
    {
        return -testkey;
    }
    return 0;
}

int input_field(
    int options,          // &1 numeric, &2 integer, &4 double
    int attr,             // display attribute
    char *fld,            // the field itself
    int len,              // field length (declare as 1 larger for \0)
    int row,              // display row
    int col,              // display column
    int (*checkkey)(int curkey)  // routine to check non data keys, or nullptr
)
{
    char savefld[81];
    char buf[81];
    int curkey;
    int i, j;
    int old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    int ret = -1;
    std::strcpy(savefld, fld);
    int insert = 0;
    bool started = false;
    int offset = 0;
    bool display = true;
    while (true)
    {
        std::strcpy(buf, fld);
        i = (int) std::strlen(buf);
        while (i < len)
        {
            buf[i++] = ' ';

        }
        buf[len] = 0;
        if (display)
        {
            // display current value
            driver_put_string(row, col, attr, buf);
            display = false;
        }
        curkey = driver_key_cursor(row+insert, col+offset);  // get a keystroke
        if (curkey == 1047)
        {
            curkey = 47; // numeric slash
        }
        switch (curkey)
        {
        case FIK_ENTER:
        case FIK_ENTER_2:
            ret = 0;
            goto inpfld_end;
        case FIK_ESC:
            goto inpfld_end;
        case FIK_RIGHT_ARROW:
            if (offset < len-1)
            {
                ++offset;
            }
            started = true;
            break;
        case FIK_LEFT_ARROW:
            if (offset > 0)
            {
                --offset;
            }
            started = true;
            break;
        case FIK_HOME:
            offset = 0;
            started = true;
            break;
        case FIK_END:
            offset = (int) std::strlen(fld);
            started = true;
            break;
        case FIK_BACKSPACE:
        case 127:                              // backspace
            if (offset > 0)
            {
                j = (int) std::strlen(fld);
                for (int k = offset-1; k < j; ++k)
                {
                    fld[k] = fld[k+1];
                }
                --offset;
            }
            started = true;
            display = true;
            break;
        case FIK_DELETE:                           // delete
            j = (int) std::strlen(fld);
            for (int k = offset; k < j; ++k)
            {
                fld[k] = fld[k+1];
            }
            started = true;
            display = true;
            break;
        case FIK_INSERT:                           // insert
            insert ^= 0x8000;
            started = true;
            break;
        case FIK_F5:
            std::strcpy(fld, savefld);
            offset = 0;
            insert = offset;
            started = false;
            display = true;
            break;
        default:
            if (nonalpha(curkey))
            {
                if (checkkey && (ret = (*checkkey)(curkey)) != 0)
                {
                    goto inpfld_end;
                }
                break;                                // non alphanum char
            }
            if (offset >= len)
            {
                break;                // at end of field
            }
            if (insert && started && std::strlen(fld) >= (size_t)len)
            {
                break;                                // insert & full
            }
            if ((options & INPUTFIELD_NUMERIC)
                && (curkey < '0' || curkey > '9')
                && curkey != '+' && curkey != '-')
            {
                if (options & INPUTFIELD_INTEGER)
                {
                    break;
                }
                // allow scientific notation, and specials "e" and "p"
                if (((curkey != 'e' && curkey != 'E') || offset >= 18)
                    && ((curkey != 'p' && curkey != 'P') || offset != 0)
                    && curkey != '.')
                {
                    break;
                }
            }
            if (!started)   // first char is data, zap field
            {
                fld[0] = 0;
            }
            if (insert)
            {
                j = (int) std::strlen(fld);
                while (j >= offset)
                {
                    fld[j+1] = fld[j];
                    --j;
                }
            }
            if ((size_t)offset >= std::strlen(fld))
            {
                fld[offset+1] = 0;
            }
            fld[offset++] = (char)curkey;
            // if "e" or "p" in first col make number e or pi
            if ((options & (INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER)) == INPUTFIELD_NUMERIC)
            {
                // floating point
                double tmpd;
                char tmpfld[30];
                bool specialv = false;
                if (*fld == 'e' || *fld == 'E')
                {
                    tmpd = std::exp(1.0);
                    specialv = true;
                }
                if (*fld == 'p' || *fld == 'P')
                {
                    tmpd = std::atan(1.0) * 4;
                    specialv = true;
                }
                if (specialv)
                {
                    if ((options & INPUTFIELD_DOUBLE) == 0)
                    {
                        roundfloatd(&tmpd);
                    }
                    std::sprintf(tmpfld, "%.15g", tmpd);
                    tmpfld[len-1] = 0; // safety, field should be long enough
                    std::strcpy(fld, tmpfld);
                    offset = 0;
                }
            }
            started = true;
            display = true;
        }
    }
inpfld_end:
    g_look_at_mouse = old_look_at_mouse;
    return ret;
}

int field_prompt(
    char const *hdg,        // heading, \n delimited lines
    char const *instr,      // additional instructions or nullptr
    char *fld,              // the field itself
    int len,                // field length (declare as 1 larger for \0)
    int (*checkkey)(int curkey) // routine to check non data keys, or nullptr
)
{
    char const *charptr;
    int boxwidth, titlelines, titlecol, titlerow;
    int promptcol;
    int i, j;
    char buf[81] = { 0 };
    helptitle();                           // clear screen, display title
    driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);     // init rest to background
    charptr = hdg;                         // count title lines, find widest
    boxwidth = 0;
    i = boxwidth;
    titlelines = 1;
    while (*charptr)
    {
        if (*(charptr++) == '\n')
        {
            ++titlelines;
            i = -1;
        }
        if (++i > boxwidth)
        {
            boxwidth = i;
        }
    }
    if (len > boxwidth)
    {
        boxwidth = len;
    }
    i = titlelines + 4;                    // total rows in box
    titlerow = (25 - i) / 2;               // top row of it all when centered
    titlerow -= titlerow / 4;              // higher is better if lots extra
    titlecol = (80 - boxwidth) / 2;        // center the box
    titlecol -= (90 - boxwidth) / 20;
    promptcol = titlecol - (boxwidth-len)/2;
    j = titlecol;                          // add margin at each side of box
    i = (82-boxwidth)/4;
    if (i > 3)
    {
        i = 3;
    }
    j -= i;
    boxwidth += i * 2;
    for (int k = -1; k < titlelines + 3; ++k) // draw empty box
    {
        driver_set_attr(titlerow + k, j, C_PROMPT_LO, boxwidth);
    }
    g_text_cbase = titlecol;                  // set left margin for putstring
    driver_put_string(titlerow, 0, C_PROMPT_HI, hdg); // display heading
    g_text_cbase = 0;
    i = titlerow + titlelines + 4;
    if (instr)
    {
        // display caller's instructions
        charptr = instr;
        j = -1;
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
    else                                     // default instructions
    {
        putstringcenter(i, 0, 80, C_PROMPT_BKGRD, "Press ENTER when finished (or ESCAPE to back out)");
    }
    return input_field(0, C_PROMPT_INPUT, fld, len,
                       titlerow+titlelines+1, promptcol, checkkey);
}


/* thinking(1,message):
      if thinking message not yet on display, it is displayed;
      otherwise the wheel is updated
      returns 0 to keep going, -1 if keystroke pending
   thinking(0,nullptr):
      call this when thinking phase is done
   */

bool thinking(int options, char const *msg)
{
    static int thinkstate = -1;
    char const *wheel[] = {"-", "\\", "|", "/"};
    static int thinkcol;
    static int count = 0;
    char buf[81];
    if (options == 0)
    {
        if (thinkstate >= 0)
        {
            thinkstate = -1;
            driver_unstack_screen();
        }
        return false;
    }
    if (thinkstate < 0)
    {
        driver_stack_screen();
        thinkstate = 0;
        helptitle();
        std::strcpy(buf, "  ");
        std::strcat(buf, msg);
        std::strcat(buf, "    ");
        driver_put_string(4, 10, C_GENERAL_HI, buf);
        thinkcol = g_text_col - 3;
        count = 0;
    }
    if ((count++) < 100)
    {
        return false;
    }
    count = 0;
    driver_put_string(4, thinkcol, C_GENERAL_HI, wheel[thinkstate]);
    driver_hide_text_cursor(); // turn off cursor
    thinkstate = (thinkstate + 1) & 3;
    return driver_key_pressed() != 0;
}


// savegraphics/restoregraphics: video.asm subroutines

#define SWAPBLKLEN 4096 // must be a power of 2
static U16 memhandle = 0;

void discardgraphics() // release expanded/extended memory if any in use
{
#ifndef XFRACT
    MemoryRelease(memhandle);
    memhandle = 0;
#endif
}

//VIDEOINFO *g_video_table;*/  /* temporarily loaded id.cfg info
int g_video_table_len;                 // number of entries in above

int showvidlength()
{
    int sz;
    sz = (sizeof(VIDEOINFO)+sizeof(int))*MAX_VIDEO_MODES;
    return sz;
}

int g_cfg_line_nums[MAX_VIDEO_MODES] = { 0 };

/* load_id_config
 *
 * Reads id.cfg, loading videoinfo entries into g_video_table.
 * Sets the number of entries, sets g_video_table_len.
 * Past g_video_table, g_cfg_line_nums are stored for update_id_cfg.
 * If id.cfg is not found or invalid, issues a message
 * (first time the problem occurs only, and only if options is
 * zero) and uses the hard-coded table.
 */
void load_id_config()
{
    std::FILE   *cfgfile;
    VIDEOINFO    vident;
    int          linenum;
    long         xdots, ydots;
    int          i;
    int          j;
    int          keynum;
    unsigned int ax;
    unsigned int bx;
    unsigned int cx;
    unsigned int dx;
    int          dotmode;
    int          colors;
    char        *fields[11] = {nullptr};
    int          textsafe2;
    int          truecolorbits;

    const std::string cfg_path{find_path("id.cfg")};
    if (cfg_path.empty()                                             // can't find the file
        || (cfgfile = std::fopen(cfg_path.c_str(), "r")) == nullptr) // can't open it
    {
        g_bad_config = config_status::BAD_NO_MESSAGE;
        return;
    }

    linenum = 0;
    char tempstring[150];
    while (g_video_table_len < MAX_VIDEO_MODES
        && std::fgets(tempstring, NUM_OF(tempstring), cfgfile))
    {
        if (std::strchr(tempstring, '\n') == nullptr)
        {
            // finish reading the line
            while (fgetc(cfgfile) != '\n' && !std::feof(cfgfile));
        }
        ++linenum;
        if (tempstring[0] == ';')
        {
            continue;   // comment line
        }
        tempstring[120] = 0;
        tempstring[(int) std::strlen(tempstring)-1] = 0; // zap trailing \n
        j = -1;
        i = j;
        // key, mode name, ax, bx, cx, dx, dotmode, x, y, colors, comments, driver
        while (true)
        {
            if (tempstring[++i] < ' ')
            {
                if (tempstring[i] == 0)
                {
                    break;
                }
                tempstring[i] = ' '; // convert tab (or whatever) to blank
            }
            else if (tempstring[i] == ',' && ++j < 11)
            {
                assert(j >= 0 && j < 11);
                fields[j] = &tempstring[i+1]; // remember start of next field
                tempstring[i] = 0;   // make field a separate string
            }
        }
        keynum = check_vidmode_keyname(tempstring);
        std::sscanf(fields[1], "%x", &ax);
        std::sscanf(fields[2], "%x", &bx);
        std::sscanf(fields[3], "%x", &cx);
        std::sscanf(fields[4], "%x", &dx);
        assert(fields[5]);
        dotmode = atoi(fields[5]);
        assert(fields[6]);
        xdots = atol(fields[6]);
        assert(fields[7]);
        ydots = atol(fields[7]);
        assert(fields[8]);
        colors = atoi(fields[8]);
        if (colors == 4 && std::strchr(strlwr(fields[8]), 'g'))
        {
            colors = 256;
            truecolorbits = 4; // 32 bits
        }
        else if (colors == 16 && std::strchr(fields[8], 'm'))
        {
            colors = 256;
            truecolorbits = 3; // 24 bits
        }
        else if (colors == 64 && std::strchr(fields[8], 'k'))
        {
            colors = 256;
            truecolorbits = 2; // 16 bits
        }
        else if (colors == 32 && std::strchr(fields[8], 'k'))
        {
            colors = 256;
            truecolorbits = 1; // 15 bits
        }
        else
        {
            truecolorbits = 0;
        }

        textsafe2   = dotmode / 100;
        dotmode    %= 100;
        if (j < 9 ||
                keynum < 0 ||
                dotmode < 0 || dotmode > 30 ||
                textsafe2 < 0 || textsafe2 > 4 ||
                xdots < MIN_PIXELS || xdots > MAX_PIXELS ||
                ydots < MIN_PIXELS || ydots > MAX_PIXELS ||
                (colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
                 colors != 256)
           )
        {
            g_bad_config = config_status::BAD_NO_MESSAGE;
            return;
        }
        g_cfg_line_nums[g_video_table_len] = linenum; // for update_fractint_cfg

        std::memset(&vident, 0, sizeof(vident));
        std::strncpy(&vident.name[0], fields[0], NUM_OF(vident.name));
        std::strncpy(&vident.comment[0], fields[9], NUM_OF(vident.comment));
        vident.comment[25] = 0;
        vident.name[25] = vident.comment[25];
        vident.keynum      = keynum;
        vident.videomodeax = ax;
        vident.videomodebx = bx;
        vident.videomodecx = cx;
        vident.videomodedx = dx;
        vident.dotmode     = truecolorbits * 1000 + textsafe2 * 100 + dotmode;
        vident.xdots       = (short)xdots;
        vident.ydots       = (short)ydots;
        vident.colors      = colors;

        // if valid, add to supported modes
        vident.driver = driver_find_by_name(fields[10]);
        if (vident.driver != nullptr)
        {
            if (vident.driver->validate_mode(vident.driver, &vident))
            {
                // look for a synonym mode and if found, overwite its key
                bool synonym_found = false;
                for (int m = 0; m < g_video_table_len; m++)
                {
                    VIDEOINFO *mode = &g_video_table[m];
                    if ((mode->driver == vident.driver) && (mode->colors == vident.colors)
                        && (mode->xdots == vident.xdots) && (mode->ydots == vident.ydots)
                        && (mode->dotmode == vident.dotmode))
                    {
                        if (0 == mode->keynum)
                        {
                            mode->keynum = vident.keynum;
                        }
                        synonym_found = true;
                        break;
                    }
                }
                // no synonym found, append it to current list of video modes
                if (!synonym_found)
                {
                    add_video_mode(vident.driver, &vident);
                }
            }
        }
    }
    std::fclose(cfgfile);
}

void bad_id_cfg_msg()
{
    stopmsg(STOPMSG_NONE,
            "File id.cfg is missing or invalid.\n"
            "See Hardware Support and Video Modes in the full documentation for help.\n"
            "I will continue with only the built-in video modes available.");
    g_bad_config = config_status::BAD_WITH_MESSAGE;
}

int check_vidmode_key(int option, int k)
{
    // returns g_video_table entry number if the passed keystroke is a
    // function key currently assigned to a video mode, -1 otherwise
    if (k == 1400)                // special value from select_vid_mode
    {
        return MAX_VIDEO_MODES-1; // for last entry with no key assigned
    }
    if (k != 0)
    {
        if (option == 0)
        {
            // check resident video mode table
            for (int i = 0; i < MAX_VIDEO_MODES; ++i)
            {
                if (g_video_table[i].keynum == k)
                {
                    return i;
                }
            }
        }
        else
        {
            // check full g_video_table
            for (int i = 0; i < g_video_table_len; ++i)
            {
                if (g_video_table[i].keynum == k)
                {
                    return i;
                }
            }
        }
    }
    return -1;
}

int check_vidmode_keyname(char const *kname)
{
    // returns key number for the passed keyname, 0 if not a keyname
    int i, keyset;
    keyset = 1058;
    if (*kname == 'S' || *kname == 's')
    {
        keyset = 1083;
        ++kname;
    }
    else if (*kname == 'C' || *kname == 'c')
    {
        keyset = 1093;
        ++kname;
    }
    else if (*kname == 'A' || *kname == 'a')
    {
        keyset = 1103;
        ++kname;
    }
    if (*kname != 'F' && *kname != 'f')
    {
        return 0;
    }
    if (*++kname < '1' || *kname > '9')
    {
        return 0;
    }
    i = *kname - '0';
    if (*++kname != 0 && *kname != ' ')
    {
        if (*kname != '0' || i != 1)
        {
            return 0;
        }
        i = 10;
        ++kname;
    }
    while (*kname)
    {
        if (*(kname++) != ' ')
        {
            return 0;
        }
    }
    if ((i += keyset) < 2)
    {
        i = 0;
    }
    return i;
}

void vidmode_keyname(int k, char *buf)
{
    // set buffer to name of passed key number
    *buf = 0;
    if (k > 0)
    {
        if (k > 1103)
        {
            *(buf++) = 'A';
            k -= 1103;
        }
        else if (k > 1093)
        {
            *(buf++) = 'C';
            k -= 1093;
        }
        else if (k > 1083)
        {
            *(buf++) = 'S';
            k -= 1083;
        }
        else
        {
            k -= 1058;
        }
        std::sprintf(buf, "F%d", k);
    }
}
