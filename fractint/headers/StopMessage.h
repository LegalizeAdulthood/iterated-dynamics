#if !defined(STOP_MESSAGE_H)
#define STOP_MESSAGE_H

#include <string>

// stop_message() flags
enum StopMessageFlag
{
	STOPMSG_NORMAL		= 0,
	STOPMSG_NO_STACK	= 1,
	STOPMSG_CANCEL		= 2,
	STOPMSG_NO_BUZZER	= 4,
	STOPMSG_FIXED_FONT	= 8,
	STOPMSG_INFO_ONLY	= 16
};

class IStopMessage
{
public:
	virtual ~IStopMessage() { }

	virtual int Execute(int flags, const std::string &msg) = 0;
};

extern int stop_message(int flags, const std::string &message);

#endif
