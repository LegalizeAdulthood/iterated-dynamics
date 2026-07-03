// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/command_result.h"
#include "ui/main_state.h"

namespace id::ui
{

CommandResult main_menu_switch(MainContext &context);
void resume_orbits_window(MainContext &context);

} // namespace id::ui
