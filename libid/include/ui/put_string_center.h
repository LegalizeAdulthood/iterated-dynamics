// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::ui
{

int put_string_center(int row, int col, int width, int attr, const char *msg);

inline int put_string_center(
    const int row, const int col, const int width, const int attr, const std::string &msg)
{
    return put_string_center(row, col, width, attr, msg.c_str());
}

} // namespace id::ui
