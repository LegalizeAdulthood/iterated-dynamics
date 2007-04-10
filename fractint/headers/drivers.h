#if !defined(DRIVERS_H)
#define DRIVERS_H

/*------------------------------------------------------------
 * driver Methods:
 *
 * init
 * Initialize the driver and return non-zero on success.
 *
 * terminate
 * schedule_alarm
 *
 * window
 * Do whatever is necessary to open up a window after the screen size
 * has been set.  In a windowed environment, the window may provide
 * scroll bars to pan a cropped view across the screen.
 *
 * resize
 * redraw
 * read_palette, write_palette
 * read_pixel, write_pixel
 * read_span, write_span
 * set_line_mode
 * draw_line
 * get_key
 * key_cursor
 * key_pressed
 * wait_key_pressed
 * unget_key
 * shell
 * set_video_mode
 * put_string
 * set_for_text, set_for_graphics, set_clear
 * move_cursor
 * hide_text_cursor
 * set_attr
 * scroll_up
 * stack_screen, unstack_screen, discard_screen
 * get_char_attr, put_char_attr
 */
struct tag_driver
{
	/* name of driver */				const char *name;
	/* driver description */			const char *description;
	/* init the driver */				int (*init)(driver *drv, int *argc, char **argv);
	/* validate a fractint.cfg mode */	int (*validate_mode)(driver *drv, VIDEOINFO *mode);
	/* find max screen extents */		void (*get_max_screen)(driver *drv, int *g_x_max, int *g_y_max);
	/* shutdown the driver */			void (*terminate)(driver *drv);
	/* pause this driver */				void (*pause)(driver *drv);
	/* resume this driver */			void (*resume)(driver *drv);
	/* refresh alarm */					void (*schedule_alarm)(driver *drv, int secs);
	/* creates a window */				void (*window)(driver *drv);
	/* handles window resize.  */		int (*resize)(driver *drv);
	/* redraws the screen */			void (*redraw)(driver *drv);
	/* read palette into g_dac_box */	int (*read_palette)(driver *drv);
	/* write g_dac_box into palette */	int (*write_palette)(driver *drv);
	/* reads a single pixel */			int (*read_pixel)(driver *drv, int x, int y);
	/* writes a single pixel */			void (*write_pixel)(driver *drv, int x, int y, int color);
	/* reads a span of pixel */			void (*read_span)(driver *drv, int y, int x, int lastx, BYTE *pixels);
	/* writes a span of pixels */		void (*write_span)(driver *drv, int y, int x, int lastx, BYTE *pixels);
										void (*get_truecolor)(driver *drv, int x, int y, int *r, int *g, int *b, int *a);
										void (*put_truecolor)(driver *drv, int x, int y, int r, int g, int b, int a);
	/* set copy/xor line */				void (*set_line_mode)(driver *drv, int mode);
	/* draw line */						void (*draw_line)(driver *drv, int x1, int y1, int x2, int y2, int color);
	/* draw string in graphics mode */	void (*display_string)(driver *drv, int x, int y, int fg, int bg, const char *text);
	/* save graphics */					void (*save_graphics)(driver *drv);
	/* restore graphics */				void (*restore_graphics)(driver *drv);
	/* poll or g_block for a key */		int (*get_key)(driver *drv);
										int (*key_cursor)(driver *drv, int row, int col);
										int (*key_pressed)(driver *drv);
										int (*wait_key_pressed)(driver *drv, int timeout);
										void (*unget_key)(driver *drv, int key);
	/* invoke a command shell */		void (*shell)(driver *drv);
										void (*set_video_mode)(driver *drv, VIDEOINFO *mode);
										void (*put_string)(driver *drv, int row, int col, int attr, const char *msg);
	/* set for text mode & save gfx */	void (*set_for_text)(driver *drv);
	/* restores graphics and data */	void (*set_for_graphics)(driver *drv);
	/* clears text screen */			void (*set_clear)(driver *drv);
	/* text screen functions */
										void (*move_cursor)(driver *drv, int row, int col);
										void (*hide_text_cursor)(driver *drv);
										void (*set_attr)(driver *drv, int row, int col, int attr, int count);
										void (*scroll_up)(driver *drv, int top, int bot);
										void (*stack_screen)(driver *drv);
										void (*unstack_screen)(driver *drv);
										void (*discard_screen)(driver *drv);
	/* sound routines */
										int (*init_fm)(driver *drv);
										void (*buzzer)(driver *drv, int kind);
										int (*sound_on)(driver *drv, int frequency);
										void (*sound_off)(driver *drv);
										void (*mute)(driver *drv);

