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
