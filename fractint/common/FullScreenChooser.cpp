#include <string>

#include "port.h"
#include "prototyp.h"
#include "id.h"

#include "drivers.h"
#include "fihelp.h"
#include "FullScreenChooser.h"
#include "miscres.h"
#include "realdos.h"

class ProductionFullScreenChooser : public AbstractFullScreenChooser
{
public:
	ProductionFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
			int numChoices, char **choices, const int *attributes,
			int boxWidth, int boxDepth, int columnWidth, int current,
			void (*formatItem)(int, char*), char *speedString,
			int (*speedPrompt)(int, int, int, char *, int), int (*checkKey)(int, int))
		: AbstractFullScreenChooser(options, heading, heading2, instructions,
			numChoices, choices, attributes,
			boxWidth, boxDepth, columnWidth, current,
			formatItem, speedString,
			speedPrompt, checkKey)
	{
	}
	virtual ~ProductionFullScreenChooser()
	{
	}

protected:
	virtual void help_title()
	{ ::help_title(); }
	virtual void driver_set_attr(int row, int col, int attr, int count)
	{ ::driver_set_attr(row, col, attr, count); }
	virtual void driver_put_string(int row, int col, int attr, const char *msg)
	{ ::driver_put_string(row, col, attr, msg); }
	virtual void driver_hide_text_cursor()
	{ ::driver_hide_text_cursor(); }
	virtual void driver_buzzer(int kind)
	{ ::driver_buzzer(kind); }
	virtual int driver_key_pressed()
	{ return ::driver_key_pressed(); }
	virtual int driver_get_key()
	{ return ::driver_get_key(); }
	virtual int getakeynohelp()
	{ return ::getakeynohelp(); }
	virtual void blank_rows(int row, int rows, int attr)
	{ ::blank_rows(row, rows, attr); }
	virtual void driver_unstack_screen()
	{ ::driver_unstack_screen(); }
};

AbstractFullScreenChooser::AbstractFullScreenChooser(int options,
		const char *heading, const char *heading2, const char *instructions,
		int numChoices, char **choices, const int *attributes,
		int boxWidth, int boxDepth, int columnWidth, int current,
		void (*formatItem)(int, char*), char *speedString,
		int (*speedPrompt)(int, int, int, char *, int), int (*checkKey)(int, int))
	: _options(options),
	_heading(heading),
	_heading2(heading2),
	_instructions(instructions),
	_numChoices(numChoices),
	_choices(choices),
	_attributes(attributes),
	_boxWidth(boxWidth),
	_boxDepth(boxDepth),
	_columnWidth(columnWidth),
	_current(current),
	_formatItem(formatItem),
	_speedString(speedString),
	_speedPrompt(speedPrompt),
	_checkKey(checkKey)
{
}

