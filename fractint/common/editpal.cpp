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

cursor *cursor::s_the_cursor = 0;

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
		cursor::cursor_wait_key();
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

	cursor::cursor_hide();
	displayf(_x + 2, _y + 2, s_fg_color, s_bg_color, boost::format("%c%02d") % _letter % _value);
	cursor::cursor_show();
}

int color_editor::edit()
{
	int key = 0;
	int diff;

	_done = false;

	if (!_hidden)
	{
		cursor::cursor_hide();
		rectangle(_x, _y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_fg_color);
		cursor::cursor_show();
	}

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif
	// TODO: refactor to IInputContext
	while (!_done)
	{
		cursor::cursor_wait_key();
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
		cursor::cursor_hide();
		rectangle(_x, _y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_bg_color);
		cursor::cursor_show();
	}

	return key;
}

/*
 * Class:     rgb_editor
 *
 * Purpose:   Edits a complete color using three CEditors for R, G and B
 */

class rgb_editor
{
public:
	rgb_editor(int x, int y,
		void (*other_key)(int, rgb_editor*, void*),
		void (*change)(rgb_editor*, void*), VOIDPTR info);
	~rgb_editor();

	void blank_sample_box();
	void draw();
	int edit();
	void update();

	PALENTRY get_rgb();

	void set_done(bool value) { _done = value; }
	void set_hidden(bool value);
	void set_position(int x, int y);
	void set_rgb(int pal, PALENTRY *rgb);

private:
	void other_key(int key, color_editor *ceditor, VOIDPTR info);
	void change(color_editor *ceditor, VOIDPTR info);

	int _x;
	int _y;            /* position */
	int _current_channel;            /* 0 = r, 1 = g, 2 = b */
	int _palette_number;             /* palette number */
	bool _done;
	bool _hidden;
	color_editor *_color_editor[3];        /* color editors 0 = r, 1 = g, 2 = b */
	void (*_other_key)(int key, rgb_editor *e, VOIDPTR info);
	void (*_change)(rgb_editor *e, VOIDPTR info);
	void *_info;

	static void rgb_editor_other_key(int key, color_editor *ceditor, void *info)
	{
		static_cast<rgb_editor *>(info)->rgb_editor_other_key(key, ceditor);
	}
	static void rgb_editor_change(color_editor *, VOIDPTR info)
	{
		static_cast<rgb_editor *>(info)->rgb_editor_change();
	}
	void rgb_editor_other_key(int key, color_editor *ceditor);
	void rgb_editor_change();
};

#define RGB_EDITOR_WIDTH 62
#define RGB_EDITOR_DEPTH (1 + 1 + COLOR_EDITOR_DEPTH*3-2 + 2)
#define RGB_EDITOR_BOX_WIDTH (RGB_EDITOR_WIDTH - (2 + COLOR_EDITOR_WIDTH + 1 + 2))
#define RGB_EDITOR_BOX_DEPTH (RGB_EDITOR_DEPTH - 4)



rgb_editor::rgb_editor(int x, int y, void (*other_key)(int, rgb_editor*, void*),
	void (*change)(rgb_editor*, void*), VOIDPTR info)
	: _x(x),
	_y(y),
	_other_key(other_key),
	_change(change),
	_info(info),
	_current_channel(0),
	_palette_number(1),
	_hidden(false)
{
	char letter[] = "RGB";
	for (int ctr = 0; ctr < 3; ctr++)
	{
		_color_editor[ctr] = new color_editor(0, 0, letter[ctr], rgb_editor_other_key, rgb_editor_change, this);
	}

	set_position(x, y);
}

rgb_editor::~rgb_editor()
{
	delete _color_editor[0];
	_color_editor[0] = 0;
	delete _color_editor[1];
	_color_editor[1] = 0;
	delete _color_editor[2];
	_color_editor[2] = 0;
}

void rgb_editor::set_hidden(bool hidden)
{
	_hidden = hidden;
	_color_editor[0]->set_hidden(hidden);
	_color_editor[1]->set_hidden(hidden);
	_color_editor[2]->set_hidden(hidden);
}

void rgb_editor::rgb_editor_other_key(int key, color_editor *ceditor)
{
	switch (key)
	{
	case 'R':
	case 'r':
		if (_current_channel != 0)
		{
			_current_channel = 0;
			ceditor->set_done(true);
		}
		break;
	case 'G':
	case 'g':
		if (_current_channel != 1)
		{
			_current_channel = 1;
			ceditor->set_done(true);
		}
		break;

	case 'B':
	case 'b':
		if (_current_channel != 2)
		{
			_current_channel = 2;
			ceditor->set_done(true);
		}
		break;

	case FIK_DELETE:   /* move to next color_editor */
	case FIK_CTL_ENTER_2:    /*double click rt mouse also! */
		if (++_current_channel > 2)
		{
			_current_channel = 0;
		}
		ceditor->set_done(true);
		break;

	case FIK_INSERT:   /* move to prev color_editor */
		if (--_current_channel < 0)
		{
			_current_channel = 2;
		}
		ceditor->set_done(true);
		break;

	default:
		_other_key(key, this, _info);
		if (_done)
		{
			ceditor->set_done(true);
		}
		break;
	}
}

#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

void rgb_editor::rgb_editor_change()
{
	if (_palette_number < g_colors && !is_reserved(_palette_number))
	{
		set_pal(_palette_number, _color_editor[0]->get_value(),
			_color_editor[1]->get_value(), _color_editor[2]->get_value());
	}
	_change(this, _info);
}

void rgb_editor::set_position(int x, int y)
{
	_x = x;
	_y = y;

	_color_editor[0]->set_position(x + 2, y + 2);
	_color_editor[1]->set_position(x + 2, y + 2 + COLOR_EDITOR_DEPTH-1);
	_color_editor[2]->set_position(x + 2, y + 2 + COLOR_EDITOR_DEPTH-1 + COLOR_EDITOR_DEPTH-1);
}

