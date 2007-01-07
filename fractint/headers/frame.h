#if !defined(FRAME_H)
#define FRAME_H

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
};

extern Frame g_frame;

extern void frame_init(HINSTANCE instance, LPCSTR title);
extern void frame_window(int width, int height);
extern void frame_set_child(HWND child);

#endif
