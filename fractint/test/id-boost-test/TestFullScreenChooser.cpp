#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

extern BYTE g_text_colors[];
#include "id.h"
#include "FullScreenChooser.h"
#include "FakeDriver.h"

class FullScreenChooserTester : public AbstractFullScreenChooser
{
public:
	FullScreenChooserTester(int options, const char *heading, const char *heading2, const char *instructions,
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
	virtual ~FullScreenChooserTester()
	{
	}

protected:
	virtual void help_title()
	{
	}
	virtual void blank_rows(int row, int rows, int attr)
	{
	}
};

BOOST_AUTO_TEST_CASE(FullScreenChooser_NoChoices)
{
	FakeDriver fakeDriver;
	FullScreenChooserTester chooser(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &fakeDriver);
	int result = chooser.Execute();
	BOOST_CHECK_EQUAL(-1, result);
}

//std::ostream &operator<<(std::ostream &stream, const FakeDriver::PutStringArg &arg)
//{
//	return stream << boost::format("{ %02d,%02d 0x%04X '%s' }\n") % arg.col % arg.row % arg.attr % arg.msg;
//}

BOOST_AUTO_TEST_CASE(FullScreenChooser_Escape)
{
	char *choices[] = { "Choice" };
	int attributes[] = { 0 };
	FakeDriver fakeDriver;
	FullScreenChooserTester chooser(0, 0, 0, 0, 1, choices, attributes, 0, 0, 0, 0, 0, 0, 0, 0, &fakeDriver);
	int keyStrokes[] = { IDK_ESC };
	fakeDriver.SetKeyStrokes(NUM_OF(keyStrokes), keyStrokes);
	int result = chooser.Execute();
	BOOST_CHECK_EQUAL(-1, result);
}