void rgb_editor::blank_sample_box()
{
	if (_hidden)
	{
		return;
	}

	cursor::cursor_hide();
	fill_rectangle(_x + 2 + COLOR_EDITOR_WIDTH + 1 + 1, _y + 2 + 1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
	cursor::cursor_show();
}

void rgb_editor::update()
{
	int x1 = _x + 2 + COLOR_EDITOR_WIDTH + 1 + 1;
	int y1 = _y + 2 + 1;

	if (_hidden)
	{
		return;
	}

	cursor::cursor_hide();

	if (_palette_number >= g_colors)
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		draw_diamond(x1 + (RGB_EDITOR_BOX_WIDTH-5)/2, y1 + (RGB_EDITOR_BOX_DEPTH-5)/2, s_fg_color);
	}

	else if (is_reserved(_palette_number))
	{
		int x2 = x1 + RGB_EDITOR_BOX_WIDTH - 3;
		int y2 = y1 + RGB_EDITOR_BOX_DEPTH - 3;

		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		driver_draw_line(x1, y1, x2, y2, s_fg_color);
		driver_draw_line(x1, y2, x2, y1, s_fg_color);
	}
	else
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, _palette_number);
	}

	_color_editor[0]->draw();
	_color_editor[1]->draw();
	_color_editor[2]->draw();
	cursor::cursor_show();
}

void rgb_editor::draw()
{
	if (_hidden)
	{
		return;
	}

	cursor::cursor_hide();
	dotted_rectangle(_x, _y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
	fill_rectangle(_x + 1, _y + 1, RGB_EDITOR_WIDTH-2, RGB_EDITOR_DEPTH-2, s_bg_color);
	rectangle(_x + 1 + COLOR_EDITOR_WIDTH + 2, _y + 2, RGB_EDITOR_BOX_WIDTH, RGB_EDITOR_BOX_DEPTH, s_fg_color);
	update();
	cursor::cursor_show();
}

int rgb_editor::edit()
{
	int key = 0;

	_done = false;

	if (!_hidden)
	{
		cursor::cursor_hide();
		rectangle(_x, _y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH, s_fg_color);
		cursor::cursor_show();
	}

	while (!_done)
	{
		key = _color_editor[_current_channel]->edit();
	}

	if (!_hidden)
	{
		cursor::cursor_hide();
		dotted_rectangle(_x, _y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
		cursor::cursor_show();
	}

	return key;
}

void rgb_editor::set_rgb(int pal, PALENTRY *rgb)
{
	_palette_number = pal;
	_color_editor[0]->set_value(rgb->red);
	_color_editor[1]->set_value(rgb->green);
	_color_editor[2]->set_value(rgb->blue);
}

PALENTRY rgb_editor::get_rgb()
{
	PALENTRY pal;

	pal.red   = (BYTE)_color_editor[0]->get_value();
	pal.green = (BYTE)_color_editor[1]->get_value();
	pal.blue  = (BYTE)_color_editor[2]->get_value();

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

class pal_table
{
public:
	pal_table();
	~pal_table();

	void process();

private:
	void calc_top_bottom();
	static void change(rgb_editor *rgb, VOIDPTR info)
	{
		static_cast<pal_table *>(info)->change(rgb);
	}
	void change(rgb_editor *rgb);
	void do_cursor(int key);
	void draw();
	void draw_status(bool stripe_mode);
	void hide(rgb_editor *rgb, bool hidden);
	void highlight_pal(int pnum, int color);
	void make_default_palettes();
	bool memory_alloc(long size);
	static void other_key(int key, rgb_editor *rgb, VOIDPTR info)
	{
		static_cast<pal_table *>(info)->other_key(key, rgb);
	}
	void other_key(int key, rgb_editor *rgb);
	void put_band(PALENTRY *pal);
	void redo();
	void restore_rect();
	void rotate(int dir, int lo, int hi);
	void save_rect();
	void save_undo_data(int first, int last);
	void save_undo_rotate(int dir, int first, int last);
	void undo();
	void undo_process(int delta);
	void update_dac();

	int get_cursor_color();

	void set_csize(int csize);
	bool set_current(int which, int curr);
	void set_hidden(bool hidden);
	void set_position(int x, int y);

	int _x;
	int _y;
	int _csize;
	int _active;   /* which rgb_editor is active (0, 1) */
	int _current[2];
	rgb_editor *_rgb_editors[2];
	move_box *_move_box;
	bool _done;
	int _exclude;
	bool _auto_select;
	PALENTRY _palette[256];
	FILE *_undo_file;
	bool _current_changed;
	int _num_redo;
	bool _hidden;
	int _stored_at;
	FILE *_file;
	char *_memory;
	PALENTRY *_save_palette[8];
	PALENTRY _fs_color;
	int _top;
	int _bottom; /* top and bottom colours of freestyle band */
	int _color_band_width; /*size of freestyle colour band */
	bool _freestyle;
};

#define PALTABLE_PALX (1)
#define PALTABLE_PALY (2 + RGB_EDITOR_DEPTH + 2)
#define UNDO_DATA        (1)
#define UNDO_DATA_SINGLE (2)
#define UNDO_ROTATE      (3)

/*  - Freestyle code - */

void pal_table::calc_top_bottom()
{
	_bottom = (_current[_active] < _color_band_width)
		? 0    : (_current[_active]) - _color_band_width;
	_top    = (_current[_active] > (255-_color_band_width))
		? 255  : (_current[_active]) + _color_band_width;
}

void pal_table::put_band(PALENTRY *pal)
{
	int r;
	int b;
	int a;

	/* clip top and bottom values to stop them running off the end of the DAC */

	calc_top_bottom();

	/* put bands either side of current colour */

	a = _current[_active];
	b = _bottom;
	r = _top;

	pal[a] = _fs_color;

	if (r != a && a != b)
	{
		make_pal_range(&pal[a], &pal[r], &pal[a], r-a, 1);
		make_pal_range(&pal[b], &pal[a], &pal[b], a-b, 1);
	}
}

/* - Undo.Redo code - */
void pal_table::save_undo_data(int first, int last)
{
	int num;

	if (_undo_file == 0)
	{
		return;
	}

	num = (last - first) + 1;

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo DATA from %d to %d (%d)", ftell(undo_file), first, last, num);
#endif

	fseek(_undo_file, 0, SEEK_CUR);
	if (num == 1)
	{
		putc(UNDO_DATA_SINGLE, _undo_file);
		putc(first, _undo_file);
		fwrite(_palette + first, 3, 1, _undo_file);
		putw(1 + 1 + 3 + sizeof(int), _undo_file);
	}
	else
	{
		putc(UNDO_DATA, _undo_file);
		putc(first, _undo_file);
		putc(last,  _undo_file);
		fwrite(_palette + first, 3, num, _undo_file);
		putw(1 + 2 + (num*3) + sizeof(int), _undo_file);
	}

	_num_redo = 0;
}

void pal_table::save_undo_rotate(int dir, int first, int last)
{
	if (_undo_file == 0)
	{
		return;
	}

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo ROTATE of %d from %d to %d", ftell(undo_file), dir, first, last);
#endif

	fseek(_undo_file, 0, SEEK_CUR);
	putc(UNDO_ROTATE, _undo_file);
	putc(first, _undo_file);
	putc(last,  _undo_file);
	putw(dir, _undo_file);
	putw(1 + 2 + sizeof(int), _undo_file);

	_num_redo = 0;
}

void pal_table::undo_process(int delta)   /* undo/redo common code */
{              /* delta = -1 for undo, +1 for redo */
	int cmd = getc(_undo_file);

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
				first = (unsigned char)getc(_undo_file);
				last  = (unsigned char)getc(_undo_file);
			}
			else  /* UNDO_DATA_SINGLE */
			{
				first = last = (unsigned char)getc(_undo_file);
			}

			num = (last - first) + 1;

#ifdef DEBUG_UNDO
			mprintf("          Reading DATA from %d to %d", first, last);
#endif

			fread(temp, 3, num, _undo_file);

			fseek(_undo_file, -(num*3), SEEK_CUR);  /* go to start of undo/redo data */
			fwrite(_palette + first, 3, num, _undo_file);  /* write redo/undo data */

			memmove(_palette + first, temp, num*3);

			update_dac();

			_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
			_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
			_rgb_editors[0]->update();
			_rgb_editors[1]->update();
			break;
		}

	case UNDO_ROTATE:
		{
			int first = (unsigned char)getc(_undo_file);
			int last  = (unsigned char)getc(_undo_file);
			int dir   = getw(_undo_file);

#ifdef DEBUG_UNDO
			mprintf("          Reading ROTATE of %d from %d to %d", dir, first, last);
#endif
			rotate(delta*dir, first, last);
			break;
		}

	default:
#ifdef DEBUG_UNDO
		mprintf("          Unknown command: %d", cmd);
#endif
		break;
	}

	fseek(_undo_file, 0, SEEK_CUR);  /* to put us in read mode */
	getw(_undo_file);  /* read size */
}

