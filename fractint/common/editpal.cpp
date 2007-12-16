/*
 * editpal.cpp
 *
 * Edits VGA 256-color palettes.
 */
#include <algorithm>
#include <string>

#include <string.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "editpal.h"
#include "fihelp.h"
#include "filesystem.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"

/*
 * misc. #defines
 */

#define FONT_DEPTH          8     /* font size */
#define CSIZE_MIN           8     /* csize cannot be smaller than this */
#define CURSOR_SIZE         5     /* length of one side of the x-hair cursor */
#ifndef XFRACT
#define CURSOR_BLINK_RATE   3     /* timer ticks between cursor blinks */
#else
#define CURSOR_BLINK_RATE   300   /* timer ticks between cursor blinks */
#endif
#define FAR_RESERVE     8192L     /* amount of mem we will leave avail. */
#define MAX_WIDTH        1024     /* palette editor cannot be wider than this */
#define TITLE   "Id"
#define TITLE_LEN (8)

#ifdef XFRACT
int g_edit_pal_cursor = 0;
#endif
std::string g_screen_file = "Id.$$1";  /* file where screen portion is stored */
BYTE     *g_line_buffer;   /* must be alloced!!! */
bool g_using_jiim = false;

static char s_undo_file[] = "Id.$$2";  /* file where undo list is stored */
static BYTE		s_fg_color,
				s_bg_color;
static bool s_reserve_colors;
static bool s_inverse;
static float    s_gamma_val = 1;

/*
 * basic data types
 */
struct PALENTRY
{
	BYTE red, green, blue;
};

/*
 * Interface to FRACTINT's graphics stuff
 */
static void set_pal(int pal, int r, int g, int b)
{
	g_dac_box[pal][0] = (BYTE) r;
	g_dac_box[pal][1] = (BYTE) g;
	g_dac_box[pal][2] = (BYTE) b;
	spindac(0, 1);
}

static void set_pal_range(int first, int how_many, PALENTRY *pal)
{
	memmove(g_dac_box + first, pal, how_many*3);
	spindac(0, 1);
}

static void get_pal_range(int first, int how_many, PALENTRY *pal)
{
	memmove(pal, g_dac_box + first, how_many*3);
}

static void rotate_pal(PALENTRY *pal, int dir, int lo, int hi)
{             /* rotate in either direction */
	PALENTRY hold;
	int      size;

	size  = 1 + (hi-lo);

	if (dir > 0)
	{
		while (dir-- > 0)
		{
			memmove(&hold, &pal[hi],  3);
			memmove(&pal[lo + 1], &pal[lo], 3*(size-1));
			memmove(&pal[lo], &hold, 3);
		}
	}

	else if (dir < 0)
	{
		while (dir++ < 0)
		{
			memmove(&hold, &pal[lo], 3);
			memmove(&pal[lo], &pal[lo + 1], 3*(size-1));
			memmove(&pal[hi], &hold,  3);
		}
	}
}

static void clip_put_line(int row, int start, int stop, BYTE *pixels)
	{
	if (row < 0 || row >= g_screen_height || start > g_screen_width || stop < 0)
	{
		return;
	}

	if (start < 0)
	{
		pixels += -start;
		start = 0;
	}

	if (stop >= g_screen_width)
	{
		stop = g_screen_width - 1;
	}

	if (start > stop)
	{
		return;
	}

	put_line(row, start, stop, pixels);
}

static void clip_get_line(int row, int start, int stop, BYTE *pixels)
{
	if (row < 0 || row >= g_screen_height || start > g_screen_width || stop < 0)
	{
		return;
	}

	if (start < 0)
	{
		pixels += -start;
		start = 0;
	}

	if (stop >= g_screen_width)
	{
		stop = g_screen_width - 1;
	}

	if (start > stop)
	{
		return;
	}

	get_line(row, start, stop, pixels);
}

static void clip_put_color(int x, int y, int color)
{
	if (x < 0 || y < 0 || x >= g_screen_width || y >= g_screen_height)
	{
		return;
	}

	g_plot_color_put_color(x, y, color);
}

static int clip_get_color(int x, int y)
{
	if (x < 0 || y < 0 || x >= g_screen_width || y >= g_screen_height)
	{
		return 0;
	}

	return getcolor(x, y);
}

static void horizontal_line(int x, int y, int width, int color)
{
	memset(g_line_buffer, color, width);
	clip_put_line(y, x, x + width-1, g_line_buffer);
}

static void vertical_line(int x, int y, int depth, int color)
{
	while (depth-- > 0)
	{
		clip_put_color(x, y++, color);
	}
}

void get_row(int x, int y, int width, char *buff)
{
	clip_get_line(y, x, x + width-1, (BYTE *)buff);
}

void put_row(int x, int y, int width, char *buff)
{
	clip_put_line(y, x, x + width-1, (BYTE *)buff);
}

static void vertical_get_row(int x, int y, int depth, char *buff)
{
	while (depth-- > 0)
	{
		*buff++ = (char)clip_get_color(x, y++);
	}
}

static void vertical_put_row(int x, int y, int depth, char *buff)
{
	while (depth-- > 0)
	{
		clip_put_color(x, y++, BYTE(*buff++));
	}
}

static void fill_rectangle(int x, int y, int width, int depth, int color)
{
	while (depth-- > 0)
	{
		horizontal_line(x, y++, width, color);
	}
}

static void rectangle(int x, int y, int width, int depth, int color)
{
	horizontal_line(x, y, width, color);
	horizontal_line(x, y + depth-1, width, color);

	vertical_line(x, y, depth, color);
	vertical_line(x + width-1, y, depth, color);
}

static void displayf(int x, int y, int fg, int bg, const boost::format &message)
{
	driver_display_string(x, y, fg, bg, str(message));
}

/*
 * create smooth shades between two colors
 */
static void make_pal_range(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
{
	int    curr;
	double rm = double(int(p2->red) - int(p1->red))/num;
	double gm = double(int(p2->green) - int(p1->green))/num;
	double bm = double(int(p2->blue) - int(p1->blue))/num;

	for (curr = 0; curr < num; curr += skip)
	{
		if (s_gamma_val == 1)
		{
			pal[curr].red   = BYTE((p1->red   == p2->red) ? p1->red   :
				int(p1->red)   + int(rm*curr));
			pal[curr].green = BYTE((p1->green == p2->green) ? p1->green :
				int(p1->green) + int(gm*curr));
			pal[curr].blue  = BYTE((p1->blue  == p2->blue) ? p1->blue  :
				int(p1->blue)  + int(bm*curr));
		}
		else
		{
			pal[curr].red   = BYTE((p1->red   == p2->red) ? p1->red   :
				int(p1->red   + pow(curr/double(num-1), double(s_gamma_val))*num*rm));
			pal[curr].green = BYTE((p1->green == p2->green) ? p1->green :
				int(p1->green + pow(curr/double(num-1), double(s_gamma_val))*num*gm));
			pal[curr].blue  = BYTE((p1->blue  == p2->blue) ? p1->blue  :
				int(p1->blue  + pow(curr/double(num-1), double(s_gamma_val))*num*bm));
		}
	}
}


/*  Swap RG GB & RB columns */
static void swap_columns_rg(PALENTRY pal[], int num)
{
	int    curr;
	int    dummy;

	for (curr = 0; curr <= num; curr++)
	{
		dummy = pal[curr].red;
		pal[curr].red = pal[curr].green;
		pal[curr].green = (BYTE)dummy;
	}
}

static void swap_columns_gb(PALENTRY pal[], int num)
{
	int    curr;
	int    dummy;

	for (curr = 0; curr <= num; curr++)
	{
		dummy = pal[curr].green;
		pal[curr].green = pal[curr].blue;
		pal[curr].blue = (BYTE)dummy;
	}
}

static void swap_columns_br(PALENTRY pal[], int num)
{
	int    curr;
	int    dummy;

	for (curr = 0; curr <= num; curr++)
	{
		dummy = pal[curr].red;
		pal[curr].red = pal[curr].blue;
		pal[curr].blue = (BYTE)dummy;
	}
}

/*
 * convert a range of colors to grey scale
 */
static void pal_range_to_grey(PALENTRY pal[], int first, int how_many)
{
	PALENTRY      *curr;
	BYTE  val;


	for (curr = &pal[first]; how_many > 0; how_many--, curr++)
	{
		val = BYTE((int(curr->red)*30 + int(curr->green)*59 + int(curr->blue)*11)/100);
		curr->red = curr->green = curr->blue = (BYTE)val;
	}
}

/*
 * convert a range of colors to their inverse
 */
static void pal_range_to_negative(PALENTRY pal[], int first, int how_many)
{
	PALENTRY      *curr;

	for (curr = &pal[first]; how_many > 0; how_many--, curr++)
	{
		curr->red   = BYTE(COLOR_CHANNEL_MAX - curr->red);
		curr->green = BYTE(COLOR_CHANNEL_MAX - curr->green);
		curr->blue  = BYTE(COLOR_CHANNEL_MAX - curr->blue);
	}
}

/*
 * draw and horizontal/vertical dotted lines
 */
static void horizontal_dotted_line(int x, int y, int width)
{
	int ctr;
	BYTE *ptr;

	for (ctr = 0, ptr = g_line_buffer; ctr < width; ctr++, ptr++)
	{
		*ptr = BYTE((ctr&2) ? s_bg_color : s_fg_color);
	}

	put_row(x, y, width, (char *)g_line_buffer);
}

static void vertical_dotted_line(int x, int y, int depth)
{
	int ctr;

	for (ctr = 0; ctr < depth; ctr++, y++)
	{
		clip_put_color(x, y, (ctr&2) ? s_bg_color : s_fg_color);
	}
}

static void dotted_rectangle(int x, int y, int width, int depth)
{
	horizontal_dotted_line(x, y, width);
	horizontal_dotted_line(x, y + depth-1, width);

	vertical_dotted_line(x, y, depth);
	vertical_dotted_line(x + width-1, y, depth);
}


/*
 * misc. routines
 *
 */
static bool is_reserved(int color)
{
	return s_reserve_colors && (color == int(s_fg_color) || color == int(s_bg_color));
}

static bool is_in_box(int x, int y, int bx, int by, int bw, int bd)
{
	return (x >= bx) && (y >= by) && (x < bx + bw) && (y < by + bd);
}

static void draw_diamond(int x, int y, int color)
{
	g_plot_color_put_color (x + 2, y + 0,    color);
	horizontal_line    (x + 1, y + 1, 3, color);
	horizontal_line    (x + 0, y + 2, 5, color);
	horizontal_line    (x + 1, y + 3, 3, color);
	g_plot_color_put_color (x + 2, y + 4,    color);
}

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

	int x() const { return _x; }
	int y() const { return _y; }
	int wait_key();
	void hide();
	void show();
	void move(int xoff, int yoff);

	static bool create();
	static void destroy();

private:
	void draw();
	void save();
	void restore();
	void set_position(int x, int y);
	void check_blink();

	int _x;
	int _y;
	int _hidden;       /* true if mouse hidden */
	long _last_blink;
	bool _blink;
	char _top[CURSOR_SIZE];        /* save line segments here */
	char _bottom[CURSOR_SIZE];
	char _left[CURSOR_SIZE];
	char _right[CURSOR_SIZE];
};

