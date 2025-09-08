// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/main_state.h"

#include <functional>

struct MenuHandler
{
    int key;
    std::function<id::ui::MainState(id::ui::MainContext &context)> handler;
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

id::ui::MainState request_fractal_type(id::ui::MainContext &context);
id::ui::MainState toggle_float(id::ui::MainContext &context);
id::ui::MainState get_history(int kbd_char);
id::ui::MainState color_cycle(id::ui::MainContext &context);
id::ui::MainState color_editing(id::ui::MainContext &context);
id::ui::MainState restore_from_image(id::ui::MainContext &context);
id::ui::MainState requested_video_fn(id::ui::MainContext &context);
id::ui::MainState request_restart(id::ui::MainContext &context);
