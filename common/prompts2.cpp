/*
        Various routines that prompt for things.
*/
#include <algorithm>
#include <string>

#include <ctype.h>
#include <string.h>
#if defined(XFRACT)
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#else
#include <direct.h>
#include <io.h>
#endif
#ifdef __hpux
#include <sys/param.h>
#endif

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "helpcom.h"

// Routines in this module

static  int check_f6_key(int curkey, int choice);
static  int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match);
static  int get_screen_corners();

// speed key state values
#define MATCHING         0      // string matches list - speed key mode
#define TEMPLATE        -2      // wild cards present - buiding template
#define SEARCHPATH      -3      // no match - building path search name

#define   FILEATTR       0x37      // File attributes; select all but volume labels
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16
#define   MAXNUMFILES    2977L

DIR_SEARCH DTA;          // Allocate DTA and define structure

#define GETFORMULA 0
#define GETLSYS    1
#define GETIFS     2
#define GETPARM    3

char commandmask[13] = {"*.par"};

// ---------------------------------------------------------------------
/*
        get_toggles() is called from FRACTINT.C whenever the 'x' key
        is pressed.  This routine prompts for several options,
        sets the appropriate variables, and returns the following code
        to the calling routine:

        -1  routine was ESCAPEd - no need to re-generate the image.
         0  nothing changed, or minor variable such as "overwrite=".
            No need to re-generate the image.
         1  major variable changed (such as "inside=").  Re-generate
            the image.

        Finally, remember to insert variables in the list *and* check
        for them in the same order!!!
*/

