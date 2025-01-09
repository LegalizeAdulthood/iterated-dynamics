// SPDX-License-Identifier: GPL-3.0-only
//
//*********************************************************************
// These routines are called by driver_get_key to allow keystrokes that
// control Iterated Dynamics to be read from a file.
//*********************************************************************
#include "slideshw.h"

#include "check_write_file.h"
#include "drivers.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "save_file.h"
#include "ui/stop_msg.h"
#include "ui/tab_display.h"
#include "ui/temp_msg.h"
#include "ValueSaver.h"
#include "video_mode.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <system_error>
#include <thread>

// Guard against Windows headers that define these as macros
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

enum
{
    MAX_MNEMONIC = 20   // max size of any mnemonic string
};

struct KeyMnemonic
{
    int code;
    char const *mnemonic;
};

static void sleep_secs(int);
static int show_temp_msg_txt(int row, int col, int attr, int secs, const char *txt);
static void message(int secs, char const *buf);
static void slide_show_err(char const *msg);
static int  get_scancode(char const *mn);
static void get_mnemonic(int code, char *mnemonic);

SlidesMode g_slides{SlidesMode::OFF}; // PLAY autokey=play, RECORD autokey=record
std::string g_auto_name{"auto.key"};    // record auto keystrokes here
bool g_busy{};

static KeyMnemonic s_key_mnemonics[] =
{
    { ID_KEY_ENTER,            "ENTER"     },
    { ID_KEY_INSERT,           "INSERT"    },
    { ID_KEY_DELETE,           "DELETE"    },
    { ID_KEY_ESC,              "ESC"       },
    { ID_KEY_TAB,              "TAB"       },
    { ID_KEY_PAGE_UP,          "PAGEUP"    },
    { ID_KEY_PAGE_DOWN,        "PAGEDOWN"  },
    { ID_KEY_HOME,             "HOME"      },
    { ID_KEY_END,              "END"       },
    { ID_KEY_LEFT_ARROW,       "LEFT"      },
    { ID_KEY_RIGHT_ARROW,      "RIGHT"     },
    { ID_KEY_UP_ARROW,         "UP"        },
    { ID_KEY_DOWN_ARROW,       "DOWN"      },
    { ID_KEY_F1,               "F1"        },
    { ID_KEY_CTL_RIGHT_ARROW,  "CTRL_RIGHT"},
    { ID_KEY_CTL_LEFT_ARROW,   "CTRL_LEFT" },
    { ID_KEY_CTL_DOWN_ARROW,   "CTRL_DOWN" },
    { ID_KEY_CTL_UP_ARROW,     "CTRL_UP"   },
    { ID_KEY_CTL_END,          "CTRL_END"  },
    { ID_KEY_CTL_HOME,         "CTRL_HOME" }
};
static std::FILE *s_slide_show_file{};
static long s_start_tick{};
static long s_ticks{};
static int s_slow_count{};
static bool s_quotes{};
static bool s_calc_wait{};
static int s_repeats{};
static int s_last1{};

static int get_scancode(char const *mn)
{
    for (KeyMnemonic const &it : s_key_mnemonics)
    {
        if (std::strcmp(mn, it.mnemonic) == 0)
        {
            return it.code;
        }
    }

    return -1;
}

static void get_mnemonic(int code, char *mnemonic)
{
    *mnemonic = 0;
    for (KeyMnemonic const &it : s_key_mnemonics)
    {
        if (code == it.code)
        {
            std::strcpy(mnemonic, it.mnemonic);
            break;
        }
    }
}

// places a temporary message on the screen in text mode
static int show_temp_msg_txt(int row, int col, int attr, int secs, const char *txt)
{
    int save_screen[80];

    for (int i = 0; i < 80; i++)
    {
        driver_move_cursor(row, i);
        save_screen[i] = driver_get_char_attr();
    }
    driver_put_string(row, col, attr, txt);
    driver_hide_text_cursor();
    sleep_secs(secs);
    for (int i = 0; i < 80; i++)
    {
        driver_move_cursor(row, i);
        driver_put_char_attr(save_screen[i]);
    }
    return 0;
}

static void message(int secs, char const *buf)
{
    if (driver_is_text())
    {
        show_temp_msg_txt(0, 0, 7, secs, buf);
    }
    else if (!show_temp_msg(buf))
    {
        sleep_secs(secs);
        clear_temp_msg();
    }
}

