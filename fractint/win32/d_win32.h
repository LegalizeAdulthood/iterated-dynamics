#pragma once

#define WIN32_MAXSCREENS 10

typedef struct tagWin32BaseDriver Win32BaseDriver;
struct tagWin32BaseDriver
{
	driver pub;

	Frame frame;
	WinText wintext;

	/* key_buffer
	*
	* When we peeked ahead and saw a keypress, stash it here for later
	* feeding to our caller.
	*/
	int key_buffer;

	int screen_count;
	BYTE *saved_screens[WIN32_MAXSCREENS];
	int saved_cursor[WIN32_MAXSCREENS+1];
	BOOL cursor_shown;
	int cursor_row;
	int cursor_col;
};

extern void win32_shell(driver *drv);
extern int win32_key_pressed(drv);
extern void win32_terminate(driver *drv);
extern int win32_init(driver *drv, int *argc, char **argv);
extern int win32_key_pressed(driver *drv);
extern void win32_unget_key(driver *drv, int key);
extern int win32_get_key(driver *drv);
extern void win32_shell(driver *drv);
extern void win32_hide_text_cursor(driver *drv);
extern void win32_set_video_mode(driver *drv, VIDEOINFO *mode);
extern void win32_put_string(driver *drv, int row, int col, int attr, const char *msg);
extern void win32_scroll_up(driver *drv, int top, int bot);
extern void win32_move_cursor(driver *drv, int row, int col);
extern void win32_set_attr(driver *drv, int row, int col, int attr, int count);
extern void win32_stack_screen(driver *drv);
extern void win32_unstack_screen(driver *drv);
extern void win32_discard_screen(driver *drv);
extern int win32_init_fm(driver *drv);
extern void win32_buzzer(driver *drv, int kind);
extern int win32_sound_on(driver *drv, int freq);
extern void win32_sound_off(driver *drv);
extern void win32_mute(driver *drv);
extern int win32_diskp(driver *drv);
extern int win32_key_cursor(driver *drv, int row, int col);
extern int win32_wait_key_pressed(driver *drv, int timeout);
extern int win32_get_char_attr(driver *drv);
extern void win32_put_char_attr(driver *drv, int char_attr);
extern int win32_get_char_attr_rowcol(driver *drv, int row, int col);
extern void win32_put_char_attr_rowcol(driver *drv, int row, int col, int char_attr);
extern void win32_delay(driver *drv, int ms);
extern void win32_get_truecolor(driver *drv, int x, int y, int *r, int *g, int *b, int *a);
extern void win32_put_truecolor(driver *drv, int x, int y, int r, int g, int b, int a);
extern void win32_set_keyboard_timeout(driver *drv, int ms);

#define WIN32_DRIVER_STRUCT(base_, desc_) STD_DRIVER_STRUCT(base_, desc_)