										int (*diskp)(driver *drv);
										int (*get_char_attr)(driver *drv);
										void (*put_char_attr)(driver *drv, int char_attr);
										int (*get_char_attr_rowcol)(driver *drv, int row, int col);
										void (*put_char_attr_rowcol)(driver *drv, int row, int col, int char_attr);
										void (*delay)(driver *drv, int ms);
										void (*set_keyboard_timeout)(driver *drv, int ms);
										void (*flush)(driver *drv);
};

#define STD_DRIVER_STRUCT(name_, desc_) \
  { \
	#name_, desc_, \
    name_##_init, \
	name_##_validate_mode, \
	name_##_get_max_screen, \
    name_##_terminate, \
	name_##_pause, \
	name_##_resume, \
    name_##_schedule_alarm, \
    name_##_window, \
    name_##_resize, \
    name_##_redraw, \
    name_##_read_palette, \
    name_##_write_palette, \
    name_##_read_pixel, \
    name_##_write_pixel, \
    name_##_read_span, \
    name_##_write_span, \
	name_##_get_truecolor, \
	name_##_put_truecolor, \
    name_##_set_line_mode, \
    name_##_draw_line, \
	name_##_display_string, \
	name_##_save_graphics, \
	name_##_restore_graphics, \
    name_##_get_key, \
	name_##_key_cursor, \
	name_##_key_pressed, \
	name_##_wait_key_pressed, \
	name_##_unget_key, \
    name_##_shell, \
    name_##_set_video_mode, \
    name_##_put_string, \
    name_##_set_for_text, \
    name_##_set_for_graphics, \
    name_##_set_clear, \
    name_##_move_cursor, \
    name_##_hide_text_cursor, \
    name_##_set_attr, \
    name_##_scroll_up, \
    name_##_stack_screen, \
    name_##_unstack_screen, \
    name_##_discard_screen, \
    name_##_init_fm, \
    name_##_buzzer, \
    name_##_sound_on, \
    name_##_sound_off, \
	name_##_mute, \
    name_##_diskp, \
	name_##_get_char_attr, \
	name_##_put_char_attr, \
	name_##_get_char_attr_rowcol, \
	name_##_put_char_attr_rowcol, \
	name_##_delay, \
	name_##_set_keyboard_timeout, \
	name_##_flush \
  }

/* Define the drivers to be included in the compilation:

    HAVE_CURSES_DRIVER		Curses based disk driver
    HAVE_X11_DRIVER			XFractint code path
    HAVE_GDI_DRIVER			Win32 GDI driver
    HAVE_WIN32_DISK_DRIVER	Win32 disk driver
*/
#if defined(XFRACT)
#define HAVE_X11_DRIVER			1
#define HAVE_GDI_DRIVER			0
#define HAVE_WIN32_DISK_DRIVER	0
#endif
#if defined(MSDOS)
#define HAVE_X11_DRIVER			0
#define HAVE_GDI_DRIVER			0
#define HAVE_WIN32_DISK_DRIVER	0
#endif
#if defined(_WIN32)
#define HAVE_X11_DRIVER			0
#define HAVE_GDI_DRIVER			1
#define HAVE_WIN32_DISK_DRIVER	1
#endif

extern int init_drivers(int *argc, char **argv);
extern void add_video_mode(driver *drv, VIDEOINFO *mode);
extern void close_drivers(void);
extern driver *driver_find_by_name(const char *name);

