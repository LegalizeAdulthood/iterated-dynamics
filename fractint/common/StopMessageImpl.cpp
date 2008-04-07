#include <string>
#include "prototyp.h"
#include "prompts2.h"
#include "StopMessageImpl.h"

class StopMessageApp : public IStopMessageApp
{
public:
	virtual ~StopMessageApp() { }

	virtual void InitializationFailure(std::string const &message)
	{ return ::init_failure(message); }
	virtual void GoodBye()
	{ ::goodbye(); }
	virtual int get_key_no_help()
	{ return ::get_key_no_help(); }
};

/* int stop_message(flags, message) displays message and waits for a key:
	message should be a max of 9 lines with \n's separating them;
	no leading or trailing \n's in message;
	no line longer than 76 chars for best appearance;
	flag options:
		&1 if already in text display mode, stackscreen is not called
			and message is displayed at (12, 0) instead of (4, 0)
		&2 if continue/cancel indication is to be returned;
			when not set, "Any key to continue..." is displayed
			when set, "Escape to cancel, any other key to continue..."
			-1 is returned for cancel, 0 for continue
		&4 set to suppress buzzer
		&8 for a fixed pitch font
		&16 for info only message (green box instead of red in DOS vsn)
*/
int stop_message(int flags, const std::string &msg)
{
	StopMessageApp app;
	return StopMessage(app, DriverManager::current(), g_externs).Execute(flags, msg);
}