/* private: */
static cursor *s_the_cursor = 0;

bool cursor::create()
{
	if (s_the_cursor != 0)
	{
		return false;
	}

	s_the_cursor = new cursor();
	return true;
}

void cursor::destroy()
{
	delete s_the_cursor;
	s_the_cursor = 0;
}

void cursor::draw()
{
	int color;

	find_special_colors();
	color = (_blink) ? g_color_medium : g_color_dark;

	vertical_line(_x, _y - CURSOR_SIZE - 1, CURSOR_SIZE, color);
	vertical_line(_x, _y + 2, CURSOR_SIZE, color);

	horizontal_line(_x - CURSOR_SIZE - 1, _y, CURSOR_SIZE, color);
	horizontal_line(_x + 2, _y, CURSOR_SIZE, color);
}

void cursor::save()
{
	vertical_get_row(_x, _y - CURSOR_SIZE - 1, CURSOR_SIZE, _top);
	vertical_get_row(_x, _y + 2, CURSOR_SIZE, _bottom);

	get_row(_x - CURSOR_SIZE - 1, _y,  CURSOR_SIZE, _left);
	get_row(_x + 2, _y,  CURSOR_SIZE, _right);
}

void cursor::restore()
{
	vertical_put_row(_x, _y - CURSOR_SIZE - 1, CURSOR_SIZE, _top);
	vertical_put_row(_x, _y + 2, CURSOR_SIZE, _bottom);

	put_row(_x - CURSOR_SIZE - 1, _y,  CURSOR_SIZE, _left);
	put_row(_x + 2, _y,  CURSOR_SIZE, _right);
}

void cursor::set_position(int x, int y)
{
	if (!_hidden)
	{
		restore();
	}

	_x = x;
	_y = y;

	if (!_hidden)
	{
		save();
		draw();
	}
}

void cursor::move(int xoff, int yoff)
{
	if (!_hidden)
	{
		restore();
	}

	_x += xoff;
	_y += yoff;

	if (_x < 0)
	{
		_x = 0;
	}
	if (_y < 0)
	{
		_y = 0;
	}
	if (_x >= g_screen_width)
	{
		_x = g_screen_width-1;
	}
	if (_y >= g_screen_height)
	{
		_y = g_screen_height-1;
	}

	if (!_hidden)
	{
		save();
		draw();
	}
}

void cursor::hide()
{
	if (_hidden++ == 0)
	{
		restore();
	}
}

void cursor::show()
{
	if (--_hidden == 0)
	{
		save();
		draw();
	}
}

#ifdef XFRACT
void cursor_start_mouse_tracking()
{
	g_edit_pal_cursor = 1;
}

void cursor_end_mouse_tracking()
{
	g_edit_pal_cursor = 0;
}
#endif

/* See if the cursor should blink yet, and blink it if so */
void cursor::check_blink()
{
	long tick;
	tick = read_ticker();

	if ((tick - _last_blink) > CURSOR_BLINK_RATE)
	{
		_blink = !_blink;
		_last_blink = tick;
		if (!_hidden)
		{
			draw();
		}
	}
	else if (tick < _last_blink)
	{
		_last_blink = tick;
	}
}

int cursor::wait_key()   /* blink cursor while waiting for a key */
{
	while (!driver_wait_key_pressed(1))
	{
		check_blink();
	}

	return driver_key_pressed();
}

/*
 * Class:     move_box
 *
 * Purpose:   Handles the rectangular move/resize box.
 */

class move_box
{
public:
	void move(int key);
	move_box(int x, int y, int csize, int base_width, int base_depth);
	~move_box();
	/* returns false if ESCAPED */
	bool process();
	bool moved() const { return _moved; }
	bool should_hide() const { return _should_hide; }
	int x() const { return _x; }
	int y() const { return _y; }
	int csize() const { return _csize; }
	void set_position(int x, int y);
	void set_csize(int csize);

private:
	void draw();
	void erase();

	int _x;
	int _y;
	int _base_width;
	int _base_depth;
	int      _csize;
	bool _moved;
	bool _should_hide;
	char *_top;
	char *_bottom;
	char *_left;
	char *_right;
};

move_box::move_box(int x, int y, int csize, int base_width, int base_depth)
	: _x(x),
	_y(y),
	_csize(csize),
	_base_width(base_width),
	_base_depth(base_depth),
	_moved(false),
	_should_hide(false),
	_top(new char[g_screen_width]),
	_bottom(new char[g_screen_width]),
	_left(new char[g_screen_height]),
	_right(new char[g_screen_height])
{
}

move_box::~move_box()
{
	delete[] _top;
	_top = 0;
	delete[] _bottom;
	_bottom = 0;
	delete[] _left;
	_left = 0;
	delete[] _right;
	_right = 0;
}

void move_box::set_position(int x, int y)
{
	_x = x;
	_y = y;
}

void move_box::set_csize(int csize)
{
	_csize = csize;
}

void move_box::draw()
{
	int width = _base_width + _csize * 16 + 1;
	int depth = _base_depth + _csize * 16 + 1;
	int x = _x;
	int y = _y;


	get_row (x, y,         width, _top);
	get_row (x, y + depth-1, width, _bottom);

	vertical_get_row(x,         y, depth, _left);
	vertical_get_row(x + width-1, y, depth, _right);

	horizontal_dotted_line(x, y,         width);
	horizontal_dotted_line(x, y + depth-1, width);

	vertical_dotted_line(x,         y, depth);
	vertical_dotted_line(x + width-1, y, depth);
}

void move_box::erase()
{
	int width = _base_width + _csize * 16 + 1;
	int depth = _base_depth + _csize * 16 + 1;

	vertical_put_row(_x,         _y, depth, _left);
	vertical_put_row(_x + width-1, _y, depth, _right);

	put_row(_x, _y,         width, _top);
	put_row(_x, _y + depth-1, width, _bottom);
}

#define BOX_INC     1
#define CSIZE_INC   2

void move_box::move(int key)
{
	bool done  = false;
	bool first = true;
	int xoff = 0;
	int yoff = 0;

	// TODO: refactor to IInputContext
	while (!done)
	{
		switch (key)
		{
		case FIK_CTL_RIGHT_ARROW:     xoff += BOX_INC*4;   break;
		case FIK_RIGHT_ARROW:       xoff += BOX_INC;     break;
		case FIK_CTL_LEFT_ARROW:      xoff -= BOX_INC*4;   break;
		case FIK_LEFT_ARROW:        xoff -= BOX_INC;     break;
		case FIK_CTL_DOWN_ARROW:      yoff += BOX_INC*4;   break;
		case FIK_DOWN_ARROW:        yoff += BOX_INC;     break;
		case FIK_CTL_UP_ARROW:        yoff -= BOX_INC*4;   break;
		case FIK_UP_ARROW:          yoff -= BOX_INC;     break;

		default:
			done = true;
		}

		if (!done)
		{
			if (!first)
			{
				driver_get_key();       /* delete key from buffer */
			}
			else
			{
				first = false;
			}
			key = driver_key_pressed();   /* peek at the next one... */
		}
	}

	xoff += _x;
	yoff += _y;   /* (xoff, yoff) = new position */

	if (xoff < 0)
	{
		xoff = 0;
	}
	if (yoff < 0)
	{
		yoff = 0;
	}

	if (xoff + _base_width + _csize*16 + 1 > g_screen_width)
	{
		xoff = g_screen_width - (_base_width + _csize*16 + 1);
	}

	if (yoff + _base_depth + _csize*16 + 1 > g_screen_height)
	{
		yoff = g_screen_height - (_base_depth + _csize*16 + 1);
	}

	if (xoff != _x || yoff != _y)
	{
		erase();
		_y = yoff;
		_x = xoff;
		draw();
	}
}

