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
	FakeFullScreenChooserApp() : _helpTitleCalled(false),
		_putStringCenterCalled(false),
		_putStringCenterLastRow(0),
		_putStringCenterLastColumn(0),
		_putStringCenterLastWidth(0),
		_putStringCenterLastAttr(0),
		_putStringCenterLastMessage(),
		_putStringCenterFakeResult(0)
	{ }
	virtual ~FakeFullScreenChooserApp() { }

	virtual int put_string_center(int row, int col, int width, int attr, const char *msg)
	{
		_putStringCenterCalled = true;
		_putStringCenterLastRow = row;
		_putStringCenterLastColumn = col;
		_putStringCenterLastWidth = width;
		_putStringCenterLastAttr = attr;
		_putStringCenterLastMessage = msg;
		return _putStringCenterFakeResult;
	}
	void SetPutStringCenterFakeResult(int value)	{ _putStringCenterFakeResult = value; }
	bool PutStringCenterCalled() const				{ return _putStringCenterCalled; }
	int PutStringCenterLastRow() const				{ return _putStringCenterLastRow; }
	int PutStringCenterLastColumn() const			{ return _putStringCenterLastColumn; }
	int PutStringCenterLastWidth() const			{ return _putStringCenterLastWidth; }
	int PutStringCenterLastAttr() const				{ return _putStringCenterLastAttr; }
	const std::string &PutStringCenterLastMessage() const { return _putStringCenterLastMessage; }
	virtual void help_title()
	{ _helpTitleCalled = true; }
	bool HelpTitleCalled() const { return _helpTitleCalled; }
	virtual bool ends_with_slash(const char *text)
	{ throw not_implemented("ends_with_slash"); }

private:
	bool _helpTitleCalled;
	bool _putStringCenterCalled;
	int _putStringCenterLastRow;
	int _putStringCenterLastColumn;
	int _putStringCenterLastWidth;
	int _putStringCenterLastAttr;
	std::string _putStringCenterLastMessage;
	int _putStringCenterFakeResult;
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
	BOOST_CHECK(app.HelpTitleCalled());
}

