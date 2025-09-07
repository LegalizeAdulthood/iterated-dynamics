// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/main_state.h"

namespace id::ui
{

struct EvolutionInfo;

extern id::ui::EvolutionInfo g_evolve_info;
extern int                   g_finish_row;
extern bool                  g_from_text;
extern bool                  g_have_evolve_info;
extern void                (*g_out_line_cleanup)();
extern bool                  g_virtual_screens;

MainState big_while_loop(bool &kbd_more, bool &stacked, bool resume_flag);
MainState big_while_loop(MainContext &context);

int key_count(int key);

} // namespace id::ui
