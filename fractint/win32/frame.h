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
	int get_key_press(bool wait_for_key);
	int pump_messages(bool wait_flag);
	void resize(int width, int height);
	void set_keyboard_timeout(int ms);
	void set_mouse_mode(int new_mode);
	int get_mouse_mode() const;

	HWND window() const;
	int width() const;
	int height() const;

	static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp);

private:
	static FrameImpl s_impl;
};

#endif
