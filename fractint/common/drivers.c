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
static Driver *available[MAX_DRIVERS];
static Driver *mode_drivers[MAXVIDEOMODES];

Driver *display = NULL;

#define NUM_OF(array_) (sizeof(array_)/sizeof(array_[0]))

static int
load_driver(Driver *drv, int *argc, char **argv)
{
	if (drv && drv->init)
	{
		const int num = (*drv->init)(drv, argc, argv);
		if (num > 0)
		{
			if (! display)
			{
				display = drv;
			}
			available[num_drivers++] = drv;
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
	vidtbl = (VIDEOINFO *) malloc(sizeof(vidtbl[0])*MAXVIDEOMODES);

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
  /* stash away driver pointer so we can init driver for selected mode */
  mode_drivers[vidtbllen] = drv;
  memcpy(&videotable[vidtbllen], mode, sizeof(videotable[0]));
  vidtbllen++;
}

void
close_drivers(void)
{
  int i;

  for (i = 0; i < num_drivers; i++)
    if (available[i]) {
      (*available[i]->terminate)(available[i]);
      available[i] = NULL;
    }

  if (vidtbl)
    free(vidtbl);
  vidtbl = NULL;
  display = NULL;
}

#if defined(USE_DRIVER_FUNCTIONS)
void
driver_terminate(void)
{
  (*display->terminate)(display);
}

#define METHOD_VOID(name_) \
void driver_##name_(void) { (*display->name_)(display); }
#define METHOD(type_,name_) \
type_ driver_##name_(void) { return (*display->name_)(display); }
#define METHOD_INT(name_) METHOD(int,name_)

METHOD_VOID(flush)

void
driver_schedule_alarm(int soon)
{
  (*display->schedule_alarm)(display, soon);
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
  return (*display->read_pixel)(display, x, y);
}

void
driver_write_pixel(int x, int y, int color)
{
  (*display->write_pixel)(display, x, y, color);
}

void
driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
  (*display->read_span)(display, y, x, lastx, pixels);
}

void
driver_write_span(int y, int x, int lastx, BYTE *pixels)
{
  (*display->write_span)(display, y, x, lastx, pixels);
}

void
driver_set_line_mode(int mode)
{
  (*display->set_line_mode)(display, mode);
}

void
driver_draw_line(int x1, int y1, int x2, int y2)
{
  (*display->draw_line)(display, x1, y1, x2, y2);
}

int
driver_get_key(int block)
{
  return (*display->get_key)(display, block);
}

METHOD_VOID(shell)

void
driver_set_video_mode(int ax, int bx, int cx, int dx)
{
  (*display->set_video_mode)(display, ax, bx, cx, dx);
}

void
driver_put_string(int row, int col, int attr, const char *msg)
{
  (*display->put_string)(display, row, col, attr, msg);
}

METHOD_VOID(set_for_text)
METHOD_VOID(set_for_graphics)
METHOD_VOID(set_clear)

BYTE *
driver_find_font(int parm)
{
  return (*display->find_font)(display, parm);
}

void
driver_move_cursor(int row, int col)
{
  (*display->move_cursor)(display, row, col);
}

METHOD_VOID(hide_text_cursor)

void
driver_set_attr(int row, int col, int attr, int count)
{
  (*display->set_attr)(display, row, col, attr, count);
}

void
driver_scroll_up(int top, int bot)
{
  (*display->scroll_up)(display, top, bot);
}

METHOD_VOID(stack_screen)
METHOD_VOID(unstack_screen)
METHOD_VOID(discard_screen)

METHOD_INT(init_fm)

void
driver_buzzer(int kind)
{
  (*display->buzzer)(display, kind);
}

int
driver_sound_on(int freq)
{
  return (*display->sound_on)(display, freq);
}

METHOD_VOID(sound_off)
METHOD_VOID(mute)
METHOD_INT(diskp)

#endif
