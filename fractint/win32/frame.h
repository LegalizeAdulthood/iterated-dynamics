#if !defined(FRAME_H)
#define FRAME_H

#define KEYBUFMAX 80

#define BUTTON_LEFT 0
#define BUTTON_RIGHT 1
#define BUTTON_MIDDLE 2

typedef struct tagFrame Frame;
struct tagFrame
{
	HINSTANCE instance;
	HWND window;
	char title[80];
	int width;
	int height;
	int nc_width;
	int nc_height;
	HWND child;
	BOOL has_focus;
	BOOL timed_out;

	/* the keypress buffer */
	unsigned int  keypress_count;
	unsigned int  keypress_head;
	unsigned int  keypress_tail;
	unsigned int  keypress_buffer[KEYBUFMAX];

	/* mouse data */
	BOOL button_down[3];
	int start_x, start_y;
	int delta_x, delta_y;
};

extern Frame g_frame;

extern void frame_init(HINSTANCE instance, LPCSTR title);
extern void frame_window(int width, int height);
extern int frame_key_pressed(void);
extern int frame_get_key_press(int option);
extern int frame_pump_messages(int waitflag);
extern void frame_schedule_alarm(int soon);
extern void frame_resize(int width, int height);
extern void frame_set_keyboard_timeout(int ms);

extern LRESULT CALLBACK frame_proc(HWND window, UINT message, WPARAM wp, LPARAM lp);

#endif
