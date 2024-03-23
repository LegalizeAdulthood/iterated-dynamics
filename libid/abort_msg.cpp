#include "abort_msg.h"

#include "stop_msg.h"

#include <cstdio>

bool abortmsg(char const *file, unsigned int line, int flags, char const *msg)
{
    char buffer[3*80];
    std::sprintf(buffer, "%s(%u):\n%s", file, line, msg);
    return stopmsg(flags, buffer);
}