int get_toggles()
{
    char const *choices[20];
    char prevsavename[FILE_MAX_DIR+1];
    char const *savenameptr;
    fullscreenvalues uvalues[25];
    int i, j, k;
    char old_usr_stdcalcmode;
    long old_maxit, old_logflag;
    int old_inside, old_outside, old_soundflag;
    int old_biomorph, old_decomp;
    int old_fillcolor;
    int old_stoppass;
    double old_closeprox;
    char const *calcmodes[] = {"1", "2", "3", "g", "g1", "g2", "g3", "g4", "g5", "g6", "b", "s", "t", "d", "o"};
    char const *soundmodes[5] = {"off", "beep", "x", "y", "z"};
    char const *insidemodes[] = {"numb", "maxiter", "zmag", "bof60", "bof61", "epsiloncross",
                          "startrail", "period", "atan", "fmod"
                         };
    char const *outsidemodes[] = {"numb", "iter", "real", "imag", "mult", "summ", "atan",
                           "fmod", "tdis"
                          };

    k = -1;

    choices[++k] = "Passes (1,2,3, g[uess], b[ound], t[ess], d[iffu], o[rbit])";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 3;
    uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
    uvalues[k].uval.ch.list = calcmodes;
    uvalues[k].uval.ch.val = (usr_stdcalcmode == '1') ? 0
                             : (usr_stdcalcmode == '2') ? 1
                             : (usr_stdcalcmode == '3') ? 2
                             : (usr_stdcalcmode == 'g' && stoppass == 0) ? 3
                             : (usr_stdcalcmode == 'g' && stoppass == 1) ? 4
                             : (usr_stdcalcmode == 'g' && stoppass == 2) ? 5
                             : (usr_stdcalcmode == 'g' && stoppass == 3) ? 6
                             : (usr_stdcalcmode == 'g' && stoppass == 4) ? 7
                             : (usr_stdcalcmode == 'g' && stoppass == 5) ? 8
                             : (usr_stdcalcmode == 'g' && stoppass == 6) ? 9
                             : (usr_stdcalcmode == 'b') ? 10
                             : (usr_stdcalcmode == 's') ? 11
                             : (usr_stdcalcmode == 't') ? 12
                             : (usr_stdcalcmode == 'd') ? 13
                             :        /* "o"rbits */      14;
    old_usr_stdcalcmode = usr_stdcalcmode;
    old_stoppass = stoppass;
#ifndef XFRACT
    choices[++k] = "Floating Point Algorithm";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = usr_floatflag ? 1 : 0;
#endif
    choices[++k] = "Maximum Iterations (2 to 2,147,483,647)";
    uvalues[k].type = 'L';
    old_maxit = maxit;
    uvalues[k].uval.Lval = old_maxit;

    choices[++k] = "Inside Color (0-# of colors, if Inside=numb)";
    uvalues[k].type = 'i';
    if (inside >= COLOR_BLACK)
    {
        uvalues[k].uval.ival = inside;
    }
    else
    {
        uvalues[k].uval.ival = 0;
    }

    choices[++k] = "Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 12;
    uvalues[k].uval.ch.llen = sizeof(insidemodes)/sizeof(*insidemodes);
    uvalues[k].uval.ch.list = insidemodes;
    if (inside >= COLOR_BLACK)    // numb
    {
        uvalues[k].uval.ch.val = 0;
    }
    else if (inside == ITER)
    {
        uvalues[k].uval.ch.val = 1;
    }
    else if (inside == ZMAG)
    {
        uvalues[k].uval.ch.val = 2;
    }
    else if (inside == BOF60)
    {
        uvalues[k].uval.ch.val = 3;
    }
    else if (inside == BOF61)
    {
        uvalues[k].uval.ch.val = 4;
    }
    else if (inside == EPSCROSS)
    {
        uvalues[k].uval.ch.val = 5;
    }
    else if (inside == STARTRAIL)
    {
        uvalues[k].uval.ch.val = 6;
    }
    else if (inside == PERIOD)
    {
        uvalues[k].uval.ch.val = 7;
    }
    else if (inside == ATANI)
    {
        uvalues[k].uval.ch.val = 8;
    }
    else if (inside == FMODI)
    {
        uvalues[k].uval.ch.val = 9;
    }
    old_inside = inside;

    choices[++k] = "Outside Color (0-# of colors, if Outside=numb)";
    uvalues[k].type = 'i';
    if (outside >= COLOR_BLACK)
    {
        uvalues[k].uval.ival = outside;
    }
    else
    {
        uvalues[k].uval.ival = 0;
    }

    choices[++k] = "Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 4;
    uvalues[k].uval.ch.llen = sizeof(outsidemodes)/sizeof(*outsidemodes);
    uvalues[k].uval.ch.list = outsidemodes;
    if (outside >= COLOR_BLACK)    // numb
    {
        uvalues[k].uval.ch.val = 0;
    }
    else
    {
        uvalues[k].uval.ch.val = -outside;
    }
    old_outside = outside;

    choices[++k] = "Savename (.GIF implied)";
    uvalues[k].type = 's';
    strcpy(prevsavename, savename.c_str());
    savenameptr = strrchr(savename.c_str(), SLASHC);
    if (savenameptr == nullptr)
    {
        savenameptr = savename.c_str();
    }
    else
    {
        savenameptr++; // point past slash
    }
    strcpy(uvalues[k].uval.sval, savenameptr);

    choices[++k] = "File Overwrite ('overwrite=')";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = fract_overwrite ? 1 : 0;

    choices[++k] = "Sound (off, beep, x, y, z)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 4;
    uvalues[k].uval.ch.llen = 5;
    uvalues[k].uval.ch.list = soundmodes;
    uvalues[k].uval.ch.val = (old_soundflag = soundflag) & SOUNDFLAG_ORBITMASK;

    if (rangeslen == 0)
    {
        choices[++k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)";
        uvalues[k].type = 'L';
    }
    else
    {
        choices[++k] = "Log Palette (n/a, ranges= parameter is in effect)";
        uvalues[k].type = '*';
    }
    old_logflag = LogFlag;
    uvalues[k].uval.Lval = old_logflag;

    choices[++k] = "Biomorph Color (-1 means OFF)";
    uvalues[k].type = 'i';
    old_biomorph = usr_biomorph;
    uvalues[k].uval.ival = old_biomorph;

    choices[++k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
    uvalues[k].type = 'i';
    old_decomp = g_decomp[0];
    uvalues[k].uval.ival = old_decomp;

    choices[++k] = "Fill Color (normal,#) (works with passes=t, b and d)";
    uvalues[k].type = 's';
    if (fillcolor < 0)
    {
        strcpy(uvalues[k].uval.sval, "normal");
    }
    else
    {
        sprintf(uvalues[k].uval.sval, "%d", fillcolor);
    }
    old_fillcolor = fillcolor;

    choices[++k] = "Proximity value for inside=epscross and fmod";
    uvalues[k].type = 'f'; // should be 'd', but prompts get messed up
    old_closeprox = g_close_proximity;
    uvalues[k].uval.dval = old_closeprox;

    int const old_help_mode = help_mode;
    help_mode = HELPXOPTS;
    i = fullscreen_prompt("Basic Options\n(not all combinations make sense)", k+1, choices, uvalues, 0, nullptr);
    help_mode = old_help_mode;
    if (i < 0)
    {
        return (-1);
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    j = 0;   // return code

    usr_stdcalcmode = calcmodes[uvalues[++k].uval.ch.val][0];
    stoppass = (int)calcmodes[uvalues[k].uval.ch.val][1] - (int)'0';

    if (stoppass < 0 || stoppass > 6 || usr_stdcalcmode != 'g')
    {
        stoppass = 0;
    }

    if (usr_stdcalcmode == 'o' && fractype == fractal_type::LYAPUNOV)   // Oops,lyapunov type
    {
        // doesn't use 'new' & breaks orbits
        usr_stdcalcmode = old_usr_stdcalcmode;
    }

    if (old_usr_stdcalcmode != usr_stdcalcmode)
    {
        j++;
    }
    if (old_stoppass != stoppass)
    {
        j++;
    }
#ifndef XFRACT
    if ((uvalues[++k].uval.ch.val != 0) != usr_floatflag)
    {
        usr_floatflag = uvalues[k].uval.ch.val != 0;
        j++;
    }
#endif
    ++k;
    maxit = uvalues[k].uval.Lval;
    if (maxit < 0)
    {
        maxit = old_maxit;
    }
    if (maxit < 2)
    {
        maxit = 2;
    }

    if (maxit != old_maxit)
    {
        j++;
    }

    inside = uvalues[++k].uval.ival;
    if (inside < COLOR_BLACK)
    {
        inside = -inside;
    }
    if (inside >= g_colors)
    {
        inside = (inside % g_colors) + (inside / g_colors);
    }

    {
        int tmp;
        tmp = uvalues[++k].uval.ch.val;
        if (tmp > 0)
        {
            switch (tmp)
            {
            case 1:
                inside = ITER;
                break;
            case 2:
                inside = ZMAG;
                break;
            case 3:
                inside = BOF60;
                break;
            case 4:
                inside = BOF61;
                break;
            case 5:
                inside = EPSCROSS;
                break;
            case 6:
                inside = STARTRAIL;
                break;
            case 7:
                inside = PERIOD;
                break;
            case 8:
                inside = ATANI;
                break;
            case 9:
                inside = FMODI;
                break;
            }
        }
    }
    if (inside != old_inside)
    {
        j++;
    }

    outside = uvalues[++k].uval.ival;
    if (outside < COLOR_BLACK)
    {
        outside = -outside;
    }
    if (outside >= g_colors)
    {
        outside = (outside % g_colors) + (outside / g_colors);
    }

    {
        int tmp;
        tmp = uvalues[++k].uval.ch.val;
        if (tmp > 0)
        {
            outside = -tmp;
        }
    }
    if (outside != old_outside)
    {
        j++;
    }

    savename = std::string {savename.c_str(), savenameptr} + uvalues[++k].uval.sval;
    if (strcmp(savename.c_str(), prevsavename))
    {
        resave_flag = 0;
        started_resaves = false; // forget pending increment
    }
    fract_overwrite = uvalues[++k].uval.ch.val != 0;

    soundflag = ((soundflag >> 3) << 3) | (uvalues[++k].uval.ch.val);
    if (soundflag != old_soundflag && ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP || (old_soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP))
    {
        j++;
    }

    LogFlag = uvalues[++k].uval.Lval;
    if (LogFlag != old_logflag)
    {
        j++;
        Log_Auto_Calc = false;          // turn it off, use the supplied value
    }

    usr_biomorph = uvalues[++k].uval.ival;
    if (usr_biomorph >= g_colors)
    {
        usr_biomorph = (usr_biomorph % g_colors) + (usr_biomorph / g_colors);
    }
    if (usr_biomorph != old_biomorph)
    {
        j++;
    }

    g_decomp[0] = uvalues[++k].uval.ival;
    if (g_decomp[0] != old_decomp)
    {
        j++;
    }

    if (strncmp(strlwr(uvalues[++k].uval.sval), "normal", 4) == 0)
    {
        fillcolor = -1;
    }
    else
    {
        fillcolor = atoi(uvalues[k].uval.sval);
    }
    if (fillcolor < 0)
    {
        fillcolor = -1;
    }
    if (fillcolor >= g_colors)
    {
        fillcolor = (fillcolor % g_colors) + (fillcolor / g_colors);
    }
    if (fillcolor != old_fillcolor)
    {
        j++;
    }

    ++k;
    g_close_proximity = uvalues[k].uval.dval;
    if (g_close_proximity != old_closeprox)
    {
        j++;
    }

    return (j);
}

/*
        get_toggles2() is similar to get_toggles, invoked by 'y' key
*/

int get_toggles2()
{
    char const *choices[18];
    fullscreenvalues uvalues[23];
    int old_rotate_lo, old_rotate_hi;
    int old_distestwidth;
    double old_potparam[3], old_inversion[3];
    long old_usr_distest;

    // fill up the choices (and previous values) arrays
    int k = -1;

    choices[++k] = "Look for finite attractor (0=no,>0=yes,<0=phase)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ch.val = finattract ? 1 : 0;

    choices[++k] = "Potential Max Color (0 means off)";
    uvalues[k].type = 'i';
    old_potparam[0] = potparam[0];
    uvalues[k].uval.ival = (int) old_potparam[0];

    choices[++k] = "          Slope";
    uvalues[k].type = 'd';
    old_potparam[1] = potparam[1];
    uvalues[k].uval.dval = old_potparam[1];

    choices[++k] = "          Bailout";
    uvalues[k].type = 'i';
    old_potparam[2] = potparam[2];
    uvalues[k].uval.ival = (int) old_potparam[2];

    choices[++k] = "          16 bit values";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = pot16bit ? 1 : 0;

    choices[++k] = "Distance Estimator (0=off, <0=edge, >0=on):";
    uvalues[k].type = 'L';
    old_usr_distest = usr_distest;
    uvalues[k].uval.Lval = old_usr_distest;

    choices[++k] = "          width factor:";
    uvalues[k].type = 'i';
    old_distestwidth = distestwidth;
    uvalues[k].uval.ival = old_distestwidth;

    choices[++k] = "Inversion radius or \"auto\" (0 means off)";
    choices[++k] = "          center X coordinate or \"auto\"";
    choices[++k] = "          center Y coordinate or \"auto\"";
    k = k - 3;
    for (int i = 0; i < 3; i++)
    {
        uvalues[++k].type = 's';
        old_inversion[i] = inversion[i];
        if (inversion[i] == AUTOINVERT)
        {
            sprintf(uvalues[k].uval.sval, "auto");
        }
        else
        {
            sprintf(uvalues[k].uval.sval, "%-1.15lg", inversion[i]);
        }
    }
    choices[++k] = "  (use fixed radius & center when zooming)";
    uvalues[k].type = '*';

    choices[++k] = "Color cycling from color (0 ... 254)";
    uvalues[k].type = 'i';
    old_rotate_lo = rotate_lo;
    uvalues[k].uval.ival = old_rotate_lo;

    choices[++k] = "              to   color (1 ... 255)";
    uvalues[k].type = 'i';
    old_rotate_hi = rotate_hi;
    uvalues[k].uval.ival = old_rotate_hi;

    int const old_help_mode = help_mode;
    help_mode = HELPYOPTS;
    {
        int i = fullscreen_prompt("Extended Options\n"
                              "(not all combinations make sense)",
                              k+1, choices, uvalues, 0, nullptr);
        help_mode = old_help_mode;
        if (i < 0)
        {
            return (-1);
        }
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    bool changed = false;

    if ((uvalues[++k].uval.ch.val != 0) != finattract)
    {
        finattract = uvalues[k].uval.ch.val != 0;
        changed = true;
    }

    potparam[0] = uvalues[++k].uval.ival;
    if (potparam[0] != old_potparam[0])
    {
        changed = true;
    }

    potparam[1] = uvalues[++k].uval.dval;
    if (potparam[0] != 0.0 && potparam[1] != old_potparam[1])
    {
        changed = true;
    }

    potparam[2] = uvalues[++k].uval.ival;
    if (potparam[0] != 0.0 && potparam[2] != old_potparam[2])
    {
        changed = true;
    }

    if ((uvalues[++k].uval.ch.val != 0) != pot16bit)
    {
        pot16bit = uvalues[k].uval.ch.val != 0;
        if (pot16bit)                   // turned it on
        {
            if (potparam[0] != 0.0)
            {
                changed = true;
            }
        }
        else // turned it off
            if (!driver_diskp())   // ditch the disk video
            {
                enddisk();
            }
            else     // keep disk video, but ditch the fraction part at end
            {
                disk16bit = false;
            }
    }

    ++k;
    usr_distest = uvalues[k].uval.Lval;
    if (usr_distest != old_usr_distest)
    {
        changed = true;
    }
    ++k;
    distestwidth = uvalues[k].uval.ival;
    if (usr_distest && distestwidth != old_distestwidth)
    {
        changed = true;
    }

    for (int i = 0; i < 3; i++)
    {
        if (uvalues[++k].uval.sval[0] == 'a' || uvalues[k].uval.sval[0] == 'A')
        {
            inversion[i] = AUTOINVERT;
        }
        else
        {
            inversion[i] = atof(uvalues[k].uval.sval);
        }
        if (old_inversion[i] != inversion[i]
                && (i == 0 || inversion[0] != 0.0))
        {
            changed = true;
        }
    }
    invert = (inversion[0] == 0.0) ? 0 : 3;
    ++k;

    rotate_lo = uvalues[++k].uval.ival;
    rotate_hi = uvalues[++k].uval.ival;
    if (rotate_lo < 0 || rotate_hi > 255 || rotate_lo > rotate_hi)
    {
        rotate_lo = old_rotate_lo;
        rotate_hi = old_rotate_hi;
    }

    return changed ? 1 : 0;
}


/*
     passes_options invoked by <p> key
*/

int passes_options()
{
    char const *choices[20];
    char const *passcalcmodes[] = {"rect", "line"};

    fullscreenvalues uvalues[25];
    int i, j, k;
    int ret;

    int old_periodicity, old_orbit_delay, old_orbit_interval;
    bool const old_keep_scrn_coords = keep_scrn_coords;
    char old_drawmode;

    ret = 0;

pass_option_restart:
    // fill up the choices (and previous values) arrays
    k = -1;

    choices[++k] = "Periodicity (0=off, <0=show, >0=on, -255..+255)";
    uvalues[k].type = 'i';
    old_periodicity = usr_periodicitycheck;
    uvalues[k].uval.ival = old_periodicity;

    choices[++k] = "Orbit delay (0 = none)";
    uvalues[k].type = 'i';
    old_orbit_delay = orbit_delay;
    uvalues[k].uval.ival = old_orbit_delay;

    choices[++k] = "Orbit interval (1 ... 255)";
    uvalues[k].type = 'i';
    old_orbit_interval = (int)orbit_interval;
    uvalues[k].uval.ival = old_orbit_interval;

    choices[++k] = "Maintain screen coordinates";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = keep_scrn_coords ? 1 : 0;

    choices[++k] = "Orbit pass shape (rect, line)";
    //   choices[++k] = "Orbit pass shape (rect,line,func)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 5;
    uvalues[k].uval.ch.llen = sizeof(passcalcmodes)/sizeof(*passcalcmodes);
    uvalues[k].uval.ch.list = passcalcmodes;
    uvalues[k].uval.ch.val = (drawmode == 'r') ? 0
                             : (drawmode == 'l') ? 1
                             :   /* function */    2;
    old_drawmode = drawmode;

    int const old_help_mode = help_mode;
    help_mode = HELPPOPTS;
    i = fullscreen_prompt("Passes Options\n"
                          "(not all combinations make sense)\n"
                          "(Press " FK_F2 " for corner parameters)\n"
                          "(Press " FK_F6 " for calculation parameters)", k+1, choices, uvalues, 0x44, nullptr);
    help_mode = old_help_mode;
    if (i < 0)
    {
        return (-1);
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    j = 0;   // return code

    usr_periodicitycheck = uvalues[++k].uval.ival;
    if (usr_periodicitycheck > 255)
    {
        usr_periodicitycheck = 255;
    }
    if (usr_periodicitycheck < -255)
    {
        usr_periodicitycheck = -255;
    }
    if (usr_periodicitycheck != old_periodicity)
    {
        j = 1;
    }


    orbit_delay = uvalues[++k].uval.ival;
    if (orbit_delay != old_orbit_delay)
    {
        j = 1;
    }


    orbit_interval = uvalues[++k].uval.ival;
    if (orbit_interval > 255)
    {
        orbit_interval = 255;
    }
    if (orbit_interval < 1)
    {
        orbit_interval = 1;
    }
    if (orbit_interval != old_orbit_interval)
    {
        j = 1;
    }

    keep_scrn_coords = uvalues[++k].uval.ch.val != 0;
    if (keep_scrn_coords != old_keep_scrn_coords)
    {
        j = 1;
    }
    if (!keep_scrn_coords)
    {
        set_orbit_corners = false;
    }

    {
        int tmp = uvalues[++k].uval.ch.val;
        switch (tmp)
        {
        default:
        case 0:
            drawmode = 'r';
            break;
        case 1:
            drawmode = 'l';
            break;
        case 2:
            drawmode = 'f';
            break;
        }
    }
    if (drawmode != old_drawmode)
    {
        j = 1;
    }

    if (i == FIK_F2)
    {
        if (get_screen_corners() > 0)
        {
            ret = 1;
        }
        if (j)
        {
            ret = 1;
        }
        goto pass_option_restart;
    }

    if (i == FIK_F6)
    {
        if (get_corners() > 0)
        {
            ret = 1;
        }
        if (j)
        {
            ret = 1;
        }
        goto pass_option_restart;
    }

    return (j + ret);
}


// for videomodes added new options "virtual x/y" that change "sx/ydots"
// for diskmode changed "viewx/ydots" to "virtual x/y" that do as above
// (since for diskmode they were updated by x/ydots that should be the
// same as sx/ydots for that mode)
// g_video_table and g_video_entry are now updated even for non-disk modes

// ---------------------------------------------------------------------
/*
    get_view_params() is called from FRACTINT.C whenever the 'v' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  minor variable changed.  No need to re-generate the image.
         1  View changed.  Re-generate the image.
*/

int get_view_params()
{
    char const *choices[16];
    fullscreenvalues uvalues[25];
    int i, k;
    float old_viewreduction, old_aspectratio;
    int old_viewxdots, old_viewydots, old_sxdots, old_sydots;
    int xmax, ymax;
    char dim1[50];
    char dim2[50];

    driver_get_max_screen(&xmax, &ymax);

    bool const old_viewwindow    = viewwindow;
    old_viewreduction = viewreduction;
    old_aspectratio   = finalaspectratio;
    old_viewxdots     = viewxdots;
    old_viewydots     = viewydots;
    old_sxdots        = sxdots;
    old_sydots        = sydots;

get_view_restart:
    // fill up the previous values arrays
    k = -1;

    if (!driver_diskp())
    {
        choices[++k] = "Preview display? (no for full screen)";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = viewwindow ? 1 : 0;

        choices[++k] = "Auto window size reduction factor";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = viewreduction;

        choices[++k] = "Final media overall aspect ratio, y/x";
        uvalues[k].type = 'f';
        uvalues[k].uval.dval = finalaspectratio;

        choices[++k] = "Crop starting coordinates to new aspect ratio?";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = viewcrop ? 1 : 0;

        choices[++k] = "Explicit size x pixels (0 for auto size)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = viewxdots;

        choices[++k] = "              y pixels (0 to base on aspect ratio)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = viewydots;
    }

    choices[++k] = "";
    uvalues[k].type = '*';

    choices[++k] = "Virtual screen total x pixels";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = sxdots;

    choices[++k] = driver_diskp() ?
                   "                     y pixels" :
                   "                     y pixels (0: by aspect ratio)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = sydots;

    choices[++k] = "Keep aspect? (cuts both x & y when either too big)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = video_cutboth ? 1 : 0;

    {
        char const *scrolltypes[] = {"fixed", "relaxed"};
        choices[++k] = "Zoombox scrolling (f[ixed], r[elaxed])";
        uvalues[k].type = 'l';
        uvalues[k].uval.ch.vlen = 7;
        uvalues[k].uval.ch.llen = sizeof(scrolltypes)/sizeof(*scrolltypes);
        uvalues[k].uval.ch.list = scrolltypes;
        uvalues[k].uval.ch.val = zscroll ? 1 : 0;
    }

    choices[++k] = "";
    uvalues[k].type = '*';

    sprintf(dim1, "Video memory limits: (for y = %4d) x <= %d", ymax,  xmax);
    choices[++k] = dim1;
    uvalues[k].type = '*';

    sprintf(dim2, "                     (for x = %4d) y <= %d", xmax, ymax);
    choices[++k] = dim2;
    uvalues[k].type = '*';

    choices[++k] = "";
    uvalues[k].type = '*';

    if (!driver_diskp())
    {
        choices[++k] = "Press F4 to reset view parameters to defaults.";
        uvalues[k].type = '*';
    }

    int const old_help_mode = help_mode;     // this prevents HELP from activating
    help_mode = HELPVIEW;
    i = fullscreen_prompt("View Window Options", k+1, choices, uvalues, 16, nullptr);
    help_mode = old_help_mode;     // re-enable HELP
    if (i < 0)
    {
        return -1;
    }

    if (i == FIK_F4 && !driver_diskp())
    {
        viewwindow = false;
        viewxdots = 0;
        viewydots = 0;
        viewreduction = 4.2F;
        viewcrop = true;
        finalaspectratio = screenaspect;
        sxdots = old_sxdots;
        sydots = old_sydots;
        video_cutboth = true;
        zscroll = true;
        goto get_view_restart;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    if (!driver_diskp())
    {
        viewwindow = uvalues[++k].uval.ch.val != 0;
        viewreduction = (float) uvalues[++k].uval.dval;
        finalaspectratio = (float) uvalues[++k].uval.dval;
        viewcrop = uvalues[++k].uval.ch.val != 0;
        viewxdots = uvalues[++k].uval.ival;
        viewydots = uvalues[++k].uval.ival;
    }

    ++k;

    sxdots = uvalues[++k].uval.ival;
    sydots = uvalues[++k].uval.ival;
    video_cutboth = uvalues[++k].uval.ch.val != 0;
    zscroll = uvalues[++k].uval.ch.val != 0;

    if ((xmax != -1) && (sxdots > xmax))
    {
        sxdots = (int) xmax;
    }
    if (sxdots < 2)
    {
        sxdots = 2;
    }
    if (sydots == 0) // auto by aspect ratio request
    {
        if (finalaspectratio == 0.0)
        {
            finalaspectratio = (viewwindow && viewxdots != 0 && viewydots != 0) ?
                               ((float) viewydots)/((float) viewxdots) : old_aspectratio;
        }
        sydots = (int)(finalaspectratio*sxdots + 0.5);
    }
    if ((ymax != -1) && (sydots > ymax))
    {
        sydots = ymax;
    }
    if (sydots < 2)
    {
        sydots = 2;
    }

    if (driver_diskp())
    {
        g_video_entry.xdots = sxdots;
        g_video_entry.ydots = sydots;
        memcpy(&g_video_table[g_adapter], &g_video_entry, sizeof(g_video_entry));
        if (finalaspectratio == 0.0)
        {
            finalaspectratio = ((float) sydots)/((float) sxdots);
        }
    }

    if (viewxdots != 0 && viewydots != 0 && viewwindow && finalaspectratio == 0.0)
    {
        finalaspectratio = ((float) viewydots)/((float) viewxdots);
    }
    else if (finalaspectratio == 0.0 && (viewxdots == 0 || viewydots == 0))
    {
        finalaspectratio = old_aspectratio;
    }

    if (finalaspectratio != old_aspectratio && viewcrop)
    {
        aspectratio_crop(old_aspectratio, finalaspectratio);
    }

    return (viewwindow != old_viewwindow
            || sxdots != old_sxdots || sydots != old_sydots
            || (viewwindow
                && (viewreduction != old_viewreduction
                    || finalaspectratio != old_aspectratio
                    || viewxdots != old_viewxdots
                    || (viewydots != old_viewydots && viewxdots)))) ? 1 : 0;
}

/*
    get_cmd_string() is called from FRACTINT.C whenever the 'g' key
    is pressed.  Return codes are:
        -1  routine was ESCAPEd - no need to re-generate the image.
         0  parameter changed, no need to regenerate
        >0  parameter changed, regenerate
*/

int get_cmd_string()
{
    int i;
    static char cmdbuf[61];

    int const old_help_mode = help_mode;
    help_mode = HELPCOMMANDS;
    i = field_prompt("Enter command string to use.", nullptr, cmdbuf, 60, nullptr);
    help_mode = old_help_mode;
    if (i >= 0 && cmdbuf[0] != 0)
    {
        i = cmdarg(cmdbuf, cmd_file::AT_AFTER_STARTUP);
        if (g_debug_flag == debug_flags::write_formula_debug_information)
        {
            backwards_v18();
            backwards_v19();
            backwards_v20();
        }
    }

    return (i);
}


// ---------------------------------------------------------------------

int Distribution = 30, Offset = 0, Slope = 25;
long con;


double starfield_values[4] =
{
    30.0, 100.0, 5.0, 0.0
};

std::string const GreyFile
{"altern.map"
};

int starfield()
{   
    int c;
    g_busy = true;
    if (starfield_values[0] <   1.0)
    {
        starfield_values[0] =   1.0;
    }
    if (starfield_values[0] > 100.0)
    {
        starfield_values[0] = 100.0;
    }
    if (starfield_values[1] <   1.0)
    {
        starfield_values[1] =   1.0;
    }
    if (starfield_values[1] > 100.0)
    {
        starfield_values[1] = 100.0;
    }
    if (starfield_values[2] <   1.0)
    {
        starfield_values[2] =   1.0;
    }
    if (starfield_values[2] > 100.0)
    {
        starfield_values[2] = 100.0;
    }

    Distribution = (int)(starfield_values[0]);
    con  = (long)(((starfield_values[1]) / 100.0) * (1L << 16));
    Slope = (int)(starfield_values[2]);

    if (ValidateLuts(GreyFile.c_str()))
    {
        stopmsg(STOPMSG_NONE, "Unable to load ALTERN.MAP");
        g_busy = false;
        return (-1);
    }
    spindac(0, 1);                 // load it, but don't spin
    for (row = 0; row < ydots; row++)
    {
        for (col = 0; col < xdots; col++)
        {
            if (driver_key_pressed())
            {
                driver_buzzer(buzzer_codes::INTERRUPT);
                g_busy = false;
                return (1);
            }
            c = getcolor(col, row);
            if (c == inside)
            {
                c = g_colors-1;
            }
            putcolor(col, row, GausianNumber(c, g_colors));
        }
    }
    driver_buzzer(buzzer_codes::COMPLETE);
    g_busy = false;
    return (0);
}

int get_starfield_params()
{
    fullscreenvalues uvalues[3];
    char const *starfield_prompts[3] =
    {
        "Star Density in Pixels per Star",
        "Percent Clumpiness",
        "Ratio of Dim stars to Bright"
    };

    if (g_colors < 255)
    {
        stopmsg(STOPMSG_NONE, "starfield requires 256 color mode");
        return (-1);
    }
    for (int i = 0; i < 3; i++)
    {
        uvalues[i].uval.dval = starfield_values[i];
        uvalues[i].type = 'f';
    }
    driver_stack_screen();
    int const old_help_mode = help_mode;
    help_mode = HELPSTARFLD;
    int const choice = fullscreen_prompt("Starfield Parameters", 3, starfield_prompts, uvalues, 0, nullptr);
    help_mode = old_help_mode;
    driver_unstack_screen();
    if (choice < 0)
    {
        return (-1);
    }
    for (int i = 0; i < 3; i++)
    {
        starfield_values[i] = uvalues[i].uval.dval;
    }

    return (0);
}

static char const *masks[] = {"*.pot", "*.gif"};

int get_rds_params()
{
    char rds6[60];
    char const *stereobars[] = {"none", "middle", "top"};
    fullscreenvalues uvalues[7];
    char const *rds_prompts[7] =
    {
        "Depth Effect (negative reverses front and back)",
        "Image width in inches",
        "Use grayscale value for depth? (if \"no\" uses color number)",
        "Calibration bars",
        "Use image map? (if \"no\" uses random dots)",
        "  If yes, use current image map name? (see below)",
        rds6
    };
    int ret;
    static char reuse = 0;
    driver_stack_screen();
    while (1)
    {
        ret = 0;

        int k = 0;
        uvalues[k].uval.ival = g_auto_stereo_depth;
        uvalues[k++].type = 'i';

        uvalues[k].uval.dval = g_auto_stereo_width;
        uvalues[k++].type = 'f';

        uvalues[k].uval.ch.val = grayflag ? 1 : 0;
        uvalues[k++].type = 'y';

        uvalues[k].type = 'l';
        uvalues[k].uval.ch.list = stereobars;
        uvalues[k].uval.ch.vlen = 6;
        uvalues[k].uval.ch.llen = 3;
        uvalues[k++].uval.ch.val  = g_calibrate;

        uvalues[k].uval.ch.val = image_map ? 1 : 0;
        uvalues[k++].type = 'y';


        if (!stereomapname.empty() && image_map)
        {
            uvalues[k].uval.ch.val = reuse;
            uvalues[k++].type = 'y';

            uvalues[k++].type = '*';
            for (auto & elem : rds6)
            {
                elem = ' ';

            }
            auto p = stereomapname.find(SLASHC);
            if (p == std::string::npos ||
                    (int) stereomapname.length() < sizeof(rds6)-2)
            {
                p = 0;
            }
            else
            {
                p++;
            }
            // center file name
            rds6[(sizeof(rds6)-(int) (stereomapname.length() - p)+2)/2] = 0;
            strcat(rds6, "[");
            strcat(rds6, &stereomapname.c_str()[p]);
            strcat(rds6, "]");
        }
        else
        {
            stereomapname.clear();
        }
        int const old_help_mode = help_mode;
        help_mode = HELPRDS;
        int const choice = fullscreen_prompt("Random Dot Stereogram Parameters", k, rds_prompts, uvalues, 0, nullptr);
        help_mode = old_help_mode;
        if (choice < 0)
        {
            ret = -1;
            break;
        }
        else
        {
            k = 0;
            g_auto_stereo_depth = uvalues[k++].uval.ival;
            g_auto_stereo_width = uvalues[k++].uval.dval;
            grayflag         = uvalues[k++].uval.ch.val != 0;
            g_calibrate        = (char)uvalues[k++].uval.ch.val;
            image_map        = uvalues[k++].uval.ch.val != 0;
            if (!stereomapname.empty() && image_map)
            {
                reuse         = (char)uvalues[k++].uval.ch.val;
            }
            else
            {
                reuse = 0;
            }
            if (image_map && !reuse)
            {
                if (getafilename("Select an Imagemap File", masks[1], stereomapname))
                {
                    continue;
                }
            }
        }
        break;
    }
    driver_unstack_screen();
    return (ret);
}

int get_a_number(double *x, double *y)
{
    char const *choices[2];

    fullscreenvalues uvalues[2];
    int i, k;

    driver_stack_screen();

    // fill up the previous values arrays
    k = -1;

    choices[++k] = "X coordinate at cursor";
    uvalues[k].type = 'd';
    uvalues[k].uval.dval = *x;

    choices[++k] = "Y coordinate at cursor";
    uvalues[k].type = 'd';
    uvalues[k].uval.dval = *y;

    i = fullscreen_prompt("Set Cursor Coordinates", k+1, choices, uvalues, 25, nullptr);
    if (i < 0)
    {
        driver_unstack_screen();
        return (-1);
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    *x = uvalues[++k].uval.dval;
    *y = uvalues[++k].uval.dval;

    driver_unstack_screen();
    return (i);
}

// ---------------------------------------------------------------------

int get_commands()              // execute commands from file
{
    int ret;
    FILE *parmfile;
    ret = 0;
    int const old_help_mode = help_mode;
    help_mode = HELPPARMFILE;
    long point = get_file_entry(GETPARM, "Parameter Set", commandmask, g_command_file, g_command_name);
    if (point >= 0 && (parmfile = fopen(g_command_file.c_str(), "rb")) != nullptr)
    {
        fseek(parmfile, point, SEEK_SET);
        ret = load_commands(parmfile);
    }
    help_mode = old_help_mode;
    return (ret);
}

// ---------------------------------------------------------------------

void goodbye()                  // we done.  Bail out
{
    end_resume();
    ReleaseParamBox();
    if (!ifs_defn.empty())
    {
        ifs_defn.clear();
    }
    free_grid_pointers();
    free_ant_storage();
    enddisk();
    discardgraphics();
    ExitCheck();
    if (!make_parameter_file)
    {
        driver_set_for_text();
    }
#if 0 && defined(XFRACT)
    UnixDone();
    printf("\n\n\n%s\n", "   Thank You for using " FRACTINT); // printf takes pointer
#endif
    if (!make_parameter_file)
    {
        driver_move_cursor(6, 0);
        discardgraphics(); // if any emm/xmm tied up there, release it
    }
    stopslideshow();
    end_help();
    int ret = 0;
    if (init_batch == batch_modes::BAILOUT_ERROR_NO_SAVE) // exit with error code for batch file
    {
        ret = 2;
    }
    else if (init_batch == batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE)
    {
        ret = 1;
    }
    close_drivers();
#if defined(_WIN32)
    _CrtDumpMemoryLeaks();
#endif
    exit(ret);
}


// ---------------------------------------------------------------------

#if !defined(_WIN32)
#ifdef XFRACT
static char searchdir[FILE_MAX_DIR];
static char searchname[FILE_MAX_PATH];
static char searchext[FILE_MAX_EXT];
static DIR *currdir = nullptr;
#endif
int  fr_findfirst(char const *path)       // Find 1st file (or subdir) meeting path/filespec
{
#ifndef XFRACT
    union REGS regs;
    regs.h.ah = 0x1A;             // Set DTA to filedata
    regs.x.dx = (unsigned)&DTA;
    intdos(&regs, &regs);
    regs.h.ah = 0x4E;             // Find 1st file meeting path
    regs.x.dx = (unsigned)path;
    regs.x.cx = FILEATTR;
    intdos(&regs, &regs);
    return (regs.x.ax);           // Return error code
#else
    if (currdir != nullptr)
    {
        closedir(currdir);
        currdir = nullptr;
    }
    splitpath(path, nullptr, searchdir, searchname, searchext);
    if (searchdir[0] == '\0')
    {
        currdir = opendir(".");
    }
    else
    {
        currdir = opendir(searchdir);
    }
    if (currdir == nullptr)
    {
        return -1;
    }
    else
    {
        return fr_findnext();
    }
#endif
}

int  fr_findnext()              // Find next file (or subdir) meeting above path/filespec
{
#ifndef XFRACT
    union REGS regs;
    regs.h.ah = 0x4F;             // Find next file meeting path
    regs.x.dx = (unsigned)&DTA;
    intdos(&regs, &regs);
    return (regs.x.ax);
#else
#ifdef DIRENT
    struct dirent *dirEntry;
#else
    struct direct *dirEntry;
#endif
    struct stat sbuf;
    char thisname[FILE_MAX_PATH];
    char tmpname[FILE_MAX_PATH];
    char thisext[FILE_MAX_EXT];
    while (1)
    {
        dirEntry = readdir(currdir);
        if (dirEntry == nullptr)
        {
            closedir(currdir);
            currdir = nullptr;
            return -1;
        }
        else if (dirEntry->d_ino != 0)
        {
            splitpath(dirEntry->d_name, nullptr, nullptr, thisname, thisext);
            strncpy(DTA.filename, dirEntry->d_name, 13);
            DTA.filename[12] = '\0';
            strcpy(tmpname, searchdir);
            strcat(tmpname, dirEntry->d_name);
            stat(tmpname, &sbuf);
            DTA.size = sbuf.st_size;
            if ((sbuf.st_mode&S_IFMT) == S_IFREG &&
                    (searchname[0] == '*' || strcmp(searchname, thisname) == 0) &&
                    (searchext[0] == '*' || strcmp(searchext, thisext) == 0))
            {
                DTA.attribute = 0;
                return 0;
            }
            else if (((sbuf.st_mode&S_IFMT) == S_IFDIR) &&
                     ((searchname[0] == '*' || searchext[0] == '*') ||
                      (strcmp(searchname, thisname) == 0)))
            {
                DTA.attribute = SUBDIR;
                return 0;
            }
        }
    }
#endif
}
#endif // !_WIN32

struct CHOICE
{
    char name[13];
    char full_name[FILE_MAX_PATH];
    char type;
};

int lccompare(VOIDPTR arg1, VOIDPTR arg2) // for sort
{
    char **choice1 = (char **) arg1;
    char **choice2 = (char **) arg2;

    return stricmp(*choice1, *choice2);
}


static int speedstate;
bool getafilename(char const *hdg, char const *file_template, char *flname)
{
    char user_file_template[FILE_MAX_PATH] = { 0 };
    int rds;  // if getting an RDS image map
    char instr[80];
    int masklen;
    char filename[FILE_MAX_PATH]; // 13 is big enough for Fractint, but not Xfractint
    char speedstr[81];
    char tmpmask[FILE_MAX_PATH];   // used to locate next file in list
    char old_flname[FILE_MAX_PATH];
    int out;
    int retried;
    // Only the first 13 characters of file names are displayed...
    CHOICE storage[MAXNUMFILES];
    CHOICE *choices[MAXNUMFILES];
    int attributes[MAXNUMFILES];
    int filecount;   // how many files
    int dircount;    // how many directories
    bool notroot;     // not the root directory
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];

    static int numtemplates = 1;
    static bool dosort = true;

    rds = (stereomapname == flname) ? 1 : 0;
    for (int i = 0; i < MAXNUMFILES; i++)
    {
        attributes[i] = 1;
        choices[i] = &storage[i];
    }
    // save filename
    strcpy(old_flname, flname);

restart:  // return here if template or directory changes
    tmpmask[0] = 0;
    if (flname[0] == 0)
    {
        strcpy(flname, DOTSLASH);
    }
    splitpath(flname , drive, dir, fname, ext);
    makepath(filename, ""   , "" , fname, ext);
    retried = 0;

retry_dir:
    if (dir[0] == 0)
    {
        strcpy(dir, ".");
    }
    expand_dirname(dir, drive);
    makepath(tmpmask, drive, dir, "", "");
    fix_dirname(tmpmask);
    if (retried == 0 && strcmp(dir, SLASH) && strcmp(dir, DOTSLASH))
    {
        int j = (int) strlen(tmpmask) - 1;
        tmpmask[j] = 0; // strip trailing backslash
        if (strchr(tmpmask, '*') || strchr(tmpmask, '?')
                || fr_findfirst(tmpmask) != 0
                || (DTA.attribute & SUBDIR) == 0)
        {
            strcpy(dir, DOTSLASH);
            ++retried;
            goto retry_dir;
        }
        tmpmask[j] = SLASHC;
    }
    if (file_template[0])
    {
        numtemplates = 1;
        splitpath(file_template, nullptr, nullptr, fname, ext);
    }
    else
    {
        numtemplates = sizeof(masks)/sizeof(masks[0]);
    }
    filecount = -1;
    dircount  = 0;
    notroot   = false;
    masklen = (int) strlen(tmpmask);
    strcat(tmpmask, "*.*");
    out = fr_findfirst(tmpmask);
    while (out == 0 && filecount < MAXNUMFILES)
    {
        if ((DTA.attribute & SUBDIR) && strcmp(DTA.filename, "."))
        {
            if (strcmp(DTA.filename, ".."))
            {
                strcat(DTA.filename, SLASH);
            }
            strncpy(choices[++filecount]->name, DTA.filename, 13);
            choices[filecount]->name[12] = 0;
            choices[filecount]->type = 1;
            strcpy(choices[filecount]->full_name, DTA.filename);
            dircount++;
            if (strcmp(DTA.filename, "..") == 0)
            {
                notroot = true;
            }
        }
        out = fr_findnext();
    }
    tmpmask[masklen] = 0;
    if (file_template[0])
    {
        makepath(tmpmask, drive, dir, fname, ext);
    }
    int j = 0;
    do
    {
        if (numtemplates > 1)
        {
            strcpy(&(tmpmask[masklen]), masks[j]);
        }
        out = fr_findfirst(tmpmask);
        while (out == 0 && filecount < MAXNUMFILES)
        {
            if (!(DTA.attribute & SUBDIR))
            {
                if (rds)
                {
                    putstringcenter(2, 0, 80, C_GENERAL_INPUT, DTA.filename);

                    splitpath(DTA.filename, nullptr, nullptr, fname, ext);
                    // just using speedstr as a handy buffer
                    makepath(speedstr, drive, dir, fname, ext);
                    strncpy(choices[++filecount]->name, DTA.filename, 13);
                    choices[filecount]->type = 0;
                }
                else
                {
                    strncpy(choices[++filecount]->name, DTA.filename, 13);
                    choices[filecount]->type = 0;
                    strcpy(choices[filecount]->full_name, DTA.filename);
                }
            }
            out = fr_findnext();
        }
    }
    while (++j < numtemplates);
    if (++filecount == 0)
    {
        strcpy(choices[filecount]->name, "*nofiles*");
        choices[filecount]->type = 0;
        ++filecount;
    }

    strcpy(instr, "Press " FK_F6 " for default directory, " FK_F4 " to toggle sort ");
    if (dosort)
    {
        strcat(instr, "off");
        shell_sort(&choices, filecount, sizeof(CHOICE *), lccompare); // sort file list
    }
    else
    {
        strcat(instr, "on");
    }
    if (!notroot && dir[0] && dir[0] != SLASHC) // must be in root directory
    {
        splitpath(tmpmask, drive, dir, fname, ext);
        strcpy(dir, SLASH);
        makepath(tmpmask, drive, dir, fname, ext);
    }
    if (numtemplates > 1)
    {
        strcat(tmpmask, " ");
        strcat(tmpmask, masks[0]);
    }

    std::string const heading{std::string{hdg} + "\n"
        + "Template: " + tmpmask};
    strcpy(speedstr, filename);
    int i;
    if (speedstr[0] == 0)
    {
        for (i = 0; i < filecount; i++) // find first file
        {
            if (choices[i]->type == 0)
            {
                break;
            }
        }
        if (i >= filecount)
        {
            i = 0;
        }
    }

    i = fullscreen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
        heading.c_str(), nullptr, instr, filecount, (char const **) choices,
        attributes, 5, 99, 12, i, nullptr, speedstr, filename_speedstr, check_f6_key);
    if (i == -FIK_F4)
    {
        dosort = !dosort;
        goto restart;
    }
    if (i == -FIK_F6)
    {
        static int lastdir = 0;
        if (lastdir == 0)
        {
            strcpy(dir, fract_dir1);
        }
        else
        {
            strcpy(dir, fract_dir2);
        }
        fix_dirname(dir);
        makepath(flname, drive, dir, "", "");
        lastdir = 1 - lastdir;
        goto restart;
    }
    if (i < 0)
    {
        // restore filename
        strcpy(flname, old_flname);
        return true;
    }
    if (speedstr[0] == 0 || speedstate == MATCHING)
    {
        if (choices[i]->type)
        {
            if (strcmp(choices[i]->name, "..") == 0) // go up a directory
            {
                if (strcmp(dir, DOTSLASH) == 0)
                {
                    strcpy(dir, DOTDOTSLASH);
                }
                else
                {
                    char *s = strrchr(dir, SLASHC);
                    if (s != nullptr) // trailing slash
                    {
                        *s = 0;
                        s = strrchr(dir, SLASHC);
                        if (s != nullptr)
                        {
                            *(s + 1) = 0;
                        }
                    }
                }
            }
            else  // go down a directory
            {
                strcat(dir, choices[i]->full_name);
            }
            fix_dirname(dir);
            makepath(flname, drive, dir, "", "");
            goto restart;
        }
        splitpath(choices[i]->full_name, nullptr, nullptr, fname, ext);
        makepath(flname, drive, dir, fname, ext);
    }
    else
    {
        if (speedstate == SEARCHPATH
                && strchr(speedstr, '*') == nullptr && strchr(speedstr, '?') == nullptr
                && ((fr_findfirst(speedstr) == 0
                     && (DTA.attribute & SUBDIR))|| strcmp(speedstr, SLASH) == 0)) // it is a directory
        {
            speedstate = TEMPLATE;
        }

        if (speedstate == TEMPLATE)
        {
            /* extract from tempstr the pathname and template information,
                being careful not to overwrite drive and directory if not
                newly specified */
            char drive1[FILE_MAX_DRIVE];
            char dir1[FILE_MAX_DIR];
            char fname1[FILE_MAX_FNAME];
            char ext1[FILE_MAX_EXT];
            splitpath(speedstr, drive1, dir1, fname1, ext1);
            if (drive1[0])
            {
                strcpy(drive, drive1);
            }
            if (dir1[0])
            {
                strcpy(dir, dir1);
            }
            makepath(flname, drive, dir, fname1, ext1);
            if (strchr(fname1, '*') || strchr(fname1, '?') ||
                    strchr(ext1,   '*') || strchr(ext1,   '?'))
            {
                makepath(user_file_template, "", "", fname1, ext1);
                file_template = user_file_template;
            }
            else if (isadirectory(flname))
            {
                fix_dirname(flname);
            }
            goto restart;
        }
        else // speedstate == SEARCHPATH
        {
            char fullpath[FILE_MAX_DIR];
            findpath(speedstr, fullpath);
            if (fullpath[0])
            {
                strcpy(flname, fullpath);
            }
            else
            {
                // failed, make diagnostic useful:
                strcpy(flname, speedstr);
                if (strchr(speedstr, SLASHC) == nullptr)
                {
                    splitpath(speedstr, nullptr, nullptr, fname, ext);
                    makepath(flname, drive, dir, fname, ext);
                }
            }
        }
    }
    g_browse_name = std::string{fname} + ext;
    return false;
}

bool getafilename(char const *hdg, char const *file_template, std::string &flname)
{
    char buff[FILE_MAX_PATH];
    strncpy(buff, flname.c_str(), FILE_MAX_PATH);
    bool const result = getafilename(hdg, file_template, buff);
    flname = buff;
    return result;
}

// choice is used by other routines called by fullscreen_choice()
static int check_f6_key(int curkey, int /*choice*/)
{
    if (curkey == FIK_F6)
    {
        return 0-FIK_F6;
    }
    else if (curkey == FIK_F4)
    {
        return 0-FIK_F4;
    }
    return 0;
}

static int filename_speedstr(int row, int col, int vid,
                             char const *speedstring, int speed_match)
{
    char const *prompt;
    if (strchr(speedstring, ':')
            || strchr(speedstring, '*') || strchr(speedstring, '*')
            || strchr(speedstring, '?'))
    {
        speedstate = TEMPLATE;  // template
        prompt = "File Template";
    }
    else if (speed_match)
    {
        speedstate = SEARCHPATH; // does not match list
        prompt = "Search Path for";
    }
    else
    {
        speedstate = MATCHING;
        prompt = speed_prompt.c_str();
    }
    driver_put_string(row, col, vid, prompt);
    return ((int) strlen(prompt));
}

#ifndef XFRACT  // This routine moved to unix.c so we can use it in hc.c
int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext)
{
    int length;
    int len;
    int offset;
    char const *tmp;
    if (drive)
    {
        drive[0] = 0;
    }
    if (dir)
    {
        dir[0]   = 0;
    }
    if (fname)
    {
        fname[0] = 0;
    }
    if (ext)
    {
        ext[0]   = 0;
    }

    length = (int) strlen(file_template);
    if (length == 0)
    {
        return (0);
    }

    offset = 0;

    // get drive
    if (length >= 2)
    {
        if (file_template[1] == ':')
        {
            if (drive)
            {
                drive[0] = file_template[offset++];
                drive[1] = file_template[offset++];
                drive[2] = 0;
            }
            else
            {
                offset++;
                offset++;
            }
        }
    }

    // get dir
    if (offset < length)
    {
        tmp = strrchr(file_template, SLASHC);
        if (tmp)
        {
            tmp++;  // first character after slash
            len = (int)(tmp - (char *)&file_template[offset]);
            if (len >= 0 && len < FILE_MAX_DIR && dir)
            {
                strncpy(dir, &file_template[offset], std::min(len, FILE_MAX_DIR));
            }
            if (len < FILE_MAX_DIR && dir)
            {
                dir[len] = 0;
            }
            offset += len;
        }
    }
    else
    {
        return (0);
    }

    // get fname
    if (offset < length)
    {
        tmp = strrchr(file_template, '.');
        if (tmp < strrchr(file_template, SLASHC) || tmp < strrchr(file_template, ':'))
        {
            tmp = nullptr; // in this case the '.' must be a directory
        }
        if (tmp)
        {
            len = (int)(tmp - (char *)&file_template[offset]);
            if ((len > 0) && (offset+len < length) && fname)
            {
                strncpy(fname, &file_template[offset], std::min(len, FILE_MAX_FNAME));
                if (len < FILE_MAX_FNAME)
                {
                    fname[len] = 0;
                }
                else
                {
                    fname[FILE_MAX_FNAME-1] = 0;
                }
            }
            offset += len;
            if ((offset < length) && ext)
            {
                strncpy(ext, &file_template[offset], FILE_MAX_EXT);
                ext[FILE_MAX_EXT-1] = 0;
            }
        }
        else if ((offset < length) && fname)
        {
            strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
            fname[FILE_MAX_FNAME-1] = 0;
        }
    }
    return 0;
}
#endif

int makepath(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext)
{
    if (template_str)
    {
        *template_str = 0;
    }
    else
    {
        return (-1);
    }
#ifndef XFRACT
    if (drive)
    {
        strcpy(template_str, drive);
    }
#endif
    if (dir)
    {
        strcat(template_str, dir);
    }
    if (fname)
    {
        strcat(template_str, fname);
    }
    if (ext)
    {
        strcat(template_str, ext);
    }
    return 0;
}

// fix up directory names
void fix_dirname(char *dirname)
{
    int length = (int) strlen(dirname); // index of last character

    // make sure dirname ends with a slash
    if (length > 0)
    {
        if (dirname[length-1] == SLASHC)
        {
            return;
        }
    }
    strcat(dirname, SLASH);
}

void fix_dirname(std::string &dirname)
{
    char buff[FILE_MAX_PATH];
    strcpy(buff, dirname.c_str());
    fix_dirname(buff);
    dirname = buff;
}

static void dir_name(char *target, char const *dir, char const *name)
{
    *target = 0;
    if (*dir != 0)
    {
        strcpy(target, dir);
    }
    strcat(target, name);
}

// removes file in dir directory
int dir_remove(char const *dir, char const *filename)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return (remove(tmp));
}

// fopens file in dir directory
FILE *dir_fopen(char const *dir, char const *filename, char const *mode)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return (fopen(tmp, mode));
}

/*
   See if double value was changed by input screen. Problem is that the
   conversion from double to string and back can make small changes
   in the value, so will it twill test as "different" even though it
   is not
*/
int cmpdbl(double old, double new_val)
{
    char buf[81];
    fullscreenvalues val;

    // change the old value with the same torture the new value had
    val.type = 'd'; // most values on this screen are type d
    val.uval.dval = old;
    prompt_valuestring(buf, &val);   // convert "old" to string

    old = atof(buf);                // convert back
    return (fabs(old-new_val) < DBL_EPSILON?0:1); // zero if same
}

int get_corners()
{
    fullscreenvalues values[15];
    char const *prompts[15];
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int i, nump, prompt_ret;
    int cmag;
    double Xctr, Yctr;
    LDBL Magnification; // LDBL not really needed here, but used to match function parameters
    double Xmagfactor, Rotation, Skew;
    double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;

    bool const ousemag = usemag;
    oxxmin = xxmin;
    oxxmax = xxmax;
    oyymin = yymin;
    oyymax = yymax;
    oxx3rd = xx3rd;
    oyy3rd = yy3rd;

gc_loop:
    for (i = 0; i < 15; ++i)
    {
        values[i].type = 'd'; // most values on this screen are type d

    }
    cmag = usemag ? 1 : 0;
    if (drawmode == 'l')
    {
        cmag = 0;
    }
    cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

    nump = -1;
    if (cmag)
    {
        prompts[++nump] = "Center X";
        values[nump].uval.dval = Xctr;
        prompts[++nump] = "Center Y";
        values[nump].uval.dval = Yctr;
        prompts[++nump] = "Magnification";
        values[nump].uval.dval = (double)Magnification;
        prompts[++nump] = "X Magnification Factor";
        values[nump].uval.dval = Xmagfactor;
        prompts[++nump] = "Rotation Angle (degrees)";
        values[nump].uval.dval = Rotation;
        prompts[++nump] = "Skew Angle (degrees)";
        values[nump].uval.dval = Skew;
        prompts[++nump] = "";
        values[nump].type = '*';
        prompts[++nump] = "Press " FK_F7 " to switch to \"corners\" mode";
        values[nump].type = '*';
    }

    else
    {
        if (drawmode == 'l')
        {
            prompts[++nump] = "Left End Point";
            values[nump].type = '*';
            prompts[++nump] = xprompt;
            values[nump].uval.dval = xxmin;
            prompts[++nump] = yprompt;
            values[nump].uval.dval = yymax;
            prompts[++nump] = "Right End Point";
            values[nump].type = '*';
            prompts[++nump] = xprompt;
            values[nump].uval.dval = xxmax;
            prompts[++nump] = yprompt;
            values[nump].uval.dval = yymin;
        }
        else
        {
            prompts[++nump] = "Top-Left Corner";
            values[nump].type = '*';
            prompts[++nump] = xprompt;
            values[nump].uval.dval = xxmin;
            prompts[++nump] = yprompt;
            values[nump].uval.dval = yymax;
            prompts[++nump] = "Bottom-Right Corner";
            values[nump].type = '*';
            prompts[++nump] = xprompt;
            values[nump].uval.dval = xxmax;
            prompts[++nump] = yprompt;
            values[nump].uval.dval = yymin;
            if (xxmin == xx3rd && yymin == yy3rd)
            {
                yy3rd = 0;
                xx3rd = yy3rd;
            }
            prompts[++nump] = "Bottom-left (zeros for top-left X, bottom-right Y)";
            values[nump].type = '*';
            prompts[++nump] = xprompt;
            values[nump].uval.dval = xx3rd;
            prompts[++nump] = yprompt;
            values[nump].uval.dval = yy3rd;
            prompts[++nump] = "Press " FK_F7 " to switch to \"center-mag\" mode";
            values[nump].type = '*';
        }
    }

    prompts[++nump] = "Press " FK_F4 " to reset to type default values";
    values[nump].type = '*';

    int const old_help_mode = help_mode;
    help_mode = HELPCOORDS;
    prompt_ret = fullscreen_prompt("Image Coordinates", nump+1, prompts, values, 0x90, nullptr);
    help_mode = old_help_mode;

    if (prompt_ret < 0)
    {
        usemag = ousemag;
        xxmin = oxxmin;
        xxmax = oxxmax;
        yymin = oyymin;
        yymax = oyymax;
        xx3rd = oxx3rd;
        yy3rd = oyy3rd;
        return (-1);
    }

    if (prompt_ret == FIK_F4)
    {
        // reset to type defaults
        xxmin = curfractalspecific->xmin;
        xx3rd = xxmin;
        xxmax = curfractalspecific->xmax;
        yymin = curfractalspecific->ymin;
        yy3rd = yymin;
        yymax = curfractalspecific->ymax;
        if (viewcrop && finalaspectratio != screenaspect)
        {
            aspectratio_crop(screenaspect, finalaspectratio);
        }
        if (bf_math != bf_math_type::NONE)
        {
            fractal_floattobf();
        }
        goto gc_loop;
    }

    if (cmag)
    {
        if (cmpdbl(Xctr         , values[0].uval.dval)
                || cmpdbl(Yctr         , values[1].uval.dval)
                || cmpdbl((double)Magnification, values[2].uval.dval)
                || cmpdbl(Xmagfactor   , values[3].uval.dval)
                || cmpdbl(Rotation     , values[4].uval.dval)
                || cmpdbl(Skew         , values[5].uval.dval))
        {
            Xctr          = values[0].uval.dval;
            Yctr          = values[1].uval.dval;
            Magnification = values[2].uval.dval;
            Xmagfactor    = values[3].uval.dval;
            Rotation      = values[4].uval.dval;
            Skew          = values[5].uval.dval;
            if (Xmagfactor == 0)
            {
                Xmagfactor = 1;
            }
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        }
    }

    else
    {
        if (drawmode == 'l')
        {
            nump = 1;
            xxmin = values[nump++].uval.dval;
            yymax = values[nump++].uval.dval;
            nump++;
            xxmax = values[nump++].uval.dval;
            yymin = values[nump++].uval.dval;
        }
        else
        {
            nump = 1;
            xxmin = values[nump++].uval.dval;
            yymax = values[nump++].uval.dval;
            nump++;
            xxmax = values[nump++].uval.dval;
            yymin = values[nump++].uval.dval;
            nump++;
            xx3rd = values[nump++].uval.dval;
            yy3rd = values[nump++].uval.dval;
            if (xx3rd == 0 && yy3rd == 0)
            {
                xx3rd = xxmin;
                yy3rd = yymin;
            }
        }
    }

    if (prompt_ret == FIK_F7 && drawmode != 'l')
    {
        // toggle corners/center-mag mode
        if (!usemag)
        {
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            usemag = true;
        }
        else
        {
            usemag = false;
        }
        goto gc_loop;
    }

    if (!cmpdbl(oxxmin, xxmin) && !cmpdbl(oxxmax, xxmax) && !cmpdbl(oyymin, yymin) &&
            !cmpdbl(oyymax, yymax) && !cmpdbl(oxx3rd, xx3rd) && !cmpdbl(oyy3rd, yy3rd))
    {
        // no change, restore values to avoid drift
        xxmin = oxxmin;
        xxmax = oxxmax;
        yymin = oyymin;
        yymax = oyymax;
        xx3rd = oxx3rd;
        yy3rd = oyy3rd;
        return 0;
    }
    else
    {
        return (1);
    }
}

static int get_screen_corners()
{
    fullscreenvalues values[15];
    char const *prompts[15];
    char xprompt[] = "          X";
    char yprompt[] = "          Y";
    int nump, prompt_ret;
    int cmag;
    double Xctr, Yctr;
    LDBL Magnification; // LDBL not really needed here, but used to match function parameters
    double Xmagfactor, Rotation, Skew;
    double oxxmin, oxxmax, oyymin, oyymax, oxx3rd, oyy3rd;
    double svxxmin, svxxmax, svyymin, svyymax, svxx3rd, svyy3rd;

    bool const ousemag = usemag;

    svxxmin = xxmin;  // save these for later since cvtcorners modifies them
    svxxmax = xxmax;  // and we need to set them for cvtcentermag to work
    svxx3rd = xx3rd;
    svyymin = yymin;
    svyymax = yymax;
    svyy3rd = yy3rd;

    if (!set_orbit_corners && !keep_scrn_coords)
    {
        oxmin = xxmin;
        oxmax = xxmax;
        ox3rd = xx3rd;
        oymin = yymin;
        oymax = yymax;
        oy3rd = yy3rd;
    }

    oxxmin = oxmin;
    oxxmax = oxmax;
    oyymin = oymin;
    oyymax = oymax;
    oxx3rd = ox3rd;
    oyy3rd = oy3rd;

    xxmin = oxmin;
    xxmax = oxmax;
    yymin = oymin;
    yymax = oymax;
    xx3rd = ox3rd;
    yy3rd = oy3rd;

gsc_loop:
    for (auto & value : values)
    {
        value.type = 'd'; // most values on this screen are type d

    }
    cmag = usemag ? 1 : 0;
    cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);

    nump = -1;
    if (cmag)
    {
        prompts[++nump] = "Center X";
        values[nump].uval.dval = Xctr;
        prompts[++nump] = "Center Y";
        values[nump].uval.dval = Yctr;
        prompts[++nump] = "Magnification";
        values[nump].uval.dval = (double)Magnification;
        prompts[++nump] = "X Magnification Factor";
        values[nump].uval.dval = Xmagfactor;
        prompts[++nump] = "Rotation Angle (degrees)";
        values[nump].uval.dval = Rotation;
        prompts[++nump] = "Skew Angle (degrees)";
        values[nump].uval.dval = Skew;
        prompts[++nump] = "";
        values[nump].type = '*';
        prompts[++nump] = "Press " FK_F7 " to switch to \"corners\" mode";
        values[nump].type = '*';
    }
    else
    {
        prompts[++nump] = "Top-Left Corner";
        values[nump].type = '*';
        prompts[++nump] = xprompt;
        values[nump].uval.dval = oxmin;
        prompts[++nump] = yprompt;
        values[nump].uval.dval = oymax;
        prompts[++nump] = "Bottom-Right Corner";
        values[nump].type = '*';
        prompts[++nump] = xprompt;
        values[nump].uval.dval = oxmax;
        prompts[++nump] = yprompt;
        values[nump].uval.dval = oymin;
        if (oxmin == ox3rd && oymin == oy3rd)
        {
            oy3rd = 0;
            ox3rd = oy3rd;
        }
        prompts[++nump] = "Bottom-left (zeros for top-left X, bottom-right Y)";
        values[nump].type = '*';
        prompts[++nump] = xprompt;
        values[nump].uval.dval = ox3rd;
        prompts[++nump] = yprompt;
        values[nump].uval.dval = oy3rd;
        prompts[++nump] = "Press " FK_F7 " to switch to \"center-mag\" mode";
        values[nump].type = '*';
    }

    prompts[++nump] = "Press " FK_F4 " to reset to type default values";
    values[nump].type = '*';

    int const old_help_mode = help_mode;
    help_mode = HELPSCRNCOORDS;
    prompt_ret = fullscreen_prompt("Screen Coordinates", nump+1, prompts, values, 0x90, nullptr);
    help_mode = old_help_mode;

    if (prompt_ret < 0)
    {
        usemag = ousemag;
        oxmin = oxxmin;
        oxmax = oxxmax;
        oymin = oyymin;
        oymax = oyymax;
        ox3rd = oxx3rd;
        oy3rd = oyy3rd;
        // restore corners
        xxmin = svxxmin;
        xxmax = svxxmax;
        yymin = svyymin;
        yymax = svyymax;
        xx3rd = svxx3rd;
        yy3rd = svyy3rd;
        return (-1);
    }

    if (prompt_ret == FIK_F4)
    {
        // reset to type defaults
        oxmin = curfractalspecific->xmin;
        ox3rd = oxmin;
        oxmax = curfractalspecific->xmax;
        oymin = curfractalspecific->ymin;
        oy3rd = oymin;
        oymax = curfractalspecific->ymax;
        xxmin = oxmin;
        xxmax = oxmax;
        yymin = oymin;
        yymax = oymax;
        xx3rd = ox3rd;
        yy3rd = oy3rd;
        if (viewcrop && finalaspectratio != screenaspect)
        {
            aspectratio_crop(screenaspect, finalaspectratio);
        }

        oxmin = xxmin;
        oxmax = xxmax;
        oymin = yymin;
        oymax = yymax;
        ox3rd = xxmin;
        oy3rd = yymin;
        goto gsc_loop;
    }

    if (cmag)
    {
        if (cmpdbl(Xctr         , values[0].uval.dval)
                || cmpdbl(Yctr         , values[1].uval.dval)
                || cmpdbl((double)Magnification, values[2].uval.dval)
                || cmpdbl(Xmagfactor   , values[3].uval.dval)
                || cmpdbl(Rotation     , values[4].uval.dval)
                || cmpdbl(Skew         , values[5].uval.dval))
        {
            Xctr          = values[0].uval.dval;
            Yctr          = values[1].uval.dval;
            Magnification = values[2].uval.dval;
            Xmagfactor    = values[3].uval.dval;
            Rotation      = values[4].uval.dval;
            Skew          = values[5].uval.dval;
            if (Xmagfactor == 0)
            {
                Xmagfactor = 1;
            }
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
            // set screen corners
            oxmin = xxmin;
            oxmax = xxmax;
            oymin = yymin;
            oymax = yymax;
            ox3rd = xx3rd;
            oy3rd = yy3rd;
        }
    }
    else
    {
        nump = 1;
        oxmin = values[nump++].uval.dval;
        oymax = values[nump++].uval.dval;
        nump++;
        oxmax = values[nump++].uval.dval;
        oymin = values[nump++].uval.dval;
        nump++;
        ox3rd = values[nump++].uval.dval;
        oy3rd = values[nump++].uval.dval;
        if (ox3rd == 0 && oy3rd == 0)
        {
            ox3rd = oxmin;
            oy3rd = oymin;
        }
    }

    if (prompt_ret == FIK_F7)
    {
        // toggle corners/center-mag mode
        if (!usemag)
        {
            cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
            usemag = true;
        }
        else
        {
            usemag = false;
        }
        goto gsc_loop;
    }

    if (!cmpdbl(oxxmin, oxmin) && !cmpdbl(oxxmax, oxmax) && !cmpdbl(oyymin, oymin) &&
            !cmpdbl(oyymax, oymax) && !cmpdbl(oxx3rd, ox3rd) && !cmpdbl(oyy3rd, oy3rd))
    {
        // no change, restore values to avoid drift
        oxmin = oxxmin;
        oxmax = oxxmax;
        oymin = oyymin;
        oymax = oyymax;
        ox3rd = oxx3rd;
        oy3rd = oyy3rd;
        // restore corners
        xxmin = svxxmin;
        xxmax = svxxmax;
        yymin = svyymin;
        yymax = svyymax;
        xx3rd = svxx3rd;
        yy3rd = svyy3rd;
        return 0;
    }
    else
    {
        set_orbit_corners = true;
        keep_scrn_coords = true;
        // restore corners
        xxmin = svxxmin;
        xxmax = svxxmax;
        yymin = svyymin;
        yymax = svyymax;
        xx3rd = svxx3rd;
        yy3rd = svyy3rd;
        return (1);
    }
}

