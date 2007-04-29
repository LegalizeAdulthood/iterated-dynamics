/* d_win32.c
 *
 * Routines for a Win32 GDI driver for fractint.
 */
#include <assert.h>
#include <stdio.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"

#include "WinText.h"
#include "frame.h"
#include "plot.h"
#include "d_win32.h"
#include "ods.h"

extern HINSTANCE g_instance;

long g_save_base = 0;						/* base clock ticks */
long g_save_ticks = 0;						/* save after this many ticks */
int g_finish_row = 0;						/* save when this row is finished */

Win32BaseDriver::Win32BaseDriver(const char *name, const char *description)
	: NamedDriver(name, description),
	m_frame(),
	m_wintext(),
	m_key_buffer(0),
	m_screen_count(-1),
	m_cursor_shown(false),
	m_cursor_row(0),
	m_cursor_col(0),
	m_inside_help(0),
	m_save_check_time(0),
	m_start(0),
	m_last(0),
	m_ticks_per_second(0)
{
	for (int i = 0; i < WIN32_MAXSCREENS; i++)
	{
		m_saved_screens[i] = NULL;
	}
	for (int i = 0; i < WIN32_MAXSCREENS + 1; i++)
	{
		m_saved_cursor[i] = 0;
	}
}

void Win32BaseDriver::max_size(int &width, int &height, bool &center_x, bool &center_y)
{
	width = m_wintext.max_width();
	height = m_wintext.max_height();
	center_x = true;
	center_y = true;
}

int Win32BaseDriver::handle_timed_save(int ch)
{
	if (ch != 0)
	{
		return ch;
	}

	/* now check for automatic/periodic saving... */
	int ticker = read_ticker();
	if (g_save_ticks && (ticker != m_save_check_time))
	{
		m_save_check_time = ticker;
		ticker -= g_save_base;
		if (ticker > g_save_ticks)
		{
			if (1 == g_finish_row)
			{
				if (g_calculation_status != CALCSTAT_IN_PROGRESS)
				{
					if ((g_got_status != GOT_STATUS_12PASS) && (g_got_status != GOT_STATUS_GUESSING))
					{
						g_finish_row = g_current_row;
					}
				}
			}
			else if (g_current_row != g_finish_row)
			{
				g_timed_save = TRUE;
				return FIK_SAVE_TIME;
			}
		}
	}

	return 0;
}

/* handle_special_keys
 *
 * First, do some slideshow processing.  Then handle F1 and TAB display.
 *
 * Because we want context sensitive help to work everywhere, with the
 * help to display indicated by a non-zero value, we need
 * to trap the F1 key at a very low level.  The same is true of the
 * TAB display.
 *
 * What we do here is check for these keys and invoke their displays.
 * To avoid a recursive invoke of help(), a static is used to avoid
 * recursing on ourselves as help will invoke get key!
 */
int Win32BaseDriver::handle_special_keys(int ch)
{
	ch = handle_timed_save(ch);
	if (ch != FIK_SAVE_TIME)
	{
		if (SLIDES_PLAY == g_slides)
		{
			if (ch == FIK_ESC)
			{
				stop_slide_show();
				ch = 0;
			}
			else if (!ch)
			{
				ch = slide_show();
			}
		}
		else if ((SLIDES_RECORD == g_slides) && ch)
		{
			record_show(ch);
		}
	}
	if (DEBUGFLAG_EDIT_TEXT_COLORS == g_debug_flag)
	{
		if ('~' == ch)
		{
			edit_text_colors();
			ch = 0;
		}
	}

	if (FIK_F1 == ch && get_help_mode() && !m_inside_help)
	{
		m_inside_help = true;
		help(0);
		m_inside_help = false;
		ch = 0;
	}
	else if (FIK_TAB == ch && g_tab_mode)
	{
		int old_tab = g_tab_mode;
		g_tab_mode = 0;
		tab_display();
		g_tab_mode = old_tab;
		ch = 0;
	}

	return ch;
}

