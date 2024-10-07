// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "main_state.h"

main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
bool request_fractal_type(bool &from_mandel);
void toggle_float();
main_state get_history(int kbd_char);
void color_cycle(int kbd_char);
bool color_editing(bool &kbd_more);
void restore_from_image(bool &from_mandel, int kbd_char, bool &stacked);
bool requested_video_fn(bool &kbd_more, int kbd_char);