bool move_box::process()
{
	int     key;
	int orig_x = _x;
	int orig_y = _y;
	int orig_csize = _csize;

	draw();

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif
	while (true)
	{
		s_the_cursor->wait_key();
		key = driver_get_key();

		if (key == FIK_ENTER || key == FIK_ENTER_2 || key == FIK_ESC || key == 'H' || key == 'h')
		{
			_moved = (_x != orig_x || _y != orig_y || _csize != orig_csize);
			break;
		}

		switch (key)
		{
		case FIK_UP_ARROW:
		case FIK_DOWN_ARROW:
		case FIK_LEFT_ARROW:
		case FIK_RIGHT_ARROW:
		case FIK_CTL_UP_ARROW:
		case FIK_CTL_DOWN_ARROW:
		case FIK_CTL_LEFT_ARROW:
		case FIK_CTL_RIGHT_ARROW:
			move(key);
			break;

		case FIK_PAGE_UP:   /* shrink */
			if (_csize > CSIZE_MIN)
			{
				int t = _csize - CSIZE_INC;
				int change;

				if (t < CSIZE_MIN)
				{
					t = CSIZE_MIN;
				}

				erase();

				change = _csize - t;
				_csize = t;
				_x += (change*16)/2;
				_y += (change*16)/2;
				draw();
			}
			break;

		case FIK_PAGE_DOWN:   /* grow */
			{
				int max_width = std::min(g_screen_width, MAX_WIDTH);

				if (_base_depth + (_csize + CSIZE_INC)*16 + 1 < g_screen_height  &&
					_base_width + (_csize + CSIZE_INC)*16 + 1 < max_width)
				{
					erase();
					_x -= (CSIZE_INC*16)/2;
					_y -= (CSIZE_INC*16)/2;
					_csize += CSIZE_INC;
					if (_y + _base_depth + _csize*16 + 1 > g_screen_height)
					{
						_y = g_screen_height - (_base_depth + _csize*16 + 1);
					}
					if (_x + _base_width + _csize*16 + 1 > max_width)
					{
						_x = max_width - (_base_width + _csize*16 + 1);
					}
					if (_y < 0)
					{
						_y = 0;
					}
					if (_x < 0)
					{
						_x = 0;
					}
					draw();
				}
			}
			break;
		}
	}

#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif

	erase();

	_should_hide = (key == 'H' || key == 'h');

	return key != FIK_ESC;
}

/*
 * Class:     color_editor
 *
 * Purpose:   Edits a single color component (R, G or B)
 *
 * Note:      Calls the "other_key" function to process keys it doesn't use.
 *            The "change" function is called whenever the value is changed
 *            by the color_editor.
 */
class color_editor
{
public:
	color_editor(int x, int y, char letter, void (*other_key)(int, color_editor*, void *),
				void (*change)(color_editor*, void*), void *info);
	~color_editor()
	{
	}

	void draw();
	void set_position(int x, int y)
	{
		_x = x;
		_y = y;
	}
	void set_value(int value) { _value = value; }
	int get_value() const { return _value; }
	void set_done(bool value) { _done = value; }
	void set_hidden(bool value) { _hidden = value; }
	int edit();

private:
	int _x;
	int _y;
	char _letter;
	int _value;
	bool _done;
	bool _hidden;
	void (*_other_key)(int key, color_editor *ce, VOIDPTR info);
	void (*_change)(color_editor *editor, VOIDPTR info);
	void *_info;
};

#define COLOR_EDITOR_WIDTH (8*3 + 4)
#define COLOR_EDITOR_DEPTH (8 + 4)



color_editor::color_editor (int x, int y, char letter,
					void (*other_key)(int, color_editor*, VOIDPTR),
					void (*change)(color_editor*, VOIDPTR), VOIDPTR info)
	: _x(x),
	_y(y),
	_letter(letter),
	_value(0),
	_other_key(other_key),
	_hidden(false),
	_change(change),
	_info(info)
{
}

void color_editor::draw()
{
	if (_hidden)
	{
		return;
	}

	s_the_cursor->hide();
	displayf(_x + 2, _y + 2, s_fg_color, s_bg_color, boost::format("%c%02d") % _letter % _value);
	s_the_cursor->show();
}

int color_editor::edit()
{
	int key = 0;
	int diff;

	_done = false;

	if (!_hidden)
	{
		s_the_cursor->hide();
		rectangle(_x, _y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_fg_color);
		s_the_cursor->show();
	}

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif
	// TODO: refactor to IInputContext
	while (!_done)
	{
		s_the_cursor->wait_key();
		key = driver_get_key();

		switch (key)
		{
		case FIK_PAGE_UP:
			if (_value < COLOR_CHANNEL_MAX)
			{
				_value += 5;
				if (_value > COLOR_CHANNEL_MAX)
				{
					_value = COLOR_CHANNEL_MAX;
				}
				draw();
				_change(this, _info);
			}
			break;

		case '+':
		case FIK_CTL_PLUS:        /*RB*/
			diff = 1;
			while (driver_key_pressed() == key)
			{
				driver_get_key();
				++diff;
			}
			if (_value < COLOR_CHANNEL_MAX)
			{
				_value += diff;
				if (_value > COLOR_CHANNEL_MAX)
				{
					_value = COLOR_CHANNEL_MAX;
				}
				draw();
				_change(this, _info);
			}
			break;

		case FIK_PAGE_DOWN:
			if (_value > 0)
			{
				_value -= 5;
				if (_value < 0)
				{
					_value = 0;
				}
				draw();
				_change(this, _info);
			}
			break;

		case '-':
		case FIK_CTL_MINUS:     /*RB*/
			diff = 1;
			while (driver_key_pressed() == key)
			{
				driver_get_key();
				++diff;
			}
			if (_value > 0)
			{
				_value -= diff;
				if (_value < 0)
				{
					_value = 0;
				}
				draw();
				_change(this, _info);
			}
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			_value = (key - '0')*10;
			if (_value > COLOR_CHANNEL_MAX)
			{
				_value = COLOR_CHANNEL_MAX;
			}
			draw();
			_change(this, _info);
			break;

		default:
			_other_key(key, this, _info);
			break;
		} /* switch */
	} /* while */
#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif

	if (!_hidden)
	{
		s_the_cursor->hide();
		rectangle(_x, _y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_bg_color);
		s_the_cursor->show();
	}

	return key;
}

/*
 * Class:     rgb_editor
 *
 * Purpose:   Edits a complete color using three CEditors for R, G and B
 */

struct rgb_editor
{
	int x;
	int y;            /* position */
	int       curr;            /* 0 = r, 1 = g, 2 = b */
	int       pal;             /* palette number */
	bool done;
	bool hidden;
	color_editor  *color[3];        /* color editors 0 = r, 1 = g, 2 = b */
	void    (*other_key)(int key, rgb_editor *e, VOIDPTR info);
	void    (*change)(rgb_editor *e, VOIDPTR info);
	void     *info;
};

static void      rgb_editor_other_key (int key, color_editor *ceditor, VOIDPTR info);
static void      rgb_editor_change    (color_editor *ceditor, VOIDPTR info);
static rgb_editor *rgb_editor_new(int x, int y,
						void (*other_key)(int, rgb_editor*, void*),
						void (*change)(rgb_editor*, void*), VOIDPTR info);
static void     rgb_editor_destroy  (rgb_editor *me);
static void     rgb_editor_set_position   (rgb_editor *me, int x, int y);
static void     rgb_editor_set_done  (rgb_editor *me, bool done);
static void     rgb_editor_set_hidden(rgb_editor *me, bool hidden);
static void     rgb_editor_blank_sample_box(rgb_editor *me);
static void     rgb_editor_update   (rgb_editor *me);
static void     rgb_editor_draw     (rgb_editor *me);
static int      rgb_editor_edit     (rgb_editor *me);
static void     rgb_editor_set_rgb   (rgb_editor *me, int pal, PALENTRY *rgb);
static PALENTRY rgb_editor_get_rgb   (rgb_editor *me);

#define RGB_EDITOR_WIDTH 62
#define RGB_EDITOR_DEPTH (1 + 1 + COLOR_EDITOR_DEPTH*3-2 + 2)
#define RGB_EDITOR_BOX_WIDTH (RGB_EDITOR_WIDTH - (2 + COLOR_EDITOR_WIDTH + 1 + 2))
#define RGB_EDITOR_BOX_DEPTH (RGB_EDITOR_DEPTH - 4)



static rgb_editor *rgb_editor_new(int x, int y, void (*other_key)(int, rgb_editor*, void*),
									void (*change)(rgb_editor*, void*), VOIDPTR info)
{
	rgb_editor *me = new rgb_editor;
	static char letter[] = "RGB";
	int             ctr;

	for (ctr = 0; ctr < 3; ctr++)
	{
		me->color[ctr] = new color_editor(0, 0, letter[ctr], rgb_editor_other_key,
											rgb_editor_change, me);
	}

	rgb_editor_set_position(me, x, y);
	me->curr      = 0;
	me->pal       = 1;
	me->hidden    = false;
	me->other_key = other_key;
	me->change    = change;
	me->info      = info;

	return me;
}

static void rgb_editor_destroy(rgb_editor *me)
{
	delete me->color[0];
	delete me->color[1];
	delete me->color[2];
	delete me;
	me = 0;
}

static void rgb_editor_set_done(rgb_editor *me, bool done)
{
	me->done = done;
}

static void rgb_editor_set_hidden(rgb_editor *me, bool hidden)
{
	me->hidden = hidden;
	me->color[0]->set_hidden(hidden);
	me->color[1]->set_hidden(hidden);
	me->color[2]->set_hidden(hidden);
}

