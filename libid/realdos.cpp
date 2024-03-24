#include "port.h"
#include "prototyp.h"

#include "realdos.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "find_path.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "memory.h"
#include "miscres.h"
#include "os.h"
#include "prompts2.h"
#include "rotate.h"
#include "string_case_compare.h"
#include "video_mode.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifdef XFRACT
#include <unistd.h>
#endif

int g_release = 2099;   // this has 2 implied decimals; increment it every synch
int g_patch_level = 8;  // patchlevel for DOS version

void helptitle()
{
    char msg[MSG_LEN], buf[MSG_LEN];
    driver_set_clear(); // clear the screen
    std::sprintf(msg, "Iterated Dynamics Version %d.%01d", g_release/100, (g_release%100)/10);
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

void process_speedstring(char *speedstring, //
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