void pal_table::undo()
{
	if (ftell(_undo_file) <= 0)   /* at beginning of file? */
	{                                  /*   nothing to undo -- exit */
		return;
	}

	fseek(_undo_file, -int(sizeof(int)), SEEK_CUR);  /* go back to get size */
	int size = getw(_undo_file);
	fseek(_undo_file, -size, SEEK_CUR);   /* go to start of undo */

#ifdef DEBUG_UNDO
	mprintf("%6ld Undo:", ftell(undo_file));
#endif

	long pos = ftell(_undo_file);
	undo_process(-1);
	fseek(_undo_file, pos, SEEK_SET);   /* go to start of me g_block */
	++_num_redo;
}

void pal_table::redo()
{
	if (_num_redo <= 0)
	{
		return;
	}

#ifdef DEBUG_UNDO
	mprintf("%6ld Redo:", ftell(undo_file));
#endif

	fseek(_undo_file, 0, SEEK_CUR);  /* to make sure we are in "read" mode */
	undo_process(1);

	--_num_redo;
}

#define STATUS_LEN (4)

void pal_table::draw_status(bool stripe_mode)
{
	int width = 1 + (_csize*16) + 1 + 1;

	if (!_hidden && (width - (RGB_EDITOR_WIDTH*2 + 4) >= STATUS_LEN*8))
	{
		int x = _x + 2 + RGB_EDITOR_WIDTH;
		int y = _y + PALTABLE_PALY - 10;
		int color = get_cursor_color();
		if (color < 0 || color >= g_colors) /* hmm, the border returns -1 */
		{
			color = 0;
		}
		cursor::cursor_hide();

		{
			driver_display_string(x, y, s_fg_color, s_bg_color,
				str(boost::format("%c%c%c%c")
					% (_auto_select ? 'A' : ' ')
					% ((_exclude == EXCLUDE_CURRENT) ? 'X' : (_exclude == EXCLUDE_RANGE) ? 'Y' : ' ')
					% (_freestyle ? 'F' : ' ')
					% (stripe_mode ? 'T' : ' ')));
			y -= 10;
			driver_display_string(x, y, s_fg_color, s_bg_color,
				str(boost::format("%d") % color));
		}
		cursor::cursor_show();
	}
}

