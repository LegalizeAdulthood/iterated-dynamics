#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "cmplx.h"
#include "drivers.h"

#include <string.h>

extern Driver *fractint_driver;
extern Driver *disk_driver;
extern Driver *x11_driver;
extern Driver *win32_driver;
extern Driver *win32_disk_driver;

/* list of drivers that are supported by source code in fractint */
/* default driver is first one in the list that initializes. */
#define MAX_DRIVERS 10
static int num_drivers = 0;
static Driver *s_available[MAX_DRIVERS];

Driver *g_driver = NULL;

#define NUM_OF(array_) (sizeof(array_)/sizeof(array_[0]))

static int
load_driver(Driver *drv, int *argc, char **argv)
{
	if (drv && drv->init)
	{
		const int num = (*drv->init)(drv, argc, argv);
		if (num > 0)
		{
			if (! g_driver)
			{
				g_driver = drv;
			}
			s_available[num_drivers++] = drv;
			return 1;
		}
	}

	return 0;
}

/*------------------------------------------------------------
 * init_drivers
 *
 * Go through the static list of drivers defined and try to initialize
 * them one at a time.  Returns the number of drivers initialized.
 */
int
init_drivers(int *argc, char **argv)
{
#if HAVE_X11_DRIVER
	load_driver(x11_driver, argc, argv);
#endif

#if HAVE_DISK_DRIVER
	load_driver(disk_driver, argc, argv);
#endif

#if HAVE_WIN32_DRIVER
	load_driver(win32_driver, argc, argv);
#endif

#if HAVE_WIN32_DISK_DRIVER
	load_driver(win32_disk_driver, argc, argv);
#endif

	return num_drivers;		/* number of drivers supported at runtime */
}

/* add_video_mode
 *
 * a driver uses this to inform the system of an available video mode
 */
void
add_video_mode(Driver *drv, VIDEOINFO *mode)
{
	_ASSERTE(g_video_table_len < MAXVIDEOMODES);
	/* stash away driver pointer so we can init driver for selected mode */
	mode->driver = drv;
	memcpy(&g_video_table[g_video_table_len], mode, sizeof(g_video_table[0]));
	g_video_table_len++;
}

void
close_drivers(void)
{
	int i;

	for (i = 0; i < num_drivers; i++)
	{
		if (s_available[i])
		{
			(*s_available[i]->terminate)(s_available[i]);
			s_available[i] = NULL;
		}
	}

	g_driver = NULL;
}

Driver *
driver_find_by_name(const char *name)
{
	int i;

	for (i = 0; i < num_drivers; i++)
	{
		if (strcmp(name, s_available[i]->name) == 0)
		{
			return s_available[i];
		}
	}
	return NULL;
}

#if defined(USE_DRIVER_FUNCTIONS)
void
driver_terminate(void)
{
	(*g_driver->terminate)(g_driver);
}

#define METHOD_VOID(name_) \
void driver_##name_(void) { (*g_driver->name_)(g_driver); }
#define METHOD(type_,name_) \
type_ driver_##name_(void) { return (*g_driver->name_)(g_driver); }
#define METHOD_INT(name_) METHOD(int,name_)

METHOD_VOID(flush)

void
driver_schedule_alarm(int soon)
{
	(*g_driver->schedule_alarm)(g_driver, soon);
}

METHOD_INT(start_video)
METHOD_INT(end_video)
METHOD_VOID(window)
METHOD_INT(resize)
METHOD_VOID(redraw)
METHOD_INT(read_palette)
METHOD_INT(write_palette)

int
driver_read_pixel(int x, int y)
{
	return (*g_driver->read_pixel)(g_driver, x, y);
}

void
driver_write_pixel(int x, int y, int color)
{
	(*g_driver->write_pixel)(g_driver, x, y, color);
}

void
driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
	(*g_driver->read_span)(g_driver, y, x, lastx, pixels);
}

void
driver_write_span(int y, int x, int lastx, BYTE *pixels)
{
	(*g_driver->write_span)(g_driver, y, x, lastx, pixels);
}

void
driver_set_line_mode(int mode)
{
	(*g_driver->set_line_mode)(g_driver, mode);
}

void
driver_draw_line(int x1, int y1, int x2, int y2, int color)
{
	(*g_driver->draw_line)(g_driver, x1, y1, x2, y2, color);
}

int
driver_get_key(void)
{
	return (*g_driver->get_key)(g_driver);
}

int
driver_key_cursor(int row, int col)
{
	return (*g_driver->key_cursor)(g_driver, row, col);
}

int
driver_key_pressed(void)
{
	return (*g_driver->key_pressed)(g_driver);
}

int
driver_wait_key_pressed(int timeout)
{
	return (*g_driver->wait_key_pressed)(g_driver, timeout);
}

METHOD_VOID(shell)

void
driver_set_video_mode(int ax, int bx, int cx, int dx)
{
	(*g_driver->set_video_mode)(g_driver, ax, bx, cx, dx);
}

void
driver_put_string(int row, int col, int attr, const char *msg)
{
	(*g_driver->put_string)(g_driver, row, col, attr, msg);
}

METHOD_VOID(set_for_text)
METHOD_VOID(set_for_graphics)
METHOD_VOID(set_clear)

BYTE *
driver_find_font(int parm)
{
	return (*g_driver->find_font)(g_driver, parm);
}

void
driver_move_cursor(int row, int col)
{
	(*g_driver->move_cursor)(g_driver, row, col);
}

METHOD_VOID(hide_text_cursor)

void
driver_set_attr(int row, int col, int attr, int count)
{
	(*g_driver->set_attr)(g_driver, row, col, attr, count);
}

void
driver_scroll_up(int top, int bot)
{
	(*g_driver->scroll_up)(g_driver, top, bot);
}

METHOD_VOID(stack_screen)
METHOD_VOID(unstack_screen)
METHOD_VOID(discard_screen)

METHOD_INT(init_fm)

void
driver_buzzer(int kind)
{
	(*g_driver->buzzer)(g_driver, kind);
}

int
driver_sound_on(int freq)
{
	return (*g_driver->sound_on)(g_driver, freq);
}

METHOD_VOID(sound_off)
METHOD_VOID(mute)
METHOD_INT(diskp)

int
driver_get_char_attr(void)
{
	return (*g_driver->get_char_attr)(g_driver);
}

void
driver_put_char_attr(int char_attr)
{
	(*g_driver->put_char_attr)(g_driver, char_attr);
}

int
driver_validate_mode(VIDEOINFO *mode)
{
	return (*g_driver->validate_mode)(g_driver, mode);
}

void
driver_unget_key(int key)
{
	(*g_driver->unget_key)(g_driver, key);
}

#endif
