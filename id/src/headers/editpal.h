#if !defined(EDIT_PAL_H)
#define EDIT_PAL_H

#include <string>

#include "port.h"
#include "externs.h"

extern std::string g_screen_file;

extern void palette_edit();
extern void put_row(int x, int y, int width, BYTE const *buff);
extern void get_row(int x, int y, int width, BYTE *buff);
extern void displayc(int, int, int, int, int);

#ifdef XFRACT
void cursor_start_mouse_tracking();
void cursor_end_mouse_tracking();
#endif

/*
 * Class:     cursor
 *
 * Purpose:   Draw the blinking cross-hair cursor.
 *
 * Note:      Only one cursor can exist (referenced through s_the_cursor).
 *            IMPORTANT: Call cursor_new before you use any other
 *            Cursor_ function!  Call cursor_destroy before exiting to
 *            deallocate memory.
 */

class cursor
{
public:
	cursor() : _x(g_screen_width/2),
		_y(g_screen_height/2),
		_hidden(1),
		_blink(false),
		_last_blink(0)
	{
	}
	~cursor()
	{
	}

	static bool create();
	static void destroy();
	static void cursor_set_position(int x, int y)
	{
		s_the_cursor->set_position(x, y);
	}
	static void cursor_move(int xoff, int yoff)
	{
		s_the_cursor->move(xoff, yoff);
	}
	static int cursor_get_x()
	{
		return s_the_cursor->x();
	}
	static int cursor_get_y()
	{
		return s_the_cursor->y();
	}
	static void cursor_hide()
	{
		s_the_cursor->hide();
	}
	static void cursor_show()
	{
		s_the_cursor->show();
	}
	static int cursor_wait_key()
	{
		return s_the_cursor->wait_key();
	}
	static void cursor_check_blink()
	{
		s_the_cursor->check_blink();
	}

private:
	int x() const { return _x; }
	int y() const { return _y; }

	void set_position(int x, int y);

	void check_blink();
	void draw();
	void hide();
	void move(int xoff, int yoff);
	void restore();
	void save();
	void show();
	int wait_key();

	enum
	{
		CURSOR_SIZE = 5     // length of one side of the x-hair cursor
	};
	int _x;
	int _y;
	int _hidden;       // true if mouse hidden
	long _last_blink;
	bool _blink;
	BYTE _top[CURSOR_SIZE];        // save line segments here
	BYTE _bottom[CURSOR_SIZE];
	BYTE _left[CURSOR_SIZE];
	BYTE _right[CURSOR_SIZE];

	static cursor *s_the_cursor;
};

class CursorHider
{
public:
	CursorHider() { cursor::cursor_hide(); }
	~CursorHider() { cursor::cursor_show(); }
};

#endif
