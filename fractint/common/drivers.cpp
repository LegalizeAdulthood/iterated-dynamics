#include <assert.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "cmplx.h"
#include "fractint.h"
#include "drivers.h"

/* list of drivers that are supported by source code in fractint */
/* default driver is first one in the list that initializes. */
#if defined(HAVE_X11_DRIVER)
extern AbstractDriver *x11_driver;
#endif
#if defined(HAVE_GDI_DRIVER)
extern AbstractDriver *gdi_driver;
#endif
#if defined(HAVE_WIN32_DISK_DRIVER)
extern AbstractDriver *disk_driver;
#endif

static AbstractDriver *s_current = NULL;

int DriverManager::s_num_drivers = 0;
AbstractDriver *DriverManager::s_drivers[DriverManager::MAX_DRIVERS] = { 0 };

int DriverManager::load(AbstractDriver *drv, int *argc, char **argv)
{
	if (drv)
	{
		const int num = drv->initialize(argc, argv);
		if (num > 0)
		{
			if (! s_current)
			{
				s_current = drv;
			}
			s_drivers[s_num_drivers++] = drv;
			return 1;
		}
	}

	return 0;
}

/*------------------------------------------------------------
 * open_drivers
 *
 * Go through the static list of drivers defined and try to initialize
 * them one at a time.  Returns the number of drivers initialized.
 */
int DriverManager::open_drivers(int *argc, char **argv)
{
#if HAVE_X11_DRIVER
	load(x11_driver, argc, argv);
#endif

#if HAVE_WIN32_DISK_DRIVER
	load(disk_driver, argc, argv);
#endif

#if HAVE_GDI_DRIVER
	load(gdi_driver, argc, argv);
#endif

	return s_num_drivers;		/* number of drivers supported at runtime */
}

/* add_video_mode
 *
 * a driver uses this to inform the system of an available video mode
 */
void add_video_mode(AbstractDriver *drv, VIDEOINFO &mode)
{
#if defined(_WIN32)
	_ASSERTE(g_video_table_len < MAXVIDEOMODES);
#endif
	/* stash away driver pointer so we can init driver for selected mode */
	mode.driver = drv;
	memcpy(&g_video_table[g_video_table_len], &mode, sizeof(g_video_table[0]));
	g_video_table_len++;
}

void DriverManager::close_drivers()
{
	for (int i = 0; i < s_num_drivers; i++)
	{
		if (s_drivers[i])
		{
			s_drivers[i]->terminate();
			s_drivers[i] = NULL;
		}
	}

	s_current = NULL;
}

AbstractDriver *DriverManager::find_by_name(const char *name)
{
	int i;

	for (i = 0; i < s_num_drivers; i++)
	{
		if (strcmp(name, s_drivers[i]->name()) == 0)
		{
			return s_drivers[i];
		}
	}
	return NULL;
}

void DriverManager::change_video_mode(VIDEOINFO &mode)
{
	if (s_current != mode.driver)
	{
		s_current->pause();
		s_current = mode.driver;
		s_current->resume();
	}
	driver_set_video_mode(mode);
}

void driver_change_video_mode(VIDEOINFO &mode)
{
	DriverManager::change_video_mode(mode);
}

void driver_set_video_mode(VIDEOINFO &mode)
{
	s_current->set_video_mode(mode);
}

const char *driver_name()
{
	return s_current->name();
}

const char *driver_description()
{
	return s_current->description();
}

void driver_terminate()
{
	s_current->terminate();
}

void driver_schedule_alarm(int soon)
{
	s_current->schedule_alarm(soon);
}

void driver_window()
{
	s_current->window();
}

int driver_resize()
{
	return s_current->resize();
}

void driver_redraw()
{
	s_current->redraw();
}

int driver_read_palette()
{
	return s_current->read_palette();
}

int driver_write_palette()
{
	return s_current->write_palette();
}

int driver_read_pixel(int x, int y)
{
	return s_current->read_pixel(x, y);
}

