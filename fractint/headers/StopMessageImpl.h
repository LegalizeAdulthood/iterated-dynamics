#pragma once

#include "drivers.h"
#include "Externals.h"
#include "StopMessage.h"

class IStopMessageApp
{
public:
	virtual ~IStopMessageApp() {}

	virtual void InitializationFailure(std::string const &message) = 0;
	virtual void GoodBye() = 0;
	virtual int get_key_no_help() = 0;
};

class StopMessage : public IStopMessage
{
public:
	StopMessage(IStopMessageApp &app, AbstractDriver *driver, Externals &externs)
		: _app(app),
		_driver(driver),
		_externs(externs)
	{
	}
	virtual ~StopMessage() { }

	virtual int Execute(int flags, const std::string &msg);

private:
	void blank_rows(int row, int rows, int attr)
	{
		char buf[81];
		std::fill(buf, buf + 80, ' ');
		buf[80] = 0;
		while (--rows >= 0)
		{
			_driver->put_string(row++, 0, attr, buf);
		}
	}

	IStopMessageApp &_app;
	AbstractDriver *_driver;
	Externals &_externs;
};
