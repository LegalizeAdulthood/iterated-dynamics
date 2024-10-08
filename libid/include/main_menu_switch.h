// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "main_state.h"

#include <functional>

struct MainMenuHandler
{
    int key;
    std::function<main_state(int &key, bool &from_mandel, bool &kbd_more, bool &stacked)> handler;
};

main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
main_state request_fractal_type(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state toggle_float(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state get_history(int kbd_char);
main_state color_cycle(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state color_editing(int &key, bool &from_mandel, bool &kbd_more, bool &stacked);
main_state restore_from_image(int &kbd_char, bool &from_mandel, bool &kbd_more, bool &stacked);
bool requested_video_fn(bool &kbd_more, int kbd_char);