void Win32BaseDriver::flush_output()
{
	if (!m_ticks_per_second)
	{
		if (!m_start)
		{
			time(&m_start);
			m_last = read_ticker();
		}
		else
		{
			time_t now = time(NULL);
			long now_ticks = read_ticker();
			if (now > m_start)
			{
				m_ticks_per_second = (now_ticks - m_last)/((long) (now - m_start));
			}
		}
	}
	else
	{
		long now = read_ticker();
		if ((now - m_last)*m_frames_per_second > m_ticks_per_second)
		{
			flush();
			m_frame.pump_messages(FALSE);
			m_last = now;
		}
	}
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* terminate --
*
*	Cleanup windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Cleans up.
*
*----------------------------------------------------------------------
*/
void Win32BaseDriver::terminate()
{
	m_wintext.destroy();
	for (int i = 0; i < NUM_OF(m_saved_screens); i++)
	{
		if (NULL != m_saved_screens[i])
		{
			free(m_saved_screens[i]);
			m_saved_screens[i] = NULL;
		}
	}
}

/*----------------------------------------------------------------------
*
* init --
*
*	Initialize the windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Initializes windows.
*
*----------------------------------------------------------------------
*/
int Win32BaseDriver::initialize(int *argc, char **argv)
{
	LPCSTR title = "FractInt for Windows";

	ODS("win32_init");
	m_frame.init(g_instance, title);
	if (!m_wintext.initialize(g_instance, NULL, "Text"))
	{
		return FALSE;
	}

	return TRUE;
}

/* key_pressed
 *
 * Return 0 if no key has been pressed, or the FIK value if it has.
 * get_key() must still be called to eat the key; this routine
 * only peeks ahead.
 *
 * When a keystroke has been found by the underlying wintext_xxx
 * message pump, stash it in the one key buffer for later use by
 * get_key.
 */
int Win32BaseDriver::key_pressed()
{
	int ch = m_key_buffer;

	if (ch)
	{
		return ch;
	}
	flush_output();
	ch = handle_special_keys(m_frame.get_key_press(0));
	_ASSERTE(m_key_buffer == 0);
	m_key_buffer = ch;

	return ch;
}

/* unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void Win32BaseDriver::unget_key(int key)
{
	_ASSERTE(0 == m_key_buffer);
	m_key_buffer = key;
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
		if (m_key_buffer)
		{
			ch = m_key_buffer;
			m_key_buffer = 0;
		}
		else
		{
			ch = handle_special_keys(m_frame.get_key_press(1));
		}
	}
	while (ch == 0);

	return ch;
}

/* shell
 *
 * Exit to a command shell.
 */
void Win32BaseDriver::shell()
{
	STARTUPINFO si =
	{
		sizeof(si)
	};
	PROCESS_INFORMATION pi = { 0 };
	char *comspec = getenv("COMSPEC");

	if (NULL == comspec)
	{
		comspec = "cmd.exe";
	}
	if (CreateProcess(NULL, comspec, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		DWORD status = WaitForSingleObject(pi.hProcess, 1000);
		while (WAIT_TIMEOUT == status)
		{
			m_frame.pump_messages(0);
			status = WaitForSingleObject(pi.hProcess, 1000);
		}
		CloseHandle(pi.hProcess);
	}
}

void Win32BaseDriver::hide_text_cursor()
{
	if (TRUE == m_cursor_shown)
	{
		m_cursor_shown = FALSE;
		m_wintext.hide_cursor();
	}
	ODS("win32_hide_text_cursor");
}

/* set_video_mode
*/
void Win32BaseDriver::set_video_mode(const VIDEOINFO &mode)
{
	extern void set_normal_dot();
	extern void set_normal_line();

	/* initially, set the virtual line to be the scan line length */
	g_vx_dots = g_screen_width;
	g_is_true_color = 0;				/* assume not truecolor */
	g_ok_to_print = FALSE;
	g_good_mode = 1;
	if (g_dot_mode != 0)
	{
		g_and_color = g_colors-1;
		g_box_count = 0;
		g_dac_learn = 1;
		g_dac_count = g_cycle_limit;
		g_got_real_dac = TRUE;			/* we are "VGA" */

		read_palette();
	}

	resize();

	if (g_disk_flag)
	{
		disk_end();
	}

	set_normal_dot();
	set_normal_line();

	set_for_graphics();
	set_clear();
}

void Win32BaseDriver::put_string(int row, int col, int attr, const char *msg)
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
		m_wintext.putstring(abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
	}
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
void Win32BaseDriver::scroll_up(int top, int bot)
{
	m_wintext.scroll_up(top, bot);
}

void Win32BaseDriver::move_cursor(int row, int col)
{
	if (row != -1)
	{
		m_cursor_row = row;
		g_text_row = row;
	}
	if (col != -1)
	{
		m_cursor_col = col;
		g_text_col = col;
	}
	row = m_cursor_row;
	col = m_cursor_col;
	m_wintext.cursor(g_text_cbase + col, g_text_rbase + row, 1);
	m_cursor_shown = TRUE;
}

void Win32BaseDriver::set_clear()
{
	m_wintext.clear();
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
	m_wintext.set_attr(g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
void Win32BaseDriver::stack_screen()
{
	m_screen_count++;
	m_saved_cursor[m_screen_count] = g_text_row*80 + g_text_col;
	if (m_screen_count)
	{
		/* already have some stacked */
		int i = m_screen_count - 1;

		_ASSERTE(i < WIN32_MAXSCREENS);
		if (i >= WIN32_MAXSCREENS)
		{
			/* bug, missing unstack? */
			stop_message(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		m_saved_screens[i] = m_wintext.screen_get();
	}
	else
	{
		set_for_text();
	}
	set_clear();
}

void Win32BaseDriver::unstack_screen()
{
	_ASSERTE(m_screen_count >= 0);
	g_text_row = m_saved_cursor[m_screen_count] / 80;
	g_text_col = m_saved_cursor[m_screen_count] % 80;
	if (--m_screen_count >= 0)
	{
		/* unstack */
		m_wintext.screen_set(m_saved_screens[m_screen_count]);
		free(m_saved_screens[m_screen_count]);
		m_saved_screens[m_screen_count] = NULL;
		move_cursor(-1, -1);
	}
	else
	{
		set_for_graphics();
	}
}

void Win32BaseDriver::discard_screen()
{
	if (--m_screen_count >= 0)
	{
		/* unstack */
		if (m_saved_screens[m_screen_count])
		{
			free(m_saved_screens[m_screen_count]);
			m_saved_screens[m_screen_count] = NULL;
		}
		set_clear();
	}
	else
	{
		set_for_graphics();
	}
}

int Win32BaseDriver::init_fm()
{
	ODS("win32_init_fm");
	return 0;
}

void Win32BaseDriver::buzzer(int kind)
{
	ODS1("win32_buzzer %d", kind);
	MessageBeep(MB_OK);
}

int Win32BaseDriver::sound_on(int freq)
{
	ODS1("win32_sound_on %d", freq);
	return 0;
}

void Win32BaseDriver::sound_off()
{
	ODS("win32_sound_off");
}

void Win32BaseDriver::mute()
{
	ODS("win32_mute");
}

int Win32BaseDriver::diskp()
{
	return 0;
}

int Win32BaseDriver::key_cursor(int row, int col)
{
	int result;

	ODS2("win32_key_cursor %d,%d", row, col);
	if (-1 != row)
	{
		m_cursor_row = row;
		g_text_row = row;
	}
	if (-1 != col)
	{
		m_cursor_col = col;
		g_text_col = col;
	}

	col = m_cursor_col;
	row = m_cursor_row;

	if (key_pressed())
	{
		result = get_key();
	}
	else
	{
		m_cursor_shown = TRUE;
		m_wintext.cursor(col, row, 1);
		result = get_key();
		hide_text_cursor();
		m_cursor_shown = FALSE;
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
	return m_wintext.get_char_attr(g_text_row, g_text_col);
}

void Win32BaseDriver::put_char_attr(int char_attr)
{
	m_wintext.put_char_attr(g_text_row, g_text_col, char_attr);
}

int Win32BaseDriver::get_char_attr_rowcol(int row, int col)
{
	return m_wintext.get_char_attr(row, col);
}

void Win32BaseDriver::put_char_attr_rowcol(int row, int col, int char_attr)
{
	m_wintext.put_char_attr(row, col, char_attr);
}

void Win32BaseDriver::delay(int ms)
{
	m_frame.pump_messages(FALSE);
	if (ms >= 0)
	{
		Sleep(ms);
	}
}

void Win32BaseDriver::set_keyboard_timeout(int ms)
{
	m_frame.set_keyboard_timeout(ms);
}

void Win32BaseDriver::pause()
{
	m_wintext.pause();
}

void Win32BaseDriver::resume()
{
	m_wintext.resume();
}

void Win32BaseDriver::set_mouse_mode(int new_mode)
{
	m_frame.set_mouse_mode(new_mode);
}

int Win32BaseDriver::get_mouse_mode() const
{
	return m_frame.get_mouse_mode();
}
