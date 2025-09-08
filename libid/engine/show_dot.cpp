// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/show_dot.h"

namespace id::engine
{

AutoShowDot g_auto_show_dot{};                            // dark, medium, bright
int g_show_dot{-1};                                       // color to show crawling graphics cursor
int g_size_dot{};                                         // size of dot crawling cursor

} // namespace id::engine
