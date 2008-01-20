#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/format.hpp>

extern BYTE g_text_colors[];
#include "id.h"
#include "FullScreenChooser.h"
#include "FakeDriver.h"

class TestFullScreenChooser : public AbstractFullScreenChooser
{
public:
	TestFullScreenChooser(int options, const char *heading, const char *heading2, const char *instructions,
		int numChoices, char **choices, const int *attributes,
		int boxWidth, int boxDepth, int columnWidth, int current,
		void (*formatItem)(int, char*), char *speedString,
		int (*speedPrompt)(int, int, int, char *, int), int (*checkKey)(int, int),
		AbstractDriver *driver)
		: AbstractFullScreenChooser(options, heading, heading2, instructions,
			numChoices, choices, attributes,
			boxWidth, boxDepth, columnWidth, current,
			formatItem, speedString,
			speedPrompt, checkKey, driver)
	{
	}
	virtual ~TestFullScreenChooser()
	{
	}

protected:
	virtual void help_title()
	{
	}
	virtual int get_key_no_help()
	{
		return driver_get_key();
	}
	virtual void blank_rows(int row, int rows, int attr)
	{
	}
};

TEST(FullScreenChooser, NoChoices)
{
	FakeDriver fakeDriver;
	TestFullScreenChooser chooser(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &fakeDriver);
	int result = chooser.Execute();
	CHECK_EQUAL(-1, result);
}

std::ostream &operator<<(std::ostream &stream, const FakeDriver::PutStringArg &arg)
{
	return stream << boost::format("{ %02d,%02d 0x%04X '%s' }\n") % arg.col % arg.row % arg.attr % arg.msg;
}

TEST(FullScreenChooser, Escape)
{
	char *choices[] = { "Choice" };
	int attributes[] = { 0 };
	FakeDriver fakeDriver;
	TestFullScreenChooser chooser(0, 0, 0, 0, 1, choices, attributes, 0, 0, 0, 0, 0, 0, 0, 0, &fakeDriver);
	int keyStrokes[] = { FIK_ESC };
	fakeDriver.SetKeyStrokes(NUM_OF(keyStrokes), keyStrokes);
	int result = chooser.Execute();
	CHECK_EQUAL(-1, result);
}