// this routine reads the file g_auto_name and returns keystrokes
int slide_show()
{
    int out;
    char buffer[81];
    if (s_calc_wait)
    {
        if (g_calc_status == CalcStatus::IN_PROGRESS || g_busy)   // restart timer - process not done
        {
            return 0; // wait for calc to finish before reading more keystrokes
        }
        s_calc_wait = false;
    }
    if (s_slide_show_file == nullptr)     // open files first time through
    {
        if (start_slide_show() == SlidesMode::OFF)
        {
            stop_slide_show();
            return 0;
        }
    }

    const clock_t now = std::clock();
    if (s_ticks) // if waiting, see if waited long enough
    {
        if (now - s_start_tick < s_ticks)   // haven't waited long enough
        {
            return 0;
        }
        s_ticks = 0;
    }
    constexpr int SLOW_ADJUST_CHAR_COUNT = 15;
    constexpr int SLOW_INITIAL_CHAR_COUNT = 5;
    if (++s_slow_count <= SLOW_ADJUST_CHAR_COUNT)
    {
        s_start_tick = now;
        s_ticks = CLOCKS_PER_SEC/10; // a slight delay so keystrokes are visible
        if (s_slow_count > SLOW_INITIAL_CHAR_COUNT)
        {
            s_ticks = std::max(CLOCKS_PER_SEC/150, s_ticks/2);
        }
        driver_set_keyboard_timeout((1000*s_ticks)/CLOCKS_PER_SEC);
    }
    if (s_repeats > 0)
    {
        s_repeats--;
        return s_last1;
    }
start:
    if (s_quotes) // reading a quoted string
    {
        out = fgetc(s_slide_show_file);
        if (out != '\"' && out != EOF)
        {
            s_last1 = out;
            return out;
        }
        s_quotes = false;
    }
    // skip white space:
    while ((out = fgetc(s_slide_show_file)) == ' ' || out == '\t' || out == '\n')
    {
    }
    switch (out)
    {
    case EOF:
        stop_slide_show();
        return 0;
    case '\"':        // begin quoted string
        s_quotes = true;
        goto start;
    case ';':         // comment from here to end of line, skip it
        while (out != '\n' && out != EOF)
        {
            out = fgetc(s_slide_show_file);
        }
        goto start;
    case '*':
        if (std::fscanf(s_slide_show_file, "%d", &s_repeats) != 1
            || s_repeats <= 1
            || s_repeats >= 256
            || std::feof(s_slide_show_file))
        {
            slide_show_err("error in * argument");
            s_repeats = 0;
            s_last1 = s_repeats;
        }
        s_repeats -= 2;
        out = s_last1;
        return out;
    }

    int i = 0;
    while (true) // get a token
    {
        if (i < 80)
        {
            buffer[i++] = (char)out;
        }
        out = fgetc(s_slide_show_file);
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
    if (std::isdigit(buffer[0]))         // an arbitrary scan code number - use it
    {
        out = std::atoi(buffer);
    }
    else if (std::strcmp(buffer, "MESSAGE") == 0)
    {
        int secs;
        if (std::fscanf(s_slide_show_file, "%d", &secs) != 1)
        {
            slide_show_err("MESSAGE needs argument");
        }
        else
        {
            char buf[41];
            buf[40] = 0;
            if (std::fgets(buf, std::size(buf), s_slide_show_file) == nullptr)
            {
                throw std::system_error(errno, std::system_category(), "slideshw failed fgets");
            }
            int len = (int) std::strlen(buf);
            buf[len-1] = 0; // zap newline
            message(secs, buf);
        }
        out = 0;
    }
    else if (std::strcmp(buffer, "GOTO") == 0)
    {
        if (std::fscanf(s_slide_show_file, "%s", buffer) != 1)
        {
            slide_show_err("GOTO needs target");
            out = 0;
        }
        else
        {
            char line[80];
            std::rewind(s_slide_show_file);
            std::strcat(buffer, ":");
            int count;
            do
            {
                count = std::fscanf(s_slide_show_file, "%s", line);
            }
            while (count == 1 && std::strcmp(line, buffer) != 0);
            if (std::feof(s_slide_show_file))
            {
                slide_show_err("GOTO target not found");
                return 0;
            }
            goto start;
        }
    }
    else if ((i = get_scancode(buffer)) > 0)
    {
        out = i;
    }
    else if (std::strcmp("WAIT", buffer) == 0)
    {
        float f_ticks;
        const int count = std::fscanf(s_slide_show_file, "%f", &f_ticks); // how many seconds to wait
        driver_set_keyboard_timeout((int)(f_ticks*1000.f)); // timeout in ms
        f_ticks *= CLOCKS_PER_SEC;             // convert from seconds to ticks
        if (count == 1)
        {
            s_ticks = (long)f_ticks;
            s_start_tick = now;  // start timing
        }
        else
        {
            slide_show_err("WAIT needs argument");
        }
        out = 0;
        s_slow_count = 0;
    }
    else if (std::strcmp("CALCWAIT", buffer) == 0) // wait for calc to finish
    {
        s_calc_wait = true;
        out = 0;
        s_slow_count = 0;
    }
    else if ((i = check_vid_mode_key_name(buffer)) != 0)
    {
        out = i;
    }
    if (out == -12345)
    {
        std::ostringstream msg;
        msg << "Can't understand " << buffer;
        slide_show_err(msg.str().c_str());
        out = 0;
    }
    s_last1 = out;
    return out;
}

SlidesMode start_slide_show()
{
    s_slide_show_file = std::fopen(g_auto_name.c_str(), "r");
    if (s_slide_show_file == nullptr)
    {
        g_slides = SlidesMode::OFF;
    }
    s_ticks = 0;
    s_quotes = false;
    s_calc_wait = false;
    s_slow_count = 0;
    return g_slides;
}

void stop_slide_show()
{
    if (s_slide_show_file)
    {
        std::fclose(s_slide_show_file);
    }
    s_slide_show_file = nullptr;
    g_slides = SlidesMode::OFF;
}

void record_show(int key)
{
    float dt = (float) s_ticks;      // save time of last call
    s_ticks = std::clock();  // current time
    if (s_slide_show_file == nullptr)
    {
        check_write_file(g_auto_name, ".key");
        s_slide_show_file = open_save_file(g_auto_name, "w");
        if (s_slide_show_file == nullptr)
        {
            return;
        }
    }
    dt = s_ticks-dt;
    dt /= CLOCKS_PER_SEC;  // dt now in seconds
    if (dt > .5) // don't bother with less than half a second
    {
        if (s_quotes) // close quotes first
        {
            s_quotes = false;
            std::fprintf(s_slide_show_file, "\"\n");
        }
        std::fprintf(s_slide_show_file, "WAIT %4.1f\n", dt);
    }
    if (key >= 32 && key < 128)
    {
        if (!s_quotes)
        {
            s_quotes = true;
            std::fputc('\"', s_slide_show_file);
        }
        std::fputc(key, s_slide_show_file);
    }
    else
    {
        if (s_quotes) // not an ASCII character - turn off quotes
        {
            std::fprintf(s_slide_show_file, "\"\n");
            s_quotes = false;
        }
        char mn[MAX_MNEMONIC]{};
        get_mnemonic(key, mn);
        if (*mn)
        {
            std::fprintf(s_slide_show_file, "%s", mn);
        }
        else if (check_vid_mode_key(key) >= 0)
        {
            char buf[10];
            vid_mode_key_name(key, buf);
            std::fputs(buf, s_slide_show_file);
        }
        else   // not ASCII and not FN key
        {
            std::fprintf(s_slide_show_file, "%4d", key);
        }
        std::fputc('\n', s_slide_show_file);
    }
}

// suspend process # of seconds
static void sleep_secs(int secs)
{
    g_slides = SlidesMode::OFF;
    const int iterations = secs * 100;
    for (int i = 0; i < iterations; ++i)
    {
        if (driver_key_pressed() != 0)
        {
            // bailout if key hit
            break;
        }
        // sleep 10ms per iteration
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    g_slides = SlidesMode::PLAY;
}

static void slide_show_err(char const *msg)
{
    char msg_buff[300] = { "Slideshow error:\n" };
    stop_slide_show();
    std::strcat(msg_buff, msg);
    stop_msg(msg_buff);
}

// handle_special_keys
//
// First, do some slideshow processing.  Then handle F1 and TAB display.
//
// Because we want context-sensitive help to work everywhere, with the
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
    if (g_slides == SlidesMode::PLAY)
    {
        if (ch == ID_KEY_ESC)
        {
            stop_slide_show();
            ch = 0;
        }
        else if (!ch)
        {
            ch = slide_show();
        }
    }
    else if ((g_slides == SlidesMode::RECORD) && ch)
    {
        record_show(ch);
    }

    static bool inside_help = false;
    if (ID_KEY_F1 == ch && g_help_mode != HelpLabels::HELP_INDEX && !inside_help)
    {
        inside_help = true;
        help();
        inside_help = false;
        ch = 0;
    }
    else if (ID_KEY_TAB == ch && g_tab_mode)
    {
        ValueSaver saved_tab_mode(g_tab_mode, false);
        tab_display();
        ch = 0;
    }

    return ch;
}