int AbstractFullScreenChooser::Execute()
{
	/* speed key prompt */
	/* boxwidth*boxdepth */
	int scrunch = (_options & CHOICE_CRUNCH) ? 1 : 0;		/* scrunch up a line */

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	/* preset current to passed string */
	if (_speedString && _speedString[0])
	{
		InitializeCurrent();
	}

	while (true)
	{
		if (_current >= _numChoices)  /* no real choice in the list? */
		{
			return -1;
		}
		if ((_attributes[_current] & 256) == 0)
		{
			break;
		}
		++_current;                  /* scan for a real choice */
	}

	int title_lines = 0;
	int title_width = 0;
	if (_heading)
	{
		const char *tmp = _heading;              /* count title lines, find widest */
		int i = 0;
		title_lines = 1;
		while (*tmp)
		{
			if (*(tmp++) == '\n')
			{
				++title_lines;
				i = -1;
			}
			if (++i > title_width)
			{
				title_width = i;
			}
		}
	}

	if (_columnWidth == 0)             /* find widest column */
	{
		for (int i = 0; i < _numChoices; ++i)
		{
			int len = int(strlen(_choices[i]));
			if (len > _columnWidth)
			{
				_columnWidth = len;
			}
		}
	}
	/* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
	int required_rows = 3 - scrunch;                /* calc rows available */
	if (_heading)
	{
		required_rows += title_lines + 1;
	}
	if (_instructions)                   /* count instructions lines */
	{
		const char *tmp = _instructions;
		++required_rows;
		while (*tmp)
		{
			if (*(tmp++) == '\n')
			{
				++required_rows;
			}
		}
		if ((_options & CHOICE_INSTRUCTIONS))          /* show std instr too */
		{
			required_rows += 2;
		}
	}
	else
	{
		required_rows += 2;              /* standard instructions */
	}
	if (_speedString)
	{
		++required_rows;   /* a row for speedkey prompt */
	}
	int i = 25 - required_rows;
	if (_boxDepth > i) /* limit the depth to max */
	{
		_boxDepth = i;
	}
	if (_boxWidth == 0)           /* pick box width and depth */
	{
		if (_numChoices <= i - 2)  /* single column is 1st choice if we can */
		{
			_boxDepth = _numChoices;
			_boxWidth = 1;
		}
		else
		{                      /* sort-of-wide is 2nd choice */
			_boxWidth = 60/(_columnWidth + 1);
			if (_boxWidth == 0
				|| (_boxDepth = (_numChoices + _boxWidth - 1)/_boxWidth) > i - 2)
			{
				_boxWidth = 80/(_columnWidth + 1); /* last gasp, full width */
				_boxDepth = (_numChoices + _boxWidth - 1)/_boxWidth;
				if (_boxDepth > i)
				{
					_boxDepth = i;
				}
			}
		}
	}
	int i2 = (80 / _boxWidth - _columnWidth) / 2 - 1;
	if (i2 == 0) /* to allow wider prompts */
	{
		i2 = 1;
	}
	if (i2 < 0)
	{
		i2 = 0;
	}
	if (i2 > 3)
	{
		i2 = 3;
	}
	int j = _boxWidth*(_columnWidth += i2) + i2;     /* overall width of box */
	if (j < title_width + 2)
	{
		j = title_width + 2;
	}
	if (j > 80)
	{
		j = 80;
	}
	if (j <= 70 && _boxWidth == 2)         /* special case makes menus nicer */
	{
		++j;
		++_columnWidth;
	}
	int k = (80 - j)/2;                       /* center the box */
	k -= (90 - j)/20;
	int top_left_col = k + i2;                     /* column of topleft choice */
	int i3 = (25 - required_rows - _boxDepth) / 2;
	i3 -= i3/4;                             /* higher is better if lots extra */
	int top_left_row = 3 + title_lines + i3;        /* row of topleft choice */

	/* now set up the overall display */
	help_title();                            /* clear, display title line */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      /* init rest to background */
	int i4;
	for (i4 = top_left_row - 1 - title_lines; i4 < top_left_row + _boxDepth + 1; ++i4)
	{
		driver_set_attr(i4, k, C_PROMPT_LO, j);          /* draw empty box */
	}
	if (_heading)
	{
		g_text_cbase = (80 - title_width)/2;   /* set left margin for driver_put_string */
		g_text_cbase -= (90 - title_width)/20; /* put heading into box */
		driver_put_string(top_left_row - title_lines - 1, 0, C_PROMPT_HI, _heading);
		g_text_cbase = 0;
	}
	if (_heading2)                               /* display 2nd heading */
	{
		driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_MED, _heading2);
	}
	int i5 = top_left_row + _boxDepth + 1;
	int speed_row = 0;
	if (_instructions == 0 || (_options & CHOICE_INSTRUCTIONS))   /* display default instructions */
	{
		if (i5 < 20)
		{
			++i5;
		}
		if (_speedString)
		{
			speed_row = i5;
			_speedString[0] = 0;
			if (++i5 < 22)
			{
				++i5;
			}
		}
		i5 -= scrunch;
		Footer(i5);
	}
	char buf[81];
	if (_instructions)                            /* display caller's instructions */
	{
		const char *tmp = _instructions;
		int j2 = -1;
		while ((buf[++j2] = *(tmp++)) != 0)
		{
			if (buf[j2] == '\n')
			{
				buf[j2] = 0;
				put_string_center(i5++, 0, 80, C_PROMPT_BKGRD, buf);
				j2 = -1;
			}
		}
		put_string_center(i5, 0, 80, C_PROMPT_BKGRD, buf);
	}

	int box_items = _boxWidth*_boxDepth;
	int top_left_choice = 0;                      /* pick topleft for init display */
	while (_current - top_left_choice >= box_items
		|| (_current - top_left_choice > box_items/2
		&& top_left_choice + box_items < _numChoices))
	{
		top_left_choice += _boxWidth;
	}
	bool redisplay = true;
	top_left_row -= scrunch;
	int rev_increment = 0;
	char current_item[81];
	while (true) /* main loop */
	{
		if (redisplay)                       /* display the current choices */
		{
			memset(buf, ' ', 80);
			buf[_boxWidth*_columnWidth] = 0;
			for (int i = (_heading2) ? 0 : -1; i <= _boxDepth; ++i)  /* blank the box */
			{
				driver_put_string(top_left_row + i, top_left_col, C_PROMPT_LO, buf);
			}
			for (int i = 0; i + top_left_choice < _numChoices && i < box_items; ++i)
			{
				/* display the choices */
				int j = i + top_left_choice;
				const char *tmp;
				if (_formatItem)
				{
					(*_formatItem)(j, buf);
					tmp = buf;
				}
				else
				{
					tmp = _choices[j];
				}
				driver_put_string(top_left_row + i/_boxWidth, top_left_col + (i % _boxWidth)*_columnWidth,
					prompt_color(_attributes[j]), tmp);
			}
			/***
			... format differs for summary/detail, whups, force box width to
			...  be 72 when detail toggle available?  (2 grey margin each
			...  side, 1 blue margin each side)
			***/
			if (top_left_choice > 0 && _heading2 == 0)
			{
				driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_LO, "(more)");
			}
			if (top_left_choice + box_items < _numChoices)
			{
				driver_put_string(top_left_row + _boxDepth, top_left_col, C_PROMPT_LO, "(more)");
			}
			redisplay = false;
		}

		int i = _current - top_left_choice;           /* highlight the current choice */
		const char *itemText;
		if (_formatItem)
		{
			(*_formatItem)(_current, current_item);
			itemText = current_item;
		}
		else
		{
			itemText = _choices[_current];
		}
		driver_put_string(top_left_row + i/_boxWidth, top_left_col + (i % _boxWidth)*_columnWidth,
			C_CHOICE_CURRENT, itemText);

		if (_speedString)                     /* show speedstring if any */
		{
			show_speed_string(speed_row, _speedString, _speedPrompt);
		}
		else
		{
			driver_hide_text_cursor();
		}

		driver_wait_key_pressed(0);					/* enables help */
		int current_key = driver_get_key();
		i = _current - top_left_choice;				/* unhighlight current choice */
		driver_put_string(top_left_row + i/_boxWidth, top_left_col + (i % _boxWidth)*_columnWidth,
			prompt_color(_attributes[_current]), itemText);

		int increment = 0;
		switch (current_key)
		{                      /* deal with input key */
		case FIK_ENTER:
		case FIK_ENTER_2:
			return _current;
		case FIK_ESC:
			return -1;
		case FIK_DOWN_ARROW:
			increment = _boxWidth;
			rev_increment = -increment;
			break;
		case FIK_CTL_DOWN_ARROW:
			increment = _boxWidth;
			rev_increment = -increment;
			{
				int newcurrent = _current;
				while ((newcurrent += _boxWidth) != _current)
				{
					if (newcurrent >= _numChoices)
					{
						newcurrent = (newcurrent % _boxWidth) - _boxWidth;
					}
					else if (!is_a_dir_name(_choices[newcurrent]))
					{
						if (_current != newcurrent)
						{
							_current = newcurrent - _boxWidth;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_UP_ARROW:
			rev_increment = _boxWidth;
			increment = -rev_increment;
			break;
		case FIK_CTL_UP_ARROW:
			rev_increment = _boxWidth;
			increment = -rev_increment;
			{
				int newcurrent = _current;
				while ((newcurrent -= _boxWidth) != _current)
				{
					if (newcurrent < 0)
					{
						newcurrent = (_numChoices - _current) % _boxWidth;
						newcurrent =  _numChoices + (newcurrent ? _boxWidth - newcurrent: 0);
					}
					else if (!is_a_dir_name(_choices[newcurrent]))
					{
						if (_current != newcurrent)
						{
							_current = newcurrent + _boxWidth;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_RIGHT_ARROW:
			increment = 1;
			rev_increment = -1;
			break;
		case FIK_CTL_RIGHT_ARROW:  /* move to next file; if at last file, go to first file */
			increment = 1;
			rev_increment = -1;
			{
				int newcurrent = _current;
				while (++newcurrent != _current)
				{
					if (newcurrent >= _numChoices)
					{
						newcurrent = -1;
					}
					else if (!is_a_dir_name(_choices[newcurrent]))
					{
						if (_current != newcurrent)
						{
							_current = newcurrent - 1;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_LEFT_ARROW:
			increment = -1;
			rev_increment = 1;
			break;
		case FIK_CTL_LEFT_ARROW: /* move to previous file; if at first file, go to last file */
			increment = -1;
			rev_increment = 1;
			{
				int newcurrent = _current;
				while (--newcurrent != _current)
				{
					if (newcurrent < 0)
					{
						newcurrent = _numChoices;
					}
					else if (!is_a_dir_name(_choices[newcurrent]))
					{
						if (_current != newcurrent)
						{
							_current = newcurrent + 1;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_PAGE_UP:
			if (_numChoices > box_items)
			{
				top_left_choice -= box_items;
				increment = -box_items;
				rev_increment = _boxWidth;
				redisplay = true;
			}
			break;
		case FIK_PAGE_DOWN:
			if (_numChoices > box_items)
			{
				top_left_choice += box_items;
				increment = box_items;
				rev_increment = -_boxWidth;
				redisplay = true;
			}
			break;
		case FIK_HOME:
			_current = -1;
			increment = 1;
			rev_increment = 1;
			break;
		case FIK_CTL_HOME:
			_current = -1;
			increment = 1;
			rev_increment = 1;
			{
				int newcurrent;
				for (newcurrent = 0; newcurrent < _numChoices; ++newcurrent)
				{
					if (!is_a_dir_name(_choices[newcurrent]))
					{
						_current = newcurrent - 1;
						break;  /* breaks the for loop */
					}
				}
			}
			break;
		case FIK_END:
			_current = _numChoices;
			increment = -1;
			rev_increment = -1;
			break;
		case FIK_CTL_END:
			_current = _numChoices;
			increment = -1;
			rev_increment = -1;
			{
				int newcurrent;
				for (newcurrent = _numChoices - 1; newcurrent >= 0; --newcurrent)
				{
					if (!is_a_dir_name(_choices[newcurrent]))
					{
						_current = newcurrent + 1;
						break;  /* breaks the for loop */
					}
				}
			}
			break;
		default:
			if (_checkKey)
			{
				int ret2 = (*_checkKey)(current_key, _current);
				if (ret2 != -1 && ret2 != 0)
				{
					return ret2;
				}
				if (ret2 == -1)
				{
					redisplay = true;
				}
			}
			if (_speedString)
			{
				process_speed_string(_speedString, _choices, current_key, &_current,
					_numChoices, _options & CHOICE_NOT_SORTED);
			}
			break;
		}
		if (increment)                  /* apply cursor movement */
		{
			_current += increment;
			if (_speedString)               /* zap speedstring */
			{
				_speedString[0] = 0;
			}
		}
		while (true)
		{                 /* adjust to a non-comment choice */
			if (_current < 0 || _current >= _numChoices)
			{
				increment = rev_increment;
			}
			else if ((_attributes[_current] & 256) == 0)
			{
				break;
			}
			_current += increment;
		}
		if (top_left_choice > _numChoices - box_items)
		{
			top_left_choice = ((_numChoices + _boxWidth - 1)/_boxWidth)*_boxWidth - box_items;
		}
		if (top_left_choice < 0)
		{
			top_left_choice = 0;
		}
		while (_current < top_left_choice)
		{
			top_left_choice -= _boxWidth;
			redisplay = true;
		}
		while (_current >= top_left_choice + box_items)
		{
			top_left_choice += _boxWidth;
			redisplay = true;
		}
	}

	return -1;
}

void AbstractFullScreenChooser::InitializeCurrent()
{
	int speedLength = int(strlen(_speedString));
	_current = 0;
	int speedCompare = strncasecmp(_speedString, _choices[_current], speedLength);
	if (_options & CHOICE_NOT_SORTED)
	{
		while (_current < _numChoices && speedCompare != 0)
		{
			++_current;
			speedCompare = strncasecmp(_speedString, _choices[_current], speedLength);
		}
		if (speedCompare != 0)
		{
			_current = 0;
		}
	}
	else
	{
		while (_current < _numChoices && speedCompare > 0)
		{
			++_current;
			speedCompare = strncasecmp(_speedString, _choices[_current], speedLength);
		}
		if (speedCompare < 0 && _current > 0)  /* oops - overshot */
		{
			--_current;
		}
	}
	if (_current >= _numChoices) /* bumped end of list */
	{
		_current = _numChoices - 1;
	}
}

void AbstractFullScreenChooser::Footer(int &i)
{
	put_string_center(i++, 0, 80, C_PROMPT_BKGRD,
		(_speedString) ? "Use the cursor keys or type a value to make a selection"
		: "Use the cursor keys to highlight your selection");
	put_string_center(i++, 0, 80, C_PROMPT_BKGRD,
			(_options & CHOICE_MENU) ? "Press ENTER for highlighted choice, or "FK_F1" for help"
		: ((_options & CHOICE_HELP) ? "Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help"
		: "Press ENTER for highlighted choice, or ESCAPE to back out"));
}

/* For file list purposes only, it's a directory name if first
	char is a dot or last char is a slash */
bool AbstractFullScreenChooser::is_a_dir_name(const char *name)
{
	return name[0] == '.' || ends_with_slash(name) ? true : false;
}

void AbstractFullScreenChooser::show_speed_string(int speedrow, char *speedstring, int (*speed_prompt)(int, int, int, char *, int))
{
	int speed_match = 0;
	int i;
	int j;
	char buf[81];
	memset(buf, ' ', 80);
	buf[80] = 0;
	driver_put_string(speedrow, 0, C_PROMPT_BKGRD, buf);
	if (*speedstring)  /* got a speedstring on the go */
	{
		driver_put_string(speedrow, 15, C_CHOICE_SP_INSTR, " ");
		if (speed_prompt)
		{
			j = speed_prompt(speedrow, 16, C_CHOICE_SP_INSTR, speedstring, speed_match);
		}
		else
		{
			driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, "Speed key string");
			j = sizeof("Speed key string")-1;
		}
		strcpy(buf, speedstring);
		i = int(strlen(buf));
		while (i < 30)
		{
			buf[i++] = ' ';
		}
		buf[i] = 0;
		driver_put_string(speedrow, 16 + j, C_CHOICE_SP_INSTR, " ");
		driver_put_string(speedrow, 17 + j, C_CHOICE_SP_KEYIN, buf);
		driver_move_cursor(speedrow, 17 + j + int(strlen(speedstring)));
	}
	else
	{
		driver_hide_text_cursor();
	}
}

void AbstractFullScreenChooser::process_speed_string(char *speedstring, char **choices, int curkey,
											 int *pcurrent, int numchoices, int is_unsorted)
{
	int i;
	int comp_result;

	i = int(strlen(speedstring));
	if (curkey == 8 && i > 0) /* backspace */
	{
		speedstring[--i] = 0;
	}
	if (33 <= curkey && curkey <= 126 && i < 30)
	{
		curkey = tolower(curkey);
		speedstring[i] = (char)curkey;
		speedstring[++i] = 0;
	}
	if (i > 0)   /* locate matching type */
	{
		*pcurrent = 0;
		while (*pcurrent < numchoices
			&& (comp_result = strncasecmp(speedstring, choices[*pcurrent], i)) != 0)
		{
			if (comp_result < 0 && !is_unsorted)
			{
				*pcurrent -= *pcurrent ? 1 : 0;
				break;
			}
			else
			{
				++*pcurrent;
			}
		}
		if (*pcurrent >= numchoices) /* bumped end of list */
		{
			*pcurrent = numchoices - 1;
				/*if the list is unsorted, and the entry found is not the exact
					entry, then go looking for the exact entry.
				*/
		}
		else if (is_unsorted && choices[*pcurrent][i])
		{
			int temp = *pcurrent;
			while (++temp < numchoices)
			{
				if (!choices[temp][i] && !strncasecmp(speedstring, choices[temp], i))
				{
					*pcurrent = temp;
					break;
				}
			}
		}
	}
}

/*
return is:
	n >= 0 for choice n selected,
	-1 for escape
	k for check_key routine return value k (if not 0 nor -1)
	speedstring[0] != 0 on return if string is present
*/
int full_screen_choice(
	int options,
	const char *heading,				/* heading info, \n delimited */
	const char *heading2,				/* column heading or 0 */
	const char *instructions,			/* instructions, \n delimited, or 0 */
	int num_choices,					/* How many choices in list */
	char **choices,						/* array of choice strings */
	const int *attributes,				/* &3: 0 normal color, 1, 3 highlight */
										/* &256 marks a dummy entry */
	int box_width,						/* box width, 0 for calc (in items) */
	int box_depth,						/* box depth, 0 for calc, 99 for max */
	int column_width,					/* data width of a column, 0 for calc */
	int current,						/* start with this item */
	void (*format_item)(int, char*),	/* routine to display an item or 0 */
	char *speed_string,					/* returned speed key value, or 0 */
	int (*speed_prompt)(int, int, int, char *, int), /* routine to display prompt or 0 */
	int (*check_key)(int, int)			/* routine to check keystroke or 0 */
	)
{
	return ProductionFullScreenChooser(options, heading, heading2, instructions,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_key).Execute();
}

int full_screen_choice(int options,
	const std::string &heading, const std::string &heading2,
	const std::string &instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int item, char *text),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_key)(int, int))
{
	return full_screen_choice(options, heading.c_str(),
		heading2.length() ? heading2.c_str() : 0,
		instructions.length() ? instructions.c_str() : 0,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_key);
}

int full_screen_choice_help(int help_mode, int options,
	const char *heading, const char *heading2, const char *instr,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char*),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_key)(int, int))
{
	int result;
	HelpModeSaver saved_help(help_mode);
	result = full_screen_choice(options, heading, heading2, instr,
		num_choices, choices, attributes, box_width, box_depth, column_width,
		current, format_item, speed_string, speed_prompt, check_key);
	return result;
}