extern driver *g_driver;			/* current driver in use */

/* always use a function for this one */
extern void driver_set_video_mode(VIDEOINFO *mode);

#define USE_DRIVER_FUNCTIONS 1

#if defined(USE_DRIVER_FUNCTIONS)

extern int driver_validate_mode(VIDEOINFO *mode);
extern void driver_get_max_screen(int *g_x_max, int *g_y_max);
extern void driver_terminate(void);
// pause and resume are only used internally in drivers.c
extern void driver_schedule_alarm(int secs);
extern void driver_window(void);
extern int driver_resize(void);
extern void driver_redraw(void);
extern int driver_read_palette(void);
extern int driver_write_palette(void);
extern int driver_read_pixel(int x, int y);
extern void driver_write_pixel(int x, int y, int color);
extern void driver_read_span(int y, int x, int lastx, BYTE *pixels);
extern void driver_write_span(int y, int x, int lastx, BYTE *pixels);
extern void driver_get_truecolor(int x, int y, int *r, int *g, int *b, int *a);
extern void driver_put_truecolor(int x, int y, int r, int g, int b, int a);
extern void driver_set_line_mode(int mode);
extern void driver_draw_line(int x1, int y1, int x2, int y2, int color);
extern int driver_get_key(void);
extern void driver_display_string(int x, int y, int fg, int bg, const char *text);
extern void driver_save_graphics(void);
extern void driver_restore_graphics(void);
extern int driver_key_cursor(int row, int col);
extern int driver_key_pressed(void);
extern int driver_wait_key_pressed(int timeout);
extern void driver_unget_key(int key);
extern void driver_shell(void);
extern void driver_put_string(int row, int col, int attr, const char *msg);
extern void driver_set_for_text(void);
extern void driver_set_for_graphics(void);
extern void driver_set_clear(void);
extern void driver_move_cursor(int row, int col);
extern void driver_hide_text_cursor(void);
extern void driver_set_attr(int row, int col, int attr, int count);
extern void driver_scroll_up(int top, int bot);
extern void driver_stack_screen(void);
extern void driver_unstack_screen(void);
extern void driver_discard_screen(void);
extern int driver_init_fm(void);
extern void driver_buzzer(int kind);
extern int driver_sound_on(int frequency);
extern void driver_sound_off(void);
extern void driver_mute(void);
extern int driver_diskp(void);
extern int driver_get_char_attr(void);
extern void driver_put_char_attr(int char_attr);
extern void driver_delay(int ms);
extern void driver_set_keyboard_timeout(int ms);
extern void driver_flush(void);
extern int driver_get_char_attr_rowcol(int row, int col);
extern void driver_put_char_attr_rowcol(int row, int col, int char_attr);

#else

