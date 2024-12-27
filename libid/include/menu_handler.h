// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "main_state.h"

#include <functional>

struct MenuHandler
{
    int key;
    std::function<main_state(MainContext &context)> handler;
};

inline bool operator<(const MenuHandler &lhs, const MenuHandler &rhs)
{
    return lhs.key < rhs.key;
}

inline bool operator<(const MenuHandler &lhs, int key)
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

main_state request_fractal_type(MainContext &context);
main_state toggle_float(MainContext &context);
main_state get_history(int kbd_char);
main_state color_cycle(MainContext &context);
main_state color_editing(MainContext &context);
main_state restore_from_image(MainContext &context);
main_state requested_video_fn(MainContext &context);
main_state request_restart(MainContext &context);
