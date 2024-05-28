/* d_win32.c
 *
 * Routines for a Win32 GDI driver for id.
 */
#include "port.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "fractype.h"
#include "helpdefs.h"
#include "helpcom.h"
#include "id_data.h"
#include "os.h"
#include "rotate.h"
#include "slideshw.h"
#include "spindac.h"
#include "stop_msg.h"
#include "text_screen.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <Windows.h>

#include <array>
#include <cassert>
#include <cstdio>
#include <ctime>

#include "win_text.h"
#include "d_win32.h"
#include "frame.h"
#include "instance.h"
#include "ods.h"
#include "plot.h"

#include <read_ticker.h>


#define DI(name_) Win32BaseDriver *name_ = (Win32BaseDriver *) drv

static void flush_output()
{
    static time_t start = 0;
    static long ticks_per_second = 0;
    static long last = 0;
    static long frames_per_second = 10;

    if (!ticks_per_second)
    {
        if (!start)
        {
            std::time(&start);
            last = readticker();
        }
        else
        {
            std::time_t now = std::time(nullptr);
            long now_ticks = readticker();
            if (now > start)
            {
                ticks_per_second = (now_ticks - last)/((long)(now - start));
            }
        }
    }
    else
    {
        long now = readticker();
        if ((now - last)*frames_per_second > ticks_per_second)
        {
            driver_flush();
            g_frame.pump_messages(false);
            last = now;
        }
    }
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

void Win32BaseDriver::terminate()
{
    ODS("Win32BaseDriver::terminate");

    wintext.destroy();
    for (int i = 0; i < std::size(saved_screens); i++)
    {
        if (saved_screens[i] != nullptr)
        {
            free(saved_screens[i]);
            saved_screens[i] = nullptr;
        }
    }

    g_frame.terminate();
}

bool Win32BaseDriver::init(int *argc, char **argv)
{
    LPCSTR title = ID_PROGRAM_NAME;

    ODS("Win32BaseDriver::init");
    g_frame.init(g_instance, title);
    if (!wintext.initialize(g_instance, nullptr, "Text"))
    {
        return false;
    }

    return true;
}

/* key_pressed
 *
 * Return 0 if no key has been pressed, or the FIK value if it has.
 * driver_get_key() must still be called to eat the key; this routine
 * only peeks ahead.
 *
 * When a keystroke has been found by the underlying wintext_xxx
 * message pump, stash it in the one key buffer for later use by
 * get_key.
 */
int Win32BaseDriver::key_pressed()
{
    int ch = key_buffer;

    if (ch)
    {
        return ch;
    }
    flush_output();
    ch = handle_special_keys(g_frame.get_key_press(false));
    key_buffer = ch;

    return ch;
}

/* unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void Win32BaseDriver::unget_key(int key)
{
    _ASSERTE(0 == key_buffer);
    key_buffer = key;
}

/* get_key
 *
 * Get a keystroke, blocking if necessary.  First, check the key buffer
 * and if that's empty ask the wintext window to pump a keystroke for us.
 * If we get it, pass it off to handle tab and help displays.  If those
 * displays ate the key, then get another one.
 */
int Win32BaseDriver::get_key()
{
    int ch;

    do
    {
        if (key_buffer)
        {
            ch = key_buffer;
            key_buffer = 0;
        }
        else
        {
            ch = handle_special_keys(g_frame.get_key_press(true));
        }
    }
    while (ch == 0);

    return ch;
}

// Spawn a command prompt.
void  Win32BaseDriver::shell()
{
    STARTUPINFO si =
    {
        sizeof(si)
    };
    PROCESS_INFORMATION pi = { 0 };
    const char *comspec = getenv("COMSPEC");
    if (comspec == nullptr)
    {
        comspec = "cmd.exe";
    }
    const std::string command_line(comspec);
    if (CreateProcessA(
            command_line.c_str(), nullptr, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi))
    {
        DWORD status = WaitForSingleObject(pi.hProcess, 100);
        while (WAIT_TIMEOUT == status)
        {
            g_frame.pump_messages(false);
            status = WaitForSingleObject(pi.hProcess, 100);
        }
        CloseHandle(pi.hProcess);
    }
    else
    {
        stopmsg("Couldn't run shell '" + command_line + "', error " + std::to_string(GetLastError()));
    }
}

void Win32BaseDriver::hide_text_cursor()
{
    if (cursor_shown)
    {
        cursor_shown = false;
        wintext.hide_cursor();
    }
}

// set_video_mode
extern void set_normal_dot();
extern void set_normal_line();

void Win32BaseDriver::set_video_mode(VIDEOINFO *mode)
{
    // initially, set the virtual line to be the scan line length
    g_is_true_color = false;            // assume not truecolor
    g_vesa_x_res = 0;                   // reset indicators used for
    g_vesa_y_res = 0;                   // virtual screen limits estimation
    g_good_mode = true;
    if (g_dot_mode != 0)
    {
        g_and_color = g_colors-1;
        g_box_count = 0;
        g_dac_learn = true;
        g_dac_count = g_cycle_limit;
        g_got_real_dac = true;          // we are "VGA"
        read_palette();
    }

    resize();
    if (g_disk_flag)
    {
        enddisk();
    }
    set_normal_dot();
    set_normal_line();
    set_for_graphics();
    set_clear();
}

void Win32BaseDriver::put_string(int row, int col, int attr, char const *msg)
{
    if (-1 != row)
    {
        g_text_row = row;
    }
    if (-1 != col)
    {
        g_text_col = col;
    }
    {
        int abs_row = g_text_rbase + g_text_row;
        int abs_col = g_text_cbase + g_text_col;
        _ASSERTE(abs_row >= 0 && abs_row < WINTEXT_MAX_ROW);
        _ASSERTE(abs_col >= 0 && abs_col < WINTEXT_MAX_COL);
        wintext.put_string(abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
    }
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
void
Win32BaseDriver::scroll_up(int top, int bot)
{
    wintext.scroll_up(top, bot);
}

void Win32BaseDriver::move_cursor(int row, int col)
{
    if (row != -1)
    {
        cursor_row = row;
        g_text_row = row;
    }
    if (col != -1)
    {
        cursor_col = col;
        g_text_col = col;
    }
    wintext.cursor(g_text_cbase + cursor_col, g_text_rbase + cursor_row, 1);
    cursor_shown = true;
}

void Win32BaseDriver::set_attr(int row, int col, int attr, int count)
{
    if (-1 != row)
    {
        g_text_row = row;
    }
    if (-1 != col)
    {
        g_text_col = col;
    }
    wintext.set_attr(g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
void Win32BaseDriver::stack_screen()
{
    saved_cursor[screen_count+1] = g_text_row*80 + g_text_col;
    if (++screen_count)
    {
        // already have some stacked
        int i = screen_count - 1;

        _ASSERTE(i < WIN32_MAXSCREENS);
        if (i >= WIN32_MAXSCREENS)
        {
            // bug, missing unstack?
            stopmsg(stopmsg_flags::NO_STACK, "Win32BaseDriver::stack_screen overflow");
            exit(1);
        }
        saved_screens[i] = wintext.screen_get();
        driver_set_clear();
    }
    else
    {
        driver_set_for_text();
        driver_set_clear();
    }
}

void Win32BaseDriver::unstack_screen()
{
    _ASSERTE(screen_count >= 0);
    g_text_row = saved_cursor[screen_count] / 80;
    g_text_col = saved_cursor[screen_count] % 80;
    if (--screen_count >= 0)
    {
        // unstack
        wintext.screen_set(saved_screens[screen_count]);
        free(saved_screens[screen_count]);
        saved_screens[screen_count] = nullptr;
        move_cursor(-1, -1);
    }
    else
    {
        set_for_graphics();
    }
}

void Win32BaseDriver::discard_screen()
{
    if (--screen_count >= 0)
    {
        // unstack
        if (saved_screens[screen_count])
        {
            free(saved_screens[screen_count]);
            saved_screens[screen_count] = nullptr;
        }
    }
    else
    {
        set_for_graphics();
    }
}

int Win32BaseDriver::init_fm()
{
    ODS("Win32BaseDriver::init_fm");
    return 0;
}

void Win32BaseDriver::buzzer(buzzer_codes kind)
{
    UINT beep{MB_OK};
    switch (kind)
    {
    case buzzer_codes::COMPLETE:
    default:
        break;
    case buzzer_codes::INTERRUPT:
        beep = MB_ICONWARNING;
        break;
    case buzzer_codes::PROBLEM:
        beep = MB_ICONERROR;
        break;
    }
    MessageBeep(beep);
}

bool Win32BaseDriver::sound_on(int freq)
{
    ODS1("Win32BaseDriver::sound_on %d", freq);
    return false;
}

void Win32BaseDriver::sound_off()
{
    ODS("Win32BaseDriver::sound_off");
}

void Win32BaseDriver::mute()
{
    ODS("Win32BaseDriver::mute");
}

bool Win32BaseDriver::diskp()
{
    return false;
}

int Win32BaseDriver::key_cursor(int row, int col)
{
    ODS2("Win32BaseDriver::key_cursor %d,%d", row, col);
    if (-1 != row)
    {
        cursor_row = row;
        g_text_row = row;
    }
    if (-1 != col)
    {
        cursor_col = col;
        g_text_col = col;
    }

    int result;
    if (key_pressed())
    {
        result = get_key();
    }
    else
    {
        cursor_shown = true;
        wintext.cursor(cursor_col, cursor_row, 1);
        result = get_key();
        hide_text_cursor();
        cursor_shown = false;
    }

    return result;
}

int Win32BaseDriver::wait_key_pressed(int timeout)
{
    int count = 10;
    while (!key_pressed())
    {
        Sleep(25);
        if (timeout && (--count == 0))
        {
            break;
        }
    }

    return key_pressed();
}

int Win32BaseDriver::get_char_attr()
{
    return wintext.get_char_attr(g_text_row, g_text_col);
}

void Win32BaseDriver::put_char_attr(int char_attr)
{
    wintext.put_char_attr(g_text_row, g_text_col, char_attr);
}

void Win32BaseDriver::delay(int ms)
{
    g_frame.pump_messages(false);
    if (ms >= 0)
    {
        Sleep(ms);
    }
}

void Win32BaseDriver::get_truecolor(int x, int y, int *r, int *g, int *b, int *a)
{
    _ASSERTE(0 && "Win32BaseDriver::get_truecolor called.");
}

void Win32BaseDriver::put_truecolor(int x, int y, int r, int g, int b, int a)
{
    _ASSERTE(0 && "Win32BaseDriver::put_truecolor called.");
}

void Win32BaseDriver::set_keyboard_timeout(int ms)
{
    g_frame.set_keyboard_timeout(ms);
}

void Win32BaseDriver::debug_text(const char *text)
{
    OutputDebugStringA(text);
}
