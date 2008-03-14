#pragma once

#include <stdexcept>
#include <vector>

class not_implemented : public std::exception
{
public:
	explicit not_implemented(char const *what) : std::exception(what)
	{
	}
	explicit not_implemented() : std::exception()
	{
	}
	virtual ~not_implemented()
	{
	}
};

class FakeDriver : public AbstractDriver
{
public:
	FakeDriver() : AbstractDriver(),
		_waitKeyPressedCalled(false),
		_waitKeyPressedLastTimeout(0),
		_waitKeyPressedFakeResult(0),
		_fakeMouseMode(0),
		_setMouseModeCalled(false),
		_setMouseModeLastMode(0),
		_keyStrokes(),
		_keyStrokeCount(0),
		_putStringArgs()
	{
		int argc = 0;
		char *argv[] = { 0 };
		DriverManager::load(this, argc, argv);
	}
	virtual ~FakeDriver()
	{
		DriverManager::unload(this);
	}

	void SetKeyStrokes(int numKeys, const int *keys)
	{
		std::copy(&keys[0], &keys[numKeys], std::back_inserter<std::vector<int> >(_keyStrokes));
	}
	void SetFakeMouseMode(int value) { _fakeMouseMode = value; }
	bool SetMouseModeCalled() const { return _setMouseModeCalled; }
	int SetMouseModeLastMode() const { return _setMouseModeLastMode; }
	bool WaitKeyPressedCalled() const { return _waitKeyPressedCalled; }
	int WaitKeyPressedLastTimeout() const { return _waitKeyPressedLastTimeout; }
	void SetWaitKeyPressedFakeResult(int value) { _waitKeyPressedFakeResult = value; }

	struct PutStringArg
	{
		PutStringArg(int row_, int col_, int attr_, const char *msg_)
			: row(row_), col(col_), attr(attr_), msg(msg_)
		{
		}
		int row;
		int col;
		int attr;
		std::string msg;
	};
	const std::vector<PutStringArg> &PutStringArgs() const { return _putStringArgs; }

	virtual int get_key_no_help()
	{
		return (_keyStrokeCount < _keyStrokes.size()) ? _keyStrokes[_keyStrokeCount++] : 0;
	}

	virtual void set_attr(int row, int col, int attr, int count)
	{
	}
	std::vector<PutStringArg> _putStringArgs;
	virtual void put_string(int row, int col, int attr, const char *msg)
	{
		_putStringArgs.push_back(PutStringArg(row, col, attr, msg));
	}
	virtual void hide_text_cursor()
	{
	}
	virtual void buzzer(int kind)
	{
	}
	virtual int key_pressed()
	{
		return _keyStrokes.size() > 0 ? _keyStrokes[_keyStrokeCount] : 0;
	}
	virtual int get_key()
	{
		return (_keyStrokeCount < _keyStrokes.size()) ? _keyStrokes[_keyStrokeCount++] : 0;
	}
	virtual void unstack_screen()
	{
	}

	virtual const char *name() const { throw not_implemented("char *name"); }
	virtual const char *description() const { throw not_implemented("char *description"); }
	virtual bool initialize(int &argc, char **argv) { return true; }
	virtual void terminate() { throw not_implemented("terminate"); }
	virtual void pause() { throw not_implemented("pause"); }
	virtual void resume() { throw not_implemented("resume"); }

	virtual int validate_mode(const VIDEOINFO &mode) { throw not_implemented("validate_mode"); }
	virtual void set_video_mode(const VIDEOINFO &mode) { throw not_implemented("set_video_mode"); }
	virtual void get_max_screen(int &x_max, int &y_max) const { throw not_implemented("get_max_screen"); }

	virtual void window() { throw not_implemented("window"); }
	virtual int resize() { throw not_implemented("resize"); }
	virtual void redraw() { throw not_implemented("redraw"); }

