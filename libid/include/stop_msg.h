#pragma once

#include <string>

// stopmsg() flags
enum class stopmsg_flags
{
    NONE        = 0,
    NO_STACK    = 1,
    CANCEL      = 2,
    NO_BUZZER   = 4,
    FIXED_FONT  = 8,
    INFO_ONLY   = 16
};

inline int operator+(stopmsg_flags value)
{
    return static_cast<int>(value);
}
inline stopmsg_flags operator|(stopmsg_flags lhs, stopmsg_flags rhs)
{
    return static_cast<stopmsg_flags>(+lhs | +rhs);
}
inline bool bit_set(stopmsg_flags flags, stopmsg_flags bit)
{
    return (+flags & +bit) == +bit;
}

bool stopmsg(stopmsg_flags flags, const std::string &msg);

// the most common case
inline bool stopmsg(const std::string &msg)
{
    return stopmsg(stopmsg_flags::NONE, msg);
}
