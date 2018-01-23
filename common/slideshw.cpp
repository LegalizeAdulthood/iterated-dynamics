//*********************************************************************
// These routines are called by driver_get_key to allow keystrokes to control
// Fractint to be read from a file.
//*********************************************************************
#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "helpcom.h"
#include "miscres.h"
#include "realdos.h"
#include "slideshw.h"

#include <ctype.h>
#include <float.h>
#include <string.h>
#include <time.h>

#include <sstream>
#include <system_error>

static void sleep_secs(int);
static int showtempmsg_txt(int row, int col, int attr, int secs, const char *txt);
static void message(int secs, char const *buf);
static void slideshowerr(char const *msg);
static int  get_scancode(char const *mn);
static void get_mnemonic(int code, char *mnemonic);

#define MAX_MNEMONIC    20   // max size of any mnemonic string

struct scancodes
{
    int code;
    char const *mnemonic;
};

static scancodes scancodes[] =
{
    { FIK_ENTER,            "ENTER"     },
    { FIK_INSERT,           "INSERT"    },
    { FIK_DELETE,           "DELETE"    },
    { FIK_ESC,              "ESC"       },
    { FIK_TAB,              "TAB"       },
    { FIK_PAGE_UP,          "PAGEUP"    },
    { FIK_PAGE_DOWN,        "PAGEDOWN"  },
    { FIK_HOME,             "HOME"      },
    { FIK_END,              "END"       },
    { FIK_LEFT_ARROW,       "LEFT"      },
    { FIK_RIGHT_ARROW,      "RIGHT"     },
    { FIK_UP_ARROW,         "UP"        },
    { FIK_DOWN_ARROW,       "DOWN"      },
    { FIK_F1,               "F1"        },
    { FIK_CTL_RIGHT_ARROW,  "CTRL_RIGHT"},
    { FIK_CTL_LEFT_ARROW,   "CTRL_LEFT" },
    { FIK_CTL_DOWN_ARROW,   "CTRL_DOWN" },
    { FIK_CTL_UP_ARROW,     "CTRL_UP"   },
    { FIK_CTL_END,          "CTRL_END"  },
    { FIK_CTL_HOME,         "CTRL_HOME" }
};

static int get_scancode(char const *mn)
{
    for (auto const &it : scancodes)
    {
        if (strcmp(mn, it.mnemonic) == 0)
        {
            return it.code;
        }
    }

    return -1;
}

static void get_mnemonic(int code, char *mnemonic)
{
    *mnemonic = 0;
    for (auto const &it : scancodes)
    {
        if (code == it.code)
        {
            strcpy(mnemonic, it.mnemonic);
            break;
        }
    }
}

bool g_busy = false;
static FILE *fpss = nullptr;
static long starttick;
static long ticks;
static int slowcount;
static bool quotes = false;
static bool calcwait = false;
static int repeats = 0;
static int last1 = 0;

// places a temporary message on the screen in text mode
static int showtempmsg_txt(int row, int col, int attr, int secs, const char *txt)
{
    int savescrn[80];

    for (int i = 0; i < 80; i++)
    {
        driver_move_cursor(row, i);
        savescrn[i] = driver_get_char_attr();
    }
    driver_put_string(row, col, attr, txt);
    driver_hide_text_cursor();
    sleep_secs(secs);
    for (int i = 0; i < 80; i++)
    {
        driver_move_cursor(row, i);
        driver_put_char_attr(savescrn[i]);
    }
    return (0);
}

static void message(int secs, char const *buf)
{
    char nearbuf[41] = { 0 };
    strncpy(nearbuf, buf, NUM_OF(nearbuf)-1);
    showtempmsg_txt(0, 0, 7, secs, nearbuf);
    if (!showtempmsg(nearbuf))
    {
        sleep_secs(secs);
        cleartempmsg();
    }
}