	virtual int read_palette() { throw not_implemented("read_palette"); }
	virtual int write_palette() { throw not_implemented("write_palette"); }

	virtual int read_pixel(int x, int y) { throw not_implemented("read_pixel"); }
	virtual void write_pixel(int x, int y, int color) { throw not_implemented("write_pixel"); }
	virtual void read_span(int y, int x, int lastx, BYTE *pixels) { throw not_implemented("read_span"); }
	virtual void write_span(int y, int x, int lastx, const BYTE *pixels) { throw not_implemented("write_span"); }
	virtual void get_truecolor(int x, int y, int &r, int &g, int &b, int &a) { throw not_implemented("get_truecolor"); }
	virtual void put_truecolor(int x, int y, int r, int g, int b, int a) { throw not_implemented("put_truecolor"); }
	virtual void set_line_mode(int mode) { throw not_implemented("set_line_mode"); }
	virtual void draw_line(int x1, int y1, int x2, int y2, int color) { throw not_implemented("draw_line"); }
	virtual void display_string(int x, int y, int fg, int bg, const char *text) { throw not_implemented("display_string"); }
	virtual void save_graphics() { throw not_implemented("save_graphics"); }
	virtual void restore_graphics() { throw not_implemented("restore_graphics"); }
	virtual void unget_key(int key) { throw not_implemented("unget_key"); }
	virtual int key_cursor(int row, int col) { throw not_implemented("key_cursor"); }
	virtual int wait_key_pressed(int timeout)
	{
		_waitKeyPressedCalled = true;
		_waitKeyPressedLastTimeout = timeout;
		return _waitKeyPressedFakeResult;
	}
	virtual void set_keyboard_timeout(int ms) { throw not_implemented("set_keyboard_timeout"); }
	virtual void shell() { throw not_implemented("shell"); }
	virtual void set_for_text() { throw not_implemented("set_for_text"); }
	virtual void set_for_graphics() { throw not_implemented("set_for_graphics"); }
	virtual void set_clear() { throw not_implemented("set_clear"); }
	virtual void move_cursor(int row, int col) { throw not_implemented("move_cursor"); }
	virtual void scroll_up(int top, int bottom) { throw not_implemented("scroll_up"); }
	virtual void stack_screen() { throw not_implemented("stack_screen"); }
	virtual void discard_screen() { throw not_implemented("discard_screen"); }
	virtual int get_char_attr() { throw not_implemented("get_char_attr"); }
	virtual void put_char_attr(int char_attr) { throw not_implemented("put_char_attr"); }
	virtual int get_char_attr_rowcol(int row, int col) { throw not_implemented("get_char_attr_rowcol"); }
	virtual void put_char_attr_rowcol(int row, int col, int char_attr) { throw not_implemented("put_char_attr_rowcol"); }

	virtual int init_fm() { throw not_implemented("init_fm"); }
	virtual int sound_on(int frequency) { throw not_implemented("sound_on"); }
	virtual void sound_off() { throw not_implemented("sound_off"); }
	virtual void mute() { throw not_implemented("mute"); }

	virtual int diskp() { throw not_implemented("diskp"); }

	virtual void delay(int ms) { throw not_implemented("delay"); }
	virtual void flush() { throw not_implemented("flush"); }
	virtual void schedule_alarm(int secs) { throw not_implemented("schedule_alarm"); }

	virtual void set_mouse_mode(int mode)
	{
		_setMouseModeCalled = true;
		_setMouseModeLastMode = mode;
	}
	virtual int get_mouse_mode() const
	{
		return _fakeMouseMode;
	}

private:
	bool _waitKeyPressedCalled;
	int _waitKeyPressedLastTimeout;
	int _waitKeyPressedFakeResult;
	int _fakeMouseMode;
	bool _setMouseModeCalled;
	int _setMouseModeLastMode;
	std::vector<int> _keyStrokes;
	std::vector<int>::size_type _keyStrokeCount;
};
