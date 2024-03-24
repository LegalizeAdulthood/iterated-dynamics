#include "field_prompt.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "help_title.h"
#include "input_field.h"
#include "os.h"
#include "put_string_center.h"

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
