// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::ui
{

extern bool g_tab_enabled; // tab display enabled

std::string perturbation_status_text(int reference_count);
int tab_display();

} // namespace id::ui