// this routine reads the file g_auto_name and returns keystrokes
int slideshw()
{
    int out, err, i;
    char buffer[81];
    if (calcwait)
    {
        if (g_calc_status == calc_status_value::IN_PROGRESS || g_busy)   // restart timer - process not done
        {
            return (0); // wait for calc to finish before reading more keystrokes
        }
        calcwait = false;
    }
    if (fpss == nullptr)     // open files first time through
    {
        if (startslideshow() == slides_mode::OFF)
        {
            stopslideshow();
            return (0);
        }
    }

    if (ticks) // if waiting, see if waited long enough
    {
        if (clock_ticks() - starttick < ticks)   // haven't waited long enough
        {
            return (0);
        }
        ticks = 0;
    }
    if (++slowcount <= 18)
    {
        starttick = clock_ticks();
        ticks = CLOCKS_PER_SEC/5; // a slight delay so keystrokes are visible
        if (slowcount > 10)
        {
            ticks /= 2;
        }
    }
    if (repeats > 0)
    {
        repeats--;
        return (last1);
    }
start:
    if (quotes) // reading a quoted string
    {
        out = fgetc(fpss);
        if (out != '\"' && out != EOF)
        {
            return (last1 = out);
        }
        quotes = false;
    }
    // skip white space:
    while ((out = fgetc(fpss)) == ' ' || out == '\t' || out == '\n')
    {
    }
    switch (out)
    {
    case EOF:
        stopslideshow();
        return (0);
    case '\"':        // begin quoted string
        quotes = true;
        goto start;
    case ';':         // comment from here to end of line, skip it
        while ((out = fgetc(fpss)) != '\n' && out != EOF)
        {
        }
        goto start;
    case '*':
        if (fscanf(fpss, "%d", &repeats) != 1
                || repeats <= 1 || repeats >= 256 || feof(fpss))
        {
            slideshowerr("error in * argument");
            repeats = 0;
            last1 = repeats;
        }
        repeats -= 2;
        return (out = last1);
    }

    i = 0;
    while (true) // get a token
    {
        if (i < 80)
        {
            buffer[i++] = (char)out;
        }
        out = fgetc(fpss);
        if (out == ' ' || out == '\t' || out == '\n' || out == EOF)
        {
            break;
        }
    }
    buffer[i] = 0;
    if (buffer[i-1] == ':')
    {
        goto start;
    }
    out = -12345;
    if (isdigit(buffer[0]))         // an arbitrary scan code number - use it
    {
        out = atoi(buffer);
    }
    else if (strcmp(buffer, "MESSAGE") == 0)
    {
        int secs;
        if (fscanf(fpss, "%d", &secs) != 1)
        {
            slideshowerr("MESSAGE needs argument");
        }
        else
        {
            int len;
            char buf[41];
            buf[40] = 0;
            if (fgets(buf, 40, fpss) == nullptr)
            {
                throw std::system_error(errno, std::system_category(), "slideshw failed fgets");
            }
            len = (int) strlen(buf);
            buf[len-1] = 0; // zap newline
            message(secs, buf);
        }
        out = 0;
    }
    else if (strcmp(buffer, "GOTO") == 0)
    {
        if (fscanf(fpss, "%s", buffer) != 1)
        {
            slideshowerr("GOTO needs target");
            out = 0;
        }
        else
        {
            char buffer1[80];
            rewind(fpss);
            strcat(buffer, ":");
            do
            {
                err = fscanf(fpss, "%s", buffer1);
            }
            while (err == 1 && strcmp(buffer1, buffer) != 0);
            if (feof(fpss))
            {
                slideshowerr("GOTO target not found");
                return (0);
            }
            goto start;
        }
    }
    else if ((i = get_scancode(buffer)) > 0)
    {
        out = i;
    }
    else if (strcmp("WAIT", buffer) == 0)
    {
        float fticks;
        err = fscanf(fpss, "%f", &fticks); // how many ticks to wait
        driver_set_keyboard_timeout((int)(fticks*1000.f));
        fticks *= CLOCKS_PER_SEC;             // convert from seconds to ticks
        if (err == 1)
        {
            ticks = (long)fticks;
            starttick = clock_ticks();  // start timing
        }
        else
        {
            slideshowerr("WAIT needs argument");
        }
        out = 0;
        slowcount = out;
    }
    else if (strcmp("CALCWAIT", buffer) == 0) // wait for calc to finish
    {
        calcwait = true;
        out = 0;
        slowcount = out;
    }
    else if ((i = check_vidmode_keyname(buffer)) != 0)
    {
        out = i;
    }
    if (out == -12345)
    {
        std::ostringstream msg;
        msg << "Can't understand " << buffer;
        slideshowerr(msg.str().c_str());
        out = 0;
    }
    return (last1 = out);
}