#define driver_validate_mode(mode_)					(*g_driver->validate_mode)(g_driver, mode_)
#define driver_get_max_screen(xmax_, ymax_)			(*g_driver->get_max_screen)(g_driver, xmax_, ymax_)
#define driver_terminate()							(*g_driver->terminate)(g_driver)
// pause and resume are only used internally in drivers.c
#define void driver_schedule_alarm(_secs)			(*g_driver->schedule_alarm)(g_driver, _secs)
#define driver_window()								(*g_driver->window)(g_driver)
#define driver_resize()								(*g_driver->resize)(g_driver)
#define driver_redraw()								(*g_driver->redraw)(g_driver)
#define driver_read_palette()						(*g_driver->read_palette)(g_driver)
#define driver_write_palette()						(*g_driver->write_palette)(g_driver)
#define driver_read_pixel(_x, _y)					(*g_driver->read_pixel)(g_driver, _x, _y)
#define driver_write_pixel(_x, _y, _color)			(*g_driver->write_pixel)(g_driver, _x, _y, _color)
#define driver_read_span(_y, _x, _lastx, _pixels)	(*g_driver->read_span(g_driver, _y, _x, _lastx, _pixels)
#define driver_write_span(_y, _x, _lastx, _pixels)	(*g_driver->write_span)(g_driver, _y, _x, _lastx, _pixels)
#define driver_get_truecolor(_x,_y, _r,_g,_b,_a)	(*g_driver->get_truecolor)(g_driver, _x, _y, _r, _g, _b, _a)
#define driver_put_truecolor(_x,_y, _r,_g,_b,_a)	(*g_driver->put_trueoclor)(g_driver, _x, _y, _r, _g, _b, _a)
#define driver_set_line_mode(_m)					(*g_driver->set_line_mode)(g_driver, _m)
#define driver_draw_line(x1_, y1_, x2_, y2_, clr_)	(*g_driver->draw_line)(g_driver, x1_, y1_, x1_, y2_, clr_)
#define driver_display_string(x_,y_,fg_,bg_,str_)	(*g_driver->display_string)(g_driver, x_, y_, fg_, bg_, str_)
#define driver_save_graphics()						(*g_driver->save_graphics)(g_driver)
#define driver_restore_graphics()					(*g_driver->restore_graphics)(g_driver)
#define driver_get_key()							(*g_driver->get_key)(g_driver)
#define driver_key_cursor(row_, col_)				(*g_driver->key_cursor)(g_driver, row_, col_)
#define driver_key_pressed()						(*g_driver->key_pressed)(g_driver)
#define driver_wait_key_pressed(timeout_)			(*g_driver->wait_key_pressed)(g_driver, timeout_)
#define driver_unget_key(key_)						(*g_driver->unget_key)(g_driver, key_)
#define driver_shell()								(*g_driver->shell)(g_driver)
#define driver_put_string(_row, _col, _attr, _msg)	(*g_driver->put_string)(g_driver, _row, _col, _attr, _msg)
#define driver_set_for_text()						(*g_driver->set_for_text)(g_driver)
#define driver_set_for_graphics()					(*g_driver->set_for_graphics)(g_driver)
#define driver_set_clear()							(*g_driver->set_clear)(g_driver)
#define driver_move_cursor(_row, _col)				(*g_driver->move_cursor)(g_driver, _row, _col)
#define driver_hide_text_cursor()					(*g_driver->hide_text_cursor)(g_driver)
#define driver_set_attr(_row, _col, _attr, _count)	(*g_driver->set_attr)(g_driver, _row, _col, _attr, _count)
#define driver_scroll_up(_top, _bot)				(*g_driver->scroll_up)(g_driver, _top, _bot)
#define driver_stack_screen()						(*g_driver->stack_screen)(g_driver)
#define driver_unstack_screen()						(*g_driver->unstack_screen)(g_driver)
#define driver_discard_screen()						(*g_driver->discard_screen)(g_driver)
#define driver_init_fm()							(*g_driver->init_fm)(g_driver)
#define driver_buzzer(_kind)						(*g_driver->buzzer)(g_driver, _kind)
#define driver_sound_on(_freq)						(*g_driver->sound_on)(g_driver, _freq)
#define driver_sound_off()							(*g_driver->sound_off)(g_driver)
#define driver_mute()								(*g_driver->mute)(g_driver)
#define driver_diskp()								(*g_driver->diskp)(g_driver)
#define driver_get_char_attr()						(*g_driver->get_char_attr)(g_driver)
#define driver_put_char_attr(char_attr_)			(*g_driver->put_char_attr)(g_driver, char_attr_)
#define driver_delay(ms_)							(*g_driver->delay)(g_driver, ms_)
#define driver_set_keyboard_timeout(ms_)			(*g_driver->set_keyboard_timeout)(g_driver, ms_)
#define driver_flush()								(*g_driver->flush)(g_driver)
#define driver_get_char_attr_rowcol(row_, col_)		(*g_driver->get_char_attr_rowcol)(g_driver, row_, col_)
#define driver_put_char_attr_rowcol(r_, c_, a_)		(*g_driver->set_char_attr_rowcol)(g_driver, r_, c_, a_)

#endif

#endif /* DRIVERS_H */
