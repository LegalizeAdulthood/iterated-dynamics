#if !defined(DRIVERS_H)
#define DRIVERS_H

/* AbstractDriver
 *
 * Abstract interface for a user interface driver for FractInt.  The sound
 * routines should be refactored out of this object at some point.
 */
class AbstractDriver
{
public:
	/* name of driver */				virtual const char *name() const = 0;
	/* driver description */			virtual const char *description() const = 0;
	/* initialize the driver */			virtual int initialize(int *argc, char **argv) = 0;
	/* shutdown the driver */			virtual void terminate() = 0;
	/* pause this driver */				virtual void pause() = 0;
	/* resume this driver */			virtual void resume() = 0;

	/* validate a fractint.cfg mode */	virtual int validate_mode(const VIDEOINFO &mode) = 0;
										virtual void set_video_mode(const VIDEOINFO &mode) = 0;
	/* find max screen extents */		virtual void get_max_screen(int &x_max, int &y_max) const = 0;

	/* creates a window */				virtual void window() = 0;
	/* handles window resize.  */		virtual int resize() = 0;
	/* redraws the screen */			virtual void redraw() = 0;

	/* read palette into g_dac_box */	virtual int read_palette() = 0;
	/* write g_dac_box into palette */	virtual int write_palette() = 0;

	/* reads a single pixel */			virtual int read_pixel(int x, int y) = 0;
	/* writes a single pixel */			virtual void write_pixel(int x, int y, int color) = 0;
	/* reads a span of pixel */			virtual void read_span(int y, int x, int lastx, BYTE *pixels) = 0;
	/* writes a span of pixels */		virtual void write_span(int y, int x, int lastx, const BYTE *pixels) = 0;
										virtual void get_truecolor(int x, int y, int &r, int &g, int &b, int &a) = 0;
										virtual void put_truecolor(int x, int y, int r, int g, int b, int a) = 0;
	/* set copy/xor line */				virtual void set_line_mode(int mode) = 0;
	/* draw line */						virtual void draw_line(int x1, int y1, int x2, int y2, int color) = 0;
	/* draw string in graphics mode */	virtual void display_string(int x, int y, int fg, int bg, const char *text) = 0;
	/* save graphics */					virtual void save_graphics() = 0;
	/* restore graphics */				virtual void restore_graphics() = 0;
	/* poll or block for a key */		virtual int get_key() = 0;
										virtual void unget_key(int key) = 0;
										virtual int key_cursor(int row, int col) = 0;
										virtual int key_pressed() = 0;
										virtual int wait_key_pressed(int timeout) = 0;
										virtual void set_keyboard_timeout(int ms) = 0;
	/* invoke a command shell */		virtual void shell() = 0;
	/* set for text mode & save gfx */	virtual void set_for_text() = 0;
	/* restores graphics and data */	virtual void set_for_graphics() = 0;
	/* clears text screen */			virtual void set_clear() = 0;
	/* text screen functions */			virtual void move_cursor(int row, int col) = 0;
										virtual void hide_text_cursor() = 0;
										virtual void put_string(int row, int col, int attr, const char *text) = 0;
										virtual void set_attr(int row, int col, int attr, int count) = 0;
										virtual void scroll_up(int top, int bottom) = 0;
										virtual void stack_screen() = 0;
										virtual void unstack_screen() = 0;
										virtual void discard_screen() = 0;
										virtual int get_char_attr() = 0;
										virtual void put_char_attr(int char_attr) = 0;
										virtual int get_char_attr_rowcol(int row, int col) = 0;
										virtual void put_char_attr_rowcol(int row, int col, int char_attr) = 0;

	/* sound routines */				virtual int init_fm() = 0;
										virtual void buzzer(int kind) = 0;
										virtual int sound_on(int frequency) = 0;
										virtual void sound_off() = 0;
										virtual void mute() = 0;

	/* returns true if disk video */	virtual int diskp() = 0;

										virtual void delay(int ms) = 0;
										virtual void flush() = 0;
	/* refresh alarm */					virtual void schedule_alarm(int secs) = 0;
};

class DriverManager
{
public:
	static int open_drivers(int *argc, char **argv);
	static void close_drivers();
	static AbstractDriver *find_by_name(const char *name);
	static void change_video_mode(VIDEOINFO &mode);

private:
	static int load(AbstractDriver *driver, int *argc, char **argv);

	static const int MAX_DRIVERS = 10;
	static int s_num_drivers;
	static AbstractDriver *s_drivers[MAX_DRIVERS];
};

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
#if defined(_WIN32)
#define HAVE_X11_DRIVER			0
#define HAVE_GDI_DRIVER			1
#define HAVE_WIN32_DISK_DRIVER	1
#endif

extern int init_drivers(int *argc, char **argv);
extern void add_video_mode(AbstractDriver *driver, VIDEOINFO &mode);
extern void close_drivers();

extern const char *driver_name();
extern const char *driver_description();
extern void driver_set_video_mode(VIDEOINFO &mode);
extern int driver_validate_mode(VIDEOINFO &mode);
extern void driver_get_max_screen(int &x_max, int &y_max);
extern void driver_terminate();
// pause and resume are only used internally in drivers.c
extern void driver_schedule_alarm(int secs);
extern void driver_window();
extern int driver_resize();
extern void driver_redraw();
extern int driver_read_palette();
extern int driver_write_palette();
extern int driver_read_pixel(int x, int y);
extern void driver_write_pixel(int x, int y, int color);
extern void driver_read_span(int y, int x, int lastx, BYTE *pixels);
extern void driver_write_span(int y, int x, int lastx, const BYTE *pixels);
extern void driver_get_truecolor(int x, int y, int &r, int &g, int &b, int &a);
extern void driver_put_truecolor(int x, int y, int r, int g, int b, int a);
extern void driver_set_line_mode(int mode);
extern void driver_draw_line(int x1, int y1, int x2, int y2, int color);
extern int driver_get_key();
extern void driver_display_string(int x, int y, int fg, int bg, const char *text);
extern void driver_save_graphics();
extern void driver_restore_graphics();
extern int driver_key_cursor(int row, int col);
extern int driver_key_pressed();
extern int driver_wait_key_pressed(int timeout);
extern void driver_unget_key(int key);
extern void driver_shell();
extern void driver_put_string(int row, int col, int attr, const char *msg);
extern void driver_set_for_text();
extern void driver_set_for_graphics();
extern void driver_set_clear();
extern void driver_move_cursor(int row, int col);
extern void driver_hide_text_cursor();
extern void driver_set_attr(int row, int col, int attr, int count);
extern void driver_scroll_up(int top, int bot);
extern void driver_stack_screen();
extern void driver_unstack_screen();
extern void driver_discard_screen();
extern int driver_init_fm();
extern void driver_buzzer(int kind);
extern int driver_sound_on(int frequency);
extern void driver_sound_off();
extern void driver_mute();
extern int driver_diskp();
extern int driver_get_char_attr();
extern void driver_put_char_attr(int char_attr);
extern void driver_delay(int ms);
extern void driver_set_keyboard_timeout(int ms);
extern void driver_flush();
extern int driver_get_char_attr_rowcol(int row, int col);
extern void driver_put_char_attr_rowcol(int row, int col, int char_attr);

#endif /* DRIVERS_H */
