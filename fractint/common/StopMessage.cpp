#include <fstream>
#include <string>

#include "TextColors.h"
#include "StopMessageImpl.h"

int StopMessage::Execute(int flags, const std::string &msg)
{
	//const char *msg = message.c_str();
	int ret;
	int toprow;
	int color;
	static unsigned char batchmode = 0;
	if (_externs.DebugMode() || _externs.InitializeBatch() >= INITBATCH_NORMAL)
	{
		std::ofstream stream((_externs.WorkDirectory() / "stop_message.txt").string().c_str(),
			std::ios_base::out | ((_externs.InitializeBatch() == INITBATCH_NONE) ? 0 : std::ios_base::ate));
		if (stream)
		{
			stream << msg << "\n";
			stream.close();
		}
	}
	if (_externs.CommandInitialize())  // & command_files hasn't finished 1st try
	{
		_app.InitializationFailure(msg);
		_app.GoodBye();
	}
	if (_externs.InitializeBatch() >= INITBATCH_NORMAL || batchmode)  // in batch mode
	{
		_externs.SetInitializeBatch(INITBATCH_BAILOUT_INTERRUPTED); // used to set errorlevel
		batchmode = 1; // fixes *second* stop_message in batch mode bug
		return -1;
	}
	ret = 0;
	MouseModeSaver saved_mouse(_driver, -IDK_ENTER);
	if ((flags & STOPMSG_NO_STACK))
	{
		blank_rows(toprow = 12, 10, 7);
	}
	else
	{
		_driver->stack_screen();
		toprow = 4;
		_driver->move_cursor(4, 0);
	}
	_externs.SetTextCbase(2); // left margin is 2
	_driver->put_string(toprow, 0, 7, msg);
	if (flags & STOPMSG_CANCEL)
	{
		_driver->put_string(_externs.TextRow() + 2, 0, 7, "Escape to cancel, any other key to continue...");
	}
	else
	{
		_driver->put_string(_externs.TextRow() + 2, 0, 7, "Any key to continue...");
	}
	_externs.SetTextCbase(0);			// back to full line
	color = (flags & STOPMSG_INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
	_driver->set_attr(toprow, 0, color, (_externs.TextRow() + 1-toprow)*80);
	_driver->hide_text_cursor();			// cursor off
	if ((flags & STOPMSG_NO_BUZZER) == 0)
	{
		_driver->buzzer((flags & STOPMSG_INFO_ONLY) ? 0 : 2);
	}
	while (_driver->key_pressed()) // flush any keyahead
	{
		_driver->get_key();
	}
	if (_externs.DebugMode() != DEBUGMODE_NO_HELP_F1_ESC)
	{
		if (_app.get_key_no_help() == IDK_ESC)
		{
			ret = -1;
		}
	}
	if (flags & STOPMSG_NO_STACK)
	{
		blank_rows(toprow, 10, 7);
	}
	else
	{
		_driver->unstack_screen();
	}
	return ret;
}
