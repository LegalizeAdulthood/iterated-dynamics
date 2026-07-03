// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/command_result.h"
#include "ui/main_state.h"

#include <functional>

namespace id::ui
{

struct MenuHandler
{
    int key;
    std::function<CommandResult(MainContext &context)> handler;
};

inline bool operator<(const MenuHandler &lhs, const MenuHandler &rhs)
{
    return lhs.key < rhs.key;
}

inline bool operator<(const MenuHandler &lhs, const int key)
{
    return lhs.key < key;
}

inline bool operator==(const MenuHandler &lhs, const MenuHandler &rhs)
{
    return lhs.key == rhs.key;
}
inline bool operator!=(const MenuHandler &lhs, const MenuHandler &rhs)
{
    return !(lhs == rhs);
}

CommandResult request_fractal_type(MainContext &context);
CommandResult toggle_float(MainContext &context);
CommandResult get_history(int kbd_char);
CommandResult color_cycle(MainContext &context);
CommandResult color_editing(MainContext &context);
CommandResult restore_from_image(MainContext &context);
CommandResult requested_video_fn(MainContext &context);
CommandResult request_restart(MainContext &context);

} // namespace id::ui
