#pragma once

#include <string>

// stopmsg() flags
enum stopmsg_flags
{
    STOPMSG_NONE        = 0,
    STOPMSG_NO_STACK    = 1,
    STOPMSG_CANCEL      = 2,
    STOPMSG_NO_BUZZER   = 4,
    STOPMSG_FIXED_FONT  = 8,
    STOPMSG_INFO_ONLY   = 16
};

bool stopmsg(int flags, char const* msg);
inline bool stopmsg(int flags, const std::string &msg)
{
    return stopmsg(flags, msg.c_str());
}
