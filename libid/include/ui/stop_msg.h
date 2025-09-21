// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::ui
{

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

inline int operator+(const StopMsgFlags value)
{
    return static_cast<int>(value);
}
inline StopMsgFlags operator|(const StopMsgFlags lhs, const StopMsgFlags rhs)
{
    return static_cast<StopMsgFlags>(+lhs | +rhs);
}
inline bool bit_set(const StopMsgFlags flags, const StopMsgFlags bit)
{
    return (+flags & +bit) == +bit;
}

bool stop_msg(StopMsgFlags flags, const std::string &msg);

// the most common case
inline bool stop_msg(const std::string &msg)
{
    return stop_msg(StopMsgFlags::NONE, msg);
}

} // namespace id::ui
