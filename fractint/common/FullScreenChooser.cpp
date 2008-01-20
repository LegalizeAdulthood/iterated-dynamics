#include <string>

#include "port.h"
#include "prototyp.h"
#include "id.h"

#include "drivers.h"
#include "fihelp.h"
#include "FullScreenChooser.h"
#include "miscres.h"
#include "realdos.h"
#include "TextColors.h"

int AbstractFullScreenChooser::prompt_color(int attributes)
{
	switch (attributes & 3)
	{
	case 1:		return C_PROMPT_LO;
	case 3:		return C_PROMPT_HI;
	default:	return C_PROMPT_MED;
	}
}

class ProductionFullScreenChooser : public AbstractFullScreenChooser
{
public:
	ProductionFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
			int numChoices, char **choices, const int *attributes,
			int boxWidth, int boxDepth, int columnWidth, int current,
			void (*formatItem)(int, char*), char *speedString,
			int (*speedPrompt)(int, int, int, char *, int), int (*checkKeystroke)(int, int),
			AbstractDriver *driver = DriverManager::current())
		: AbstractFullScreenChooser(options, heading, heading2, instructions,
			numChoices, choices, attributes,
			boxWidth, boxDepth, columnWidth, current,
			formatItem, speedString,
			speedPrompt, checkKeystroke, driver)
	{
	}
	virtual ~ProductionFullScreenChooser()
	{
	}

protected:
	virtual void help_title()
	{ ::help_title(); }
	virtual int get_key_no_help()
	{ return ::get_key_no_help(); }
	virtual void blank_rows(int row, int rows, int attr)
	{ ::blank_rows(row, rows, attr); }
};

AbstractFullScreenChooser::AbstractFullScreenChooser(int options,
		const char *heading, const char *heading2, const char *instructions,
		int numChoices, char **choices, const int *attributes,
		int boxWidth, int boxDepth, int columnWidth, int current,
		void (*formatItem)(int, char*), char *speedString,
		int (*speedPrompt)(int, int, int, char *, int), int (*checkKeystroke)(int, int),
		AbstractDriver *driver)
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
	_checkKeystroke(checkKeystroke),
	_driver(driver)
{
}