/* get browse parameters , called from fractint.c and loadfile.c
   returns 3 if anything changes.  code pinched from get_view_params */

int get_browse_params()
{
    char const *choices[10];
    fullscreenvalues uvalues[25];
    int i, k;

    bool old_auto_browse = g_auto_browse;
    bool old_browse_check_fractal_type = g_browse_check_fractal_type;
    bool old_brwscheckparms = g_browse_check_fractal_params;
    bool old_doublecaution  = confirm_file_deletes;
    int old_smallest_box_size_shown = smallest_box_size_shown;
    double old_smallest_window_display_size = smallest_window_display_size;
    std::string old_browse_mask = g_browse_mask;

get_brws_restart:
    // fill up the previous values arrays
    k = -1;

    choices[++k] = "Autobrowsing? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_auto_browse ? 1 : 0;

    choices[++k] = "Ask about GIF video mode? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_ask_video ? 1 : 0;

    choices[++k] = "Check fractal type? (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_browse_check_fractal_type ? 1 : 0;

    choices[++k] = "Check fractal parameters (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_browse_check_fractal_params ? 1 : 0;

    choices[++k] = "Confirm file deletes (y/n)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = confirm_file_deletes ? 1 : 0;

    choices[++k] = "Smallest window to display (size in pixels)";
    uvalues[k].type = 'f';
    uvalues[k].uval.dval = smallest_window_display_size;

    choices[++k] = "Smallest box size shown before crosshairs used (pix)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = smallest_box_size_shown;
    choices[++k] = "Browse search filename mask ";
    uvalues[k].type = 's';
    strcpy(uvalues[k].uval.sval, g_browse_mask.c_str());

    choices[++k] = "";
    uvalues[k].type = '*';

    choices[++k] = "Press " FK_F4 " to reset browse parameters to defaults.";
    uvalues[k].type = '*';

    int const old_help_mode = help_mode;     // this prevents HELP from activating
    help_mode = HELPBRWSPARMS;
    i = fullscreen_prompt("Browse ('L'ook) Mode Options", k+1, choices, uvalues, 16, nullptr);
    help_mode = old_help_mode;     // re-enable HELP
    if (i < 0)
    {
        return (0);
    }

    if (i == FIK_F4)
    {
        smallest_window_display_size = 6;
        g_auto_browse = false;
        g_ask_video = true;
        g_browse_check_fractal_params = true;
        g_browse_check_fractal_type = true;
        confirm_file_deletes = true;
        smallest_box_size_shown = 3;
        g_browse_mask = "*.gif";
        goto get_brws_restart;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;

    g_auto_browse = uvalues[++k].uval.ch.val != 0;
    g_ask_video = uvalues[++k].uval.ch.val != 0;
    g_browse_check_fractal_type = uvalues[++k].uval.ch.val != 0;
    g_browse_check_fractal_params = uvalues[++k].uval.ch.val != 0;
    confirm_file_deletes = uvalues[++k].uval.ch.val != 0;
    smallest_window_display_size = uvalues[++k].uval.dval;
    if (smallest_window_display_size < 0)
    {
        smallest_window_display_size = 0 ;
    }
    smallest_box_size_shown = uvalues[++k].uval.ival;
    if (smallest_box_size_shown < 1)
    {
        smallest_box_size_shown = 1;
    }
    if (smallest_box_size_shown > 10)
    {
        smallest_box_size_shown = 10;
    }

    g_browse_mask = uvalues[++k].uval.sval;

    i = 0;
    if (g_auto_browse != old_auto_browse ||
            g_browse_check_fractal_type != old_browse_check_fractal_type ||
            g_browse_check_fractal_params != old_brwscheckparms ||
            confirm_file_deletes != old_doublecaution ||
            smallest_window_display_size != old_smallest_window_display_size ||
            smallest_box_size_shown != old_smallest_box_size_shown ||
            !stricmp(g_browse_mask.c_str(), old_browse_mask.c_str()))
    {
        i = -3;
    }

    if (evolving)
    {
        // can't browse
        g_auto_browse = false;
        i = 0;
    }

    return (i);
}

// merge existing full path with new one
// attempt to detect if file or directory

#define ATFILENAME 0
#define SSTOOLSINI 1
#define ATCOMMANDINTERACTIVE 2
#define ATFILENAMESETNAME  3

#ifndef XFRACT
#include <direct.h>
#endif

// copies the proposed new filename to the fullpath variable
// does not copy directories for PAR files
// (modes AT_AFTER_STARTUP and AT_CMD_LINE_SET_NAME)
// attempts to extract directory and test for existence
// (modes AT_CMD_LINE and SSTOOLS_INI)
int merge_pathnames(char *oldfullpath, char const *filename, cmd_file mode)
{
    char newfilename[FILE_MAX_PATH];
    strcpy(newfilename, filename);
    bool isadir_error = false;
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char temp_path[FILE_MAX_PATH];

    char drive1[FILE_MAX_DRIVE];
    char dir1[FILE_MAX_DIR];
    char fname1[FILE_MAX_FNAME];
    char ext1[FILE_MAX_EXT];

    // no dot or slash so assume a file
    bool isafile = strchr(newfilename, '.') == nullptr
                   && strchr(newfilename, SLASHC) == nullptr;
    bool isadir = isadirectory(newfilename);
    if (isadir)
    {
        fix_dirname(newfilename);
    }
#ifndef XFRACT
    // if drive, colon, slash, is a directory
    if ((int) strlen(newfilename) == 3 &&
            newfilename[1] == ':' &&
            newfilename[2] == SLASHC)
    {
        isadir = true;
    }
    // if drive, colon, with no slash, is a directory
    if ((int) strlen(newfilename) == 2 &&
            newfilename[1] == ':')
    {
        newfilename[2] = SLASHC;
        newfilename[3] = 0;
        isadir = true;
    }
    // if dot, slash, '0', its the current directory, set up full path
    if (newfilename[0] == '.' &&
            newfilename[1] == SLASHC && newfilename[2] == 0)
    {
        temp_path[0] = (char)('a' + _getdrive() - 1);
        temp_path[1] = ':';
        temp_path[2] = 0;
        expand_dirname(newfilename, temp_path);
        strcat(temp_path, newfilename);
        strcpy(newfilename, temp_path);
        isadir = true;
    }
    // if dot, slash, its relative to the current directory, set up full path
    if (newfilename[0] == '.' &&
            newfilename[1] == SLASHC)
    {
        bool test_dir = false;
        temp_path[0] = (char)('a' + _getdrive() - 1);
        temp_path[1] = ':';
        temp_path[2] = 0;
        if (strrchr(newfilename, '.') == newfilename)
        {
            test_dir = true;    // only one '.' assume its a directory
        }
        expand_dirname(newfilename, temp_path);
        strcat(temp_path, newfilename);
        strcpy(newfilename, temp_path);
        if (!test_dir)
        {
            int len = (int) strlen(newfilename);
            newfilename[len-1] = 0; // get rid of slash added by expand_dirname
        }
    }
#else
    findpath(newfilename, temp_path);
    strcpy(newfilename, temp_path);
#endif
    // check existence
    if (!isadir || isafile)
    {
        if (fr_findfirst(newfilename) == 0)
        {
            if (DTA.attribute & SUBDIR) // exists and is dir
            {
                fix_dirname(newfilename);  // add trailing slash
                isadir = true;
                isafile = false;
            }
            else
            {
                isafile = true;
            }
        }
    }

    splitpath(newfilename, drive, dir, fname, ext);
    splitpath(oldfullpath, drive1, dir1, fname1, ext1);
    bool const get_path = (mode == cmd_file::AT_CMD_LINE)
        || (mode == cmd_file::SSTOOLS_INI);
    if ((int) strlen(drive) != 0 && get_path)
    {
        strcpy(drive1, drive);
    }
    if ((int) strlen(dir) != 0 && get_path)
    {
        strcpy(dir1, dir);
    }
    if ((int) strlen(fname) != 0)
    {
        strcpy(fname1, fname);
    }
    if ((int) strlen(ext) != 0)
    {
        strcpy(ext1, ext);
    }
    if (!isadir && !isafile && get_path)
    {
        makepath(oldfullpath, drive1, dir1, nullptr, nullptr);
        int len = (int) strlen(oldfullpath);
        if (len > 0)
        {
            char save;
            // strip trailing slash
            save = oldfullpath[len-1];
            if (save == SLASHC)
            {
                oldfullpath[len-1] = 0;
            }
            if (access(oldfullpath, 0))
            {
                isadir_error = true;
            }
            oldfullpath[len-1] = save;
        }
    }
    makepath(oldfullpath, drive1, dir1, fname1, ext1);
    return isadir_error ? -1 : (isadir ? 1 : 0);
}

int merge_pathnames(std::string &oldfullpath, char const *filename, cmd_file mode)
{
    char buff[FILE_MAX_PATH];
    strcpy(buff, oldfullpath.c_str());
    int const result = merge_pathnames(buff, filename, mode);
    oldfullpath = buff;
    return result;
}

// extract just the filename/extension portion of a path
void extract_filename(char *target, char const *source)
{
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    splitpath(source, nullptr, nullptr, fname, ext);
    makepath(target, "", "", fname, ext);
}

std::string extract_filename(char const *source)
{
    char target[FILE_MAX_FNAME];
    extract_filename(target, source);
    return target;
}

// tells if filename has extension
// returns pointer to period or nullptr
char const *has_ext(char const *source)
{
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT] = { 0 };
    splitpath(source, nullptr, nullptr, fname, ext);
    char const *ret = nullptr;
    if (ext[0] != 0)
    {
        ret = strrchr(source, '.');
    }
    return ret;
}

void shell_sort(void *v1, int n, unsigned sz, int (*fct)(VOIDPTR arg1, VOIDPTR arg2))
{
    void *temp;
    char *v;
    v = (char *)v1;
    for (int gap = n/2; gap > 0; gap /= 2)
    {
        for (int i = gap; i < n; i++)
        {
            for (int j = i-gap; j >= 0; j -= gap)
            {
                if (fct((char **)(v+j*sz), (char **)(v+(j+gap)*sz)) <= 0)
                {
                    break;
                }
                temp = *(char **)(v+j*sz);
                *(char **)(v+j*sz) = *(char **)(v+(j+gap)*sz);
                *(char **)(v+(j+gap)*sz) = static_cast<char *>(temp);
            }
        }
    }
}
