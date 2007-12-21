#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/format.hpp>

extern BYTE g_text_colors[];
#include "id.h"
#include "FullScreenChooser.h"

class TestFullScreenChooser : public AbstractFullScreenChooser
{
public:
	TestFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
		int numChoices, char **choices, const int *attributes,
		int boxWidth, int boxDepth, int columnWidth, int current,
		void (*formatItem)(int, char*), char *speedString,
		int (*speedPrompt)(int, int, int, char *, int), int (*checkKey)(int, int))
		: AbstractFullScreenChooser(options, heading, heading2, instructions,
			numChoices, choices, attributes,
			boxWidth, boxDepth, columnWidth, current,
			formatItem, speedString,
			speedPrompt, checkKey),
		_keyStrokes(),
		_keyStrokeCount(0),
		_driver_put_string_args()
	{
	}
	virtual ~TestFullScreenChooser()
	{
	}

	void SetKeyStrokes(int numKeys, const int *keys)
	{
		std::copy(&keys[0], &keys[numKeys], std::back_inserter<std::vector<int> >(_keyStrokes));
	}

	struct driver_put_string_arg
	{
		driver_put_string_arg(int row_, int col_, int attr_, const char *msg_)
			: row(row_), col(col_), attr(attr_), msg(msg_)
		{
		}
		int row;
		int col;
		int attr;
		std::string msg;
	};
	const std::vector<driver_put_string_arg> &driver_put_string_args() const { return _driver_put_string_args; }

protected:
	virtual void help_title()
	{
	}
	virtual void driver_set_attr(int row, int col, int attr, int count)
	{
	}
	std::vector<driver_put_string_arg> _driver_put_string_args;
	virtual void driver_put_string(int row, int col, int attr, const char *msg)
	{
		_driver_put_string_args.push_back(driver_put_string_arg(row, col, attr, msg));
	}
	virtual void driver_hide_text_cursor()
	{
	}
	virtual void driver_buzzer(int kind)
	{
	}
	virtual int driver_key_pressed()
	{
		return _keyStrokes.size() > 0 ? _keyStrokes[_keyStrokeCount] : 0;
	}
	virtual int driver_get_key()
	{
		return (_keyStrokeCount < _keyStrokes.size()) ? _keyStrokes[_keyStrokeCount++] : 0;
	}
	virtual int getakeynohelp()
	{
		return (_keyStrokeCount < _keyStrokes.size()) ? _keyStrokes[_keyStrokeCount++] : 0;
	}
	virtual void blank_rows(int row, int rows, int attr)
	{
	}
	virtual void driver_unstack_screen()
	{
	}

private:
	std::vector<int> _keyStrokes;
	std::vector<int>::size_type _keyStrokeCount;
};

TEST(NoChoices, FullScreenChooser)
{
	TestFullScreenChooser chooser(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	int result = chooser.Execute();
	CHECK_EQUAL(-1, result);
}

std::ostream &operator<<(std::ostream &stream, const TestFullScreenChooser::driver_put_string_arg &arg)
{
	return stream << boost::format("{ %02d,%02d 0x%04X '%s' }\n") % arg.col % arg.row % arg.attr % arg.msg;
}

TEST(Escape, FullScreenChooser)
{
	char *choices[] = { "Choice" };
	int attributes[] = { 0 };
	TestFullScreenChooser chooser(0, 0, 0, 0, 1, choices, attributes, 0, 0, 0, 0, 0, 0, 0, 0);
	int keyStrokes[] = { FIK_ESC };
	chooser.SetKeyStrokes(NUM_OF(keyStrokes), keyStrokes);
	int result = chooser.Execute();
	CHECK_EQUAL(-1, result);
	std::copy(chooser.driver_put_string_args().begin(), chooser.driver_put_string_args().end(),
		std::ostream_iterator<TestFullScreenChooser::driver_put_string_arg>(std::cout));
}
