#include "realdos.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "find_path.h"
#include "help_title.h"
#include "memory.h"
#include "os.h"

#include <cstdio>
#include <cstring>

#ifdef XFRACT
#include <unistd.h>
#endif

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
