#pragma once

#define WIN32_MAXSCREENS 10

typedef struct tagWin32BaseDriver Win32BaseDriver;
struct tagWin32BaseDriver
{
	Driver pub;

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

extern void win32_shell(Driver *drv);
extern void win32_terminate(Driver *drv);
extern int win32_init(Driver *drv, int *argc, char **argv);
extern int win32_key_pressed(Driver *drv);
extern void win32_unget_key(Driver *drv, int key);
extern int win32_get_key(Driver *drv);
extern void win32_shell(Driver *drv);
extern void win32_hide_text_cursor(Driver *drv);
extern void win32_set_video_mode(Driver *drv, VIDEOINFO *mode);
extern void win32_put_string(Driver *drv, int row, int col, int attr, const char *msg);
extern void win32_scroll_up(Driver *drv, int top, int bot);
extern void win32_move_cursor(Driver *drv, int row, int col);
extern void win32_set_attr(Driver *drv, int row, int col, int attr, int count);
extern void win32_stack_screen(Driver *drv);
extern void win32_unstack_screen(Driver *drv);
extern void win32_discard_screen(Driver *drv);
extern int win32_init_fm(Driver *drv);
extern void win32_buzzer(Driver *drv, int kind);
extern int win32_sound_on(Driver *drv, int freq);
extern void win32_sound_off(Driver *drv);
extern void win32_mute(Driver *drv);
extern int win32_diskp(Driver *drv);
extern int win32_key_cursor(Driver *drv, int row, int col);
extern int win32_wait_key_pressed(Driver *drv, int timeout);
extern int win32_get_char_attr(Driver *drv);
extern void win32_put_char_attr(Driver *drv, int char_attr);
extern int win32_get_char_attr_rowcol(Driver *drv, int row, int col);
extern void win32_put_char_attr_rowcol(Driver *drv, int row, int col, int char_attr);
extern void win32_delay(Driver *drv, int ms);
extern void win32_get_truecolor(Driver *drv, int x, int y, int *r, int *g, int *b, int *a);
extern void win32_put_truecolor(Driver *drv, int x, int y, int r, int g, int b, int a);
extern void win32_set_keyboard_timeout(Driver *drv, int ms);

#define WIN32_DRIVER_STRUCT(base_, desc_) STD_DRIVER_STRUCT(base_, desc_)
