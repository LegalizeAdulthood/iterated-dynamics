// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "main_state.h"

#include <functional>

struct MenuHandler
{
    int key;
    std::function<MainState(MainContext &context)> handler;
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

MainState request_fractal_type(MainContext &context);
MainState toggle_float(MainContext &context);
MainState get_history(int kbd_char);
MainState color_cycle(MainContext &context);
MainState color_editing(MainContext &context);
MainState restore_from_image(MainContext &context);
MainState requested_video_fn(MainContext &context);
MainState request_restart(MainContext &context);
