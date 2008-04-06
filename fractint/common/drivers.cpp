#include <cassert>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "cmplx.h"
#include "id.h"
#include "drivers.h"

// list of drivers that are supported by source code in fractint
// default driver is first one in the list that initializes.
#if defined(HAVE_X11_DRIVER)
extern AbstractDriver *x11_driver;
#endif
#if defined(HAVE_GDI_DRIVER)
extern AbstractDriver *gdi_driver;
#endif
#if defined(HAVE_WIN32_DISK_DRIVER)
extern AbstractDriver *disk_driver;
#endif

AbstractDriver *DriverManager::s_current = 0;

int DriverManager::s_num_drivers = 0;
AbstractDriver *DriverManager::s_drivers[DriverManager::MAX_DRIVERS] = { 0 };

int DriverManager::load(AbstractDriver *drv, int &argc, char **argv)
{
	if (drv)
	{
		if (drv->initialize(argc, argv))
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

void DriverManager::unload(AbstractDriver *drv)
{
	for (int i = 0; i < s_num_drivers; i++)
	{
		if (drv == s_drivers[i])
		{
			for (int j = i; j < s_num_drivers-1; j++)
			{
				s_drivers[j] = s_drivers[j+1];
			}
			s_drivers[s_num_drivers-1] = 0;
			s_num_drivers--;
			if (s_current == drv)
			{
				s_current = 0;
			}
		}
	}
}

/*------------------------------------------------------------
 * open_drivers
 *
 * Go through the static list of drivers defined and try to initialize
 * them one at a time.  Returns the number of drivers initialized.
 */
int DriverManager::open_drivers(int &argc, char **argv)
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

	return s_num_drivers;		// number of drivers supported at runtime
}

/* add_video_mode
 *
 * a driver uses this to inform the system of an available video mode
 */
void add_video_mode(AbstractDriver *drv, VIDEOINFO &mode)
{
	// stash away driver pointer so we can init driver for selected mode
	mode.driver = drv;
	g_.AddVideoModeToTable(mode);
}

void DriverManager::close_drivers()
{
	for (int i = 0; i < s_num_drivers; i++)
	{
		if (s_drivers[i])
		{
			s_drivers[i]->terminate();
			s_drivers[i] = 0;
		}
	}

	s_current = 0;
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
	return 0;
}

void DriverManager::change_video_mode(const VIDEOINFO &mode)
{
	if (s_current != mode.driver)
	{
		s_current->pause();
		s_current = mode.driver;
		s_current->resume();
	}
	driver_set_video_mode(mode);
}

void driver_change_video_mode(const VIDEOINFO &mode)
{
	DriverManager::change_video_mode(mode);
}

void driver_set_video_mode(const VIDEOINFO &mode)
{
	DriverManager::current()->set_video_mode(mode);
}

const char *driver_name()
{
	return DriverManager::current()->name();
}

const char *driver_description()
{
	return DriverManager::current()->description();
}

void driver_terminate()
{
	DriverManager::current()->terminate();
}

void driver_schedule_alarm(int soon)
{
	DriverManager::current()->schedule_alarm(soon);
}

void driver_window()
{
	DriverManager::current()->window();
}

int driver_resize()
{
	return DriverManager::current()->resize();
}

void driver_redraw()
{
	DriverManager::current()->redraw();
}

int driver_read_palette()
{
	return DriverManager::current()->read_palette();
}

int driver_write_palette()
{
	return DriverManager::current()->write_palette();
}

int driver_read_pixel(int x, int y)
{
	return DriverManager::current()->read_pixel(x, y);
}

void driver_write_pixel(int x, int y, int color)
{
	DriverManager::current()->write_pixel(x, y, color);
}

void driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
	DriverManager::current()->read_span(y, x, lastx, pixels);
}

void driver_write_span(int y, int x, int lastx, const BYTE *pixels)
{
	DriverManager::current()->write_span(y, x, lastx, pixels);
}

void driver_set_line_mode(int mode)
{
	DriverManager::current()->set_line_mode(mode);
}

void driver_draw_line(int x1, int y1, int x2, int y2, int color)
{
	DriverManager::current()->draw_line(x1, y1, x2, y2, color);
}

int driver_get_key()
{
	return DriverManager::current()->get_key();
}