void pal_table::highlight_pal(int pnum, int color)
{
	if (_hidden)
	{
		return;
	}

	cursor::cursor_hide();

	int x = _x + PALTABLE_PALX + (pnum % 16)*_csize;
	int y = _y + PALTABLE_PALY + (pnum/16)*_csize;
	if (color < 0)
	{
		dotted_rectangle(x, y, _csize + 1, _csize + 1);
	}
	else
	{
		rectangle(x, y, _csize + 1, _csize + 1, color);
	}

	cursor::cursor_show();
}

void pal_table::draw()
{
	if (_hidden)
	{
		return;
	}

	cursor::cursor_hide();
	int width = 1 + (_csize*16) + 1 + 1;
	rectangle(_x, _y, width, 2 + RGB_EDITOR_DEPTH + 2 + (_csize*16) + 1 + 1, s_fg_color);
	fill_rectangle(_x + 1, _y + 1, width-2, 2 + RGB_EDITOR_DEPTH + 2 + (_csize*16) + 1 + 1-2, s_bg_color);
	horizontal_line(_x, _y + PALTABLE_PALY-1, width, s_fg_color);
	if (width - (RGB_EDITOR_WIDTH*2 + 4) >= TITLE_LEN*8)
	{
		int center = (width - TITLE_LEN*8)/2;

		driver_display_string(_x + center, _y + RGB_EDITOR_DEPTH/2-6, s_fg_color, s_bg_color, TITLE);
	}

	_rgb_editors[0]->draw();
	_rgb_editors[1]->draw();

	for (int pal = 0; pal < 256; pal++)
	{
		int xoff = PALTABLE_PALX + (pal % 16)*_csize;
		int yoff = PALTABLE_PALY + (pal/16)*_csize;

		if (pal >= g_colors)
		{
			fill_rectangle(_x + xoff + 1, _y + yoff + 1, _csize-1, _csize-1, s_bg_color);
			draw_diamond(_x + xoff + _csize/2 - 1, _y + yoff + _csize/2 - 1, s_fg_color);
		}
		else if (is_reserved(pal))
		{
			int x1 = _x + xoff + 1;
			int y1 = _y + yoff + 1;
			int x2 = x1 + _csize - 2;
			int y2 = y1 + _csize - 2;
			fill_rectangle(_x + xoff + 1, _y + yoff + 1, _csize-1, _csize-1, s_bg_color);
			driver_draw_line(x1, y1, x2, y2, s_fg_color);
			driver_draw_line(x1, y2, x2, y1, s_fg_color);
		}
		else
		{
			fill_rectangle(_x + xoff + 1, _y + yoff + 1, _csize-1, _csize-1, pal);
		}
	}

	if (_active == 0)
	{
		highlight_pal(_current[1], -1);
		highlight_pal(_current[0], s_fg_color);
	}
	else
	{
		highlight_pal(_current[0], -1);
		highlight_pal(_current[1], s_fg_color);
	}

	draw_status(false);
	cursor::cursor_show();
}

bool pal_table::set_current(int which, int curr)
{
	bool redraw = (which < 0);

	if (redraw)
	{
		which = _active;
		curr = _current[which];
	}
	else if (curr == _current[which] || curr < 0)
	{
		return false;
	}

	cursor::cursor_hide();

	highlight_pal(_current[0], s_bg_color);
	highlight_pal(_current[1], s_bg_color);
	highlight_pal(_top,     s_bg_color);
	highlight_pal(_bottom,  s_bg_color);

	if (_freestyle)
	{
		_current[which] = curr;
		calc_top_bottom();
		highlight_pal(_top,    -1);
		highlight_pal(_bottom, -1);
		highlight_pal(_current[_active], s_fg_color);
		_rgb_editors[which]->set_rgb(_current[which], &_fs_color);
		_rgb_editors[which]->update();
		update_dac();
		cursor::cursor_show();
		return true;
	}

	_current[which] = curr;

	if (_current[0] != _current[1])
	{
		highlight_pal(_current[_active == 0 ? 1 : 0], -1);
	}
	highlight_pal(_current[_active], s_fg_color);

	_rgb_editors[which]->set_rgb(_current[which], &(_palette[_current[which]]));

	if (redraw)
	{
		int other = (which == 0) ? 1 : 0;
		_rgb_editors[other]->set_rgb(_current[other], &(_palette[_current[other]]));
		_rgb_editors[0]->update();
		_rgb_editors[1]->update();
	}
	else
	{
		_rgb_editors[which]->update();
	}

	if (_exclude)
	{
		update_dac();
	}

	cursor::cursor_show();
	_current_changed = false;
	return true;
}


bool pal_table::memory_alloc(long size)
{
	if (DEBUGMODE_USE_DISK == g_debug_mode)
	{
		_stored_at = NOWHERE;
		return false;   /* can't do it */
	}

	char *temp = new char[FAR_RESERVE];   /* minimum free space */
	if (temp == 0)
	{
		_stored_at = NOWHERE;
		return false;   /* can't do it */
	}
	delete[] temp;

	_memory = new char[size];
	if (_memory == 0)
	{
		_stored_at = NOWHERE;
		return false;
	}
	else
	{
		_stored_at = MEMORY;
		return true;
	}
}