static void rgb_editor_other_key(int key, color_editor *ceditor, VOIDPTR info) /* private */
{
	rgb_editor *me = (rgb_editor *)info;

	switch (key)
	{
	case 'R':
	case 'r':
		if (me->curr != 0)
		{
			me->curr = 0;
			ceditor->set_done(true);
		}
		break;
	case 'G':
	case 'g':
		if (me->curr != 1)
		{
			me->curr = 1;
			ceditor->set_done(true);
		}
		break;

	case 'B':
	case 'b':
		if (me->curr != 2)
		{
			me->curr = 2;
			ceditor->set_done(true);
		}
		break;

	case FIK_DELETE:   /* move to next color_editor */
	case FIK_CTL_ENTER_2:    /*double click rt mouse also! */
		if (++me->curr > 2)
		{
			me->curr = 0;
		}
		ceditor->set_done(true);
		break;

	case FIK_INSERT:   /* move to prev color_editor */
		if (--me->curr < 0)
		{
			me->curr = 2;
		}
		ceditor->set_done(true);
		break;

	default:
		me->other_key(key, me, me->info);
		if (me->done)
		{
			ceditor->set_done(true);
		}
		break;
	}
}

#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void rgb_editor_change(color_editor *ceditor, VOIDPTR info) /* private */
{
	rgb_editor *me = (rgb_editor *)info;

	ceditor = 0; /* just for warning */
	if (me->pal < g_colors && !is_reserved(me->pal))
	{
		set_pal(me->pal, me->color[0]->get_value(),
			me->color[1]->get_value(), me->color[2]->get_value());
	}
	me->change(me, me->info);
}

static void rgb_editor_set_position(rgb_editor *me, int x, int y)
{
	me->x = x;
	me->y = y;

	me->color[0]->set_position(x + 2, y + 2);
	me->color[1]->set_position(x + 2, y + 2 + COLOR_EDITOR_DEPTH-1);
	me->color[2]->set_position(x + 2, y + 2 + COLOR_EDITOR_DEPTH-1 + COLOR_EDITOR_DEPTH-1);
}

static void rgb_editor_blank_sample_box(rgb_editor *me)
{
	if (me->hidden)
	{
		return;
	}

	s_the_cursor->hide();
	fill_rectangle(me->x + 2 + COLOR_EDITOR_WIDTH + 1 + 1, me->y + 2 + 1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
	s_the_cursor->show();
}

static void rgb_editor_update(rgb_editor *me)
{
	int x1 = me->x + 2 + COLOR_EDITOR_WIDTH + 1 + 1;
	int y1 = me->y + 2 + 1;

	if (me->hidden)
	{
		return;
	}

	s_the_cursor->hide();

	if (me->pal >= g_colors)
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		draw_diamond(x1 + (RGB_EDITOR_BOX_WIDTH-5)/2, y1 + (RGB_EDITOR_BOX_DEPTH-5)/2, s_fg_color);
	}

	else if (is_reserved(me->pal))
	{
		int x2 = x1 + RGB_EDITOR_BOX_WIDTH - 3;
		int y2 = y1 + RGB_EDITOR_BOX_DEPTH - 3;

		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		driver_draw_line(x1, y1, x2, y2, s_fg_color);
		driver_draw_line(x1, y2, x2, y1, s_fg_color);
	}
	else
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, me->pal);
	}

	me->color[0]->draw();
	me->color[1]->draw();
	me->color[2]->draw();
	s_the_cursor->show();
}

static void rgb_editor_draw(rgb_editor *me)
{
	if (me->hidden)
	{
		return;
	}

	s_the_cursor->hide();
	dotted_rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
	fill_rectangle(me->x + 1, me->y + 1, RGB_EDITOR_WIDTH-2, RGB_EDITOR_DEPTH-2, s_bg_color);
	rectangle(me->x + 1 + COLOR_EDITOR_WIDTH + 2, me->y + 2, RGB_EDITOR_BOX_WIDTH, RGB_EDITOR_BOX_DEPTH, s_fg_color);
	rgb_editor_update(me);
	s_the_cursor->show();
}

static int rgb_editor_edit(rgb_editor *me)
{
	int key = 0;

	me->done = false;

	if (!me->hidden)
	{
		s_the_cursor->hide();
		rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH, s_fg_color);
		s_the_cursor->show();
	}

	while (!me->done)
	{
		key = me->color[me->curr]->edit();
	}

	if (!me->hidden)
	{
		s_the_cursor->hide();
		dotted_rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
		s_the_cursor->show();
	}

	return key;
}

static void rgb_editor_set_rgb(rgb_editor *me, int pal, PALENTRY *rgb)
{
	me->pal = pal;
	me->color[0]->set_value(rgb->red);
	me->color[1]->set_value(rgb->green);
	me->color[2]->set_value(rgb->blue);
}

static PALENTRY rgb_editor_get_rgb(rgb_editor *me)
{
	PALENTRY pal;

	pal.red   = (BYTE)me->color[0]->get_value();
	pal.green = (BYTE)me->color[1]->get_value();
	pal.blue  = (BYTE)me->color[2]->get_value();

	return pal;
}

/*
 * Class:     pal_table
 *
 * Purpose:   This is where it all comes together.  Creates the two RGBEditors
 *            and the palette. Moves the cursor, hides/restores the screen,
 *            handles (S)hading, (C)opying, e(X)clude mode, the "Y" exclusion
 *            mode, (Z)oom option, (H)ide palette, rotation, etc.
 *
 */
/*

Modes:
	Auto:          "A", " "
	Exclusion:     "X", "Y", " "
	Freestyle:     "F", " "
	S(t)ripe mode: "T", " "

*/
#define EXCLUDE_NONE	0
#define EXCLUDE_CURRENT	1
#define EXCLUDE_RANGE	2

struct pal_table
{
	int x;
	int y;
	int csize;
	int active;   /* which rgb_editor is active (0, 1) */
	int curr[2];
	rgb_editor *rgb[2];
	move_box *movebox;
	bool done;
	int exclude;
	bool auto_select;
	PALENTRY pal[256];
	FILE *undo_file;
	bool curr_changed;
	int num_redo;
	bool hidden;
	int stored_at;
	FILE *file;
	char *memory;
	PALENTRY *save_pal[8];
	PALENTRY fs_color;
	int top;
	int bottom; /* top and bottom colours of freestyle band */
	int bandwidth; /*size of freestyle colour band */
	bool freestyle;
};

static void pal_table_draw_status(pal_table *me, bool stripe_mode);
static void pal_table_highlight_pal(pal_table *me, int pnum, int color);
static void pal_table_draw(pal_table *me);
static bool pal_table_set_current(pal_table *me, int which, int curr);
static bool pal_table_memory_alloc(pal_table *me, long size);
static void pal_table_save_rect(pal_table *me);
static void pal_table_restore_rect(pal_table *me);
static void pal_table_set_position(pal_table *me, int x, int y);
static void pal_table_set_csize(pal_table *me, int csize);
static int pal_table_get_cursor_color(pal_table *me);
static void pal_table_do_cursor(pal_table *me, int key);
static void pal_table_rotate(pal_table *me, int dir, int lo, int hi);
static void pal_table_update_dac(pal_table *me);
static void pal_table_other_key(int key, rgb_editor *rgb, VOIDPTR info);
static void pal_table_save_undo_data(pal_table *me, int first, int last);
static void pal_table_save_undo_rotate(pal_table *me, int dir, int first, int last);
static void pal_table_undo_process(pal_table *me, int delta);
static void pal_table_undo(pal_table *me);
static void pal_table_redo(pal_table *me);
static void pal_table_change(rgb_editor *rgb, VOIDPTR info);
static pal_table *pal_table_new();
static void pal_table_destroy(pal_table *me);
static void pal_table_process(pal_table *me);
static void pal_table_set_hidden(pal_table *me, bool hidden);
static void pal_table_hide(pal_table *me, rgb_editor *rgb, bool hidden);

#define PALTABLE_PALX (1)
#define PALTABLE_PALY (2 + RGB_EDITOR_DEPTH + 2)
#define UNDO_DATA        (1)
#define UNDO_DATA_SINGLE (2)
#define UNDO_ROTATE      (3)

/*  - Freestyle code - */

static void pal_table_calc_top_bottom(pal_table *me)
{
	me->bottom = (me->curr[me->active] < me->bandwidth)
		? 0    : (me->curr[me->active]) - me->bandwidth;
	me->top    = (me->curr[me->active] > (255-me->bandwidth))
		? 255  : (me->curr[me->active]) + me->bandwidth;
}

static void pal_table_put_band(pal_table *me, PALENTRY *pal)
{
	int r;
	int b;
	int a;

	/* clip top and bottom values to stop them running off the end of the DAC */

	pal_table_calc_top_bottom(me);

	/* put bands either side of current colour */

	a = me->curr[me->active];
	b = me->bottom;
	r = me->top;

	pal[a] = me->fs_color;

	if (r != a && a != b)
	{
		make_pal_range(&pal[a], &pal[r], &pal[a], r-a, 1);
		make_pal_range(&pal[b], &pal[a], &pal[b], a-b, 1);
	}

}

