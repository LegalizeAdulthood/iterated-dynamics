#pragma once
#include "stop_msg.h"

bool abortmsg(char const *file, unsigned int line, stopmsg_flags flags, char const *msg);

#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