int AbstractFullScreenChooser::Execute()
{
	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);

	if (InitializeCurrent())
	{
		return -1;
	}

	CountTitleLinesAndWidth();
	FindWidestColumn();

	/* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
	int scrunch = (_options & CHOICE_CRUNCH) ? 1 : 0;
	int requiredRows = GetRequiredRows(scrunch);
	ComputeBoxDepthAndWidth(requiredRows);
	int i2 = (80/_boxWidth - _columnWidth)/2 - 1;
	if (i2 == 0) /* to allow wider prompts */
	{
		i2 = 1;
	}
	else if (i2 < 0)
	{
		i2 = 0;
	}
	else if (i2 > 3)
	{
		i2 = 3;
	}
	_columnWidth += i2;
	int overallWidth = _boxWidth*_columnWidth + i2;     /* overall width of box */
	if (overallWidth < _titleWidth + 2)
	{
		overallWidth = _titleWidth + 2;
	}
	if (overallWidth > 80)
	{
		overallWidth = 80;
	}
	if (overallWidth <= 70 && _boxWidth == 2)         /* special case makes menus nicer */
	{
		++overallWidth;
		++_columnWidth;
	}
	int k = (80 - overallWidth)/2;                       /* center the box */
	k -= (90 - overallWidth)/20;
	int top_left_col = k + i2;                     /* column of topleft choice */
	int i3 = (25 - requiredRows - _boxDepth) / 2;
	i3 -= i3/4;                             /* higher is better if lots extra */
	int top_left_row = 3 + _titleLines + i3;        /* row of topleft choice */

	/* now set up the overall display */
	help_title();                            /* clear, display title line */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      /* init rest to background */
	for (int i = top_left_row - 1 - _titleLines; i < top_left_row + _boxDepth + 1; ++i)
	{
		driver_set_attr(i, k, C_PROMPT_LO, overallWidth);          /* draw empty box */
	}
	if (_heading)
	{
		g_text_cbase = (80 - _titleWidth)/2;   /* set left margin for driver_put_string */
		g_text_cbase -= (90 - _titleWidth)/20; /* put heading into box */
		driver_put_string(top_left_row - _titleLines - 1, 0, C_PROMPT_HI, _heading);
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
			show_speed_string(speed_row);
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
			if (_checkKeystroke)
			{
				int ret2 = (*_checkKeystroke)(current_key, _current);
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
				process_speed_string(current_key, (_options & CHOICE_NOT_SORTED) != 0);
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

void AbstractFullScreenChooser::ComputeBoxDepthAndWidth(int requiredRows)
{
	int i = 25 - requiredRows;
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
}

int AbstractFullScreenChooser::GetRequiredRows(int scrunch)
{
	int requiredRows = 3 - scrunch;                /* calc rows available */
	if (_heading)
	{
		requiredRows += _titleLines + 1;
	}
	if (_instructions)                   /* count instructions lines */
	{
		const char *tmp = _instructions;
		++requiredRows;
		while (*tmp)
		{
			if (*(tmp++) == '\n')
			{
				++requiredRows;
			}
		}
		if ((_options & CHOICE_INSTRUCTIONS))          /* show std instr too */
		{
			requiredRows += 2;
		}
	}
	else
	{
		requiredRows += 2;              /* standard instructions */
	}
	if (_speedString)
	{
		++requiredRows;   /* a row for speedkey prompt */
	}
	return requiredRows;
}

void AbstractFullScreenChooser::FindWidestColumn()
{
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
}

void AbstractFullScreenChooser::CountTitleLinesAndWidth()
{
	_titleLines = 0;
	_titleWidth = 0;
	if (_heading)
	{
		const char *tmp = _heading;              /* count title lines, find widest */
		int i = 0;
		_titleLines = 1;
		while (*tmp)
		{
			if (*(tmp++) == '\n')
			{
				++_titleLines;
				i = -1;
			}
			if (++i > _titleWidth)
			{
				_titleWidth = i;
			}
		}
	}
}

bool AbstractFullScreenChooser::InitializeCurrent()
{
	/* preset current to passed string */
	if (_speedString && _speedString[0])
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

	while (true)
	{
		if (_current >= _numChoices)  /* no real choice in the list? */
		{
			return true;
		}
		if ((_attributes[_current] & 256) == 0)
		{
			break;
		}
		++_current;                  /* scan for a real choice */
	}

	return false;
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

void AbstractFullScreenChooser::show_speed_string(int speedrow)
{
	int speed_match = 0;
	int i;
	int j;
	char buf[81];
	memset(buf, ' ', 80);
	buf[80] = 0;
	driver_put_string(speedrow, 0, C_PROMPT_BKGRD, buf);
	if (*_speedString)  /* got a speedstring on the go */
	{
		driver_put_string(speedrow, 15, C_CHOICE_SP_INSTR, " ");
		if (_speedPrompt)
		{
			j = _speedPrompt(speedrow, 16, C_CHOICE_SP_INSTR, _speedString, speed_match);
		}
		else
		{
			driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, "Speed key string");
			j = sizeof("Speed key string")-1;
		}
		strcpy(buf, _speedString);
		i = int(strlen(buf));
		while (i < 30)
		{
			buf[i++] = ' ';
		}
		buf[i] = 0;
		driver_put_string(speedrow, 16 + j, C_CHOICE_SP_INSTR, " ");
		driver_put_string(speedrow, 17 + j, C_CHOICE_SP_KEYIN, buf);
		driver_move_cursor(speedrow, 17 + j + int(strlen(_speedString)));
	}
	else
	{
		driver_hide_text_cursor();
	}
}

void AbstractFullScreenChooser::process_speed_string(int curkey, bool is_unsorted)
{
	int i;
	int comp_result;

	i = int(strlen(_speedString));
	if (curkey == 8 && i > 0) /* backspace */
	{
		_speedString[--i] = 0;
	}
	if (33 <= curkey && curkey <= 126 && i < 30)
	{
		curkey = tolower(curkey);
		_speedString[i] = (char)curkey;
		_speedString[++i] = 0;
	}
	if (i > 0)   /* locate matching type */
	{
		_current = 0;
		while (_current < _numChoices
			&& (comp_result = strncasecmp(_speedString, _choices[_current], i)) != 0)
		{
			if (comp_result < 0 && !is_unsorted)
			{
				_current -= _current ? 1 : 0;
				break;
			}
			else
			{
				++_current;
			}
		}
		if (_current >= _numChoices) /* bumped end of list */
		{
			_current = _numChoices - 1;
				/*if the list is unsorted, and the entry found is not the exact
					entry, then go looking for the exact entry.
				*/
		}
		else if (is_unsorted && _choices[_current][i])
		{
			int temp = _current;
			while (++temp < _numChoices)
			{
				if (!_choices[temp][i] && !strncasecmp(_speedString, _choices[temp], i))
				{
					_current = temp;
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
	k for check_keystroke routine return value k (if not 0 nor -1)
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
	int (*check_keystroke)(int, int)			/* routine to check keystroke or 0 */
	)
{
	return ProductionFullScreenChooser(options, heading, heading2, instructions,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_keystroke).Execute();
}

int full_screen_choice(int options,
	const std::string &heading, const std::string &heading2,
	const std::string &instructions,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int item, char *text),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int))
{
	return full_screen_choice(options, heading.c_str(),
		heading2.length() ? heading2.c_str() : 0,
		instructions.length() ? instructions.c_str() : 0,
		num_choices, choices, attributes,
		box_width, box_depth, column_width, current,
		format_item, speed_string, speed_prompt, check_keystroke);
}

int full_screen_choice_help(int help_mode, int options,
	const char *heading, const char *heading2, const char *instr,
	int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char*),
	char *speed_string, int (*speed_prompt)(int, int, int, char *, int),
	int (*check_keystroke)(int, int))
{
	int result;
	HelpModeSaver saved_help(help_mode);
	result = full_screen_choice(options, heading, heading2, instr,
		num_choices, choices, attributes, box_width, box_depth, column_width,
		current, format_item, speed_string, speed_prompt, check_keystroke);
	return result;
}