/* - Undo.Redo code - */
static void pal_table_save_undo_data(pal_table *me, int first, int last)
{
	int num;

	if (me->undo_file == 0)
	{
		return;
	}

	num = (last - first) + 1;

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo DATA from %d to %d (%d)", ftell(me->undo_file), first, last, num);
#endif

	fseek(me->undo_file, 0, SEEK_CUR);
	if (num == 1)
	{
		putc(UNDO_DATA_SINGLE, me->undo_file);
		putc(first, me->undo_file);
		fwrite(me->pal + first, 3, 1, me->undo_file);
		putw(1 + 1 + 3 + sizeof(int), me->undo_file);
	}
	else
	{
		putc(UNDO_DATA, me->undo_file);
		putc(first, me->undo_file);
		putc(last,  me->undo_file);
		fwrite(me->pal + first, 3, num, me->undo_file);
		putw(1 + 2 + (num*3) + sizeof(int), me->undo_file);
	}

	me->num_redo = 0;
}

static void pal_table_save_undo_rotate(pal_table *me, int dir, int first, int last)
{
	if (me->undo_file == 0)
	{
		return;
	}

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo ROTATE of %d from %d to %d", ftell(me->undo_file), dir, first, last);
#endif

	fseek(me->undo_file, 0, SEEK_CUR);
	putc(UNDO_ROTATE, me->undo_file);
	putc(first, me->undo_file);
	putc(last,  me->undo_file);
	putw(dir, me->undo_file);
	putw(1 + 2 + sizeof(int), me->undo_file);

	me->num_redo = 0;
}

static void pal_table_undo_process(pal_table *me, int delta)   /* undo/redo common code */
{              /* delta = -1 for undo, +1 for redo */
	int cmd = getc(me->undo_file);

	switch (cmd)
	{
	case UNDO_DATA:
	case UNDO_DATA_SINGLE:
		{
			int first;
			int last;
			int num;
			PALENTRY temp[256];

			if (cmd == UNDO_DATA)
			{
				first = (unsigned char)getc(me->undo_file);
				last  = (unsigned char)getc(me->undo_file);
			}
			else  /* UNDO_DATA_SINGLE */
			{
				first = last = (unsigned char)getc(me->undo_file);
			}

			num = (last - first) + 1;

#ifdef DEBUG_UNDO
			mprintf("          Reading DATA from %d to %d", first, last);
#endif

			fread(temp, 3, num, me->undo_file);

			fseek(me->undo_file, -(num*3), SEEK_CUR);  /* go to start of undo/redo data */
			fwrite(me->pal + first, 3, num, me->undo_file);  /* write redo/undo data */

			memmove(me->pal + first, temp, num*3);

			pal_table_update_dac(me);

			rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			rgb_editor_update(me->rgb[0]);
			rgb_editor_update(me->rgb[1]);
			break;
		}

	case UNDO_ROTATE:
		{
			int first = (unsigned char)getc(me->undo_file);
			int last  = (unsigned char)getc(me->undo_file);
			int dir   = getw(me->undo_file);

#ifdef DEBUG_UNDO
			mprintf("          Reading ROTATE of %d from %d to %d", dir, first, last);
#endif
			pal_table_rotate(me, delta*dir, first, last);
			break;
		}

	default:
#ifdef DEBUG_UNDO
		mprintf("          Unknown command: %d", cmd);
#endif
		break;
	}

	fseek(me->undo_file, 0, SEEK_CUR);  /* to put us in read mode */
	getw(me->undo_file);  /* read size */
}

static void pal_table_undo(pal_table *me)
{
	int  size;
	long pos;

	if (ftell(me->undo_file) <= 0)   /* at beginning of file? */
	{                                  /*   nothing to undo -- exit */
		return;
	}

	fseek(me->undo_file, -int(sizeof(int)), SEEK_CUR);  /* go back to get size */
	size = getw(me->undo_file);
	fseek(me->undo_file, -size, SEEK_CUR);   /* go to start of undo */

#ifdef DEBUG_UNDO
	mprintf("%6ld Undo:", ftell(me->undo_file));
#endif

	pos = ftell(me->undo_file);
	pal_table_undo_process(me, -1);
	fseek(me->undo_file, pos, SEEK_SET);   /* go to start of me g_block */
	++me->num_redo;
}

static void pal_table_redo(pal_table *me)
{
	if (me->num_redo <= 0)
	{
		return;
	}

#ifdef DEBUG_UNDO
	mprintf("%6ld Redo:", ftell(me->undo_file));
#endif

	fseek(me->undo_file, 0, SEEK_CUR);  /* to make sure we are in "read" mode */
	pal_table_undo_process(me, 1);

	--me->num_redo;
}

#define STATUS_LEN (4)

static void pal_table_draw_status(pal_table *me, bool stripe_mode)
{
	int color;
	int width = 1 + (me->csize*16) + 1 + 1;

	if (!me->hidden && (width - (RGB_EDITOR_WIDTH*2 + 4) >= STATUS_LEN*8))
	{
		int x = me->x + 2 + RGB_EDITOR_WIDTH;
		int y = me->y + PALTABLE_PALY - 10;
		color = pal_table_get_cursor_color(me);
		if (color < 0 || color >= g_colors) /* hmm, the border returns -1 */
		{
			color = 0;
		}
		s_the_cursor->hide();

		{
			driver_display_string(x, y, s_fg_color, s_bg_color,
				str(boost::format("%c%c%c%c")
					% (me->auto_select ? 'A' : ' ')
					% ((me->exclude == EXCLUDE_CURRENT) ? 'X' : (me->exclude == EXCLUDE_RANGE) ? 'Y' : ' ')
					% (me->freestyle ? 'F' : ' ')
					% (stripe_mode ? 'T' : ' ')));
			y -= 10;
			driver_display_string(x, y, s_fg_color, s_bg_color,
				str(boost::format("%d") % color));
		}
		s_the_cursor->show();
	}
}

static void pal_table_highlight_pal(pal_table *me, int pnum, int color)
{
	int x = me->x + PALTABLE_PALX + (pnum % 16) * me->csize;
	int y = me->y + PALTABLE_PALY + (pnum/16) * me->csize;
	int size = me->csize;

	if (me->hidden)
	{
		return;
	}

	s_the_cursor->hide();

	if (color < 0)
	{
		dotted_rectangle(x, y, size + 1, size + 1);
	}
	else
	{
		rectangle(x, y, size + 1, size + 1, color);
	}

	s_the_cursor->show();
}

static void pal_table_draw(pal_table *me)
{
	int pal;
	int xoff;
	int yoff;
	int width;

	if (me->hidden)
	{
		return;
	}

	s_the_cursor->hide();
	width = 1 + (me->csize*16) + 1 + 1;
	rectangle(me->x, me->y, width, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1, s_fg_color);
	fill_rectangle(me->x + 1, me->y + 1, width-2, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1-2, s_bg_color);
	horizontal_line(me->x, me->y + PALTABLE_PALY-1, width, s_fg_color);
	if (width - (RGB_EDITOR_WIDTH*2 + 4) >= TITLE_LEN*8)
	{
		int center = (width - TITLE_LEN*8)/2;

		driver_display_string(me->x + center, me->y + RGB_EDITOR_DEPTH/2-6, s_fg_color, s_bg_color, TITLE);
	}

	rgb_editor_draw(me->rgb[0]);
	rgb_editor_draw(me->rgb[1]);

	for (pal = 0; pal < 256; pal++)
	{
		xoff = PALTABLE_PALX + (pal % 16)*me->csize;
		yoff = PALTABLE_PALY + (pal/16)*me->csize;

		if (pal >= g_colors)
		{
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, s_bg_color);
			draw_diamond(me->x + xoff + me->csize/2 - 1, me->y + yoff + me->csize/2 - 1, s_fg_color);
		}
		else if (is_reserved(pal))
		{
			int x1 = me->x + xoff + 1;
			int y1 = me->y + yoff + 1;
			int x2 = x1 + me->csize - 2;
			int y2 = y1 + me->csize - 2;
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, s_bg_color);
			driver_draw_line(x1, y1, x2, y2, s_fg_color);
			driver_draw_line(x1, y2, x2, y1, s_fg_color);
		}
		else
		{
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, pal);
		}
	}

	if (me->active == 0)
	{
		pal_table_highlight_pal(me, me->curr[1], -1);
		pal_table_highlight_pal(me, me->curr[0], s_fg_color);
	}
	else
	{
		pal_table_highlight_pal(me, me->curr[0], -1);
		pal_table_highlight_pal(me, me->curr[1], s_fg_color);
	}

	pal_table_draw_status(me, false);
	s_the_cursor->show();
}

