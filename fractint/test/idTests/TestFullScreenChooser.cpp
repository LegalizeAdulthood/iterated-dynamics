#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <vector>

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
		_keyStrokeCount(0)
	{
	}
	virtual ~TestFullScreenChooser()
	{
	}

	void SetKeyStrokes(int numKeys, const int *keys)
	{
		std::copy(&keys[0], &keys[numKeys], std::back_inserter<std::vector<int> >(_keyStrokes));
	}

protected:
	virtual void help_title()
	{
	}
	virtual void driver_set_attr(int row, int col, int attr, int count)
	{
	}
	virtual void driver_put_string(int row, int col, int attr, const char *msg)
	{
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

TEST(Escape, FullScreenChooser)
{
	char *choices[] = { "Choice" };
	int attributes[] = { 0 };
	TestFullScreenChooser chooser(0, 0, 0, 0, 1, choices, attributes, 0, 0, 0, 0, 0, 0, 0, 0);
	int keyStrokes[] = { FIK_ESC };
	chooser.SetKeyStrokes(NUM_OF(keyStrokes), keyStrokes);
	int result = chooser.Execute();
	CHECK_EQUAL(-1, result);
}
