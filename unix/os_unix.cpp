#include <string.h>
#include <sys/statvfs.h>

#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "mpmath.h"
#include "prototyp.h"

// Global variables (yuck!)
struct MP Ans = { 0 };
static char extrasegment[0x18000] = { 0 };
void *extraseg = &extrasegment[0];
int fm_attack = 0;
int fm_decay = 0;
int fm_release = 0;
int fm_sustain = 0;
int fm_vol = 0;
int fm_wavetype = 0;
int hi_atten = 0;
long linitx = 0;
long linity = 0;
int polyphony = 0;
long savebase = 0;              // base clock ticks
long saveticks = 0;             // save after this many ticks

/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAXVIDEOMODES] = { 0 };
int g_vxdots = 0;

// Global variables that should be phased out (old video mode stuff)
int g_video_vram = 0;

bool isadirectory(char *s)
{
    int len;
    char sv;
    if (strchr(s,'*') || strchr(s,'?'))
        return false; // for my purposes, not a directory

    len = (int) strlen(s);
    if (len > 0)
        sv = s[len-1];   // last char
    else
        sv = 0;

    if (fr_findfirst(s) != 0) // couldn't find it
    {
        // any better ideas??
        if (sv == SLASHC) // we'll guess it is a directory
            return true;
        else
            return false; // no slashes - we'll guess it's a file
    }
    else if ((DTA.attribute & SUBDIR) != 0) {
        if (sv == SLASHC) {
            // strip trailing slash and try again
            s[len-1] = 0;
            if (fr_findfirst(s) != 0) // couldn't find it
                return false;
            else if ((DTA.attribute & SUBDIR) != 0)
                return true;  // we're SURE it's a directory
            else
                return false;
        } else
            return true;  // we're SURE it's a directory
    }
    return false;
}

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
    return -1;
}

unsigned long get_disk_space()
{
    // TODO
    return 0x7FFFFFFF;
}

void init_failure(const char *message)
{
    fputs(message, stderr);
}