int driver_key_cursor(int row, int col)
{
	return DriverManager::current()->key_cursor(row, col);
}

int driver_key_pressed()
{
	return DriverManager::current()->key_pressed();
}

int driver_wait_key_pressed(int timeout)
{
	return DriverManager::current()->wait_key_pressed(timeout);
}

void driver_shell()
{
	DriverManager::current()->shell();
}

void driver_put_string(int row, int col, int attr, const std::string &text)
{
	DriverManager::current()->put_string(row, col, attr, text);
}

void driver_put_string(int row, int col, int attr, const char *msg)
{
	DriverManager::current()->put_string(row, col, attr, msg);
}

void driver_set_for_text()
{
	DriverManager::current()->set_for_text();
}

void driver_set_for_graphics()
{
	DriverManager::current()->set_for_graphics();
}

void driver_set_clear()
{
	DriverManager::current()->set_clear();
}

void driver_move_cursor(int row, int col)
{
	DriverManager::current()->move_cursor(row, col);
}

void driver_hide_text_cursor()
{
	DriverManager::current()->hide_text_cursor();
}

void driver_set_attr(int row, int col, int attr, int count)
{
	DriverManager::current()->set_attr(row, col, attr, count);
}

void driver_scroll_up(int top, int bot)
{
	DriverManager::current()->scroll_up(top, bot);
}

void driver_stack_screen()
{
	DriverManager::current()->stack_screen();
}

void driver_unstack_screen()
{
	DriverManager::current()->unstack_screen();
}

void driver_discard_screen()
{
	DriverManager::current()->discard_screen();
}

int driver_init_fm()
{
	return DriverManager::current()->init_fm();
}

void driver_buzzer(int kind)
{
	DriverManager::current()->buzzer(kind);
}

int driver_sound_on(int freq)
{
	return DriverManager::current()->sound_on(freq);
}

void driver_sound_off()
{
	DriverManager::current()->sound_off();
}

void driver_mute()
{
	DriverManager::current()->mute();
}

int driver_diskp()
{
	return DriverManager::current()->diskp();
}

int driver_get_char_attr()
{
	return DriverManager::current()->get_char_attr();
}

void driver_put_char_attr(int char_attr)
{
	DriverManager::current()->put_char_attr(char_attr);
}

int driver_get_char_attr_rowcol(int row, int col)
{
	return DriverManager::current()->get_char_attr_rowcol(row, col);
}

void driver_put_char_attr_rowcol(int row, int col, int char_attr)
{
	DriverManager::current()->put_char_attr_rowcol(row, col, char_attr);
}

int driver_validate_mode(const VIDEOINFO &mode)
{
	return DriverManager::current()->validate_mode(mode);
}

void driver_unget_key(int key)
{
	DriverManager::current()->unget_key(key);
}

void driver_delay(int ms)
{
	DriverManager::current()->delay(ms);
}

void driver_get_truecolor(int x, int y, int &r, int &g, int &b, int &a)
{
	DriverManager::current()->get_truecolor(x, y, r, g, b, a);
}

void driver_put_truecolor(int x, int y, int r, int g, int b, int a)
{
	DriverManager::current()->put_truecolor(x, y, r, g, b, a);
}

void driver_display_string(int x, int y, int fg, int bg, const std::string &text)
{
	DriverManager::current()->display_string(x, y, fg, bg, text.c_str());
}
void driver_display_string(int x, int y, int fg, int bg, const char *text)
{
	DriverManager::current()->display_string(x, y, fg, bg, text);
}

void driver_save_graphics()
{
	DriverManager::current()->save_graphics();
}

void driver_restore_graphics()
{
	DriverManager::current()->restore_graphics();
}

void driver_get_max_screen(int &x_max, int &y_max)
{
	DriverManager::current()->get_max_screen(x_max, y_max);
}

void driver_set_keyboard_timeout(int ms)
{
	DriverManager::current()->set_keyboard_timeout(ms);
}

void driver_flush()
{
	DriverManager::current()->flush();
}

void driver_set_mouse_mode(int new_mode)
{
	DriverManager::current()->set_mouse_mode(new_mode);
}

int driver_get_mouse_mode()
{
	return DriverManager::current()->get_mouse_mode();
}
