// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/text_screen.h"

int g_text_row{};   // for putstring(-1,...)
int g_text_col{};   // for putstring(..,-1,...)
int g_text_row_base{}; // g_text_row is relative to this
int g_text_col_base{}; // g_text_col is relative to this
