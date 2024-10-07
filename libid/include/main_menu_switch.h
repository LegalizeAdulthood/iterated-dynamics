// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "main_state.h"

main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
bool request_fractal_type(bool &from_mandel);
