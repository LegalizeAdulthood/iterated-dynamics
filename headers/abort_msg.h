#pragma once

bool abortmsg(char const *file, unsigned int line, int flags, char const *msg);

#define ABORT(flags_, msg_) abortmsg(__FILE__, __LINE__, flags_, msg_)
