// SPDX-License-Identifier: GPL-3.0-only
//
#include "mouse.h"

// g_look_at_mouse:
//     0  ignore the mouse entirely
//    <0  only test for left button click; if it occurs return fake key number of opposite sign
//     1  return <Enter> key for left button, arrow keys for mouse movement,
//        mouse sensitivity is suitable for graphics cursor
//     2  same as 1 but sensitivity is suitable for text cursor
//     3  specials for zoom box, left/right double-clicks generate fake
//        keys, mouse movement generates a variety of fake keys
//        depending on state of buttons
//
int g_look_at_mouse{};          //
bool g_cursor_mouse_tracking{}; //
