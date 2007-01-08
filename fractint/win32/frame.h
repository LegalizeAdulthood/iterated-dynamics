#if !defined(FRAME_H)
#define FRAME_H

#define KEYBUFMAX 80

typedef struct tagFrame Frame;
struct tagFrame
{
	HINSTANCE instance;
	HWND window;
	char title[80];
	int width;
	int height;
	HWND child;
	BOOL has_focus;

	/* the keypress buffer */
	unsigned int  keypress_count;
	unsigned int  keypress_head;
	unsigned int  keypress_tail;
	unsigned int  keypress_buffer[KEYBUFMAX];
};

extern Frame g_frame;

extern void frame_init(HINSTANCE instance, LPCSTR title);
extern void frame_window(int width, int height);
extern void frame_set_child(HWND child);
extern int frame_key_pressed(Frame *frame);

#endif
