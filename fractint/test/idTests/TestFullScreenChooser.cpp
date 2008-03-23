#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

//extern BYTE g_text_colors[];
#include "id.h"
#include "FullScreenChooser.h"
#include "FakeDriver.h"
#include "FakeExternals.h"

class FakeFullScreenChooserApp : public IFullScreenChooserApp
{
public:
	virtual ~FakeFullScreenChooserApp() { }

	virtual int put_string_center(int row, int col, int width, int attr, const char *msg)
	{ throw not_implemented("put_string_center"); }
	virtual void help_title()
	{ throw not_implemented("help_title"); }
	virtual bool ends_with_slash(const char *text)
	{ throw not_implemented("ends_with_slash"); }
};

BOOST_AUTO_TEST_CASE(FullScreenChooser_NoChoices)
{
	FakeFullScreenChooserApp app;
	FakeExternals externs;
	FakeDriver fakeDriver;
	AbstractFullScreenChooser chooser(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		app, externs, &fakeDriver);
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
	FakeFullScreenChooserApp app;
	FakeExternals externs;
	FakeDriver fakeDriver;
	AbstractFullScreenChooser chooser(0, 0, 0, 0, 1, choices, attributes, 0, 0, 0, 0, 0, 0, 0, 0,
		app, externs, &fakeDriver);
	int keyStrokes[] = { IDK_ESC };
	fakeDriver.SetKeyStrokes(NUM_OF(keyStrokes), keyStrokes);
	int result = chooser.Execute();
	BOOST_CHECK_EQUAL(-1, result);
}

