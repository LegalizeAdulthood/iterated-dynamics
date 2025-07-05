// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

bool thinking(int options, const char *msg);

inline bool thinking(const char *msg)
{
    return thinking(1, msg);
}

inline void thinking_end()
{
    thinking(0, nullptr);
}