void pal_table::save_rect()
{
	/* first, do any de-allocationg */
	if (_stored_at == MEMORY)
	{
		delete [] _memory;
		_memory = 0;
	}

	/* allocate space and store the rectangle */

	int width = PALTABLE_PALX + _csize * 16 + 1 + 1;
	int depth = PALTABLE_PALY + _csize * 16 + 1 + 1;
	char buff[MAX_WIDTH];
	if (memory_alloc(long(width)*depth))
	{
		char  *ptr = _memory;
		char  *bufptr = buff; /* MSC needs me indirection to get it right */

		cursor::cursor_hide();
		for (int yoff = 0; yoff < depth; yoff++)
		{
			get_row(_x, _y + yoff, width, buff);
			horizontal_line (_x, _y + yoff, width, s_bg_color);
			memcpy(ptr, bufptr, width);
			ptr += width;
		}
		cursor::cursor_show();
	}
	else /* to disk */
	{
		_stored_at = DISK;

		if (_file == 0)
		{
			_file = dir_fopen(g_temp_dir, g_screen_file, "wb");
			if (_file == 0)
			{
				_stored_at = NOWHERE;
				driver_buzzer(BUZZER_ERROR);
				return;
			}
		}

		rewind(_file);
		cursor::cursor_hide();
		for (int yoff = 0; yoff < depth; yoff++)
		{
			get_row(_x, _y + yoff, width, buff);
			horizontal_line (_x, _y + yoff, width, s_bg_color);
			if (fwrite(buff, width, 1, _file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
			}
		}
		cursor::cursor_show();
	}
}


void pal_table::restore_rect()
{
	if (_hidden)
	{
		return;
	}

	char buff[MAX_WIDTH];
	int width = PALTABLE_PALX + _csize * 16 + 1 + 1;
	int depth = PALTABLE_PALY + _csize * 16 + 1 + 1;
	switch (_stored_at)
	{
	case DISK:
		rewind(_file);
		cursor::cursor_hide();
		for (int yoff = 0; yoff < depth; yoff++)
		{
			if (fread(buff, width, 1, _file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
			}
			put_row(_x, _y + yoff, width, buff);
		}
		cursor::cursor_show();
		break;

	case MEMORY:
		{
			char  *ptr = _memory;
			char  *bufptr = buff; /* MSC needs me indirection to get it right */

			cursor::cursor_hide();
			for (int yoff = 0; yoff < depth; yoff++)
			{
				memcpy(bufptr, ptr, width);
				put_row(_x, _y + yoff, width, buff);
				ptr += width;
			}
			cursor::cursor_show();
			break;
		}

	case NOWHERE:
		break;
	} /* switch */
}


void pal_table::set_position(int x, int y)
{
	_x = x;
	_y = y;

	_rgb_editors[0]->set_position(x + 2, y + 2);
	int width = PALTABLE_PALX + _csize*16 + 1 + 1;
	_rgb_editors[1]->set_position(x + width-2-RGB_EDITOR_WIDTH, y + 2);
}


void pal_table::set_csize(int csize)
{
	_csize = csize;
	set_position(_x, _y);
}


int pal_table::get_cursor_color()
{
	int x = cursor::cursor_get_x();
	int y = cursor::cursor_get_y();
	int size;
	int color = getcolor(x, y);

	if (is_reserved(color))
	{
		if (is_in_box(x, y, _x, _y, 1 + (_csize*16) + 1 + 1, 2 + RGB_EDITOR_DEPTH + 2 + (_csize*16) + 1 + 1))
		{  /* is the cursor over the editor? */
			x -= _x + PALTABLE_PALX;
			y -= _y + PALTABLE_PALY;
			size = _csize;

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

void pal_table::do_cursor(int key)
{
	bool done = false;
	bool first = true;
	int xoff = 0;
	int yoff = 0;

	while (!done)
	{
		switch (key)
		{
		case FIK_CTL_RIGHT_ARROW:	xoff += CURS_INC*4;	break;
		case FIK_RIGHT_ARROW:		xoff += CURS_INC;	break;
		case FIK_CTL_LEFT_ARROW:	xoff -= CURS_INC*4;	break;
		case FIK_LEFT_ARROW:		xoff -= CURS_INC;	break;
		case FIK_CTL_DOWN_ARROW:	yoff += CURS_INC*4;	break;
		case FIK_DOWN_ARROW:		yoff += CURS_INC;	break;
		case FIK_CTL_UP_ARROW:		yoff -= CURS_INC*4;	break;
		case FIK_UP_ARROW:			yoff -= CURS_INC;	break;

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

	cursor::cursor_move(xoff, yoff);

	if (_auto_select)
	{
		set_current(_active, get_cursor_color());
	}
}


void pal_table::change(rgb_editor *rgb)
{
	int pnum = _current[_active];

	if (_freestyle)
	{
		_fs_color = rgb->get_rgb();
		update_dac();
		return;
	}

	if (!_current_changed)
	{
		save_undo_data(pnum, pnum);
		_current_changed = true;
	}

	_palette[pnum] = rgb->get_rgb();

	if (_current[0] == _current[1])
	{
		int      other = _active == 0 ? 1 : 0;
		PALENTRY color;

		color = _rgb_editors[_active]->get_rgb();
		_rgb_editors[other]->set_rgb(_current[other], &color);

		cursor::cursor_hide();
		_rgb_editors[other]->update();
		cursor::cursor_show();
	}
}


void pal_table::update_dac()
{
	if (_exclude)
	{
		memset(g_dac_box, 0, 256*3);
		if (_exclude == EXCLUDE_CURRENT)
		{
			int a = _current[_active];
			memmove(g_dac_box[a], &_palette[a], 3);
		}
		else
		{
			int a = _current[0];
			int b = _current[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			memmove(g_dac_box[a], &_palette[a], 3*(1 + (b-a)));
		}
	}
	else
	{
		memmove(g_dac_box[0], _palette, 3*g_colors);

		if (_freestyle)
		{
			put_band((PALENTRY *) g_dac_box);   /* apply band to g_dac_box */
		}
	}

	if (!_hidden)
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


void pal_table::rotate(int dir, int lo, int hi)
{
	rotate_pal(_palette, dir, lo, hi);

	cursor::cursor_hide();

	/* update the DAC.  */

	update_dac();

	/* update the editors. */

	_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
	_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
	_rgb_editors[0]->update();
	_rgb_editors[1]->update();

	cursor::cursor_show();
}


void pal_table::other_key(int key, rgb_editor *rgb)
{
	switch (key)
	{
	case '\\':    /* move/resize */
		if (_hidden)
		{
			break;           /* cannot move a hidden pal */
		}
		cursor::cursor_hide();
		restore_rect();
		_move_box->set_position(_x, _y);
		_move_box->set_csize(_csize);
		if (_move_box->process())
		{
			if (_move_box->should_hide())
			{
				set_hidden(true);
			}
			else if (_move_box->moved())
			{
				set_position(_move_box->x(), _move_box->y());
				set_csize(_move_box->csize());
				save_rect();
			}
		}
		draw();
		cursor::cursor_show();

		_rgb_editors[_active]->set_done(true);

		if (_auto_select)
		{
			set_current(_active, get_cursor_color());
		}
		break;

	case 'Y':    /* exclude range */
	case 'y':
		_exclude = (_exclude == EXCLUDE_RANGE) ? EXCLUDE_NONE : EXCLUDE_RANGE;
		update_dac();
		break;

	case 'X':
	case 'x':     /* exclude current entry */
		_exclude = (_exclude == EXCLUDE_CURRENT) ? EXCLUDE_NONE : EXCLUDE_CURRENT;
		update_dac();
		break;

	case FIK_RIGHT_ARROW:
	case FIK_LEFT_ARROW:
	case FIK_UP_ARROW:
	case FIK_DOWN_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_DOWN_ARROW:
		do_cursor(key);
		break;

	case FIK_ESC:
		_done = true;
		rgb->set_done(true);
		break;

	case ' ':     /* select the other palette register */
		_active = (_active == 0) ? 1 : 0;
		if (_auto_select)
		{
			set_current(_active, get_cursor_color());
		}
		else
		{
			set_current(-1, 0);
		}
		if (_exclude || _freestyle)
		{
			update_dac();
		}
		rgb->set_done(true);
		break;

	case FIK_ENTER:    /* set register to color under cursor.  useful when not */
	case FIK_ENTER_2:  /* in auto_select mode */
		if (_freestyle)
		{
			save_undo_data(_bottom, _top);
			put_band(_palette);
		}

		set_current(_active, get_cursor_color());

		if (_exclude || _freestyle)
		{
			update_dac();
		}

		rgb->set_done(true);
		break;

	case 'D':    /* copy (Duplicate?) color in inactive to color in active */
	case 'd':
		{
			int a = _active;
			int b = (a == 0) ? 1 : 0;
			PALENTRY t;

			t = _rgb_editors[b]->get_rgb();
			cursor::cursor_hide();

			_rgb_editors[a]->set_rgb(_current[a], &t);
			_rgb_editors[a]->update();
			change(_rgb_editors[a], this);
			update_dac();

			cursor::cursor_show();
			break;
		}

	case '=':    /* create a shade range between the two entries */
		{
			int a = _current[0];
			int b = _current[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			save_undo_data(a, b);

			if (a != b)
			{
				make_pal_range(&_palette[a], &_palette[b], &_palette[a], b-a, 1);
				update_dac();
			}

			break;
		}

	case '!':    /* swap r<->g */
		{
			int a = _current[0];
			int b = _current[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			save_undo_data(a, b);

			if (a != b)
			{
				swap_columns_rg(&_palette[a], b-a);
				update_dac();
			}
			break;
		}

	case '@':    /* swap g<->b */
	case '"':    /* UK keyboards */
	case 151:    /* French keyboards */
		{
			int a = _current[0];
			int b = _current[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			save_undo_data(a, b);

			if (a != b)
			{
				swap_columns_gb(&_palette[a], b-a);
				update_dac();
			}

			break;
		}

	case '#':    /* swap r<->b */
	case 156:    /* UK keyboards (pound sign) */
	case '$':    /* For French keyboards */
		{
			int a = _current[0];
			int b = _current[1];

			if (a > b)
			{
				int t = a;
				a = b;
				b = t;
			}

			save_undo_data(a, b);

			if (a != b)
			{
				swap_columns_br(&_palette[a], b-a);
				update_dac();
			}

			break;
		}

	case 'T':
	case 't':   /* s(T)ripe mode */
		{
			int key;

			cursor::cursor_hide();
			draw_status(true);
			key = getakeynohelp();
			cursor::cursor_show();

			if (key >= '1' && key <= '9')
			{
				int a = _current[0];
				int b = _current[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				save_undo_data(a, b);

				if (a != b)
				{
					make_pal_range(&_palette[a], &_palette[b], &_palette[a], b-a, key-'0');
					update_dac();
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
		_auto_select = !_auto_select;
		if (_auto_select)
		{
			set_current(_active, get_cursor_color());
			if (_exclude)
			{
				update_dac();
			}
		}
		break;

	case 'H':
	case 'h': /* toggle hide/display of palette editor */
		cursor::cursor_hide();
		hide(rgb, !_hidden);
		cursor::cursor_show();
		break;

	case '.':   /* rotate once */
	case ',':
		{
			int dir = (key == '.') ? 1 : -1;

			save_undo_rotate(dir, g_rotate_lo, g_rotate_hi);
			rotate(dir, g_rotate_lo, g_rotate_hi);
			break;
		}

	case '>':   /* continuous rotation (until a key is pressed) */
	case '<':
		{
			int  dir;
			long tick;
			int  diff = 0;

			cursor::cursor_hide();

			if (!_hidden)
			{
				_rgb_editors[0]->blank_sample_box();
				_rgb_editors[1]->blank_sample_box();
				_rgb_editors[0]->set_hidden(true);
				_rgb_editors[1]->set_hidden(true);
			}

			do
			{
				dir = (key == '>') ? 1 : -1;

				while (!driver_key_pressed())
				{
					tick = read_ticker();
					rotate(dir, g_rotate_lo, g_rotate_hi);
					diff += dir;
					while (read_ticker() == tick)   /* wait until a tick passes */
					{
					}
				}

				key = driver_get_key();
			}
			while (key == '<' || key == '>');

			if (!_hidden)
			{
				_rgb_editors[0]->set_hidden(false);
				_rgb_editors[1]->set_hidden(false);
				_rgb_editors[0]->update();
				_rgb_editors[1]->update();
			}

			if (diff != 0)
			{
				save_undo_rotate(diff, g_rotate_lo, g_rotate_hi);
			}

			cursor::cursor_show();
			break;
		}

	case 'I':     /* invert the fg & bg g_colors */
	case 'i':
		s_inverse = !s_inverse;
		update_dac();
		break;

	case 'V':
	case 'v':  /* set the reserved g_colors to the editor colors */
		if (_current[0] >= g_colors || _current[1] >= g_colors ||
			_current[0] == _current[1])
		{
			driver_buzzer(BUZZER_ERROR);
			break;
		}

		s_fg_color = (BYTE)_current[0];
		s_bg_color = (BYTE)_current[1];

		if (!_hidden)
		{
			cursor::cursor_hide();
			update_dac();
			draw();
			cursor::cursor_show();
		}

		_rgb_editors[_active]->set_done(true);
		break;

	case 'O':    /* set rotate_lo and rotate_hi to editors */
	case 'o':
		if (_current[0] > _current[1])
		{
			g_rotate_lo = _current[1];
			g_rotate_hi = _current[0];
		}
		else
		{
			g_rotate_lo = _current[0];
			g_rotate_hi = _current[1];
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

			if (_save_palette[which] != 0)
			{
				cursor::cursor_hide();

				save_undo_data(0, 255);
				memcpy(_palette, _save_palette[which], 256*3);
				update_dac();

				set_current(-1, 0);
				cursor::cursor_show();
				_rgb_editors[_active]->set_done(true);
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

			if (_save_palette[which] != 0)
			{
				memcpy(_save_palette[which], _palette, 256*3);
			}
			else
			{
				driver_buzzer(BUZZER_ERROR); /* oops! short on memory! */
			}
			break;
		}

	case 'L':     /* load a .map palette */
	case 'l':
		save_undo_data(0, 255);
		load_palette();
		get_pal_range(0, g_colors, _palette);
		update_dac();
		_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
		_rgb_editors[0]->update();
		_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
		_rgb_editors[1]->update();
		break;

	case 'S':     /* save a .map palette */
	case 's':
		set_pal_range(0, g_colors, _palette);
		save_palette();
		update_dac();
		break;

	case 'C':     /* color cycling sub-mode */
	case 'c':
		{
			bool oldhidden = _hidden;

			save_undo_data(0, 255);

			cursor::cursor_hide();
			if (!oldhidden)
			{
				hide(rgb, true);
			}
			set_pal_range(0, g_colors, _palette);
			::rotate(0);
			get_pal_range(0, g_colors, _palette);
			update_dac();
			if (!oldhidden)
			{
				_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
				_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
				hide(rgb, false);
			}
			cursor::cursor_show();
			break;
		}

	case 'F':
	case 'f':    /* toggle freestyle palette edit mode */
		_freestyle = !_freestyle;
		set_current(-1, 0);
		if (!_freestyle)   /* if turning off... */
		{
			update_dac();
		}
		break;

	case FIK_CTL_DEL:  /* rt plus down */
		if (_color_band_width >0)
		{
			_color_band_width--;
		}
		else
		{
			_color_band_width = 0;
		}
		set_current(-1, 0);
		break;

	case FIK_CTL_INSERT: /* rt plus up */
		if (_color_band_width <255)
		{
			_color_band_width ++;
		}
		else
		{
			_color_band_width = 255;
		}
		set_current(-1, 0);
		break;

	case 'W':   /* convert to greyscale */
	case 'w':
		switch (_exclude)
		{
		case EXCLUDE_NONE:   /* normal mode.  convert all colors to grey scale */
			save_undo_data(0, 255);
			pal_range_to_grey(_palette, 0, 256);
			break;

		case EXCLUDE_CURRENT:   /* 'x' mode. convert current color to grey scale.  */
			save_undo_data(_current[_active], _current[_active]);
			pal_range_to_grey(_palette, _current[_active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = _current[0];
				int b = _current[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				save_undo_data(a, b);
				pal_range_to_grey(_palette, a, 1 + (b-a));
				break;
			}
		}

		update_dac();
		_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
		_rgb_editors[0]->update();
		_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
		_rgb_editors[1]->update();
		break;

	case 'N':   /* convert to negative color */
	case 'n':
		switch (_exclude)
		{
		case EXCLUDE_NONE:      /* normal mode.  convert all colors to grey scale */
			save_undo_data(0, 255);
			pal_range_to_negative(_palette, 0, 256);
			break;

		case EXCLUDE_CURRENT:      /* 'x' mode. convert current color to grey scale.  */
			save_undo_data(_current[_active], _current[_active]);
			pal_range_to_negative(_palette, _current[_active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = _current[0];
				int b = _current[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				save_undo_data(a, b);
				pal_range_to_negative(_palette, a, 1 + (b-a));
				break;
			}
		}

		update_dac();
		_rgb_editors[0]->set_rgb(_current[0], &(_palette[_current[0]]));
		_rgb_editors[0]->update();
		_rgb_editors[1]->set_rgb(_current[1], &(_palette[_current[1]]));
		_rgb_editors[1]->update();
		break;

	case 'U':     /* Undo */
	case 'u':
		undo();
		break;

	case 'e':    /* Redo */
	case 'E':
		redo();
		break;
	} /* switch */
	draw_status(false);
}

void pal_table::make_default_palettes()  /* creates default Fkey palettes */
{
	for (int i = 0; i < 8; i++) /* copy original palette to save areas */
	{
		if (_save_palette[i] != 0)
		{
			memcpy(_save_palette[i], _palette, 256*3);
		}
	}
}

pal_table::pal_table()
	: _move_box(new move_box(0, 0, 0, PALTABLE_PALX + 1, PALTABLE_PALY + 1)),
	_active(0),
	_auto_select(true),
	_exclude(EXCLUDE_NONE),
	_hidden(false),
	_stored_at(NOWHERE),
	_file(0),
	_memory(0),
	_freestyle(false),
	_color_band_width(15),
	_top(255),
	_bottom(0),
	_undo_file(dir_fopen(g_temp_dir, s_undo_file, "w+b")),
	_current_changed(false),
	_num_redo(0)
{
	for (int ctr = 0; ctr < 8; ctr++)
	{
		_save_palette[ctr] = new PALENTRY[256];
	}

	_rgb_editors[0] = new rgb_editor(0, 0, other_key, change, this);
	_rgb_editors[1] = new rgb_editor(0, 0, other_key, change, this);

	_current[0]     = 1;
	_current[1]     = 1;

	_fs_color.red   = 42;
	_fs_color.green = 42;
	_fs_color.blue  = 42;

	_rgb_editors[0]->set_rgb(_current[0], &_palette[_current[0]]);
	_rgb_editors[1]->set_rgb(_current[1], &_palette[_current[0]]);

	set_position(0, 0);
	int csize = ((g_screen_height-(PALTABLE_PALY + 1 + 1))/2)/16;
	if (csize < CSIZE_MIN)
	{
		csize = CSIZE_MIN;
	}
	set_csize(csize);
}


void pal_table::set_hidden(bool hidden)
{
	_hidden = hidden;
	_rgb_editors[0]->set_hidden(hidden);
	_rgb_editors[1]->set_hidden(hidden);
	update_dac();
}



void pal_table::hide(rgb_editor *rgb, bool hidden)
{
	if (hidden)
	{
		restore_rect();
		set_hidden(true);
		s_reserve_colors = false;
		if (_auto_select)
		{
			set_current(_active, get_cursor_color());
		}
	}
	else
	{
		set_hidden(false);
		s_reserve_colors = true;
		if (_stored_at == NOWHERE)  /* do we need to save screen? */
		{
			save_rect();
		}
		draw();
		if (_auto_select)
		{
			set_current(_active, get_cursor_color());
		}
		rgb->set_done(true);
	}
}

template <typename T>
inline void destroy(T *&ptr)
{
	delete ptr;
	ptr = 0;
}
template <typename T>
inline void destroy_array(T *&ptr)
{
	delete[] ptr;
	ptr = 0;
}

pal_table::~pal_table()
{
	if (_file)
	{
		fclose(_file);
		dir_remove(g_temp_dir, g_screen_file);
	}

	if (_undo_file)
	{
		fclose(_undo_file);
		dir_remove(g_temp_dir, s_undo_file);
	}

	destroy(_memory);

	for (int i = 0; i < 8; i++)
	{
		destroy_array(_save_palette[i]);
	}

	destroy(_rgb_editors[0]);
	destroy(_rgb_editors[1]);
	destroy(_move_box);
}


void pal_table::process()
{
	get_pal_range(0, g_colors, _palette);

	/* Make sure all palette entries are 0-COLOR_CHANNEL_MAX */
	for (int ctr = 0; ctr < 768; ctr++)
	{
		((char *)_palette)[ctr] &= COLOR_CHANNEL_MAX;
	}

	update_dac();

	_rgb_editors[0]->set_rgb(_current[0], &_palette[_current[0]]);
	_rgb_editors[1]->set_rgb(_current[1], &_palette[_current[0]]);

	if (!_hidden)
	{
		_move_box->set_position(_x, _y);
		_move_box->set_csize(_csize);
		if (!_move_box->process())
		{
			set_pal_range(0, g_colors, _palette);
			return;
		}

		set_position(_move_box->x(), _move_box->y());
		set_csize(_move_box->csize());

		if (_move_box->should_hide())
		{
			set_hidden(true);
			s_reserve_colors = false;   /* <EAN> */
		}
		else
		{
			s_reserve_colors = true;    /* <EAN> */
			save_rect();
			draw();
		}
	}

	set_current(_active,          get_cursor_color());
	set_current((_active == 1) ? 0 : 1, get_cursor_color());
	cursor::cursor_show();
	make_default_palettes();
	_done = false;

	while (!_done)
	{
		_rgb_editors[_active]->edit();
	}

	cursor::cursor_hide();
	restore_rect();
	set_pal_range(0, g_colors, _palette);
}


/*
 * interface to FRACTINT
 */



void palette_edit()       /* called by fractint */
{
	if (g_screen_width < 133 || g_screen_height < 174)
	{
		return; /* prevents crash when physical screen is too small */
	}

	HelpModeSaver saved_help(HELPXHAIR);
	MouseModeSaver saved_mouse(LOOK_MOUSE_ZOOM_BOX);

	g_plot_color = g_plot_color_put_color;

	g_line_buffer = new BYTE[std::max(g_screen_width, g_screen_height)];

	int oldsxoffs = g_sx_offset;
	int oldsyoffs = g_sy_offset;
	g_sx_offset = 0;
	g_sy_offset = 0;

	s_reserve_colors = true;
	s_inverse = false;
	s_fg_color = BYTE(255 % g_colors);
	s_bg_color = BYTE(s_fg_color-1);

	cursor::create();
	{
		pal_table().process();
	}
	cursor::destroy();

	g_sx_offset = oldsxoffs;
	g_sy_offset = oldsyoffs;
	destroy_array(g_line_buffer);
}