void driver_write_pixel(int x, int y, int color)
{
	s_current->write_pixel(x, y, color);
}

void driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
	s_current->read_span(y, x, lastx, pixels);
}

void driver_write_span(int y, int x, int lastx, const BYTE *pixels)
{
	s_current->write_span(y, x, lastx, pixels);
}

void driver_set_line_mode(int mode)
{
	s_current->set_line_mode(mode);
}

void driver_draw_line(int x1, int y1, int x2, int y2, int color)
{
	s_current->draw_line(x1, y1, x2, y2, color);
}

int driver_get_key()
{
	return s_current->get_key();
}

int driver_key_cursor(int row, int col)
{
	return s_current->key_cursor(row, col);
}

int driver_key_pressed()
{
	return s_current->key_pressed();
}

int driver_wait_key_pressed(int timeout)
{
	return s_current->wait_key_pressed(timeout);
}

void driver_shell()
{
	s_current->shell();
}

void driver_put_string(int row, int col, int attr, const char *msg)
{
	s_current->put_string(row, col, attr, msg);
}

void driver_set_for_text()
{
	s_current->set_for_text();
}

void driver_set_for_graphics()
{
	s_current->set_for_graphics();
}

void driver_set_clear()
{
	s_current->set_clear();
}

void driver_move_cursor(int row, int col)
{
	s_current->move_cursor(row, col);
}

void driver_hide_text_cursor()
{
	s_current->hide_text_cursor();
}

void driver_set_attr(int row, int col, int attr, int count)
{
	s_current->set_attr(row, col, attr, count);
}

void driver_scroll_up(int top, int bot)
{
	s_current->scroll_up(top, bot);
}

void driver_stack_screen()
{
	s_current->stack_screen();
}

void driver_unstack_screen()
{
	s_current->unstack_screen();
}

void driver_discard_screen()
{
	s_current->discard_screen();
}

int driver_init_fm()
{
	return s_current->init_fm();
}

void driver_buzzer(int kind)
{
	s_current->buzzer(kind);
}

int driver_sound_on(int freq)
{
	return s_current->sound_on(freq);
}

void driver_sound_off()
{
	s_current->sound_off();
}

void driver_mute()
{
	s_current->mute();
}

int driver_diskp()
{
	return s_current->diskp();
}

int driver_get_char_attr()
{
	return s_current->get_char_attr();
}

void driver_put_char_attr(int char_attr)
{
	s_current->put_char_attr(char_attr);
}

int driver_get_char_attr_rowcol(int row, int col)
{
	return s_current->get_char_attr_rowcol(row, col);
}

void driver_put_char_attr_rowcol(int row, int col, int char_attr)
{
	s_current->put_char_attr_rowcol(row, col, char_attr);
}

int driver_validate_mode(const VIDEOINFO &mode)
{
	return s_current->validate_mode(mode);
}

void driver_unget_key(int key)
{
	s_current->unget_key(key);
}

void driver_delay(int ms)
{
	s_current->delay(ms);
}

void driver_get_truecolor(int x, int y, int &r, int &g, int &b, int &a)
{
	s_current->get_truecolor(x, y, r, g, b, a);
}

void driver_put_truecolor(int x, int y, int r, int g, int b, int a)
{
	s_current->put_truecolor(x, y, r, g, b, a);
}

void driver_display_string(int x, int y, int fg, int bg, const char *text)
{
	s_current->display_string(x, y, fg, bg, text);
}

void driver_save_graphics()
{
	s_current->save_graphics();
}

void driver_restore_graphics()
{
	s_current->restore_graphics();
}

void driver_get_max_screen(int &x_max, int &y_max)
{
	s_current->get_max_screen(x_max, y_max);
}

void driver_set_keyboard_timeout(int ms)
{
	s_current->set_keyboard_timeout(ms);
}

void driver_flush()
{
	s_current->flush();
}
