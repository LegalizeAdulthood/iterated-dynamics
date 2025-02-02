// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// stop_msg() flags
enum class StopMsgFlags
{
    NONE        = 0,
    NO_STACK    = 1,
    CANCEL      = 2,
    NO_BUZZER   = 4,
    FIXED_FONT  = 8,
    INFO_ONLY   = 16
};

inline int operator+(StopMsgFlags value)
{
    return static_cast<int>(value);
}
inline StopMsgFlags operator|(StopMsgFlags lhs, StopMsgFlags rhs)
{
    return static_cast<StopMsgFlags>(+lhs | +rhs);
}
inline bool bit_set(StopMsgFlags flags, StopMsgFlags bit)
{
    return (+flags & +bit) == +bit;
}

bool stop_msg(StopMsgFlags flags, const std::string &msg);

// the most common case
inline bool stop_msg(const std::string &msg)
{
    return stop_msg(StopMsgFlags::NONE, msg);
}