static bool pal_table_set_current(pal_table *me, int which, int curr)
{
	bool redraw = (which < 0);

	if (redraw)
	{
		which = me->active;
		curr = me->curr[which];
	}
	else if (curr == me->curr[which] || curr < 0)
	{
		return false;
	}

	s_the_cursor->hide();

	pal_table_highlight_pal(me, me->curr[0], s_bg_color);
	pal_table_highlight_pal(me, me->curr[1], s_bg_color);
	pal_table_highlight_pal(me, me->top,     s_bg_color);
	pal_table_highlight_pal(me, me->bottom,  s_bg_color);

	if (me->freestyle)
	{
		me->curr[which] = curr;
		pal_table_calc_top_bottom(me);
		pal_table_highlight_pal(me, me->top,    -1);
		pal_table_highlight_pal(me, me->bottom, -1);
		pal_table_highlight_pal(me, me->curr[me->active], s_fg_color);
		rgb_editor_set_rgb(me->rgb[which], me->curr[which], &me->fs_color);
		rgb_editor_update(me->rgb[which]);
		pal_table_update_dac(me);
		s_the_cursor->show();
		return true;
	}

	me->curr[which] = curr;

	if (me->curr[0] != me->curr[1])
	{
		pal_table_highlight_pal(me, me->curr[me->active == 0 ? 1 : 0], -1);
	}
	pal_table_highlight_pal(me, me->curr[me->active], s_fg_color);

	rgb_editor_set_rgb(me->rgb[which], me->curr[which], &(me->pal[me->curr[which]]));

	if (redraw)
	{
		int other = (which == 0) ? 1 : 0;
		rgb_editor_set_rgb(me->rgb[other], me->curr[other], &(me->pal[me->curr[other]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_update(me->rgb[1]);
	}
	else
	{
		rgb_editor_update(me->rgb[which]);
	}

	if (me->exclude)
	{
		pal_table_update_dac(me);
	}

	s_the_cursor->show();
	me->curr_changed = false;
	return true;
}


static bool pal_table_memory_alloc(pal_table *me, long size)
{
	if (DEBUGMODE_USE_DISK == g_debug_mode)
	{
		me->stored_at = NOWHERE;
		return false;   /* can't do it */
	}

	char *temp = new char[FAR_RESERVE];   /* minimum free space */
	if (temp == 0)
	{
		me->stored_at = NOWHERE;
		return false;   /* can't do it */
	}
	delete[] temp;

	me->memory = new char[size];
	if (me->memory == 0)
	{
		me->stored_at = NOWHERE;
		return false;
	}
	else
	{
		me->stored_at = MEMORY;
		return true;
	}
}


static void pal_table_save_rect(pal_table *me)
{
	char buff[MAX_WIDTH];
	int width = PALTABLE_PALX + me->csize * 16 + 1 + 1;
	int depth = PALTABLE_PALY + me->csize * 16 + 1 + 1;
	int  yoff;


	/* first, do any de-allocationg */

	switch (me->stored_at)
	{
	case NOWHERE:
		break;

	case DISK:
		break;

	case MEMORY:
		delete[] me->memory;
		me->memory = 0;
		break;
	}

	/* allocate space and store the rectangle */

	if (pal_table_memory_alloc(me, long(width)*depth))
	{
		char  *ptr = me->memory;
		char  *bufptr = buff; /* MSC needs me indirection to get it right */

		s_the_cursor->hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			get_row(me->x, me->y + yoff, width, buff);
			horizontal_line (me->x, me->y + yoff, width, s_bg_color);
			memcpy(ptr, bufptr, width);
			ptr += width;
		}
		s_the_cursor->show();
	}
	else /* to disk */
	{
		me->stored_at = DISK;

		if (me->file == 0)
		{
			me->file = dir_fopen(g_temp_dir, g_screen_file, "wb");
			if (me->file == 0)
			{
				me->stored_at = NOWHERE;
				driver_buzzer(BUZZER_ERROR);
				return;
			}
		}

		rewind(me->file);
		s_the_cursor->hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			get_row(me->x, me->y + yoff, width, buff);
			horizontal_line (me->x, me->y + yoff, width, s_bg_color);
			if (fwrite(buff, width, 1, me->file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
			}
		}
		s_the_cursor->show();
	}
}


static void pal_table_restore_rect(pal_table *me)
{
	char buff[MAX_WIDTH];
	int width = PALTABLE_PALX + me->csize * 16 + 1 + 1;
	int depth = PALTABLE_PALY + me->csize * 16 + 1 + 1;
	int  yoff;

	if (me->hidden)
	{
		return;
	}

	switch (me->stored_at)
	{
	case DISK:
		rewind(me->file);
		s_the_cursor->hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			if (fread(buff, width, 1, me->file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
			}
			put_row(me->x, me->y + yoff, width, buff);
		}
		s_the_cursor->show();
		break;

	case MEMORY:
		{
			char  *ptr = me->memory;
			char  *bufptr = buff; /* MSC needs me indirection to get it right */

			s_the_cursor->hide();
			for (yoff = 0; yoff < depth; yoff++)
			{
				memcpy(bufptr, ptr, width);
				put_row(me->x, me->y + yoff, width, buff);
				ptr += width;
			}
			s_the_cursor->show();
			break;
		}

	case NOWHERE:
		break;
	} /* switch */
}


static void pal_table_set_position(pal_table *me, int x, int y)
{
	int width = PALTABLE_PALX + me->csize*16 + 1 + 1;

	me->x = x;
	me->y = y;

	rgb_editor_set_position(me->rgb[0], x + 2, y + 2);
	rgb_editor_set_position(me->rgb[1], x + width-2-RGB_EDITOR_WIDTH, y + 2);
}


static void pal_table_set_csize(pal_table *me, int csize)
{
	me->csize = csize;
	pal_table_set_position(me, me->x, me->y);
}


static int pal_table_get_cursor_color(pal_table *me)
{
	int x = s_the_cursor->x();
	int y = s_the_cursor->y();
	int size;
	int color = getcolor(x, y);

	if (is_reserved(color))
	{
		if (is_in_box(x, y, me->x, me->y, 1 + (me->csize*16) + 1 + 1, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1))
		{  /* is the cursor over the editor? */
			x -= me->x + PALTABLE_PALX;
			y -= me->y + PALTABLE_PALY;
			size = me->csize;

			if (x < 0 || y < 0 || x > size*16 || y > size*16)
			{
				return -1;
			}

			if (x == size*16)
			{
				--x;
			}
			if (y == size*16)
			{
				--y;
			}

			return (y/size)*16 + x/size;
		}
		else
		{
			return color;
		}
	}

	return color;
}



#define CURS_INC 1

static void pal_table_do_cursor(pal_table *me, int key)
{
	bool done = false;
	bool first = true;
	int xoff = 0;
	int yoff = 0;

	while (!done)
	{
		switch (key)
		{
		case FIK_CTL_RIGHT_ARROW:     xoff += CURS_INC*4;   break;
		case FIK_RIGHT_ARROW:       xoff += CURS_INC;     break;
		case FIK_CTL_LEFT_ARROW:      xoff -= CURS_INC*4;   break;
		case FIK_LEFT_ARROW:        xoff -= CURS_INC;     break;
		case FIK_CTL_DOWN_ARROW:      yoff += CURS_INC*4;   break;
		case FIK_DOWN_ARROW:        yoff += CURS_INC;     break;
		case FIK_CTL_UP_ARROW:        yoff -= CURS_INC*4;   break;
		case FIK_UP_ARROW:          yoff -= CURS_INC;     break;

		default:
			done = true;
		}

		if (!done)
		{
			if (!first)
			{
				driver_get_key();       /* delete key from buffer */
			}
			else
			{
				first = false;
			}
			key = driver_key_pressed();   /* peek at the next one... */
		}
	}

	s_the_cursor->move(xoff, yoff);

	if (me->auto_select)
	{
		pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
	}
}


#ifdef __CLINT__
#   pragma argsused
#endif

static void pal_table_change(rgb_editor *rgb, VOIDPTR info)
{
	pal_table *me = (pal_table *)info;
	int       pnum = me->curr[me->active];

	if (me->freestyle)
	{
		me->fs_color = rgb_editor_get_rgb(rgb);
		pal_table_update_dac(me);
		return;
	}

	if (!me->curr_changed)
	{
		pal_table_save_undo_data(me, pnum, pnum);
		me->curr_changed = true;
	}

	me->pal[pnum] = rgb_editor_get_rgb(rgb);

	if (me->curr[0] == me->curr[1])
	{
		int      other = me->active == 0 ? 1 : 0;
		PALENTRY color;

		color = rgb_editor_get_rgb(me->rgb[me->active]);
		rgb_editor_set_rgb(me->rgb[other], me->curr[other], &color);

		s_the_cursor->hide();
		rgb_editor_update(me->rgb[other]);
		s_the_cursor->show();
	}
}


static void pal_table_update_dac(pal_table *me)
{
	if (me->exclude)
	{
		memset(g_dac_box, 0, 256*3);
		if (me->exclude == EXCLUDE_CURRENT)
		{
			int a = me->curr[me->active];
			memmove(g_dac_box[a], &me->pal[a], 3);
		}
		else
		{
			int a = me->curr[0];
			int b = me->curr[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			memmove(g_dac_box[a], &me->pal[a], 3*(1 + (b-a)));
		}
	}
	else
	{
		memmove(g_dac_box[0], me->pal, 3*g_colors);

		if (me->freestyle)
		{
			pal_table_put_band(me, (PALENTRY *) g_dac_box);   /* apply band to g_dac_box */
		}
	}

	if (!me->hidden)
	{
		if (s_inverse)
		{
			memset(g_dac_box[s_fg_color], 0, 3);         /* g_dac_box[fg] = (0, 0, 0) */
			memset(g_dac_box[s_bg_color], 48, 3);        /* g_dac_box[bg] = (48, 48, 48) */
		}
		else
		{
			memset(g_dac_box[s_bg_color], 0, 3);         /* g_dac_box[bg] = (0, 0, 0) */
			memset(g_dac_box[s_fg_color], 48, 3);        /* g_dac_box[fg] = (48, 48, 48) */
		}
	}

	spindac(0, 1);
}


static void pal_table_rotate(pal_table *me, int dir, int lo, int hi)
{

	rotate_pal(me->pal, dir, lo, hi);

	s_the_cursor->hide();

	/* update the DAC.  */

	pal_table_update_dac(me);

	/* update the editors. */

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
	rgb_editor_update(me->rgb[0]);
	rgb_editor_update(me->rgb[1]);

	s_the_cursor->show();
}


static void pal_table_other_key(int key, rgb_editor *rgb, VOIDPTR info)
{
	pal_table *me = (pal_table *)info;

	switch (key)
	{
	case '\\':    /* move/resize */
		if (me->hidden)
		{
			break;           /* cannot move a hidden pal */
		}
		s_the_cursor->hide();
		pal_table_restore_rect(me);
		me->movebox->set_position(me->x, me->y);
		me->movebox->set_csize(me->csize);
		if (me->movebox->process())
		{
			if (me->movebox->should_hide())
			{
				pal_table_set_hidden(me, true);
			}
			else if (me->movebox->moved())
			{
				pal_table_set_position(me, me->movebox->x(), me->movebox->y());
				pal_table_set_csize(me, me->movebox->csize());
				pal_table_save_rect(me);
			}
		}
		pal_table_draw(me);
		s_the_cursor->show();

		rgb_editor_set_done(me->rgb[me->active], true);

		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		break;

	case 'Y':    /* exclude range */
	case 'y':
		me->exclude = (me->exclude == EXCLUDE_RANGE) ? EXCLUDE_NONE : EXCLUDE_RANGE;
		pal_table_update_dac(me);
		break;

	case 'X':
	case 'x':     /* exclude current entry */
		me->exclude = (me->exclude == EXCLUDE_CURRENT) ? EXCLUDE_NONE : EXCLUDE_CURRENT;
		pal_table_update_dac(me);
		break;

	case FIK_RIGHT_ARROW:
	case FIK_LEFT_ARROW:
	case FIK_UP_ARROW:
	case FIK_DOWN_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_DOWN_ARROW:
		pal_table_do_cursor(me, key);
		break;

	case FIK_ESC:
		me->done = true;
		rgb_editor_set_done(rgb, true);
		break;

	case ' ':     /* select the other palette register */
		me->active = (me->active == 0) ? 1 : 0;
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		else
		{
			pal_table_set_current(me, -1, 0);
		}
		if (me->exclude || me->freestyle)
		{
			pal_table_update_dac(me);
		}
		rgb_editor_set_done(rgb, true);
		break;

	case FIK_ENTER:    /* set register to color under cursor.  useful when not */
	case FIK_ENTER_2:  /* in auto_select mode */
		if (me->freestyle)
		{
			pal_table_save_undo_data(me, me->bottom, me->top);
			pal_table_put_band(me, me->pal);
		}

		pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));

		if (me->exclude || me->freestyle)
		{
			pal_table_update_dac(me);
		}

		rgb_editor_set_done(rgb, true);
		break;

	case 'D':    /* copy (Duplicate?) color in inactive to color in active */
	case 'd':
		{
			int a = me->active;
			int b = (a == 0) ? 1 : 0;
			PALENTRY t;

			t = rgb_editor_get_rgb(me->rgb[b]);
			s_the_cursor->hide();

			rgb_editor_set_rgb(me->rgb[a], me->curr[a], &t);
			rgb_editor_update(me->rgb[a]);
			pal_table_change(me->rgb[a], me);
			pal_table_update_dac(me);

			s_the_cursor->show();
			break;
		}

	case '=':    /* create a shade range between the two entries */
		{
			int a = me->curr[0];
			int b = me->curr[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				make_pal_range(&me->pal[a], &me->pal[b], &me->pal[a], b-a, 1);
				pal_table_update_dac(me);
			}

			break;
		}

	case '!':    /* swap r<->g */
		{
			int a = me->curr[0];
			int b = me->curr[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_rg(&me->pal[a], b-a);
				pal_table_update_dac(me);
			}
			break;
		}

	case '@':    /* swap g<->b */
	case '"':    /* UK keyboards */
	case 151:    /* French keyboards */
		{
			int a = me->curr[0];
			int b = me->curr[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_gb(&me->pal[a], b-a);
				pal_table_update_dac(me);
			}

			break;
		}

	case '#':    /* swap r<->b */
	case 156:    /* UK keyboards (pound sign) */
	case '$':    /* For French keyboards */
		{
			int a = me->curr[0];
			int b = me->curr[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_br(&me->pal[a], b-a);
				pal_table_update_dac(me);
			}

			break;
		}

	case 'T':
	case 't':   /* s(T)ripe mode */
		{
			int key;

			s_the_cursor->hide();
			pal_table_draw_status(me, true);
			key = getakeynohelp();
			s_the_cursor->show();

			if (key >= '1' && key <= '9')
			{
				int a = me->curr[0];
				int b = me->curr[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				pal_table_save_undo_data(me, a, b);

				if (a != b)
				{
					make_pal_range(&me->pal[a], &me->pal[b], &me->pal[a], b-a, key-'0');
					pal_table_update_dac(me);
				}
			}
			break;
		}

	case 'M':   /* set gamma */
	case 'm':
		{
			int i;
			char buf[20];
			strcpy(buf, boost::format("%.3f") % (1.0/s_gamma_val));
			driver_stack_screen();
			i = field_prompt("Enter gamma value", 0, buf, 20, 0);
			driver_unstack_screen();
			if (i != -1)
			{
				sscanf(buf, "%f", &s_gamma_val);
				if (s_gamma_val == 0)
				{
					s_gamma_val = 0.0000000001f;
				}
				s_gamma_val = float(1./s_gamma_val);
			}
		}
		break;

	case 'A':   /* toggle auto-select mode */
	case 'a':
		me->auto_select = !me->auto_select;
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
			if (me->exclude)
			{
				pal_table_update_dac(me);
			}
		}
		break;

	case 'H':
	case 'h': /* toggle hide/display of palette editor */
		s_the_cursor->hide();
		pal_table_hide(me, rgb, !me->hidden);
		s_the_cursor->show();
		break;

	case '.':   /* rotate once */
	case ',':
		{
			int dir = (key == '.') ? 1 : -1;

			pal_table_save_undo_rotate(me, dir, g_rotate_lo, g_rotate_hi);
			pal_table_rotate(me, dir, g_rotate_lo, g_rotate_hi);
			break;
		}

	case '>':   /* continuous rotation (until a key is pressed) */
	case '<':
		{
			int  dir;
			long tick;
			int  diff = 0;

			s_the_cursor->hide();

			if (!me->hidden)
			{
				rgb_editor_blank_sample_box(me->rgb[0]);
				rgb_editor_blank_sample_box(me->rgb[1]);
				rgb_editor_set_hidden(me->rgb[0], true);
				rgb_editor_set_hidden(me->rgb[1], true);
			}

			do
			{
				dir = (key == '>') ? 1 : -1;

				while (!driver_key_pressed())
				{
					tick = read_ticker();
					pal_table_rotate(me, dir, g_rotate_lo, g_rotate_hi);
					diff += dir;
					while (read_ticker() == tick)   /* wait until a tick passes */
					{
					}
				}

				key = driver_get_key();
			}
			while (key == '<' || key == '>');

			if (!me->hidden)
			{
				rgb_editor_set_hidden(me->rgb[0], false);
				rgb_editor_set_hidden(me->rgb[1], false);
				rgb_editor_update(me->rgb[0]);
				rgb_editor_update(me->rgb[1]);
			}

			if (diff != 0)
			{
				pal_table_save_undo_rotate(me, diff, g_rotate_lo, g_rotate_hi);
			}

			s_the_cursor->show();
			break;
		}

	case 'I':     /* invert the fg & bg g_colors */
	case 'i':
		s_inverse = !s_inverse;
		pal_table_update_dac(me);
		break;

	case 'V':
	case 'v':  /* set the reserved g_colors to the editor colors */
		if (me->curr[0] >= g_colors || me->curr[1] >= g_colors ||
			me->curr[0] == me->curr[1])
		{
			driver_buzzer(BUZZER_ERROR);
			break;
		}

		s_fg_color = (BYTE)me->curr[0];
		s_bg_color = (BYTE)me->curr[1];

		if (!me->hidden)
		{
			s_the_cursor->hide();
			pal_table_update_dac(me);
			pal_table_draw(me);
			s_the_cursor->show();
		}

		rgb_editor_set_done(me->rgb[me->active], true);
		break;

	case 'O':    /* set rotate_lo and rotate_hi to editors */
	case 'o':
		if (me->curr[0] > me->curr[1])
		{
			g_rotate_lo = me->curr[1];
			g_rotate_hi = me->curr[0];
		}
		else
		{
			g_rotate_lo = me->curr[0];
			g_rotate_hi = me->curr[1];
		}
		break;

	case FIK_F2:    /* restore a palette */
	case FIK_F3:
	case FIK_F4:
	case FIK_F5:
	case FIK_F6:
	case FIK_F7:
	case FIK_F8:
	case FIK_F9:
		{
			int which = key - FIK_F2;

			if (me->save_pal[which] != 0)
			{
				s_the_cursor->hide();

				pal_table_save_undo_data(me, 0, 255);
				memcpy(me->pal, me->save_pal[which], 256*3);
				pal_table_update_dac(me);

				pal_table_set_current(me, -1, 0);
				s_the_cursor->show();
				rgb_editor_set_done(me->rgb[me->active], true);
			}
			else
			{
				driver_buzzer(BUZZER_ERROR);   /* error buzz */
			}
			break;
		}

	case FIK_SF2:   /* save a palette */
	case FIK_SF3:
	case FIK_SF4:
	case FIK_SF5:
	case FIK_SF6:
	case FIK_SF7:
	case FIK_SF8:
	case FIK_SF9:
		{
			int which = key - FIK_SF2;

			if (me->save_pal[which] != 0)
			{
				memcpy(me->save_pal[which], me->pal, 256*3);
			}
			else
			{
				driver_buzzer(BUZZER_ERROR); /* oops! short on memory! */
			}
			break;
		}

	case 'L':     /* load a .map palette */
	case 'l':
		pal_table_save_undo_data(me, 0, 255);
		load_palette();
#ifndef XFRACT
		get_pal_range(0, g_colors, me->pal);
#else
		get_pal_range(0, 256, me->pal);
#endif
		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'S':     /* save a .map palette */
	case 's':
#ifndef XFRACT
		set_pal_range(0, g_colors, me->pal);
#else
		set_pal_range(0, 256, me->pal);
#endif
		save_palette();
		pal_table_update_dac(me);
		break;

	case 'C':     /* color cycling sub-mode */
	case 'c':
		{
			bool oldhidden = me->hidden;

			pal_table_save_undo_data(me, 0, 255);

			s_the_cursor->hide();
			if (!oldhidden)
			{
				pal_table_hide(me, rgb, true);
			}
			set_pal_range(0, g_colors, me->pal);
			rotate(0);
			get_pal_range(0, g_colors, me->pal);
			pal_table_update_dac(me);
			if (!oldhidden)
			{
				rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
				rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
				pal_table_hide(me, rgb, false);
			}
			s_the_cursor->show();
			break;
		}

	case 'F':
	case 'f':    /* toggle freestyle palette edit mode */
		me->freestyle = !me->freestyle;
		pal_table_set_current(me, -1, 0);
		if (!me->freestyle)   /* if turning off... */
		{
			pal_table_update_dac(me);
		}
		break;

	case FIK_CTL_DEL:  /* rt plus down */
		if (me->bandwidth >0)
		{
			me->bandwidth--;
		}
		else
		{
			me->bandwidth = 0;
		}
		pal_table_set_current(me, -1, 0);
		break;

	case FIK_CTL_INSERT: /* rt plus up */
		if (me->bandwidth <255)
		{
			me->bandwidth ++;
		}
		else
		{
			me->bandwidth = 255;
		}
		pal_table_set_current(me, -1, 0);
		break;

	case 'W':   /* convert to greyscale */
	case 'w':
		switch (me->exclude)
		{
		case EXCLUDE_NONE:   /* normal mode.  convert all colors to grey scale */
			pal_table_save_undo_data(me, 0, 255);
			pal_range_to_grey(me->pal, 0, 256);
			break;

		case EXCLUDE_CURRENT:   /* 'x' mode. convert current color to grey scale.  */
			pal_table_save_undo_data(me, me->curr[me->active], me->curr[me->active]);
			pal_range_to_grey(me->pal, me->curr[me->active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = me->curr[0];
				int b = me->curr[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				pal_table_save_undo_data(me, a, b);
				pal_range_to_grey(me->pal, a, 1 + (b-a));
				break;
			}
		}

		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'N':   /* convert to negative color */
	case 'n':
		switch (me->exclude)
		{
		case EXCLUDE_NONE:      /* normal mode.  convert all colors to grey scale */
			pal_table_save_undo_data(me, 0, 255);
			pal_range_to_negative(me->pal, 0, 256);
			break;

		case EXCLUDE_CURRENT:      /* 'x' mode. convert current color to grey scale.  */
			pal_table_save_undo_data(me, me->curr[me->active], me->curr[me->active]);
			pal_range_to_negative(me->pal, me->curr[me->active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = me->curr[0];
				int b = me->curr[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				pal_table_save_undo_data(me, a, b);
				pal_range_to_negative(me->pal, a, 1 + (b-a));
				break;
			}
		}

		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'U':     /* Undo */
	case 'u':
		pal_table_undo(me);
		break;

	case 'e':    /* Redo */
	case 'E':
		pal_table_redo(me);
		break;
	} /* switch */
	pal_table_draw_status(me, false);
}

static void pal_table_make_default_palettes(pal_table *me)  /* creates default Fkey palettes */
{
	int i;
	for (i = 0; i < 8; i++) /* copy original palette to save areas */
	{
		if (me->save_pal[i] != 0)
		{
			memcpy(me->save_pal[i], me->pal, 256*3);
		}
	}
}



static pal_table *pal_table_new()
{
	pal_table *me = new pal_table;
	int           csize;
	int           ctr;
	for (ctr = 0; ctr < 8; ctr++)
	{
		me->save_pal[ctr] = new PALENTRY[256];
	}

	me->rgb[0] = rgb_editor_new(0, 0, pal_table_other_key,
						pal_table_change, me);
	me->rgb[1] = rgb_editor_new(0, 0, pal_table_other_key,
						pal_table_change, me);

	me->movebox = new move_box(0, 0, 0, PALTABLE_PALX + 1, PALTABLE_PALY + 1);

	me->active      = 0;
	me->curr[0]     = 1;
	me->curr[1]     = 1;
	me->auto_select = true;
	me->exclude     = EXCLUDE_NONE;
	me->hidden      = false;
	me->stored_at   = NOWHERE;
	me->file        = 0;
	me->memory      = 0;

	me->fs_color.red   = 42;
	me->fs_color.green = 42;
	me->fs_color.blue  = 42;
	me->freestyle      = false;
	me->bandwidth      = 15;
	me->top            = 255;
	me->bottom         = 0;

	me->undo_file    = dir_fopen(g_temp_dir, s_undo_file, "w+b");
	me->curr_changed = false;
	me->num_redo     = 0;

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	pal_table_set_position(me, 0, 0);
	csize = ((g_screen_height-(PALTABLE_PALY + 1 + 1))/2)/16;

	if (csize < CSIZE_MIN)
	{
		csize = CSIZE_MIN;
	}
	pal_table_set_csize(me, csize);

	return me;
}


static void pal_table_set_hidden(pal_table *me, bool hidden)
{
	me->hidden = hidden;
	rgb_editor_set_hidden(me->rgb[0], hidden);
	rgb_editor_set_hidden(me->rgb[1], hidden);
	pal_table_update_dac(me);
}



static void pal_table_hide(pal_table *me, rgb_editor *rgb, bool hidden)
{
	if (hidden)
	{
		pal_table_restore_rect(me);
		pal_table_set_hidden(me, true);
		s_reserve_colors = false;
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
	}
	else
	{
		pal_table_set_hidden(me, false);
		s_reserve_colors = true;
		if (me->stored_at == NOWHERE)  /* do we need to save screen? */
		{
			pal_table_save_rect(me);
		}
		pal_table_draw(me);
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		rgb_editor_set_done(rgb, true);
	}
}


static void pal_table_destroy(pal_table *me)
{

	if (me->file != 0)
	{
		fclose(me->file);
		dir_remove(g_temp_dir, g_screen_file);
	}

	if (me->undo_file != 0)
	{
		fclose(me->undo_file);
		dir_remove(g_temp_dir, s_undo_file);
	}

	delete[] me->memory;

	for (int i = 0; i < 8; i++)
	{
		delete[] me->save_pal[i];
		me->save_pal[i] = 0;
	}

	rgb_editor_destroy(me->rgb[0]);
	rgb_editor_destroy(me->rgb[1]);
	delete me->movebox;
	delete me;
	me = 0;
}


static void pal_table_process(pal_table *me)
{
	get_pal_range(0, g_colors, me->pal);

	/* Make sure all palette entries are 0-COLOR_CHANNEL_MAX */

	int ctr;
	for (ctr = 0; ctr < 768; ctr++)
	{
		((char *)me->pal)[ctr] &= COLOR_CHANNEL_MAX;
	}

	pal_table_update_dac(me);

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	if (!me->hidden)
	{
		me->movebox->set_position(me->x, me->y);
		me->movebox->set_csize(me->csize);
		if (!me->movebox->process())
		{
			set_pal_range(0, g_colors, me->pal);
			return;
		}

		pal_table_set_position(me, me->movebox->x(), me->movebox->y());
		pal_table_set_csize(me, me->movebox->csize());

		if (me->movebox->should_hide())
		{
			pal_table_set_hidden(me, true);
			s_reserve_colors = false;   /* <EAN> */
		}
		else
		{
			s_reserve_colors = true;    /* <EAN> */
			pal_table_save_rect(me);
			pal_table_draw(me);
		}
	}

	pal_table_set_current(me, me->active,          pal_table_get_cursor_color(me));
	pal_table_set_current(me, (me->active == 1) ? 0 : 1, pal_table_get_cursor_color(me));
	s_the_cursor->show();
	pal_table_make_default_palettes(me);
	me->done = false;

	while (!me->done)
	{
		rgb_editor_edit(me->rgb[me->active]);
	}

	s_the_cursor->hide();
	pal_table_restore_rect(me);
	set_pal_range(0, g_colors, me->pal);
}


/*
 * interface to FRACTINT
 */



void palette_edit()       /* called by fractint */
{
	int       oldsxoffs      = g_sx_offset;
	int       oldsyoffs      = g_sy_offset;
	pal_table *pt;

	if (g_screen_width < 133 || g_screen_height < 174)
	{
		return; /* prevents crash when physical screen is too small */
	}

	HelpModeSaver saved_help(HELPXHAIR);
	MouseModeSaver saved_mouse(LOOK_MOUSE_ZOOM_BOX);

	g_plot_color = g_plot_color_put_color;

	g_line_buffer = new BYTE[std::max(g_screen_width, g_screen_height)];

	g_sx_offset = 0;
	g_sy_offset = 0;

	s_reserve_colors = true;
	s_inverse = false;
	s_fg_color = BYTE(255 % g_colors);
	s_bg_color = BYTE(s_fg_color-1);

	cursor::create();
	pt = pal_table_new();
	pal_table_process(pt);
	pal_table_destroy(pt);
	cursor::destroy();

	g_sx_offset = oldsxoffs;
	g_sy_offset = oldsyoffs;
	delete[] g_line_buffer;
	g_line_buffer = 0;
}
