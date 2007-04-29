#if !defined(FRAME_H)
#define FRAME_H

#define KEYBUFMAX 80

#define BUTTON_LEFT 0
#define BUTTON_RIGHT 1
#define BUTTON_MIDDLE 2

class FrameImpl;

class Frame
{
public:
	Frame() {}
	void init(HINSTANCE instance, LPCTSTR title);
	void create(int width, int height);
	int get_key_press(int option);
	int pump_messages(int waitflag);
	void resize(int width, int height);
	void set_keyboard_timeout(int ms);

	HWND window() const;
	int width() const;
	int height() const;

	static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp);

private:
	static FrameImpl s_impl;
};

#endif
