// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/main_state.h"

namespace id::ui
{

struct EvolutionInfo;

extern bool                  g_ask_video;               // flag for video prompting
extern bool                  g_compare_gif;             // compare two gif files flag
extern EvolutionInfo         g_evolve_info;             //
extern int                   g_finish_row;              // save when this row is finished
extern bool                  g_from_text;               // = true if we're in graphics mode
extern bool                  g_have_evolve_info;        //
extern void                (*g_out_line_cleanup)();     //
extern bool                  g_virtual_screens;         //

MainState big_while_loop(bool &kbd_more, bool &stacked, bool resume_flag);
MainState big_while_loop(MainContext &context);

int key_count(int key);

} // namespace id::ui