slides_mode
startslideshow()
{
    fpss = fopen(g_auto_name.c_str(), "r");
    if (fpss == nullptr)
    {
        g_slides = slides_mode::OFF;
    }
    ticks = 0;
    quotes = false;
    calcwait = false;
    slowcount = 0;
    return g_slides;
}

void stopslideshow()
{
    if (fpss)
    {
        fclose(fpss);
    }
    fpss = nullptr;
    g_slides = slides_mode::OFF;
}

void recordshw(int key)
{
    char mn[MAX_MNEMONIC];
    float dt;
    dt = (float)ticks;      // save time of last call
    ticks = clock_ticks();  // current time
    if (fpss == nullptr)
    {
        fpss = fopen(g_auto_name.c_str(), "w");
        if (fpss == nullptr)
        {
            return;
        }
    }
    dt = ticks-dt;
    dt /= CLOCKS_PER_SEC;  // dt now in seconds
    if (dt > .5) // don't bother with less than half a second
    {
        if (quotes) // close quotes first
        {
            quotes = false;
            fprintf(fpss, "\"\n");
        }
        fprintf(fpss, "WAIT %4.1f\n", dt);
    }
    if (key >= 32 && key < 128)
    {
        if (!quotes)
        {
            quotes = true;
            fputc('\"', fpss);
        }
        fputc(key, fpss);
    }
    else
    {
        if (quotes) // not an ASCII character - turn off quotes
        {
            fprintf(fpss, "\"\n");
            quotes = false;
        }
        get_mnemonic(key, mn);
        if (*mn)
        {
            fprintf(fpss, "%s", mn);
        }
        else if (check_vidmode_key(0, key) >= 0)
        {
            char buf[10];
            vidmode_keyname(key, buf);
            fputs(buf, fpss);
        }
        else   // not ASCII and not FN key
        {
            fprintf(fpss, "%4d", key);
        }
        fputc('\n', fpss);
    }
}

// suspend process # of seconds
static void sleep_secs(int secs)
{
    long stop;
    stop = clock_ticks() + (long)secs*CLOCKS_PER_SEC;
    while (clock_ticks() < stop && driver_key_pressed() == 0)
    {
    } // bailout if key hit
}

static void slideshowerr(char const *msg)
{
    char msgbuf[300] = { "Slideshow error:\n" };
    stopslideshow();
    strcat(msgbuf, msg);
    stopmsg(STOPMSG_NONE, msgbuf);
}

// handle_special_keys
//
// First, do some slideshow processing.  Then handle F1 and TAB display.
//
// Because we want context sensitive help to work everywhere, with the
// help to display indicated by a non-zero value in help_mode, we need
// to trap the F1 key at a very low level.  The same is true of the
// TAB display.
//
// What we do here is check for these keys and invoke their displays.
// To avoid a recursive invoke of help(), a static is used to avoid
// recursing on ourselves as help will invoke get key!
//
int handle_special_keys(int ch)
{
    if (slides_mode::PLAY == g_slides)
    {
        if (ch == FIK_ESC)
        {
            stopslideshow();
            ch = 0;
        }
        else if (!ch)
        {
            ch = slideshw();
        }
    }
    else if ((slides_mode::RECORD == g_slides) && ch)
    {
        recordshw(ch);
    }

    static bool inside_help = false;
    if (FIK_F1 == ch && g_help_mode && !inside_help)
    {
        inside_help = true;
        help(0);
        inside_help = false;
        ch = 0;
    }
    else if (FIK_TAB == ch && g_tab_mode)
    {
        bool const old_tab_mode = g_tab_mode;
        g_tab_mode = false;
        tab_display();
        g_tab_mode = old_tab_mode;
        ch = 0;
    }

    return ch;
}
